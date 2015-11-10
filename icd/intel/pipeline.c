/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: GregF <greg@LunarG.com>
 * Author: Tony Barbour <tony@LunarG.com>
 *
 */

#include "genhw/genhw.h"
#include "compiler/pipeline/pipeline_compiler_interface.h"
#include "cmd.h"
#include "format.h"
#include "shader.h"
#include "pipeline.h"
#include "mem.h"

static int translate_blend_func(VkBlendOp func)
{
   switch (func) {
   case VK_BLEND_OP_ADD:                return GEN6_BLENDFUNCTION_ADD;
   case VK_BLEND_OP_SUBTRACT:           return GEN6_BLENDFUNCTION_SUBTRACT;
   case VK_BLEND_OP_REVERSE_SUBTRACT:   return GEN6_BLENDFUNCTION_REVERSE_SUBTRACT;
   case VK_BLEND_OP_MIN:                return GEN6_BLENDFUNCTION_MIN;
   case VK_BLEND_OP_MAX:                return GEN6_BLENDFUNCTION_MAX;
   default:
      assert(!"unknown blend func");
      return GEN6_BLENDFUNCTION_ADD;
   };
}

static int translate_blend(VkBlendFactor blend)
{
   switch (blend) {
   case VK_BLEND_FACTOR_ZERO:                     return GEN6_BLENDFACTOR_ZERO;
   case VK_BLEND_FACTOR_ONE:                      return GEN6_BLENDFACTOR_ONE;
   case VK_BLEND_FACTOR_SRC_COLOR:                return GEN6_BLENDFACTOR_SRC_COLOR;
   case VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:      return GEN6_BLENDFACTOR_INV_SRC_COLOR;
   case VK_BLEND_FACTOR_DST_COLOR:               return GEN6_BLENDFACTOR_DST_COLOR;
   case VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR:     return GEN6_BLENDFACTOR_INV_DST_COLOR;
   case VK_BLEND_FACTOR_SRC_ALPHA:                return GEN6_BLENDFACTOR_SRC_ALPHA;
   case VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:      return GEN6_BLENDFACTOR_INV_SRC_ALPHA;
   case VK_BLEND_FACTOR_DST_ALPHA:               return GEN6_BLENDFACTOR_DST_ALPHA;
   case VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:     return GEN6_BLENDFACTOR_INV_DST_ALPHA;
   case VK_BLEND_FACTOR_CONSTANT_COLOR:           return GEN6_BLENDFACTOR_CONST_COLOR;
   case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR: return GEN6_BLENDFACTOR_INV_CONST_COLOR;
   case VK_BLEND_FACTOR_CONSTANT_ALPHA:           return GEN6_BLENDFACTOR_CONST_ALPHA;
   case VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA: return GEN6_BLENDFACTOR_INV_CONST_ALPHA;
   case VK_BLEND_FACTOR_SRC_ALPHA_SATURATE:       return GEN6_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case VK_BLEND_FACTOR_SRC1_COLOR:               return GEN6_BLENDFACTOR_SRC1_COLOR;
   case VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR:     return GEN6_BLENDFACTOR_INV_SRC1_COLOR;
   case VK_BLEND_FACTOR_SRC1_ALPHA:               return GEN6_BLENDFACTOR_SRC1_ALPHA;
   case VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA:     return GEN6_BLENDFACTOR_INV_SRC1_ALPHA;
   default:
      assert(!"unknown blend factor");
      return GEN6_BLENDFACTOR_ONE;
   };
}

static int translate_compare_func(VkCompareOp func)
{
    switch (func) {
    case VK_COMPARE_OP_NEVER:         return GEN6_COMPAREFUNCTION_NEVER;
    case VK_COMPARE_OP_LESS:          return GEN6_COMPAREFUNCTION_LESS;
    case VK_COMPARE_OP_EQUAL:         return GEN6_COMPAREFUNCTION_EQUAL;
    case VK_COMPARE_OP_LESS_OR_EQUAL:    return GEN6_COMPAREFUNCTION_LEQUAL;
    case VK_COMPARE_OP_GREATER:       return GEN6_COMPAREFUNCTION_GREATER;
    case VK_COMPARE_OP_NOT_EQUAL:     return GEN6_COMPAREFUNCTION_NOTEQUAL;
    case VK_COMPARE_OP_GREATER_OR_EQUAL: return GEN6_COMPAREFUNCTION_GEQUAL;
    case VK_COMPARE_OP_ALWAYS:        return GEN6_COMPAREFUNCTION_ALWAYS;
    default:
      assert(!"unknown compare_func");
      return GEN6_COMPAREFUNCTION_NEVER;
    }
}

static int translate_stencil_op(VkStencilOp op)
{
    switch (op) {
    case VK_STENCIL_OP_KEEP:       return GEN6_STENCILOP_KEEP;
    case VK_STENCIL_OP_ZERO:       return GEN6_STENCILOP_ZERO;
    case VK_STENCIL_OP_REPLACE:    return GEN6_STENCILOP_REPLACE;
    case VK_STENCIL_OP_INCREMENT_AND_CLAMP:  return GEN6_STENCILOP_INCRSAT;
    case VK_STENCIL_OP_DECREMENT_AND_CLAMP:  return GEN6_STENCILOP_DECRSAT;
    case VK_STENCIL_OP_INVERT:     return GEN6_STENCILOP_INVERT;
    case VK_STENCIL_OP_INCREMENT_AND_WRAP:   return GEN6_STENCILOP_INCR;
    case VK_STENCIL_OP_DECREMENT_AND_WRAP:   return GEN6_STENCILOP_DECR;
    default:
      assert(!"unknown stencil op");
      return GEN6_STENCILOP_KEEP;
    }
}

static int translate_sample_count(VkSampleCountFlagBits samples)
{
    switch (samples) {
    case VK_SAMPLE_COUNT_1_BIT:     return 1;
    case VK_SAMPLE_COUNT_2_BIT:     return 2;
    case VK_SAMPLE_COUNT_4_BIT:     return 4;
    case VK_SAMPLE_COUNT_8_BIT:     return 8;
    case VK_SAMPLE_COUNT_16_BIT:    return 16;
    case VK_SAMPLE_COUNT_32_BIT:    return 32;
    case VK_SAMPLE_COUNT_64_BIT:    return 64;
    default:
      assert(!"unknown sample count");
      return 1;
    }
}

struct intel_pipeline_create_info {
    VkFlags                                use_pipeline_dynamic_state;
    VkGraphicsPipelineCreateInfo           graphics;
    VkPipelineVertexInputStateCreateInfo   vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineDepthStencilStateCreateInfo  db;
    VkPipelineColorBlendStateCreateInfo    cb;
    VkPipelineRasterizationStateCreateInfo        rs;
    VkPipelineTessellationStateCreateInfo  tess;
    VkPipelineMultisampleStateCreateInfo   ms;
    VkPipelineViewportStateCreateInfo      vp;

    VkComputePipelineCreateInfo            compute;

    VkPipelineShaderStageCreateInfo        vs;
    VkPipelineShaderStageCreateInfo        tcs;
    VkPipelineShaderStageCreateInfo        tes;
    VkPipelineShaderStageCreateInfo        gs;
    VkPipelineShaderStageCreateInfo        fs;
};

/* in S1.3 */
struct intel_pipeline_sample_position {
    int8_t x, y;
};

