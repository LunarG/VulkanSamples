/*
 * Vulkan
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
 */
#pragma once

#include <unordered_map>

extern std::unordered_map<void *, VkLayerDispatchTable *> tableMap;
extern std::unordered_map<void *, VkLayerInstanceDispatchTable *> tableInstanceMap;
VkLayerDispatchTable * initDeviceTable(const VkBaseLayerObject *devw);
VkLayerInstanceDispatchTable * initInstanceTable(const VkBaseLayerObject *instancew);

// Map lookup must be thread safe
static inline VkLayerDispatchTable *device_dispatch_table(VkObject object)
{
    VkLayerDispatchTable *pDisp  = *(VkLayerDispatchTable **) object;
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap.find((void *) pDisp);
    assert(it != tableMap.end() && "Not able to find device dispatch entry");
    return it->second;
}

static inline VkLayerInstanceDispatchTable *instance_dispatch_table(VkObject object)
{
    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) object;
    std::unordered_map<void *, VkLayerInstanceDispatchTable *>::const_iterator it = tableInstanceMap.find((void *) pDisp);
    assert(it != tableInstanceMap.end() && "Not able to find instance dispatch entry");
    return it->second;
}
