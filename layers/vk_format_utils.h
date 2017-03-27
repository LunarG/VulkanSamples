/* Copyright (c) 2015-2017 The Khronos Group Inc.
 * Copyright (c) 2015-2017 Valve Corporation
 * Copyright (c) 2015-2017 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Dave Houlton <daveh@lunarg.com>
 */

#pragma once
#include <stdbool.h>
#include <vector>
#include "vulkan/vulkan.h"

#if !defined(VK_LAYER_EXPORT)
#if defined(__GNUC__) && __GNUC__ >= 4
#define VK_LAYER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#define VK_LAYER_EXPORT __attribute__((visibility("default")))
#else
#define VK_LAYER_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum VkFormatCompatibilityClass {
    VK_FORMAT_COMPATIBILITY_CLASS_NONE_BIT = 0,
    VK_FORMAT_COMPATIBILITY_CLASS_8_BIT = 1,
    VK_FORMAT_COMPATIBILITY_CLASS_16_BIT = 2,
    VK_FORMAT_COMPATIBILITY_CLASS_24_BIT = 3,
    VK_FORMAT_COMPATIBILITY_CLASS_32_BIT = 4,
    VK_FORMAT_COMPATIBILITY_CLASS_48_BIT = 5,
    VK_FORMAT_COMPATIBILITY_CLASS_64_BIT = 6,
    VK_FORMAT_COMPATIBILITY_CLASS_96_BIT = 7,
    VK_FORMAT_COMPATIBILITY_CLASS_128_BIT = 8,
    VK_FORMAT_COMPATIBILITY_CLASS_192_BIT = 9,
    VK_FORMAT_COMPATIBILITY_CLASS_256_BIT = 10,
    VK_FORMAT_COMPATIBILITY_CLASS_BC1_RGB_BIT = 11,
    VK_FORMAT_COMPATIBILITY_CLASS_BC1_RGBA_BIT = 12,
    VK_FORMAT_COMPATIBILITY_CLASS_BC2_BIT = 13,
    VK_FORMAT_COMPATIBILITY_CLASS_BC3_BIT = 14,
    VK_FORMAT_COMPATIBILITY_CLASS_BC4_BIT = 15,
    VK_FORMAT_COMPATIBILITY_CLASS_BC5_BIT = 16,
    VK_FORMAT_COMPATIBILITY_CLASS_BC6H_BIT = 17,
    VK_FORMAT_COMPATIBILITY_CLASS_BC7_BIT = 18,
    VK_FORMAT_COMPATIBILITY_CLASS_ETC2_RGB_BIT = 19,
    VK_FORMAT_COMPATIBILITY_CLASS_ETC2_RGBA_BIT = 20,
    VK_FORMAT_COMPATIBILITY_CLASS_ETC2_EAC_RGBA_BIT = 21,
    VK_FORMAT_COMPATIBILITY_CLASS_EAC_R_BIT = 22,
    VK_FORMAT_COMPATIBILITY_CLASS_EAC_RG_BIT = 23,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_4X4_BIT = 24,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_5X4_BIT = 25,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_5X5_BIT = 26,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_6X5_BIT = 27,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_6X6_BIT = 28,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X5_BIT = 29,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X6_BIT = 20,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_8X8_BIT = 31,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X5_BIT = 32,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X6_BIT = 33,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X8_BIT = 34,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_10X10_BIT = 35,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X10_BIT = 36,
    VK_FORMAT_COMPATIBILITY_CLASS_ASTC_12X12_BIT = 37,
    VK_FORMAT_COMPATIBILITY_CLASS_D16_BIT = 38,
    VK_FORMAT_COMPATIBILITY_CLASS_D24_BIT = 39,
    VK_FORMAT_COMPATIBILITY_CLASS_D32_BIT = 30,
    VK_FORMAT_COMPATIBILITY_CLASS_S8_BIT = 41,
    VK_FORMAT_COMPATIBILITY_CLASS_D16S8_BIT = 42,
    VK_FORMAT_COMPATIBILITY_CLASS_D24S8_BIT = 43,
    VK_FORMAT_COMPATIBILITY_CLASS_D32S8_BIT = 44,
    VK_FORMAT_COMPATIBILITY_CLASS_MAX_ENUM = 45
} VkFormatCompatibilityClass;

static inline bool VkFormatIsUndef(VkFormat format) { return (format == VK_FORMAT_UNDEFINED); }

VK_LAYER_EXPORT bool VkFormatIsDepthOrStencil(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsDepthAndStencil(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsDepthOnly(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsStencilOnly(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsCompressed_ETC2_EAC(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsCompressed_ASTC_LDR(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsCompressed_BC(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsNorm(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsUNorm(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsSNorm(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsInt(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsSInt(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsUInt(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsFloat(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsSRGB(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsUScaled(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsSScaled(VkFormat format);
VK_LAYER_EXPORT bool VkFormatIsCompressed(VkFormat format);

static inline bool VkFormatIsColor(VkFormat format) {
    return !(VkFormatIsUndef(format) || VkFormatIsDepthOrStencil(format));
}

static inline bool VkFormatHasDepth(VkFormat format) {
    return (VkFormatIsDepthOnly(format) || VkFormatIsDepthAndStencil(format));
}

static inline bool VkFormatHasStencil(VkFormat format) {
    return (VkFormatIsStencilOnly(format) || VkFormatIsDepthAndStencil(format));
}

VK_LAYER_EXPORT VkExtent3D VkFormatCompressedTexelBlockExtent(VkFormat format);
VK_LAYER_EXPORT size_t VkFormatGetSize(VkFormat format);
VK_LAYER_EXPORT unsigned int VkFormatGetChannelCount(VkFormat format);
VK_LAYER_EXPORT VkFormatCompatibilityClass VkFormatGetCompatibilityClass(VkFormat format);

VK_LAYER_EXPORT VkDeviceSize VkSafeModulo(VkDeviceSize dividend, VkDeviceSize divisor);

#ifdef __cplusplus
}
#endif
