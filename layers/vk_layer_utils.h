/**************************************************************************
 *
 * Copyright 2015 Lunarg, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/
#pragma once
#include <stdbool.h>
#ifndef WIN32
#include <strings.h> /* for ffs() */
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline bool vk_format_is_undef(VkFormat format)
{
    return (format == VK_FORMAT_UNDEFINED);
}

bool vk_format_is_depth_or_stencil(VkFormat format);
bool vk_format_is_depth_and_stencil(VkFormat format);
bool vk_format_is_depth_only(VkFormat format);
bool vk_format_is_stencil_only(VkFormat format);

static inline bool vk_format_is_color(VkFormat format)
{
    return !(vk_format_is_undef(format) || vk_format_is_depth_or_stencil(format));
}

bool   vk_format_is_norm(VkFormat format);
bool   vk_format_is_int(VkFormat format);
bool   vk_format_is_sint(VkFormat format);
bool   vk_format_is_uint(VkFormat format);
bool   vk_format_is_float(VkFormat format);
bool   vk_format_is_srgb(VkFormat format);
bool   vk_format_is_compressed(VkFormat format);
size_t vk_format_get_size(VkFormat format);

static inline int u_ffs(int val)
{
#ifdef WIN32
        return __lzcnt(val) + 1;
#else
        return ffs(val);
#endif
}

#ifdef __cplusplus
}
#endif


