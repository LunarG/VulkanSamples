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

#include "shader.h"
#include "pipeline_priv.h"
#include "genhw/genhw.h"
#include "genhw/gen_render_3d.xml.h"

struct intel_pipeline_builder {
    const struct intel_gpu *gpu;

    XGL_GRAPHICS_PIPELINE_CREATE_INFO   graphics;
    XGL_PIPELINE_IA_STATE_CREATE_INFO   ia;
    XGL_PIPELINE_DB_STATE_CREATE_INFO   db;
    XGL_PIPELINE_CB_STATE               cb;
    XGL_PIPELINE_RS_STATE_CREATE_INFO   rs;
    XGL_PIPELINE_TESS_STATE_CREATE_INFO tess;
    XGL_PIPELINE_SHADER                 vs;
    XGL_PIPELINE_SHADER                 tcs;
    XGL_PIPELINE_SHADER                 tes;
    XGL_PIPELINE_SHADER                 gs;
    XGL_PIPELINE_SHADER                 fs;

    XGL_COMPUTE_PIPELINE_CREATE_INFO    compute;
    XGL_PIPELINE_SHADER                 cs;
};

struct intel_pipeline_builder_create_info {
    XGL_STRUCTURE_TYPE struct_type;
    XGL_VOID *next;
};

static XGL_RESULT pipeline_ia_state(struct intel_pipeline *pipeline,
                                    const XGL_PIPELINE_IA_STATE_CREATE_INFO* ia_state)
{
    pipeline->ia_state = *ia_state;

    if (ia_state->provokingVertex == XGL_PROVOKING_VERTEX_FIRST) {
        pipeline->provoking_vertex_tri = 0;
        pipeline->provoking_vertex_trifan = 1;
        pipeline->provoking_vertex_line = 0;
    } else {
        pipeline->provoking_vertex_tri = 2;
        pipeline->provoking_vertex_trifan = 2;
        pipeline->provoking_vertex_line = 1;
    }

    switch (ia_state->topology) {
    case XGL_TOPOLOGY_POINT_LIST:
        pipeline->prim_type = GEN6_3DPRIM_POINTLIST;
        break;
    case XGL_TOPOLOGY_LINE_LIST:
        pipeline->prim_type = GEN6_3DPRIM_LINELIST;
        break;
    case XGL_TOPOLOGY_LINE_STRIP:
        pipeline->prim_type = GEN6_3DPRIM_LINESTRIP;
        break;
    case XGL_TOPOLOGY_TRIANGLE_LIST:
        pipeline->prim_type = GEN6_3DPRIM_TRILIST;
        break;
    case XGL_TOPOLOGY_TRIANGLE_STRIP:
        pipeline->prim_type = GEN6_3DPRIM_TRISTRIP;
        break;
    case XGL_TOPOLOGY_RECT_LIST:
        /*
         * TODO: Rect lists are special in XGL, do we need to do
         * something special here?
         * XGL Guide:
         * The rectangle list is a special geometry primitive type
         * that can be used for implementing post-processing techniques
         * or efficient copy operations. There are some special limitations
         * for rectangle primitives. They cannot be clipped, must
         * be axis aligned and cannot have depth gradient.
         * Failure to comply with these restrictions results in
         * undefined rendering results.
         */
        pipeline->prim_type = GEN6_3DPRIM_RECTLIST;
        break;
    case XGL_TOPOLOGY_QUAD_LIST:
        pipeline->prim_type = GEN6_3DPRIM_QUADLIST;
        break;
    case XGL_TOPOLOGY_QUAD_STRIP:
        pipeline->prim_type = GEN6_3DPRIM_QUADSTRIP;
        break;
    case XGL_TOPOLOGY_LINE_LIST_ADJ:
        pipeline->prim_type = GEN6_3DPRIM_LINELIST_ADJ;
        break;
    case XGL_TOPOLOGY_LINE_STRIP_ADJ:
        pipeline->prim_type = GEN6_3DPRIM_LINESTRIP_ADJ;
        break;
    case XGL_TOPOLOGY_TRIANGLE_LIST_ADJ:
        pipeline->prim_type = GEN6_3DPRIM_TRILIST_ADJ;
        break;
    case XGL_TOPOLOGY_TRIANGLE_STRIP_ADJ:
        pipeline->prim_type = GEN6_3DPRIM_TRISTRIP_ADJ;
        break;
    case XGL_TOPOLOGY_PATCH:
        // TODO: implement something here
        break;
    default:
        return XGL_ERROR_BAD_PIPELINE_DATA;
    }

    if (ia_state->primitiveRestartEnable) {
        pipeline->primitive_restart = true;
        pipeline->primitive_restart_index = ia_state->primitiveRestartIndex;
    } else {
        pipeline->primitive_restart = false;
    }

    if (ia_state->disableVertexReuse) {
        // TODO: What do we do to disable vertex reuse?
    }

    return XGL_SUCCESS;
}

