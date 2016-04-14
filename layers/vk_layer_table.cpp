/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (c) 2015-2016 Google, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and/or associated documentation files (the "Materials"), to
 * deal in the Materials without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Materials, and to permit persons to whom the Materials
 * are furnished to do so, subject to the following conditions:
 *
 * The above copyright notice(s) and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
 * USE OR OTHER DEALINGS IN THE MATERIALS
 *
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */
#include <assert.h>
#include <unordered_map>
#include "vk_dispatch_table_helper.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_table.h"
static device_table_map tableMap;
static instance_table_map tableInstanceMap;

#define DISPATCH_MAP_DEBUG 0

// Map lookup must be thread safe
VkLayerDispatchTable *device_dispatch_table(void *object) {
    dispatch_key key = get_dispatch_key(object);
    device_table_map::const_iterator it = tableMap.find((void *)key);
    assert(it != tableMap.end() && "Not able to find device dispatch entry");
    return it->second;
}

VkLayerInstanceDispatchTable *instance_dispatch_table(void *object) {
    dispatch_key key = get_dispatch_key(object);
    instance_table_map::const_iterator it = tableInstanceMap.find((void *)key);
#if DISPATCH_MAP_DEBUG
    if (it != tableInstanceMap.end()) {
        fprintf(stderr, "instance_dispatch_table: map: %p, object: %p, key: %p, table: %p\n", &tableInstanceMap, object, key,
                it->second);
    } else {
        fprintf(stderr, "instance_dispatch_table: map: %p, object: %p, key: %p, table: UNKNOWN\n", &tableInstanceMap, object, key);
    }
#endif
    assert(it != tableInstanceMap.end() && "Not able to find instance dispatch entry");
    return it->second;
}

void destroy_dispatch_table(device_table_map &map, dispatch_key key) {
#if DISPATCH_MAP_DEBUG
    device_table_map::const_iterator it = map.find((void *)key);
    if (it != map.end()) {
        fprintf(stderr, "destroy device dispatch_table: map: %p, key: %p, table: %p\n", &map, key, it->second);
    } else {
        fprintf(stderr, "destroy device dispatch table: map: %p, key: %p, table: UNKNOWN\n", &map, key);
        assert(it != map.end());
    }
#endif
    map.erase(key);
}

void destroy_dispatch_table(instance_table_map &map, dispatch_key key) {
#if DISPATCH_MAP_DEBUG
    instance_table_map::const_iterator it = map.find((void *)key);
    if (it != map.end()) {
        fprintf(stderr, "destroy instance dispatch_table: map: %p, key: %p, table: %p\n", &map, key, it->second);
    } else {
        fprintf(stderr, "destroy instance dispatch table: map: %p, key: %p, table: UNKNOWN\n", &map, key);
        assert(it != map.end());
    }
#endif
    map.erase(key);
}

void destroy_device_dispatch_table(dispatch_key key) { destroy_dispatch_table(tableMap, key); }

void destroy_instance_dispatch_table(dispatch_key key) { destroy_dispatch_table(tableInstanceMap, key); }

VkLayerDispatchTable *get_dispatch_table(device_table_map &map, void *object) {
    dispatch_key key = get_dispatch_key(object);
    device_table_map::const_iterator it = map.find((void *)key);
#if DISPATCH_MAP_DEBUG
    if (it != map.end()) {
        fprintf(stderr, "device_dispatch_table: map: %p, object: %p, key: %p, table: %p\n", &tableInstanceMap, object, key,
                it->second);
    } else {
        fprintf(stderr, "device_dispatch_table: map: %p, object: %p, key: %p, table: UNKNOWN\n", &tableInstanceMap, object, key);
    }
#endif
    assert(it != map.end() && "Not able to find device dispatch entry");
    return it->second;
}

