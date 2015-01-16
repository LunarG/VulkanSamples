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
 *   LunarG
 */

#include "gpu.h"
#include "shader.h"
#include "pipeline.h"
#include "compiler/shader/compiler_interface.h"
#include "compiler/pipeline/pipeline_compiler_interface.h"
#include "compiler/pipeline/brw_context.h"
#include "compiler/pipeline/brw_shader.h"
#include "compiler/mesa-utils/src/mesa/main/context.h"
#include "compiler/mesa-utils/src/glsl/ralloc.h"
#include "compiler/pipeline/brw_device_info.h"
#include "compiler/pipeline/brw_wm.h"


static void initialize_brw_context(struct brw_context *brw,
                                   const struct intel_gpu *gpu)
{

    // create a stripped down context for compilation
    initialize_mesa_context_to_defaults(&brw->ctx);

    //
    // init the things pulled from DRI in brwCreateContext
    //
    struct brw_device_info *devInfo = rzalloc(brw, struct brw_device_info);
    switch (intel_gpu_gen(gpu)) {
    case INTEL_GEN(7.5):
        devInfo->gen = 7;
        devInfo->is_haswell = true;
        break;
    case INTEL_GEN(7):
        devInfo->gen = 7;
        break;
    case INTEL_GEN(6):
        devInfo->gen = 6;
        break;
    default:
        assert(!"unsupported GEN");
        break;
    }

    devInfo->gt = gpu->gt;
    devInfo->has_llc = true;
    devInfo->has_pln = true;
    devInfo->has_compr4 = true;
    devInfo->has_negative_rhw_bug = false;
    devInfo->needs_unlit_centroid_workaround = true;

    // hand code values until we have something to pull from
    // use brw_device_info_hsw_gt3
    brw->intelScreen = rzalloc(brw, struct intel_screen);
    brw->intelScreen->devinfo = devInfo;

    brw->gen = brw->intelScreen->devinfo->gen;
    brw->gt = brw->intelScreen->devinfo->gt;
    brw->is_g4x = brw->intelScreen->devinfo->is_g4x;
    brw->is_baytrail = brw->intelScreen->devinfo->is_baytrail;
    brw->is_haswell = brw->intelScreen->devinfo->is_haswell;
    brw->has_llc = brw->intelScreen->devinfo->has_llc;
    brw->has_pln = brw->intelScreen->devinfo->has_pln;
    brw->has_compr4 = brw->intelScreen->devinfo->has_compr4;
    brw->has_negative_rhw_bug = brw->intelScreen->devinfo->has_negative_rhw_bug;
    brw->needs_unlit_centroid_workaround =
       brw->intelScreen->devinfo->needs_unlit_centroid_workaround;

    brw->vs.base.stage = MESA_SHADER_VERTEX;
    brw->gs.base.stage = MESA_SHADER_GEOMETRY;
    brw->wm.base.stage = MESA_SHADER_FRAGMENT;

    //
    // init what remains of intel_screen
    //
    brw->intelScreen->deviceID = 0;
    brw->intelScreen->program_id = 0;

    brw_vec4_alloc_reg_set(brw->intelScreen);

    brw->shader_prog = brw_new_shader_program(&brw->ctx, 0);
}

static void hexdump(FILE *fp, void *ptr, int buflen) {
  unsigned int *buf = (unsigned int*)ptr;
  int i, j;
  for (i=0; i<(buflen/4); i+=4) {
    fprintf(fp,"%06x: ", i);
    for (j=0; j<4; j++)
      if (i+j < (buflen/4))
        fprintf(fp,"%08x ", buf[i+j]);
      else
        fprintf(fp,"   ");
    fprintf(fp,"\n");
  }

  fflush(fp);
}

