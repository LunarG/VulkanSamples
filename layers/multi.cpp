/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Jon Ashburn <jon@lunarg.com>
 *
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>
#include "vk_loader_platform.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_table.h"

#ifdef __cplusplus
extern "C" {
#endif


static device_table_map multi1_device_table_map;
/******************************** Layer multi1 functions **************************/

/* hook DestroyDevice to remove tableMap entry */
VK_LAYER_EXPORT void VKAPI multi1DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    VkLayerDispatchTable *pDisp = get_dispatch_table(multi1_device_table_map, device);
    dispatch_key key = get_dispatch_key(device);

    printf("At start of multi1 layer vkDestroyDevice()\n");
    pDisp->DestroyDevice(device, pAllocator);
    multi1_device_table_map.erase(key);
    printf("Completed multi1 layer vkDestroyDevice()\n");
}

VK_LAYER_EXPORT VkResult VKAPI multi1CreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler)
{
    VkLayerDispatchTable *pDisp = get_dispatch_table(multi1_device_table_map, device);

    printf("At start of multi1 layer vkCreateSampler()\n");
    VkResult result = pDisp->CreateSampler(device, pCreateInfo, pAllocator, pSampler);
    printf("Completed multi1 layer vkCreateSampler()\n");
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI multi1CreateGraphicsPipelines(
                                VkDevice device,
                                VkPipelineCache pipelineCache,
                                uint32_t count,
                                const VkGraphicsPipelineCreateInfo* pCreateInfos,
                                const VkAllocationCallbacks* pAllocator,
                                VkPipeline* pPipelines)
{
    VkLayerDispatchTable *pDisp = get_dispatch_table(multi1_device_table_map, device);

    printf("At start of multi1 layer vkCreateGraphicsPipeline()\n");
    VkResult result = pDisp->CreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
    printf("Completed multi1 layer vkCreateGraphicsPipeline()\n");
    return result;
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI multi1GetDeviceProcAddr(VkDevice device, const char* pName)
{

    if (device == NULL)
        return NULL;

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(pName, "multi1GetDeviceProcAddr") || !strcmp(pName, "vkGetDeviceProcAddr")) {
        initDeviceTable(multi1_device_table_map, (const VkBaseLayerObject *) device);
        return (PFN_vkVoidFunction) multi1GetDeviceProcAddr;
    }

    if (!strcmp("vkDestroyDevice", pName))
        return (PFN_vkVoidFunction) multi1DestroyDevice;
    if (!strcmp("vkCreateSampler", pName))
        return (PFN_vkVoidFunction) multi1CreateSampler;
    if (!strcmp("vkCreateGraphicsPipelines", pName))
        return (PFN_vkVoidFunction) multi1CreateGraphicsPipelines;
    else {
        VkLayerDispatchTable *pTable = get_dispatch_table(multi1_device_table_map, device);
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(device, pName);
    }
}


static instance_table_map multi2_instance_table_map;
/******************************** Layer multi2 functions **************************/
VK_LAYER_EXPORT VkResult VKAPI multi2EnumeratePhysicalDevices(
                                            VkInstance instance,
                                            uint32_t* pPhysicalDeviceCount,
                                            VkPhysicalDevice* pPhysicalDevices)
{
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(multi2_instance_table_map, instance);

    printf("At start of wrapped multi2 vkEnumeratePhysicalDevices()\n");
    VkResult result = pDisp->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    printf("Completed multi2 layer vkEnumeratePhysicalDevices()\n");
    return result;
}

VK_LAYER_EXPORT void VKAPI multi2GetPhysicalDeviceProperties(
                                        VkPhysicalDevice physicalDevice,
                                        VkPhysicalDeviceProperties* pProperties)
{
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(multi2_instance_table_map, physicalDevice);
    printf("At start of wrapped multi2 vkGetPhysicalDeviceProperties()\n");
    pDisp->GetPhysicalDeviceProperties(physicalDevice, pProperties);
    printf("Completed multi2 layer vkGetPhysicalDeviceProperties()\n");
}

VK_LAYER_EXPORT void VKAPI multi2GetPhysicalDeviceFeatures(
                                        VkPhysicalDevice physicalDevice,
                                        VkPhysicalDeviceFeatures* pFeatures)
{
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(multi2_instance_table_map, physicalDevice);
    printf("At start of wrapped multi2 vkGetPhysicalDeviceFeatures()\n");
    pDisp->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);
    printf("Completed multi2 layer vkGetPhysicalDeviceFeatures()\n");
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT void VKAPI multi2DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(multi2_instance_table_map, instance);
    dispatch_key key = get_dispatch_key(instance);

    printf("At start of wrapped multi2 vkDestroyInstance()\n");
    pDisp->DestroyInstance(instance, pAllocator);
    multi2_instance_table_map.erase(key);
    printf("Completed multi2 layer vkDestroyInstance()\n");
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI multi2GetInstanceProcAddr(VkInstance inst, const char* pName)
{
    if (inst == NULL)
        return NULL;

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(pName, "multi2GetInstanceProcAddr") || !strcmp(pName, "vkGetInstanceProcAddr")) {
        initInstanceTable(multi2_instance_table_map, (const VkBaseLayerObject *) inst);
        return (PFN_vkVoidFunction) multi2GetInstanceProcAddr;
    }

    if (!strcmp("vkEnumeratePhysicalDevices", pName))
        return (PFN_vkVoidFunction) multi2EnumeratePhysicalDevices;
    if (!strcmp("GetPhysicalDeviceProperties", pName))
        return (PFN_vkVoidFunction) multi2GetPhysicalDeviceProperties;
    if (!strcmp("GetPhysicalDeviceFeatures", pName))
        return (PFN_vkVoidFunction) multi2GetPhysicalDeviceFeatures;
    if (!strcmp("vkDestroyInstance", pName))
        return (PFN_vkVoidFunction) multi2DestroyInstance;
    else {
        VkLayerInstanceDispatchTable *pTable = get_dispatch_table(multi2_instance_table_map, inst);
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(inst, pName);
    }
}

#ifdef __cplusplus
}    //extern "C"
#endif

