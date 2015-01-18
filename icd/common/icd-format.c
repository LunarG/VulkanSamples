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

#include <string.h> /* for memcpy */
#include "icd-utils.h"
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
    [XGL_CH_FMT_R8G8B8]         = { 3,  3 },
    [XGL_CH_FMT_R16G16B16]      = { 6,  3 },
    [XGL_CH_FMT_B10G10R10A2]    = { 4,  4 },
    [XGL_CH_FMT_R64]            = { 8,  1 },
    [XGL_CH_FMT_R64G64]         = { 16, 2 },
    [XGL_CH_FMT_R64G64B64]      = { 24, 3 },
    [XGL_CH_FMT_R64G64B64A64]   = { 32, 4 },
};

size_t icd_format_get_size(XGL_FORMAT format)
{
    return icd_format_table[format.channelFormat].size;
}

unsigned int icd_format_get_channel_count(XGL_FORMAT format)
{
    return icd_format_table[format.channelFormat].channel_count;
}

/**
 * Convert a raw RGBA color to a raw value.  \p value must have at least
 * icd_format_get_size(format) bytes.
 */
void icd_format_get_raw_value(XGL_FORMAT format,
                              const XGL_UINT32 color[4],
                              void *value)
{
    /* assume little-endian */
    switch (format.channelFormat) {
    case XGL_CH_FMT_UNDEFINED:
        break;
    case XGL_CH_FMT_R4G4:
        ((uint8_t *) value)[0]  = (color[0] & 0xf) << 0   |
                                  (color[1] & 0xf) << 4;
        break;
    case XGL_CH_FMT_R4G4B4A4:
        ((uint16_t *) value)[0] = (color[0] & 0xf) << 0   |
                                  (color[1] & 0xf) << 4   |
                                  (color[2] & 0xf) << 8   |
                                  (color[3] & 0xf) << 12;
        break;
    case XGL_CH_FMT_R5G6B5:
        ((uint16_t *) value)[0] = (color[0] & 0x1f) << 0  |
                                  (color[1] & 0x3f) << 5  |
                                  (color[2] & 0x1f) << 11;
        break;
    case XGL_CH_FMT_B5G6R5:
        ((uint16_t *) value)[0] = (color[2] & 0x1f) << 0  |
                                  (color[1] & 0x3f) << 5  |
                                  (color[0] & 0x1f) << 11;
        break;
    case XGL_CH_FMT_R5G5B5A1:
        ((uint16_t *) value)[0] = (color[0] & 0x1f) << 0  |
                                  (color[1] & 0x1f) << 5  |
                                  (color[2] & 0x1f) << 10 |
                                  (color[3] & 0x1)  << 15;
        break;
    case XGL_CH_FMT_R8:
        ((uint8_t *) value)[0]  = (uint8_t) color[0];
        break;
    case XGL_CH_FMT_R8G8:
        ((uint8_t *) value)[0]  = (uint8_t) color[0];
        ((uint8_t *) value)[1]  = (uint8_t) color[1];
        break;
    case XGL_CH_FMT_R8G8B8A8:
        ((uint8_t *) value)[0]  = (uint8_t) color[0];
        ((uint8_t *) value)[1]  = (uint8_t) color[1];
        ((uint8_t *) value)[2]  = (uint8_t) color[2];
        ((uint8_t *) value)[3]  = (uint8_t) color[3];
        break;
    case XGL_CH_FMT_B8G8R8A8:
        ((uint8_t *) value)[0]  = (uint8_t) color[2];
        ((uint8_t *) value)[1]  = (uint8_t) color[1];
        ((uint8_t *) value)[2]  = (uint8_t) color[0];
        ((uint8_t *) value)[3]  = (uint8_t) color[3];
        break;
    case XGL_CH_FMT_R10G11B11:
        ((uint32_t *) value)[0] = (color[0] & 0x3ff) << 0  |
                                  (color[1] & 0x7ff) << 10 |
                                  (color[2] & 0x7ff) << 21;
        break;
    case XGL_CH_FMT_R11G11B10:
        ((uint32_t *) value)[0] = (color[0] & 0x7ff) << 0  |
                                  (color[1] & 0x7ff) << 11 |
                                  (color[2] & 0x3ff) << 22;
        break;
    case XGL_CH_FMT_R10G10B10A2:
        ((uint32_t *) value)[0] = (color[0] & 0x3ff) << 0  |
                                  (color[1] & 0x3ff) << 10 |
                                  (color[2] & 0x3ff) << 20 |
                                  (color[3] & 0x3)   << 30;
        break;
    case XGL_CH_FMT_R16:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        break;
    case XGL_CH_FMT_R16G16:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        ((uint16_t *) value)[1] = (uint16_t) color[1];
        break;
    case XGL_CH_FMT_R16G16B16A16:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        ((uint16_t *) value)[1] = (uint16_t) color[1];
        ((uint16_t *) value)[2] = (uint16_t) color[2];
        ((uint16_t *) value)[3] = (uint16_t) color[3];
        break;
    case XGL_CH_FMT_R32:
        ((uint32_t *) value)[0] = color[0];
        break;
    case XGL_CH_FMT_R32G32:
        ((uint32_t *) value)[0] = color[0];
        ((uint32_t *) value)[1] = color[1];
        break;
    case XGL_CH_FMT_R32G32B32:
        ((uint32_t *) value)[0] = color[0];
        ((uint32_t *) value)[1] = color[1];
        ((uint32_t *) value)[2] = color[2];
        break;
    case XGL_CH_FMT_R32G32B32A32:
        ((uint32_t *) value)[0] = color[0];
        ((uint32_t *) value)[1] = color[1];
        ((uint32_t *) value)[2] = color[2];
        ((uint32_t *) value)[3] = color[3];
        break;
    case XGL_CH_FMT_R16G8:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        ((char *) value)[2] = (uint8_t) color[1];
        break;
    case XGL_CH_FMT_R32G8:
        ((uint32_t *) value)[0] = (uint32_t) color[0];
        ((char *) value)[4] = (uint8_t) color[1];
        break;
    case XGL_CH_FMT_R9G9B9E5:
        ((uint32_t *) value)[0] = (color[0] & 0x1ff) << 0  |
                                  (color[1] & 0x1ff) << 9  |
                                  (color[2] & 0x1ff) << 18 |
                                  (color[3] & 0x1f)  << 27;
        break;
    case XGL_CH_FMT_BC1:
    case XGL_CH_FMT_BC4:
        memcpy(value, color, 8);
        break;
    case XGL_CH_FMT_BC2:
    case XGL_CH_FMT_BC3:
    case XGL_CH_FMT_BC5:
    case XGL_CH_FMT_BC6U:
    case XGL_CH_FMT_BC6S:
    case XGL_CH_FMT_BC7:
        memcpy(value, color, 16);
        break;
    case XGL_CH_FMT_R8G8B8:
        ((uint8_t *) value)[0]  = (uint8_t) color[0];
        ((uint8_t *) value)[1]  = (uint8_t) color[1];
        ((uint8_t *) value)[2]  = (uint8_t) color[2];
        break;
    case XGL_CH_FMT_R16G16B16:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        ((uint16_t *) value)[1] = (uint16_t) color[1];
        ((uint16_t *) value)[2] = (uint16_t) color[2];
        break;
    case XGL_CH_FMT_B10G10R10A2:
        ((uint32_t *) value)[0] = (color[2] & 0x3ff) << 0  |
                                  (color[1] & 0x3ff) << 10 |
                                  (color[0] & 0x3ff) << 20 |
                                  (color[3] & 0x3)   << 30;
        break;
    case XGL_CH_FMT_R64:
        /* higher 32 bits always 0 */
        ((uint64_t *) value)[0] = color[0];
        break;
    case XGL_CH_FMT_R64G64:
        ((uint64_t *) value)[0] = color[0];
        ((uint64_t *) value)[1] = color[1];
        break;
    case XGL_CH_FMT_R64G64B64:
        ((uint64_t *) value)[0] = color[0];
        ((uint64_t *) value)[1] = color[1];
        ((uint64_t *) value)[2] = color[2];
        break;
    case XGL_CH_FMT_R64G64B64A64:
        ((uint64_t *) value)[0] = color[0];
        ((uint64_t *) value)[1] = color[1];
        ((uint64_t *) value)[2] = color[2];
        ((uint64_t *) value)[3] = color[3];
        break;
    default:
        assert(!"unknown format");
        break;
    }
}
