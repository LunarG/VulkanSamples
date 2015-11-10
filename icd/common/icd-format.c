/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Jeremy Hayes <jeremy@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 *
 */

#include <string.h> /* for memcpy */
#include "icd-utils.h"
#include "icd-format.h"

static const struct icd_format_info {
    size_t size;
    uint32_t channel_count;
} icd_format_table[VK_FORMAT_RANGE_SIZE] = {
    [VK_FORMAT_UNDEFINED]            = { 0,  0 },
    [VK_FORMAT_R4G4_UNORM_PACK8]           = { 1,  2 },
    [VK_FORMAT_R4G4_USCALED]         = { 1,  2 },
    [VK_FORMAT_R4G4B4A4_UNORM_PACK16]       = { 2,  4 },
    [VK_FORMAT_R4G4B4A4_USCALED]     = { 2,  4 },
    [VK_FORMAT_R5G6B5_UNORM_PACK16]         = { 2,  3 },
    [VK_FORMAT_R5G6B5_USCALED]       = { 2,  3 },
    [VK_FORMAT_B5G6R5_UNORM_PACK16]         = { 2, 3 },
    [VK_FORMAT_B5G6R5_USCALED]       = { 2, 3 },
    [VK_FORMAT_R5G5B5A1_UNORM_PACK16]       = { 2,  4 },
    [VK_FORMAT_R5G5B5A1_USCALED]     = { 2,  4 },
    [VK_FORMAT_R8_UNORM]             = { 1,  1 },
    [VK_FORMAT_R8_SNORM]             = { 1,  1 },
    [VK_FORMAT_R8_USCALED]           = { 1,  1 },
    [VK_FORMAT_R8_SSCALED]           = { 1,  1 },
    [VK_FORMAT_R8_UINT]              = { 1,  1 },
    [VK_FORMAT_R8_SINT]              = { 1,  1 },
    [VK_FORMAT_R8_SRGB]              = { 1,  1 },
    [VK_FORMAT_R8G8_UNORM]           = { 2,  2 },
    [VK_FORMAT_R8G8_SNORM]           = { 2,  2 },
    [VK_FORMAT_R8G8_USCALED]         = { 2,  2 },
    [VK_FORMAT_R8G8_SSCALED]         = { 2,  2 },
    [VK_FORMAT_R8G8_UINT]            = { 2,  2 },
    [VK_FORMAT_R8G8_SINT]            = { 2,  2 },
    [VK_FORMAT_R8G8_SRGB]            = { 2,  2 },
    [VK_FORMAT_R8G8B8_UNORM]         = { 3,  3 },
    [VK_FORMAT_R8G8B8_SNORM]         = { 3,  3 },
    [VK_FORMAT_R8G8B8_USCALED]       = { 3,  3 },
    [VK_FORMAT_R8G8B8_SSCALED]       = { 3,  3 },
    [VK_FORMAT_R8G8B8_UINT]          = { 3,  3 },
    [VK_FORMAT_R8G8B8_SINT]          = { 3,  3 },
    [VK_FORMAT_R8G8B8_SRGB]          = { 3,  3 },
    [VK_FORMAT_B8G8R8_UNORM]         = { 3, 3 },
    [VK_FORMAT_B8G8R8_SNORM]         = { 3, 3 },
    [VK_FORMAT_B8G8R8_USCALED]       = { 3, 3 },
    [VK_FORMAT_B8G8R8_SSCALED]       = { 3, 3 },
    [VK_FORMAT_B8G8R8_UINT]          = { 3, 3 },
    [VK_FORMAT_B8G8R8_SINT]          = { 3, 3 },
    [VK_FORMAT_B8G8R8_SRGB]          = { 3, 3 },
    [VK_FORMAT_R8G8B8A8_UNORM]       = { 4,  4 },
    [VK_FORMAT_R8G8B8A8_SNORM]       = { 4,  4 },
    [VK_FORMAT_R8G8B8A8_USCALED]     = { 4,  4 },
    [VK_FORMAT_R8G8B8A8_SSCALED]     = { 4,  4 },
    [VK_FORMAT_R8G8B8A8_UINT]        = { 4,  4 },
    [VK_FORMAT_R8G8B8A8_SINT]        = { 4,  4 },
    [VK_FORMAT_R8G8B8A8_SRGB]        = { 4,  4 },
    [VK_FORMAT_B8G8R8A8_UNORM]       = { 4, 4 },
    [VK_FORMAT_B8G8R8A8_SNORM]       = { 4, 4 },
    [VK_FORMAT_B8G8R8A8_USCALED]     = { 4, 4 },
    [VK_FORMAT_B8G8R8A8_SSCALED]     = { 4, 4 },
    [VK_FORMAT_B8G8R8A8_UINT]        = { 4, 4 },
    [VK_FORMAT_B8G8R8A8_SINT]        = { 4, 4 },
    [VK_FORMAT_B8G8R8A8_SRGB]        = { 4, 4 },
    [VK_FORMAT_A2R10G10B10_UNORM_PACK32]    = { 4, 4 },
    [VK_FORMAT_A2R10G10B10_SNORM_PACK32]    = { 4, 4 },
    [VK_FORMAT_A2R10G10B10_USCALED_PACK32]  = { 4, 4 },
    [VK_FORMAT_A2R10G10B10_SSCALED_PACK32]  = { 4, 4 },
    [VK_FORMAT_A2R10G10B10_UINT_PACK32]     = { 4, 4 },
    [VK_FORMAT_A2R10G10B10_SINT_PACK32]     = { 4, 4 },
    [VK_FORMAT_A2B10G10R10_UNORM_PACK32]    = { 4,  4 },
    [VK_FORMAT_A2B10G10R10_SNORM_PACK32]    = { 4,  4 },
    [VK_FORMAT_A2B10G10R10_USCALED_PACK32]  = { 4,  4 },
    [VK_FORMAT_A2B10G10R10_SSCALED_PACK32]  = { 4,  4 },
    [VK_FORMAT_A2B10G10R10_UINT_PACK32]     = { 4,  4 },
    [VK_FORMAT_A2B10G10R10_SINT_PACK32]     = { 4,  4 },
    [VK_FORMAT_R16_UNORM]            = { 2,  1 },
    [VK_FORMAT_R16_SNORM]            = { 2,  1 },
    [VK_FORMAT_R16_USCALED]          = { 2,  1 },
    [VK_FORMAT_R16_SSCALED]          = { 2,  1 },
    [VK_FORMAT_R16_UINT]             = { 2,  1 },
    [VK_FORMAT_R16_SINT]             = { 2,  1 },
    [VK_FORMAT_R16_SFLOAT]           = { 2,  1 },
    [VK_FORMAT_R16G16_UNORM]         = { 4,  2 },
    [VK_FORMAT_R16G16_SNORM]         = { 4,  2 },
    [VK_FORMAT_R16G16_USCALED]       = { 4,  2 },
    [VK_FORMAT_R16G16_SSCALED]       = { 4,  2 },
    [VK_FORMAT_R16G16_UINT]          = { 4,  2 },
    [VK_FORMAT_R16G16_SINT]          = { 4,  2 },
    [VK_FORMAT_R16G16_SFLOAT]        = { 4,  2 },
    [VK_FORMAT_R16G16B16_UNORM]      = { 6,  3 },
    [VK_FORMAT_R16G16B16_SNORM]      = { 6,  3 },
    [VK_FORMAT_R16G16B16_USCALED]    = { 6,  3 },
    [VK_FORMAT_R16G16B16_SSCALED]    = { 6,  3 },
    [VK_FORMAT_R16G16B16_UINT]       = { 6,  3 },
    [VK_FORMAT_R16G16B16_SINT]       = { 6,  3 },
    [VK_FORMAT_R16G16B16_SFLOAT]     = { 6,  3 },
    [VK_FORMAT_R16G16B16A16_UNORM]   = { 8,  4 },
    [VK_FORMAT_R16G16B16A16_SNORM]   = { 8,  4 },
    [VK_FORMAT_R16G16B16A16_USCALED] = { 8,  4 },
    [VK_FORMAT_R16G16B16A16_SSCALED] = { 8,  4 },
    [VK_FORMAT_R16G16B16A16_UINT]    = { 8,  4 },
    [VK_FORMAT_R16G16B16A16_SINT]    = { 8,  4 },
    [VK_FORMAT_R16G16B16A16_SFLOAT]  = { 8,  4 },
    [VK_FORMAT_R32_UINT]             = { 4,  1 },
    [VK_FORMAT_R32_SINT]             = { 4,  1 },
    [VK_FORMAT_R32_SFLOAT]           = { 4,  1 },
    [VK_FORMAT_R32G32_UINT]          = { 8,  2 },
    [VK_FORMAT_R32G32_SINT]          = { 8,  2 },
    [VK_FORMAT_R32G32_SFLOAT]        = { 8,  2 },
    [VK_FORMAT_R32G32B32_UINT]       = { 12, 3 },
    [VK_FORMAT_R32G32B32_SINT]       = { 12, 3 },
    [VK_FORMAT_R32G32B32_SFLOAT]     = { 12, 3 },
    [VK_FORMAT_R32G32B32A32_UINT]    = { 16, 4 },
    [VK_FORMAT_R32G32B32A32_SINT]    = { 16, 4 },
    [VK_FORMAT_R32G32B32A32_SFLOAT]  = { 16, 4 },
    [VK_FORMAT_R64_SFLOAT]           = { 8,  1 },
    [VK_FORMAT_R64G64_SFLOAT]        = { 16, 2 },
    [VK_FORMAT_R64G64B64_SFLOAT]     = { 24, 3 },
    [VK_FORMAT_R64G64B64A64_SFLOAT]  = { 32, 4 },
    [VK_FORMAT_B10G11R11_UFLOAT_PACK32]     = { 4,  3 },
    [VK_FORMAT_E5B9G9R9_UFLOAT_PACK32]      = { 4,  3 },
    [VK_FORMAT_D16_UNORM]            = { 2,  1 },
    [VK_FORMAT_X8_D24_UNORM_PACK32]         = { 3,  1 },
    [VK_FORMAT_D32_SFLOAT]           = { 4,  1 },
    [VK_FORMAT_S8_UINT]              = { 1,  1 },
    [VK_FORMAT_D16_UNORM_S8_UINT]    = { 3,  2 },
    [VK_FORMAT_D24_UNORM_S8_UINT]    = { 4,  2 },
    [VK_FORMAT_D32_SFLOAT_S8_UINT]   = { 4,  2 },
    [VK_FORMAT_BC1_RGB_UNORM_BLOCK]        = { 8,  4 },
    [VK_FORMAT_BC1_RGB_SRGB_BLOCK]         = { 8,  4 },
    [VK_FORMAT_BC1_RGBA_UNORM_BLOCK]       = { 8,  4 },
    [VK_FORMAT_BC1_RGBA_SRGB_BLOCK]        = { 8,  4 },
    [VK_FORMAT_BC2_UNORM_BLOCK]            = { 16, 4 },
    [VK_FORMAT_BC2_SRGB_BLOCK]             = { 16, 4 },
    [VK_FORMAT_BC3_UNORM_BLOCK]            = { 16, 4 },
    [VK_FORMAT_BC3_SRGB_BLOCK]             = { 16, 4 },
    [VK_FORMAT_BC4_UNORM_BLOCK]            = { 8,  4 },
    [VK_FORMAT_BC4_SNORM_BLOCK]            = { 8,  4 },
    [VK_FORMAT_BC5_UNORM_BLOCK]            = { 16, 4 },
    [VK_FORMAT_BC5_SNORM_BLOCK]            = { 16, 4 },
    [VK_FORMAT_BC6H_UFLOAT_BLOCK]          = { 16, 4 },
    [VK_FORMAT_BC6H_SFLOAT_BLOCK]          = { 16, 4 },
    [VK_FORMAT_BC7_UNORM_BLOCK]            = { 16, 4 },
    [VK_FORMAT_BC7_SRGB_BLOCK]             = { 16, 4 },
    /* TODO: Initialize remaining compressed formats. */
    [VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK]    = { 0, 0 },
    [VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK]  = { 0, 0 },
    [VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK]  = { 0, 0 },
    [VK_FORMAT_EAC_R11_UNORM_BLOCK]        = { 0, 0 },
    [VK_FORMAT_EAC_R11_SNORM_BLOCK]        = { 0, 0 },
    [VK_FORMAT_EAC_R11G11_UNORM_BLOCK]     = { 0, 0 },
    [VK_FORMAT_EAC_R11G11_SNORM_BLOCK]     = { 0, 0 },
    [VK_FORMAT_ASTC_4x4_UNORM_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_4x4_SRGB_BLOCK]        = { 0, 0 },
    [VK_FORMAT_ASTC_5x4_UNORM_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_5x4_SRGB_BLOCK]        = { 0, 0 },
    [VK_FORMAT_ASTC_5x5_UNORM_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_5x5_SRGB_BLOCK]        = { 0, 0 },
    [VK_FORMAT_ASTC_6x5_UNORM_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_6x5_SRGB_BLOCK]        = { 0, 0 },
    [VK_FORMAT_ASTC_6x6_UNORM_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_6x6_SRGB_BLOCK]        = { 0, 0 },
    [VK_FORMAT_ASTC_8x5_UNORM_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_8x5_SRGB_BLOCK]        = { 0, 0 },
    [VK_FORMAT_ASTC_8x6_UNORM_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_8x6_SRGB_BLOCK]        = { 0, 0 },
    [VK_FORMAT_ASTC_8x8_UNORM_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_8x8_SRGB_BLOCK]        = { 0, 0 },
    [VK_FORMAT_ASTC_10x5_UNORM_BLOCK]      = { 0, 0 },
    [VK_FORMAT_ASTC_10x5_SRGB_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_10x6_UNORM_BLOCK]      = { 0, 0 },
    [VK_FORMAT_ASTC_10x6_SRGB_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_10x8_UNORM_BLOCK]      = { 0, 0 },
    [VK_FORMAT_ASTC_10x8_SRGB_BLOCK]       = { 0, 0 },
    [VK_FORMAT_ASTC_10x10_UNORM_BLOCK]     = { 0, 0 },
    [VK_FORMAT_ASTC_10x10_SRGB_BLOCK]      = { 0, 0 },
    [VK_FORMAT_ASTC_12x10_UNORM_BLOCK]     = { 0, 0 },
    [VK_FORMAT_ASTC_12x10_SRGB_BLOCK]      = { 0, 0 },
    [VK_FORMAT_ASTC_12x12_UNORM_BLOCK]     = { 0, 0 },
    [VK_FORMAT_ASTC_12x12_SRGB_BLOCK]      = { 0, 0 },
};

