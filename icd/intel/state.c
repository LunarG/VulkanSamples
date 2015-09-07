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
 *   Chia-I Wu <olv@lunarg.com>
 */

#include <math.h>
#include "genhw/genhw.h"
#include "dev.h"
#include "state.h"

static void
viewport_get_guardband(const struct intel_gpu *gpu,
                       int center_x, int center_y,
                       int *min_gbx, int *max_gbx,
                       int *min_gby, int *max_gby)
{
   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 234:
    *
    *     "Per-Device Guardband Extents
    *
    *       - Supported X,Y ScreenSpace "Guardband" Extent: [-16K,16K-1]
    *       - Maximum Post-Clamp Delta (X or Y): 16K"
    *
    *     "In addition, in order to be correctly rendered, objects must have a
    *      screenspace bounding box not exceeding 8K in the X or Y direction.
    *      This additional restriction must also be comprehended by software,
    *      i.e., enforced by use of clipping."
    *
    * From the Ivy Bridge PRM, volume 2 part 1, page 248:
    *
    *     "Per-Device Guardband Extents
    *
    *       - Supported X,Y ScreenSpace "Guardband" Extent: [-32K,32K-1]
    *       - Maximum Post-Clamp Delta (X or Y): N/A"
    *
    *     "In addition, in order to be correctly rendered, objects must have a
    *      screenspace bounding box not exceeding 8K in the X or Y direction.
    *      This additional restriction must also be comprehended by software,
    *      i.e., enforced by use of clipping."
    *
    * Combined, the bounding box of any object can not exceed 8K in both
    * width and height.
    *
    * Below we set the guardband as a squre of length 8K, centered at where
    * the viewport is.  This makes sure all objects passing the GB test are
    * valid to the renderer, and those failing the XY clipping have a
    * better chance of passing the GB test.
    */
   const int max_extent = (intel_gpu_gen(gpu) >= INTEL_GEN(7)) ? 32768 : 16384;
   const int half_len = 8192 / 2;

   /* make sure the guardband is within the valid range */
   if (center_x - half_len < -max_extent)
      center_x = -max_extent + half_len;
   else if (center_x + half_len > max_extent - 1)
      center_x = max_extent - half_len;

   if (center_y - half_len < -max_extent)
      center_y = -max_extent + half_len;
   else if (center_y + half_len > max_extent - 1)
      center_y = max_extent - half_len;

   *min_gbx = (float) (center_x - half_len);
   *max_gbx = (float) (center_x + half_len);
   *min_gby = (float) (center_y - half_len);
   *max_gby = (float) (center_y + half_len);
}

static VkResult
viewport_state_alloc_cmd(struct intel_dynamic_viewport *state,
                         const struct intel_gpu *gpu,
                         const VkDynamicViewportStateCreateInfo *info)
{
    INTEL_GPU_ASSERT(gpu, 6, 7.5);

    state->viewport_count = info->viewportAndScissorCount;

    assert(info->viewportAndScissorCount <= INTEL_MAX_VIEWPORTS);

    if (intel_gpu_gen(gpu) >= INTEL_GEN(7)) {
        state->cmd_len = 16 * info->viewportAndScissorCount;

        state->cmd_clip_pos = 8;
    } else {
        state->cmd_len = 8 * info->viewportAndScissorCount;

        state->cmd_clip_pos = state->cmd_len;
        state->cmd_len += 4 * info->viewportAndScissorCount;
    }

    state->cmd_cc_pos = state->cmd_len;
    state->cmd_len += 2 * info->viewportAndScissorCount;

    state->cmd_scissor_rect_pos = state->cmd_len;
    state->cmd_len += 2 * info->viewportAndScissorCount;

    state->cmd = intel_alloc(state, sizeof(uint32_t) * state->cmd_len,
            0, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!state->cmd)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    return VK_SUCCESS;
}

