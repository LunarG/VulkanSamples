/*
 * Vulkan
 *
 * Copyright (C) 2015 LunarG, Inc.
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
 *   Jon Ashburn <jon@lunarg.com>
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 *   Ian Elliott <ian@lunarg.com>
 */

//#define _ISOC11_SOURCE /* for aligned_alloc() */
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "vk_loader_platform.h"
#include "loader.h"
#include "wsi_swapchain.h"

static const VkExtensionProperties wsi_swapchain_extension_info = {
        .extName = VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME,
        .specVersion = VK_EXT_KHR_SWAPCHAIN_REVISION,
};

void wsi_swapchain_add_instance_extensions(
        const struct loader_instance *inst,
        struct loader_extension_list *ext_list)
{
    loader_add_to_ext_list(inst, ext_list, 1, &wsi_swapchain_extension_info);
}

void wsi_swapchain_create_instance(
        struct loader_instance *ptr_instance,
        const VkInstanceCreateInfo *pCreateInfo)
{
    ptr_instance->wsi_swapchain_enabled = false;

    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            ptr_instance->wsi_swapchain_enabled = true;
            return;
        }
    }
}

/*
 * This is the trampoline entrypoint
 * for GetPhysicalDeviceSurfaceSupportKHR
 */
VkResult wsi_swapchain_GetPhysicalDeviceSurfaceSupportKHR(
        VkPhysicalDevice                        physicalDevice,
        uint32_t                                queueNodeIndex,
        const VkSurfaceDescriptionKHR*          pSurfaceDescription,
        VkBool32*                               pSupported)
{
    const VkLayerInstanceDispatchTable *disp;
// TBD/TODO: DO WE NEED TO DO LOCKING FOR THIS FUNCTION?
    disp = loader_get_instance_dispatch(physicalDevice);
//    loader_platform_thread_lock_mutex(&loader_lock);
    VkResult res = disp->GetPhysicalDeviceSurfaceSupportKHR(
                                                      physicalDevice,
                                                      queueNodeIndex,
                                                      pSurfaceDescription,
                                                      pSupported);
//    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

/*
 * This is the instance chain terminator function
 * for GetPhysicalDeviceSurfaceSupportKHR
 */
VkResult VKAPI loader_GetPhysicalDeviceSurfaceSupportKHR(
        VkPhysicalDevice                        physicalDevice,
        uint32_t                                queueNodeIndex,
        const VkSurfaceDescriptionKHR*          pSurfaceDescription,
        VkBool32*                               pSupported)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(physicalDevice, &gpu_index);

    assert(pSupported && "GetPhysicalDeviceSurfaceSupportKHR: Error, null pSupported");
    *pSupported = false;

    assert(icd->GetPhysicalDeviceSurfaceSupportKHR && "loader: null GetPhysicalDeviceSurfaceSupportKHR ICD pointer");

    return icd->GetPhysicalDeviceSurfaceSupportKHR(physicalDevice,
                                                  queueNodeIndex,
                                                  pSurfaceDescription,
                                                  pSupported);
}


void *wsi_swapchain_GetInstanceProcAddr(
        struct loader_instance *ptr_instance,
        const char*                             pName)
{
    if (ptr_instance == VK_NULL_HANDLE || !ptr_instance->wsi_swapchain_enabled) {
        return NULL;
    }

    if (!strcmp(pName, "vkGetPhysicalDeviceSurfaceSupportKHR")) {
        return (void*) wsi_swapchain_GetPhysicalDeviceSurfaceSupportKHR;
    }

    return NULL;
}