static uint8_t pack_sample_position(const struct intel_dev *dev,
                                    const struct intel_pipeline_sample_position *pos)
{
    return (pos->x + 8) << 4 | (pos->y + 8);
}

void intel_pipeline_init_default_sample_patterns(const struct intel_dev *dev,
                                                 uint8_t *pat_1x, uint8_t *pat_2x,
                                                 uint8_t *pat_4x, uint8_t *pat_8x,
                                                 uint8_t *pat_16x)
{
    static const struct intel_pipeline_sample_position default_1x[1] = {
        {  0,  0 },
    };
    static const struct intel_pipeline_sample_position default_2x[2] = {
        { -4, -4 },
        {  4,  4 },
    };
    static const struct intel_pipeline_sample_position default_4x[4] = {
        { -2, -6 },
        {  6, -2 },
        { -6,  2 },
        {  2,  6 },
    };
    static const struct intel_pipeline_sample_position default_8x[8] = {
        { -1,  1 },
        {  1,  5 },
        {  3, -5 },
        {  5,  3 },
        { -7, -1 },
        { -3, -7 },
        {  7, -3 },
        { -5,  7 },
    };
    static const struct intel_pipeline_sample_position default_16x[16] = {
        {  0,  2 },
        {  3,  0 },
        { -3, -2 },
        { -2, -4 },
        {  4,  3 },
        {  5,  1 },
        {  6, -1 },
        {  2, -6 },
        { -4,  5 },
        { -5, -5 },
        { -1, -7 },
        {  7, -3 },
        { -7,  4 },
        {  1, -8 },
        { -6,  6 },
        { -8,  7 },
    };
    int i;

    pat_1x[0] = pack_sample_position(dev, default_1x);
    for (i = 0; i < 2; i++)
        pat_2x[i] = pack_sample_position(dev, &default_2x[i]);
    for (i = 0; i < 4; i++)
        pat_4x[i] = pack_sample_position(dev, &default_4x[i]);
    for (i = 0; i < 8; i++)
        pat_8x[i] = pack_sample_position(dev, &default_8x[i]);
    for (i = 0; i < 16; i++)
        pat_16x[i] = pack_sample_position(dev, &default_16x[i]);
}

struct intel_pipeline_shader *intel_pipeline_shader_create_meta(struct intel_dev *dev,
                                                                enum intel_dev_meta_shader id)
{
    struct intel_pipeline_shader *sh;
    VkResult ret;

    sh = intel_alloc(dev, sizeof(*sh), 0, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
    if (!sh)
        return NULL;
    memset(sh, 0, sizeof(*sh));

    ret = intel_pipeline_shader_compile_meta(sh, dev->gpu, id);
    if (ret != VK_SUCCESS) {
        intel_free(dev, sh);
        return NULL;
    }

    switch (id) {
    case INTEL_DEV_META_VS_FILL_MEM:
    case INTEL_DEV_META_VS_COPY_MEM:
    case INTEL_DEV_META_VS_COPY_MEM_UNALIGNED:
        sh->max_threads = intel_gpu_get_max_threads(dev->gpu,
                VK_SHADER_STAGE_VERTEX_BIT);
        break;
    default:
        sh->max_threads = intel_gpu_get_max_threads(dev->gpu,
                VK_SHADER_STAGE_FRAGMENT_BIT);
        break;
    }

    return sh;
}

void intel_pipeline_shader_destroy(struct intel_dev *dev,
                                   struct intel_pipeline_shader *sh)
{
    intel_pipeline_shader_cleanup(sh, dev->gpu);
    intel_free(dev, sh);
}

static VkResult pipeline_build_shader(struct intel_pipeline *pipeline,
                                        const VkPipelineShaderStageCreateInfo *sh_info,
                                        struct intel_pipeline_shader *sh)
{
    struct intel_shader_module *mod =
        intel_shader_module(sh_info->module);
    const struct intel_ir *ir =
        intel_shader_module_get_ir(mod, sh_info->stage);
    VkResult ret;

    if (!ir)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    ret = intel_pipeline_shader_compile(sh,
            pipeline->dev->gpu, pipeline->pipeline_layout, sh_info, ir);

    if (ret != VK_SUCCESS)
        return ret;

    sh->max_threads =
        intel_gpu_get_max_threads(pipeline->dev->gpu, sh_info->stage);

    /* 1KB aligned */
    sh->scratch_offset = u_align(pipeline->scratch_size, 1024);
    pipeline->scratch_size = sh->scratch_offset +
        sh->per_thread_scratch_size * sh->max_threads;

    pipeline->active_shaders |= sh_info->stage;

