/*
 * XGL
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "shader.h"
#include "pipeline_priv.h"
#include "compiler/pipeline/pipeline_compiler_interface.h"

static struct intel_pipeline_rmap_slot *rmap_get_slot(struct intel_pipeline_rmap *rmap,
                                                      XGL_DESCRIPTOR_SET_SLOT_TYPE type,
                                                      XGL_UINT index)
{
    const XGL_UINT resource_offset = rmap->rt_count;
    const XGL_UINT uav_offset = resource_offset + rmap->resource_count;
    const XGL_UINT sampler_offset = uav_offset + rmap->uav_count;
    const XGL_UINT ve_offset = sampler_offset + rmap->sampler_count;
    struct intel_pipeline_rmap_slot *slot;

    switch (type) {
    case XGL_SLOT_UNUSED:
        slot = NULL;
        break;
    case XGL_SLOT_SHADER_RESOURCE:
        slot = &rmap->slots[resource_offset + index];
        break;
    case XGL_SLOT_SHADER_UAV:
        slot = &rmap->slots[uav_offset + index];
        break;
    case XGL_SLOT_SHADER_SAMPLER:
        slot = &rmap->slots[sampler_offset + index];
        break;
    case XGL_SLOT_VERTEX_INPUT:
        slot = &rmap->slots[ve_offset + index];
        break;
    default:
        assert(!"unknown rmap slot type");
        slot = NULL;
        break;
    }

    return slot;
}

static bool rmap_init_slots_with_path(struct intel_pipeline_rmap *rmap,
                                      const XGL_DESCRIPTOR_SET_MAPPING *mapping,
                                      XGL_UINT *nest_path,
                                      XGL_UINT nest_level)
{
    XGL_UINT i;

    for (i = 0; i < mapping->descriptorCount; i++) {
        const XGL_DESCRIPTOR_SLOT_INFO *info = &mapping->pDescriptorInfo[i];
        struct intel_pipeline_rmap_slot *slot;

        if (info->slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET) {
            nest_path[nest_level] = i;
            if (!rmap_init_slots_with_path(rmap, info->pNextLevelSet,
                        nest_path, nest_level + 1))
                return false;

            continue;
        }

        slot = rmap_get_slot(rmap, info->slotObjectType,
                info->shaderEntityIndex);
        if (!slot)
            continue;

        assert(!slot->path_len);
        slot->path_len = nest_level + 1;

        if (nest_level) {
            slot->u.path = icd_alloc(sizeof(slot->u.path[0]) *
                    slot->path_len, 0, XGL_SYSTEM_ALLOC_INTERNAL);
            if (!slot->u.path) {
                slot->path_len = 0;
                return false;
            }

            memcpy(slot->u.path, nest_path,
                    sizeof(slot->u.path[0]) * nest_level);
            slot->u.path[nest_level] = i;
        } else {
            slot->u.index = i;
        }
    }

    return true;
}

static bool rmap_init_slots(struct intel_pipeline_rmap *rmap,
                            const XGL_DESCRIPTOR_SET_MAPPING *mapping,
                            XGL_UINT depth)
{
    XGL_UINT *nest_path;
    bool ok;

    if (depth) {
        nest_path = icd_alloc(sizeof(nest_path[0]) * depth,
                0, XGL_SYSTEM_ALLOC_INTERNAL_TEMP);
        if (!nest_path)
            return false;
    } else {
        nest_path = NULL;
    }

    ok = rmap_init_slots_with_path(rmap, mapping, nest_path, 0);

    if (nest_path)
        icd_free(nest_path);

    return ok;
}

static void rmap_update_count(struct intel_pipeline_rmap *rmap,
                              XGL_DESCRIPTOR_SET_SLOT_TYPE type,
                              XGL_UINT index)
{
    switch (type) {
    case XGL_SLOT_UNUSED:
        break;
    case XGL_SLOT_SHADER_RESOURCE:
        if (rmap->resource_count < index + 1)
            rmap->resource_count = index + 1;
        break;
    case XGL_SLOT_SHADER_UAV:
        if (rmap->uav_count < index + 1)
            rmap->uav_count = index + 1;
        break;
    case XGL_SLOT_SHADER_SAMPLER:
        if (rmap->sampler_count < index + 1)
            rmap->sampler_count = index + 1;
        break;
    case XGL_SLOT_VERTEX_INPUT:
        if (rmap->vb_count < index + 1)
            rmap->vb_count = index + 1;
        break;
    default:
        assert(!"unknown rmap slot type");
        break;
    }
}

static XGL_UINT rmap_init_counts(struct intel_pipeline_rmap *rmap,
                                 const XGL_DESCRIPTOR_SET_MAPPING *mapping)
{
    XGL_UINT depth = 0;
    XGL_UINT i;

    for (i = 0; i < mapping->descriptorCount; i++) {
        const XGL_DESCRIPTOR_SLOT_INFO *info = &mapping->pDescriptorInfo[i];

        if (info->slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET) {
            const XGL_UINT d = rmap_init_counts(rmap,
                    info->pNextLevelSet);
            if (depth < d + 1)
                depth = d + 1;

            continue;
        }

        rmap_update_count(rmap, info->slotObjectType,
                info->shaderEntityIndex);
    }

    return depth;
}

static void rmap_destroy(struct intel_pipeline_rmap *rmap)
{
    XGL_UINT i;

    for (i = 0; i < rmap->slot_count; i++) {
        struct intel_pipeline_rmap_slot *slot = &rmap->slots[i];

        switch (slot->path_len) {
        case 0:
        case 1:
        case INTEL_PIPELINE_RMAP_SLOT_RT:
        case INTEL_PIPELINE_RMAP_SLOT_DYN:
            break;
        default:
            icd_free(slot->u.path);
            break;
        }
    }

    icd_free(rmap->slots);
    icd_free(rmap);
}

static struct intel_pipeline_rmap *rmap_create(struct intel_dev *dev,
                                               const XGL_DESCRIPTOR_SET_MAPPING *mapping,
                                               const XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO *dyn,
                                               XGL_UINT rt_count)
{
    struct intel_pipeline_rmap *rmap;
    struct intel_pipeline_rmap_slot *slot;
    XGL_UINT depth, rt;

    rmap = icd_alloc(sizeof(*rmap), 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!rmap)
        return NULL;

    memset(rmap, 0, sizeof(*rmap));

    depth = rmap_init_counts(rmap, mapping);

    /* add RTs and the dynamic memory view */
    rmap_update_count(rmap, dyn->slotObjectType, dyn->shaderEntityIndex);
    rmap->rt_count = rt_count;

    rmap->slot_count = rmap->rt_count + rmap->resource_count +
        rmap->uav_count + rmap->sampler_count + rmap->vb_count;

    rmap->slots = icd_alloc(sizeof(rmap->slots[0]) * rmap->slot_count,
            0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!rmap->slots) {
        icd_free(rmap);
        return NULL;
    }

    memset(rmap->slots, 0, sizeof(rmap->slots[0]) * rmap->slot_count);

    if (!rmap_init_slots(rmap, mapping, depth)) {
        rmap_destroy(rmap);
        return NULL;
    }

    /* add RTs and the dynamic memory view */
    slot = rmap_get_slot(rmap, dyn->slotObjectType, dyn->shaderEntityIndex);
    if (slot) {
        slot->path_len = INTEL_PIPELINE_RMAP_SLOT_DYN;
        slot->u.index = 0;
    }
    for (rt = 0; rt < rmap->rt_count; rt++) {
        slot = &rmap->slots[rt];
        slot->path_len = INTEL_PIPELINE_RMAP_SLOT_RT;
        slot->u.index = rt;
    }

    return rmap;
}

