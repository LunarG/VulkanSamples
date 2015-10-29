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
 */

#include <string.h>
#include "vulkan.h"
#include "vk_layer_utils.h"

typedef struct _VULKAN_FORMAT_INFO {
    size_t size;
    uint32_t channel_count;
} VULKAN_FORMAT_INFO;


// Set up data structure with number of bytes and number of channels
// for each Vulkan format.
static const VULKAN_FORMAT_INFO vk_format_table[VK_FORMAT_RANGE_SIZE] = {
    { 0,  0 }, //    [VK_FORMAT_UNDEFINED]
    { 1,  2 }, //    [VK_FORMAT_R4G4_UNORM]
    { 1,  2 }, //    [VK_FORMAT_R4G4_USCALED]
    { 2,  4 }, //    [VK_FORMAT_R4G4B4A4_UNORM]
    { 2,  4 }, //    [VK_FORMAT_R4G4B4A4_USCALED]
    { 2,  3 }, //    [VK_FORMAT_R5G6B5_UNORM]
    { 2,  3 }, //    [VK_FORMAT_R5G6B5_USCALED]
    { 2,  4 }, //    [VK_FORMAT_R5G5B5A1_UNORM]
    { 2,  4 }, //    [VK_FORMAT_R5G5B5A1_USCALED]
    { 1,  1 }, //    [VK_FORMAT_R8_UNORM]
    { 1,  1 }, //    [VK_FORMAT_R8_SNORM]
    { 1,  1 }, //    [VK_FORMAT_R8_USCALED]
    { 1,  1 }, //    [VK_FORMAT_R8_SSCALED]
    { 1,  1 }, //    [VK_FORMAT_R8_UINT]
    { 1,  1 }, //    [VK_FORMAT_R8_SINT]
    { 1,  1 }, //    [VK_FORMAT_R8_SRGB]
    { 2,  2 }, //    [VK_FORMAT_R8G8_UNORM]
    { 2,  2 }, //    [VK_FORMAT_R8G8_SNORM]
    { 2,  2 }, //    [VK_FORMAT_R8G8_USCALED]
    { 2,  2 }, //    [VK_FORMAT_R8G8_SSCALED]
    { 2,  2 }, //    [VK_FORMAT_R8G8_UINT]
    { 2,  2 }, //    [VK_FORMAT_R8G8_SINT]
    { 2,  2 }, //    [VK_FORMAT_R8G8_SRGB]
    { 3,  3 }, //    [VK_FORMAT_R8G8B8_UNORM]
    { 3,  3 }, //    [VK_FORMAT_R8G8B8_SNORM]
    { 3,  3 }, //    [VK_FORMAT_R8G8B8_USCALED]
    { 3,  3 }, //    [VK_FORMAT_R8G8B8_SSCALED]
    { 3,  3 }, //    [VK_FORMAT_R8G8B8_UINT]
    { 3,  3 }, //    [VK_FORMAT_R8G8B8_SINT]
    { 3,  3 }, //    [VK_FORMAT_R8G8B8_SRGB]
    { 4,  4 }, //    [VK_FORMAT_R8G8B8A8_UNORM]
    { 4,  4 }, //    [VK_FORMAT_R8G8B8A8_SNORM]
    { 4,  4 }, //    [VK_FORMAT_R8G8B8A8_USCALED]
    { 4,  4 }, //    [VK_FORMAT_R8G8B8A8_SSCALED]
    { 4,  4 }, //    [VK_FORMAT_R8G8B8A8_UINT]
    { 4,  4 }, //    [VK_FORMAT_R8G8B8A8_SINT]
    { 4,  4 }, //    [VK_FORMAT_R8G8B8A8_SRGB]
    { 4,  4 }, //    [VK_FORMAT_R10G10B10A2_UNORM]
    { 4,  4 }, //    [VK_FORMAT_R10G10B10A2_SNORM]
    { 4,  4 }, //    [VK_FORMAT_R10G10B10A2_USCALED]
    { 4,  4 }, //    [VK_FORMAT_R10G10B10A2_SSCALED]
    { 4,  4 }, //    [VK_FORMAT_R10G10B10A2_UINT]
    { 4,  4 }, //    [VK_FORMAT_R10G10B10A2_SINT]
    { 2,  1 }, //    [VK_FORMAT_R16_UNORM]
    { 2,  1 }, //    [VK_FORMAT_R16_SNORM]
    { 2,  1 }, //    [VK_FORMAT_R16_USCALED]
    { 2,  1 }, //    [VK_FORMAT_R16_SSCALED]
    { 2,  1 }, //    [VK_FORMAT_R16_UINT]
    { 2,  1 }, //    [VK_FORMAT_R16_SINT]
    { 2,  1 }, //    [VK_FORMAT_R16_SFLOAT]
    { 4,  2 }, //    [VK_FORMAT_R16G16_UNORM]
    { 4,  2 }, //    [VK_FORMAT_R16G16_SNORM]
    { 4,  2 }, //    [VK_FORMAT_R16G16_USCALED]
    { 4,  2 }, //    [VK_FORMAT_R16G16_SSCALED]
    { 4,  2 }, //    [VK_FORMAT_R16G16_UINT]
    { 4,  2 }, //    [VK_FORMAT_R16G16_SINT]
    { 4,  2 }, //    [VK_FORMAT_R16G16_SFLOAT]
    { 6,  3 }, //    [VK_FORMAT_R16G16B16_UNORM]
    { 6,  3 }, //    [VK_FORMAT_R16G16B16_SNORM]
    { 6,  3 }, //    [VK_FORMAT_R16G16B16_USCALED]
    { 6,  3 }, //    [VK_FORMAT_R16G16B16_SSCALED]
    { 6,  3 }, //    [VK_FORMAT_R16G16B16_UINT]
    { 6,  3 }, //    [VK_FORMAT_R16G16B16_SINT]
    { 6,  3 }, //    [VK_FORMAT_R16G16B16_SFLOAT]
    { 8,  4 }, //    [VK_FORMAT_R16G16B16A16_UNORM]
    { 8,  4 }, //    [VK_FORMAT_R16G16B16A16_SNORM]
    { 8,  4 }, //    [VK_FORMAT_R16G16B16A16_USCALED]
    { 8,  4 }, //    [VK_FORMAT_R16G16B16A16_SSCALED]
    { 8,  4 }, //    [VK_FORMAT_R16G16B16A16_UINT]
    { 8,  4 }, //    [VK_FORMAT_R16G16B16A16_SINT]
    { 8,  4 }, //    [VK_FORMAT_R16G16B16A16_SFLOAT]
    { 4,  1 }, //    [VK_FORMAT_R32_UINT]
    { 4,  1 }, //    [VK_FORMAT_R32_SINT]
    { 4,  1 }, //    [VK_FORMAT_R32_SFLOAT]
    { 8,  2 }, //    [VK_FORMAT_R32G32_UINT]
    { 8,  2 }, //    [VK_FORMAT_R32G32_SINT]
    { 8,  2 }, //    [VK_FORMAT_R32G32_SFLOAT]
    { 12, 3 }, //    [VK_FORMAT_R32G32B32_UINT]
    { 12, 3 }, //    [VK_FORMAT_R32G32B32_SINT]
    { 12, 3 }, //    [VK_FORMAT_R32G32B32_SFLOAT]
    { 16, 4 }, //    [VK_FORMAT_R32G32B32A32_UINT]
    { 16, 4 }, //    [VK_FORMAT_R32G32B32A32_SINT]
    { 16, 4 }, //    [VK_FORMAT_R32G32B32A32_SFLOAT]
    { 8,  1 }, //    [VK_FORMAT_R64_SFLOAT]
    { 16, 2 }, //    [VK_FORMAT_R64G64_SFLOAT]
    { 24, 3 }, //    [VK_FORMAT_R64G64B64_SFLOAT]
    { 32, 4 }, //    [VK_FORMAT_R64G64B64A64_SFLOAT]
    { 4,  3 }, //    [VK_FORMAT_R11G11B10_UFLOAT]
    { 4,  3 }, //    [VK_FORMAT_R9G9B9E5_UFLOAT]
    { 2,  1 }, //    [VK_FORMAT_D16_UNORM]
    { 3,  1 }, //    [VK_FORMAT_D24_UNORM_X8]
    { 4,  1 }, //    [VK_FORMAT_D32_SFLOAT]
    { 1,  1 }, //    [VK_FORMAT_S8_UINT]
    { 3,  2 }, //    [VK_FORMAT_D16_UNORM_S8_UINT]
    { 4,  2 }, //    [VK_FORMAT_D24_UNORM_S8_UINT]
    { 4,  2 }, //    [VK_FORMAT_D32_SFLOAT_S8_UINT]
    { 8,  4 }, //    [VK_FORMAT_BC1_RGB_UNORM]
    { 8,  4 }, //    [VK_FORMAT_BC1_RGB_SRGB]
    { 8,  4 }, //    [VK_FORMAT_BC1_RGBA_UNORM]
    { 8,  4 }, //    [VK_FORMAT_BC1_RGBA_SRGB]
    { 16, 4 }, //    [VK_FORMAT_BC2_UNORM]
    { 16, 4 }, //    [VK_FORMAT_BC2_SRGB]
    { 16, 4 }, //    [VK_FORMAT_BC3_UNORM]
    { 16, 4 }, //    [VK_FORMAT_BC3_SRGB]
    { 8,  4 }, //    [VK_FORMAT_BC4_UNORM]
    { 8,  4 }, //    [VK_FORMAT_BC4_SNORM]
    { 16, 4 }, //    [VK_FORMAT_BC5_UNORM]
    { 16, 4 }, //    [VK_FORMAT_BC5_SNORM]
    { 16, 4 }, //    [VK_FORMAT_BC6H_UFLOAT]
    { 16, 4 }, //    [VK_FORMAT_BC6H_SFLOAT]
    { 16, 4 }, //    [VK_FORMAT_BC7_UNORM]
    { 16, 4 }, //    [VK_FORMAT_BC7_SRGB]
    { 8, 3 },  //    [VK_FORMAT_ETC2_R8G8B8_UNORM]
    { 8, 3 },  //    [VK_FORMAT_ETC2_R8G8B8_SRGB]
    { 8, 4 },  //    [VK_FORMAT_ETC2_R8G8B8A1_UNORM]
    { 8, 4 },  //    [VK_FORMAT_ETC2_R8G8B8A1_SRGB]
    { 8, 4 },  //    [VK_FORMAT_ETC2_R8G8B8A8_UNORM]
    { 8, 4 },  //    [VK_FORMAT_ETC2_R8G8B8A8_SRGB]
    { 8, 1 },  //    [VK_FORMAT_EAC_R11_UNORM]
    { 8, 1 },  //    [VK_FORMAT_EAC_R11_SNORM]
    { 16, 2 }, //    [VK_FORMAT_EAC_R11G11_UNORM]
    { 16, 2 }, //    [VK_FORMAT_EAC_R11G11_SNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_4x4_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_4x4_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_5x4_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_5x4_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_5x5_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_5x5_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_6x5_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_6x5_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_6x6_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_6x6_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_8x5_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_8x5_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_8x6_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_8x6_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_8x8_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_8x8_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_10x5_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_10x5_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_10x6_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_10x6_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_10x8_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_10x8_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_10x10_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_10x10_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_12x10_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_12x10_SRGB]
    { 16, 4 }, //    [VK_FORMAT_ASTC_12x12_UNORM]
    { 16, 4 }, //    [VK_FORMAT_ASTC_12x12_SRGB]
    { 2, 3 },  //    [VK_FORMAT_B5G6R5_UNORM]
    { 2, 3 },  //    [VK_FORMAT_B5G6R5_USCALED]
    { 3, 3 },  //    [VK_FORMAT_B8G8R8_UNORM]
    { 3, 3 },  //    [VK_FORMAT_B8G8R8_SNORM]
    { 3, 3 },  //    [VK_FORMAT_B8G8R8_USCALED]
    { 3, 3 },  //    [VK_FORMAT_B8G8R8_SSCALED]
    { 3, 3 },  //    [VK_FORMAT_B8G8R8_UINT]
    { 3, 3 },  //    [VK_FORMAT_B8G8R8_SINT]
    { 3, 3 },  //    [VK_FORMAT_B8G8R8_SRGB]
    { 4, 4 },  //    [VK_FORMAT_B8G8R8A8_UNORM]
    { 4, 4 },  //    [VK_FORMAT_B8G8R8A8_SNORM]
    { 4, 4 },  //    [VK_FORMAT_B8G8R8A8_USCALED]
    { 4, 4 },  //    [VK_FORMAT_B8G8R8A8_SSCALED]
    { 4, 4 },  //    [VK_FORMAT_B8G8R8A8_UINT]
    { 4, 4 },  //    [VK_FORMAT_B8G8R8A8_SINT]
    { 4, 4 },  //    [VK_FORMAT_B8G8R8A8_SRGB]
    { 4, 4 },  //    [VK_FORMAT_B10G10R10A2_UNORM]
    { 4, 4 },  //    [VK_FORMAT_B10G10R10A2_SNORM]
    { 4, 4 },  //    [VK_FORMAT_B10G10R10A2_USCALED]
    { 4, 4 },  //    [VK_FORMAT_B10G10R10A2_SSCALED]
    { 4, 4 },  //    [VK_FORMAT_B10G10R10A2_UINT]
    { 4, 4 },  //    [VK_FORMAT_B10G10R10A2_SINT]
};