static void base_prog_dump(FILE *fp, struct brw_stage_prog_data* base)
{
    fprintf(fp, "data->base.binding_table.size_bytes = %u\n",
                 base->binding_table.size_bytes);
    fprintf(fp, "data->base.binding_table.pull_constants_start = %u\n",
                 base->binding_table.pull_constants_start);
    fprintf(fp, "data->base.binding_table.texture_start = %u\n",
                 base->binding_table.texture_start);
    fprintf(fp, "data->base.binding_table.gather_texture_start = %u\n",
                 base->binding_table.gather_texture_start);
    fprintf(fp, "data->base.binding_table.ubo_start = %u\n",
                 base->binding_table.ubo_start);
    fprintf(fp, "data->base.binding_table.abo_start = %u\n",
                 base->binding_table.abo_start);
    fprintf(fp, "data->base.binding_table.shader_time_start = %u\n",
                 base->binding_table.shader_time_start);

    fprintf(fp, "data->base.nr_params = %u\n",
                 base->nr_params);
    fprintf(fp, "data->base.nr_pull_params = %u\n",
                 base->nr_pull_params);

    fprintf(fp, "== push constants: ==\n");
    fprintf(fp, "data->base.nr_params = %u\n",
                 base->nr_params);

    for (int i = 0; i < base->nr_params; ++i) {
        fprintf(fp, "data->base.param = %p\n",
                     base->param);
        fprintf(fp, "*data->base.param = %p\n",
                     *base->param);
        fprintf(fp, "**data->base.param = %f\n",
                     **base->param);
    }

    fprintf(fp, "== pull constants: ==\n");
    fprintf(fp, "data->base.nr_pull_params = %u\n",
                 base->nr_pull_params);

    for (int i = 0; i < base->nr_pull_params; ++i) {
        fprintf(fp, "data->base.pull_param = %p\n",
                     base->pull_param);
        fprintf(fp, "*data->base.pull_param = %p\n",
                     *base->pull_param);
        fprintf(fp, "**data->base.pull_param = %f\n",
                     **base->pull_param);
    }
}

static void vs_data_dump(FILE *fp, struct brw_vs_prog_data *data)
{
    fprintf(fp, "\n=== begin brw_vs_prog_data ===\n");

    base_prog_dump(fp, &data->base.base);

    fprintf(fp, "data->base.vue_map.slots_valid = 0x%" PRIX64 "\n",
                 data->base.vue_map.slots_valid);

    for (int i = 0; i < BRW_VARYING_SLOT_COUNT; ++i)
        fprintf(fp, "data->base.vue_map.varying_to_slot[%i] = %i\n", i,
               (int) data->base.vue_map.varying_to_slot[i]);

    for (int i = 0; i < BRW_VARYING_SLOT_COUNT; ++i)
        fprintf(fp, "data->base.vue_map.slot_to_varying[%i] = %i\n", i,
               (int) data->base.vue_map.slot_to_varying[i]);

    fprintf(fp, "data->base.vue_map.num_slots = %i\n",
                 data->base.vue_map.num_slots);
    fprintf(fp, "data->base.dispatch_grf_start_reg = %u\n",
                 data->base.dispatch_grf_start_reg);
    fprintf(fp, "data->base.curb_read_length = %u\n",
                 data->base.curb_read_length);
    fprintf(fp, "data->base.urb_read_length = %u\n",
                 data->base.urb_read_length);
    fprintf(fp, "data->base.total_grf = %u\n",
                 data->base.total_grf);
    fprintf(fp, "data->base.total_scratch = %u\n",
                 data->base.total_scratch);
    fprintf(fp, "data->base.urb_entry_size = %u\n",
                 data->base.urb_entry_size);

    fprintf(fp, "data->inputs_read = 0x%" PRIX64 "\n",
                 data->inputs_read);
    fprintf(fp, "data->uses_vertexid = %s\n",
                 data->uses_vertexid ? "true" : "false");
    fprintf(fp, "data->uses_instanceid = %s\n",
                 data->uses_instanceid ? "true" : "false");

    fprintf(fp, "=== end brw_vs_prog_data ===\n");

    fflush(fp);
}