    return VK_SUCCESS;
}

static VkResult pipeline_build_shaders(struct intel_pipeline *pipeline,
                                         const struct intel_pipeline_create_info *info)
{
    VkResult ret = VK_SUCCESS;

    if (ret == VK_SUCCESS && info->vs.module)
        ret = pipeline_build_shader(pipeline, &info->vs, &pipeline->vs);
    if (ret == VK_SUCCESS && info->tcs.module)
        ret = pipeline_build_shader(pipeline, &info->tcs,&pipeline->tcs);
    if (ret == VK_SUCCESS && info->tes.module)
        ret = pipeline_build_shader(pipeline, &info->tes,&pipeline->tes);
    if (ret == VK_SUCCESS && info->gs.module)
        ret = pipeline_build_shader(pipeline, &info->gs, &pipeline->gs);
    if (ret == VK_SUCCESS && info->fs.module)
        ret = pipeline_build_shader(pipeline, &info->fs, &pipeline->fs);

    if (ret == VK_SUCCESS && info->compute.stage.module) {
        ret = pipeline_build_shader(pipeline,
                &info->compute.stage, &pipeline->cs);
    }

    return ret;
}
static uint32_t *pipeline_cmd_ptr(struct intel_pipeline *pipeline, int cmd_len)
{
    uint32_t *ptr;

    assert(pipeline->cmd_len + cmd_len < INTEL_PSO_CMD_ENTRIES);
    ptr = &pipeline->cmds[pipeline->cmd_len];
    pipeline->cmd_len += cmd_len;
    return ptr;
}

static VkResult pipeline_build_ia(struct intel_pipeline *pipeline,
                                    const struct intel_pipeline_create_info* info)
{
    pipeline->topology = info->ia.topology;
    pipeline->disable_vs_cache = false;

    switch (info->ia.topology) {
    case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
        pipeline->prim_type = GEN6_3DPRIM_POINTLIST;
        break;
    case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
        pipeline->prim_type = GEN6_3DPRIM_LINELIST;
        break;
    case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
        pipeline->prim_type = GEN6_3DPRIM_LINESTRIP;
        break;
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        pipeline->prim_type = GEN6_3DPRIM_TRILIST;
        break;
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
        pipeline->prim_type = GEN6_3DPRIM_TRISTRIP;
        break;
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
        pipeline->prim_type = GEN6_3DPRIM_TRIFAN;
        break;
    case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
        pipeline->prim_type = GEN6_3DPRIM_LINELIST_ADJ;
        break;
    case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
        pipeline->prim_type = GEN6_3DPRIM_LINESTRIP_ADJ;
        break;
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
        pipeline->prim_type = GEN6_3DPRIM_TRILIST_ADJ;
        break;
    case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
        pipeline->prim_type = GEN6_3DPRIM_TRISTRIP_ADJ;
        break;
    case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
        pipeline->prim_type = GEN7_3DPRIM_PATCHLIST_1 +
            info->tess.patchControlPoints - 1;
        break;
    default:
        assert(!"unsupported primitive topology format");
        break;
    }

    if (info->ia.primitiveRestartEnable) {
        pipeline->primitive_restart = true;
        pipeline->primitive_restart_index = 0;
    } else {
        pipeline->primitive_restart = false;
    }

    return VK_SUCCESS;
}

static VkResult pipeline_build_rs_state(struct intel_pipeline *pipeline,
                                          const struct intel_pipeline_create_info* info)
{
    const VkPipelineRasterizationStateCreateInfo *rs_state = &info->rs;
    bool ccw;

    pipeline->depthClipEnable = !rs_state->depthClampEnable;
    pipeline->rasterizerDiscardEnable = rs_state->rasterizerDiscardEnable;
    pipeline->depthBiasEnable = rs_state->depthBiasEnable;

    switch (rs_state->polygonMode) {
    case VK_POLYGON_MODE_POINT:
        pipeline->cmd_sf_fill |= GEN7_SF_DW1_FRONTFACE_POINT |
                              GEN7_SF_DW1_BACKFACE_POINT;
        break;
    case VK_POLYGON_MODE_LINE:
        pipeline->cmd_sf_fill |= GEN7_SF_DW1_FRONTFACE_WIREFRAME |
                              GEN7_SF_DW1_BACKFACE_WIREFRAME;
        break;
    case VK_POLYGON_MODE_FILL:
    default:
        pipeline->cmd_sf_fill |= GEN7_SF_DW1_FRONTFACE_SOLID |
                              GEN7_SF_DW1_BACKFACE_SOLID;
        break;
    }

    ccw = (rs_state->frontFace == VK_FRONT_FACE_COUNTER_CLOCKWISE);
    /* flip the winding order */

    if (ccw) {
        pipeline->cmd_sf_fill |= GEN7_SF_DW1_FRONTWINDING_CCW;
        pipeline->cmd_clip_cull |= GEN7_CLIP_DW1_FRONTWINDING_CCW;
    }

    switch (rs_state->cullMode) {
    case VK_CULL_MODE_NONE:
    default:
        pipeline->cmd_sf_cull |= GEN7_SF_DW2_CULLMODE_NONE;
        pipeline->cmd_clip_cull |= GEN7_CLIP_DW1_CULLMODE_NONE;
        break;
    case VK_CULL_MODE_FRONT_BIT:
        pipeline->cmd_sf_cull |= GEN7_SF_DW2_CULLMODE_FRONT;
        pipeline->cmd_clip_cull |= GEN7_CLIP_DW1_CULLMODE_FRONT;
        break;
    case VK_CULL_MODE_BACK_BIT:
        pipeline->cmd_sf_cull |= GEN7_SF_DW2_CULLMODE_BACK;
        pipeline->cmd_clip_cull |= GEN7_CLIP_DW1_CULLMODE_BACK;
        break;
    case VK_CULL_MODE_FRONT_AND_BACK:
        pipeline->cmd_sf_cull |= GEN7_SF_DW2_CULLMODE_BOTH;
        pipeline->cmd_clip_cull |= GEN7_CLIP_DW1_CULLMODE_BOTH;
        break;
    }

    /* only GEN7+ needs cull mode in 3DSTATE_CLIP */
    if (intel_gpu_gen(pipeline->dev->gpu) == INTEL_GEN(6))
        pipeline->cmd_clip_cull = 0;

    return VK_SUCCESS;
}

static void pipeline_destroy(struct intel_obj *obj)
{
    struct intel_pipeline *pipeline = intel_pipeline_from_obj(obj);

    if (pipeline->active_shaders & SHADER_VERTEX_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->vs, pipeline->dev->gpu);
    }

    if (pipeline->active_shaders & SHADER_TESS_CONTROL_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->tcs, pipeline->dev->gpu);
    }

    if (pipeline->active_shaders & SHADER_TESS_EVAL_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->tes, pipeline->dev->gpu);
    }

    if (pipeline->active_shaders & SHADER_GEOMETRY_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->gs, pipeline->dev->gpu);
    }

    if (pipeline->active_shaders & SHADER_FRAGMENT_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->fs, pipeline->dev->gpu);
    }

    if (pipeline->active_shaders & SHADER_COMPUTE_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->cs, pipeline->dev->gpu);
    }

    intel_base_destroy(&pipeline->obj.base);
}

static void pipeline_build_urb_alloc_gen6(struct intel_pipeline *pipeline,
                                          const struct intel_pipeline_create_info *info)
{
    const struct intel_gpu *gpu = pipeline->dev->gpu;
    const int urb_size = ((gpu->gt == 2) ? 64 : 32) * 1024;
    const struct intel_pipeline_shader *vs = &pipeline->vs;
    const struct intel_pipeline_shader *gs = &pipeline->gs;
    int vs_entry_size, gs_entry_size;
    int vs_size, gs_size;

    INTEL_GPU_ASSERT(gpu, 6, 6);

    vs_entry_size = ((vs->in_count >= vs->out_count) ?
        vs->in_count : vs->out_count);
    gs_entry_size = (gs) ? gs->out_count : 0;

    /* in bytes */
    vs_entry_size *= sizeof(float) * 4;
    gs_entry_size *= sizeof(float) * 4;

    if (pipeline->active_shaders & SHADER_GEOMETRY_FLAG) {
        vs_size = urb_size / 2;
        gs_size = vs_size;
    } else {
        vs_size = urb_size;
        gs_size = 0;
    }

    /* 3DSTATE_URB */
    {
        const uint8_t cmd_len = 3;
        const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_URB) |
                             (cmd_len - 2);
        int vs_alloc_size, gs_alloc_size;
        int vs_entry_count, gs_entry_count;
        uint32_t *dw;

        /* in 1024-bit rows */
        vs_alloc_size = (vs_entry_size + 128 - 1) / 128;
        gs_alloc_size = (gs_entry_size + 128 - 1) / 128;

        /* valid range is [1, 5] */
        if (!vs_alloc_size)
            vs_alloc_size = 1;
        if (!gs_alloc_size)
            gs_alloc_size = 1;
        assert(vs_alloc_size <= 5 && gs_alloc_size <= 5);

        /* valid range is [24, 256], multiples of 4 */
        vs_entry_count = (vs_size / 128 / vs_alloc_size) & ~3;
        if (vs_entry_count > 256)
            vs_entry_count = 256;
        assert(vs_entry_count >= 24);

        /* valid range is [0, 256], multiples of 4 */
        gs_entry_count = (gs_size / 128 / gs_alloc_size) & ~3;
        if (gs_entry_count > 256)
            gs_entry_count = 256;

        dw = pipeline_cmd_ptr(pipeline, cmd_len);

        dw[0] = dw0;
        dw[1] = (vs_alloc_size - 1) << GEN6_URB_DW1_VS_ENTRY_SIZE__SHIFT |
                vs_entry_count << GEN6_URB_DW1_VS_ENTRY_COUNT__SHIFT;
        dw[2] = gs_entry_count << GEN6_URB_DW2_GS_ENTRY_COUNT__SHIFT |
                (gs_alloc_size - 1) << GEN6_URB_DW2_GS_ENTRY_SIZE__SHIFT;
    }
}

