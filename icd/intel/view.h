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
    uint32_t cmd_len;
};

struct intel_buf_view {
    struct intel_obj obj;

    struct intel_buf *buf;

    /* SURFACE_STATE */
    uint32_t cmd[8];
    uint32_t fs_cmd[8];
    uint32_t cmd_len;
};

struct intel_att_view {
    struct intel_img *img;

    uint32_t mipLevel;
    uint32_t baseArrayLayer;
    uint32_t array_size;

    /* SURFACE_STATE for readback */
    uint32_t cmd[8];
    uint32_t cmd_len;

    /*
     * SURFACE_STATE when is_rt is true.  Otherwise,
     *
     * 3DSTATE_DEPTH_BUFFER
     * 3DSTATE_STENCIL_BUFFER
     * 3DSTATE_HIER_DEPTH_BUFFER
     */
    uint32_t att_cmd[10];
    bool is_rt;
    bool has_stencil;
    bool has_hiz;
};

struct intel_img_view {
    struct intel_obj obj;

    struct intel_img *img;

    VkChannelMapping shader_swizzles;

    /* SURFACE_STATE */
    uint32_t cmd[8];
    uint32_t cmd_len;

    struct intel_att_view att_view;
};

static inline struct intel_buf_view *intel_buf_view(VkBufferView view)
{
    return *(struct intel_buf_view **) &view;
}

static inline struct intel_buf_view *intel_buf_view_from_obj(struct intel_obj *obj)
{
    return (struct intel_buf_view *) obj;
}

static inline struct intel_img_view *intel_img_view(VkImageView view)
{
    return *(struct intel_img_view **) &view;
}

static inline struct intel_img_view *intel_img_view_from_obj(struct intel_obj *obj)
{
    return (struct intel_img_view *) obj;
}

void intel_null_view_init(struct intel_null_view *view,
                          struct intel_dev *dev);

VkResult intel_buf_view_create(struct intel_dev *dev,
                                 const VkBufferViewCreateInfo *info,
                                 struct intel_buf_view **view_ret);

void intel_buf_view_destroy(struct intel_buf_view *view);

void intel_img_view_init(struct intel_dev *dev, const VkImageViewCreateInfo *info,
                         struct intel_img_view *view);

VkResult intel_img_view_create(struct intel_dev *dev,
                                 const VkImageViewCreateInfo *info,
                                 struct intel_img_view **view_ret);
void intel_img_view_destroy(struct intel_img_view *view);

void intel_att_view_init(struct intel_dev *dev,
                         const VkImageViewCreateInfo *info,
                         struct intel_att_view *att_view);
void intel_att_view_destroy(struct intel_att_view *view);

#endif /* VIEW_H */
