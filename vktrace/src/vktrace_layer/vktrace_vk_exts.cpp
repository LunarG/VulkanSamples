/*
 *
 * Copyright (C) 2015 Valve Corporation
 * All Rights Reserved.
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
 */
#include "vktrace_lib_helpers.h"
#include "vulkan/vk_lunarg_debug_marker.h"
#include "vulkan/vk_lunarg_debug_report.h"

#include "vulkan/vk_layer.h"

void ext_init_create_instance(
        layer_instance_data             *instData,
        VkInstance                      inst,
        uint32_t                        extension_count,
        const char*const*               ppEnabledExtensions)    // extension names to be enabled
{
    PFN_vkGetInstanceProcAddr gpa = instData->instTable.GetInstanceProcAddr;

    instData->instTable.DbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback) gpa(inst, "vkDbgCreateMsgCallback");
    instData->instTable.DbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback) gpa(inst, "vkDbgDestroyMsgCallback");
    instData->instTable.GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) gpa(inst, "vkGetPhysicalDeviceSurfaceSupportKHR");
    instData->instTable.DestroySurfaceKHR = (PFN_vkDestroySurfaceKHR) gpa(inst, "vkDestroySurfaceKHR");
    instData->instTable.GetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) gpa(inst, "vkGetPhysicalDeviceSurfaceCapaabilitiesKHR");
    instData->instTable.GetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) gpa(inst, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    instData->instTable.GetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) gpa(inst, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    instData->LunargDebugReportEnabled = false;
    instData->KHRSurfaceEnabled = false;
    instData->KHRXlibSurfaceEnabled = false;
    instData->KHRXcbSurfaceEnabled = false;
    instData->KHRWaylandSurfaceEnabled = false;
    instData->KHRMirSurfaceEnabled = false;
    instData->KHRWin32SurfaceEnabled = false;
    for (uint32_t i = 0; i < extension_count; i++) {
        if (strcmp(ppEnabledExtensions[i], VK_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            instData->LunargDebugReportEnabled = true;
        }
        if (strcmp(ppEnabledExtensions[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            instData->KHRSurfaceEnabled = true;
        }
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(ppEnabledExtensions[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0) {
            instData->KHRXlibSurfaceEnabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(ppEnabledExtensions[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0) {
            instData->KHRXcbSurfaceEnabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(ppEnabledExtensions[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0) {
            instData->KHRMirSurfaceEnabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(ppEnabledExtensions[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0) {
            instData->KHRWaylandSurfaceEnabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(ppEnabledExtensions[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
            instData->KHRWin32SurfaceEnabled = true;
        }
#endif
    }
}

void ext_init_create_device(
        layer_device_data               *devData,
        VkDevice                        dev,
        uint32_t                        extension_count,
        const char*const*               ppEnabledExtensions)  // extension names to be enabled
{
    PFN_vkGetDeviceProcAddr gpa = devData->devTable.GetDeviceProcAddr;
   
    devData->devTable.CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) gpa(dev, "vkCreateSwapchainKHR");
    devData->devTable.DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) gpa(dev, "vkDestroySwapchainKHR");
    devData->devTable.GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) gpa(dev, "vkGetSwapchainImagesKHR");
    devData->devTable.AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) gpa(dev, "vkAcquireNextImageKHR");
    devData->devTable.QueuePresentKHR = (PFN_vkQueuePresentKHR) gpa(dev, "vkQueuePresentKHR");
    
    devData->debugMarkerTable.CmdDbgMarkerBegin = (PFN_vkCmdDbgMarkerBegin) gpa(dev, "vkCmdDbgMarkerBegin");
    devData->debugMarkerTable.CmdDbgMarkerEnd = (PFN_vkCmdDbgMarkerEnd) gpa(dev, "vkCmdDbgMarkerEnd");
    devData->debugMarkerTable.DbgSetObjectName = (PFN_vkDbgSetObjectName) gpa(dev, "vkDbgSetObjectName");
    devData->debugMarkerTable.DbgSetObjectTag = (PFN_vkDbgSetObjectTag) gpa(dev, "vkDbgSetObjectTag");

    devData->LunargDebugMarkerEnabled = false;
    devData->KHRDeviceSwapchainEnabled = false;
    for (uint32_t i = 0; i < extension_count; i++) {
        if (strcmp(ppEnabledExtensions[i], DEBUG_MARKER_EXTENSION_NAME) == 0) {
            devData->LunargDebugMarkerEnabled = true;
        }
        if (strcmp(ppEnabledExtensions[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            devData->KHRDeviceSwapchainEnabled = true;
        }
    }
}
