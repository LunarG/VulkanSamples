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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>
#include "loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vkLayer.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"

static std::unordered_map<void *, VkLayerDispatchTable *> tableMap;
static std::unordered_map<void *, VkLayerInstanceDispatchTable *> tableInstanceMap;

/* Various dispatchable objects will use the same underlying dispatch table if they
 * are created from that "parent" object. Thus use pointer to dispatch table
 * as the key to these table maps.
 *    Instance -> PhysicalDevice
 *    Device -> CmdBuffer or Queue
 * If use the object themselves as key to map then implies Create entrypoints have to be intercepted
 * and a new key inserted into map */
static VkLayerInstanceDispatchTable * initLayerInstanceTable(const VkBaseLayerObject *instancew)
{
    VkLayerInstanceDispatchTable *pTable;
    assert(instancew);
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instancew->baseObject;

    std::unordered_map<void *, VkLayerInstanceDispatchTable *>::const_iterator it = tableInstanceMap.find((void *) *ppDisp);
    if (it == tableInstanceMap.end())
    {
        pTable =  new VkLayerInstanceDispatchTable;
        tableInstanceMap[(void *) *ppDisp] = pTable;
    } else
    {
        return it->second;
    }

    layer_init_instance_dispatch_table(pTable, instancew);

    return pTable;
}

static VkLayerDispatchTable * initLayerTable(const VkBaseLayerObject *devw)
{
    VkLayerDispatchTable *pTable;
    assert(devw);
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) (devw->baseObject);

    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap.find((void *) *ppDisp);
    if (it == tableMap.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap[(void *) *ppDisp] = pTable;
    } else
    {
        return it->second;
    }

    layer_initialize_dispatch_table(pTable, devw);

    return pTable;
}

VK_LAYER_EXPORT VkResult VKAPI vkLayerExtension1(VkDevice device)
{
    printf("In vkLayerExtension1() call w/ device: %p\n", (void*)device);
    printf("vkLayerExtension1 returning SUCCESS\n");
    return VK_SUCCESS;
}

#define BASIC_LAYER_EXT_ARRAY_SIZE 2

static const VkExtensionProperties basicExts[BASIC_LAYER_EXT_ARRAY_SIZE] = {
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        "Basic",
        0x10,
        "Sample layer: Basic ",
//        0,
//        NULL,
    },
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        "vkLayerExtension1",
        0x10,
        "Sample layer: Basic",
//        0,
//        NULL,
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = BASIC_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= BASIC_LAYER_EXT_ARRAY_SIZE)
                return VK_ERROR_INVALID_VALUE;
            memcpy((VkExtensionProperties *) pData, &basicExts[extensionIndex], sizeof(VkExtensionProperties));
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(
                                            VkInstance instance,
                                            uint32_t* pPhysicalDeviceCount,
                                            VkPhysicalDevice* pPhysicalDevices)
{
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instance;
    VkLayerInstanceDispatchTable *pInstTable = tableInstanceMap[*ppDisp];
    printf("At start of wrapped vkEnumeratePhysicalDevices() call w/ inst: %p\n", (void*)instance);
    VkResult result = pInstTable->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    printf("Completed wrapped vkEnumeratePhysicalDevices() call w/ count %u\n", *pPhysicalDeviceCount);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) gpu;
    VkLayerInstanceDispatchTable* pInstTable = tableInstanceMap[*ppDisp];

    printf("At start of wrapped vkCreateDevice() call w/ gpu: %p\n", (void*)gpu);
    VkResult result = pInstTable->CreateDevice(gpu, pCreateInfo, pDevice);
    printf("Completed wrapped vkCreateDevice() call w/ pDevice, Device %p: %p\n", (void*)pDevice, (void *) *pDevice);
    return result;
}

/* hook DestroyDevice to remove tableMap entry */
VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(VkDevice device)
{
    VkLayerDispatchTable *pDisp =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult res = pTable->DestroyDevice(device);
    tableMap.erase(pDisp);
    return res;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VkResult VKAPI vkDestroyInstance(VkInstance instance)
{
   VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) instance;
    VkLayerInstanceDispatchTable *pTable = tableInstanceMap[pDisp];
    VkResult res = pTable->DestroyInstance(instance);
    tableInstanceMap.erase(pDisp);
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFormatInfo(VkDevice device, VkFormat format, VkFormatInfoType infoType, size_t* pDataSize, void* pData)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;
    VkLayerDispatchTable* pTable = tableMap[*ppDisp];

    printf("At start of wrapped vkGetFormatInfo() call w/ device: %p\n", (void*)device);
    VkResult result = pTable->GetFormatInfo(device, format, infoType, pDataSize, pData);
    printf("Completed wrapped vkGetFormatInfo() call w/ device: %p\n", (void*)device);
    return result;
}

VK_LAYER_EXPORT void * VKAPI vkGetDeviceProcAddr(VkDevice device, const char* pName)
{
    if (device == NULL)
        return NULL;

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp("vkGetDeviceProcAddr", pName)) {
        initLayerTable((const VkBaseLayerObject *) device);
        return (void *) vkGetDeviceProcAddr;
    }

    if (!strcmp("vkGetFormatInfo", pName))
        return (void *) vkGetFormatInfo;
    if (!strcmp("vkDestroyDevice", pName))
        return (void *) vkDestroyDevice;
    if (!strcmp("vkLayerExtension1", pName))
        return (void *) vkLayerExtension1;
    else
    {
        VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;
        VkLayerDispatchTable* pTable = tableMap[*ppDisp];
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(device, pName);
    }
}

VK_LAYER_EXPORT void * VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* pName)
{
    if (instance == NULL)
        return NULL;

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp("vkGetInstanceProcAddr", pName)) {
        initLayerInstanceTable((const VkBaseLayerObject *) instance);
        return (void *) vkGetInstanceProcAddr;
    }

    if (!strcmp("vkDestroyInstance", pName))
        return (void *) vkDestroyInstance;
    if (!strcmp("vkEnumeratePhysicalDevices", pName))
        return (void*) vkEnumeratePhysicalDevices;
    if (!strcmp("vkGetGlobalExtensionInfo", pName))
        return (void*) vkGetGlobalExtensionInfo;
    if (!strcmp("vkCreateDevice", pName))
        return (void *) vkCreateDevice;
    else
    {
        VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instance;
        VkLayerInstanceDispatchTable* pTable = tableInstanceMap[*ppDisp];
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(instance, pName);
    }

}