static void pipeline_build_urb_alloc_gen7(struct intel_pipeline *pipeline,
                                          const struct intel_pipeline_create_info *info)
{
    const struct intel_gpu *gpu = pipeline->dev->gpu;
    const int urb_size = ((gpu->gt == 3) ? 512 :
                          (gpu->gt == 2) ? 256 : 128) * 1024;
    const struct intel_pipeline_shader *vs = &pipeline->vs;
    const struct intel_pipeline_shader *gs = &pipeline->gs;
    /* some space is reserved for PCBs */
    int urb_offset = ((gpu->gt == 3) ? 32 : 16) * 1024;
    int vs_entry_size, gs_entry_size;
    int vs_size, gs_size;

    INTEL_GPU_ASSERT(gpu, 7, 7.5);

    vs_entry_size = ((vs->in_count >= vs->out_count) ?
        vs->in_count : vs->out_count);
    gs_entry_size = (gs) ? gs->out_count : 0;

    /* in bytes */
    vs_entry_size *= sizeof(float) * 4;
    gs_entry_size *= sizeof(float) * 4;

    if (pipeline->active_shaders & SHADER_GEOMETRY_FLAG) {
        vs_size = (urb_size - urb_offset) / 2;
        gs_size = vs_size;
    } else {
        vs_size = urb_size - urb_offset;
        gs_size = 0;
    }

    /* 3DSTATE_URB_* */
    {
        const uint8_t cmd_len = 2;
        int vs_alloc_size, gs_alloc_size;
        int vs_entry_count, gs_entry_count;
        uint32_t *dw;

        /* in 512-bit rows */
        vs_alloc_size = (vs_entry_size + 64 - 1) / 64;
        gs_alloc_size = (gs_entry_size + 64 - 1) / 64;

        if (!vs_alloc_size)
            vs_alloc_size = 1;
        if (!gs_alloc_size)
            gs_alloc_size = 1;

        /* avoid performance decrease due to banking */
        if (vs_alloc_size == 5)
            vs_alloc_size = 6;

        /* in multiples of 8 */
        vs_entry_count = (vs_size / 64 / vs_alloc_size) & ~7;
        assert(vs_entry_count >= 32);

        gs_entry_count = (gs_size / 64 / gs_alloc_size) & ~7;

        if (intel_gpu_gen(gpu) >= INTEL_GEN(7.5)) {
            const int max_vs_entry_count =
                (gpu->gt >= 2) ? 1664 : 640;
            const int max_gs_entry_count =
                (gpu->gt >= 2) ? 640 : 256;
            if (vs_entry_count >= max_vs_entry_count)
                vs_entry_count = max_vs_entry_count;
            if (gs_entry_count >= max_gs_entry_count)
                gs_entry_count = max_gs_entry_count;
        } else {
            const int max_vs_entry_count =
                (gpu->gt == 2) ? 704 : 512;
            const int max_gs_entry_count =
                (gpu->gt == 2) ? 320 : 192;
            if (vs_entry_count >= max_vs_entry_count)
                vs_entry_count = max_vs_entry_count;
            if (gs_entry_count >= max_gs_entry_count)
                gs_entry_count = max_gs_entry_count;
        }

        dw = pipeline_cmd_ptr(pipeline, cmd_len*4);
        dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_VS) | (cmd_len - 2);
        dw[1] = (urb_offset / 8192) << GEN7_URB_DW1_OFFSET__SHIFT |
                (vs_alloc_size - 1) << GEN7_URB_DW1_ENTRY_SIZE__SHIFT |
                vs_entry_count;

        dw += 2;
        if (gs_size)
            urb_offset += vs_size;
        dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_GS) | (cmd_len - 2);
        dw[1] = (urb_offset  / 8192) << GEN7_URB_DW1_OFFSET__SHIFT |
                (gs_alloc_size - 1) << GEN7_URB_DW1_ENTRY_SIZE__SHIFT |
                gs_entry_count;

        dw += 2;
        dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_HS) | (cmd_len - 2);
        dw[1] = (urb_offset / 8192)  << GEN7_URB_DW1_OFFSET__SHIFT;

        dw += 2;
        dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_DS) | (cmd_len - 2);
        dw[1] = (urb_offset / 8192)  << GEN7_URB_DW1_OFFSET__SHIFT;
    }
}

static void pipeline_build_vertex_elements(struct intel_pipeline *pipeline,
                                           const struct intel_pipeline_create_info *info)
{
    const struct intel_pipeline_shader *vs = &pipeline->vs;
    uint8_t cmd_len;
    uint32_t *dw;
    uint32_t i, j;
    uint32_t attr_count;
    uint32_t attrs_processed;
    int comps[4];

    INTEL_GPU_ASSERT(pipeline->dev->gpu, 6, 7.5);

    attr_count = u_popcountll(vs->inputs_read);
    cmd_len = 1 + 2 * attr_count;
    if (vs->uses & (INTEL_SHADER_USE_VID | INTEL_SHADER_USE_IID))
        cmd_len += 2;

    if (cmd_len == 1)
        return;

    dw = pipeline_cmd_ptr(pipeline, cmd_len);

    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_ELEMENTS) |
            (cmd_len - 2);
    dw++;

    /* VERTEX_ELEMENT_STATE */
    for (i = 0, attrs_processed = 0; attrs_processed < attr_count; i++) {
        VkVertexInputAttributeDescription *attr = NULL;

        /*
         * The compiler will pack the shader references and then
         * indicate which locations are used via the bitmask in
         * vs->inputs_read.
         */
        if (!(vs->inputs_read & (1L << i))) {
            continue;
        }

        /*
         * For each bit set in the vs->inputs_read we'll need
         * to find the corresponding attribute record and then
         * set up the next HW vertex element based on that attribute.
         */
        for (j = 0; j < info->vi.vertexAttributeDescriptionCount; j++) {
            if (info->vi.pVertexAttributeDescriptions[j].location == i) {
                attr = (VkVertexInputAttributeDescription *) &info->vi.pVertexAttributeDescriptions[j];
                attrs_processed++;
                break;
            }
        }
        assert(attr != NULL);

        const int format =
            intel_format_translate_color(pipeline->dev->gpu, attr->format);

        comps[0] = GEN6_VFCOMP_STORE_0;
        comps[1] = GEN6_VFCOMP_STORE_0;
        comps[2] = GEN6_VFCOMP_STORE_0;
        comps[3] = icd_format_is_int(attr->format) ?
            GEN6_VFCOMP_STORE_1_INT : GEN6_VFCOMP_STORE_1_FP;

        switch (icd_format_get_channel_count(attr->format)) {
        case 4: comps[3] = GEN6_VFCOMP_STORE_SRC; /* fall through */
        case 3: comps[2] = GEN6_VFCOMP_STORE_SRC; /* fall through */
        case 2: comps[1] = GEN6_VFCOMP_STORE_SRC; /* fall through */
        case 1: comps[0] = GEN6_VFCOMP_STORE_SRC; break;
        default:
            break;
        }

        assert(attr->offset <= 2047);

        dw[0] = attr->binding << GEN6_VE_DW0_VB_INDEX__SHIFT |
                GEN6_VE_DW0_VALID |
                format << GEN6_VE_DW0_FORMAT__SHIFT |
                attr->offset;

        dw[1] = comps[0] << GEN6_VE_DW1_COMP0__SHIFT |
                comps[1] << GEN6_VE_DW1_COMP1__SHIFT |
                comps[2] << GEN6_VE_DW1_COMP2__SHIFT |
                comps[3] << GEN6_VE_DW1_COMP3__SHIFT;

        dw += 2;
    }

    if (vs->uses & (INTEL_SHADER_USE_VID | INTEL_SHADER_USE_IID)) {
        comps[0] = (vs->uses & INTEL_SHADER_USE_VID) ?
            GEN6_VFCOMP_STORE_VID : GEN6_VFCOMP_STORE_0;
        comps[1] = (vs->uses & INTEL_SHADER_USE_IID) ?
            GEN6_VFCOMP_STORE_IID : GEN6_VFCOMP_NOSTORE;
        comps[2] = GEN6_VFCOMP_NOSTORE;
        comps[3] = GEN6_VFCOMP_NOSTORE;

        dw[0] = GEN6_VE_DW0_VALID;
        dw[1] = comps[0] << GEN6_VE_DW1_COMP0__SHIFT |
                comps[1] << GEN6_VE_DW1_COMP1__SHIFT |
                comps[2] << GEN6_VE_DW1_COMP2__SHIFT |
                comps[3] << GEN6_VE_DW1_COMP3__SHIFT;

        dw += 2;
    }
}

