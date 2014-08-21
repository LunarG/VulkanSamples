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
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include "intel.h"
#include "obj.h"
#include "dev.h"

#define INTEL_MAX_DRAW_BUFFERS    8
#define INTEL_MAX_CONST_BUFFERS   (1 + 12)
#define INTEL_MAX_SAMPLER_VIEWS   16
#define INTEL_MAX_SAMPLERS        16
#define INTEL_MAX_SO_BINDINGS     64
#define INTEL_MAX_SO_BUFFERS      4
#define INTEL_MAX_VIEWPORTS       1

#define INTEL_MAX_VS_SURFACES        (INTEL_MAX_CONST_BUFFERS + INTEL_MAX_SAMPLER_VIEWS)
#define INTEL_VS_CONST_SURFACE(i)    (i)
#define INTEL_VS_TEXTURE_SURFACE(i)  (INTEL_MAX_CONST_BUFFERS  + i)

#define INTEL_MAX_GS_SURFACES        (INTEL_MAX_SO_BINDINGS)
#define INTEL_GS_SO_SURFACE(i)       (i)

#define INTEL_MAX_WM_SURFACES        (INTEL_MAX_DRAW_BUFFERS + INTEL_MAX_CONST_BUFFERS + INTEL_MAX_SAMPLER_VIEWS)
#define INTEL_WM_DRAW_SURFACE(i)     (i)
#define INTEL_WM_CONST_SURFACE(i)    (INTEL_MAX_DRAW_BUFFERS + i)
#define INTEL_WM_TEXTURE_SURFACE(i)  (INTEL_MAX_DRAW_BUFFERS + INTEL_MAX_CONST_BUFFERS  + i)

#define SHADER_VERTEX_FLAG            (1 << XGL_SHADER_STAGE_VERTEX)
#define SHADER_TESS_CONTROL_FLAG      (1 << XGL_SHADER_STAGE_TESS_CONTROL)
#define SHADER_TESS_EVAL_FLAG         (1 << XGL_SHADER_STAGE_TESS_EVALUATION)
#define SHADER_GEOMETRY_FLAG          (1 << XGL_SHADER_STAGE_GEOMETRY)
#define SHADER_FRAGMENT_FLAG          (1 << XGL_SHADER_STAGE_FRAGMENT)
#define SHADER_COMPUTE_FLAG           (1 << XGL_SHADER_STAGE_COMPUTE)

/**
 * 3D pipeline.
 */
struct intel_pipeline {
    struct intel_obj obj;

    struct intel_dev *dev;

    struct intel_bo *workaround_bo;

    uint32_t packed_sample_position_1x;
    uint32_t packed_sample_position_4x;
    uint32_t packed_sample_position_8x[2];

    bool has_gen6_wa_pipe_control;

    /* XGL IA_STATE */
    XGL_PIPELINE_IA_STATE_CREATE_INFO ia_state;
    int prim_type;
    bool primitive_restart;
    uint32_t primitive_restart_index;

    /* Index of provoking vertex for each prim type */
    int provoking_vertex_tri;
    int provoking_vertex_trifan;
    int provoking_vertex_line;

    // TODO: This should probably be Intel HW state, not XGL state.
    /* Depth Buffer format */
    XGL_FORMAT db_format;

    XGL_PIPELINE_CB_STATE cb_state;

    // XGL_PIPELINE_RS_STATE_CREATE_INFO rs_state;
    bool depthClipEnable;
    bool rasterizerDiscardEnable;
    float pointSize;

    XGL_PIPELINE_TESS_STATE_CREATE_INFO tess_state;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO shader_state;

    uint32_t active_shaders;
    XGL_PIPELINE_SHADER vs;
    XGL_PIPELINE_SHADER fs;
    XGL_PIPELINE_SHADER gs;
    XGL_PIPELINE_SHADER tess_control;
    XGL_PIPELINE_SHADER tess_eval;
    XGL_PIPELINE_SHADER compute;

    XGL_SIZE total_size;  // FB memory app needs to allocate for this pipeline

    int reduced_prim;
    int so_num_vertices, so_max_vertices;

    uint32_t SF_VIEWPORT;
    uint32_t CLIP_VIEWPORT;
    uint32_t SF_CLIP_VIEWPORT; /* GEN7+ */
    uint32_t CC_VIEWPORT;

    uint32_t COLOR_CALC_STATE;
    uint32_t BLEND_STATE;
    uint32_t DEPTH_STENCIL_STATE;

    uint32_t SCISSOR_RECT;

    struct {
        uint32_t BINDING_TABLE_STATE;
        int BINDING_TABLE_STATE_size;
        uint32_t SURFACE_STATE[INTEL_MAX_VS_SURFACES];
        uint32_t SAMPLER_STATE;
        uint32_t SAMPLER_BORDER_COLOR_STATE[INTEL_MAX_SAMPLERS];
        uint32_t PUSH_CONSTANT_BUFFER;
        int PUSH_CONSTANT_BUFFER_size;
    } vs_state;

    struct {
        uint32_t BINDING_TABLE_STATE;
        int BINDING_TABLE_STATE_size;
        uint32_t SURFACE_STATE[INTEL_MAX_GS_SURFACES];
        bool active;
    } gs_state;

    struct {
        uint32_t BINDING_TABLE_STATE;
        int BINDING_TABLE_STATE_size;
        uint32_t SURFACE_STATE[INTEL_MAX_WM_SURFACES];
        uint32_t SAMPLER_STATE;
        uint32_t SAMPLER_BORDER_COLOR_STATE[INTEL_MAX_SAMPLERS];
        uint32_t PUSH_CONSTANT_BUFFER;
        int PUSH_CONSTANT_BUFFER_size;
    } wm_state;
};

static inline struct intel_pipeline *intel_pipeline(XGL_PIPELINE pipeline)
{
    return (struct intel_pipeline *) pipeline;
}

static inline struct intel_pipeline *intel_pipeline_from_base(struct intel_base *base)
{
    return (struct intel_pipeline *) base;
}

static inline struct intel_pipeline *intel_pipeline_from_obj(struct intel_obj *obj)
{
    return intel_pipeline_from_base(&obj->base);
}

XGL_RESULT XGLAPI intelCreateGraphicsPipeline(
    XGL_DEVICE                                  device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    XGL_PIPELINE*                               pPipeline);

XGL_RESULT XGLAPI intelCreateComputePipeline(
    XGL_DEVICE                                  device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO*     pCreateInfo,
    XGL_PIPELINE*                               pPipeline);

XGL_RESULT XGLAPI intelStorePipeline(
    XGL_PIPELINE                                pipeline,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

XGL_RESULT XGLAPI intelLoadPipeline(
    XGL_DEVICE                                  device,
    XGL_SIZE                                    dataSize,
    const XGL_VOID*                             pData,
    XGL_PIPELINE*                               pPipeline);

XGL_RESULT XGLAPI intelCreatePipelineDelta(
    XGL_DEVICE                                  device,
    XGL_PIPELINE                                p1,
    XGL_PIPELINE                                p2,
    XGL_PIPELINE_DELTA*                         delta);
#endif // PIPELINE_H
