/*
 *
 * Copyright (C) 2015-2016 Valve Corporation
 * Copyright (C) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Jon Ashburn <jon@lunarg.com>
 *
 */

#pragma once
#include <unordered_map>
#include "vulkan/vk_layer.h"


struct devExts {
    bool wsi_swapchain_enabled;
};
struct instExts {
    bool wsi_surface_enabled;
    bool wsi_surf_xcb_enabled;
    bool wsi_surf_xlib_enabled;
    bool wsi_surf_mir_enabled;
    bool wsi_surf_wayland_enabled;
    bool wsi_surf_win32_enabled;
    bool debug_report_enabled;
};
//TODO remove this maps and use the wrapped object Vkdevice object instead
static std::unordered_map<void *, struct devExts> deviceExtMap;

struct wrapped_phys_dev_obj {
    VkLayerInstanceDispatchTable *loader_disp;
    struct wrapped_inst_obj *inst;  // parent instance object
    void *obj;
};

struct wrapped_inst_obj {
    VkLayerInstanceDispatchTable *loader_disp;
    instExts exts;
    VkLayerInstanceDispatchTable layer_disp;    //this layer's dispatch table
    PFN_vkSetInstanceLoaderData pfn_inst_init;
    struct wrapped_phys_dev_obj *ptr_phys_devs; // any enumerated phys devs
    VkInstance obj;
};

struct wrapped_dev_obj {
    VkLayerDispatchTable *disp;
    devExts exts; //TODO use this
    VkLayerInstanceDispatchTable *layer_disp;  // TODO use this
    PFN_vkSetDeviceLoaderData pfn_dev_init;  //TODO use this
    void *obj;
};

static inline VkInstance unwrap_instance(const VkInstance instance,  wrapped_inst_obj **inst) {
   *inst = reinterpret_cast<wrapped_inst_obj *> (instance);
   return (*inst)->obj;
}

static inline VkPhysicalDevice unwrap_phys_dev(const VkPhysicalDevice physical_device,  wrapped_phys_dev_obj **phys_dev) {
   *phys_dev = reinterpret_cast<wrapped_phys_dev_obj *> (physical_device);
   return reinterpret_cast <VkPhysicalDevice> ((*phys_dev)->obj);
}

static void create_device_register_extensions(const VkDeviceCreateInfo *pCreateInfo, VkDevice device) {
    uint32_t i;
    VkLayerDispatchTable *pDisp = device_dispatch_table(device);
    PFN_vkGetDeviceProcAddr gpa = pDisp->GetDeviceProcAddr;
    pDisp->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR = (PFN_vkQueuePresentKHR)gpa(device, "vkQueuePresentKHR");

    deviceExtMap[pDisp].wsi_swapchain_enabled = false;
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            deviceExtMap[pDisp].wsi_swapchain_enabled = true;
    }
}

static void create_instance_register_extensions(const VkInstanceCreateInfo *pCreateInfo, VkInstance instance, struct wrapped_inst_obj *inst) {
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp = &inst->layer_disp;
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;

    // KHR_surface
    pDisp->DestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)gpa(instance, "vkDestroySurfaceKHR");
    pDisp->GetPhysicalDeviceSurfaceSupportKHR =
        (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    pDisp->GetPhysicalDeviceSurfaceCapabilitiesKHR =
        (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)gpa(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pDisp->GetPhysicalDeviceSurfaceFormatsKHR =
        (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)gpa(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    pDisp->GetPhysicalDeviceSurfacePresentModesKHR =
        (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)gpa(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

    // KHR_XXX_surface
#ifdef VK_USE_PLATFORM_XCB_KHR
    pDisp->CreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)gpa(instance, "vkCreateXcbSurfaceKHR");
    pDisp->GetPhysicalDeviceXcbPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
    pDisp->CreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)gpa(instance, "vkCreateXlibSurfaceKHR");
    pDisp->GetPhysicalDeviceXlibPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XLIB_KHR

#ifdef VK_USE_PLATFORM_MIR_KHR
    pDisp->CreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR)gpa(instance, "vkCreateMirSurfaceKHR");
    pDisp->GetPhysicalDeviceMirPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif // VK_USE_PLATFORM_MIR_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    pDisp->CreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)gpa(instance, "vkCreateWaylandSurfaceKHR");
    pDisp->GetPhysicalDeviceWaylandPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif // VK_USE_PLATFORM_WAYLAND_KHR

#ifdef VK_USE_PLATFORM_WIN32_KHR
    pDisp->CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)gpa(instance, "vkCreateWin32SurfaceKHR");
    pDisp->GetPhysicalDeviceWin32PresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif // VK_USE_PLATFORM_WIN32_KHR

    //EXT_debug_report
    pDisp->CreateDebugReportCallbackEXT =
        (PFN_vkCreateDebugReportCallbackEXT)gpa(instance, "vkCreateDebugReportCallbackEXT");
    pDisp->DestroyDebugReportCallbackEXT =
        (PFN_vkDestroyDebugReportCallbackEXT)gpa(instance, "vkDestroyDebugReportCallbackEXT");
    pDisp->DebugReportMessageEXT =
        (PFN_vkDebugReportMessageEXT)gpa(instance, "vkDebugReportMessageEXT");


    inst->exts.wsi_surface_enabled = false;
    inst->exts.wsi_surf_xcb_enabled = false;
    inst->exts.wsi_surf_xlib_enabled = false;
    inst->exts.wsi_surf_mir_enabled = false;
    inst->exts.wsi_surf_wayland_enabled = false;
    inst->exts.wsi_surf_win32_enabled = false;
    inst->exts.debug_report_enabled = false;
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0)
            inst->exts.wsi_surface_enabled = true;
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
            inst->exts.debug_report_enabled = true;
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0)
            inst->exts.wsi_surf_xcb_enabled = true;
#endif  // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0)
            inst->exts.wsi_surf_xlib_enabled = true;

#endif  // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0)
            inst->exts.wsi_surf_wayland_enabled = true;

#endif  // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0)
            inst->exts.wsi_surf_mir_enabled = true;

#endif  // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0)
            inst->exts.wsi_surf_win32_enabled = true;
#endif  // VK_USE_PLATFORM_WIN32_KHR

    }
}