static void fs_data_dump(FILE *fp, struct brw_wm_prog_data* data)
{
    fprintf(fp, "\n=== begin brw_wm_prog_data ===\n");

    base_prog_dump(fp, &data->base);

    fprintf(fp, "data->curb_read_length = %u\n",
                 data->curb_read_length);
    fprintf(fp, "data->num_varying_inputs = %u\n",
                 data->num_varying_inputs);

    fprintf(fp, "data->first_curbe_grf = %u\n",
                 data->first_curbe_grf);
    fprintf(fp, "data->first_curbe_grf_16 = %u\n",
                 data->first_curbe_grf_16);
    fprintf(fp, "data->reg_blocks = %u\n",
                 data->reg_blocks);
    fprintf(fp, "data->reg_blocks_16 = %u\n",
                 data->reg_blocks_16);
    fprintf(fp, "data->total_scratch = %u\n",
                 data->total_scratch);
    fprintf(fp, "data->binding_table.render_target_start = %u\n",
                 data->binding_table.render_target_start);

    fprintf(fp, "data->dual_src_blend = %s\n",
                 data->dual_src_blend ? "true" : "false");
    fprintf(fp, "data->uses_pos_offset = %s\n",
                 data->uses_pos_offset ? "true" : "false");
    fprintf(fp, "data->uses_omask = %s\n",
                 data->uses_omask ? "true" : "false");
    fprintf(fp, "data->prog_offset_16 = %u\n",
                 data->prog_offset_16);

    fprintf(fp, "data->barycentric_interp_modes = %u\n",
                 data->barycentric_interp_modes);

    for (int i = 0; i < VARYING_SLOT_MAX; ++i) {
        fprintf(fp, "data->urb_setup[%i] = %i\n",
                  i, data->urb_setup[i]);
    }

    fprintf(fp, "=== end brw_wm_prog_data ===\n");

    fflush(fp);
}

