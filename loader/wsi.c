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
 * Author: Ian Elliott <ian@lunarg.com>
 *
 */

//#define _ISOC11_SOURCE /* for aligned_alloc() */
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include "vk_loader_platform.h"
#include "loader.h"
#include "wsi.h"

static const VkExtensionProperties wsi_surface_extension_info = {
        .extensionName = VK_KHR_SURFACE_EXTENSION_NAME,
        .specVersion = VK_KHR_SURFACE_REVISION,
};

#ifdef _WIN32
static const VkExtensionProperties wsi_win32_surface_extension_info = {
        .extensionName = VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        .specVersion = VK_KHR_WIN32_SURFACE_REVISION,
};
#else // _WIN32
static const VkExtensionProperties wsi_mir_surface_extension_info = {
        .extensionName = VK_KHR_MIR_SURFACE_EXTENSION_NAME,
        .specVersion = VK_KHR_MIR_SURFACE_REVISION,
};

static const VkExtensionProperties wsi_wayland_surface_extension_info = {
        .extensionName = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
        .specVersion = VK_KHR_WAYLAND_SURFACE_REVISION,
};

static const VkExtensionProperties wsi_xcb_surface_extension_info = {
        .extensionName = VK_KHR_XCB_SURFACE_EXTENSION_NAME,
        .specVersion = VK_KHR_XCB_SURFACE_REVISION,
};

static const VkExtensionProperties wsi_xlib_surface_extension_info = {
        .extensionName = VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        .specVersion = VK_KHR_XLIB_SURFACE_REVISION,
};
#endif // _WIN32

void wsi_add_instance_extensions(
        const struct loader_instance *inst,
        struct loader_extension_list *ext_list)
{
    loader_add_to_ext_list(inst, ext_list, 1, &wsi_surface_extension_info);
#ifdef _WIN32
    loader_add_to_ext_list(inst, ext_list, 1, &wsi_win32_surface_extension_info);
#else // _WIN32
    loader_add_to_ext_list(inst, ext_list, 1, &wsi_mir_surface_extension_info);
    loader_add_to_ext_list(inst, ext_list, 1, &wsi_wayland_surface_extension_info);
    loader_add_to_ext_list(inst, ext_list, 1, &wsi_xcb_surface_extension_info);
    loader_add_to_ext_list(inst, ext_list, 1, &wsi_xlib_surface_extension_info);
#endif // _WIN32
}

void wsi_create_instance(
        struct loader_instance *ptr_instance,
        const VkInstanceCreateInfo *pCreateInfo)
{
    ptr_instance->wsi_enabled = false;
#ifdef _WIN32
    ptr_instance->wsi_surface_enabled = true;
#else // _WIN32
    ptr_instance->wsi_mir_surface_enabled = false;
    ptr_instance->wsi_wayland_surface_enabled = false;
    ptr_instance->wsi_xcb_surface_enabled = false;
    ptr_instance->wsi_xlib_surface_enabled = false;
#endif // _WIN32
    ptr_instance->wsi_swapchain_enabled = false;

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            ptr_instance->wsi_surface_enabled = true;
        }
#ifdef _WIN32
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
            ptr_instance->wsi_surface_enabled = true;
        }
#else // _WIN32
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0) {
            ptr_instance->wsi_mir_surface_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0) {
            ptr_instance->wsi_wayland_surface_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0) {
            ptr_instance->wsi_xcb_surface_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0) {
            ptr_instance->wsi_xlib_surface_enabled = true;
        }
#endif // _WIN32
    }
}

/*
 * This is the trampoline entrypoint
 * for GetPhysicalDeviceSurfaceSupportKHR
 */
VKAPI_ATTR VkResult VKAPI_CALL wsi_swapchain_GetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice                            physicalDevice,
    uint32_t                                    queueFamilyIndex,
    VkSurfaceKHR                                surface,
    VkBool32*                                   pSupported)
{
    const VkLayerInstanceDispatchTable *disp;
    disp = loader_get_instance_dispatch(physicalDevice);
    VkResult res = disp->GetPhysicalDeviceSurfaceSupportKHR(
                                                      physicalDevice,
                                                      queueFamilyIndex,
                                                      surface,
                                                      pSupported);
    return res;
}

/*
 * This is the instance chain terminator function
 * for GetPhysicalDeviceSurfaceSupportKHR
 */
VKAPI_ATTR VkResult VKAPI_CALL loader_GetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice                            physicalDevice,
    uint32_t                                    queueFamilyIndex,
    VkSurfaceKHR                                surface,
    VkBool32*                                   pSupported)
{
    struct loader_physical_device *phys_dev = (struct loader_physical_device *) physicalDevice;
    struct loader_icd *icd = phys_dev->this_icd;

    assert(pSupported && "GetPhysicalDeviceSurfaceSupportKHR: Error, null pSupported");
    *pSupported = false;

    assert(icd->GetPhysicalDeviceSurfaceSupportKHR && "loader: null GetPhysicalDeviceSurfaceSupportKHR ICD pointer");

    return icd->GetPhysicalDeviceSurfaceSupportKHR(phys_dev->phys_dev,
                                                   queueFamilyIndex,
                                                   surface,
                                                   pSupported);
}

bool wsi_swapchain_instance_gpa(struct loader_instance *ptr_instance,
                                 const char* name, void **addr)
{
    *addr = NULL;

    if (!strcmp("vkGetPhysicalDeviceSurfaceSupportKHR", name)) {
        *addr = ptr_instance->wsi_swapchain_enabled ? (void *) wsi_swapchain_GetPhysicalDeviceSurfaceSupportKHR : NULL;
        return true;
    }
    return false;
}
