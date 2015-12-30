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
 * Author: Ian Elliott <ian@lunarg.com>
 *
 */

#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include "vulkan/vk_layer.h"
#include "vulkan/vk_ext_debug_report.h"
#include "vk_layer_config.h"
#include "vk_layer_logging.h"
#include <vector>
#include <unordered_map>

static const VkLayerProperties globalLayerProps[] = {
    {
        "VK_LAYER_LUNARG_swapchain",
        VK_API_VERSION,                 // specVersion
        VK_MAKE_VERSION(0, 1, 0),       // implementationVersion
        "layer: swapchain",
    }
};

static const VkLayerProperties deviceLayerProps[] = {
    {
        "VK_LAYER_LUNARG_swapchain",
        VK_API_VERSION,                 // specVersion
        VK_MAKE_VERSION(0, 1, 0),       // implementationVersion
        "layer: swapchain",
    }
};


using namespace std;


// Swapchain ERROR codes
typedef enum _SWAPCHAIN_ERROR
{
    SWAPCHAIN_INVALID_HANDLE,                   // Handle used that isn't currently valid
    SWAPCHAIN_NULL_POINTER,                     // Pointer set to NULL, instead of being a valid pointer
    SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,         // Did not enable WSI extension, but called WSI function 
    SWAPCHAIN_DEL_DEVICE_BEFORE_SWAPCHAINS,     // Called vkDestroyDevice() before vkDestroySwapchainKHR()
    SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,        // Called vkCreateSwapchainKHR() without calling a query (e.g. vkGetPhysicalDeviceSurfaceCapabilitiesKHR())
    SWAPCHAIN_CREATE_SWAP_BAD_MIN_IMG_COUNT,    // Called vkCreateSwapchainKHR() with out-of-bounds minImageCount
    SWAPCHAIN_CREATE_SWAP_OUT_OF_BOUNDS_EXTENTS,// Called vkCreateSwapchainKHR() with out-of-bounds imageExtent
    SWAPCHAIN_CREATE_SWAP_EXTENTS_NO_MATCH_WIN, // Called vkCreateSwapchainKHR() with imageExtent that doesn't match window's extent
    SWAPCHAIN_CREATE_SWAP_BAD_PRE_TRANSFORM,    // Called vkCreateSwapchainKHR() with a non-supported preTransform
    SWAPCHAIN_CREATE_SWAP_BAD_COMPOSITE_ALPHA,  // Called vkCreateSwapchainKHR() with a non-supported compositeAlpha
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_ARRAY_SIZE,   // Called vkCreateSwapchainKHR() with a non-supported imageArraySize
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_USAGE_FLAGS,  // Called vkCreateSwapchainKHR() with a non-supported imageUsageFlags
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_COLOR_SPACE,  // Called vkCreateSwapchainKHR() with a non-supported imageColorSpace
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_FORMAT,       // Called vkCreateSwapchainKHR() with a non-supported imageFormat
    SWAPCHAIN_CREATE_SWAP_BAD_IMG_FMT_CLR_SP,   // Called vkCreateSwapchainKHR() with a non-supported imageColorSpace
    SWAPCHAIN_CREATE_SWAP_BAD_PRESENT_MODE,     // Called vkCreateSwapchainKHR() with a non-supported presentMode
    SWAPCHAIN_CREATE_SWAP_BAD_SHARING_MODE,     // Called vkCreateSwapchainKHR() with a non-supported imageSharingMode
    SWAPCHAIN_CREATE_SWAP_BAD_SHARING_VALUES,   // Called vkCreateSwapchainKHR() with bad values when imageSharingMode is VK_SHARING_MODE_CONCURRENT
    SWAPCHAIN_DESTROY_SWAP_DIFF_DEVICE,         // Called vkDestroySwapchainKHR() with a different VkDevice than vkCreateSwapchainKHR()
    SWAPCHAIN_APP_OWNS_TOO_MANY_IMAGES,         // vkAcquireNextImageKHR() asked for more images than are available
    SWAPCHAIN_INDEX_TOO_LARGE,                  // Index is too large for swapchain
    SWAPCHAIN_INDEX_NOT_IN_USE,                 // vkQueuePresentKHR() given index that is not owned by app
    SWAPCHAIN_BAD_BOOL,                         // VkBool32 that doesn't have value of VK_TRUE or VK_FALSE (e.g. is a non-zero form of true)
    SWAPCHAIN_INVALID_COUNT,                    // Second time a query called, the pCount value didn't match first time
} SWAPCHAIN_ERROR;


// The following is for logging error messages:
#define LAYER_NAME (char *) "Swapchain"
#define LOG_ERROR_NON_VALID_OBJ(objType, type, obj)                     \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), __LINE__, SWAPCHAIN_INVALID_HANDLE, LAYER_NAME, \
                "%s() called with a non-valid %s.", __FUNCTION__, (obj)) \
    : VK_FALSE
#define LOG_ERROR_NULL_POINTER(objType, type, obj)                     \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), 0, SWAPCHAIN_NULL_POINTER, LAYER_NAME, \
                "%s() called with NULL pointer %s.", __FUNCTION__, (obj)) \
    : VK_FALSE