static XGL_RESULT pipeline_rs_state(struct intel_pipeline *pipeline,
                                    const XGL_PIPELINE_RS_STATE_CREATE_INFO* rs_state)
{
    pipeline->depthClipEnable = rs_state->depthClipEnable;
    pipeline->rasterizerDiscardEnable = rs_state->rasterizerDiscardEnable;
    pipeline->pointSize = rs_state->pointSize;
    return XGL_SUCCESS;
}

static void pipeline_destroy(struct intel_obj *obj)
{
    struct intel_pipeline *pipeline = intel_pipeline_from_obj(obj);

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
        intel_rmap_destroy(pipeline->vs_rmap);
    if (pipeline->fs_rmap)
        intel_rmap_destroy(pipeline->fs_rmap);

    intel_base_destroy(&pipeline->obj.base);
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
        pipeline->intel_vs.pCode = kernel;
        pipeline->intel_vs.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_VERTEX_FLAG;
        pipeline->vs_rmap = intel_rmap_create(pipeline->dev,
                &info->descriptorSetMapping[0],
                &info->dynamicMemoryViewMapping, 0);
        if (!pipeline->vs_rmap) {
            icd_free(kernel);
            return XGL_ERROR_OUT_OF_MEMORY;
        }
        break;
    case XGL_SHADER_STAGE_GEOMETRY:
        pipeline->gs.pCode = kernel;
        pipeline->gs.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_GEOMETRY_FLAG;
        break;
    case XGL_SHADER_STAGE_FRAGMENT:
        pipeline->fs = *info;
        pipeline->intel_fs.pCode = kernel;
        pipeline->intel_fs.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_FRAGMENT_FLAG;
        /* assuming one RT; need to parse the shader */
        pipeline->fs_rmap = intel_rmap_create(pipeline->dev,
                &info->descriptorSetMapping[0],
                &info->dynamicMemoryViewMapping, 1);
        if (!pipeline->fs_rmap) {
            icd_free(kernel);
            return XGL_ERROR_OUT_OF_MEMORY;
        }
        break;
    case XGL_SHADER_STAGE_TESS_CONTROL:
        pipeline->tess_control.pCode = kernel;
        pipeline->tess_control.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_TESS_CONTROL_FLAG;
        break;
    case XGL_SHADER_STAGE_TESS_EVALUATION:
        pipeline->tess_eval.pCode = kernel;
        pipeline->tess_eval.codeSize = sh->ir->size;
        pipeline->active_shaders |= SHADER_TESS_EVAL_FLAG;
        break;
    case XGL_SHADER_STAGE_COMPUTE:
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

static XGL_RESULT builder_validate(const struct intel_pipeline_builder *builder,
                                   const struct intel_pipeline *pipeline)
{
    /*
     * Validate required elements
     */
    if (!(pipeline->active_shaders & SHADER_VERTEX_FLAG)) {
        // TODO: Log debug message: Vertex Shader required.
        return XGL_ERROR_BAD_PIPELINE_DATA;
    }

    /*
     * Tessalation control and evaluation have to both have a shader defined or
     * neither should have a shader defined.
     */
    if (((pipeline->active_shaders & SHADER_TESS_CONTROL_FLAG) == 0) !=
         ((pipeline->active_shaders & SHADER_TESS_EVAL_FLAG) == 0) ) {
        // TODO: Log debug message: Both Tess control and Tess eval are required to use tessalation
        return XGL_ERROR_BAD_PIPELINE_DATA;
    }

    if ((pipeline->active_shaders & SHADER_COMPUTE_FLAG) &&
        (pipeline->active_shaders & (SHADER_VERTEX_FLAG | SHADER_TESS_CONTROL_FLAG |
                                     SHADER_TESS_EVAL_FLAG | SHADER_GEOMETRY_FLAG |
                                     SHADER_FRAGMENT_FLAG))) {
        // TODO: Log debug message: Can only specify compute shader when doing compute
        return XGL_ERROR_BAD_PIPELINE_DATA;
    }

    /*
     * XGL_TOPOLOGY_PATCH primitive topology is only valid for tessellation pipelines.
     * Mismatching primitive topology and tessellation fails graphics pipeline creation.
     */
    if (pipeline->active_shaders & (SHADER_TESS_CONTROL_FLAG | SHADER_TESS_EVAL_FLAG) &&
        (pipeline->ia_state.topology != XGL_TOPOLOGY_PATCH)) {
        // TODO: Log debug message: Invalid topology used with tessalation shader.
        return XGL_ERROR_BAD_PIPELINE_DATA;
    }

    if ((pipeline->ia_state.topology == XGL_TOPOLOGY_PATCH) &&
            (pipeline->active_shaders & ~(SHADER_TESS_CONTROL_FLAG | SHADER_TESS_EVAL_FLAG))) {
        // TODO: Log debug message: Cannot use TOPOLOGY_PATCH on non-tessalation shader.
        return XGL_ERROR_BAD_PIPELINE_DATA;
    }

    return XGL_SUCCESS;
}

static void builder_build_urb_alloc_gen6(struct intel_pipeline_builder *builder,
                                         struct intel_pipeline *pipeline)
{
    const int urb_size = ((builder->gpu->gt == 2) ? 64 : 32) * 1024;
    const struct intel_shader *vs = intel_shader(builder->vs.shader);
    const struct intel_shader *gs = intel_shader(builder->gs.shader);
    int vs_entry_size, gs_entry_size;
    int vs_size, gs_size;

    INTEL_GPU_ASSERT(builder->gpu, 6, 6);

    vs_entry_size = ((vs->in_count >= vs->out_count) ?
        vs->in_count : vs->out_count);
    gs_entry_size = (gs) ? gs->out_count : 0;

    /* in bytes */
    vs_entry_size *= sizeof(float) * 4;
    gs_entry_size *= sizeof(float) * 4;

    if (gs) {
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

        STATIC_ASSERT(ARRAY_SIZE(pipeline->cmd_urb_alloc) >= cmd_len);
        pipeline->cmd_urb_alloc_len = cmd_len;
        dw = pipeline->cmd_urb_alloc;

        dw[0] = dw0;
        dw[1] = (vs_alloc_size - 1) << GEN6_URB_DW1_VS_ENTRY_SIZE__SHIFT |
                vs_entry_count << GEN6_URB_DW1_VS_ENTRY_COUNT__SHIFT;
        dw[2] = gs_entry_count << GEN6_URB_DW2_GS_ENTRY_COUNT__SHIFT |
                (gs_alloc_size - 1) << GEN6_URB_DW2_GS_ENTRY_SIZE__SHIFT;
    }
}

static void builder_build_urb_alloc_gen7(struct intel_pipeline_builder *builder,
                                         struct intel_pipeline *pipeline)
{
    const int urb_size = ((builder->gpu->gt == 3) ? 512 :
                          (builder->gpu->gt == 2) ? 256 : 128) * 1024;
    const struct intel_shader *vs = intel_shader(builder->vs.shader);
    const struct intel_shader *gs = intel_shader(builder->gs.shader);
    /* some space is reserved for PCBs */
    int urb_offset = ((builder->gpu->gt == 3) ? 32 : 16) * 1024;
    int vs_entry_size, gs_entry_size;
    int vs_size, gs_size;

    INTEL_GPU_ASSERT(builder->gpu, 7, 7.5);

    vs_entry_size = ((vs->in_count >= vs->out_count) ?
        vs->in_count : vs->out_count);
    gs_entry_size = (gs) ? gs->out_count : 0;

    /* in bytes */
    vs_entry_size *= sizeof(float) * 4;
    gs_entry_size *= sizeof(float) * 4;

    if (gs) {
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

        if (intel_gpu_gen(builder->gpu) >= INTEL_GEN(7.5)) {
            const int max_vs_entry_count =
                (builder->gpu->gt >= 2) ? 1644 : 640;
            const int max_gs_entry_count =
                (builder->gpu->gt >= 2) ? 640 : 256;
            if (vs_entry_count >= max_vs_entry_count)
                vs_entry_count = max_vs_entry_count;
            if (gs_entry_count >= max_gs_entry_count)
                gs_entry_count = max_gs_entry_count;
        } else {
            const int max_vs_entry_count =
                (builder->gpu->gt == 2) ? 704 : 512;
            const int max_gs_entry_count =
                (builder->gpu->gt == 2) ? 320 : 192;
            if (vs_entry_count >= max_vs_entry_count)
                vs_entry_count = max_vs_entry_count;
            if (gs_entry_count >= max_gs_entry_count)
                gs_entry_count = max_gs_entry_count;
        }

        STATIC_ASSERT(ARRAY_SIZE(pipeline->cmd_urb_alloc) >= cmd_len * 4);
        pipeline->cmd_urb_alloc_len = cmd_len * 4;

        dw = pipeline->cmd_urb_alloc;
        dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_VS) | (cmd_len - 2);
        dw[1] = (urb_offset / 8192) << GEN7_URB_ANY_DW1_OFFSET__SHIFT |
                (vs_alloc_size - 1) << GEN7_URB_ANY_DW1_ENTRY_SIZE__SHIFT |
                vs_entry_count;

        dw += 2;
        if (gs_size)
            urb_offset += vs_size;
        dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_GS) | (cmd_len - 2);
        dw[1] = (urb_offset  / 8192) << GEN7_URB_ANY_DW1_OFFSET__SHIFT |
                (gs_alloc_size - 1) << GEN7_URB_ANY_DW1_ENTRY_SIZE__SHIFT |
                gs_entry_count;

        dw += 2;
        dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_HS) | (cmd_len - 2);
        dw[1] = (urb_offset / 8192)  << GEN7_URB_ANY_DW1_OFFSET__SHIFT;

        dw += 2;
        dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_DS) | (cmd_len - 2);
        dw[1] = (urb_offset / 8192)  << GEN7_URB_ANY_DW1_OFFSET__SHIFT;
    }
}

