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
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 *   Chia-I Wu <olv@lunarg.com>
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include "intel.h"
#include "obj.h"
#include "desc.h"
#include "dev.h"
#include "state.h"

enum intel_pipeline_shader_use {
    INTEL_SHADER_USE_VID                = (1 << 0),
    INTEL_SHADER_USE_IID                = (1 << 1),

    INTEL_SHADER_USE_KILL               = (1 << 2),
    INTEL_SHADER_USE_DEPTH              = (1 << 3),
    INTEL_SHADER_USE_W                  = (1 << 4),
};

/* This order must match Pixel Shader Computed Depth Mode in 3DSTATE_WM */
enum intel_computed_depth_mode {
    INTEL_COMPUTED_DEPTH_MODE_NONE,
    INTEL_COMPUTED_DEPTH_MODE_ON,
    INTEL_COMPUTED_DEPTH_MODE_ON_GE,
    INTEL_COMPUTED_DEPTH_MODE_ON_LE
};

enum intel_pipeline_rmap_slot_type {
    INTEL_PIPELINE_RMAP_UNUSED,
    INTEL_PIPELINE_RMAP_RT,
    INTEL_PIPELINE_RMAP_SURFACE,
    INTEL_PIPELINE_RMAP_SAMPLER,
};

struct intel_pipeline_rmap_slot {
    enum intel_pipeline_rmap_slot_type type;

    uint32_t index; /* in the render target array or layout chain */
    union {
        struct {
            struct intel_desc_offset offset;
            int dynamic_offset_index;
        } surface;
        struct intel_desc_offset sampler;
    } u;
};

/**
 * Shader resource mapping.
 */
struct intel_pipeline_rmap {
    /* this is not an intel_obj */

    uint32_t rt_count;
    uint32_t texture_resource_count;
    uint32_t resource_count;
    uint32_t uav_count;
    uint32_t sampler_count;

    /*
     * rt_count slots +
     * resource_count slots +
     * uav_count slots +
     * sampler_count slots
     */
    struct intel_pipeline_rmap_slot *slots;
    uint32_t slot_count;
};

#define SHADER_VERTEX_FLAG            VK_SHADER_STAGE_VERTEX_BIT
#define SHADER_TESS_CONTROL_FLAG      VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
#define SHADER_TESS_EVAL_FLAG         VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
#define SHADER_GEOMETRY_FLAG          VK_SHADER_STAGE_GEOMETRY_BIT
#define SHADER_FRAGMENT_FLAG          VK_SHADER_STAGE_FRAGMENT_BIT
#define SHADER_COMPUTE_FLAG           VK_SHADER_STAGE_COMPUTE_BIT

struct intel_pipeline_shader {
    /* this is not an intel_obj */

    void *pCode;
    uint32_t codeSize;

    /*
     * must grab everything we need from shader object as that
     * can go away after the pipeline is created
     */
    VkFlags uses;
    uint64_t inputs_read;
    uint64_t outputs_written;
    uint32_t outputs_offset;
    uint32_t generic_input_start;

    VkBool32 enable_user_clip;
    VkBool32 reads_user_clip;

    uint32_t in_count;
    uint32_t out_count;

    uint32_t sampler_count;
    uint32_t surface_count;

    uint32_t ubo_start;

    // geometry shader specific
    uint32_t output_size_hwords;
    uint32_t output_topology;
    uint32_t control_data_header_size_hwords;
    uint32_t control_data_format;
    VkBool32 include_primitive_id;
    int32_t  invocations;
    VkBool32 dual_instanced_dispatch;
    VkBool32 discard_adj;

    uint32_t urb_grf_start;
    uint32_t urb_grf_start_16;

    /* If present, where does the SIMD16 kernel start? */
    uint32_t offset_16;

    VkFlags barycentric_interps;
    VkFlags point_sprite_enables;

    VkDeviceSize per_thread_scratch_size;

    enum intel_computed_depth_mode computed_depth_mode;

    struct intel_pipeline_rmap *rmap;

    /* these are set up by the driver */
    uint32_t max_threads;
    VkDeviceSize scratch_offset;
};

