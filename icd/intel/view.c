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
#include "gpu.h"
#include "img.h"
#include "mem.h"
#include "view.h"

void intel_null_view_init(struct intel_null_view *view,
                          struct intel_dev *dev)
{
    memset(view, 0, sizeof(*view));
}

void intel_mem_view_init(struct intel_mem_view *view,
                         struct intel_dev *dev,
                         const XGL_MEMORY_VIEW_ATTACH_INFO *info)
{
    memset(view, 0, sizeof(*view));

    view->mem = intel_mem(info->mem);
}

static void img_view_destroy(struct intel_obj *obj)
{
    struct intel_img_view *view = intel_img_view_from_obj(obj);

    intel_img_view_destroy(view);
}

XGL_RESULT intel_img_view_create(struct intel_dev *dev,
                                 const XGL_IMAGE_VIEW_CREATE_INFO *info,
                                 struct intel_img_view **view_ret)
{
    struct intel_img *img = intel_img(info->image);
    struct intel_img_view *view;

    view = (struct intel_img_view *) intel_base_create(dev, sizeof(*view),
            dev->base.dbg, XGL_DBG_OBJECT_IMAGE_VIEW, info, 0);
    if (!view)
        return XGL_ERROR_OUT_OF_MEMORY;

    view->obj.destroy = img_view_destroy;

    view->img = img;
    view->swizzles = info->channels;
    view->min_lod = info->minLod;

    *view_ret = view;

    return XGL_SUCCESS;
}

void intel_img_view_destroy(struct intel_img_view *view)
{
    intel_base_destroy(&view->obj.base);
}

static void rt_view_destroy(struct intel_obj *obj)
{
    struct intel_rt_view *view = intel_rt_view_from_obj(obj);

    intel_rt_view_destroy(view);
}

XGL_RESULT intel_rt_view_create(struct intel_dev *dev,
                                const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO *info,
                                struct intel_rt_view **view_ret)
{
    struct intel_img *img = intel_img(info->image);
    struct intel_rt_view *view;

    view = (struct intel_rt_view *) intel_base_create(dev, sizeof(*view),
            dev->base.dbg, XGL_DBG_OBJECT_COLOR_TARGET_VIEW, info, 0);
    if (!view)
        return XGL_ERROR_OUT_OF_MEMORY;

    view->obj.destroy = rt_view_destroy;

    view->img = img;

    *view_ret = view;

    return XGL_SUCCESS;
}

void intel_rt_view_destroy(struct intel_rt_view *view)
{
    intel_base_destroy(&view->obj.base);
}

static void ds_view_destroy(struct intel_obj *obj)
{
    struct intel_ds_view *view = intel_ds_view_from_obj(obj);

    intel_ds_view_destroy(view);
}

XGL_RESULT intel_ds_view_create(struct intel_dev *dev,
                                const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO *info,
                                struct intel_ds_view **view_ret)
{
    struct intel_img *img = intel_img(info->image);
    struct intel_ds_view *view;

    view = (struct intel_ds_view *) intel_base_create(dev, sizeof(*view),
            dev->base.dbg, XGL_DBG_OBJECT_DEPTH_STENCIL_VIEW, info, 0);
    if (!view)
        return XGL_ERROR_OUT_OF_MEMORY;

    view->obj.destroy = ds_view_destroy;

    view->img = img;

    *view_ret = view;

    return XGL_SUCCESS;
}

void intel_ds_view_destroy(struct intel_ds_view *view)
{
    intel_base_destroy(&view->obj.base);
}

XGL_RESULT XGLAPI intelCreateImageView(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_VIEW_CREATE_INFO*           pCreateInfo,
    XGL_IMAGE_VIEW*                             pView)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_img_view_create(dev, pCreateInfo,
            (struct intel_img_view **) pView);
}

XGL_RESULT XGLAPI intelCreateColorAttachmentView(
    XGL_DEVICE                                  device,
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW*                  pView)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_rt_view_create(dev, pCreateInfo,
            (struct intel_rt_view **) pView);
}

XGL_RESULT XGLAPI intelCreateDepthStencilView(
    XGL_DEVICE                                  device,
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*   pCreateInfo,
    XGL_DEPTH_STENCIL_VIEW*                     pView)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_ds_view_create(dev, pCreateInfo,
            (struct intel_ds_view **) pView);
}