// Return true if format is a depth or stencil format
bool vk_format_is_depth_or_stencil(VkFormat format)
{
    return (vk_format_is_depth_and_stencil(format) ||
            vk_format_is_depth_only(format)        ||
            vk_format_is_stencil_only(format));
}

// Return true if format contains depth and stencil information
bool vk_format_is_depth_and_stencil(VkFormat format)
{
    bool is_ds = false;

    switch (format) {
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

// Return true if format is a stencil-only format
bool vk_format_is_stencil_only(VkFormat format)
{
    return (format == VK_FORMAT_S8_UINT);
}

// Return true if format is a depth-only format
bool vk_format_is_depth_only(VkFormat format)
{
    bool is_depth = false;

    switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D24_UNORM_X8:
    case VK_FORMAT_D32_SFLOAT:
        is_depth = true;
        break;
    default:
        break;
    }

    return is_depth;
}

// Return true if format is of time UNORM
bool vk_format_is_norm(VkFormat format)
{
    bool is_norm = false;

    switch (format) {
    case VK_FORMAT_R4G4_UNORM:
    case VK_FORMAT_R4G4B4A4_UNORM:
    case VK_FORMAT_R5G6B5_UNORM:
    case VK_FORMAT_R5G5B5A1_UNORM:
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R10G10B10A2_UNORM:
    case VK_FORMAT_R10G10B10A2_SNORM:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SNORM:
    case VK_FORMAT_R16G16B16_UNORM:
    case VK_FORMAT_R16G16B16_SNORM:
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SNORM:
    case VK_FORMAT_BC1_RGB_UNORM:
    case VK_FORMAT_BC2_UNORM:
    case VK_FORMAT_BC3_UNORM:
    case VK_FORMAT_BC4_UNORM:
    case VK_FORMAT_BC4_SNORM:
    case VK_FORMAT_BC5_UNORM:
    case VK_FORMAT_BC5_SNORM:
    case VK_FORMAT_BC7_UNORM:
    case VK_FORMAT_ETC2_R8G8B8_UNORM:
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM:
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM:
    case VK_FORMAT_EAC_R11_UNORM:
    case VK_FORMAT_EAC_R11_SNORM:
    case VK_FORMAT_EAC_R11G11_UNORM:
    case VK_FORMAT_EAC_R11G11_SNORM:
    case VK_FORMAT_ASTC_4x4_UNORM:
    case VK_FORMAT_ASTC_5x4_UNORM:
    case VK_FORMAT_ASTC_5x5_UNORM:
    case VK_FORMAT_ASTC_6x5_UNORM:
    case VK_FORMAT_ASTC_6x6_UNORM:
    case VK_FORMAT_ASTC_8x5_UNORM:
    case VK_FORMAT_ASTC_8x6_UNORM:
    case VK_FORMAT_ASTC_8x8_UNORM:
    case VK_FORMAT_ASTC_10x5_UNORM:
    case VK_FORMAT_ASTC_10x6_UNORM:
    case VK_FORMAT_ASTC_10x8_UNORM:
    case VK_FORMAT_ASTC_10x10_UNORM:
    case VK_FORMAT_ASTC_12x10_UNORM:
    case VK_FORMAT_ASTC_12x12_UNORM:
    case VK_FORMAT_B5G6R5_UNORM:
    case VK_FORMAT_B8G8R8_UNORM:
    case VK_FORMAT_B8G8R8_SNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_B10G10R10A2_UNORM:
    case VK_FORMAT_B10G10R10A2_SNORM:
        is_norm = true;
        break;
    default:
        break;
    }

    return is_norm;
};


// Return true if format is an integer format
bool vk_format_is_int(VkFormat format)
{
    return (vk_format_is_sint(format) || vk_format_is_uint(format));
}

// Return true if format is an unsigned integer format
bool vk_format_is_uint(VkFormat format)
{
    bool is_uint = false;

    switch (format) {
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R10G10B10A2_UINT:
    case VK_FORMAT_R16_UINT:
    case VK_FORMAT_R16G16_UINT:
    case VK_FORMAT_R16G16B16_UINT:
    case VK_FORMAT_R16G16B16A16_UINT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_B8G8R8_UINT:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_B10G10R10A2_UINT:
        is_uint = true;
        break;
    default:
        break;
    }

    return is_uint;
}

// Return true if format is a signed integer format
bool vk_format_is_sint(VkFormat format)
{
    bool is_sint = false;

    switch (format) {
    case VK_FORMAT_R8_SINT:
    case VK_FORMAT_R8G8_SINT:
    case VK_FORMAT_R8G8B8_SINT:
    case VK_FORMAT_R8G8B8A8_SINT:
    case VK_FORMAT_R10G10B10A2_SINT:
    case VK_FORMAT_R16_SINT:
    case VK_FORMAT_R16G16_SINT:
    case VK_FORMAT_R16G16B16_SINT:
    case VK_FORMAT_R16G16B16A16_SINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_B8G8R8_SINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_B10G10R10A2_SINT:
        is_sint = true;
        break;
    default:
        break;
    }

    return is_sint;
}

// Return true if format is a floating-point format
bool vk_format_is_float(VkFormat format)
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
    case VK_FORMAT_R11G11B10_UFLOAT:
    case VK_FORMAT_R9G9B9E5_UFLOAT:
    case VK_FORMAT_BC6H_UFLOAT:
    case VK_FORMAT_BC6H_SFLOAT:
        is_float = true;
        break;
    default:
        break;
    }

    return is_float;
}

