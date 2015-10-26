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
 * Authors:
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#ifndef GENERIC_H
#define GENERIC_H
#include "vk_layer.h"

/*
 * This file contains static functions for the generated layer Generic
 */

// The following is for logging error messages:
struct layer_data {
    debug_report_data *report_data;
    VkDbgMsgCallback logging_callback;

    layer_data() :
        report_data(nullptr),
        logging_callback(nullptr)
    {};
};

static const VkLayerProperties globalLayerProps[] = {
    {
        "Generic",
        VK_API_VERSION,                 // specVersion
        VK_MAKE_VERSION(0, 1, 0),       // implVersion
        "layer: Generic",
    }
};

static const VkLayerProperties deviceLayerProps[] = {
    {
        "Generic",
        VK_API_VERSION,                 // specVersion
        VK_MAKE_VERSION(0, 1, 0),       // implVersion
        "layer: Generic",
    }
};

struct devExts {
    bool wsi_enabled;
};
struct instExts {
    bool wsi_enabled;
};
static std::unordered_map<void *, struct devExts>     deviceExtMap;
static std::unordered_map<void *, struct instExts>    instanceExtMap;

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i;
    VkLayerDispatchTable *pDisp  = device_dispatch_table(device);
    PFN_vkGetDeviceProcAddr gpa = pDisp->GetDeviceProcAddr;
    pDisp->GetSurfacePropertiesKHR = (PFN_vkGetSurfacePropertiesKHR) gpa(device, "vkGetSurfacePropertiesKHR");
    pDisp->GetSurfaceFormatsKHR = (PFN_vkGetSurfaceFormatsKHR) gpa(device, "vkGetSurfaceFormatsKHR");
    pDisp->GetSurfacePresentModesKHR = (PFN_vkGetSurfacePresentModesKHR) gpa(device, "vkGetSurfacePresentModesKHR");
    pDisp->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");

    deviceExtMap[pDisp].wsi_enabled = false;
    for (i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME) == 0)
            deviceExtMap[pDisp].wsi_enabled = true;

    }
}

static void createInstanceRegisterExtensions(const VkInstanceCreateInfo* pCreateInfo, VkInstance instance)
{
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp  = instance_dispatch_table(instance);
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;
    pDisp->GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    instanceExtMap[pDisp].wsi_enabled = false;
    for (i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].wsi_enabled = true;

    }
}
#endif // GENERIC_H

