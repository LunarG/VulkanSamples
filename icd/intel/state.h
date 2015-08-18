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

#ifndef STATE_H
#define STATE_H

#include "intel.h"
#include "obj.h"

/* Should we add intel_state back, as the base class for dynamic states? */

struct intel_dynamic_viewport {
    struct intel_obj obj;

    uint32_t viewport_count;
    /* SF_CLIP_VIEWPORTs, CC_VIEWPORTs, and SCISSOR_RECTs */
    uint32_t *cmd;
    uint32_t cmd_len;
    uint32_t cmd_clip_pos;
    uint32_t cmd_cc_pos;
    uint32_t cmd_scissor_rect_pos;
};

struct intel_dynamic_raster_line {
    struct intel_obj obj;
    VkDynamicRasterLineStateCreateInfo raster_line_info;
};

struct intel_dynamic_raster_depth_bias {
    struct intel_obj obj;
    VkDynamicRasterDepthBiasStateCreateInfo raster_depth_bias_info;
};

struct intel_dynamic_color_blend {
    struct intel_obj obj;
    VkDynamicColorBlendStateCreateInfo color_blend_info;
};

struct intel_dynamic_depth {
    struct intel_obj obj;
    VkDynamicDepthStateCreateInfo depth_info;
};

struct intel_dynamic_stencil {
    struct intel_obj obj;
    VkDynamicStencilStateCreateInfo stencil_info_front;
    /* TODO: enable back facing stencil state */
    /*VkDynamicStencilStateCreateInfo stencil_info_back;*/
};

static inline struct intel_dynamic_viewport *intel_dynamic_viewport(VkDynamicViewportState state)
{
    return *(struct intel_dynamic_viewport **) &state;
}

static inline struct intel_dynamic_viewport *intel_viewport_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_viewport *) obj;
}

static inline struct intel_dynamic_raster_line *intel_dynamic_raster_line(VkDynamicRasterLineState state)
{
    return *(struct intel_dynamic_raster_line **) &state;
}

static inline struct intel_dynamic_raster_line *intel_raster_line_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_raster_line *) obj;
}

static inline struct intel_dynamic_raster_depth_bias *intel_dynamic_raster_depth_bias(VkDynamicRasterDepthBiasState state)
{
    return *(struct intel_dynamic_raster_depth_bias **) &state;
}

static inline struct intel_dynamic_raster_depth_bias *intel_raster_depth_bias_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_raster_depth_bias *) obj;
}

static inline struct intel_dynamic_color_blend *intel_dynamic_color_blend(VkDynamicColorBlendState state)
{
    return *(struct intel_dynamic_color_blend **) &state;
}

static inline struct intel_dynamic_color_blend *intel_blend_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_color_blend *) obj;
}

static inline struct intel_dynamic_depth *intel_dynamic_depth(VkDynamicDepthState state)
{
    return *(struct intel_dynamic_depth **) &state;
}

static inline struct intel_dynamic_depth *intel_depth_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_depth *) obj;
}

static inline struct intel_dynamic_stencil *intel_dynamic_stencil(VkDynamicStencilState state)
{
    return *(struct intel_dynamic_stencil **) &state;
}

static inline struct intel_dynamic_stencil *intel_stencil_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_stencil *) obj;
}

VkResult intel_viewport_state_create(struct intel_dev *dev,
                                       const VkDynamicViewportStateCreateInfo *info,
                                       struct intel_dynamic_viewport **state_ret);
void intel_viewport_state_destroy(struct intel_dynamic_viewport *state);

VkResult intel_raster_line_state_create(struct intel_dev *dev,
                                        const VkDynamicRasterLineStateCreateInfo *info,
                                        struct intel_dynamic_raster_line **state_ret);
void intel_raster_line_state_destroy(struct intel_dynamic_raster_line *state);
VkResult intel_raster_depth_bias_state_create(struct intel_dev *dev,
                                              const VkDynamicRasterDepthBiasStateCreateInfo *info,
                                              struct intel_dynamic_raster_depth_bias **state_ret);
void intel_raster_depth_bias_state_destroy(struct intel_dynamic_raster_depth_bias *state);
VkResult intel_blend_state_create(struct intel_dev *dev,
                                    const VkDynamicColorBlendStateCreateInfo *info,
                                    struct intel_dynamic_color_blend **state_ret);
void intel_blend_state_destroy(struct intel_dynamic_color_blend *state);

VkResult intel_depth_state_create(struct intel_dev *dev,
                                 const VkDynamicDepthStateCreateInfo *info,
                                 struct intel_dynamic_depth **state_ret);
void intel_depth_state_destroy(struct intel_dynamic_depth *state);

VkResult intel_stencil_state_create(struct intel_dev *dev,
                                 const VkDynamicStencilStateCreateInfo *info_front,
                                 const VkDynamicStencilStateCreateInfo *info_back,
                                 struct intel_dynamic_stencil **state_ret);
void intel_stencil_state_destroy(struct intel_dynamic_stencil *state);
#endif /* STATE_H */