static void pipeline_build_fragment_SBE(struct intel_pipeline *pipeline,
                                        const struct intel_pipeline_create_info *info)
{
    const struct intel_pipeline_shader *fs = &pipeline->fs;
    uint8_t cmd_len;
    uint32_t *body;
    uint32_t attr_skip, attr_count;
    uint32_t vue_offset, vue_len;
    uint32_t i;

    // If GS is active, use its outputs
    const struct intel_pipeline_shader *src =
            (pipeline->active_shaders & SHADER_GEOMETRY_FLAG)
                    ? &pipeline->gs
                    : &pipeline->vs;

    INTEL_GPU_ASSERT(pipeline->dev->gpu, 6, 7.5);

    cmd_len = 14;

    if (intel_gpu_gen(pipeline->dev->gpu) >= INTEL_GEN(7))
        body = pipeline_cmd_ptr(pipeline, cmd_len);
    else
        body = pipeline->cmd_3dstate_sbe;

    assert(!fs->reads_user_clip || src->enable_user_clip);
    attr_skip = src->outputs_offset;
    if (src->enable_user_clip != fs->reads_user_clip) {
        attr_skip += 2;
    }
    assert(src->out_count >= attr_skip);
    attr_count = src->out_count - attr_skip;

    // LUNARG TODO: We currently are only handling 16 attrs;
    // ultimately, we need to handle 32
    assert(fs->in_count <= 16);
    assert(attr_count <= 16);

    vue_offset = attr_skip / 2;
    vue_len = (attr_count + 1) / 2;
    if (!vue_len)
        vue_len = 1;

    body[0] = GEN7_RENDER_CMD(3D, 3DSTATE_SBE) |
            (cmd_len - 2);

    // LUNARG TODO: If the attrs needed by the FS are exactly
    // what is written by the VS, we don't need to enable
    // swizzling, improving performance. Even if we swizzle,
    // we can improve performance by reducing vue_len to
    // just include the values needed by the FS:
    // vue_len = ceiling((max_vs_out + 1)/2)

    body[1] = GEN7_SBE_DW1_ATTR_SWIZZLE_ENABLE |
          fs->in_count << GEN7_SBE_DW1_ATTR_COUNT__SHIFT |
          vue_len << GEN7_SBE_DW1_URB_READ_LEN__SHIFT |
          vue_offset << GEN7_SBE_DW1_URB_READ_OFFSET__SHIFT;

    /* Vulkan default is point origin upper left */
    body[1] |= GEN7_SBE_DW1_POINT_SPRITE_TEXCOORD_UPPERLEFT;

    uint16_t src_slot[fs->in_count];
    int32_t fs_in = 0;
    int32_t src_out = - (vue_offset * 2 - src->outputs_offset);
    for (i=0; i < 64; i++) {
        bool srcWrites = src->outputs_written & (1L << i);
        bool fsReads   = fs->inputs_read      & (1L << i);

        if (fsReads) {
            assert(src_out >= 0);
            assert(fs_in < fs->in_count);
            src_slot[fs_in] = src_out;

            if (!srcWrites) {
                // If the vertex shader did not write this input, we cannot
                // program the SBE to read it.  Our choices are to allow it to
                // read junk from a GRF, or get zero.  We're choosing zero.
                if (i >= fs->generic_input_start) {
                    src_slot[fs_in] = GEN8_SBE_SWIZ_CONST_0000 |
                                     GEN8_SBE_SWIZ_OVERRIDE_X |
                                     GEN8_SBE_SWIZ_OVERRIDE_Y |
                                     GEN8_SBE_SWIZ_OVERRIDE_Z |
                                     GEN8_SBE_SWIZ_OVERRIDE_W;
                }
            }

            fs_in += 1;
        }
        if (srcWrites) {
            src_out += 1;
        }
    }

    for (i = 0; i < 8; i++) {
        uint16_t hi, lo;

        /* no attr swizzles */
        if (i * 2 + 1 < fs->in_count) {
            lo = src_slot[i * 2];
            hi = src_slot[i * 2 + 1];
        } else if (i * 2 < fs->in_count) {
            lo = src_slot[i * 2];
            hi = 0;
        } else {
            hi = 0;
            lo = 0;
        }

        body[2 + i] = hi << GEN8_SBE_SWIZ_HIGH__SHIFT | lo;
    }

    if (info->ia.topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
        body[10] = fs->point_sprite_enables;
    else
        body[10] = 0;

    body[11] = 0; /* constant interpolation enables */
    body[12] = 0; /* WrapShortest enables */
    body[13] = 0;
}

static void pipeline_build_gs(struct intel_pipeline *pipeline,
                              const struct intel_pipeline_create_info *info)
{
    // gen7_emit_3DSTATE_GS done by cmd_pipeline
}

static void pipeline_build_hs(struct intel_pipeline *pipeline,
                              const struct intel_pipeline_create_info *info)
{
    const uint8_t cmd_len = 7;
    const uint32_t dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_HS) | (cmd_len - 2);
    uint32_t *dw;

    INTEL_GPU_ASSERT(pipeline->dev->gpu, 7, 7.5);

    dw = pipeline_cmd_ptr(pipeline, cmd_len);
    dw[0] = dw0;
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 0;
    dw[5] = 0;
    dw[6] = 0;
}

static void pipeline_build_te(struct intel_pipeline *pipeline,
                              const struct intel_pipeline_create_info *info)
{
    const uint8_t cmd_len = 4;
    const uint32_t dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_TE) | (cmd_len - 2);
    uint32_t *dw;

    INTEL_GPU_ASSERT(pipeline->dev->gpu, 7, 7.5);

    dw = pipeline_cmd_ptr(pipeline, cmd_len);
    dw[0] = dw0;
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
}

static void pipeline_build_ds(struct intel_pipeline *pipeline,
                              const struct intel_pipeline_create_info *info)
{
    const uint8_t cmd_len = 6;
    const uint32_t dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_DS) | (cmd_len - 2);
    uint32_t *dw;

    INTEL_GPU_ASSERT(pipeline->dev->gpu, 7, 7.5);

    dw = pipeline_cmd_ptr(pipeline, cmd_len);
    dw[0] = dw0;
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 0;
    dw[5] = 0;
}

