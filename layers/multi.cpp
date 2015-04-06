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
 *
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

static void initLayerTable(const VkBaseLayerObject *gpuw, VkLayerDispatchTable *pTable, const unsigned int layerNum);

/******************************** Layer multi1 functions **************************/
static std::unordered_map<void *, VkLayerDispatchTable *> tableMap1;
static bool layer1_first_activated = false;

static VkLayerDispatchTable * getLayer1Table(const VkBaseLayerObject *gpuw)
{
    VkLayerDispatchTable *pTable;

    assert(gpuw);
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap1.find((void *) gpuw);
    if (it == tableMap1.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap1[(void *) gpuw] = pTable;
        initLayerTable(gpuw, pTable, 1);
        return pTable;
    } else
    {
        return it->second;
    }
}
#ifdef __cplusplus
extern "C" {
#endif


VK_LAYER_EXPORT VkResult VKAPI multi1CreateDevice(VkPhysicalGpu gpu, const VkDeviceCreateInfo* pCreateInfo,
                                                      VkDevice* pDevice)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
    VkLayerDispatchTable* pTable = getLayer1Table(gpuw);

    printf("At start of multi1 layer vkCreateDevice()\n");
    VkResult result = pTable->CreateDevice((VkPhysicalGpu)gpuw->nextObject, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table
    tableMap1.emplace(*pDevice, pTable);
    printf("Completed multi1 layer vkCreateDevice()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1CreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo,
                                                                VkPipeline* pPipeline)
{
    VkLayerDispatchTable* pTable = tableMap1[device];

    printf("At start of multi1 layer vkCreateGraphicsPipeline()\n");
    VkResult result = pTable->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    // create a mapping for the pipeline object into the dispatch table
    tableMap1.emplace(*pPipeline, pTable);
    printf("Completed multi1 layer vkCreateGraphicsPipeline()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1StorePipeline(VkPipeline pipeline, size_t* pDataSize, void* pData)
{
    VkLayerDispatchTable* pTable = tableMap1[pipeline];

    printf("At start of multi1 layer vkStorePipeline()\n");
    VkResult result = pTable->StorePipeline(pipeline, pDataSize, pData);
    printf("Completed multi1 layer vkStorePipeline()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1EnumerateLayers(VkPhysicalGpu gpu, size_t maxLayerCount, size_t maxStringSize,
                                                         size_t* pOutLayerCount, char* const* pOutLayers,
                                                         void* pReserved)
{
    if (gpu == NULL)
        return vkEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);

    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
    VkLayerDispatchTable* pTable = getLayer1Table(gpuw);

    printf("At start of multi1 layer vkEnumerateLayers()\n");
    VkResult result = pTable->EnumerateLayers((VkPhysicalGpu)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
    printf("Completed multi1 layer vkEnumerateLayers()\n");
    return result;
}

VK_LAYER_EXPORT void * VKAPI multi1GetProcAddr(VkPhysicalGpu gpu, const char* pName)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;

    if (gpu == NULL)
        return NULL;

    getLayer1Table(gpuw);

    if (!strncmp("vkCreateDevice", pName, sizeof ("vkCreateDevice")))
        return (void *) multi1CreateDevice;
    else if (!strncmp("vkEnumerateLayers", pName, sizeof ("vkEnumerateLayers")))
        return (void *) multi1EnumerateLayers;
    else if (!strncmp("vkCreateGraphicsPipeline", pName, sizeof ("vkCreateGraphicsPipeline")))
        return (void *) multi1CreateGraphicsPipeline;
    else if (!strncmp("vkStorePipeline", pName, sizeof ("vkStorePipeline")))
        return (void *) multi1StorePipeline;
    else if (!strncmp("vkGetExtensionSupport", pName, sizeof ("vkGetExtensionSupport")))
        return (void *) vkGetExtensionSupport;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((VkPhysicalGpu) gpuw->nextObject, pName);
    }
}

/******************************** Layer multi2 functions **************************/
static std::unordered_map<void *, VkLayerDispatchTable *> tableMap2;
static bool layer2_first_activated = false;

static VkLayerDispatchTable * getLayer2Table(const VkBaseLayerObject *gpuw)
{
    VkLayerDispatchTable *pTable;

    assert(gpuw);
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap2.find((void *) gpuw);
    if (it == tableMap2.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap2[(void *) gpuw] = pTable;
        initLayerTable(gpuw, pTable, 2);
        return pTable;
    } else
    {
        return it->second;
    }
}

VK_LAYER_EXPORT VkResult VKAPI multi2CreateDevice(VkPhysicalGpu gpu, const VkDeviceCreateInfo* pCreateInfo,
                                                      VkDevice* pDevice)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
    VkLayerDispatchTable* pTable = getLayer2Table(gpuw);

    printf("At start of multi2 vkCreateDevice()\n");
    VkResult result = pTable->CreateDevice((VkPhysicalGpu)gpuw->nextObject, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table for layer2
    tableMap2.emplace(*pDevice, pTable);
    printf("Completed multi2 layer vkCreateDevice()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi2CreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo,
                                                             VkCmdBuffer* pCmdBuffer)
{
    VkLayerDispatchTable* pTable = tableMap2[device];

    printf("At start of multi2 layer vkCreateCommandBuffer()\n");
    VkResult result = pTable->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    // create a mapping for CmdBuffer object into the dispatch table for layer 2
    tableMap2.emplace(*pCmdBuffer, pTable);
    printf("Completed multi2 layer vkCreateCommandBuffer()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi2BeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo)
{
    VkLayerDispatchTable* pTable = tableMap2[cmdBuffer];

    printf("At start of multi2 layer vkBeginCommandBuffer()\n");
    VkResult result = pTable->BeginCommandBuffer(cmdBuffer, pBeginInfo);
    printf("Completed multi2 layer vkBeginCommandBuffer()\n");
    return result;

}

VK_LAYER_EXPORT VkResult VKAPI multi2EnumerateLayers(VkPhysicalGpu gpu, size_t maxLayerCount, size_t maxStringSize,
                                                         size_t* pOutLayerCount, char* const* pOutLayers,
                                                         void* pReserved)
{
    if (gpu == NULL)
        return vkEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);

    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
    VkLayerDispatchTable* pTable = getLayer2Table(gpuw);

    printf("At start of multi2 layer vkEnumerateLayers()\n");
    VkResult result = pTable->EnumerateLayers((VkPhysicalGpu)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
    printf("Completed multi2 layer vkEnumerateLayers()\n");
    return result;
}

VK_LAYER_EXPORT void * VKAPI multi2GetProcAddr(VkPhysicalGpu gpu, const char* pName)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;

    if (gpu == NULL)
        return NULL;

    getLayer2Table(gpuw);

    if (!strncmp("vkCreateDevice", pName, sizeof ("vkCreateDevice")))
        return (void *) multi2CreateDevice;
    else if (!strncmp("vkEnumerateLayers", pName, sizeof ("vkEnumerateLayers")))
        return (void *) multi2EnumerateLayers;
    else if (!strncmp("vkCreateCommandBuffer", pName, sizeof ("vkCreateCommandBuffer")))
        return (void *) multi2CreateCommandBuffer;
    else if (!strncmp("vkBeginCommandBuffer", pName, sizeof ("vkBeginCommandBuffer")))
        return (void *) multi2BeginCommandBuffer;
    else if (!strncmp("vkGetExtensionSupport", pName, sizeof ("vkGetExtensionSupport")))
        return (void *) vkGetExtensionSupport;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((VkPhysicalGpu) gpuw->nextObject, pName);
    }
}

/********************************* Common functions ********************************/
VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalGpu gpu, size_t maxLayerCount, size_t maxStringSize,
                                                      size_t* pOutLayerCount, char* const* pOutLayers,
                                                      void* pReserved)
{
    if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL || pOutLayers[1] == NULL || pReserved == NULL)
        return VK_ERROR_INVALID_POINTER;

    if (maxLayerCount < 2)
        return VK_ERROR_INITIALIZATION_FAILED;
    *pOutLayerCount = 2;
    strncpy((char *) pOutLayers[0], "multi1", maxStringSize);
    strncpy((char *) pOutLayers[1], "multi2", maxStringSize);
    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetExtensionSupport(VkPhysicalGpu gpu, const char* pExtName)
{
    VkResult result;
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;

    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    if (!strncmp(pExtName, "multi1", strlen("multi1")))
    {
        result = VK_SUCCESS;
    } else if (!strncmp(pExtName, "multi2", strlen("multi2")))
    {
        result = VK_SUCCESS;
    } else if (!tableMap1.empty() && (tableMap1.find(gpuw) != tableMap1.end()))
    {
        VkLayerDispatchTable* pTable = tableMap1[gpuw];
        result = pTable->GetExtensionSupport((VkPhysicalGpu)gpuw->nextObject, pExtName);
    } else if (!tableMap2.empty() && (tableMap2.find(gpuw) != tableMap2.end()))
    {
        VkLayerDispatchTable* pTable = tableMap2[gpuw];
        result = pTable->GetExtensionSupport((VkPhysicalGpu)gpuw->nextObject, pExtName);
    } else
    {
        result = VK_ERROR_INVALID_EXTENSION;
    }
    return result;
}

VK_LAYER_EXPORT void * VKAPI vkGetProcAddr(VkPhysicalGpu gpu, const char* pName)
{
    // to find each layers GPA routine Loader will search via "<layerName>GetProcAddr"
    if (!strncmp("multi1GetProcAddr", pName, sizeof("multi1GetProcAddr")))
        return (void *) multi1GetProcAddr;
    else if (!strncmp("multi2GetProcAddr", pName, sizeof("multi2GetProcAddr")))
        return (void *) multi2GetProcAddr;
    else if (!strncmp("vkGetProcAddr", pName, sizeof("vkGetProcAddr")))
        return (void *) vkGetProcAddr;

    // use first layer activated as GPA dispatch table activation happens in order
    else if (layer1_first_activated)
        return multi1GetProcAddr(gpu, pName);
    else if (layer2_first_activated)
        return multi2GetProcAddr(gpu, pName);
    else
        return NULL;

}

#ifdef __cplusplus
}    //extern "C"
#endif

static void initLayerTable(const VkBaseLayerObject *gpuw, VkLayerDispatchTable *pTable, const unsigned int layerNum)
{
    if (layerNum == 2 && layer1_first_activated == false)
        layer2_first_activated = true;
    if (layerNum == 1 && layer2_first_activated == false)
        layer1_first_activated = true;

    layer_initialize_dispatch_table(pTable, gpuw->pGPA, (VkPhysicalGpu) gpuw->nextObject);
}