static struct intel_pipeline_rmap_slot *rmap_get_slot(struct intel_pipeline_rmap *rmap,
                                                      XGL_DESCRIPTOR_SET_SLOT_TYPE type,
                                                      XGL_UINT index)
{
    // The ordering of below offsets is important.  Textures need to come before
    // buffers with the current compiler conventions.
    const XGL_UINT texture_resource_offset = rmap->rt_count;
    const XGL_UINT resource_offset = texture_resource_offset + rmap->texture_resource_count;
    const XGL_UINT uav_offset = resource_offset + rmap->resource_count;
    const XGL_UINT sampler_offset = uav_offset + rmap->uav_count;
    struct intel_pipeline_rmap_slot *slot;

    switch (type) {
    case XGL_SLOT_UNUSED:
        slot = NULL;
        break;
    case XGL_SLOT_SHADER_TEXTURE_RESOURCE:
        slot = &rmap->slots[texture_resource_offset + index];
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
        if (!slot || slot->path_len)
            continue;

        slot->path_len = nest_level + 1;

        if (nest_level) {
            slot->u.path = (XGL_UINT *) icd_alloc(sizeof(slot->u.path[0]) *
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
        nest_path = (XGL_UINT *) icd_alloc(sizeof(nest_path[0]) * depth,
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
                              XGL_UINT index, XGL_UINT rt_count, XGL_UINT ubo_start)
{
    rmap->rt_count = rt_count;

    switch (type) {
    case XGL_SLOT_UNUSED:
        break;
    case XGL_SLOT_SHADER_TEXTURE_RESOURCE:
        if (rmap->texture_resource_count < index + 1)
            if (index < ubo_start - rt_count)
                rmap->texture_resource_count = index + 1;
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

static XGL_UINT rmap_init_counts(struct intel_pipeline_rmap *rmap,
                                 const XGL_DESCRIPTOR_SET_MAPPING *mapping,
                                 XGL_UINT rt_count, XGL_UINT ubo_start)
{
    XGL_UINT depth = 0;
    XGL_UINT i;

    for (i = 0; i < mapping->descriptorCount; i++) {
        const XGL_DESCRIPTOR_SLOT_INFO *info = &mapping->pDescriptorInfo[i];

        if (info->slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET) {
            const XGL_UINT d = rmap_init_counts(rmap,
                    info->pNextLevelSet, rt_count, ubo_start);
            if (depth < d + 1)
                depth = d + 1;

            continue;
        }

        rmap_update_count(rmap, info->slotObjectType,
                info->shaderEntityIndex, rt_count, ubo_start);
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

static struct intel_pipeline_rmap *rmap_create(const struct intel_gpu *gpu,
                                               const XGL_DESCRIPTOR_SET_MAPPING *mapping,
                                               const XGL_DYNAMIC_BUFFER_VIEW_SLOT_INFO *dyn,
                                               XGL_UINT rt_count, XGL_UINT ubo_start)
{
    struct intel_pipeline_rmap *rmap;
    struct intel_pipeline_rmap_slot *slot;
    XGL_UINT depth, rt;

    rmap = (struct intel_pipeline_rmap *)
        icd_alloc(sizeof(*rmap), 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!rmap)
        return NULL;

    memset(rmap, 0, sizeof(*rmap));

    depth = rmap_init_counts(rmap, mapping, rt_count, ubo_start);

    /* add RTs and the dynamic buffer view */
    rmap_update_count(rmap, dyn->slotObjectType, dyn->shaderEntityIndex, rt_count, ubo_start);

    rmap->slot_count = rmap->rt_count + rmap->texture_resource_count + rmap->resource_count +
        rmap->uav_count + rmap->sampler_count;

    rmap->slots = (struct intel_pipeline_rmap_slot *)
        icd_alloc(sizeof(rmap->slots[0]) * rmap->slot_count,
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

    /* add RTs and the dynamic buffer view */
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

extern "C" {

struct brw_context *intel_create_brw_context(const struct intel_gpu *gpu)
{
    // create a brw_context
    struct brw_context *brw = rzalloc(NULL, struct brw_context);

    // allocate sub structures on the stack
    initialize_brw_context(brw, gpu);

    return brw;
}

void intel_destroy_brw_context(struct brw_context *brw)
{
    ralloc_free(brw->shader_prog);
    ralloc_free(brw);
}

// invoke backend compiler to generate ISA and supporting data structures
XGL_RESULT intel_pipeline_shader_compile(struct intel_pipeline_shader *pipe_shader,
                                         const struct intel_gpu *gpu,
                                         const XGL_PIPELINE_SHADER *info)
{
    const struct intel_ir *ir = intel_shader(info->shader)->ir;
    /* XXX how about constness? */
    struct gl_shader_program *sh_prog = (struct gl_shader_program *) ir;
    XGL_RESULT status = XGL_SUCCESS;

    struct brw_context *brw = intel_create_brw_context(gpu);

    // LunarG : TODO - should this have been set for us somewhere?
    sh_prog->Type = sh_prog->Shaders[0]->Stage;

    if (brw_link_shader(&brw->ctx, sh_prog)) {

        // first take at standalone backend compile
        switch(sh_prog->Shaders[0]->Type) {
        case GL_VERTEX_SHADER:
        {
            pipe_shader->codeSize = get_vs_program_size(brw->shader_prog);

            pipe_shader->pCode = icd_alloc(pipe_shader->codeSize, 0, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
            if (!pipe_shader->pCode) {
                status = XGL_ERROR_OUT_OF_MEMORY;
                break;
            }

            // copy the ISA out of our compile context, it is about to poof away
            memcpy(pipe_shader->pCode, get_vs_program(brw->shader_prog), pipe_shader->codeSize);

            struct brw_vs_prog_data *data = get_vs_prog_data(brw->shader_prog);

            if (data->uses_vertexid)
                pipe_shader->uses |= INTEL_SHADER_USE_VID;

            if (data->uses_instanceid)
                pipe_shader->uses |= INTEL_SHADER_USE_IID;

            assert(VERT_ATTRIB_MAX - VERT_ATTRIB_GENERIC0 < 64);
            uint64_t user_attr_read = 0;
            for (int i=VERT_ATTRIB_GENERIC0; i < VERT_ATTRIB_MAX; i++) {
                if (data->inputs_read & BITFIELD64_BIT(i)) {
                    user_attr_read |= (1L << (i - VERT_ATTRIB_GENERIC0));
                }
            }
            pipe_shader->inputs_read = user_attr_read;

            pipe_shader->enable_user_clip = sh_prog->Vert.UsesClipDistance;

            assert(VARYING_SLOT_MAX - VARYING_SLOT_CLIP_DIST0 < 64);
            uint64_t varyings_written = 0;
            for (int i=VARYING_SLOT_CLIP_DIST0; i < VARYING_SLOT_MAX; i++) {
                if (data->base.vue_map.varying_to_slot[i] >= 0) {
                    varyings_written |= (1 << (i - VARYING_SLOT_CLIP_DIST0));
                }
            }
            pipe_shader->outputs_written = varyings_written;

            pipe_shader->outputs_offset = BRW_SF_URB_ENTRY_READ_OFFSET * 2;

            // These are really best guesses, and will require more work to
            // understand as we turn on more features
            pipe_shader->in_count = u_popcount(user_attr_read) +
                    ((data->uses_vertexid || data->uses_instanceid) ? 1 : 0);
            pipe_shader->out_count = data->base.vue_map.num_slots;// = 2;
            pipe_shader->urb_grf_start = data->base.dispatch_grf_start_reg;// = 1;
            pipe_shader->surface_count = data->base.base.binding_table.size_bytes / 4;
            pipe_shader->ubo_start     = data->base.base.binding_table.ubo_start;

            pipe_shader->per_thread_scratch_size = data->base.total_scratch;

            if (unlikely(INTEL_DEBUG & DEBUG_VS)) {
                printf("out_count: %d\n", pipe_shader->out_count);

                vs_data_dump(stdout, data);

                fprintf(stdout,"\nISA generated by compiler:\n");
                fprintf(stdout,"ISA size: %i\n", pipe_shader->codeSize);
                hexdump(stdout, pipe_shader->pCode, pipe_shader->codeSize);
                fflush(stdout);
            }
        }
            break;

        case GL_FRAGMENT_SHADER:
        {
            // Start pulling bits out of our compile result.
            // see upload_ps_state() for references about what I believe each of these values are

            // I would prefer to find a way to pull this data out without exposing
            // the internals of the compiler, but it hasn't presented itself yet

            pipe_shader->codeSize = get_wm_program_size(brw->shader_prog);

            pipe_shader->pCode = icd_alloc(pipe_shader->codeSize, 0, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
            if (!pipe_shader->pCode) {
                status = XGL_ERROR_OUT_OF_MEMORY;
                break;
            }

            // copy the ISA out of our compile context, it is about to poof away
            memcpy(pipe_shader->pCode, get_wm_program(brw->shader_prog), pipe_shader->codeSize);

            struct brw_wm_prog_data *data = get_wm_prog_data(brw->shader_prog);

            assert(VARYING_SLOT_MAX - VARYING_SLOT_CLIP_DIST0 < 64);
            uint64_t varyings_read = 0;
            for (int i=VARYING_SLOT_CLIP_DIST0; i < VARYING_SLOT_MAX; i++) {
                if (data->urb_setup[i] >= 0) {
                    varyings_read |= (1 << (i - VARYING_SLOT_CLIP_DIST0));
                }
            }
            pipe_shader->inputs_read = varyings_read;

            pipe_shader->generic_input_start = VARYING_SLOT_VAR0 - VARYING_SLOT_CLIP_DIST0;

            pipe_shader->reads_user_clip = data->urb_setup[VARYING_SLOT_CLIP_DIST0] >= 0 ||
                                           data->urb_setup[VARYING_SLOT_CLIP_DIST1] >= 0;

            pipe_shader->surface_count = data->base.binding_table.size_bytes / 4;
            pipe_shader->ubo_start     = data->base.binding_table.ubo_start;
            pipe_shader->urb_grf_start = data->first_curbe_grf;
            pipe_shader->in_count      = data->num_varying_inputs;

            // These are programmed based on gen7_wm_state.c::upload_wm_state()
            struct gl_fragment_program *fp = (struct gl_fragment_program *)
               sh_prog->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program;

            if (fp->UsesKill)
                pipe_shader->uses |= INTEL_SHADER_USE_KILL;

            if (fp->Base.InputsRead & VARYING_BIT_POS)
                pipe_shader->uses |= INTEL_SHADER_USE_DEPTH | INTEL_SHADER_USE_W;

            if (fp->Base.OutputsWritten & BITFIELD64_BIT(FRAG_RESULT_DEPTH)) {

                switch (fp->FragDepthLayout) {
                   case FRAG_DEPTH_LAYOUT_NONE:
                   case FRAG_DEPTH_LAYOUT_ANY:
                      pipe_shader->computed_depth_mode = INTEL_COMPUTED_DEPTH_MODE_ON;
                      break;
                   case FRAG_DEPTH_LAYOUT_GREATER:
                      pipe_shader->computed_depth_mode = INTEL_COMPUTED_DEPTH_MODE_ON_GE;
                      break;
                   case FRAG_DEPTH_LAYOUT_LESS:
                      pipe_shader->computed_depth_mode = INTEL_COMPUTED_DEPTH_MODE_ON_LE;
                      break;
                   case FRAG_DEPTH_LAYOUT_UNCHANGED:
                      break;
                }
            }

            // Ensure this is 1:1, or create a converter
            pipe_shader->barycentric_interps = data->barycentric_interp_modes;

            struct brw_stage_state *stage_state = &brw->wm.base;
            pipe_shader->sampler_count = stage_state->sampler_count;

            // TODO - Figure out multiple FS outputs
            pipe_shader->out_count = 1;

            pipe_shader->per_thread_scratch_size = data->total_scratch;

            if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
                // print out the supporting structures generated by the BE compile:
                fs_data_dump(stdout, data);

                printf("in_count: %d\n", pipe_shader->in_count);

                fprintf(stdout,"\nISA generated by compiler:\n");
                fprintf(stdout,"ISA size: %i\n", pipe_shader->codeSize);
                hexdump(stdout, pipe_shader->pCode, pipe_shader->codeSize);
                fflush(stdout);
            }
        }
            break;

        case GL_GEOMETRY_SHADER:
        case GL_COMPUTE_SHADER:
        default:
            assert(0);
            status = XGL_ERROR_BAD_PIPELINE_DATA;
        }
    } else {
        assert(0);
        status = XGL_ERROR_BAD_PIPELINE_DATA;
    }

    if (status == XGL_SUCCESS) {
        XGL_UINT rt_count = 0;

        if (info->stage == XGL_SHADER_STAGE_FRAGMENT) {
            const struct gl_program *prog =
                sh_prog->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program;

            rt_count = _mesa_bitcount_64(prog->OutputsWritten &
                    ~(BITFIELD64_BIT(FRAG_RESULT_DEPTH) |
                      BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK)));
        }

        XGL_DESCRIPTOR_SET_MAPPING null_map;
        XGL_DESCRIPTOR_SET_MAPPING *pMapping = info->pDescriptorSetMapping;
        null_map.descriptorCount = 0;

        if (! info->descriptorSetMappingCount) {
            pMapping = &null_map;
        }

        pipe_shader->rmap = rmap_create(gpu,
                pMapping,
                &info->dynamicBufferViewMapping,
                rt_count, pipe_shader->ubo_start);

        if (pipe_shader->rmap) {
            // Ensure that all textures in descriptor set were consumed.  This
            // is temporary until we move resource map building to compiler.
            assert(pipe_shader->ubo_start == rt_count +
                    pipe_shader->rmap->texture_resource_count);
        } else {
            intel_pipeline_shader_cleanup(pipe_shader);
            status = XGL_ERROR_OUT_OF_MEMORY;
        }
    }

    intel_destroy_brw_context(brw);

    return status;
}

void intel_pipeline_shader_cleanup(struct intel_pipeline_shader *sh)
{
    icd_free(sh->pCode);
    if (sh->rmap)
        rmap_destroy(sh->rmap);
    memset(sh, 0, sizeof(*sh));
}

void intel_disassemble_kernel(const struct intel_gpu *gpu,
                              const void *kernel, XGL_SIZE size)
{
    struct brw_compile c;

    memset(&c, 0, sizeof(c));
    c.brw = intel_create_brw_context(gpu);
    c.store = (struct brw_instruction *) kernel;

    brw_dump_compile(&c, stderr, 0, size);
}

} // extern "C"