static void pipeline_build_depth_stencil(struct intel_pipeline *pipeline,
                                         const struct intel_pipeline_create_info *info)
{
    pipeline->cmd_depth_stencil = 0;

    if (info->db.stencilTestEnable) {
        pipeline->cmd_depth_stencil = 1 << 31 |
               translate_compare_func(info->db.front.compareOp) << 28 |
               translate_stencil_op(info->db.front.failOp) << 25 |
               translate_stencil_op(info->db.front.depthFailOp) << 22 |
               translate_stencil_op(info->db.front.passOp) << 19 |
               1 << 15 |
               translate_compare_func(info->db.back.compareOp) << 12 |
               translate_stencil_op(info->db.back.failOp) << 9 |
               translate_stencil_op(info->db.back.depthFailOp) << 6 |
               translate_stencil_op(info->db.back.passOp) << 3;
     }

    pipeline->stencilTestEnable = info->db.stencilTestEnable;

    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 360:
     *
     *     "Enabling the Depth Test function without defining a Depth Buffer is
     *      UNDEFINED."
     *
     * From the Sandy Bridge PRM, volume 2 part 1, page 375:
     *
     *     "A Depth Buffer must be defined before enabling writes to it, or
     *      operation is UNDEFINED."
     *
     * TODO We do not check these yet.
     */
    if (info->db.depthTestEnable) {
       pipeline->cmd_depth_test = GEN6_ZS_DW2_DEPTH_TEST_ENABLE |
               translate_compare_func(info->db.depthCompareOp) << 27;
    } else {
       pipeline->cmd_depth_test = GEN6_COMPAREFUNCTION_ALWAYS << 27;
    }

    if (info->db.depthWriteEnable)
       pipeline->cmd_depth_test |= GEN6_ZS_DW2_DEPTH_WRITE_ENABLE;
}

static void pipeline_build_msaa(struct intel_pipeline *pipeline,
                                const struct intel_pipeline_create_info *info)
{
    uint32_t cmd, cmd_len;
    uint32_t *dw;

    INTEL_GPU_ASSERT(pipeline->dev->gpu, 6, 7.5);

    pipeline->sample_count =
        translate_sample_count(info->ms.rasterizationSamples);

    pipeline->alphaToCoverageEnable = info->ms.alphaToCoverageEnable;
    pipeline->alphaToOneEnable = info->ms.alphaToOneEnable;

    /* 3DSTATE_SAMPLE_MASK */
    cmd = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLE_MASK);
    cmd_len = 2;

    dw = pipeline_cmd_ptr(pipeline, cmd_len);
    dw[0] = cmd | (cmd_len - 2);
    if (info->ms.pSampleMask) {
        /* "Bit B of mask word M corresponds to sample 32*M + B."
         * "The array is sized to a length of ceil(rasterizationSamples / 32) words."
         * "If pSampleMask is NULL, it is treated as if the mask has all bits enabled,"
         * "i.e. no coverage is removed from primitives."
         */
        assert(pipeline->sample_count / 32 == 0);
        dw[1] = *info->ms.pSampleMask & ((1 << pipeline->sample_count) - 1);
     } else {
        dw[1] = (1 << pipeline->sample_count) - 1;
     }

    pipeline->cmd_sample_mask = dw[1];
}

static void pipeline_build_cb(struct intel_pipeline *pipeline,
                              const struct intel_pipeline_create_info *info)
{
    uint32_t i;

    INTEL_GPU_ASSERT(pipeline->dev->gpu, 6, 7.5);
    STATIC_ASSERT(ARRAY_SIZE(pipeline->cmd_cb) >= INTEL_MAX_RENDER_TARGETS*2);
    assert(info->cb.attachmentCount <= INTEL_MAX_RENDER_TARGETS);

    uint32_t *dw = pipeline->cmd_cb;

    for (i = 0; i < info->cb.attachmentCount; i++) {
        const VkPipelineColorBlendAttachmentState *att = &info->cb.pAttachments[i];
        uint32_t dw0, dw1;


        dw0 = 0;
        dw1 = GEN6_RT_DW1_COLORCLAMP_RTFORMAT |
              GEN6_RT_DW1_PRE_BLEND_CLAMP |
              GEN6_RT_DW1_POST_BLEND_CLAMP;

        if (att->blendEnable) {
            dw0 = 1 << 31 |
                    translate_blend_func(att->alphaBlendOp) << 26 |
                    translate_blend(att->srcAlphaBlendFactor) << 20 |
                    translate_blend(att->dstAlphaBlendFactor) << 15 |
                    translate_blend_func(att->colorBlendOp) << 11 |
                    translate_blend(att->srcColorBlendFactor) << 5 |
                    translate_blend(att->dstColorBlendFactor);

            if (att->alphaBlendOp != att->colorBlendOp ||
                att->srcAlphaBlendFactor != att->srcColorBlendFactor ||
                att->dstAlphaBlendFactor != att->dstColorBlendFactor)
                dw0 |= 1 << 30;

            pipeline->dual_source_blend_enable = icd_pipeline_cb_att_needs_dual_source_blending(att);
        }

        if (info->cb.logicOpEnable && info->cb.logicOp != VK_LOGIC_OP_COPY) {
            int logicop;

            switch (info->cb.logicOp) {
            case VK_LOGIC_OP_CLEAR:            logicop = GEN6_LOGICOP_CLEAR; break;
            case VK_LOGIC_OP_AND:              logicop = GEN6_LOGICOP_AND; break;
            case VK_LOGIC_OP_AND_REVERSE:      logicop = GEN6_LOGICOP_AND_REVERSE; break;
            case VK_LOGIC_OP_AND_INVERTED:     logicop = GEN6_LOGICOP_AND_INVERTED; break;
            case VK_LOGIC_OP_NO_OP:             logicop = GEN6_LOGICOP_NOOP; break;
            case VK_LOGIC_OP_XOR:              logicop = GEN6_LOGICOP_XOR; break;
            case VK_LOGIC_OP_OR:               logicop = GEN6_LOGICOP_OR; break;
            case VK_LOGIC_OP_NOR:              logicop = GEN6_LOGICOP_NOR; break;
            case VK_LOGIC_OP_EQUIVALENT:            logicop = GEN6_LOGICOP_EQUIV; break;
            case VK_LOGIC_OP_INVERT:           logicop = GEN6_LOGICOP_INVERT; break;
            case VK_LOGIC_OP_OR_REVERSE:       logicop = GEN6_LOGICOP_OR_REVERSE; break;
            case VK_LOGIC_OP_COPY_INVERTED:    logicop = GEN6_LOGICOP_COPY_INVERTED; break;
            case VK_LOGIC_OP_OR_INVERTED:      logicop = GEN6_LOGICOP_OR_INVERTED; break;
            case VK_LOGIC_OP_NAND:             logicop = GEN6_LOGICOP_NAND; break;
            case VK_LOGIC_OP_SET:              logicop = GEN6_LOGICOP_SET; break;
            default:
                assert(!"unknown logic op");
                logicop = GEN6_LOGICOP_CLEAR;
                break;
            }

            dw1 |= GEN6_RT_DW1_LOGICOP_ENABLE |
                   logicop << GEN6_RT_DW1_LOGICOP_FUNC__SHIFT;
        }

        if (!(att->colorWriteMask & 0x1))
            dw1 |= GEN6_RT_DW1_WRITE_DISABLE_R;
        if (!(att->colorWriteMask & 0x2))
            dw1 |= GEN6_RT_DW1_WRITE_DISABLE_G;
        if (!(att->colorWriteMask & 0x4))
            dw1 |= GEN6_RT_DW1_WRITE_DISABLE_B;
        if (!(att->colorWriteMask & 0x8))
            dw1 |= GEN6_RT_DW1_WRITE_DISABLE_A;

        dw[2 * i] = dw0;
        dw[2 * i + 1] = dw1;
    }

    for (i=info->cb.attachmentCount; i < INTEL_MAX_RENDER_TARGETS; i++)
    {
        dw[2 * i] = 0;
        dw[2 * i + 1] = GEN6_RT_DW1_COLORCLAMP_RTFORMAT |
                GEN6_RT_DW1_PRE_BLEND_CLAMP |
                GEN6_RT_DW1_POST_BLEND_CLAMP |
                GEN6_RT_DW1_WRITE_DISABLE_R |
                GEN6_RT_DW1_WRITE_DISABLE_G |
                GEN6_RT_DW1_WRITE_DISABLE_B |
                GEN6_RT_DW1_WRITE_DISABLE_A;
    }

}

