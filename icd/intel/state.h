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

struct intel_dynamic_line_width {
    struct intel_obj obj;
    VkDynamicLineWidthStateCreateInfo line_width_info;
};

struct intel_dynamic_depth_bias {
    struct intel_obj obj;
    VkDynamicDepthBiasStateCreateInfo depth_bias_info;
};

struct intel_dynamic_blend {
    struct intel_obj obj;
    VkDynamicBlendStateCreateInfo blend_info;
};

struct intel_dynamic_depth_bounds {
    struct intel_obj obj;
    VkDynamicDepthBoundsStateCreateInfo depth_bounds_info;
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

static inline struct intel_dynamic_line_width *intel_dynamic_line_width(VkDynamicLineWidthState state)
{
    return *(struct intel_dynamic_line_width **) &state;
}

static inline struct intel_dynamic_line_width *intel_line_width_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_line_width *) obj;
}

static inline struct intel_dynamic_depth_bias *intel_dynamic_depth_bias(VkDynamicDepthBiasState state)
{
    return *(struct intel_dynamic_depth_bias **) &state;
}

static inline struct intel_dynamic_depth_bias *intel_depth_bias_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_depth_bias *) obj;
}

static inline struct intel_dynamic_blend *intel_dynamic_blend(VkDynamicBlendState state)
{
    return *(struct intel_dynamic_blend **) &state;
}

static inline struct intel_dynamic_blend *intel_blend_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_blend *) obj;
}

static inline struct intel_dynamic_depth_bounds *intel_dynamic_depth_bounds(VkDynamicDepthBoundsState state)
{
    return *(struct intel_dynamic_depth_bounds **) &state;
}

static inline struct intel_dynamic_depth_bounds *intel_depth_bounds_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_depth_bounds *) obj;
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

VkResult intel_line_width_state_create(struct intel_dev *dev,
                                        const VkDynamicLineWidthStateCreateInfo *info,
                                        struct intel_dynamic_line_width **state_ret);
void intel_line_width_state_destroy(struct intel_dynamic_line_width *state);
VkResult intel_depth_bias_state_create(struct intel_dev *dev,
                                              const VkDynamicDepthBiasStateCreateInfo *info,
                                              struct intel_dynamic_depth_bias **state_ret);
void intel_depth_bias_state_destroy(struct intel_dynamic_depth_bias *state);
VkResult intel_blend_state_create(struct intel_dev *dev,
                                    const VkDynamicBlendStateCreateInfo *info,
                                    struct intel_dynamic_blend **state_ret);
void intel_blend_state_destroy(struct intel_dynamic_blend *state);

VkResult intel_depth_bounds_state_create(struct intel_dev *dev,
                                 const VkDynamicDepthBoundsStateCreateInfo *info,
                                 struct intel_dynamic_depth_bounds **state_ret);
void intel_depth_bounds_state_destroy(struct intel_dynamic_depth_bounds *state);

VkResult intel_stencil_state_create(struct intel_dev *dev,
                                 const VkDynamicStencilStateCreateInfo *info_front,
                                 const VkDynamicStencilStateCreateInfo *info_back,
                                 struct intel_dynamic_stencil **state_ret);
void intel_stencil_state_destroy(struct intel_dynamic_stencil *state);
#endif /* STATE_H */
