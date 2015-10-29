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
 */
#pragma once

#include "vk_debug_marker_lunarg.h"
#include "vk_debug_report_lunarg.h"
#include "vk_ext_khr_swapchain.h"
#include "vk_ext_khr_device_swapchain.h"
#include "vk_layer.h"
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
