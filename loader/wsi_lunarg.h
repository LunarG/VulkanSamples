/*
 * Vulkan
 *
 * Copyright (C) 2015 LunarG, Inc.
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
 *   Jon Ashburn <jon@lunarg.com>
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#include "vk_wsi_lunarg.h"

VkResult loader_GetDisplayInfoWSI(
        VkDisplayWSI                            display,
        VkDisplayInfoTypeWSI                    infoType,
        size_t*                                 pDataSize,
        void*                                   pData);

void wsi_lunarg_register_extensions(
        const VkInstanceCreateInfo*             pCreateInfo);

void *wsi_lunarg_GetInstanceProcAddr(
        VkInstance                              instance,
        const char*                             pName,
        bool*                                   enabled);

void *wsi_lunarg_GetDeviceProcAddr(
        VkDevice                                device,
        const char*                             pName,
        bool*                                   enabled);
