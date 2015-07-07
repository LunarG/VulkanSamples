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
#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_layer.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "vk_loader_platform.h"
#include "vk_layer_extension_utils.h"

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

// Map lookup must be thread safe
static inline VkLayerDispatchTable *device_dispatch_table1(VkObject object)
{
    VkLayerDispatchTable *pDisp  = *(VkLayerDispatchTable **) object;
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap1.find((void *) pDisp);
    assert(it != tableMap1.end() && "Not able to find device dispatch entry");
    return it->second;
}

static inline VkLayerInstanceDispatchTable *instance_dispatch_table1(VkObject object)
{
    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) object;
    std::unordered_map<void *, VkLayerInstanceDispatchTable *>::const_iterator it = tableInstanceMap1.find((void *) pDisp);
    assert(it != tableInstanceMap1.end() && "Not able to find instance dispatch entry");
    return it->second;
}

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

/* hook DestroyDevice to remove tableMap entry */
VK_LAYER_EXPORT VkResult VKAPI multi1DestroyDevice(VkDevice device)
{
    VkLayerDispatchTable *pDisp = *(VkLayerDispatchTable **) device;
    VkResult res = device_dispatch_table1(device)->DestroyDevice(device);
    tableMap1.erase(pDisp);
    return res;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VkResult VKAPI multi1DestroyInstance(VkInstance instance)
{
    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) instance;
    VkResult res = instance_dispatch_table1(instance)->DestroyInstance(instance);
    tableInstanceMap1.erase(pDisp);
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI multi1CreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;

    printf("At start of multi1 layer vkCreateSampler()\n");
    VkResult result = device_dispatch_table1(device)->CreateSampler(device, pCreateInfo, pSampler);
    printf("Completed multi1 layer vkCreateSampler()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1CreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo,
                                                                VkPipeline* pPipeline)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;

    printf("At start of multi1 layer vkCreateGraphicsPipeline()\n");
    VkResult result = device_dispatch_table1(device)->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    printf("Completed multi1 layer vkCreateGraphicsPipeline()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1StorePipeline(VkDevice device, VkPipeline pipeline, size_t* pDataSize, void* pData)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;

    printf("At start of multi1 layer vkStorePipeline()\n");
    VkResult result = device_dispatch_table1(device)->StorePipeline(device, pipeline, pDataSize, pData);
    printf("Completed multi1 layer vkStorePipeline()\n");
    return result;
}

VK_LAYER_EXPORT void * VKAPI multi1GetDeviceProcAddr(VkDevice device, const char* pName)
{
    VkBaseLayerObject* devw = (VkBaseLayerObject *) device;

    if (device == NULL)
        return NULL;



    if (!strcmp("vkGetDeviceProcAddr", pName)) {
        getLayer1Table(devw);
        return (void *) multi1GetDeviceProcAddr;
    }
    if (!strcmp("vkDestroyDevice", pName))
        return (void *) multi1DestroyDevice;
    if (!strcmp("vkCreateSampler", pName))
        return (void *) multi1CreateSampler;
    if (!strcmp("vkCreateGraphicsPipeline", pName))
        return (void *) multi1CreateGraphicsPipeline;
    if (!strcmp("vkStorePipeline", pName))
        return (void *) multi1StorePipeline;
    else {
        VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;
        VkLayerDispatchTable* pTable = device_dispatch_table1(device);
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(device, pName);
    }
}

VK_LAYER_EXPORT void * VKAPI multi1GetInstanceProcAddr(VkInstance inst, const char* pName)
{
    VkBaseLayerObject* instw = (VkBaseLayerObject *) inst;

    if (inst == NULL)
        return NULL;



    if (!strcmp("vkGetInstanceProcAddr", pName)) {
        getLayer1InstanceTable(instw);
        return (void *) multi1GetInstanceProcAddr;
    }
    if (!strcmp("vkDestroyInstance", pName))
        return (void *) multi1DestroyInstance;
    if (!strcmp("GetGlobalExtensionProperties", pName))
        return (void*) vkGetGlobalExtensionProperties;
    if (!strcmp("GetGlobalLayerProperties", pName))
        return (void*) vkGetGlobalLayerProperties;
    if (!strcmp("GetPhysicalDeviceExtensionProperties", pName))
        return (void*) vkGetPhysicalDeviceExtensionProperties;
    if (!strcmp("GetPhysicalDeviceLayerProperties", pName))
        return (void*) vkGetPhysicalDeviceLayerProperties;
    else {
        VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) inst;
        VkLayerInstanceDispatchTable* pTable = instance_dispatch_table1(inst);
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(inst, pName);
    }
}

/******************************** Layer multi2 functions **************************/
static std::unordered_map<void *, VkLayerDispatchTable *> tableMap2;
static std::unordered_map<void *, VkLayerInstanceDispatchTable *> tableInstanceMap2;
static bool layer2_first_activated = false;

// Map lookup must be thread safe
static inline VkLayerDispatchTable *device_dispatch_table2(VkObject object)
{
    VkLayerDispatchTable *pDisp  = *(VkLayerDispatchTable **) object;
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap2.find((void *) pDisp);
    assert(it != tableMap2.end() && "Not able to find device dispatch entry");
    return it->second;
}

