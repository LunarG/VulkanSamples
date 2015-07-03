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
#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_layer.h"
#include "vk_layer_table.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "vk_loader_platform.h"


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

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(
                                               uint32_t extensionIndex,
                                               VkExtensionProperties*    pData)
{
    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    uint32_t *count;

    if (extensionIndex >= BASIC_LAYER_EXT_ARRAY_SIZE)
        return VK_ERROR_INVALID_VALUE;
    memcpy((VkExtensionProperties *) pData, &basicExts[extensionIndex], sizeof(VkExtensionProperties));

    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionCount(uint32_t* pCount)
{
    *pCount = BASIC_LAYER_EXT_ARRAY_SIZE;
    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(
                                            VkInstance instance,
                                            uint32_t* pPhysicalDeviceCount,
                                            VkPhysicalDevice* pPhysicalDevices)
{
    printf("At start of wrapped vkEnumeratePhysicalDevices() call w/ inst: %p\n", (void*)instance);
    VkResult result = instance_dispatch_table(instance)->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    printf("Completed wrapped vkEnumeratePhysicalDevices() call w/ count %u\n", *pPhysicalDeviceCount);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    printf("At start of wrapped vkCreateDevice() call w/ gpu: %p\n", (void*)gpu);
    VkResult result = device_dispatch_table(*pDevice)->CreateDevice(gpu, pCreateInfo, pDevice);
    printf("Completed wrapped vkCreateDevice() call w/ pDevice, Device %p: %p\n", (void*)pDevice, (void *) *pDevice);
    return result;
}

/* hook DestroyDevice to remove tableMap entry */
VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(VkDevice device)
{
    dispatch_key key = get_dispatch_key(device);
    VkResult res = device_dispatch_table(device)->DestroyDevice(device);
    destroy_device_dispatch_table(key);
    return res;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VkResult VKAPI vkDestroyInstance(VkInstance instance)
{
    dispatch_key key = get_dispatch_key(instance);
    VkResult res = instance_dispatch_table(instance)->DestroyInstance(instance);
    destroy_instance_dispatch_table(key);
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceFormatInfo(VkPhysicalDevice gpu, VkFormat format, VkFormatProperties *pFormatInfo)
{
    printf("At start of wrapped vkGetPhysicalDeviceFormatInfo() call w/ gpu: %p\n", (void*)gpu);
    VkResult result = instance_dispatch_table(gpu)->GetPhysicalDeviceFormatInfo(gpu, format, pFormatInfo);
    printf("Completed wrapped vkGetPhysicalDeviceFormatInfo() call w/ gpu: %p\n", (void*)gpu);
    return result;
}

VK_LAYER_EXPORT void * VKAPI vkGetDeviceProcAddr(VkDevice device, const char* pName)
{
    if (device == NULL)
        return NULL;

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp("vkGetDeviceProcAddr", pName)) {
        initDeviceTable((const VkBaseLayerObject *) device);
        return (void *) vkGetDeviceProcAddr;
    }

    if (!strcmp("vkCreateDevice", pName))
        return (void *) vkCreateDevice;
    if (!strcmp("vkDestroyDevice", pName))
        return (void *) vkDestroyDevice;
    if (!strcmp("vkLayerExtension1", pName))
        return (void *) vkLayerExtension1;
    else
    {
        if (device_dispatch_table(device)->GetDeviceProcAddr == NULL)
            return NULL;
        return device_dispatch_table(device)->GetDeviceProcAddr(device, pName);
    }
}

VK_LAYER_EXPORT void * VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* pName)
{
    if (instance == NULL)
        return NULL;

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp("vkGetInstanceProcAddr", pName)) {
        initInstanceTable((const VkBaseLayerObject *) instance);
        return (void *) vkGetInstanceProcAddr;
    }
    if (!strcmp("vkGetPhysicalDeviceFormatInfo", pName))
        return (void *) vkGetPhysicalDeviceFormatInfo;

    if (!strcmp("vkDestroyInstance", pName))
        return (void *) vkDestroyInstance;
    if (!strcmp("vkEnumeratePhysicalDevices", pName))
        return (void*) vkEnumeratePhysicalDevices;
    if (!strcmp("vkGetGlobalExtensionCount", pName))
        return (void*) vkGetGlobalExtensionCount;
    if (!strcmp("vkGetGlobalExtensionProperties", pName))
        return (void*) vkGetGlobalExtensionProperties;
    else
    {
        if (instance_dispatch_table(instance)->GetInstanceProcAddr == NULL)
            return NULL;
        return instance_dispatch_table(instance)->GetInstanceProcAddr(instance, pName);
    }

}
