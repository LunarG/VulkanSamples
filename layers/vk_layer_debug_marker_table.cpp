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
#include "vk_debug_marker_layer.h"
std::unordered_map<void *, VkLayerDebugMarkerDispatchTable *> tableDebugMarkerMap;

/* Various dispatchable objects will use the same underlying dispatch table if they
 * are created from that "parent" object. Thus use pointer to dispatch table
 * as the key to these table maps.
 *    Instance -> PhysicalDevice
 *    Device -> CommandBuffer or Queue
 * If use the object themselves as key to map then implies Create entrypoints have to be intercepted
 * and a new key inserted into map */
VkLayerDebugMarkerDispatchTable * initDebugMarkerTable(VkDevice device)
{
    VkLayerDebugMarkerDispatchTable *pDebugMarkerTable;

    assert(device);
    VkLayerDispatchTable *pDisp = *(VkLayerDispatchTable **) device;

    std::unordered_map<void *, VkLayerDebugMarkerDispatchTable *>::const_iterator it = tableDebugMarkerMap.find((void *) pDisp);
    if (it == tableDebugMarkerMap.end())
    {
        pDebugMarkerTable = new VkLayerDebugMarkerDispatchTable;
        tableDebugMarkerMap[(void *) pDisp] = pDebugMarkerTable;
    } else
    {
        return it->second;
    }

    pDebugMarkerTable->CmdDbgMarkerBegin = (PFN_vkCmdDbgMarkerBegin) pDisp->GetDeviceProcAddr(device, "vkCmdDbgMarkerBegin");
    pDebugMarkerTable->CmdDbgMarkerEnd   = (PFN_vkCmdDbgMarkerEnd) pDisp->GetDeviceProcAddr(device, "vkCmdDbgMarkerEnd");
    pDebugMarkerTable->DbgSetObjectTag   = (PFN_vkDbgSetObjectTag) pDisp->GetDeviceProcAddr(device, "vkDbgSetObjectTag");
    pDebugMarkerTable->DbgSetObjectName  = (PFN_vkDbgSetObjectName) pDisp->GetDeviceProcAddr(device, "vkDbgSetObjectName");

    return pDebugMarkerTable;
}
