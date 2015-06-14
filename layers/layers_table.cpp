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
#include <assert.h>
#include <unordered_map>
#include "vk_dispatch_table_helper.h"
#include "vkLayer.h"
#include "layers_table.h"
device_table_map tableMap;
instance_table_map tableInstanceMap;


/* Various dispatchable objects will use the same underlying dispatch table if they
 * are created from that "parent" object. Thus use pointer to dispatch table
 * as the key to these table maps.
 *    Instance -> PhysicalDevice
 *    Device -> CmdBuffer or Queue
 * If use the object themselves as key to map then implies Create entrypoints have to be intercepted
 * and a new key inserted into map */
VkLayerInstanceDispatchTable * initInstanceTable(instance_table_map &map, const VkBaseLayerObject *instancew)
{
    VkLayerInstanceDispatchTable *pTable;
    assert(instancew);
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instancew->baseObject;

    std::unordered_map<void *, VkLayerInstanceDispatchTable *>::const_iterator it = map.find((void *) *ppDisp);
    if (it == tableInstanceMap.end())
    {
        pTable =  new VkLayerInstanceDispatchTable;
        map[(void *) *ppDisp] = pTable;
    } else
    {
        return it->second;
    }

    layer_init_instance_dispatch_table(pTable, instancew);

    return pTable;
}

VkLayerInstanceDispatchTable * initInstanceTable(const VkBaseLayerObject *instancew)
{
    return initInstanceTable(tableInstanceMap, instancew);
}

VkLayerDispatchTable * initDeviceTable(device_table_map &map, const VkBaseLayerObject *devw)
{
    VkLayerDispatchTable *layer_device_table = NULL;
    assert(devw);
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) (devw->baseObject);
    VkLayerDispatchTable *base_device_table = *ppDisp;

    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = map.find((void *) base_device_table);
    if (it == tableMap.end())
    {
        layer_device_table =  new VkLayerDispatchTable;
        map[(void *) base_device_table] = layer_device_table;
        fprintf(stderr, "initDeviceTable(%p): %p => %p\n", devw, base_device_table, layer_device_table);
    } else
    {
        return it->second;
    }

    layer_initialize_dispatch_table(layer_device_table, devw);

    return layer_device_table;
}

VkLayerDispatchTable * initDeviceTable(const VkBaseLayerObject *devw)
{
    return initDeviceTable(tableMap, devw);
}