static VkResult
viewport_state_init(struct intel_dynamic_viewport *state,
                    const struct intel_gpu *gpu,
                    const VkDynamicViewportStateCreateInfo *info)
{
    const uint32_t sf_stride = (intel_gpu_gen(gpu) >= INTEL_GEN(7)) ? 16 : 8;
    const uint32_t clip_stride = (intel_gpu_gen(gpu) >= INTEL_GEN(7)) ? 16 : 4;
    uint32_t *sf_viewport, *clip_viewport, *cc_viewport, *scissor_rect;
    uint32_t i;
    VkResult ret;

    INTEL_GPU_ASSERT(gpu, 6, 7.5);

    ret = viewport_state_alloc_cmd(state, gpu, info);
    if (ret != VK_SUCCESS)
        return ret;

    sf_viewport = state->cmd;
    clip_viewport = state->cmd + state->cmd_clip_pos;
    cc_viewport = state->cmd + state->cmd_cc_pos;
    scissor_rect = state->cmd + state->cmd_scissor_rect_pos;

    for (i = 0; i < info->viewportAndScissorCount; i++) {
        const VkViewport *viewport = &info->pViewports[i];
        uint32_t *dw = NULL;
        float translate[3], scale[3];
        int min_gbx, max_gbx, min_gby, max_gby;

        scale[0] = viewport->width / 2.0f;
        scale[1] = viewport->height / 2.0f;
        scale[2] = viewport->maxDepth - viewport->minDepth;
        translate[0] = viewport->originX + scale[0];
        translate[1] = viewport->originY + scale[1];
        translate[2] = viewport->minDepth;

        viewport_get_guardband(gpu, (int) translate[0], (int) translate[1],
                &min_gbx, &max_gbx, &min_gby, &max_gby);

        /* SF_VIEWPORT */
        dw = sf_viewport;
        dw[0] = u_fui(scale[0]);
        dw[1] = u_fui(scale[1]);
        dw[2] = u_fui(scale[2]);
        dw[3] = u_fui(translate[0]);
        dw[4] = u_fui(translate[1]);
        dw[5] = u_fui(translate[2]);
        dw[6] = 0;
        dw[7] = 0;
        sf_viewport += sf_stride;

        /* CLIP_VIEWPORT */
        dw = clip_viewport;
        dw[0] = u_fui(((float) min_gbx - translate[0]) / fabsf(scale[0]));
        dw[1] = u_fui(((float) max_gbx - translate[0]) / fabsf(scale[0]));
        dw[2] = u_fui(((float) min_gby - translate[1]) / fabsf(scale[1]));
        dw[3] = u_fui(((float) max_gby - translate[1]) / fabsf(scale[1]));
        clip_viewport += clip_stride;

        /* CC_VIEWPORT */
        dw = cc_viewport;
        dw[0] = u_fui(viewport->minDepth);
        dw[1] = u_fui(viewport->maxDepth);
        cc_viewport += 2;
    }

    for (i = 0; i < info->viewportAndScissorCount; i++) {
        const VkRect2D *scissor = &info->pScissors[i];
        /* SCISSOR_RECT */
        int16_t max_x, max_y;
        uint32_t *dw = NULL;

        max_x = (scissor->offset.x + scissor->extent.width - 1) & 0xffff;
        max_y = (scissor->offset.y + scissor->extent.height - 1) & 0xffff;

        dw = scissor_rect;
        if (scissor->extent.width && scissor->extent.height) {
            dw[0] = (scissor->offset.y & 0xffff) << 16 |
                                                    (scissor->offset.x & 0xffff);
            dw[1] = max_y << 16 | max_x;
        } else {
            dw[0] = 1 << 16 | 1;
            dw[1] = 0;
        }
        scissor_rect += 2;
    }

    return VK_SUCCESS;
}

static void viewport_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_viewport *state = intel_viewport_state_from_obj(obj);

    intel_viewport_state_destroy(state);
}

VkResult intel_viewport_state_create(struct intel_dev *dev,
                                       const VkDynamicViewportStateCreateInfo *info,
                                       struct intel_dynamic_viewport **state_ret)
{
    struct intel_dynamic_viewport *state;
    VkResult ret;

    state = (struct intel_dynamic_viewport *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE,
            info, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = viewport_state_destroy;

    ret = viewport_state_init(state, dev->gpu, info);
    if (ret != VK_SUCCESS) {
        intel_viewport_state_destroy(state);
        return ret;
    }

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_viewport_state_destroy(struct intel_dynamic_viewport *state)
{
    intel_free(state, state->cmd);
    intel_base_destroy(&state->obj.base);
}

static void line_width_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_line_width *state = intel_line_width_state_from_obj(obj);

    intel_line_width_state_destroy(state);
}

VkResult intel_line_width_state_create(struct intel_dev *dev,
                                        const VkDynamicLineWidthStateCreateInfo *info,
                                        struct intel_dynamic_line_width **state_ret)
{
    struct intel_dynamic_line_width *state;

