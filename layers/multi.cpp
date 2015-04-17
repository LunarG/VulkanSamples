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
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap1.find((void *) gpuw->baseObject);
    if (it == tableMap1.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap1[(void *) gpuw->baseObject] = pTable;
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


VK_LAYER_EXPORT VkResult VKAPI multi1CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo,
                                                      VkDevice* pDevice)
{
    VkLayerDispatchTable* pTable = tableMap1[gpu];
    printf("At start of multi1 layer vkCreateDevice()\n");
    VkResult result = pTable->CreateDevice(gpu, pCreateInfo, pDevice);
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

VK_LAYER_EXPORT VkResult VKAPI multi1StorePipeline(VkDevice device, VkPipeline pipeline, size_t* pDataSize, void* pData)
{
    VkLayerDispatchTable* pTable = tableMap1[pipeline];

    printf("At start of multi1 layer vkStorePipeline()\n");
    VkResult result = pTable->StorePipeline(device, pipeline, pDataSize, pData);
    printf("Completed multi1 layer vkStorePipeline()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1EnumerateLayers(VkPhysicalDevice gpu, size_t maxLayerCount, size_t maxStringSize,
                                                         size_t* pOutLayerCount, char* const* pOutLayers,
                                                         void* pReserved)
{
    if (gpu == NULL)
        return vkEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);

    VkLayerDispatchTable* pTable = tableMap1[gpu];
    printf("At start of multi1 layer vkEnumerateLayers()\n");
    VkResult result = pTable->EnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
    printf("Completed multi1 layer vkEnumerateLayers()\n");
    return result;
}

VK_LAYER_EXPORT void * VKAPI multi1GetProcAddr(VkPhysicalDevice gpu, const char* pName)
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
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((VkPhysicalDevice) gpuw->nextObject, pName);
    }
}

/******************************** Layer multi2 functions **************************/
static std::unordered_map<void *, VkLayerDispatchTable *> tableMap2;
static bool layer2_first_activated = false;

static VkLayerDispatchTable * getLayer2Table(const VkBaseLayerObject *gpuw)
{
    VkLayerDispatchTable *pTable;

    assert(gpuw);
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap2.find((void *) gpuw->baseObject);
    if (it == tableMap2.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap2[(void *) gpuw->baseObject] = pTable;
        initLayerTable(gpuw, pTable, 2);
        return pTable;
    } else
    {
        return it->second;
    }
}

VK_LAYER_EXPORT VkResult VKAPI multi2CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo,
                                                      VkDevice* pDevice)
{
    VkLayerDispatchTable* pTable = tableMap2[gpu];

    printf("At start of multi2 vkCreateDevice()\n");
    VkResult result = pTable->CreateDevice(gpu, pCreateInfo, pDevice);
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

VK_LAYER_EXPORT VkResult VKAPI multi2EnumerateLayers(VkPhysicalDevice gpu, size_t maxLayerCount, size_t maxStringSize,
                                                         size_t* pOutLayerCount, char* const* pOutLayers,
                                                         void* pReserved)
{
    if (gpu == NULL)
        return vkEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);

    VkLayerDispatchTable* pTable = tableMap2[gpu];

    printf("At start of multi2 layer vkEnumerateLayers()\n");
    VkResult result = pTable->EnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
    printf("Completed multi2 layer vkEnumerateLayers()\n");
    return result;
}

VK_LAYER_EXPORT void * VKAPI multi2GetProcAddr(VkPhysicalDevice gpu, const char* pName)
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
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((VkPhysicalDevice) gpuw->nextObject, pName);
    }
}

/********************************* Common functions ********************************/
VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalDevice gpu, size_t maxLayerCount, size_t maxStringSize,
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

struct extProps {
    uint32_t version;
    const char * const name;
};

#define MULTI_LAYER_EXT_ARRAY_SIZE 2
static const struct extProps multiExts[MULTI_LAYER_EXT_ARRAY_SIZE] = {
    // TODO what is the version?
    0x10, "multi1",
    0x10, "multi2",
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    VkResult result;

    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    VkExtensionProperties *ext_props;
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = MULTI_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= MULTI_LAYER_EXT_ARRAY_SIZE)
                return VK_ERROR_INVALID_VALUE;
            ext_props = (VkExtensionProperties *) pData;
            ext_props->version = multiExts[extensionIndex].version;
            strncpy(ext_props->extName, multiExts[extensionIndex].name,
                                        VK_MAX_EXTENSION_NAME);
            ext_props->extName[VK_MAX_EXTENSION_NAME - 1] = '\0';
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

VK_LAYER_EXPORT void * VKAPI vkGetProcAddr(VkPhysicalDevice gpu, const char* pName)
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

    layer_initialize_dispatch_table(pTable, gpuw->pGPA, (VkPhysicalDevice) gpuw->nextObject);
}