static XGL_RESULT pipeline_shader_copy_ir(struct intel_pipeline_shader *sh,
                                          const struct intel_shader *ir)
{


    sh->pCode = icd_alloc(ir->ir->size, 0, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
    if (!sh->pCode)
        return XGL_ERROR_OUT_OF_MEMORY;

    memcpy(sh->pCode, ir->ir->kernel, ir->ir->size);
    sh->codeSize = ir->ir->size;

    sh->uses = ir->uses;

    sh->in_count = ir->in_count;
    sh->out_count = ir->out_count;
    sh->sampler_count = ir->sampler_count;
    sh->surface_count = ir->surface_count;
    sh->urb_grf_start = ir->urb_grf_start;
    sh->barycentric_interps = ir->barycentric_interps;

    return XGL_SUCCESS;
}

static XGL_RESULT pipeline_build_vs(struct intel_pipeline *pipeline,
                                    const struct intel_pipeline_create_info *info)
{
    struct intel_pipeline_shader *vs = &pipeline->vs;
    XGL_RESULT ret;

    ret = pipeline_shader_copy_ir(vs, intel_shader(info->vs.shader));
    if (ret != XGL_SUCCESS)
        return ret;

    assert(!info->vs.linkConstBufferCount);

    vs->rmap = rmap_create(pipeline->dev,
            &info->vs.descriptorSetMapping[0],
            &info->vs.dynamicMemoryViewMapping, 0);
    if (!vs->rmap) {
        icd_free(vs->pCode);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    pipeline->active_shaders |= SHADER_VERTEX_FLAG;

    return XGL_SUCCESS;
}

static XGL_RESULT pipeline_build_tcs(struct intel_pipeline *pipeline,
                                     const struct intel_pipeline_create_info *info)
{
    struct intel_pipeline_shader *tcs = &pipeline->tcs;
    XGL_RESULT ret;

    ret = pipeline_shader_copy_ir(tcs, intel_shader(info->tcs.shader));
    if (ret != XGL_SUCCESS)
        return ret;

    assert(!info->tcs.linkConstBufferCount);

    pipeline->active_shaders |= SHADER_TESS_CONTROL_FLAG;

    return XGL_SUCCESS;
}

static XGL_RESULT pipeline_build_tes(struct intel_pipeline *pipeline,
                                     const struct intel_pipeline_create_info *info)
{
    struct intel_pipeline_shader *tes = &pipeline->tes;
    XGL_RESULT ret;

    ret = pipeline_shader_copy_ir(tes, intel_shader(info->tes.shader));
    if (ret != XGL_SUCCESS)
        return ret;

    assert(!info->tes.linkConstBufferCount);

    pipeline->active_shaders |= SHADER_TESS_EVAL_FLAG;

    return XGL_SUCCESS;
}

static XGL_RESULT pipeline_build_gs(struct intel_pipeline *pipeline,
                                    const struct intel_pipeline_create_info *info)
{
    struct intel_pipeline_shader *gs = &pipeline->gs;
    XGL_RESULT ret;

    ret = pipeline_shader_copy_ir(gs, intel_shader(info->gs.shader));
    if (ret != XGL_SUCCESS)
        return ret;

    assert(!info->tes.linkConstBufferCount);

    pipeline->active_shaders |= SHADER_GEOMETRY_FLAG;

    return XGL_SUCCESS;
}

static XGL_RESULT pipeline_build_fs(struct intel_pipeline *pipeline,
                                    const struct intel_pipeline_create_info *info)
{
    struct intel_pipeline_shader *fs = &pipeline->fs;
    XGL_RESULT ret;

    ret = pipeline_shader_copy_ir(fs, intel_shader(info->fs.shader));
    if (ret != XGL_SUCCESS)
        return ret;

    assert(!info->fs.linkConstBufferCount);


    // Right here, lower the IR to ISA using NOS
    // This must be after assignment of pipeline constant
    // buffer, but before the ISA copy (which can eventually
    // go away)

    ret = intel_pipeline_shader_compile(fs, intel_shader(info->fs.shader));
    if (ret != XGL_SUCCESS)
        return ret;

    /* assuming one RT; need to parse the shader */
    fs->rmap = rmap_create(pipeline->dev,
            &info->fs.descriptorSetMapping[0],
            &info->fs.dynamicMemoryViewMapping, 1);
    if (!fs->rmap) {
        icd_free(fs->pCode);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    pipeline->active_shaders |= SHADER_FRAGMENT_FLAG;

    return XGL_SUCCESS;
}

static XGL_RESULT pipeline_build_cs(struct intel_pipeline *pipeline,
                                    const struct intel_pipeline_create_info *info)
{
    struct intel_pipeline_shader *cs = &pipeline->cs;
    XGL_RESULT ret;

    ret = pipeline_shader_copy_ir(cs, intel_shader(info->compute.cs.shader));
    if (ret != XGL_SUCCESS)
        return ret;

    assert(!info->compute.cs.linkConstBufferCount);

    pipeline->active_shaders |= SHADER_COMPUTE_FLAG;

    return XGL_SUCCESS;
}

XGL_RESULT pipeline_build_shaders(struct intel_pipeline *pipeline,
                                  const struct intel_pipeline_create_info *info)
{
    XGL_RESULT ret = XGL_SUCCESS;

    if (ret == XGL_SUCCESS && info->vs.shader)
        ret = pipeline_build_vs(pipeline, info);
    if (ret == XGL_SUCCESS && info->tcs.shader)
        ret = pipeline_build_tcs(pipeline, info);
    if (ret == XGL_SUCCESS && info->tes.shader)
        ret = pipeline_build_tes(pipeline, info);
    if (ret == XGL_SUCCESS && info->gs.shader)
        ret = pipeline_build_gs(pipeline, info);
    if (ret == XGL_SUCCESS && info->fs.shader)
        ret = pipeline_build_fs(pipeline, info);

    if (ret == XGL_SUCCESS && info->compute.cs.shader)
        ret = pipeline_build_cs(pipeline, info);

    return ret;
}

static void pipeline_tear_shader(struct intel_pipeline_shader *sh)
{
    icd_free(sh->pCode);
    if (sh->rmap)
        rmap_destroy(sh->rmap);
}

void pipeline_tear_shaders(struct intel_pipeline *pipeline)
{
    if (pipeline->active_shaders & SHADER_VERTEX_FLAG) {
        pipeline_tear_shader(&pipeline->vs);
    }

    if (pipeline->active_shaders & SHADER_TESS_CONTROL_FLAG) {
        pipeline_tear_shader(&pipeline->tcs);
    }

    if (pipeline->active_shaders & SHADER_TESS_EVAL_FLAG) {
        pipeline_tear_shader(&pipeline->tes);
    }

    if (pipeline->active_shaders & SHADER_GEOMETRY_FLAG) {
        pipeline_tear_shader(&pipeline->gs);
    }

    if (pipeline->active_shaders & SHADER_FRAGMENT_FLAG) {
        pipeline_tear_shader(&pipeline->fs);
    }

    if (pipeline->active_shaders & SHADER_COMPUTE_FLAG) {
        pipeline_tear_shader(&pipeline->cs);
    }
}

struct intel_pipeline_shader *intel_pipeline_shader_create_meta(struct intel_dev *dev,
                                                                enum intel_dev_meta_shader id)
{
    static const uint32_t gen6_clear_code[] = {
        0x00600001, 0x202003be, 0x00000040, 0x00000000, // mov(8)          m1<1>F          g2<0,1,0>F                      { align1 1Q };
        0x00600001, 0x204003be, 0x00000044, 0x00000000, // mov(8)          m2<1>F          g2.1<0,1,0>F                    { align1 1Q };
        0x00600001, 0x206003be, 0x00000048, 0x00000000, // mov(8)          m3<1>F          g2.2<0,1,0>F                    { align1 1Q };
        0x00600001, 0x208003be, 0x0000004c, 0x00000000, // mov(8)          m4<1>F          g2.3<0,1,0>F                    { align1 1Q };
        0x05600032, 0x20001fc8, 0x008d0020, 0x88019400, // sendc(8)        null            m1<8,8,1>F
                                                        // render RT write SIMD8 LastRT Surface = 0 mlen 4 rlen 0 { align1 1Q EOT };
    };
    static const uint32_t gen7_clear_code[] = {
        0x20010b01, 0x00027c00,                         // mov(8)          g124<1>F        g2<0,1,0>F                      { align1 1Q compacted };
        0x20150b01, 0x00027d00,                         // mov(8)          g125<1>F        g2.1<0,1,0>F                    { align1 1Q compacted };
        0x20190b01, 0x00027e00,                         // mov(8)          g126<1>F        g2.2<0,1,0>F                    { align1 1Q compacted };
        0x201d0b01, 0x00027f00,                         // mov(8)          g127<1>F        g2.3<0,1,0>F                    { align1 1Q compacted };
        0x05600032, 0x20001fa8, 0x008d0f80, 0x88031400, // sendc(8)        null            g124<8,8,1>F
                                                        // render RT write SIMD8 LastRT Surface = 0 mlen 4 rlen 0 { align1 1Q EOT };
    };
    static const uint32_t gen6_copy_mem_code[] = {
        0x00600040, 0x20a06d29, 0x00480028, 0x10101010, // add(8)          g5<1>UW         g1.4<2,4,0>UW   0x10101010V     { align1 1Q };
        0x00600001, 0x20a00062, 0x00000000, 0x00000000, // mov(8)          m5<1>UD         0x00000000UD                    { align1 1Q };
        0x00600001, 0x20c0013d, 0x008d00a0, 0x00000000, // mov(8)          g6<1>F          g5<8,8,1>UW                     { align1 1Q };
        0x00600040, 0x20607fbd, 0x008d00c0, 0x3f000000, // add(8)          g3<1>F          g6<8,8,1>F      0.5F            { align1 1Q };
        0x00600001, 0x204003a5, 0x008d0060, 0x00000000, // mov(8)          g2<1>D          g3<8,8,1>F                      { align1 1Q };
        0x00600040, 0x204014a6, 0x008d0040, 0x00000080, // add(8)          m2<1>D          g2<8,8,1>D      g4<0,1,0>D      { align1 1Q };
        0x02600031, 0x20401fc9, 0x008d0040, 0x08417001, // send(8)         g2<1>UW         m2<8,8,1>F
                                                        // sampler (1, 0, 7, 1) mlen 4 rlen 4              { align1 1Q };
        0x00600001, 0x202003be, 0x008d0040, 0x00000000, // mov(8)          m1<1>F          g2<8,8,1>F                      { align1 1Q };
        0x00600001, 0x204003be, 0x008d0060, 0x00000000, // mov(8)          m2<1>F          g3<8,8,1>F                      { align1 1Q };
        0x00600001, 0x206003be, 0x008d0080, 0x00000000, // mov(8)          m3<1>F          g4<8,8,1>F                      { align1 1Q };
        0x00600001, 0x208003be, 0x008d00a0, 0x00000000, // mov(8)          m4<1>F          g5<8,8,1>F                      { align1 1Q };
        0x05600032, 0x20001fc8, 0x008d0020, 0x88019400, // sendc(8)        null            m1<8,8,1>F
                                                        // render RT write SIMD8 LastRT Surface = 0 mlen 4 rlen 0 { align1 1Q EOT };
    };
    static const uint32_t gen7_copy_mem_code[] = {
        0x00600040, 0x20a06d29, 0x00480028, 0x10101010, // add(8)          g5<1>UW         g1.4<2,4,0>UW   0x10101010V     { align1 1Q };
        0x00600001, 0x20600065, 0x00000000, 0x00000000, // mov(8)          g3<1>D          0x00000000UD                    { align1 1Q };
        0x00600001, 0x20c0013d, 0x008d00a0, 0x00000000, // mov(8)          g6<1>F          g5<8,8,1>UW                     { align1 1Q };
        0x00600040, 0x20a07fbd, 0x008d00c0, 0x3f000000, // add(8)          g5<1>F          g6<8,8,1>F      0.5F            { align1 1Q };
        0x2000eb01, 0x00050707,                         // mov(8)          g7<1>D          g5<8,8,1>F                      { align1 1Q compacted };
        0x20018b40, 0x04070207,                         // add(8)          g2<1>D          g7<8,8,1>D      g4<0,1,0>D      { align1 1Q compacted };
        0x02600031, 0x2f801fa9, 0x008d0040, 0x04427001, // send(8)         g124<1>UW       g2<8,8,1>F
                                                        // sampler (1, 0, 7, 1) mlen 2 rlen 4              { align1 1Q };
        0x05600032, 0x20001fa8, 0x008d0f80, 0x88031400, // sendc(8)        null            g124<8,8,1>F
                                                        // render RT write SIMD8 LastRT Surface = 0 mlen 4 rlen 0 { align1 1Q EOT };
    };
    XGL_UINT surface_count, urb_grf_start;
    struct intel_pipeline_shader *sh;
    const void *code;
    XGL_SIZE code_size;

    switch (intel_gpu_gen(dev->gpu)) {
    case INTEL_GEN(6):
        if (id == INTEL_DEV_META_FS_COPY_MEM) {
            code = gen6_copy_mem_code;
            code_size = sizeof(gen6_copy_mem_code);
            surface_count = 2;
            urb_grf_start = 4;
        } else {
            code = gen6_clear_code;
            code_size = sizeof(gen6_clear_code);
            surface_count = 1;
            urb_grf_start = 2;
        }
        break;
    case INTEL_GEN(7):
    case INTEL_GEN(7.5):
        if (id == INTEL_DEV_META_FS_COPY_MEM) {
            code = gen7_copy_mem_code;
            code_size = sizeof(gen7_copy_mem_code);
            surface_count = 2;
            urb_grf_start = 4;
        } else {
            code = gen7_clear_code;
            code_size = sizeof(gen7_clear_code);
            surface_count = 1;
            urb_grf_start = 2;
        }
        break;
    default:
        code = NULL;
        break;
    }

    if (!code)
        return NULL;

    sh = icd_alloc(sizeof(*sh), 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!sh)
        return NULL;
    memset(sh, 0, sizeof(*sh));

    sh->pCode = icd_alloc(code_size, 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!sh->pCode) {
        icd_free(sh);
        return NULL;
    }

    memcpy(sh->pCode, code, code_size);
    sh->codeSize = code_size;

    sh->out_count = 1;
    sh->surface_count = surface_count;
    sh->urb_grf_start = urb_grf_start;

    return sh;
}

void intel_pipeline_shader_destroy(struct intel_pipeline_shader *sh)
{
    if (sh->rmap)
        rmap_destroy(sh->rmap);
    if (sh->pCode)
        icd_free(sh->pCode);
    icd_free(sh);
}