    state = (struct intel_dynamic_line_width *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_LINE_WIDTH_STATE,
            info, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = line_width_state_destroy;
    state->line_width_info = *info;

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_line_width_state_destroy(struct intel_dynamic_line_width *state)
{
    intel_base_destroy(&state->obj.base);
}

static void depth_bias_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_depth_bias *state = intel_depth_bias_state_from_obj(obj);

    intel_depth_bias_state_destroy(state);
}

VkResult intel_depth_bias_state_create(struct intel_dev *dev,
                                              const VkDynamicDepthBiasStateCreateInfo *info,
                                              struct intel_dynamic_depth_bias **state_ret)
{
    struct intel_dynamic_depth_bias *state;

    state = (struct intel_dynamic_depth_bias *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_DEPTH_BIAS_STATE,
            info, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = depth_bias_state_destroy;
    state->depth_bias_info = *info;

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_depth_bias_state_destroy(struct intel_dynamic_depth_bias *state)
{
    intel_base_destroy(&state->obj.base);
}

static void blend_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_blend *state = intel_blend_state_from_obj(obj);

    intel_blend_state_destroy(state);
}

VkResult intel_blend_state_create(struct intel_dev *dev,
                                    const VkDynamicBlendStateCreateInfo *info,
                                    struct intel_dynamic_blend **state_ret)
{
    struct intel_dynamic_blend *state;

    state = (struct intel_dynamic_blend *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_BLEND_STATE,
            info, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = blend_state_destroy;
    state->blend_info = *info;

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_blend_state_destroy(struct intel_dynamic_blend *state)
{
    intel_base_destroy(&state->obj.base);
}

static void depth_bounds_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_depth_bounds *state = intel_depth_bounds_state_from_obj(obj);

    intel_depth_bounds_state_destroy(state);
}

VkResult intel_depth_bounds_state_create(struct intel_dev *dev,
                                 const VkDynamicDepthBoundsStateCreateInfo *info,
                                 struct intel_dynamic_depth_bounds **state_ret)
{
    struct intel_dynamic_depth_bounds *state;

    state = (struct intel_dynamic_depth_bounds *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE,
            info, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = depth_bounds_state_destroy;

    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 359:
     *
     *     "If the Depth Buffer is either undefined or does not have a surface
     *      format of D32_FLOAT_S8X24_UINT or D24_UNORM_S8_UINT and separate
     *      stencil buffer is disabled, Stencil Test Enable must be DISABLED"
     *
     * From the Sandy Bridge PRM, volume 2 part 1, page 370:
     *
     *     "This field (Stencil Test Enable) cannot be enabled if
     *      Surface Format in 3DSTATE_DEPTH_BUFFER is set to D16_UNORM."
     *
     * TODO We do not check these yet.
     */

    state->depth_bounds_info = *info;

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_depth_bounds_state_destroy(struct intel_dynamic_depth_bounds *state)
{
    intel_base_destroy(&state->obj.base);
}

static void stencil_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_stencil *state = intel_stencil_state_from_obj(obj);

    intel_stencil_state_destroy(state);
}

VkResult intel_stencil_state_create(struct intel_dev *dev,
                                 const VkDynamicStencilStateCreateInfo *info_front,
                                 const VkDynamicStencilStateCreateInfo *info_back,
                                 struct intel_dynamic_stencil **state_ret)
{
    struct intel_dynamic_stencil *state;

    /* TODO: enable back facing stencil state */
    /* Some plumbing needs to be done if we want to support info_back.
     * In the meantime, catch that back facing info has been submitted. */
    assert(info_front == info_back || info_back == NULL);

