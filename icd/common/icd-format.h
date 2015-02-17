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
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ICD_FORMAT_H
#define ICD_FORMAT_H

#include <stdbool.h>
#include "icd.h"

static inline bool icd_format_is_undef(XGL_FORMAT format)
{
    return (format == XGL_FMT_UNDEFINED);
}

bool icd_format_is_ds(XGL_FORMAT format);

static inline bool icd_format_is_color(XGL_FORMAT format)
{
    return !(icd_format_is_undef(format) || icd_format_is_ds(format));
}

bool icd_format_is_norm(XGL_FORMAT format);

bool icd_format_is_int(XGL_FORMAT format);

bool icd_format_is_float(XGL_FORMAT format);

bool icd_format_is_srgb(XGL_FORMAT format);

bool icd_format_is_compressed(XGL_FORMAT format);

static inline int icd_format_get_block_width(XGL_FORMAT format)
{
    /* all compressed formats use 4x4 blocks */
    return (icd_format_is_compressed(format)) ? 4 : 1;
}

STATIC_INLINE bool icd_blend_mode_is_dual_src(XGL_BLEND mode)
{
    return (mode == XGL_BLEND_SRC1_COLOR) ||
           (mode == XGL_BLEND_SRC1_ALPHA) ||
           (mode == XGL_BLEND_ONE_MINUS_SRC1_COLOR) ||
           (mode == XGL_BLEND_ONE_MINUS_SRC1_ALPHA);
}

STATIC_INLINE bool icd_pipeline_cb_att_needs_dual_source_blending(const XGL_PIPELINE_CB_ATTACHMENT_STATE *att)
{
    if (icd_blend_mode_is_dual_src(att->srcBlendColor) ||
        icd_blend_mode_is_dual_src(att->srcBlendAlpha) ||
        icd_blend_mode_is_dual_src(att->destBlendColor) ||
        icd_blend_mode_is_dual_src(att->destBlendAlpha)) {
        return true;
    }
    return false;
}

size_t icd_format_get_size(XGL_FORMAT format);

XGL_IMAGE_FORMAT_CLASS icd_format_get_class(XGL_FORMAT format);

unsigned int icd_format_get_channel_count(XGL_FORMAT format);

void icd_format_get_raw_value(XGL_FORMAT format,
                              const uint32_t color[4],
                              void *value);

#endif /* ICD_FORMAT_H */
