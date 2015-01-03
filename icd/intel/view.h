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

#ifndef VIEW_H
#define VIEW_H

#include "obj.h"
#include "intel.h"

struct intel_img;
struct intel_mem;

struct intel_null_view {
    /* this is not an intel_obj */

    /* SURFACE_STATE */
    uint32_t cmd[8];
    XGL_UINT cmd_len;
};

struct intel_mem_view {
    /* this is not an intel_obj */

    struct intel_mem *mem;
    XGL_MEMORY_VIEW_ATTACH_INFO info;

    /* SURFACE_STATE */
    uint32_t cmd[8];
    XGL_UINT cmd_len;
};

struct intel_img_view {
    struct intel_obj obj;

    struct intel_img *img;

    XGL_FLOAT min_lod;
    XGL_CHANNEL_MAPPING shader_swizzles;

    /* SURFACE_STATE */
    uint32_t cmd[8];
    XGL_UINT cmd_len;
};

struct intel_rt_view {
    struct intel_obj obj;

    struct intel_img *img;

    /* SURFACE_STATE */
    uint32_t cmd[8];
    XGL_UINT cmd_len;
};

struct intel_ds_view {
    struct intel_obj obj;

    struct intel_img *img;

    /*
     * 3DSTATE_DEPTH_BUFFER
     * 3DSTATE_STENCIL_BUFFER
     * 3DSTATE_HIER_DEPTH_BUFFER
     */
    uint32_t cmd[10];
};

static inline struct intel_img_view *intel_img_view(XGL_IMAGE_VIEW view)
{
    return (struct intel_img_view *) view;
}

static inline struct intel_img_view *intel_img_view_from_obj(struct intel_obj *obj)
{
    return (struct intel_img_view *) obj;
}

static inline struct intel_rt_view *intel_rt_view(XGL_COLOR_ATTACHMENT_VIEW view)
{
    return (struct intel_rt_view *) view;
}

static inline struct intel_rt_view *intel_rt_view_from_obj(struct intel_obj *obj)
{
    return (struct intel_rt_view *) obj;
}

static inline struct intel_ds_view *intel_ds_view(XGL_DEPTH_STENCIL_VIEW view)
{
    return (struct intel_ds_view *) view;
}

static inline struct intel_ds_view *intel_ds_view_from_obj(struct intel_obj *obj)
{
    return (struct intel_ds_view *) obj;
}

void intel_null_view_init(struct intel_null_view *view,
                          struct intel_dev *dev);

void intel_mem_view_init(struct intel_mem_view *view,
                         struct intel_dev *dev,
                         const XGL_MEMORY_VIEW_ATTACH_INFO *info);

XGL_RESULT intel_img_view_create(struct intel_dev *dev,
                                 const XGL_IMAGE_VIEW_CREATE_INFO *info,
                                 struct intel_img_view **view_ret);
void intel_img_view_destroy(struct intel_img_view *view);

XGL_RESULT intel_rt_view_create(struct intel_dev *dev,
                                const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO *info,
                                struct intel_rt_view **view_ret);
void intel_rt_view_destroy(struct intel_rt_view *view);

XGL_RESULT intel_ds_view_create(struct intel_dev *dev,
                                const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO *info,
                                struct intel_ds_view **view_ret);
void intel_ds_view_destroy(struct intel_ds_view *view);

#endif /* VIEW_H */
