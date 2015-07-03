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

struct intel_dynamic_raster {
    struct intel_obj obj;
    VkDynamicRasterStateCreateInfo raster_info;
};

struct intel_dynamic_color_blend {
    struct intel_obj obj;
    VkDynamicColorBlendStateCreateInfo color_blend_info;
};

struct intel_dynamic_depth_stencil {
    struct intel_obj obj;
    VkDynamicDepthStencilStateCreateInfo depth_stencil_info;
};

static inline struct intel_dynamic_viewport *intel_dynamic_viewport(VkDynamicViewportState state)
{
    return *(struct intel_dynamic_viewport **) &state;
}

static inline struct intel_dynamic_viewport *intel_viewport_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_viewport *) obj;
}

static inline struct intel_dynamic_raster *intel_dynamic_raster(VkDynamicRasterState state)
{
    return *(struct intel_dynamic_raster **) &state;
}

static inline struct intel_dynamic_raster *intel_raster_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_raster *) obj;
}

static inline struct intel_dynamic_color_blend *intel_dynamic_color_blend(VkDynamicColorBlendState state)
{
    return *(struct intel_dynamic_color_blend **) &state;
}

static inline struct intel_dynamic_color_blend *intel_blend_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_color_blend *) obj;
}

static inline struct intel_dynamic_depth_stencil *intel_dynamic_depth_stencil(VkDynamicDepthStencilState state)
{
    return *(struct intel_dynamic_depth_stencil **) &state;
}

static inline struct intel_dynamic_depth_stencil *intel_depth_stencil_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_depth_stencil *) obj;
}

VkResult intel_viewport_state_create(struct intel_dev *dev,
                                       const VkDynamicViewportStateCreateInfo *info,
                                       struct intel_dynamic_viewport **state_ret);
void intel_viewport_state_destroy(struct intel_dynamic_viewport *state);

VkResult intel_raster_state_create(struct intel_dev *dev,
                                     const VkDynamicRasterStateCreateInfo *info,
                                     struct intel_dynamic_raster **state_ret);
void intel_raster_state_destroy(struct intel_dynamic_raster *state);

VkResult intel_blend_state_create(struct intel_dev *dev,
                                    const VkDynamicColorBlendStateCreateInfo *info,
                                    struct intel_dynamic_color_blend **state_ret);
void intel_blend_state_destroy(struct intel_dynamic_color_blend *state);

VkResult intel_depth_stencil_state_create(struct intel_dev *dev,
                                 const VkDynamicDepthStencilStateCreateInfo *info,
                                 struct intel_dynamic_depth_stencil **state_ret);
void intel_depth_stencil_state_destroy(struct intel_dynamic_depth_stencil *state);

#endif /* STATE_H */
