/*
 *
 * Copyright (C) 2015 Valve Corporation
 * Copyright (C) 2015 Google Inc.
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
 * Author: David Pinedo <david@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 */
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "vk_dispatch_table_helper.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_table.h"
#include "vk_layer_extension_utils.h"

static const VkLayerProperties globalLayerProps[] = {
    {
        "Basic",
        VK_API_VERSION,                 // specVersion
        VK_MAKE_VERSION(0, 1, 0),       // implementationVersion
        "layer: Basic",
    }
};


VK_LAYER_EXPORT VkResult VKAPI vkLayerExtension1(VkDevice device)
{
    printf("In vkLayerExtension1() call w/ device: %p\n", (void*)device);
    printf("vkLayerExtension1 returning SUCCESS\n");
    return VK_SUCCESS;
}

static const VkLayerProperties basic_physicaldevice_layers[] = {
    {
        "Basic",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Sample layer: Basic, implements vkLayerExtension1",
    }
};

/* Must use Vulkan name so that loader finds it */
VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* Mem tracker's physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(basic_physicaldevice_layers), basic_physicaldevice_layers,
                                   pCount, pProperties);
}

static const VkExtensionProperties basic_physicaldevice_extensions[] = {
    {
        "vkLayerExtension1",
        VK_MAKE_VERSION(0, 1, 0),
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice        physicalDevice,
        const char             *pLayerName,
        uint32_t               *pCount,
        VkExtensionProperties  *pProperties)
{
    if (pLayerName == NULL) {
        return instance_dispatch_table(physicalDevice)->EnumerateDeviceExtensionProperties(
                                            physicalDevice,
                                            NULL,
                                            pCount,
                                            pProperties);
    } else {
        return util_GetExtensionProperties(ARRAY_SIZE(basic_physicaldevice_extensions),
                                           basic_physicaldevice_extensions,
                                           pCount, pProperties);
    }
}

VK_LAYER_EXPORT VkResult VKAPI basic_EnumeratePhysicalDevices(
                                            VkInstance instance,
                                            uint32_t* pPhysicalDeviceCount,
                                            VkPhysicalDevice* pPhysicalDevices)
{
    printf("At start of wrapped vkEnumeratePhysicalDevices() call w/ inst: %p\n", (void*)instance);
    VkResult result = instance_dispatch_table(instance)->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    printf("Completed wrapped vkEnumeratePhysicalDevices() call w/ count %u\n", *pPhysicalDeviceCount);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI basic_CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    printf("At start of wrapped vkCreateDevice() call w/ gpu: %p\n", (void*)gpu);
    VkResult result = device_dispatch_table(*pDevice)->CreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    printf("Completed wrapped vkCreateDevice() call w/ pDevice, Device %p: %p\n", (void*)pDevice, (void *) *pDevice);
    return result;
}

/* hook DestroyDevice to remove tableMap entry */
VK_LAYER_EXPORT void VKAPI basic_DestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(device);
    device_dispatch_table(device)->DestroyDevice(device, pAllocator);
    destroy_device_dispatch_table(key);
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT void VKAPI basic_DestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    dispatch_key key = get_dispatch_key(instance);
    instance_dispatch_table(instance)->DestroyInstance(instance, pAllocator);
    destroy_instance_dispatch_table(key);
}

VK_LAYER_EXPORT void VKAPI basic_GetPhysicalDeviceFormatProperties(VkPhysicalDevice gpu, VkFormat format, VkFormatProperties *pFormatInfo)
{
    printf("At start of wrapped vkGetPhysicalDeviceFormatProperties() call w/ gpu: %p\n", (void*)gpu);
    instance_dispatch_table(gpu)->GetPhysicalDeviceFormatProperties(gpu, format, pFormatInfo);
    printf("Completed wrapped vkGetPhysicalDeviceFormatProperties() call w/ gpu: %p\n", (void*)gpu);
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(VkDevice device, const char* pName)
{
    if (device == NULL)
        return NULL;

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp("vkGetDeviceProcAddr", pName)) {
        initDeviceTable((const VkBaseLayerObject *) device);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }

    if (!strcmp("vkCreateDevice", pName))
        return (PFN_vkVoidFunction) basic_CreateDevice;
    if (!strcmp("vkDestroyDevice", pName))
        return (PFN_vkVoidFunction) basic_DestroyDevice;
    if (!strcmp("vkLayerExtension1", pName))
        return (PFN_vkVoidFunction) vkLayerExtension1;
    else
    {
        if (device_dispatch_table(device)->GetDeviceProcAddr == NULL)
            return NULL;
        return device_dispatch_table(device)->GetDeviceProcAddr(device, pName);
    }
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* pName)
{
    if (instance == NULL)
        return NULL;

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp("vkGetInstanceProcAddr", pName)) {
        initInstanceTable((const VkBaseLayerObject *) instance);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }

    if (!strcmp("vkEnumerateDeviceLayerProperties", pName))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp("vkEnumerateDeviceExtensionProperties", pName))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;
    if (!strcmp("vkGetPhysicalDeviceFormatProperties", pName))
        return (PFN_vkVoidFunction) basic_GetPhysicalDeviceFormatProperties;
    if (!strcmp("vkDestroyInstance", pName))
        return (PFN_vkVoidFunction) basic_DestroyInstance;
    if (!strcmp("vkEnumeratePhysicalDevices", pName))
        return (PFN_vkVoidFunction) basic_EnumeratePhysicalDevices;

    if (instance_dispatch_table(instance)->GetInstanceProcAddr == NULL)
        return NULL;
    return instance_dispatch_table(instance)->GetInstanceProcAddr(instance, pName);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,  VkExtensionProperties* pProperties)
{
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceLayerProperties(uint32_t *pCount,  VkLayerProperties* pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(globalLayerProps), globalLayerProps, pCount, pProperties);
}
