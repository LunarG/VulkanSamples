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

#include "pipeline.h"
#include "brw_defines.h"

static XGL_RESULT intelPipelineIAState(struct intel_dev *dev, struct intel_pipeline *pipeline,
                                       const XGL_PIPELINE_IA_STATE_CREATE_INFO* ia_state)
{
    if (ia_state->provokingVertex == XGL_PROVOKING_VERTEX_FIRST) {
        pipeline->provoking_vertex_tri = BRW_PROVOKING_VERTEX_0;
        pipeline->provoking_vertex_trifan = BRW_PROVOKING_VERTEX_1;
        pipeline->provoking_vertex_line = BRW_PROVOKING_VERTEX_0;
    } else {
        pipeline->provoking_vertex_tri = BRW_PROVOKING_VERTEX_2;
        pipeline->provoking_vertex_trifan = BRW_PROVOKING_VERTEX_2;
        pipeline->provoking_vertex_line = BRW_PROVOKING_VERTEX_1;
    }

    switch (ia_state->topology) {
    case XGL_TOPOLOGY_POINT_LIST:
        pipeline->prim_type = _3DPRIM_POINTLIST;
        break;
    case XGL_TOPOLOGY_LINE_LIST:
        pipeline->prim_type = _3DPRIM_LINELIST;
        break;
    case XGL_TOPOLOGY_LINE_STRIP:
        pipeline->prim_type = _3DPRIM_LINESTRIP;
        break;
    case XGL_TOPOLOGY_TRIANGLE_LIST:
        pipeline->prim_type = _3DPRIM_TRILIST;
        break;
    case XGL_TOPOLOGY_TRIANGLE_STRIP:
        pipeline->prim_type = _3DPRIM_TRISTRIP;
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
        pipeline->prim_type = _3DPRIM_RECTLIST;
        break;
    case XGL_TOPOLOGY_QUAD_LIST:
        pipeline->prim_type = _3DPRIM_QUADLIST;
        break;
    case XGL_TOPOLOGY_QUAD_STRIP:
        pipeline->prim_type = _3DPRIM_QUADSTRIP;
        break;
    case XGL_TOPOLOGY_LINE_LIST_ADJ:
        // TODO: implement
        break;
    case XGL_TOPOLOGY_LINE_STRIP_ADJ:
        // TODO: implement
        break;
    case XGL_TOPOLOGY_TRIANGLE_LIST_ADJ:
        // TODO: implement
        break;
    case XGL_TOPOLOGY_TRIANGLE_STRIP_ADJ:
        // TODO: implement
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

static XGL_RESULT intelPipelineRSState(struct intel_dev *dev, struct intel_pipeline *pipeline,
                                       const XGL_PIPELINE_RS_STATE_CREATE_INFO* rs_state)
{
    pipeline->depthClipEnable = rs_state->depthClipEnable;
    pipeline->rasterizerDiscardEnable = rs_state->rasterizerDiscardEnable;
    pipeline->pointSize = rs_state->pointSize;
    return XGL_SUCCESS;
}

static void intelPipelineDestroy(struct intel_obj *obj)
{
    struct intel_pipeline *pipeline = intel_pipeline_from_obj(obj);

    intel_base_destroy(&pipeline->obj.base);
}

XGL_RESULT XGLAPI intelCreateGraphicsPipeline(
    XGL_DEVICE                                  device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    XGL_PIPELINE*                               pPipeline)
{
    union {
        const void *ptr;
        const struct {
            XGL_STRUCTURE_TYPE struct_type;
            XGL_VOID *next;
        } *header;
        const XGL_GRAPHICS_PIPELINE_CREATE_INFO* graphics_pipeline;
        const XGL_PIPELINE_IA_STATE_CREATE_INFO* ia_state;
        const XGL_PIPELINE_DB_STATE_CREATE_INFO* db_state;
        const XGL_PIPELINE_CB_STATE* cb_state;
        const XGL_PIPELINE_RS_STATE_CREATE_INFO* rs_state;
        const XGL_PIPELINE_TESS_STATE_CREATE_INFO* tess_state;
        const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* shader_state;
    } info = { .ptr = pCreateInfo };
    struct intel_dev *dev = intel_dev(device);
    struct intel_pipeline *pipeline;
    XGL_RESULT result;

    pipeline = (struct intel_pipeline *) intel_base_create(dev, sizeof(*pipeline),
            dev->base.dbg, XGL_DBG_OBJECT_GRAPHICS_PIPELINE, pCreateInfo, 0);
    if (!pipeline) {
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    pipeline->dev = dev;
    pipeline->obj.destroy = intelPipelineDestroy;

    do {
        result = XGL_SUCCESS;

        switch (info.header->struct_type) {
        case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
            // TODO: Should not see Compute Pipeline structs processing CreateGraphicsPipeline
            return XGL_ERROR_BAD_PIPELINE_DATA;

        case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
            if (info.graphics_pipeline->flags & XGL_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT) {
                // TODO: process disable optimization.
            }
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
            result = intelPipelineIAState(dev, pipeline, info.ia_state);
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:
            pipeline->db_format = info.db_state->format;
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            pipeline->cb_state = *info.cb_state;
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
            result = intelPipelineRSState(dev, pipeline, info.rs_state);
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
            pipeline->tess_state = *info.tess_state;
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            switch (info.shader_state->shader.stage) {
            case XGL_SHADER_STAGE_VERTEX:
                pipeline->vs = info.shader_state->shader;
                pipeline->active_shaders |= SHADER_VERTEX_FLAG;
                break;
            case XGL_SHADER_STAGE_GEOMETRY:
                pipeline->gs = info.shader_state->shader;
                pipeline->active_shaders |= SHADER_GEOMETRY_FLAG;
                break;
            case XGL_SHADER_STAGE_FRAGMENT:
                pipeline->fs = info.shader_state->shader;
                pipeline->active_shaders |= SHADER_FRAGMENT_FLAG;
                break;
            case XGL_SHADER_STAGE_TESS_CONTROL:
                pipeline->tess_control = info.shader_state->shader;
                pipeline->active_shaders |= SHADER_TESS_CONTROL_FLAG;
                break;
            case XGL_SHADER_STAGE_TESS_EVALUATION:
                pipeline->tess_eval = info.shader_state->shader;
                pipeline->active_shaders |= SHADER_TESS_EVAL_FLAG;
                break;
            case XGL_SHADER_STAGE_COMPUTE:
                pipeline->compute = info.shader_state->shader;
                pipeline->active_shaders |= SHADER_COMPUTE_FLAG;
                break;
            default:
                // TODO: Log debug message
                result = XGL_ERROR_BAD_PIPELINE_DATA;
                goto error_exit;
            }
            break;

        default:
            // TODO: Log debug message
            result = XGL_ERROR_BAD_PIPELINE_DATA;
            goto error_exit;
        }

        if (result != XGL_SUCCESS) {
            // TODO: What needs to happen if pipeline build fails?
            goto error_exit;
        }
        info.ptr = info.header->next;
    } while (info.ptr != NULL);

    /*
     * Validate required elements
     */
    if (!(pipeline->active_shaders & SHADER_VERTEX_FLAG)) {
        // TODO: Log debug message: Vertex Shader required.
        result = XGL_ERROR_BAD_PIPELINE_DATA;
        goto error_exit;
    }

    /*
     * Tessalation control and evaluation have to both have a shader defined or
     * neither should have a shader defined.
     */
    if (((pipeline->active_shaders & SHADER_TESS_CONTROL_FLAG) == 0) !=
         ((pipeline->active_shaders & SHADER_TESS_EVAL_FLAG) == 0) ) {
        // TODO: Log debug message: Both Tess control and Tess eval are required to use tessalation
        result = XGL_ERROR_BAD_PIPELINE_DATA;
        goto error_exit;
    }

    if ((pipeline->active_shaders & SHADER_COMPUTE_FLAG) &&
        (pipeline->active_shaders & (SHADER_VERTEX_FLAG | SHADER_TESS_CONTROL_FLAG |
                                     SHADER_TESS_EVAL_FLAG | SHADER_GEOMETRY_FLAG |
                                     SHADER_FRAGMENT_FLAG))) {
        // TODO: Log debug message: Can only specify compute shader when doing compute
        result = XGL_ERROR_BAD_PIPELINE_DATA;
        goto error_exit;
    }

    /*
     * Now compile everything into a pipeline object ready for the HW.
     */

    *pPipeline = pipeline;

    return XGL_SUCCESS;

error_exit:
    intel_base_destroy(&pipeline->obj.base);
    return result;
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