/*
 * On GEN6, there are
 *
 *  - 3DSTATE_URB (3)
 *  - 3DSTATE_VERTEX_ELEMENTS (1+2*INTEL_MAX_VERTEX_ELEMENT_COUNT)
 *  - 3DSTATE_SAMPLE_MASK (2)
 *
 * On GEN7, there are
 *
 *  - 3DSTATE_URB_x (2*4)
 *  - 3DSTATE_VERTEX_ELEMENTS (1+2*INTEL_MAX_VERTEX_ELEMENT_COUNT)
 *  - 3DSTATE_SBE (14)
 *  - 3DSTATE_HS (7)
 *  - 3DSTATE_TE (4)
 *  - 3DSTATE_DS (6)
 *  - 3DSTATE_SAMPLE_MASK (2)
 */
#define INTEL_PSO_CMD_ENTRIES   128

/**
 * 3D pipeline.
 */
struct intel_pipeline {
    struct intel_obj obj;

    struct intel_dev *dev;

    const struct intel_pipeline_layout *pipeline_layout;

    VkVertexInputBindingDescription vb[INTEL_MAX_VERTEX_BINDING_COUNT];
    uint32_t vb_count;

    /* VkPipelineIaStateCreateInfo */
    VkPrimitiveTopology topology;
    int prim_type;
    bool disable_vs_cache;
    bool primitive_restart;
    uint32_t primitive_restart_index;

    // TODO: This should probably be Intel HW state, not VK state.
    /* Depth Buffer format */
    VkFormat db_format;

    VkPipelineColorBlendStateCreateInfo cb_state;

    // VkPipelineRsStateCreateInfo rs_state;
    bool depthClipEnable;
    bool rasterizerDiscardEnable;
    bool depthBiasEnable;

    bool alphaToCoverageEnable;

    VkPipelineTessellationStateCreateInfo tess_state;

    uint32_t active_shaders;
    struct intel_pipeline_shader vs;
    struct intel_pipeline_shader tcs;
    struct intel_pipeline_shader tes;
    struct intel_pipeline_shader gs;
    struct intel_pipeline_shader fs;
    struct intel_pipeline_shader cs;
    VkDeviceSize scratch_size;

    uint32_t wa_flags;

    uint32_t cmds[INTEL_PSO_CMD_ENTRIES];
    uint32_t cmd_len;

    bool dual_source_blend_enable;

    /* The following are only partial HW commands that will need
     * more processing before sending to the HW
     */
    // VkPipelineDsStateCreateInfo ds_state
    bool stencilTestEnable;

    /* Dynamic state specified at PSO create time */
    struct {
        VkFlags use_pipeline_dynamic_state;
        struct intel_dynamic_viewport viewport;
        struct intel_dynamic_line_width line_width;
        struct intel_dynamic_depth_bias depth_bias;
        struct intel_dynamic_blend blend;
        struct intel_dynamic_depth_bounds depth_bounds;
        struct intel_dynamic_stencil stencil;
    } state;

    uint32_t cmd_depth_stencil;
    uint32_t cmd_depth_test;

    uint32_t cmd_sf_fill;
    uint32_t cmd_clip_cull;
    uint32_t cmd_sf_cull;
    uint32_t cmd_cb[2 * INTEL_MAX_RENDER_TARGETS];
    uint32_t sample_count;
    uint32_t cmd_sample_mask;

    uint32_t cmd_3dstate_sbe[14];
};

static inline struct intel_pipeline *intel_pipeline(VkPipeline pipeline)
{
    return *(struct intel_pipeline **) &pipeline;
}

static inline struct intel_pipeline *intel_pipeline_from_base(struct intel_base *base)
{
    return (struct intel_pipeline *) base;
}

static inline struct intel_pipeline *intel_pipeline_from_obj(struct intel_obj *obj)
{
    return intel_pipeline_from_base(&obj->base);
}

struct intel_pipeline_shader *intel_pipeline_shader_create_meta(struct intel_dev *dev,
                                                                enum intel_dev_meta_shader id);
void intel_pipeline_shader_destroy(struct intel_dev *dev,
                                   struct intel_pipeline_shader *sh);

void intel_pipeline_init_default_sample_patterns(const struct intel_dev *dev,
                                                 uint8_t *pat_1x, uint8_t *pat_2x,
                                                 uint8_t *pat_4x, uint8_t *pat_8x,
                                                 uint8_t *pat_16x);

#endif /* PIPELINE_H */
