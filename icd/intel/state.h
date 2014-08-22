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

#ifndef STATE_H
#define STATE_H

#include "intel.h"
#include "obj.h"

/* Should we add intel_state back, as the base class for dynamic states? */

struct intel_viewport_state {
    struct intel_obj obj;

    bool scissor_enable;
    /* SF_CLIP_VIEWPORTs, CC_VIEWPORTs, and SCISSOR_RECTs */
    uint32_t *cmd;
    XGL_UINT cmd_len;
    XGL_UINT cmd_align;
    XGL_UINT cmd_clip_offset;
    XGL_UINT cmd_cc_offset;
    XGL_UINT cmd_scissor_rect_offset;
};

struct intel_raster_state {
    struct intel_obj obj;

    uint32_t cmd_clip_cull;
    uint32_t cmd_sf_fill;
    uint32_t cmd_sf_cull;
    uint32_t cmd_depth_offset_const;
    uint32_t cmd_depth_offset_scale;
    uint32_t cmd_depth_offset_clamp;
};

struct intel_msaa_state {
    struct intel_obj obj;

    /* 3DSTATE_MULTISAMPLE and 3DSTATE_SAMPLE_MASK */
    uint32_t cmd[6];
    XGL_UINT cmd_len;
};

struct intel_blend_state {
    struct intel_obj obj;

    /* BLEND_STATE */
    uint32_t cmd[XGL_MAX_COLOR_ATTACHMENTS * 2];
    /* a part of COLOR_CALC_STATE */
    uint32_t cmd_blend_color[4];
};

struct intel_ds_state {
    struct intel_obj obj;

    /* DEPTH_STENCIL_STATE */
    uint32_t cmd[3];
    /* a part of COLOR_CALC_STATE */
    uint32_t cmd_stencil_ref;
};

static inline struct intel_viewport_state *intel_viewport_state(XGL_VIEWPORT_STATE_OBJECT state)
{
    return (struct intel_viewport_state *) state;
}

static inline struct intel_viewport_state *intel_viewport_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_viewport_state *) obj;
}

static inline struct intel_raster_state *intel_raster_state(XGL_RASTER_STATE_OBJECT state)
{
    return (struct intel_raster_state *) state;
}

static inline struct intel_raster_state *intel_raster_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_raster_state *) obj;
}

static inline struct intel_msaa_state *intel_msaa_state(XGL_VIEWPORT_STATE_OBJECT state)
{
    return (struct intel_msaa_state *) state;
}

static inline struct intel_msaa_state *intel_msaa_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_msaa_state *) obj;
}

static inline struct intel_blend_state *intel_blend_state(XGL_VIEWPORT_STATE_OBJECT state)
{
    return (struct intel_blend_state *) state;
}

static inline struct intel_blend_state *intel_blend_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_blend_state *) obj;
}

static inline struct intel_ds_state *intel_ds_state(XGL_VIEWPORT_STATE_OBJECT state)
{
    return (struct intel_ds_state *) state;
}

static inline struct intel_ds_state *intel_ds_state_from_obj(struct intel_obj *obj)
{
    return (struct intel_ds_state *) obj;
}

XGL_RESULT intel_viewport_state_create(struct intel_dev *dev,
                                       const XGL_VIEWPORT_STATE_CREATE_INFO *info,
                                       struct intel_viewport_state **state_ret);
void intel_viewport_state_destroy(struct intel_viewport_state *state);

XGL_RESULT intel_raster_state_create(struct intel_dev *dev,
                                     const XGL_RASTER_STATE_CREATE_INFO *info,
                                     struct intel_raster_state **state_ret);
void intel_raster_state_destroy(struct intel_raster_state *state);

XGL_RESULT intel_msaa_state_create(struct intel_dev *dev,
                                   const XGL_MSAA_STATE_CREATE_INFO *info,
                                   struct intel_msaa_state **state_ret);
void intel_msaa_state_destroy(struct intel_msaa_state *state);

XGL_RESULT intel_blend_state_create(struct intel_dev *dev,
                                    const XGL_COLOR_BLEND_STATE_CREATE_INFO *info,
                                    struct intel_blend_state **state_ret);
void intel_blend_state_destroy(struct intel_blend_state *state);

XGL_RESULT intel_ds_state_create(struct intel_dev *dev,
                                 const XGL_DEPTH_STENCIL_STATE_CREATE_INFO *info,
                                 struct intel_ds_state **state_ret);
void intel_ds_state_destroy(struct intel_ds_state *state);

XGL_RESULT XGLAPI intelCreateViewportState(
    XGL_DEVICE                                  device,
    const XGL_VIEWPORT_STATE_CREATE_INFO*       pCreateInfo,
    XGL_VIEWPORT_STATE_OBJECT*                  pState);

XGL_RESULT XGLAPI intelCreateRasterState(
    XGL_DEVICE                                  device,
    const XGL_RASTER_STATE_CREATE_INFO*         pCreateInfo,
    XGL_RASTER_STATE_OBJECT*                    pState);

XGL_RESULT XGLAPI intelCreateMsaaState(
    XGL_DEVICE                                  device,
    const XGL_MSAA_STATE_CREATE_INFO*           pCreateInfo,
    XGL_MSAA_STATE_OBJECT*                      pState);

XGL_RESULT XGLAPI intelCreateColorBlendState(
    XGL_DEVICE                                  device,
    const XGL_COLOR_BLEND_STATE_CREATE_INFO*    pCreateInfo,
    XGL_COLOR_BLEND_STATE_OBJECT*               pState);

XGL_RESULT XGLAPI intelCreateDepthStencilState(
    XGL_DEVICE                                  device,
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO*  pCreateInfo,
    XGL_DEPTH_STENCIL_STATE_OBJECT*             pState);

#endif /* STATE_H */