// Return true if format is in the SRGB colorspace
bool vk_format_is_srgb(VkFormat format)
{
    bool is_srgb = false;

    switch (format) {
    case VK_FORMAT_R8_SRGB:
    case VK_FORMAT_R8G8_SRGB:
    case VK_FORMAT_R8G8B8_SRGB:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_BC1_RGB_SRGB:
    case VK_FORMAT_BC2_SRGB:
    case VK_FORMAT_BC3_SRGB:
    case VK_FORMAT_BC7_SRGB:
    case VK_FORMAT_ETC2_R8G8B8_SRGB:
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB:
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB:
    case VK_FORMAT_ASTC_4x4_SRGB:
    case VK_FORMAT_ASTC_5x4_SRGB:
    case VK_FORMAT_ASTC_5x5_SRGB:
    case VK_FORMAT_ASTC_6x5_SRGB:
    case VK_FORMAT_ASTC_6x6_SRGB:
    case VK_FORMAT_ASTC_8x5_SRGB:
    case VK_FORMAT_ASTC_8x6_SRGB:
    case VK_FORMAT_ASTC_8x8_SRGB:
    case VK_FORMAT_ASTC_10x5_SRGB:
    case VK_FORMAT_ASTC_10x6_SRGB:
    case VK_FORMAT_ASTC_10x8_SRGB:
    case VK_FORMAT_ASTC_10x10_SRGB:
    case VK_FORMAT_ASTC_12x10_SRGB:
    case VK_FORMAT_ASTC_12x12_SRGB:
    case VK_FORMAT_B8G8R8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
        is_srgb = true;
        break;
    default:
        break;
    }

    return is_srgb;
}

