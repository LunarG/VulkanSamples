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
 *   Ian Elliott <ian@lunarg.com>
 */

#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <stdio.h>
#include <string.h>
#include <unordered_map>
#include "vk_loader_platform.h"
#include "vk_layer.h"
#include "vk_layer_config.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"

using namespace std;

// The following is for logging error messages:
typedef struct _layer_data {
    debug_report_data report_data;
    VkDbgMsgCallback logging_callback;
} layer_data;
#define LAYER_NAME (char *) "Swapchain"
#define LOG_ERROR_NON_VALID_OBJ(objType, type, obj)                     \
    log_msg(&mydata.report_data, VK_DBG_REPORT_ERROR_BIT, (objType),    \
            (uint64_t) (obj), 0, 0, LAYER_NAME,                         \
            "%s() called with a non-valid %s.", __FUNCTION__, (obj))

#define LOG_ERROR(objType, type, obj, fmt, ...)                         \
    log_msg(&mydata.report_data, VK_DBG_REPORT_ERROR_BIT, (objType),    \
            (uint64_t) (obj), 0, 0, LAYER_NAME, (fmt), __VA_ARGS__)
#define LOG_PERF_WARNING(objType, type, obj, fmt, ...)                  \
    log_msg(&mydata.report_data, VK_DBG_REPORT_PERF_WARN_BIT, (objType), \
            (uint64_t) (obj), 0, 0, LAYER_NAME, (fmt), __VA_ARGS__)


// NOTE: The following struct's/typedef's are for keeping track of
// info that is used for validating the WSI extensions.

// Forward declarations:
struct _SwpInstance;
struct _SwpPhysicalDevice;
struct _SwpDevice;
struct _SwpSwapchain;
struct _SwpImage;

typedef _SwpInstance SwpInstance;
typedef _SwpPhysicalDevice SwpPhysicalDevice;
typedef _SwpDevice SwpDevice;
typedef _SwpSwapchain SwpSwapchain;
typedef _SwpImage SwpImage;

// Create one of these for each VkInstance:
struct _SwpInstance {
    // The actual handle for this VkInstance:
    VkInstance instance;

    // When vkEnumeratePhysicalDevices is called, the VkPhysicalDevice's are
    // remembered:
    unordered_map<const void*, SwpPhysicalDevice*> physicalDevices;

    // Set to true if "VK_EXT_KHR_swapchain" was enabled for this VkInstance:
    bool swapchainExtensionEnabled;

    // TODO: Add additional booleans for platform-specific extensions:
};

// Create one of these for each VkPhysicalDevice within a VkInstance:
struct _SwpPhysicalDevice {
    // The actual handle for this VkPhysicalDevice:
    VkPhysicalDevice physicalDevice;

    // Corresponding VkDevice (and info) to this VkPhysicalDevice:
    SwpDevice *pDevice;

    // VkInstance that this VkPhysicalDevice is associated with:
    SwpInstance *pInstance;

    // Which queueFamilyIndices support presenting with WSI swapchains:
    unordered_map<uint32_t, VkBool32> queueFamilyIndexSupport;
};

// Create one of these for each VkDevice within a VkInstance:
struct _SwpDevice {
    // The actual handle for this VkDevice:
    VkDevice device;

    // Corresponding VkPhysicalDevice (and info) to this VkDevice:
    SwpPhysicalDevice *pPhysicalDevice;

    // Set to true if "VK_EXT_KHR_device_swapchain" was enabled:
    bool deviceSwapchainExtensionEnabled;

// TODO: Record/use this info per-surface, not per-device, once a
// non-dispatchable surface object is added to WSI:
    // Results of vkGetSurfacePropertiesKHR():
    bool gotSurfaceProperties;
    VkSurfacePropertiesKHR surfaceProperties;

// TODO: Record/use this info per-surface, not per-device, once a
// non-dispatchable surface object is added to WSI:
    // Count and VkSurfaceFormatKHR's returned by vkGetSurfaceFormatsKHR():
    uint32_t surfaceFormatCount;
    VkSurfaceFormatKHR* pSurfaceFormats;

// TODO: Record/use this info per-surface, not per-device, once a
// non-dispatchable surface object is added to WSI:
    // Count and VkPresentModeKHR's returned by vkGetSurfacePresentModesKHR():
    uint32_t presentModeCount;
    VkPresentModeKHR* pPresentModes;

    // When vkCreateSwapchainKHR is called, the VkSwapchainKHR's are
    // remembered:
    unordered_map<uint64_t, SwpSwapchain*> swapchains;
};

// Create one of these for each VkImage within a VkSwapchainKHR:
struct _SwpImage {
    // The actual handle for this VkImage:
    VkImage image;

    // Corresponding VkSwapchainKHR (and info) to this VkImage:
    SwpSwapchain *pSwapchain;

    // true if application got this image from vkAcquireNextImageKHR(), and
    // hasn't yet called vkQueuePresentKHR() for it; otherwise false:
    bool ownedByApp;
};

// Create one of these for each VkSwapchainKHR within a VkDevice:
struct _SwpSwapchain {
    // The actual handle for this VkSwapchainKHR:
    VkSwapchainKHR swapchain;

    // Corresponding VkDevice (and info) to this VkSwapchainKHR:
    SwpDevice *pDevice;

    // When vkGetSwapchainImagesKHR is called, the VkImage's are
    // remembered:
    uint32_t imageCount;
    unordered_map<int, SwpImage> images;
};

#endif // SWAPCHAIN_H
