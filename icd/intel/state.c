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

#include "dev.h"
#include "state.h"

static void viewport_state_destroy(struct intel_obj *obj)
{
    struct intel_viewport_state *state = intel_viewport_state_from_obj(obj);

    intel_viewport_state_destroy(state);
}

XGL_RESULT intel_viewport_state_create(struct intel_dev *dev,
                                       const XGL_VIEWPORT_STATE_CREATE_INFO *info,
                                       struct intel_viewport_state **state_ret)
{
    struct intel_viewport_state *state;

    state = (struct intel_viewport_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_VIEWPORT_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = viewport_state_destroy;

    //emit_viewport_state(dev->gpu, info, state->cmd);

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_viewport_state_destroy(struct intel_viewport_state *state)
{
    intel_base_destroy(&state->obj.base);
}

static void raster_state_destroy(struct intel_obj *obj)
{
    struct intel_raster_state *state = intel_raster_state_from_obj(obj);

    intel_raster_state_destroy(state);
}

XGL_RESULT intel_raster_state_create(struct intel_dev *dev,
                                     const XGL_RASTER_STATE_CREATE_INFO *info,
                                     struct intel_raster_state **state_ret)
{
    struct intel_raster_state *state;

    state = (struct intel_raster_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_RASTER_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = raster_state_destroy;

    //emit_raster_state(dev->gpu, info, state->cmd);

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_raster_state_destroy(struct intel_raster_state *state)
{
    intel_base_destroy(&state->obj.base);
}

static void msaa_state_destroy(struct intel_obj *obj)
{
    struct intel_msaa_state *state = intel_msaa_state_from_obj(obj);

    intel_msaa_state_destroy(state);
}

XGL_RESULT intel_msaa_state_create(struct intel_dev *dev,
                                   const XGL_MSAA_STATE_CREATE_INFO *info,
                                   struct intel_msaa_state **state_ret)
{
    struct intel_msaa_state *state;

    state = (struct intel_msaa_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_MSAA_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = msaa_state_destroy;

    //emit_msaa_state(dev->gpu, info, state->cmd);

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_msaa_state_destroy(struct intel_msaa_state *state)
{
    intel_base_destroy(&state->obj.base);
}

static void blend_state_destroy(struct intel_obj *obj)
{
    struct intel_blend_state *state = intel_blend_state_from_obj(obj);

    intel_blend_state_destroy(state);
}

XGL_RESULT intel_blend_state_create(struct intel_dev *dev,
                                    const XGL_COLOR_BLEND_STATE_CREATE_INFO *info,
                                    struct intel_blend_state **state_ret)
{
    struct intel_blend_state *state;

    state = (struct intel_blend_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_MSAA_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = blend_state_destroy;

    //emit_blend_state(dev->gpu, info, state->cmd);

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_blend_state_destroy(struct intel_blend_state *state)
{
    intel_base_destroy(&state->obj.base);
}

static void ds_state_destroy(struct intel_obj *obj)
{
    struct intel_ds_state *state = intel_ds_state_from_obj(obj);

    intel_ds_state_destroy(state);
}

XGL_RESULT intel_ds_state_create(struct intel_dev *dev,
                                 const XGL_DEPTH_STENCIL_STATE_CREATE_INFO *info,
                                 struct intel_ds_state **state_ret)
{
    struct intel_ds_state *state;

    state = (struct intel_ds_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_MSAA_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = ds_state_destroy;

    //emit_ds_state(dev->gpu, info, state->cmd);

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_ds_state_destroy(struct intel_ds_state *state)
{
    intel_base_destroy(&state->obj.base);
}

XGL_RESULT XGLAPI intelCreateViewportState(
    XGL_DEVICE                                  device,
    const XGL_VIEWPORT_STATE_CREATE_INFO*       pCreateInfo,
    XGL_VIEWPORT_STATE_OBJECT*                  pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_viewport_state_create(dev, pCreateInfo,
            (struct intel_viewport_state **) pState);
}

XGL_RESULT XGLAPI intelCreateRasterState(
    XGL_DEVICE                                  device,
    const XGL_RASTER_STATE_CREATE_INFO*         pCreateInfo,
    XGL_RASTER_STATE_OBJECT*                    pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_raster_state_create(dev, pCreateInfo,
            (struct intel_raster_state **) pState);
}

XGL_RESULT XGLAPI intelCreateMsaaState(
    XGL_DEVICE                                  device,
    const XGL_MSAA_STATE_CREATE_INFO*           pCreateInfo,
    XGL_MSAA_STATE_OBJECT*                      pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_msaa_state_create(dev, pCreateInfo,
            (struct intel_msaa_state **) pState);
}

XGL_RESULT XGLAPI intelCreateColorBlendState(
    XGL_DEVICE                                  device,
    const XGL_COLOR_BLEND_STATE_CREATE_INFO*    pCreateInfo,
    XGL_COLOR_BLEND_STATE_OBJECT*               pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_blend_state_create(dev, pCreateInfo,
            (struct intel_blend_state **) pState);
}

XGL_RESULT XGLAPI intelCreateDepthStencilState(
    XGL_DEVICE                                  device,
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO*  pCreateInfo,
    XGL_DEPTH_STENCIL_STATE_OBJECT*             pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_ds_state_create(dev, pCreateInfo,
            (struct intel_ds_state **) pState);
}
