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

static void raster_line_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_raster_line *state = intel_raster_line_state_from_obj(obj);

    intel_raster_line_state_destroy(state);
}

VkResult intel_raster_line_state_create(struct intel_dev *dev,
                                        const VkDynamicRasterLineStateCreateInfo *info,
                                        struct intel_dynamic_raster_line **state_ret)
{
    struct intel_dynamic_raster_line *state;

    state = (struct intel_dynamic_raster_line *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_RASTER_LINE_STATE,
            info, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = raster_line_state_destroy;
    state->raster_line_info = *info;

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_raster_line_state_destroy(struct intel_dynamic_raster_line *state)
{
    intel_base_destroy(&state->obj.base);
}

static void raster_depth_bias_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_raster_depth_bias *state = intel_raster_depth_bias_state_from_obj(obj);

    intel_raster_depth_bias_state_destroy(state);
}

VkResult intel_raster_depth_bias_state_create(struct intel_dev *dev,
                                              const VkDynamicRasterDepthBiasStateCreateInfo *info,
                                              struct intel_dynamic_raster_depth_bias **state_ret)
{
    struct intel_dynamic_raster_depth_bias *state;

    state = (struct intel_dynamic_raster_depth_bias *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_RASTER_DEPTH_BIAS_STATE,
            info, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = raster_depth_bias_state_destroy;
    state->raster_depth_bias_info = *info;

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_raster_depth_bias_state_destroy(struct intel_dynamic_raster_depth_bias *state)
{
    intel_base_destroy(&state->obj.base);
}

static void blend_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_color_blend *state = intel_blend_state_from_obj(obj);

    intel_blend_state_destroy(state);
}

VkResult intel_blend_state_create(struct intel_dev *dev,
                                    const VkDynamicColorBlendStateCreateInfo *info,
                                    struct intel_dynamic_color_blend **state_ret)
{
    struct intel_dynamic_color_blend *state;

    state = (struct intel_dynamic_color_blend *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_COLOR_BLEND_STATE,
            info, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = blend_state_destroy;
    state->color_blend_info = *info;

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_blend_state_destroy(struct intel_dynamic_color_blend *state)
{
    intel_base_destroy(&state->obj.base);
}

static void depth_stencil_state_destroy(struct intel_obj *obj)
{
    struct intel_dynamic_depth_stencil *state = intel_depth_stencil_state_from_obj(obj);

    intel_depth_stencil_state_destroy(state);
}

VkResult intel_depth_stencil_state_create(struct intel_dev *dev,
                                 const VkDynamicDepthStencilStateCreateInfo *info,
                                 struct intel_dynamic_depth_stencil **state_ret)
{
    struct intel_dynamic_depth_stencil *state;

    state = (struct intel_dynamic_depth_stencil *) intel_base_create(&dev->base.handle,
            sizeof(*state), dev->base.dbg, VK_OBJECT_TYPE_DYNAMIC_DEPTH_STENCIL_STATE,
            info, 0);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    state->obj.destroy = depth_stencil_state_destroy;

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

    state->depth_stencil_info = *info;

    *state_ret = state;

    return VK_SUCCESS;
}

void intel_depth_stencil_state_destroy(struct intel_dynamic_depth_stencil *state)
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

ICD_EXPORT VkResult VKAPI vkDestroyDynamicViewportState(
    VkDevice                                device,
    VkDynamicViewportState                  dynamicViewportState)

{
    struct intel_obj *obj = intel_obj(dynamicViewportState.handle);

    obj->destroy(obj);
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicRasterLineState(
    VkDevice                                  device,
    const VkDynamicRasterLineStateCreateInfo* pCreateInfo,
    VkDynamicRasterLineState*                 pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_raster_line_state_create(dev, pCreateInfo,
            (struct intel_dynamic_raster_line **) pState);
}

ICD_EXPORT VkResult VKAPI vkDestroyDynamicRasterLineState(
    VkDevice                                device,
    VkDynamicRasterLineState                dynamicRasterLineState)

{
    struct intel_obj *obj = intel_obj(dynamicRasterLineState.handle);

    obj->destroy(obj);
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicRasterDepthBiasState(
    VkDevice                                        device,
    const VkDynamicRasterDepthBiasStateCreateInfo*  pCreateInfo,
    VkDynamicRasterDepthBiasState*                  pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_raster_depth_bias_state_create(dev, pCreateInfo,
            (struct intel_dynamic_raster_depth_bias **) pState);
}

ICD_EXPORT VkResult VKAPI vkDestroyDynamicRasterDepthBiasState(
    VkDevice                                device,
    VkDynamicRasterDepthBiasState           dynamicRasterDepthBiasState)

{
    struct intel_obj *obj = intel_obj(dynamicRasterDepthBiasState.handle);

    obj->destroy(obj);
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(
    VkDevice                                      device,
    const VkDynamicColorBlendStateCreateInfo*     pCreateInfo,
    VkDynamicColorBlendState*                     pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_blend_state_create(dev, pCreateInfo,
            (struct intel_dynamic_color_blend **) pState);
}

ICD_EXPORT VkResult VKAPI vkDestroyDynamicColorBlendState(
    VkDevice                                device,
    VkDynamicColorBlendState                dynamicColorBlendState)

{
    struct intel_obj *obj = intel_obj(dynamicColorBlendState.handle);

    obj->destroy(obj);
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(
    VkDevice                                        device,
    const VkDynamicDepthStencilStateCreateInfo*     pCreateInfo,
    VkDynamicDepthStencilState*                     pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_depth_stencil_state_create(dev, pCreateInfo,
            (struct intel_dynamic_depth_stencil **) pState);
}

ICD_EXPORT VkResult VKAPI vkDestroyDynamicDepthStencilState(
    VkDevice                                device,
    VkDynamicDepthStencilState              dynamicDepthStencilState)

{
    struct intel_obj *obj = intel_obj(dynamicDepthStencilState.handle);

    obj->destroy(obj);
    return VK_SUCCESS;
}