bool icd_format_is_ds(VkFormat format)
{
    bool is_ds = false;

    switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        is_ds = true;
        break;
    default:
        break;
    }

    return is_ds;
}

bool icd_format_is_norm(VkFormat format)
{
    bool is_norm = false;

    switch (format) {
    case VK_FORMAT_R4G4_UNORM_PACK8:
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_B8G8R8_SNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        is_norm = true;
        break;
    default:
        break;
    }

    return is_norm;
};

bool icd_format_is_int(VkFormat format)
{
    bool is_int = false;

    switch (format) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        is_int = true;
        break;
    default:
        break;
    }

    return is_int;
}

bool icd_format_is_float(VkFormat format)
{
    bool is_float = false;

    switch (format) {
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R16G16B16_SFLOAT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R32G32B32_SFLOAT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    case VK_FORMAT_R64_SFLOAT:
    case VK_FORMAT_R64G64_SFLOAT:
    case VK_FORMAT_R64G64B64_SFLOAT:
    case VK_FORMAT_R64G64B64A64_SFLOAT:
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
        is_float = true;
        break;
    default:
        break;
    }

    return is_float;
}

bool icd_format_is_srgb(VkFormat format)
{
    bool is_srgb = false;

    switch (format) {
    case VK_FORMAT_R8_SRGB:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
        is_srgb = true;
        break;
    default:
        break;
    }

    return is_srgb;
}