static void pipeline_build_state(struct intel_pipeline *pipeline,
                                 const struct intel_pipeline_create_info *info)
{
    if (info->use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_VIEWPORT) {
        pipeline->state.viewport.viewport_count = info->vp.viewportCount;
        memcpy(pipeline->state.viewport.viewports, info->vp.pViewports, info->vp.viewportCount * sizeof(VkViewport));
    }
    if (info->use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_SCISSOR) {
        pipeline->state.viewport.scissor_count = info->vp.scissorCount;
        memcpy(pipeline->state.viewport.scissors, info->vp.pScissors, info->vp.scissorCount * sizeof(VkRect2D));
    }
    if (info->use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_LINE_WIDTH) {
        pipeline->state.line_width.line_width = info->rs.lineWidth;
    }
    if (info->use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BIAS) {
        pipeline->state.depth_bias.depth_bias = info->rs.depthBiasConstantFactor;
        pipeline->state.depth_bias.depth_bias_clamp = info->rs.depthBiasClamp;
        pipeline->state.depth_bias.slope_scaled_depth_bias = info->rs.depthBiasSlopeFactor;
    }
    if (info->use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_BLEND_CONSTANTS) {
        pipeline->state.blend.blend_const[0] = info->cb.blendConstants[0];
        pipeline->state.blend.blend_const[1] = info->cb.blendConstants[1];
        pipeline->state.blend.blend_const[2] = info->cb.blendConstants[2];
        pipeline->state.blend.blend_const[3] = info->cb.blendConstants[3];
    }
    if (info->use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BOUNDS) {
        pipeline->state.depth_bounds.min_depth_bounds = info->db.minDepthBounds;
        pipeline->state.depth_bounds.max_depth_bounds = info->db.maxDepthBounds;
    }
    if (info->use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK) {
        pipeline->state.stencil.front.stencil_compare_mask = info->db.front.compareMask;
        pipeline->state.stencil.back.stencil_compare_mask = info->db.back.compareMask;
    }
    if (info->use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK) {

        pipeline->state.stencil.front.stencil_write_mask = info->db.front.writeMask;
        pipeline->state.stencil.back.stencil_write_mask = info->db.back.writeMask;
    }
    if (info->use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_STENCIL_REFERENCE) {
        pipeline->state.stencil.front.stencil_reference = info->db.front.reference;
        pipeline->state.stencil.back.stencil_reference = info->db.back.reference;
    }

    pipeline->state.use_pipeline_dynamic_state = info->use_pipeline_dynamic_state;
}


static VkResult pipeline_build_all(struct intel_pipeline *pipeline,
                                   const struct intel_pipeline_create_info *info)
{
    VkResult ret;

    pipeline_build_state(pipeline, info);

    ret = pipeline_build_shaders(pipeline, info);
    if (ret != VK_SUCCESS)
        return ret;

    /* TODOVV: Move test to validation layer
     *  This particular test is based on a limit imposed by
     *  INTEL_MAX_VERTEX_BINDING_COUNT, which should be migrated
     *  to API-defined maxVertexInputBindings setting and then
     *  this check can be in DeviceLimits layer
     */
    if (info->vi.vertexBindingDescriptionCount > ARRAY_SIZE(pipeline->vb) ||
        info->vi.vertexAttributeDescriptionCount > ARRAY_SIZE(pipeline->vb)) {
        return VK_ERROR_VALIDATION_FAILED;
    }

    pipeline->vb_count = info->vi.vertexBindingDescriptionCount;
    memcpy(pipeline->vb, info->vi.pVertexBindingDescriptions,
            sizeof(pipeline->vb[0]) * pipeline->vb_count);

    pipeline_build_vertex_elements(pipeline, info);
    pipeline_build_fragment_SBE(pipeline, info);
    pipeline_build_msaa(pipeline, info);
    pipeline_build_depth_stencil(pipeline, info);

    if (intel_gpu_gen(pipeline->dev->gpu) >= INTEL_GEN(7)) {
        pipeline_build_urb_alloc_gen7(pipeline, info);
        pipeline_build_gs(pipeline, info);
        pipeline_build_hs(pipeline, info);
        pipeline_build_te(pipeline, info);
        pipeline_build_ds(pipeline, info);

        pipeline->wa_flags = INTEL_CMD_WA_GEN6_PRE_DEPTH_STALL_WRITE |
                             INTEL_CMD_WA_GEN6_PRE_COMMAND_SCOREBOARD_STALL |
                             INTEL_CMD_WA_GEN7_PRE_VS_DEPTH_STALL_WRITE |
                             INTEL_CMD_WA_GEN7_POST_COMMAND_CS_STALL |
                             INTEL_CMD_WA_GEN7_POST_COMMAND_DEPTH_STALL;
    } else {
        pipeline_build_urb_alloc_gen6(pipeline, info);

        pipeline->wa_flags = INTEL_CMD_WA_GEN6_PRE_DEPTH_STALL_WRITE |
                             INTEL_CMD_WA_GEN6_PRE_COMMAND_SCOREBOARD_STALL;
    }

    ret = pipeline_build_ia(pipeline, info);

    if (ret == VK_SUCCESS)
        ret = pipeline_build_rs_state(pipeline, info);

    if (ret == VK_SUCCESS) {
        pipeline_build_cb(pipeline, info);
        pipeline->cb_state = info->cb;
        pipeline->tess_state = info->tess;
    }

    return ret;
}

