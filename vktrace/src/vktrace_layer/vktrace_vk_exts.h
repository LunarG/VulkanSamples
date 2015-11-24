/*
 *
 * Copyright (C) 2015 Valve Corporation
 * All Rights Reserved.
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
 * Author: Jon Ashburn <jon@lunarg.com>
 */
#pragma once


// FIXME/TODO: DEVELOP A BETTER APPROACH FOR SETTING THE DEFAULT VALUES FOR
// THESE PLATFORM-SPECIFIC MACROS APPROPRIATELY:
#ifdef _WIN32
// The Win32 default is to support the WIN32 platform:
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#else // _WIN32 (i.e. Linux)
// The Linux default is to support the XCB platform:
#if (!defined(VK_USE_PLATFORM_MIR_KHR) && \
     !defined(VK_USE_PLATFORM_WAYLAND_KHR) && \
     !defined(VK_USE_PLATFORM_XCB_KHR) && \
     !defined(VK_USE_PLATFORM_XLIB_KHR))
#define VK_USE_PLATFORM_XCB_KHR
#endif
#endif // _WIN32

#include "vulkan/vk_lunarg_debug_marker.h"
#include "vulkan/vk_lunarg_debug_report.h"

#include "vulkan/vk_layer.h"
#include "vktrace_lib_helpers.h"

void ext_init_create_instance(
        layer_instance_data             *instData,
        VkInstance                      inst,
        uint32_t                        extension_count,
        const char*const*               ppEnabledExtensions);

void ext_init_create_device(
        layer_device_data               *devData,
        VkDevice                        dev,
        uint32_t                        extension_count,
        const char*const*               ppEnabledExtensions);
