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

#ifndef IMG_H
#define IMG_H

#include "kmd/winsys.h"
#include "intel.h"
#include "layout.h"
#include "obj.h"

struct intel_img {
    struct intel_obj obj;

    XGL_IMAGE_TYPE type;
    int32_t depth;
    uint32_t mip_levels;
    uint32_t array_size;
    XGL_FLAGS usage;
    XGL_IMAGE_FORMAT_CLASS format_class;  // should this be integrated into intel_layout?
    uint32_t samples;
    struct intel_layout layout;

    /* layout of separate stencil */
    struct intel_layout *s8_layout;

    size_t total_size;
    size_t aux_offset;
    size_t s8_offset;

    void *wsi_data;
};

static inline struct intel_img *intel_img(XGL_IMAGE image)
{
    return (struct intel_img *) image;
}

static inline struct intel_img *intel_img_from_base(struct intel_base *base)
{
    return (struct intel_img *) base;
}

static inline struct intel_img *intel_img_from_obj(struct intel_obj *obj)
{
    return intel_img_from_base(&obj->base);
}

XGL_RESULT intel_img_create(struct intel_dev *dev,
                            const XGL_IMAGE_CREATE_INFO *info,
                            bool scanout,
                            struct intel_img **img_ret);

void intel_img_destroy(struct intel_img *img);

static inline bool intel_img_can_enable_hiz(const struct intel_img *img,
                                            uint32_t level)
{
    return (img->layout.aux == INTEL_LAYOUT_AUX_HIZ &&
            img->layout.aux_enables & (1 << level));
}

#endif /* IMG_H */