static VkResult pipeline_create_info_init(struct intel_pipeline_create_info  *info,
                                          const VkGraphicsPipelineCreateInfo *vkinfo)
{
    memset(info, 0, sizeof(*info));

    /*
     * Do we need to set safe defaults in case the app doesn't provide all of
     * the necessary create infos?
     */
    info->ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    info->ms.pSampleMask = NULL;

    memcpy(&info->graphics, vkinfo, sizeof (info->graphics));

    void *dst;
    for (uint32_t i = 0; i < vkinfo->stageCount; i++) {
        const VkPipelineShaderStageCreateInfo *thisStage = &vkinfo->pStages[i];
        switch (thisStage->stage) {
            case VK_SHADER_STAGE_VERTEX_BIT:
                dst = &info->vs;
                break;
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                dst = &info->tcs;
                break;
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                dst = &info->tes;
                break;
            case VK_SHADER_STAGE_GEOMETRY_BIT:
                dst = &info->gs;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                dst = &info->fs;
                break;
            case VK_SHADER_STAGE_COMPUTE_BIT:
                dst = &info->compute;
                break;
            default:
                assert(!"unsupported shader stage");
                break;
        }
        memcpy(dst, thisStage, sizeof(VkPipelineShaderStageCreateInfo));
    }

    if (vkinfo->pVertexInputState != NULL) {
        memcpy(&info->vi, vkinfo->pVertexInputState, sizeof (info->vi));
    }
    if (vkinfo->pInputAssemblyState != NULL) {
        memcpy(&info->ia, vkinfo->pInputAssemblyState, sizeof (info->ia));
    }
    if (vkinfo->pDepthStencilState != NULL) {
        memcpy(&info->db, vkinfo->pDepthStencilState, sizeof (info->db));
    }
    if (vkinfo->pColorBlendState != NULL) {
        memcpy(&info->cb, vkinfo->pColorBlendState, sizeof (info->cb));
    }
    if (vkinfo->pRasterizationState != NULL) {
        memcpy(&info->rs, vkinfo->pRasterizationState, sizeof (info->rs));
    }
    if (vkinfo->pTessellationState != NULL) {
        memcpy(&info->tess, vkinfo->pTessellationState, sizeof (info->tess));
    }
    if (vkinfo->pMultisampleState != NULL) {
        memcpy(&info->ms, vkinfo->pMultisampleState, sizeof (info->ms));
    }
    if (vkinfo->pViewportState != NULL) {
        memcpy(&info->vp, vkinfo->pViewportState, sizeof (info->vp));
    }

    /* by default, take all dynamic state from the pipeline */
    info->use_pipeline_dynamic_state = INTEL_USE_PIPELINE_DYNAMIC_VIEWPORT |
                                       INTEL_USE_PIPELINE_DYNAMIC_SCISSOR |
                                       INTEL_USE_PIPELINE_DYNAMIC_BLEND_CONSTANTS |
                                       INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BIAS |
                                       INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BOUNDS |
                                       INTEL_USE_PIPELINE_DYNAMIC_LINE_WIDTH |
                                       INTEL_USE_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK |
                                       INTEL_USE_PIPELINE_DYNAMIC_STENCIL_REFERENCE |
                                       INTEL_USE_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK;
    if (vkinfo->pDynamicState != NULL) {
        for (uint32_t i = 0; i < vkinfo->pDynamicState->dynamicStateCount; i++) {
            /* Mark dynamic state indicated by app as not using pipeline state */
            switch (vkinfo->pDynamicState->pDynamicStates[i]) {
            case VK_DYNAMIC_STATE_VIEWPORT:
                info->use_pipeline_dynamic_state &= ~INTEL_USE_PIPELINE_DYNAMIC_VIEWPORT;
                break;
            case VK_DYNAMIC_STATE_SCISSOR:
                info->use_pipeline_dynamic_state &= ~INTEL_USE_PIPELINE_DYNAMIC_SCISSOR;
                break;
            case VK_DYNAMIC_STATE_LINE_WIDTH:
                info->use_pipeline_dynamic_state &= ~INTEL_USE_PIPELINE_DYNAMIC_LINE_WIDTH;
                break;
            case VK_DYNAMIC_STATE_DEPTH_BIAS:
                info->use_pipeline_dynamic_state &= ~INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BIAS;
                break;
            case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
                info->use_pipeline_dynamic_state &= ~INTEL_USE_PIPELINE_DYNAMIC_BLEND_CONSTANTS;
                break;
            case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
                info->use_pipeline_dynamic_state &= ~INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BOUNDS;
                break;
            case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
                info->use_pipeline_dynamic_state &= ~INTEL_USE_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK;
                break;
            case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
                info->use_pipeline_dynamic_state &= ~INTEL_USE_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK;
                break;
            case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
                info->use_pipeline_dynamic_state &= ~INTEL_USE_PIPELINE_DYNAMIC_STENCIL_REFERENCE;
                break;
            default:
                assert(!"Invalid dynamic state");
                break;
            }
        }
    }

    return VK_SUCCESS;
}

static VkResult graphics_pipeline_create(struct intel_dev *dev,
                                         const VkGraphicsPipelineCreateInfo *info_,
                                         struct intel_pipeline **pipeline_ret)
{
    struct intel_pipeline_create_info info;
    struct intel_pipeline *pipeline;
    VkResult ret;

    ret = pipeline_create_info_init(&info, info_);

    if (ret != VK_SUCCESS)
        return ret;

    pipeline = (struct intel_pipeline *) intel_base_create(&dev->base.handle,
                        sizeof (*pipeline), dev->base.dbg,
                        VK_OBJECT_TYPE_PIPELINE, info_, 0);
    if (!pipeline)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    pipeline->dev = dev;
    pipeline->pipeline_layout = intel_pipeline_layout(info.graphics.layout);

    pipeline->obj.destroy = pipeline_destroy;

    ret = pipeline_build_all(pipeline, &info);
    if (ret != VK_SUCCESS) {
        pipeline_destroy(&pipeline->obj);
        return ret;
    }

    VkMemoryAllocateInfo mem_reqs;
    mem_reqs.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_reqs.allocationSize = pipeline->scratch_size;
    mem_reqs.pNext = NULL;
    mem_reqs.memoryTypeIndex = 0;
    intel_mem_alloc(dev, &mem_reqs, &pipeline->obj.mem);

    *pipeline_ret = pipeline;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(
    VkDevice                                    device,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipelineCache*                            pPipelineCache)
{

    // non-dispatchable objects only need to be 64 bits currently
    *((uint64_t *)pPipelineCache) = 1;
    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    const VkAllocationCallbacks*                     pAllocator)
{
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    return VK_ERROR_VALIDATION_FAILED;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(
    VkDevice                                    device,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    return VK_ERROR_VALIDATION_FAILED;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
    VkDevice                                  device,
    VkPipelineCache                           pipelineCache,
    uint32_t                                  createInfoCount,
    const VkGraphicsPipelineCreateInfo*       pCreateInfos,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipeline*                               pPipelines)
{
    struct intel_dev *dev = intel_dev(device);
    uint32_t i;
    VkResult res = VK_SUCCESS;
    bool one_succeeded = false;

    for (i = 0; i < createInfoCount; i++) {
        res =  graphics_pipeline_create(dev, &(pCreateInfos[i]),
            (struct intel_pipeline **) &(pPipelines[i]));
        //return NULL handle for unsuccessful creates
        if (res != VK_SUCCESS)
            pPipelines[i] = VK_NULL_HANDLE;
        else
            one_succeeded = true;
    }
    //return VK_SUCCESS if any of count creates succeeded
    if (one_succeeded)
        return VK_SUCCESS;
    else
        return res;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
    VkDevice                                  device,
    VkPipelineCache                           pipelineCache,
    uint32_t                                  createInfoCount,
    const VkComputePipelineCreateInfo*        pCreateInfos,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipeline*                               pPipelines)
{
    return VK_ERROR_VALIDATION_FAILED;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(
    VkDevice                                device,
    VkPipeline                              pipeline,
    const VkAllocationCallbacks*                     pAllocator)

 {
    struct intel_obj *obj = intel_obj(pipeline);

    intel_mem_free(obj->mem);
    obj->destroy(obj);
 }