static XGL_RESULT builder_build_all(struct intel_pipeline_builder *builder,
                                    struct intel_pipeline *pipeline)
{
    XGL_RESULT ret;

    if (intel_gpu_gen(builder->gpu) >= INTEL_GEN(7)) {
        builder_build_urb_alloc_gen7(builder, pipeline);
    } else {
        builder_build_urb_alloc_gen6(builder, pipeline);
    }

    ret = pipeline_ia_state(pipeline, &builder->ia);

    if (ret == XGL_SUCCESS)
        ret = pipeline_rs_state(pipeline, &builder->rs);

    if (ret == XGL_SUCCESS && builder->vs.shader)
        ret = pipeline_shader(pipeline, &builder->vs);
    if (ret == XGL_SUCCESS && builder->tcs.shader)
        ret = pipeline_shader(pipeline, &builder->tcs);
    if (ret == XGL_SUCCESS && builder->tes.shader)
        ret = pipeline_shader(pipeline, &builder->tes);
    if (ret == XGL_SUCCESS && builder->gs.shader)
        ret = pipeline_shader(pipeline, &builder->gs);
    if (ret == XGL_SUCCESS && builder->fs.shader)
        ret = pipeline_shader(pipeline, &builder->fs);

    if (ret == XGL_SUCCESS) {
        pipeline->db_format = builder->db.format;
        pipeline->cb_state = builder->cb;
        pipeline->tess_state = builder->tess;
    }

