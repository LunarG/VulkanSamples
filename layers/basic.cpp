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

static VkLayerDispatchTable * initLayerTable(const VkBaseLayerObject *gpuw)
{
    VkLayerDispatchTable *pTable;

    assert(gpuw);
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap.find((void *) gpuw);
    if (it == tableMap.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap[(void *) gpuw] = pTable;
    } else
    {
        return it->second;
    }

    layer_initialize_dispatch_table(pTable, gpuw->pGPA, (VkPhysicalGpu) gpuw->nextObject);

    return pTable;
}

VK_LAYER_EXPORT VkResult VKAPI vkLayerExtension1(VkDevice device)
{
    printf("In vkLayerExtension1() call w/ device: %p\n", (void*)device);
    printf("vkLayerExtension1 returning SUCCESS\n");
    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetExtensionSupport(VkPhysicalGpu gpu, const char* pExtName)
{
    VkResult result;
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;

    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    if (!strncmp(pExtName, "vkLayerExtension1", strlen("vkLayerExtension1")))
    {
        result = VK_SUCCESS;
    } else if (!strncmp(pExtName, "Basic", strlen("Basic")))
    {
        result = VK_SUCCESS;
    } else if (!tableMap.empty() && (tableMap.find(gpuw) != tableMap.end()))
    {
        printf("At start of wrapped vkGetExtensionSupport() call w/ gpu: %p\n", (void*)gpu);
        VkLayerDispatchTable* pTable = tableMap[gpuw];
        result = pTable->GetExtensionSupport((VkPhysicalGpu)gpuw->nextObject, pExtName);
        printf("Completed wrapped vkGetExtensionSupport() call w/ gpu: %p\n", (void*)gpu);
    } else
    {
        result = VK_ERROR_INVALID_EXTENSION;
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalGpu gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
    VkLayerDispatchTable* pTable = tableMap[gpuw];

    printf("At start of wrapped vkCreateDevice() call w/ gpu: %p\n", (void*)gpu);
    VkResult result = pTable->CreateDevice((VkPhysicalGpu)gpuw->nextObject, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table
    tableMap.emplace(*pDevice, pTable);
    printf("Completed wrapped vkCreateDevice() call w/ pDevice, Device %p: %p\n", (void*)pDevice, (void *) *pDevice);
    return result;
}
VK_LAYER_EXPORT VkResult VKAPI vkGetFormatInfo(VkDevice device, VkFormat format, VkFormatInfoType infoType, size_t* pDataSize, void* pData)
{
    VkLayerDispatchTable* pTable = tableMap[device];

    printf("At start of wrapped vkGetFormatInfo() call w/ device: %p\n", (void*)device);
    VkResult result = pTable->GetFormatInfo(device, format, infoType, pDataSize, pData);
    printf("Completed wrapped vkGetFormatInfo() call w/ device: %p\n", (void*)device);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalGpu gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    if (gpu != NULL)
    {
        VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
        VkLayerDispatchTable* pTable = initLayerTable(gpuw);

        printf("At start of wrapped vkEnumerateLayers() call w/ gpu: %p\n", gpu);
        VkResult result = pTable->EnumerateLayers((VkPhysicalGpu)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
        return result;
    } else
    {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL || pReserved == NULL)
            return VK_ERROR_INVALID_POINTER;

        // Example of a layer that is only compatible with Intel's GPUs
        VkBaseLayerObject* gpuw = (VkBaseLayerObject*) pReserved;
        PFN_vkGetGpuInfo fpGetGpuInfo;
        VkPhysicalGpuProperties gpuProps;
        size_t dataSize = sizeof(VkPhysicalGpuProperties);
        fpGetGpuInfo = (PFN_vkGetGpuInfo) gpuw->pGPA((VkPhysicalGpu) gpuw->nextObject, "vkGetGpuInfo");
        fpGetGpuInfo((VkPhysicalGpu) gpuw->nextObject, VK_INFO_TYPE_PHYSICAL_GPU_PROPERTIES, &dataSize, &gpuProps);
        if (gpuProps.vendorId == 0x8086)
        {
            *pOutLayerCount = 1;
            strncpy((char *) pOutLayers[0], "Basic", maxStringSize);
        } else
        {
            *pOutLayerCount = 0;
        }
        return VK_SUCCESS;
    }
}

VK_LAYER_EXPORT void * VKAPI vkGetProcAddr(VkPhysicalGpu gpu, const char* pName)
{
    if (gpu == NULL)
        return NULL;

    initLayerTable((const VkBaseLayerObject *) gpu);

    if (!strncmp("vkGetProcAddr", pName, sizeof("vkGetProcAddr")))
        return (void *) vkGetProcAddr;
    else if (!strncmp("vkCreateDevice", pName, sizeof ("vkCreateDevice")))
        return (void *) vkCreateDevice;
    else if (!strncmp("vkGetExtensionSupport", pName, sizeof ("vkGetExtensionSupport")))
        return (void *) vkGetExtensionSupport;
    else if (!strncmp("vkEnumerateLayers", pName, sizeof ("vkEnumerateLayers")))
        return (void *) vkEnumerateLayers;
    else if (!strncmp("vkGetFormatInfo", pName, sizeof ("vkGetFormatInfo")))
        return (void *) vkGetFormatInfo;
    else if (!strncmp("vkLayerExtension1", pName, sizeof("vkLayerExtension1")))
        return (void *) vkLayerExtension1;
    else {
        VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((VkPhysicalGpu) gpuw->nextObject, pName);
    }
}
