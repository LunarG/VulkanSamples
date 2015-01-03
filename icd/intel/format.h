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

#ifndef FORMAT_H
#define FORMAT_H

#include "intel.h"

struct intel_gpu;

static inline bool intel_format_is_depth(const struct intel_gpu *gpu,
                                         XGL_FORMAT format)
{
    return (format.numericFormat == XGL_NUM_FMT_DS &&
            (format.channelFormat == XGL_CH_FMT_R16 ||
             format.channelFormat == XGL_CH_FMT_R32));
}

static inline bool intel_format_is_stencil(const struct intel_gpu *gpu,
                                           XGL_FORMAT format)
{
   return (format.numericFormat == XGL_NUM_FMT_DS &&
           format.channelFormat == XGL_CH_FMT_R8);
}

int intel_format_translate_color(const struct intel_gpu *gpu,
                                 XGL_FORMAT format);

#endif /* FORMAT_H */
