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

static void initLayerTable(const VkBaseLayerObject *devw, VkLayerDispatchTable *pTable, const unsigned int layerNum);
static void initLayerInstanceTable(const VkBaseLayerObject *instw, VkLayerInstanceDispatchTable *pTable, const unsigned int layerNum);
/* Various dispatchable objects will use the same underlying dispatch table if they
 * are created from that "parent" object. Thus use pointer to dispatch table
 * as the key to table maps (tableMap1, tableInstanceMap1, tableMap2, tableInstanceMap2.
 *    Instance -> PhysicalDevice
 *    Device -> CmdBuffer or Queue
 * If use the object themselves as key to map then implies Create entrypoints have to be intercepted
 * and a new key inserted into map */
/******************************** Layer multi1 functions **************************/
static std::unordered_map<void *, VkLayerDispatchTable *> tableMap1;
static std::unordered_map<void *, VkLayerInstanceDispatchTable *> tableInstanceMap1;
static bool layer1_first_activated = false;

static VkLayerDispatchTable *getLayer1Table(const VkBaseLayerObject *devw)
{
    VkLayerDispatchTable *pTable;
    assert(devw);
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) devw->baseObject;

    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap1.find((void *) *ppDisp);
    if (it == tableMap1.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap1[(void *) *ppDisp] = pTable;
        initLayerTable(devw, pTable, 1);
        return pTable;
    } else
    {
        return it->second;
    }
}
static VkLayerInstanceDispatchTable *getLayer1InstanceTable(const VkBaseLayerObject *instw)
{
    VkLayerInstanceDispatchTable *pTable;
    assert(instw);
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instw->baseObject;

    std::unordered_map<void *, VkLayerInstanceDispatchTable *>::const_iterator it = tableInstanceMap1.find((void *) *ppDisp);
    if (it == tableInstanceMap1.end())
    {
        pTable =  new VkLayerInstanceDispatchTable;
        tableInstanceMap1[(void *) *ppDisp] = pTable;
        initLayerInstanceTable(instw, pTable, 1);
        return pTable;
    } else
    {
        return it->second;
    }
}
#ifdef __cplusplus
extern "C" {
#endif

/* hook DextroyDevice to remove tableMap entry */
VK_LAYER_EXPORT VkResult VKAPI multi1DestroyDevice(VkDevice device)
{
    VkLayerDispatchTable *pDisp = *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap1[pDisp];
    VkResult res = pTable->DestroyDevice(device);
    tableMap1.erase(pDisp);
    return res;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VkResult VKAPI multi1DestroyInstance(VkInstance instance)
{
    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) instance;
    VkLayerInstanceDispatchTable *pTable = tableInstanceMap1[pDisp];
    VkResult res = pTable->DestroyInstance(instance);
    tableInstanceMap1.erase(pDisp);
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI multi1CreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap1[*ppDisp];

    printf("At start of multi1 layer vkCreateSampler()\n");
    VkResult result = pTable->CreateSampler(device, pCreateInfo, pSampler);
    printf("Completed multi1 layer vkCreateSampler()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1CreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo,
                                                                VkPipeline* pPipeline)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap1[*ppDisp];

    printf("At start of multi1 layer vkCreateGraphicsPipeline()\n");
    VkResult result = pTable->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    printf("Completed multi1 layer vkCreateGraphicsPipeline()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1StorePipeline(VkDevice device, VkPipeline pipeline, size_t* pDataSize, void* pData)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap1[*ppDisp];

    printf("At start of multi1 layer vkStorePipeline()\n");
    VkResult result = pTable->StorePipeline(device, pipeline, pDataSize, pData);
    printf("Completed multi1 layer vkStorePipeline()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1EnumerateLayers(VkPhysicalDevice gpu, size_t maxStringSize,
                                                         size_t* pLayerCount, char* const* pOutLayers,
                                                         void* pReserved)
{
    if (gpu == NULL)
        return vkEnumerateLayers(gpu, maxStringSize, pLayerCount, pOutLayers, pReserved);

    VkLayerInstanceDispatchTable* pTable = tableInstanceMap1[gpu];
    printf("At start of multi1 layer vkEnumerateLayers()\n");
    VkResult result = pTable->EnumerateLayers(gpu, maxStringSize, pLayerCount, pOutLayers, pReserved);
    printf("Completed multi1 layer vkEnumerateLayers()\n");
    return result;
}

VK_LAYER_EXPORT void * VKAPI multi1GetDeviceProcAddr(VkDevice device, const char* pName)
{
    VkBaseLayerObject* devw = (VkBaseLayerObject *) device;

    if (device == NULL)
        return NULL;

    getLayer1Table(devw);

    if (!strcmp("vkGetDeviceProcAddr", pName))
        return (void *) multi1GetDeviceProcAddr;
    if (!strcmp("vkDestroyDevice", pName))
        return (void *) multi1DestroyDevice;
    if (!strcmp("vkCreateSampler", pName))
        return (void *) multi1CreateSampler;
    else if (!strcmp("vkCreateGraphicsPipeline", pName))
        return (void *) multi1CreateGraphicsPipeline;
    else if (!strcmp("vkStorePipeline", pName))
        return (void *) multi1StorePipeline;
    else {
        if (devw->pGPA == NULL)
            return NULL;
        return devw->pGPA((VkObject) devw->nextObject, pName);
    }
}

VK_LAYER_EXPORT void * VKAPI multi1GetInstanceProcAddr(VkInstance inst, const char* pName)
{
    VkBaseLayerObject* instw = (VkBaseLayerObject *) inst;

    if (inst == NULL)
        return NULL;

    getLayer1InstanceTable(instw);

    if (!strcmp("vkGetInstanceProcAddr", pName))
        return (void *) multi1GetInstanceProcAddr;
    if (!strcmp("vkDestroyInstance", pName))
        return (void *) multi1DestroyInstance;
    if (!strcmp("vkEnumerateLayers", pName))
        return (void *) multi1EnumerateLayers;
    else if (!strcmp("GetGlobalExtensionInfo", pName))
        return (void*) vkGetGlobalExtensionInfo;
    else {
        if (instw->pGPA == NULL)
            return NULL;
        return instw->pGPA((VkObject) instw->nextObject, pName);
    }
}

/******************************** Layer multi2 functions **************************/
static std::unordered_map<void *, VkLayerDispatchTable *> tableMap2;
static std::unordered_map<void *, VkLayerInstanceDispatchTable *> tableInstanceMap2;
static bool layer2_first_activated = false;

static VkLayerInstanceDispatchTable *getLayer2InstanceTable(const VkBaseLayerObject *instw)
{
    VkLayerInstanceDispatchTable *pTable;
    assert(instw);
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instw->baseObject;

    std::unordered_map<void *, VkLayerInstanceDispatchTable *>::const_iterator it = tableInstanceMap2.find((void *) *ppDisp);
    if (it == tableInstanceMap2.end())
    {
        pTable =  new VkLayerInstanceDispatchTable;
        tableInstanceMap2[(void *) *ppDisp] = pTable;
        initLayerInstanceTable(instw, pTable, 2);
        return pTable;
    } else
    {
        return it->second;
    }
}

static VkLayerDispatchTable *getLayer2Table(const VkBaseLayerObject *devw)
{
    VkLayerDispatchTable *pTable;
    assert(devw);
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) devw->baseObject;

    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap2.find((void *) *ppDisp);
    if (it == tableMap2.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap2[(void *) *ppDisp] = pTable;
        initLayerTable(devw, pTable, 2);
        return pTable;
    } else
    {
        return it->second;
    }
}

VK_LAYER_EXPORT VkResult VKAPI multi2EnumeratePhysicalDevices(
                                            VkInstance instance,
                                            uint32_t* pPhysicalDeviceCount,
                                            VkPhysicalDevice* pPhysicalDevices)
{
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instance;
    VkLayerInstanceDispatchTable *pInstTable = tableInstanceMap2[*ppDisp];

    printf("At start of wrapped multi2 vkEnumeratePhysicalDevices()\n");
    VkResult result = pInstTable->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    printf("Completed multi2 layer vkEnumeratePhysicalDevices()\n");
    return result;
}

/* hook DextroyDevice to remove tableMap entry */
VK_LAYER_EXPORT VkResult VKAPI multi2DestroyDevice(VkDevice device)
{
    VkLayerDispatchTable *pDisp = *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap2[pDisp];
    VkResult res = pTable->DestroyDevice(device);
    tableMap2.erase(pDisp);
    return res;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VkResult VKAPI multi2DestroyInstance(VkInstance instance)
{
    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) instance;
    VkLayerInstanceDispatchTable *pTable = tableInstanceMap2[pDisp];
    VkResult res = pTable->DestroyInstance(instance);
    tableInstanceMap2.erase(pDisp);
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI multi2CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo,
                                                      VkDevice* pDevice)
{
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) gpu;
    VkLayerInstanceDispatchTable *pInstTable = tableInstanceMap2[*ppDisp];

    printf("At start of multi2 vkCreateDevice()\n");
    VkResult result = pInstTable->CreateDevice(gpu, pCreateInfo, pDevice);
    printf("Completed multi2 layer vkCreateDevice()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi2CreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo,
                                                             VkCmdBuffer* pCmdBuffer)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap2[*ppDisp];

    printf("At start of multi2 layer vkCreateCommandBuffer()\n");
    VkResult result = pTable->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    printf("Completed multi2 layer vkCreateCommandBuffer()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi2BeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) cmdBuffer;
    VkLayerDispatchTable *pTable = tableMap2[*ppDisp];

    printf("At start of multi2 layer vkBeginCommandBuffer()\n");
    VkResult result = pTable->BeginCommandBuffer(cmdBuffer, pBeginInfo);
    printf("Completed multi2 layer vkBeginCommandBuffer()\n");
    return result;

}

VK_LAYER_EXPORT VkResult VKAPI multi2EnumerateLayers(VkPhysicalDevice gpu, size_t maxStringSize,
                                                         size_t* pLayerCount, char* const* pOutLayers,
                                                         void* pReserved)
{
    if (gpu == NULL)
        return vkEnumerateLayers(gpu, maxStringSize, pLayerCount, pOutLayers, pReserved);

    VkLayerInstanceDispatchTable* pTable = tableInstanceMap2[gpu];

    printf("At start of multi2 layer vkEnumerateLayers()\n");
    VkResult result = pTable->EnumerateLayers(gpu, maxStringSize, pLayerCount, pOutLayers, pReserved);
    printf("Completed multi2 layer vkEnumerateLayers()\n");
    return result;
}

VK_LAYER_EXPORT void * VKAPI multi2GetDeviceProcAddr(VkDevice device, const char* pName)
{
    VkBaseLayerObject* devw = (VkBaseLayerObject *) device;

    if (device == NULL)
        return NULL;

    getLayer2Table(devw);

    if (!strcmp("vkGetDeviceProcAddr", pName))
        return (void *) multi2GetDeviceProcAddr;
    if (!strcmp("vkDestroyDevice", pName))
        return (void *) multi2DestroyDevice;
    if (!strcmp("vkCreateCommandBuffer", pName))
        return (void *) multi2CreateCommandBuffer;
    else if (!strcmp("vkBeginCommandBuffer", pName))
        return (void *) multi2BeginCommandBuffer;
    else {
        if (devw->pGPA == NULL)
            return NULL;
        return devw->pGPA((VkObject) devw->nextObject, pName);
    }
}

VK_LAYER_EXPORT void * VKAPI multi2GetInstanceProcAddr(VkInstance inst, const char* pName)
{
    VkBaseLayerObject* instw = (VkBaseLayerObject *) inst;

    if (inst == NULL)
        return NULL;

    getLayer2InstanceTable(instw);

    if (!strcmp("vkGetInstanceProcAddr", pName))
        return (void *) multi2GetInstanceProcAddr;
    if (!strcmp("vkEnumeratePhysicalDevices", pName))
        return (void *) multi2EnumeratePhysicalDevices;
    if (!strcmp("vkDestroyInstance", pName))
        return (void *) multi2DestroyInstance;
    if (!strcmp("vkCreateDevice", pName))
        return (void *) multi2CreateDevice;
    else if (!strcmp("vkEnumerateLayers", pName))
        return (void *) multi2EnumerateLayers;
    else if (!strcmp("GetGlobalExtensionInfo", pName))
        return (void*) vkGetGlobalExtensionInfo;
    else {
        if (instw->pGPA == NULL)
            return NULL;
        return instw->pGPA((VkObject) instw->nextObject, pName);
    }
}

/********************************* Common functions ********************************/
VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalDevice gpu, size_t maxStringSize,
                                                 size_t* pLayerCount, char* const* pOutLayers,
                                                 void* pReserved)
{
    if (pLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL || pOutLayers[1] == NULL || pReserved == NULL)
        return VK_ERROR_INVALID_POINTER;

    if (*pLayerCount < 2)
        return VK_ERROR_INITIALIZATION_FAILED;
    *pLayerCount = 2;
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

VK_LAYER_EXPORT void * VKAPI vkGetDeviceProcAddr(VkDevice device, const char* pName)
{
    // to find each layers GPA routine Loader will search via "<layerName>GetDeviceProcAddr"
    if (!strcmp("multi1GetDeviceProcAddr", pName))
        return (void *) multi1GetDeviceProcAddr;
    else if (!strcmp("multi2GetDeviceProcAddr", pName))
        return (void *) multi2GetDeviceProcAddr;
    else if (!strcmp("vkGetDeviceProcAddr", pName))
        return (void *) vkGetDeviceProcAddr;

    // use first layer activated as GPA dispatch table activation happens in order
    else if (layer1_first_activated)
        return multi1GetDeviceProcAddr(device, pName);
    else if (layer2_first_activated)
        return multi2GetDeviceProcAddr(device, pName);
    else
        return NULL;

}

VK_LAYER_EXPORT void * VKAPI vkGetInstanceProcAddr(VkInstance inst, const char* pName)
{
    // to find each layers GPA routine Loader will search via "<layerName>GetInstanceProcAddr"
    if (!strcmp("multi1GetInstanceProcAddr", pName))
        return (void *) multi1GetInstanceProcAddr;
    else if (!strcmp("multi2GetInstanceProcAddr", pName))
        return (void *) multi2GetInstanceProcAddr;
    else if (!strcmp("vkGetInstanceProcAddr", pName))
        return (void *) vkGetInstanceProcAddr;

    // use first layer activated as GPA dispatch table activation happens in order
    else if (layer1_first_activated)
        return multi1GetInstanceProcAddr(inst, pName);
    else if (layer2_first_activated)
        return multi2GetInstanceProcAddr(inst, pName);
    else
        return NULL;

}
#ifdef __cplusplus
}    //extern "C"
#endif

static void initLayerTable(const VkBaseLayerObject *devw, VkLayerDispatchTable *pTable, const unsigned int layerNum)
{
    if (layerNum == 2 && layer1_first_activated == false)
        layer2_first_activated = true;
    if (layerNum == 1 && layer2_first_activated == false)
        layer1_first_activated = true;

    layer_initialize_dispatch_table(pTable, (PFN_vkGetDeviceProcAddr) devw->pGPA, (VkDevice) devw->nextObject);
}

static void initLayerInstanceTable(const VkBaseLayerObject *instw, VkLayerInstanceDispatchTable *pTable, const unsigned int layerNum)
{
    if (layerNum == 2 && layer1_first_activated == false)
        layer2_first_activated = true;
    if (layerNum == 1 && layer2_first_activated == false)
        layer1_first_activated = true;

    layer_init_instance_dispatch_table(pTable, (PFN_vkGetInstanceProcAddr) instw->pGPA, (VkInstance) instw->nextObject);
}