bool icd_format_is_compressed(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11_SNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
        return true;
    default:
        return false;
    }
}

size_t icd_format_get_size(VkFormat format)
{
    return icd_format_table[format].size;
}

unsigned int icd_format_get_channel_count(VkFormat format)
{
    return icd_format_table[format].channel_count;
}

/**
 * Convert a raw RGBA color to a raw value.  \p value must have at least
 * icd_format_get_size(format) bytes.
 */
void icd_format_get_raw_value(VkFormat format,
                              const uint32_t color[4],
                              void *value)
{
    /* assume little-endian */
    switch (format) {
    case VK_FORMAT_UNDEFINED:
        break;
    case VK_FORMAT_R4G4_UNORM_PACK8:
    case VK_FORMAT_R4G4_USCALED:
        ((uint8_t *) value)[0]  = (color[0] & 0xf) << 0   |
                                  (color[1] & 0xf) << 4;
        break;
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
    case VK_FORMAT_R4G4B4A4_USCALED:
        ((uint16_t *) value)[0] = (color[0] & 0xf) << 0   |
                                  (color[1] & 0xf) << 4   |
                                  (color[2] & 0xf) << 8   |
                                  (color[3] & 0xf) << 12;
        break;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
    case VK_FORMAT_R5G6B5_USCALED:
        ((uint16_t *) value)[0] = (color[0] & 0x1f) << 0  |
                                  (color[1] & 0x3f) << 5  |
                                  (color[2] & 0x1f) << 11;
        break;
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        ((uint16_t *) value)[0] = (color[2] & 0x1f) << 0  |
                                  (color[1] & 0x3f) << 5  |
                                  (color[0] & 0x1f) << 11;
        break;
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
    case VK_FORMAT_R5G5B5A1_USCALED:
        ((uint16_t *) value)[0] = (color[0] & 0x1f) << 0  |
                                  (color[1] & 0x1f) << 5  |
                                  (color[2] & 0x1f) << 10 |
                                  (color[3] & 0x1)  << 15;
        break;
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8_USCALED:
    case VK_FORMAT_R8_SSCALED:
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8_SRGB:
        ((uint8_t *) value)[0]  = (uint8_t) color[0];
        break;
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8_USCALED:
    case VK_FORMAT_R8G8_SSCALED:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8_SRGB:
        ((uint8_t *) value)[0]  = (uint8_t) color[0];
        ((uint8_t *) value)[1]  = (uint8_t) color[1];
        break;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R8G8B8A8_USCALED:
    case VK_FORMAT_R8G8B8A8_SSCALED:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R8G8B8A8_SRGB:
        ((uint8_t *) value)[0]  = (uint8_t) color[0];
        ((uint8_t *) value)[1]  = (uint8_t) color[1];
        ((uint8_t *) value)[2]  = (uint8_t) color[2];
        ((uint8_t *) value)[3]  = (uint8_t) color[3];
        break;
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SRGB:
        ((uint8_t *) value)[0]  = (uint8_t) color[2];
        ((uint8_t *) value)[1]  = (uint8_t) color[1];
        ((uint8_t *) value)[2]  = (uint8_t) color[0];
        ((uint8_t *) value)[3]  = (uint8_t) color[3];
        break;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        ((uint32_t *) value)[0] = (color[0] & 0x7ff) << 0  |
                                  (color[1] & 0x7ff) << 11 |
                                  (color[2] & 0x3ff) << 22;
        break;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        ((uint32_t *) value)[0] = (color[0] & 0x3ff) << 0  |
                                  (color[1] & 0x3ff) << 10 |
                                  (color[2] & 0x3ff) << 20 |
                                  (color[3] & 0x3)   << 30;
        break;
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_USCALED:
    case VK_FORMAT_R16_SSCALED:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16_SFLOAT:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        break;
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16_USCALED:
    case VK_FORMAT_R16G16_SSCALED:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16_SFLOAT:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        ((uint16_t *) value)[1] = (uint16_t) color[1];
        break;
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_R16G16B16A16_USCALED:
    case VK_FORMAT_R16G16B16A16_SSCALED:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        ((uint16_t *) value)[1] = (uint16_t) color[1];
        ((uint16_t *) value)[2] = (uint16_t) color[2];
        ((uint16_t *) value)[3] = (uint16_t) color[3];
        break;
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32_SFLOAT:
        ((uint32_t *) value)[0] = color[0];
        break;
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32_SFLOAT:
        ((uint32_t *) value)[0] = color[0];
        ((uint32_t *) value)[1] = color[1];
        break;
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32_SFLOAT:
        ((uint32_t *) value)[0] = color[0];
        ((uint32_t *) value)[1] = color[1];
        ((uint32_t *) value)[2] = color[2];
        break;
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        ((uint32_t *) value)[0] = color[0];
        ((uint32_t *) value)[1] = color[1];
        ((uint32_t *) value)[2] = color[2];
        ((uint32_t *) value)[3] = color[3];
        break;
    case VK_FORMAT_D16_UNORM_S8_UINT:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        ((char *) value)[2] = (uint8_t) color[1];
        break;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        ((uint32_t *) value)[0] = (uint32_t) color[0];
        ((char *) value)[4] = (uint8_t) color[1];
        break;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        ((uint32_t *) value)[0] = (color[0] & 0x1ff) << 0  |
                                  (color[1] & 0x1ff) << 9  |
                                  (color[2] & 0x1ff) << 18 |
                                  (color[3] & 0x1f)  << 27;
        break;
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
        memcpy(value, color, 8);
        break;
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC2_SRGB_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC3_SRGB_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
    case VK_FORMAT_BC7_SRGB_BLOCK:
        memcpy(value, color, 16);
        break;
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8_USCALED:
    case VK_FORMAT_R8G8B8_SSCALED:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8_SRGB:
        ((uint8_t *) value)[0]  = (uint8_t) color[0];
        ((uint8_t *) value)[1]  = (uint8_t) color[1];
        ((uint8_t *) value)[2]  = (uint8_t) color[2];
        break;
    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16_USCALED:
    case VK_FORMAT_R16G16B16_SSCALED:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16_SFLOAT:
        ((uint16_t *) value)[0] = (uint16_t) color[0];
        ((uint16_t *) value)[1] = (uint16_t) color[1];
        ((uint16_t *) value)[2] = (uint16_t) color[2];
        break;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        ((uint32_t *) value)[0] = (color[2] & 0x3ff) << 0  |
                                  (color[1] & 0x3ff) << 10 |
                                  (color[0] & 0x3ff) << 20 |
                                  (color[3] & 0x3)   << 30;
        break;
    case VK_FORMAT_R64_SFLOAT:
        /* higher 32 bits always 0 */
        ((uint64_t *) value)[0] = color[0];
        break;
    case VK_FORMAT_R64G64_SFLOAT:
        ((uint64_t *) value)[0] = color[0];
        ((uint64_t *) value)[1] = color[1];
        break;
    case VK_FORMAT_R64G64B64_SFLOAT:
        ((uint64_t *) value)[0] = color[0];
        ((uint64_t *) value)[1] = color[1];
        ((uint64_t *) value)[2] = color[2];
        break;
    case VK_FORMAT_R64G64B64A64_SFLOAT:
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
