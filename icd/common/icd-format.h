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
    return (format.numericFormat == XGL_NUM_FMT_UNDEFINED);
}

static inline bool icd_format_is_ds(XGL_FORMAT format)
{
    return (format.numericFormat == XGL_NUM_FMT_DS);
}

static inline bool icd_format_is_color(XGL_FORMAT format)
{
    return !(icd_format_is_undef(format) || icd_format_is_ds(format));
}

static inline bool icd_format_is_norm(XGL_FORMAT format)
{
    return (format.numericFormat == XGL_NUM_FMT_UNORM ||
            format.numericFormat == XGL_NUM_FMT_SNORM);
};

static inline bool icd_format_is_int(XGL_FORMAT format)
{
    return (format.numericFormat == XGL_NUM_FMT_UINT ||
            format.numericFormat == XGL_NUM_FMT_SINT);
}

static inline bool icd_format_is_float(XGL_FORMAT format)
{
    return (format.numericFormat == XGL_NUM_FMT_FLOAT);
}

static inline bool icd_format_is_srgb(XGL_FORMAT format)
{
    return (format.numericFormat == XGL_NUM_FMT_SRGB);
}

static inline bool icd_format_is_compressed(XGL_FORMAT format)
{
    switch (format.channelFormat) {
    case XGL_CH_FMT_BC1:
    case XGL_CH_FMT_BC2:
    case XGL_CH_FMT_BC3:
    case XGL_CH_FMT_BC4:
    case XGL_CH_FMT_BC5:
    case XGL_CH_FMT_BC6U:
    case XGL_CH_FMT_BC6S:
    case XGL_CH_FMT_BC7:
        return true;
    default:
        return false;
    }
}

static inline bool icd_format_is_equal(XGL_FORMAT a, XGL_FORMAT b)
{
    return (a.channelFormat == b.channelFormat &&
            a.numericFormat == b.numericFormat);
}

static inline int icd_format_get_block_width(XGL_FORMAT format)
{
    /* all compressed formats use 4x4 blocks */
    return (icd_format_is_compressed(format)) ? 4 : 1;
}

XGL_SIZE icd_format_get_size(XGL_FORMAT format);

XGL_UINT icd_format_get_channel_count(XGL_FORMAT format);

void icd_format_get_raw_value(XGL_FORMAT format,
                              const XGL_UINT32 color[4],
                              void *value);

#endif /* ICD_FORMAT_H */