    return ret;
}

static XGL_RESULT builder_init(struct intel_pipeline_builder *builder,
                               const struct intel_gpu *gpu,
                               const struct intel_pipeline_builder_create_info *info)
{
    memset(builder, 0, sizeof(*builder));

    builder->gpu = gpu;

    while (info) {
        const void *src = (const void *) info;
        XGL_SIZE size;
        void *dst;

        switch (info->struct_type) {
        case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
            size = sizeof(builder->graphics);
            dst = &builder->graphics;
            break;
        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
            size = sizeof(builder->ia);
            dst = &builder->ia;
            break;
        case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:
            size = sizeof(builder->db);
            dst = &builder->db;
            break;
        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            size = sizeof(builder->cb);
            dst = &builder->cb;
            break;
        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
            size = sizeof(builder->rs);
            dst = &builder->rs;
            break;
        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
            size = sizeof(builder->tess);
            dst = &builder->tess;
            break;
        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            {
                const XGL_PIPELINE_SHADER *shader = (const XGL_PIPELINE_SHADER *) (info + 1);

                src = (const void *) shader;
                size = sizeof(*shader);

                switch (shader->stage) {
                case XGL_SHADER_STAGE_VERTEX:
                    dst = &builder->vs;
                    break;
                case XGL_SHADER_STAGE_TESS_CONTROL:
                    dst = &builder->tcs;
                    break;
                case XGL_SHADER_STAGE_TESS_EVALUATION:
                    dst = &builder->tes;
                    break;
                case XGL_SHADER_STAGE_GEOMETRY:
                    dst = &builder->gs;
                    break;
                case XGL_SHADER_STAGE_FRAGMENT:
                    dst = &builder->fs;
                    break;
                case XGL_SHADER_STAGE_COMPUTE:
                    dst = &builder->cs;
                    break;
                default:
                    return XGL_ERROR_BAD_PIPELINE_DATA;
                    break;
                }
            }
            break;
        case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
            size = sizeof(builder->compute);
            dst = &builder->compute;
            break;
        default:
            return XGL_ERROR_BAD_PIPELINE_DATA;
            break;
        }

        memcpy(dst, src, size);

        info = info->next;
    }

    return XGL_SUCCESS;
}