static inline VkLayerInstanceDispatchTable *instance_dispatch_table2(VkObject object)
{
    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) object;
    std::unordered_map<void *, VkLayerInstanceDispatchTable *>::const_iterator it = tableInstanceMap2.find((void *) pDisp);
    assert(it != tableInstanceMap2.end() && "Not able to find instance dispatch entry");
    return it->second;
}

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

    printf("At start of wrapped multi2 vkEnumeratePhysicalDevices()\n");
    VkResult result = instance_dispatch_table2(instance)->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    printf("Completed multi2 layer vkEnumeratePhysicalDevices()\n");
    return result;
}

/* hook DextroyDevice to remove tableMap entry */
VK_LAYER_EXPORT VkResult VKAPI multi2DestroyDevice(VkDevice device)
{
    VkLayerDispatchTable *pDisp = *(VkLayerDispatchTable **) device;
    VkResult res = device_dispatch_table2(device)->DestroyDevice(device);
    tableMap2.erase(pDisp);
    return res;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VkResult VKAPI multi2DestroyInstance(VkInstance instance)
{
    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) instance;
    VkResult res = instance_dispatch_table2(instance)->DestroyInstance(instance);
    tableInstanceMap2.erase(pDisp);
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI multi2CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo,
                                                      VkDevice* pDevice)
{
    printf("At start of multi2 vkCreateDevice()\n");
    VkResult result = device_dispatch_table2(*pDevice)->CreateDevice(gpu, pCreateInfo, pDevice);
    printf("Completed multi2 layer vkCreateDevice()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi2CreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo,
                                                             VkCmdBuffer* pCmdBuffer)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;

    printf("At start of multi2 layer vkCreateCommandBuffer()\n");
    VkResult result = device_dispatch_table2(device)->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    printf("Completed multi2 layer vkCreateCommandBuffer()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi2BeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo)
{
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) cmdBuffer;

    printf("At start of multi2 layer vkBeginCommandBuffer()\n");
    VkResult result = device_dispatch_table2(cmdBuffer)->BeginCommandBuffer(cmdBuffer, pBeginInfo);
    printf("Completed multi2 layer vkBeginCommandBuffer()\n");
    return result;

}

VK_LAYER_EXPORT void * VKAPI multi2GetDeviceProcAddr(VkDevice device, const char* pName)
{
    VkBaseLayerObject* devw = (VkBaseLayerObject *) device;

    if (device == NULL)
        return NULL;

    if (!strcmp("vkGetDeviceProcAddr", pName)) {
        getLayer2Table(devw);
        return (void *) multi2GetDeviceProcAddr;
    }
    if (!strcmp("vkCreateDevice", pName))
        return (void *) multi2CreateDevice;
    if (!strcmp("vkDestroyDevice", pName))
        return (void *) multi2DestroyDevice;
    if (!strcmp("vkCreateCommandBuffer", pName))
        return (void *) multi2CreateCommandBuffer;
    else if (!strcmp("vkBeginCommandBuffer", pName))
        return (void *) multi2BeginCommandBuffer;
    else {
        VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;
        VkLayerDispatchTable* pTable = device_dispatch_table2(device);
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(device, pName);
    }
}

VK_LAYER_EXPORT void * VKAPI multi2GetInstanceProcAddr(VkInstance inst, const char* pName)
{
    VkBaseLayerObject* instw = (VkBaseLayerObject *) inst;

    if (inst == NULL)
        return NULL;

    if (!strcmp("vkGetInstanceProcAddr", pName)) {
        getLayer2InstanceTable(instw);
        return (void *) multi2GetInstanceProcAddr;
    }
    if (!strcmp("vkEnumeratePhysicalDevices", pName))
        return (void *) multi2EnumeratePhysicalDevices;
    if (!strcmp("vkDestroyInstance", pName))
        return (void *) multi2DestroyInstance;
    if (!strcmp("GetGlobalExtensionProperties", pName))
        return (void*) vkGetGlobalExtensionProperties;
    if (!strcmp("GetGlobalLayerProperties", pName))
        return (void*) vkGetGlobalLayerProperties;
    if (!strcmp("GetPhysicalDeviceExtensionProperties", pName))
        return (void*) vkGetPhysicalDeviceExtensionProperties;
    if (!strcmp("GetPhysicalDeviceLayerProperties", pName))
        return (void*) vkGetPhysicalDeviceLayerProperties;
    else {
        VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) inst;
        VkLayerInstanceDispatchTable* pTable = instance_dispatch_table2(inst);
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(inst, pName);
    }
}

/********************************* Common functions ********************************/

struct extProps {
    uint32_t version;
    const char * const name;
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* multi does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    /* multi does not have any global layers */
    return util_GetLayerProperties(0, NULL, pCount, pProperties);
}

#define MULTI_LAYER_ARRAY_SIZE 1
static const VkLayerProperties multi_device_layers[MULTI_LAYER_ARRAY_SIZE] = {
    {
        "Multi1",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Sample layer: multi",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* Multi does not have any physical device extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    return util_GetLayerProperties(MULTI_LAYER_ARRAY_SIZE, multi_device_layers,
                                   pCount, pProperties);
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

    layer_initialize_dispatch_table(pTable, devw);
}

static void initLayerInstanceTable(const VkBaseLayerObject *instw, VkLayerInstanceDispatchTable *pTable, const unsigned int layerNum)
{
    if (layerNum == 2 && layer1_first_activated == false)
        layer2_first_activated = true;
    if (layerNum == 1 && layer2_first_activated == false)
        layer1_first_activated = true;

    layer_init_instance_dispatch_table(pTable, instw);
}
