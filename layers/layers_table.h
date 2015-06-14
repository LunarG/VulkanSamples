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

typedef std::unordered_map<void *, VkLayerDispatchTable *> device_table_map;
typedef std::unordered_map<void *, VkLayerInstanceDispatchTable *> instance_table_map;
extern device_table_map tableMap;
extern instance_table_map tableInstanceMap;
VkLayerDispatchTable * initDeviceTable(const VkBaseLayerObject *devw);
VkLayerDispatchTable * initDeviceTable(device_table_map &map, const VkBaseLayerObject *devw);
VkLayerInstanceDispatchTable * initInstanceTable(const VkBaseLayerObject *instancew);
VkLayerInstanceDispatchTable * initInstanceTable(instance_table_map &map, const VkBaseLayerObject *instancew);

typedef void *dispatch_key;

static inline dispatch_key get_dispatch_key(VkObject object)
{
    return (dispatch_key) *(VkLayerDispatchTable **) object;
}

// Map lookup must be thread safe
static inline VkLayerDispatchTable *device_dispatch_table(VkObject object)
{
//    VkLayerDispatchTable *pDisp  = *(VkLayerDispatchTable **) object;
    dispatch_key key = get_dispatch_key(object);
    device_table_map::const_iterator it = tableMap.find((void *) key);
    assert(it != tableMap.end() && "Not able to find device dispatch entry");
    return it->second;
}

static inline VkLayerInstanceDispatchTable *instance_dispatch_table(VkObject object)
{
//    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) object;
    dispatch_key key = get_dispatch_key(object);
    instance_table_map::const_iterator it = tableInstanceMap.find((void *) key);
    assert(it != tableInstanceMap.end() && "Not able to find instance dispatch entry");
    return it->second;
}

static inline VkLayerDispatchTable *get_dispatch_table(device_table_map &map, VkObject object)
{
//    VkLayerDispatchTable *pDisp  = *(VkLayerDispatchTable **) object;
    dispatch_key key = get_dispatch_key(object);
    device_table_map::const_iterator it = map.find((void *) key);
    assert(it != map.end() && "Not able to find device dispatch entry");
    return it->second;
}

static inline VkLayerInstanceDispatchTable *get_dispatch_table(instance_table_map &map, VkObject object)
{
//    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) object;
    dispatch_key key = get_dispatch_key(object);
    instance_table_map::const_iterator it = map.find((void *) key);
    assert(it != map.end() && "Not able to find instance dispatch entry");
    return it->second;
}
