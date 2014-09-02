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

static struct intel_rmap_slot *rmap_get_slot(struct intel_rmap *rmap,
                                             XGL_DESCRIPTOR_SET_SLOT_TYPE type,
                                             XGL_UINT index)
{
    const XGL_UINT resource_offset = rmap->rt_count;
    const XGL_UINT uav_offset = resource_offset + rmap->resource_count;
    const XGL_UINT sampler_offset = uav_offset + rmap->uav_count;
    struct intel_rmap_slot *slot;

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
    default:
        assert(!"unknown rmap slot type");
        slot = NULL;
        break;
    }

    return slot;
}

static bool rmap_init_slots_with_path(struct intel_rmap *rmap,
                                      const XGL_DESCRIPTOR_SET_MAPPING *mapping,
                                      XGL_UINT *nest_path,
                                      XGL_UINT nest_level)
{
    XGL_UINT i;

    for (i = 0; i < mapping->descriptorCount; i++) {
        const XGL_DESCRIPTOR_SLOT_INFO *info = &mapping->pDescriptorInfo[i];
        struct intel_rmap_slot *slot;

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

static bool rmap_init_slots(struct intel_rmap *rmap,
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

static void rmap_update_count(struct intel_rmap *rmap,
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
    default:
        assert(!"unknown rmap slot type");
        break;
    }
}

static XGL_UINT rmap_init_counts(struct intel_rmap *rmap,
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

static void rmap_destroy(struct intel_rmap *rmap)
{
    XGL_UINT i;

    for (i = 0; i < rmap->slot_count; i++) {
        struct intel_rmap_slot *slot = &rmap->slots[i];

        switch (slot->path_len) {
        case 0:
        case 1:
        case INTEL_RMAP_SLOT_RT:
        case INTEL_RMAP_SLOT_DYN:
            break;
        default:
            icd_free(slot->u.path);
            break;
        }
    }

    icd_free(rmap->slots);
    icd_free(rmap);
}

static struct intel_rmap *rmap_create(struct intel_dev *dev,
                                      const XGL_DESCRIPTOR_SET_MAPPING *mapping,
                                      const XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO *dyn,
                                      XGL_UINT rt_count)
{
    struct intel_rmap *rmap;
    struct intel_rmap_slot *slot;
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
        rmap->uav_count + rmap->sampler_count;

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
        slot->path_len = INTEL_RMAP_SLOT_DYN;
        slot->u.index = 0;
    }
    for (rt = 0; rt < rmap->rt_count; rt++) {
        slot = &rmap->slots[rt];
        slot->path_len = INTEL_RMAP_SLOT_RT;
        slot->u.index = rt;
    }

    return rmap;
}

static void intel_pipe_shader_init(struct intel_shader *sh,
                                   struct intel_pipe_shader *pipe_sh)
{
    pipe_sh->in_count = sh->in_count;
    pipe_sh->out_count = sh->out_count;
    pipe_sh->sampler_count = sh->sampler_count;
    pipe_sh->surface_count = sh->surface_count;
    pipe_sh->barycentric_interps = sh->barycentric_interps;
    pipe_sh->urb_read_length = sh->urb_read_length;
    pipe_sh->urb_grf_start = sh->urb_grf_start;
    pipe_sh->uses = sh->uses;
}

static XGL_RESULT pipeline_shader(struct intel_pipeline *pipeline,
                                  const XGL_PIPELINE_SHADER *info)
{
    struct intel_shader *sh = intel_shader(info->shader);
    void *kernel;

    // TODO: process shader object and include in pipeline
    // For now that processing is simply a copy so that the app
    // can destroy the original shader object after pipeline creation.
    kernel = icd_alloc(sh->ir->size, 0, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
    if (!kernel)
        return XGL_ERROR_OUT_OF_MEMORY;

    // TODO: This should be a compile step
    memcpy(kernel, sh->ir->kernel, sh->ir->size);

    switch (info->stage) {
    case XGL_SHADER_STAGE_VERTEX:
        /*
         * TODO: What should we do here?
         * shader_state (XGL_PIPELINE_SHADER) contains links
         * to application memory in the pLinkConstBufferInfo and
         * it's pBufferData pointers. Do we need to bring all that
         * into the driver or is it okay to rely on those references
         * holding good data. In OpenGL we'd make a driver copy. Not
         * as clear for XGL.
         * For now, use the app pointers.
         */
        pipeline->vs = *info;

       /*
        * Grab what we need from the intel_shader object as that
        * could go away after the pipeline is created.
        */
        intel_pipe_shader_init(sh, &pipeline->intel_vs);
        pipeline->intel_vs.pCode = kernel;
        pipeline->intel_vs.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_VERTEX_FLAG;
        pipeline->vs_rmap = rmap_create(pipeline->dev,
                &info->descriptorSetMapping[0],
                &info->dynamicMemoryViewMapping, 0);
        if (!pipeline->vs_rmap) {
            icd_free(kernel);
            return XGL_ERROR_OUT_OF_MEMORY;
        }
        break;
    case XGL_SHADER_STAGE_GEOMETRY:
        intel_pipe_shader_init(sh, &pipeline->gs);
        pipeline->gs.pCode = kernel;
        pipeline->gs.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_GEOMETRY_FLAG;
        break;
    case XGL_SHADER_STAGE_FRAGMENT:
        pipeline->fs = *info;
        intel_pipe_shader_init(sh, &pipeline->intel_fs);
        pipeline->intel_fs.pCode = kernel;
        pipeline->intel_fs.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_FRAGMENT_FLAG;
        /* assuming one RT; need to parse the shader */
        pipeline->fs_rmap = rmap_create(pipeline->dev,
                &info->descriptorSetMapping[0],
                &info->dynamicMemoryViewMapping, 1);
        if (!pipeline->fs_rmap) {
            icd_free(kernel);
            return XGL_ERROR_OUT_OF_MEMORY;
        }
        break;
    case XGL_SHADER_STAGE_TESS_CONTROL:
        intel_pipe_shader_init(sh, &pipeline->tess_control);
        pipeline->tess_control.pCode = kernel;
        pipeline->tess_control.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_TESS_CONTROL_FLAG;
        break;
    case XGL_SHADER_STAGE_TESS_EVALUATION:
        intel_pipe_shader_init(sh, &pipeline->tess_eval);
        pipeline->tess_eval.pCode = kernel;
        pipeline->tess_eval.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_TESS_EVAL_FLAG;
        break;
    case XGL_SHADER_STAGE_COMPUTE:
        intel_pipe_shader_init(sh, &pipeline->compute);
        pipeline->compute.pCode = kernel;
        pipeline->compute.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_COMPUTE_FLAG;
        break;
    default:
        assert(!"unknown shader stage");
        break;
    }

    return XGL_SUCCESS;
}

XGL_RESULT pipeline_build_shaders(struct intel_pipeline *pipeline,
                                  const struct intel_pipeline_create_info *info)
{
    XGL_RESULT ret = XGL_SUCCESS;

    if (ret == XGL_SUCCESS && info->vs.shader)
        ret = pipeline_shader(pipeline, &info->vs);
    if (ret == XGL_SUCCESS && info->tcs.shader)
        ret = pipeline_shader(pipeline, &info->tcs);
    if (ret == XGL_SUCCESS && info->tes.shader)
        ret = pipeline_shader(pipeline, &info->tes);
    if (ret == XGL_SUCCESS && info->gs.shader)
        ret = pipeline_shader(pipeline, &info->gs);
    if (ret == XGL_SUCCESS && info->fs.shader)
        ret = pipeline_shader(pipeline, &info->fs);

    return ret;
}

void pipeline_tear_shaders(struct intel_pipeline *pipeline)
{
    if (pipeline->active_shaders & SHADER_VERTEX_FLAG) {
        icd_free(pipeline->intel_vs.pCode);
    }
    if (pipeline->active_shaders & SHADER_GEOMETRY_FLAG) {
        icd_free(pipeline->gs.pCode);
    }
    if (pipeline->active_shaders & SHADER_FRAGMENT_FLAG) {
        icd_free(pipeline->intel_fs.pCode);
    }
    if (pipeline->active_shaders & SHADER_TESS_CONTROL_FLAG) {
        icd_free(pipeline->tess_control.pCode);
    }
    if (pipeline->active_shaders & SHADER_TESS_EVAL_FLAG) {
        icd_free(pipeline->tess_eval.pCode);
    }

    if (pipeline->vs_rmap)
        rmap_destroy(pipeline->vs_rmap);
    if (pipeline->fs_rmap)
        rmap_destroy(pipeline->fs_rmap);
}