#define LOG_ERROR_INVALID_COUNT(objType, type, obj, obj2, val)          \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), 0, SWAPCHAIN_INVALID_COUNT, LAYER_NAME, \
                "%s() called with non-NULL %s, and with %s not matching " \
                "the value (%d) that was returned when %s was NULL.",   \
                __FUNCTION__, (obj2), (obj), (val), (obj2))             \
    : VK_FALSE
#define LOG_ERROR(objType, type, obj, enm, fmt, ...)                    \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (objType), \
                (uint64_t) (obj), __LINE__, (enm), LAYER_NAME, (fmt), __VA_ARGS__) \
    : VK_FALSE
#define LOG_PERF_WARNING(objType, type, obj, enm, fmt, ...)             \
    (my_data) ?                                                         \
        log_msg(my_data->report_data, VK_DEBUG_REPORT_PERF_WARN_BIT_EXT, (objType), \
                (uint64_t) (obj), __LINE__, (enm), LAYER_NAME, (fmt), __VA_ARGS__) \
    : VK_FALSE


// NOTE: The following struct's/typedef's are for keeping track of
// info that is used for validating the WSI extensions.

// Forward declarations:
struct _SwpInstance;
struct _SwpSurface;
struct _SwpPhysicalDevice;
struct _SwpDevice;
struct _SwpSwapchain;
struct _SwpImage;

typedef _SwpInstance SwpInstance;
typedef _SwpSurface SwpSurface;
typedef _SwpPhysicalDevice SwpPhysicalDevice;
typedef _SwpDevice SwpDevice;
typedef _SwpSwapchain SwpSwapchain;
typedef _SwpImage SwpImage;

// Create one of these for each VkInstance:
struct _SwpInstance {
    // The actual handle for this VkInstance:
    VkInstance instance;

    // Remember the VkSurfaceKHR's that are created for this VkInstance:
    unordered_map<const void*, SwpSurface*> surfaces;

    // When vkEnumeratePhysicalDevices is called, the VkPhysicalDevice's are
    // remembered:
    unordered_map<const void*, SwpPhysicalDevice*> physicalDevices;

    // Set to true if VK_KHR_SURFACE_EXTENSION_NAME was enabled for this VkInstance:
    bool surfaceExtensionEnabled;

    // TODO: Add additional booleans for platform-specific extensions:
};

// Create one of these for each VkSurfaceKHR:
struct _SwpSurface {
    // The actual handle for this VkSurfaceKHR:
    VkSurfaceKHR surface;

    // VkInstance that this VkSurfaceKHR is associated with:
    SwpInstance *pInstance;

    // Which platform this VkSurfaceKHR is associated with:
    VkIcdWsiPlatform platform;

    // TODO: Add additional platform-specific info:
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

// TODO: Record/use this info per-surface, not per-device, once a
// non-dispatchable surface object is added to WSI:
    // Results of vkGetPhysicalDeviceSurfaceCapabilitiesKHR():
    bool gotSurfaceCapabilities;
    VkSurfaceCapabilitiesKHR surfaceCapabilities;

// TODO: Record/use this info per-surface, not per-device, once a
// non-dispatchable surface object is added to WSI:
    // Count and VkSurfaceFormatKHR's returned by vkGetPhysicalDeviceSurfaceFormatsKHR():
    uint32_t surfaceFormatCount;
    VkSurfaceFormatKHR* pSurfaceFormats;

// TODO: Record/use this info per-surface, not per-device, once a
// non-dispatchable surface object is added to WSI:
    // Count and VkPresentModeKHR's returned by vkGetPhysicalDeviceSurfacePresentModesKHR():
    uint32_t presentModeCount;
    VkPresentModeKHR* pPresentModes;
};

// Create one of these for each VkDevice within a VkInstance:
struct _SwpDevice {
    // The actual handle for this VkDevice:
    VkDevice device;

    // Corresponding VkPhysicalDevice (and info) to this VkDevice:
    SwpPhysicalDevice *pPhysicalDevice;

    // Set to true if VK_KHR_SWAPCHAIN_EXTENSION_NAME was enabled:
    bool swapchainExtensionEnabled;

    // When vkCreateSwapchainKHR is called, the VkSwapchainKHR's are
    // remembered:
    unordered_map<VkSwapchainKHR, SwpSwapchain*> swapchains;
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

struct layer_data {
    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerDispatchTable* device_dispatch_table;
    VkLayerInstanceDispatchTable* instance_dispatch_table;
    // NOTE: The following are for keeping track of info that is used for
    // validating the WSI extensions.
    std::unordered_map<void *, SwpInstance>       instanceMap;
    std::unordered_map<void *, SwpSurface>        surfaceMap;
    std::unordered_map<void *, SwpPhysicalDevice> physicalDeviceMap;
    std::unordered_map<void *, SwpDevice>         deviceMap;
    std::unordered_map<VkSwapchainKHR, SwpSwapchain>    swapchainMap;

    layer_data() :
        report_data(nullptr),
        device_dispatch_table(nullptr),
        instance_dispatch_table(nullptr)
    {};
};

#endif // SWAPCHAIN_H