// Return true if format is compressed
bool vk_format_is_compressed(VkFormat format)
{
    switch (format) {
    case VK_FORMAT_BC1_RGB_UNORM:
    case VK_FORMAT_BC1_RGB_SRGB:
    case VK_FORMAT_BC2_UNORM:
    case VK_FORMAT_BC2_SRGB:
    case VK_FORMAT_BC3_UNORM:
    case VK_FORMAT_BC3_SRGB:
    case VK_FORMAT_BC4_UNORM:
    case VK_FORMAT_BC4_SNORM:
    case VK_FORMAT_BC5_UNORM:
    case VK_FORMAT_BC5_SNORM:
    case VK_FORMAT_BC6H_UFLOAT:
    case VK_FORMAT_BC6H_SFLOAT:
    case VK_FORMAT_BC7_UNORM:
    case VK_FORMAT_BC7_SRGB:
    case VK_FORMAT_ETC2_R8G8B8_UNORM:
    case VK_FORMAT_ETC2_R8G8B8_SRGB:
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM:
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB:
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM:
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB:
    case VK_FORMAT_EAC_R11_UNORM:
    case VK_FORMAT_EAC_R11_SNORM:
    case VK_FORMAT_EAC_R11G11_UNORM:
    case VK_FORMAT_EAC_R11G11_SNORM:
    case VK_FORMAT_ASTC_4x4_UNORM:
    case VK_FORMAT_ASTC_4x4_SRGB:
    case VK_FORMAT_ASTC_5x4_UNORM:
    case VK_FORMAT_ASTC_5x4_SRGB:
    case VK_FORMAT_ASTC_5x5_UNORM:
    case VK_FORMAT_ASTC_5x5_SRGB:
    case VK_FORMAT_ASTC_6x5_UNORM:
    case VK_FORMAT_ASTC_6x5_SRGB:
    case VK_FORMAT_ASTC_6x6_UNORM:
    case VK_FORMAT_ASTC_6x6_SRGB:
    case VK_FORMAT_ASTC_8x5_UNORM:
    case VK_FORMAT_ASTC_8x5_SRGB:
    case VK_FORMAT_ASTC_8x6_UNORM:
    case VK_FORMAT_ASTC_8x6_SRGB:
    case VK_FORMAT_ASTC_8x8_UNORM:
    case VK_FORMAT_ASTC_8x8_SRGB:
    case VK_FORMAT_ASTC_10x5_UNORM:
    case VK_FORMAT_ASTC_10x5_SRGB:
    case VK_FORMAT_ASTC_10x6_UNORM:
    case VK_FORMAT_ASTC_10x6_SRGB:
    case VK_FORMAT_ASTC_10x8_UNORM:
    case VK_FORMAT_ASTC_10x8_SRGB:
    case VK_FORMAT_ASTC_10x10_UNORM:
    case VK_FORMAT_ASTC_10x10_SRGB:
    case VK_FORMAT_ASTC_12x10_UNORM:
    case VK_FORMAT_ASTC_12x10_SRGB:
    case VK_FORMAT_ASTC_12x12_UNORM:
    case VK_FORMAT_ASTC_12x12_SRGB:
        return true;
    default:
        return false;
    }
}

// Return size, in bytes, of a pixel of the specified format
size_t vk_format_get_size(VkFormat format)
{
    return vk_format_table[format].size;
}

// Return the number of channels for a given format
unsigned int vk_format_get_channel_count(VkFormat format)
{
    return vk_format_table[format].channel_count;
}
