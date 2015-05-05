/*
 * Vulkan
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

extern "C" {
#include "desc.h"
#include "gpu.h"
#include "shader.h"
#include "pipeline.h"
}

#include "compiler/shader/compiler_interface.h"
#include "compiler/pipeline/pipeline_compiler_interface.h"
#include "compiler/pipeline/brw_context.h"
#include "compiler/pipeline/brw_shader.h"
#include "compiler/mesa-utils/src/mesa/main/context.h"
#include "compiler/mesa-utils/src/glsl/ralloc.h"
#include "compiler/pipeline/brw_device_info.h"
#include "compiler/pipeline/brw_wm.h"

#ifndef STANDALONE_SHADER_COMPILER

    static const bool standaloneCompiler = false;

#else

    static const bool standaloneCompiler = true;

    // remove this when standalone resource map creation exists
    bool intel_desc_iter_init_for_binding(struct intel_desc_iter *iter,
                                          const struct intel_desc_layout *layout,
                                          uint32_t binding_index, uint32_t array_base)
    {
        return true;
    }

#endif //STANDALONE_SHADER_COMPILER

struct brw_binding_table {
    uint32_t count;

    uint32_t rt_start;

    uint32_t texture_start;
    uint32_t texture_count;

    uint32_t ubo_start;
    uint32_t ubo_count;

    uint32_t *sampler_binding;
    uint32_t *sampler_set;

    uint32_t *uniform_binding;
    uint32_t *uniform_set;
};

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

static void base_prog_dump(FILE *fp, struct brw_stage_prog_data* stage_prog_data)
{
    fprintf(fp, "stage_prog_data->binding_table.size_bytes = %u\n",
                 stage_prog_data->binding_table.size_bytes);
    fprintf(fp, "stage_prog_data->binding_table.pull_constants_start = %u\n",
                 stage_prog_data->binding_table.pull_constants_start);
    fprintf(fp, "stage_prog_data->binding_table.texture_start = %u\n",
                 stage_prog_data->binding_table.texture_start);
    fprintf(fp, "stage_prog_data->binding_table.gather_texture_start = %u\n",
                 stage_prog_data->binding_table.gather_texture_start);
    fprintf(fp, "stage_prog_data->binding_table.ubo_start = %u\n",
                 stage_prog_data->binding_table.ubo_start);
    fprintf(fp, "stage_prog_data->binding_table.abo_start = %u\n",
                 stage_prog_data->binding_table.abo_start);
    fprintf(fp, "stage_prog_data->binding_table.shader_time_start = %u\n",
                 stage_prog_data->binding_table.shader_time_start);

    fprintf(fp, "stage_prog_data->nr_params = %u\n",
                 stage_prog_data->nr_params);
    fprintf(fp, "stage_prog_data->nr_pull_params = %u\n",
                 stage_prog_data->nr_pull_params);

    fprintf(fp, "== push constants: ==\n");
    fprintf(fp, "stage_prog_data->nr_params = %u\n",
                 stage_prog_data->nr_params);

    for (int i = 0; i < stage_prog_data->nr_params; ++i) {
        fprintf(fp, "stage_prog_data->param = %p\n",
                     stage_prog_data->param);
        fprintf(fp, "*stage_prog_data->param = %p\n",
                     *stage_prog_data->param);
        fprintf(fp, "**stage_prog_data->param = %f\n",
                     **stage_prog_data->param);
    }

    fprintf(fp, "== pull constants: ==\n");
    fprintf(fp, "stage_prog_data->nr_pull_params = %u\n",
                 stage_prog_data->nr_pull_params);

    for (int i = 0; i < stage_prog_data->nr_pull_params; ++i) {
        fprintf(fp, "stage_prog_data->pull_param = %p\n",
                     stage_prog_data->pull_param);
        fprintf(fp, "*stage_prog_data->pull_param = %p\n",
                     *stage_prog_data->pull_param);
        fprintf(fp, "**stage_prog_data->pull_param = %f\n",
                     **stage_prog_data->pull_param);
    }
}


static void base_vec4_prog_dump(FILE *fp, struct brw_vec4_prog_data* vec4_prog_data)
{
    fprintf(fp, "vec4_prog_data->vue_map.slots_valid = 0x%" PRIX64 "\n",
                 vec4_prog_data->vue_map.slots_valid);

    for (int i = 0; i < BRW_VARYING_SLOT_COUNT; ++i)
        fprintf(fp, "vec4_prog_data->vue_map.varying_to_slot[%i] = %i\n", i,
               (int) vec4_prog_data->vue_map.varying_to_slot[i]);

    for (int i = 0; i < BRW_VARYING_SLOT_COUNT; ++i)
        fprintf(fp, "vec4_prog_data->vue_map.slot_to_varying[%i] = %i\n", i,
               (int) vec4_prog_data->vue_map.slot_to_varying[i]);

    fprintf(fp, "vec4_prog_data->vue_map.num_slots = %i\n",
                 vec4_prog_data->vue_map.num_slots);
    fprintf(fp, "vec4_prog_data->dispatch_grf_start_reg = %u\n",
                 vec4_prog_data->dispatch_grf_start_reg);
    fprintf(fp, "vec4_prog_data->curb_read_length = %u\n",
                 vec4_prog_data->curb_read_length);
    fprintf(fp, "vec4_prog_data->urb_read_length = %u\n",
                 vec4_prog_data->urb_read_length);
    fprintf(fp, "vec4_prog_data->total_grf = %u\n",
                 vec4_prog_data->total_grf);
    fprintf(fp, "vec4_prog_data->total_scratch = %u\n",
                 vec4_prog_data->total_scratch);
    fprintf(fp, "vec4_prog_data->urb_entry_size = %u\n",
                 vec4_prog_data->urb_entry_size);
}

static void vs_data_dump(FILE *fp, struct brw_vs_prog_data *vs_prog_data)
{
    fprintf(fp, "\n=== begin brw_vs_prog_data ===\n");

    base_prog_dump(fp, &vs_prog_data->base.base);

    base_vec4_prog_dump(fp, &vs_prog_data->base);

    fprintf(fp, "vs_prog_data->inputs_read = 0x%" PRIX64 "\n",
                 vs_prog_data->inputs_read);
    fprintf(fp, "vs_prog_data->uses_vertexid = %s\n",
                 vs_prog_data->uses_vertexid ? "true" : "false");
    fprintf(fp, "vs_prog_data->uses_instanceid = %s\n",
                 vs_prog_data->uses_instanceid ? "true" : "false");

    fprintf(fp, "=== end brw_vs_prog_data ===\n");

    fflush(fp);
}

static void gs_data_dump(FILE *fp, struct brw_gs_prog_data *gs_prog_data)
{
    fprintf(fp, "\n=== begin brw_gs_prog_data ===\n");

    base_prog_dump(fp, &gs_prog_data->base.base);

    base_vec4_prog_dump(fp, &gs_prog_data->base);

    fprintf(fp, "gs_prog_data->output_vertex_size_hwords = %u\n",
                 gs_prog_data->output_vertex_size_hwords);
    fprintf(fp, "gs_prog_data->output_topology = %u\n",
                 gs_prog_data->output_topology);
    fprintf(fp, "gs_prog_data->control_data_header_size_hwords = %u\n",
                 gs_prog_data->control_data_header_size_hwords);
    fprintf(fp, "gs_prog_data->control_data_format = %u\n",
                 gs_prog_data->control_data_format);
    fprintf(fp, "gs_prog_data->include_primitive_id = %s\n",
                 gs_prog_data->include_primitive_id ? "true" : "false");
    fprintf(fp, "gs_prog_data->invocations = %u\n",
                 gs_prog_data->invocations);
    fprintf(fp, "gs_prog_data->dual_instanced_dispatch = %s\n",
                 gs_prog_data->dual_instanced_dispatch ? "true" : "false");

    fprintf(fp, "=== end brw_gs_prog_data ===\n");

    fflush(fp);
}

static void fs_data_dump(FILE *fp, struct brw_wm_prog_data* wm_prog_data)
{
    fprintf(fp, "\n=== begin brw_wm_prog_data ===\n");

    base_prog_dump(fp, &wm_prog_data->base);

    fprintf(fp, "wm_prog_data->curb_read_length = %u\n",
                 wm_prog_data->curb_read_length);
    fprintf(fp, "wm_prog_data->num_varying_inputs = %u\n",
                 wm_prog_data->num_varying_inputs);
    fprintf(fp, "wm_prog_data->first_curbe_grf = %u\n",
                 wm_prog_data->first_curbe_grf);
    fprintf(fp, "wm_prog_data->first_curbe_grf_16 = %u\n",
                 wm_prog_data->first_curbe_grf_16);
    fprintf(fp, "wm_prog_data->reg_blocks = %u\n",
                 wm_prog_data->reg_blocks);
    fprintf(fp, "wm_prog_data->reg_blocks_16 = %u\n",
                 wm_prog_data->reg_blocks_16);
    fprintf(fp, "wm_prog_data->total_scratch = %u\n",
                 wm_prog_data->total_scratch);
    fprintf(fp, "wm_prog_data->binding_table.render_target_start = %u\n",
                 wm_prog_data->binding_table.render_target_start);
    fprintf(fp, "wm_prog_data->dual_src_blend = %s\n",
                 wm_prog_data->dual_src_blend ? "true" : "false");
    fprintf(fp, "wm_prog_data->uses_pos_offset = %s\n",
                 wm_prog_data->uses_pos_offset ? "true" : "false");
    fprintf(fp, "wm_prog_data->uses_omask = %s\n",
                 wm_prog_data->uses_omask ? "true" : "false");
    fprintf(fp, "wm_prog_data->prog_offset_16 = %u\n",
                 wm_prog_data->prog_offset_16);
    fprintf(fp, "wm_prog_data->barycentric_interp_modes = %u\n",
                 wm_prog_data->barycentric_interp_modes);

    for (int i = 0; i < VARYING_SLOT_MAX; ++i) {
        fprintf(fp, "wm_prog_data->urb_setup[%i] = %i\n",
                  i, wm_prog_data->urb_setup[i]);
    }

    fprintf(fp, "=== end brw_wm_prog_data ===\n");

    fflush(fp);
}

static inline void pipe_interface_free(const void *handle, void *ptr)
{
    if (standaloneCompiler)
        free(ptr);
    else
        intel_free(handle, ptr);
}

static inline void *pipe_interface_alloc(const void *handle,
                                                    size_t size, size_t alignment,
                                                    VkSystemAllocType type)
{
    if (standaloneCompiler)
        return calloc(size, sizeof(char));
    else
        return intel_alloc(handle, size, alignment, type);
}

static void rmap_destroy(const struct intel_gpu *gpu,
                         struct intel_pipeline_rmap *rmap)
{
    pipe_interface_free(gpu, rmap->slots);
    pipe_interface_free(gpu, rmap);
}

static struct intel_pipeline_rmap *rmap_create(const struct intel_gpu *gpu,
                                               const struct intel_pipeline_layout *pipeline_layout,
                                               const struct brw_binding_table *bt)
{
    struct intel_pipeline_rmap *rmap;
    struct intel_desc_iter iter;
    uint32_t surface_count, i;

    rmap = (struct intel_pipeline_rmap *)
        pipe_interface_alloc(gpu, sizeof(*rmap), 0, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!rmap)
        return NULL;

    // remove this when standalone resource map creation exists
    if (standaloneCompiler)
        return rmap;

    memset(rmap, 0, sizeof(*rmap));

    /* Fix the compiler and fix these!  No point in understanding them. */
    rmap->rt_count = bt->texture_start;
    rmap->texture_resource_count = bt->ubo_start - bt->texture_start;
    rmap->uav_count = bt->count - bt->ubo_start;
    rmap->sampler_count = rmap->texture_resource_count;
    surface_count = rmap->rt_count + rmap->texture_resource_count +
        rmap->uav_count;
    rmap->slot_count = surface_count + rmap->sampler_count;

    rmap->slots = (struct intel_pipeline_rmap_slot *)
        pipe_interface_alloc(gpu, sizeof(rmap->slots[0]) * rmap->slot_count,
            0, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!rmap->slots) {
        pipe_interface_free(gpu, rmap);
        return NULL;
    }

    memset(rmap->slots, 0, sizeof(rmap->slots[0]) * rmap->slot_count);

    for (i = 0; i < bt->rt_start; i++)
        rmap->slots[i].type = INTEL_PIPELINE_RMAP_UNUSED;

    for (i = bt->rt_start; i < bt->texture_start; i++) {
        rmap->slots[i].type = INTEL_PIPELINE_RMAP_RT;
        rmap->slots[i].index = i - bt->rt_start;
    }

    for (i = bt->texture_start; i < bt->ubo_start; i++) {
        rmap->slots[i].type = INTEL_PIPELINE_RMAP_SURFACE;
        rmap->slots[i].index = bt->sampler_set[i - bt->texture_start];

        // use the set and binding data to find correct dset slot
        // XXX validate both set and binding
        // XXX no array support
        intel_desc_iter_init_for_binding(&iter,
                pipeline_layout->layouts[rmap->slots[i].index],
                bt->sampler_binding[i - bt->texture_start], 0);

        rmap->slots[i].u.surface.offset = iter.begin;
        rmap->slots[i].u.surface.dynamic_offset_index = -1;

        rmap->slots[bt->count + i - bt->texture_start].type =
            INTEL_PIPELINE_RMAP_SAMPLER;
        rmap->slots[bt->count + i - bt->texture_start].index =
            rmap->slots[i].index;
        rmap->slots[bt->count + i - bt->texture_start].u.sampler =
            iter.begin;
    }

    for (i = bt->ubo_start; i < bt->count; i++) {
        rmap->slots[i].type = INTEL_PIPELINE_RMAP_SURFACE;
        rmap->slots[i].index = bt->uniform_set[i - bt->ubo_start];

        // use the set and binding data to find correct dset slot
        // XXX validate both set and binding
        // XXX no array support
        intel_desc_iter_init_for_binding(&iter,
                pipeline_layout->layouts[rmap->slots[i].index],
                bt->uniform_binding[i - bt->ubo_start], 0);

        rmap->slots[i].u.surface.offset = iter.begin;
        rmap->slots[i].u.surface.dynamic_offset_index = -1;
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

void unpack_set_and_binding(const int location, int &set, int &binding)
{
    // Logic mirrored from LunarGLASS GLSL backend
    set = (unsigned) location >> 16;
    binding = location & 0xFFFF;

    // Unbias set, which was biased by 1 to distinguish between "set=0" and nothing.
    bool setPresent = (set != 0);
    if (setPresent)
        --set;
}

const char *shader_stage_to_string(VkShaderStage stage)
{
   switch (stage) {
   case VK_SHADER_STAGE_VERTEX:          return "vertex";
   case VK_SHADER_STAGE_TESS_CONTROL:    return "tessellation evaluation";
   case VK_SHADER_STAGE_TESS_EVALUATION: return "tessellation control";
   case VK_SHADER_STAGE_GEOMETRY:        return "geometry";
   case VK_SHADER_STAGE_FRAGMENT:        return "fragment";
   case VK_SHADER_STAGE_COMPUTE:         return "compute";
   default:
       assert(0 && "Unknown shader stage");
       return "unknown";
   }
}

static VkResult build_binding_table(const struct intel_gpu *gpu,
                                    struct brw_context *brw,
                                    struct brw_binding_table *bt,
                                    brw_stage_prog_data data,
                                    struct gl_shader *sh,
                                    VkShaderStage stage)
{
    bt->count         = data.binding_table.size_bytes / 4;
    bt->texture_start = data.binding_table.texture_start;
    bt->texture_count = data.binding_table.ubo_start - data.binding_table.texture_start;
    bt->ubo_start     = data.binding_table.ubo_start;
    bt->ubo_count     = bt->count - data.binding_table.ubo_start;

    if (bt->ubo_count != sh->NumUniformBlocks) {
        // If there is no UBO data to pull from, the shader is using a default uniform, which
        // will not work in VK.  We need a binding slot to pull from.
        intel_log(gpu, VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, VK_NULL_HANDLE, 0, 0,
                "compile error: UBO mismatch, %s shader may read from global, non-block uniform", shader_stage_to_string(stage));

        assert(0);
        return VK_ERROR_BAD_PIPELINE_DATA;
    }

    // Sampler mapping data
    bt->sampler_binding = (uint32_t*) rzalloc_size(brw, bt->texture_count * sizeof(uint32_t));
    bt->sampler_set     = (uint32_t*) rzalloc_size(brw, bt->texture_count * sizeof(uint32_t));
    for (int i = 0; i < bt->texture_count; ++i) {
        int location = sh->SamplerUnits[i];
        int set = 0;
        int binding = 0;

        unpack_set_and_binding(location, set, binding);

        bt->sampler_binding[i] = binding;
        bt->sampler_set[i]     = set;
    }

    // UBO mapping data
    bt->uniform_binding = (uint32_t*) rzalloc_size(brw, bt->ubo_count * sizeof(uint32_t));
    bt->uniform_set     = (uint32_t*) rzalloc_size(brw, bt->ubo_count * sizeof(uint32_t));
    for (int i = 0; i < bt->ubo_count; ++i) {
        int location = sh->UniformBlocks[i].Binding;
        int set = 0;
        int binding = 0;

        unpack_set_and_binding(location, set, binding);

        bt->uniform_binding[i] = binding;
        bt->uniform_set[i]     = set;
    }

    return VK_SUCCESS;
}

// invoke backend compiler to generate ISA and supporting data structures
VkResult intel_pipeline_shader_compile(struct intel_pipeline_shader *pipe_shader,
                                         const struct intel_gpu *gpu,
                                         const struct intel_pipeline_layout *pipeline_layout,
                                         const VkPipelineShader *info,
                                         const struct intel_ir* ir)
{
    /* XXX how about constness? */
    struct gl_shader_program *sh_prog = (struct gl_shader_program *) ir;
    VkResult status = VK_SUCCESS;
    struct brw_binding_table bt;

    struct brw_context *brw = intel_create_brw_context(gpu);

    memset(&bt, 0, sizeof(bt));

    // LunarG : TODO - should this have been set for us somewhere?
    sh_prog->Type = sh_prog->Shaders[0]->Stage;

    if (brw_link_shader(&brw->ctx, sh_prog)) {

        // first take at standalone backend compile
        switch(sh_prog->Shaders[0]->Type) {
        case GL_VERTEX_SHADER:
        {
            pipe_shader->codeSize = get_vs_program_size(brw->shader_prog);

            pipe_shader->pCode = pipe_interface_alloc(gpu, pipe_shader->codeSize, 0, VK_SYSTEM_ALLOC_TYPE_INTERNAL_SHADER);

            if (!pipe_shader->pCode) {
                status = VK_ERROR_OUT_OF_HOST_MEMORY;
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

            status = build_binding_table(gpu, brw, &bt, data->base.base, sh_prog->_LinkedShaders[MESA_SHADER_VERTEX], VK_SHADER_STAGE_VERTEX);
            if (status != VK_SUCCESS)
                break;

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

        case GL_GEOMETRY_SHADER:
        {
            pipe_shader->codeSize = get_gs_program_size(brw->shader_prog);

            pipe_shader->pCode = pipe_interface_alloc(gpu, pipe_shader->codeSize, 0, VK_SYSTEM_ALLOC_TYPE_INTERNAL_SHADER);

            if (!pipe_shader->pCode) {
                status = VK_ERROR_OUT_OF_HOST_MEMORY;
                break;
            }

            // copy the ISA out of our compile context, it is about to poof away
            memcpy(pipe_shader->pCode, get_gs_program(brw->shader_prog), pipe_shader->codeSize);

            struct brw_gs_prog_data *data = get_gs_prog_data(brw->shader_prog);

            struct gl_geometry_program *gp = (struct gl_geometry_program *)
               sh_prog->_LinkedShaders[MESA_SHADER_GEOMETRY]->Program;

            // for now, assume inputs have not changed, but need to hook up vue_map_vs
            pipe_shader->inputs_read = gp->Base.InputsRead;

            // urb entries are reported in pairs, see vec4_gs_visitor::setup_varying_inputs()
            pipe_shader->in_count = data->base.urb_read_length * 2;

            pipe_shader->enable_user_clip = sh_prog->Geom.UsesClipDistance;
            pipe_shader->discard_adj      = (sh_prog->Geom.InputType == GL_LINES ||
                                             sh_prog->Geom.InputType == GL_TRIANGLES);

            assert(VARYING_SLOT_MAX - VARYING_SLOT_CLIP_DIST0 < 64);
            uint64_t varyings_written = 0;
            for (int i=VARYING_SLOT_CLIP_DIST0; i < VARYING_SLOT_MAX; i++) {
                if (data->base.vue_map.varying_to_slot[i] >= 0) {
                    varyings_written |= (1 << (i - VARYING_SLOT_CLIP_DIST0));
                }
            }
            pipe_shader->outputs_written = varyings_written;
            pipe_shader->outputs_offset = BRW_SF_URB_ENTRY_READ_OFFSET * 2;
            pipe_shader->out_count = data->base.vue_map.num_slots;

            // The following were all programmed in brw_gs_do_compile
            pipe_shader->output_size_hwords              = data->output_vertex_size_hwords;
            pipe_shader->output_topology                 = data->output_topology;
            pipe_shader->control_data_header_size_hwords = data->control_data_header_size_hwords;
            pipe_shader->control_data_format             = data->control_data_format;
            pipe_shader->include_primitive_id            = data->include_primitive_id;
            pipe_shader->invocations                     = data->invocations;
            pipe_shader->dual_instanced_dispatch         = data->dual_instanced_dispatch;

            // The rest duplicated from VS, merge it and clean up
            pipe_shader->urb_grf_start = data->base.dispatch_grf_start_reg;
            pipe_shader->surface_count = data->base.base.binding_table.size_bytes / 4;
            pipe_shader->ubo_start     = data->base.base.binding_table.ubo_start;
            pipe_shader->per_thread_scratch_size = data->base.total_scratch;

            status = build_binding_table(gpu, brw, &bt, data->base.base, sh_prog->_LinkedShaders[MESA_SHADER_GEOMETRY], VK_SHADER_STAGE_GEOMETRY);
            if (status != VK_SUCCESS)

            if (unlikely(INTEL_DEBUG & DEBUG_GS)) {
                printf("out_count: %d\n", pipe_shader->out_count);

                gs_data_dump(stdout, data);

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

            pipe_shader->pCode = pipe_interface_alloc(gpu, pipe_shader->codeSize, 0, VK_SYSTEM_ALLOC_TYPE_INTERNAL_SHADER);

            if (!pipe_shader->pCode) {
                status = VK_ERROR_OUT_OF_HOST_MEMORY;
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

            // Pass on SIMD16 info
            pipe_shader->urb_grf_start_16 = data->first_curbe_grf_16;
            pipe_shader->offset_16        = data->prog_offset_16;

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

            if (data->urb_setup[VARYING_SLOT_PNTC] >= 0)
                pipe_shader->point_sprite_enables = 1 << data->urb_setup[VARYING_SLOT_PNTC];

            struct brw_stage_state *stage_state = &brw->wm.base;
            pipe_shader->sampler_count = stage_state->sampler_count;

            // TODO - Figure out multiple FS outputs
            pipe_shader->out_count = 1;

            pipe_shader->per_thread_scratch_size = data->total_scratch;

            // call common code for common binding table entries
            status = build_binding_table(gpu, brw, &bt, data->base, sh_prog->_LinkedShaders[MESA_SHADER_FRAGMENT], VK_SHADER_STAGE_FRAGMENT);
            if (status != VK_SUCCESS)
                break;

            // and then tack on the remaining field for FS
            bt.rt_start = data->binding_table.render_target_start;

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

        case GL_COMPUTE_SHADER:
        default:
            assert(0);
            status = VK_ERROR_BAD_PIPELINE_DATA;
        }
    } else {
        assert(0);
        status = VK_ERROR_BAD_PIPELINE_DATA;
    }

    if (status == VK_SUCCESS) {
        pipe_shader->rmap = rmap_create(gpu, pipeline_layout, &bt);
        if (!pipe_shader->rmap) {
            intel_pipeline_shader_cleanup(pipe_shader, gpu);
            status = VK_ERROR_OUT_OF_HOST_MEMORY;
        }
    }

    intel_destroy_brw_context(brw);

    return status;
}

void intel_pipeline_shader_cleanup(struct intel_pipeline_shader *sh,
                                   const struct intel_gpu *gpu)
{
    pipe_interface_free(gpu, sh->pCode);
    if (sh->rmap)
        rmap_destroy(gpu, sh->rmap);
    memset(sh, 0, sizeof(*sh));
}


void intel_disassemble_kernel(const struct intel_gpu *gpu,
                              const void *kernel, size_t size)
{
    struct brw_compile c;

    memset(&c, 0, sizeof(c));
    c.brw = intel_create_brw_context(gpu);
    c.store = (struct brw_instruction *) kernel;

    brw_dump_compile(&c, stderr, 0, size);
}

} // extern "C"
