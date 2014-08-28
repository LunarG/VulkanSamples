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

static XGL_RESULT pipeline_ia_state(struct intel_dev *dev, struct intel_pipeline *pipeline,
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

static XGL_RESULT pipeline_rs_state(struct intel_dev *dev, struct intel_pipeline *pipeline,
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
        icd_free(intel_shader(pipeline->intel_vs.pCode));
    }
    if (pipeline->active_shaders & SHADER_GEOMETRY_FLAG) {
        icd_free(intel_shader(pipeline->gs.pCode));
    }
    if (pipeline->active_shaders & SHADER_FRAGMENT_FLAG) {
        icd_free(intel_shader(pipeline->intel_fs.pCode));
    }
    if (pipeline->active_shaders & SHADER_TESS_CONTROL_FLAG) {
        icd_free(intel_shader(pipeline->tess_control.pCode));
    }
    if (pipeline->active_shaders & SHADER_TESS_EVAL_FLAG) {
        icd_free(intel_shader(pipeline->tess_eval.pCode));
    }

    if (pipeline->vs_rmap)
        intel_rmap_destroy(pipeline->vs_rmap);
    if (pipeline->fs_rmap)
        intel_rmap_destroy(pipeline->fs_rmap);

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
    struct intel_shader *shader;
    const XGL_PIPELINE_SHADER* shader_state;
    void *shaderCode;
    XGL_RESULT result;

    pipeline = (struct intel_pipeline *) intel_base_create(dev, sizeof(*pipeline),
            dev->base.dbg, XGL_DBG_OBJECT_GRAPHICS_PIPELINE, pCreateInfo, 0);
    if (!pipeline) {
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    pipeline->dev = dev;
    pipeline->obj.destroy = pipeline_destroy;
    pipeline->total_size = 0;

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
            result = pipeline_ia_state(dev, pipeline, info.ia_state);
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:
            pipeline->db_format = info.db_state->format;
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            pipeline->cb_state = *info.cb_state;
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
            result = pipeline_rs_state(dev, pipeline, info.rs_state);
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
            pipeline->tess_state = *info.tess_state;
            break;

        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            shader_state = &info.shader_state->shader;
            shader = intel_shader(shader_state->shader);

            // TODO: process shader object and include in pipeline
            // For now that processing is simply a copy so that the app
            // can destroy the original shader object after pipeline creation.
            shaderCode = (void *) icd_alloc(shader->ir->size, 4, XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
            if (!shaderCode) {
                result = XGL_ERROR_OUT_OF_MEMORY;
                goto error_exit;
            }

            switch (shader_state->stage) {
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
                pipeline->vs = *shader_state;
                pipeline->intel_vs.pCode = shaderCode;
                pipeline->intel_vs.codeSize = shader->ir->size;
                pipeline->active_shaders |= SHADER_VERTEX_FLAG;
                pipeline->vs_rmap = intel_rmap_create(dev,
                        &shader_state->descriptorSetMapping[0],
                        &shader_state->dynamicMemoryViewMapping, 0);
                if (!pipeline->vs_rmap) {
                    result = XGL_ERROR_OUT_OF_MEMORY;
                    goto error_exit;
                }
                break;
            case XGL_SHADER_STAGE_GEOMETRY:
                pipeline->gs.pCode = shaderCode;
                pipeline->gs.codeSize = shader->ir->size;
                pipeline->active_shaders |= SHADER_GEOMETRY_FLAG;
                break;
            case XGL_SHADER_STAGE_FRAGMENT:
                pipeline->fs = *shader_state;
                pipeline->intel_fs.pCode = shaderCode;
                pipeline->intel_fs.codeSize = shader->ir->size;
                pipeline->active_shaders |= SHADER_FRAGMENT_FLAG;
                /* assuming one RT; need to parse the shader */
                pipeline->fs_rmap = intel_rmap_create(dev,
                        &shader_state->descriptorSetMapping[0],
                        &shader_state->dynamicMemoryViewMapping, 1);
                if (!pipeline->fs_rmap) {
                    result = XGL_ERROR_OUT_OF_MEMORY;
                    goto error_exit;
                }
                break;
            case XGL_SHADER_STAGE_TESS_CONTROL:
                pipeline->tess_control.pCode = shaderCode;
                pipeline->tess_control.codeSize = shader->ir->size;
                pipeline->active_shaders |= SHADER_TESS_CONTROL_FLAG;
                break;
            case XGL_SHADER_STAGE_TESS_EVALUATION:
                pipeline->tess_eval.pCode = shaderCode;
                pipeline->tess_eval.codeSize = shader->ir->size;
                pipeline->active_shaders |= SHADER_TESS_EVAL_FLAG;
                break;
            case XGL_SHADER_STAGE_COMPUTE:
                pipeline->compute.pCode = shaderCode;
                pipeline->compute.codeSize = shader->ir->size;
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
     * XGL_TOPOLOGY_PATCH primitive topology is only valid for tessellation pipelines.
     * Mismatching primitive topology and tessellation fails graphics pipeline creation.
     */
    if (pipeline->active_shaders & (SHADER_TESS_CONTROL_FLAG | SHADER_TESS_EVAL_FLAG) &&
        (pipeline->ia_state.topology != XGL_TOPOLOGY_PATCH)) {
        // TODO: Log debug message: Invalid topology used with tessalation shader.
        result = XGL_ERROR_BAD_PIPELINE_DATA;
        goto error_exit;
    }

    if ((pipeline->ia_state.topology == XGL_TOPOLOGY_PATCH) &&
            (pipeline->active_shaders & ~(SHADER_TESS_CONTROL_FLAG | SHADER_TESS_EVAL_FLAG))) {
        // TODO: Log debug message: Cannot use TOPOLOGY_PATCH on non-tessalation shader.
        result = XGL_ERROR_BAD_PIPELINE_DATA;
        goto error_exit;
    }

    /*
     * Now compile everything into a pipeline object ready for the HW.
     */

    *pPipeline = pipeline;

    return XGL_SUCCESS;

error_exit:
    pipeline_destroy(&pipeline->obj);
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