VkLayerInstanceDispatchTable *get_dispatch_table(instance_table_map &map, void *object) {
    //    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) object;
    dispatch_key key = get_dispatch_key(object);
    instance_table_map::const_iterator it = map.find((void *)key);
#if DISPATCH_MAP_DEBUG
    if (it != map.end()) {
        fprintf(stderr, "instance_dispatch_table: map: %p, object: %p, key: %p, table: %p\n", &tableInstanceMap, object, key,
                it->second);
    } else {
        fprintf(stderr, "instance_dispatch_table: map: %p, object: %p, key: %p, table: UNKNOWN\n", &tableInstanceMap, object, key);
    }
#endif
    assert(it != map.end() && "Not able to find instance dispatch entry");
    return it->second;
}

VkLayerInstanceCreateInfo *get_chain_info(const VkInstanceCreateInfo *pCreateInfo, VkLayerFunction func) {
    VkLayerInstanceCreateInfo *chain_info = (VkLayerInstanceCreateInfo *)pCreateInfo->pNext;
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO && chain_info->function == func)) {
        chain_info = (VkLayerInstanceCreateInfo *)chain_info->pNext;
    }
    assert(chain_info != NULL);
    return chain_info;
}

VkLayerDeviceCreateInfo *get_chain_info(const VkDeviceCreateInfo *pCreateInfo, VkLayerFunction func) {
    VkLayerDeviceCreateInfo *chain_info = (VkLayerDeviceCreateInfo *)pCreateInfo->pNext;
    while (chain_info && !(chain_info->sType == VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO && chain_info->function == func)) {
        chain_info = (VkLayerDeviceCreateInfo *)chain_info->pNext;
    }
    assert(chain_info != NULL);
    return chain_info;
}

/* Various dispatchable objects will use the same underlying dispatch table if they
 * are created from that "parent" object. Thus use pointer to dispatch table
 * as the key to these table maps.
 *    Instance -> PhysicalDevice
 *    Device -> CommandBuffer or Queue
 * If use the object themselves as key to map then implies Create entrypoints have to be intercepted
 * and a new key inserted into map */
VkLayerInstanceDispatchTable *initInstanceTable(VkInstance instance, const PFN_vkGetInstanceProcAddr gpa, instance_table_map &map) {
    VkLayerInstanceDispatchTable *pTable;
    dispatch_key key = get_dispatch_key(instance);
    instance_table_map::const_iterator it = map.find((void *)key);

    if (it == map.end()) {
        pTable = new VkLayerInstanceDispatchTable;
        map[(void *)key] = pTable;
#if DISPATCH_MAP_DEBUG
        fprintf(stderr, "New, Instance: map: %p, key: %p, table: %p\n", &map, key, pTable);
#endif
    } else {
#if DISPATCH_MAP_DEBUG
        fprintf(stderr, "Instance: map: %p, key: %p, table: %p\n", &map, key, it->second);
#endif
        return it->second;
    }

    layer_init_instance_dispatch_table(instance, pTable, gpa);

    return pTable;
}

VkLayerInstanceDispatchTable *initInstanceTable(VkInstance instance, const PFN_vkGetInstanceProcAddr gpa) {
    return initInstanceTable(instance, gpa, tableInstanceMap);
}

VkLayerDispatchTable *initDeviceTable(VkDevice device, const PFN_vkGetDeviceProcAddr gpa, device_table_map &map) {
    VkLayerDispatchTable *pTable;
    dispatch_key key = get_dispatch_key(device);
    device_table_map::const_iterator it = map.find((void *)key);

    if (it == map.end()) {
        pTable = new VkLayerDispatchTable;
        map[(void *)key] = pTable;
#if DISPATCH_MAP_DEBUG
        fprintf(stderr, "New, Device: map: %p, key: %p, table: %p\n", &map, key, pTable);
#endif
    } else {
#if DISPATCH_MAP_DEBUG
        fprintf(stderr, "Device: map: %p, key: %p, table: %p\n", &map, key, it->second);
#endif
        return it->second;
    }

    layer_init_device_dispatch_table(device, pTable, gpa);

    return pTable;
}

VkLayerDispatchTable *initDeviceTable(VkDevice device, const PFN_vkGetDeviceProcAddr gpa) {
    return initDeviceTable(device, gpa, tableMap);
}