static XGL_RESULT graphics_pipeline_create(struct intel_dev *dev,
                                           const XGL_GRAPHICS_PIPELINE_CREATE_INFO *info,
                                           struct intel_pipeline **pipeline_ret)
{
    struct intel_pipeline_builder builder;
    struct intel_pipeline *pipeline;
    XGL_RESULT ret;

    ret = builder_init(&builder, dev->gpu,
            (const struct intel_pipeline_builder_create_info *) info);
    if (ret != XGL_SUCCESS)
        return ret;

    pipeline = (struct intel_pipeline *)
        intel_base_create(dev, sizeof(*pipeline), dev->base.dbg,
                XGL_DBG_OBJECT_GRAPHICS_PIPELINE, info, 0);
    if (!pipeline)
        return XGL_ERROR_OUT_OF_MEMORY;

    pipeline->dev = dev;
    pipeline->obj.destroy = pipeline_destroy;
    pipeline->total_size = 0;

    ret = builder_build_all(&builder, pipeline);
    if (ret == XGL_SUCCESS)
        ret = builder_validate(&builder, pipeline);
    if (ret != XGL_SUCCESS) {
        pipeline_destroy(&pipeline->obj);
        return ret;
    }

    *pipeline_ret = pipeline;

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelCreateGraphicsPipeline(
    XGL_DEVICE                                  device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    XGL_PIPELINE*                               pPipeline)
{
    struct intel_dev *dev = intel_dev(device);

    return graphics_pipeline_create(dev, pCreateInfo,
            (struct intel_pipeline **) pPipeline);
}

XGL_RESULT XGLAPI intelCreateComputePipeline(
    XGL_DEVICE                                  device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO*     pCreateInfo,
    XGL_PIPELINE*                               pPipeline)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI intelStorePipeline(
    XGL_PIPELINE                                pipeline,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI intelLoadPipeline(
    XGL_DEVICE                                  device,
    XGL_SIZE                                    dataSize,
    const XGL_VOID*                             pData,
    XGL_PIPELINE*                               pPipeline)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI intelCreatePipelineDelta(
    XGL_DEVICE                                  device,
    XGL_PIPELINE                                p1,
    XGL_PIPELINE                                p2,
    XGL_PIPELINE_DELTA*                         delta)
{
    return XGL_ERROR_UNAVAILABLE;
}
