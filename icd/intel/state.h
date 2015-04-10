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

struct intel_dynamic_vp {
    struct intel_obj obj;

    uint32_t viewport_count;
    /* SF_CLIP_VIEWPORTs, CC_VIEWPORTs, and SCISSOR_RECTs */
    uint32_t *cmd;
    uint32_t cmd_len;
    uint32_t cmd_clip_pos;
    uint32_t cmd_cc_pos;
    uint32_t cmd_scissor_rect_pos;
};

struct intel_dynamic_rs {
    struct intel_obj obj;
    VkDynamicRsStateCreateInfo rs_info;
};

struct intel_dynamic_cb {
    struct intel_obj obj;
    VkDynamicCbStateCreateInfo cb_info;
};

struct intel_dynamic_ds {
    struct intel_obj obj;
    VkDynamicDsStateCreateInfo ds_info;
};

static inline struct intel_dynamic_vp *intel_dynamic_vp(VkDynamicVpStateObject state)
{
    return (struct intel_dynamic_vp *) state;
}

static inline struct intel_dynamic_vp *intel_viewport_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_vp *) obj;
}

static inline struct intel_dynamic_rs *intel_dynamic_rs(VkDynamicRsStateObject state)
{
    return (struct intel_dynamic_rs *) state;
}

static inline struct intel_dynamic_rs *intel_raster_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_rs *) obj;
}

static inline struct intel_dynamic_cb *intel_dynamic_cb(VkDynamicCbStateObject state)
{
    return (struct intel_dynamic_cb *) state;
}

static inline struct intel_dynamic_cb *intel_blend_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_cb *) obj;
}

static inline struct intel_dynamic_ds *intel_dynamic_ds(VkDynamicDsStateObject state)
{
    return (struct intel_dynamic_ds *) state;
}

static inline struct intel_dynamic_ds *intel_ds_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_ds *) obj;
}

VkResult intel_viewport_state_create(struct intel_dev *dev,
                                       const VkDynamicVpStateCreateInfo *info,
                                       struct intel_dynamic_vp **state_ret);
void intel_viewport_state_destroy(struct intel_dynamic_vp *state);

VkResult intel_raster_state_create(struct intel_dev *dev,
                                     const VkDynamicRsStateCreateInfo *info,
                                     struct intel_dynamic_rs **state_ret);
void intel_raster_state_destroy(struct intel_dynamic_rs *state);

VkResult intel_blend_state_create(struct intel_dev *dev,
                                    const VkDynamicCbStateCreateInfo *info,
                                    struct intel_dynamic_cb **state_ret);
void intel_blend_state_destroy(struct intel_dynamic_cb *state);

VkResult intel_ds_state_create(struct intel_dev *dev,
                                 const VkDynamicDsStateCreateInfo *info,
                                 struct intel_dynamic_ds **state_ret);
void intel_ds_state_destroy(struct intel_dynamic_ds *state);

#endif /* STATE_H */
