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

#include "icd-format.h"

static const struct icd_format_info {
    XGL_SIZE size;
    XGL_UINT channel_count;
} icd_format_table[XGL_MAX_CH_FMT + 1] = {
    [XGL_CH_FMT_UNDEFINED]      = { 0,  0 },
    [XGL_CH_FMT_R4G4]           = { 1,  2 },
    [XGL_CH_FMT_R4G4B4A4]       = { 2,  4 },
    [XGL_CH_FMT_R5G6B5]         = { 2,  3 },
    [XGL_CH_FMT_B5G6R5]         = { 2,  3 },
    [XGL_CH_FMT_R5G5B5A1]       = { 2,  4 },
    [XGL_CH_FMT_R8]             = { 1,  1 },
    [XGL_CH_FMT_R8G8]           = { 2,  2 },
    [XGL_CH_FMT_R8G8B8A8]       = { 4,  4 },
    [XGL_CH_FMT_B8G8R8A8]       = { 4,  4 },
    [XGL_CH_FMT_R10G11B11]      = { 4,  3 },
    [XGL_CH_FMT_R11G11B10]      = { 4,  3 },
    [XGL_CH_FMT_R10G10B10A2]    = { 4,  4 },
    [XGL_CH_FMT_R16]            = { 2,  1 },
    [XGL_CH_FMT_R16G16]         = { 4,  2 },
    [XGL_CH_FMT_R16G16B16A16]   = { 8,  4 },
    [XGL_CH_FMT_R32]            = { 4,  1 },
    [XGL_CH_FMT_R32G32]         = { 8,  2 },
    [XGL_CH_FMT_R32G32B32]      = { 12, 3 },
    [XGL_CH_FMT_R32G32B32A32]   = { 16, 4 },
    [XGL_CH_FMT_R16G8]          = { 3,  2 },
    [XGL_CH_FMT_R32G8]          = { 5,  2 },
    [XGL_CH_FMT_R9G9B9E5]       = { 4,  3 },
    [XGL_CH_FMT_BC1]            = { 8,  4 },
    [XGL_CH_FMT_BC2]            = { 16, 4 },
    [XGL_CH_FMT_BC3]            = { 16, 4 },
    [XGL_CH_FMT_BC4]            = { 8,  4 },
    [XGL_CH_FMT_BC5]            = { 16, 4 },
    [XGL_CH_FMT_BC6U]           = { 16, 4 },
    [XGL_CH_FMT_BC6S]           = { 16, 4 },
    [XGL_CH_FMT_BC7]            = { 16, 4 },
};

unsigned int icd_format_get_size(XGL_FORMAT format)
{
    return icd_format_table[format.channelFormat].size;
}

unsigned int icd_format_get_channel_count(XGL_FORMAT format)
{
    return icd_format_table[format.channelFormat].channel_count;
}