    state = (struct intel_dynamic_stencil *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE,
            info_front, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = stencil_state_destroy;

    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 359:
     *
     *     "If the Depth Buffer is either undefined or does not have a surface
     *      format of D32_FLOAT_S8X24_UINT or D24_UNORM_S8_UINT and separate
     *      stencil buffer is disabled, Stencil Test Enable must be DISABLED"
     *
     * From the Sandy Bridge PRM, volume 2 part 1, page 370:
     *
     *     "This field (Stencil Test Enable) cannot be enabled if
     *      Surface Format in 3DSTATE_DEPTH_BUFFER is set to D16_UNORM."
     *
     * TODO We do not check these yet.
     */

    state->stencil_info_front = *info_front;
    /* TODO: enable back facing stencil state */
    /*state->stencil_info_back  = *info_back;*/

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_stencil_state_destroy(struct intel_dynamic_stencil *state)
{
    intel_base_destroy(&state->obj.base);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicViewportState(
    VkDevice                                    device,
    const VkDynamicViewportStateCreateInfo*     pCreateInfo,
    VkDynamicViewportState*                     pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_viewport_state_create(dev, pCreateInfo,
            (struct intel_dynamic_viewport **) pState);
}

ICD_EXPORT void VKAPI vkDestroyDynamicViewportState(
    VkDevice                                device,
    VkDynamicViewportState                  dynamicViewportState)

{
    struct intel_obj *obj = intel_obj(dynamicViewportState.handle);

    obj->destroy(obj);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicLineWidthState(
    VkDevice                                  device,
    const VkDynamicLineWidthStateCreateInfo*  pCreateInfo,
    VkDynamicLineWidthState*                  pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_line_width_state_create(dev, pCreateInfo,
            (struct intel_dynamic_line_width **) pState);
}

ICD_EXPORT void VKAPI vkDestroyDynamicLineWidthState(
    VkDevice                                device,
    VkDynamicLineWidthState                 dynamicLineWidthState)

{
    struct intel_obj *obj = intel_obj(dynamicLineWidthState.handle);

    obj->destroy(obj);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicDepthBiasState(
    VkDevice                                        device,
    const VkDynamicDepthBiasStateCreateInfo*        pCreateInfo,
    VkDynamicDepthBiasState*                        pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_depth_bias_state_create(dev, pCreateInfo,
            (struct intel_dynamic_depth_bias **) pState);
}

ICD_EXPORT void VKAPI vkDestroyDynamicDepthBiasState(
    VkDevice                                device,
    VkDynamicDepthBiasState                 dynamicDepthBiasState)

{
    struct intel_obj *obj = intel_obj(dynamicDepthBiasState.handle);

    obj->destroy(obj);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicBlendState(
    VkDevice                                      device,
    const VkDynamicBlendStateCreateInfo*          pCreateInfo,
    VkDynamicBlendState*                          pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_blend_state_create(dev, pCreateInfo,
            (struct intel_dynamic_blend **) pState);
}

ICD_EXPORT void VKAPI vkDestroyDynamicBlendState(
    VkDevice                                device,
    VkDynamicBlendState                     dynamicBlendState)

{
    struct intel_obj *obj = intel_obj(dynamicBlendState.handle);

    obj->destroy(obj);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicDepthBoundsState(
    VkDevice                                        device,
    const VkDynamicDepthBoundsStateCreateInfo*      pCreateInfo,
    VkDynamicDepthBoundsState*                      pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_depth_bounds_state_create(dev, pCreateInfo,
            (struct intel_dynamic_depth_bounds **) pState);
}

ICD_EXPORT void VKAPI vkDestroyDynamicDepthBoundsState(
    VkDevice                                device,
    VkDynamicDepthBoundsState               dynamicDepthBoundsState)

{
    struct intel_obj *obj = intel_obj(dynamicDepthBoundsState.handle);

    obj->destroy(obj);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicStencilState(
    VkDevice                                        device,
    const VkDynamicStencilStateCreateInfo*          pCreateInfoFront,
    const VkDynamicStencilStateCreateInfo*          pCreateInfoBack,
    VkDynamicStencilState*                          pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_stencil_state_create(dev, pCreateInfoFront, pCreateInfoBack,
            (struct intel_dynamic_stencil **) pState);
}

ICD_EXPORT void VKAPI vkDestroyDynamicStencilState(
    VkDevice                                device,
    VkDynamicStencilState                   dynamicStencilState)

{
    struct intel_obj *obj = intel_obj(dynamicStencilState.handle);

    obj->destroy(obj);
}
