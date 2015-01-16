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

    XGL_UINT viewport_count;
    bool has_scissor_rects;
    /* SF_CLIP_VIEWPORTs, CC_VIEWPORTs, and SCISSOR_RECTs */
    uint32_t *cmd;
    XGL_UINT cmd_len;
    XGL_UINT cmd_clip_pos;
    XGL_UINT cmd_cc_pos;
    XGL_UINT cmd_scissor_rect_pos;
};

struct intel_dynamic_rs {
    struct intel_obj obj;
    XGL_DYNAMIC_RS_STATE_CREATE_INFO rs_info;
};

struct intel_dynamic_cb {
    struct intel_obj obj;
    XGL_DYNAMIC_CB_STATE_CREATE_INFO cb_info;
};

struct intel_dynamic_ds {
    struct intel_obj obj;
    XGL_DYNAMIC_DS_STATE_CREATE_INFO ds_info;
};

static inline struct intel_dynamic_vp *intel_dynamic_vp(XGL_DYNAMIC_VP_STATE_OBJECT state)
{
    return (struct intel_dynamic_vp *) state;
}

static inline struct intel_dynamic_vp *intel_viewport_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_vp *) obj;
}

static inline struct intel_dynamic_rs *intel_dynamic_rs(XGL_DYNAMIC_RS_STATE_OBJECT state)
{
    return (struct intel_dynamic_rs *) state;
}

static inline struct intel_dynamic_rs *intel_raster_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_rs *) obj;
}

static inline struct intel_dynamic_cb *intel_dynamic_cb(XGL_DYNAMIC_CB_STATE_OBJECT state)
{
    return (struct intel_dynamic_cb *) state;
}

static inline struct intel_dynamic_cb *intel_blend_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_cb *) obj;
}

static inline struct intel_dynamic_ds *intel_dynamic_ds(XGL_DYNAMIC_DS_STATE_OBJECT state)
{
    return (struct intel_dynamic_ds *) state;
}

static inline struct intel_dynamic_ds *intel_ds_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_dynamic_ds *) obj;
}

XGL_RESULT intel_viewport_state_create(struct intel_dev *dev,
                                       const XGL_DYNAMIC_VP_STATE_CREATE_INFO *info,
                                       struct intel_dynamic_vp **state_ret);
void intel_viewport_state_destroy(struct intel_dynamic_vp *state);

XGL_RESULT intel_raster_state_create(struct intel_dev *dev,
                                     const XGL_DYNAMIC_RS_STATE_CREATE_INFO *info,
                                     struct intel_dynamic_rs **state_ret);
void intel_raster_state_destroy(struct intel_dynamic_rs *state);

XGL_RESULT intel_blend_state_create(struct intel_dev *dev,
                                    const XGL_DYNAMIC_CB_STATE_CREATE_INFO *info,
                                    struct intel_dynamic_cb **state_ret);
void intel_blend_state_destroy(struct intel_dynamic_cb *state);

XGL_RESULT intel_ds_state_create(struct intel_dev *dev,
                                 const XGL_DYNAMIC_DS_STATE_CREATE_INFO *info,
                                 struct intel_dynamic_ds **state_ret);
void intel_ds_state_destroy(struct intel_dynamic_ds *state);

#endif /* STATE_H */
