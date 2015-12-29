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

#include <stdio.h>
#include <string.h>
#include <vk_loader_platform.h>
#include <vk_icd.h>
#include "swapchain.h"
#include "vk_layer_extension_utils.h"
#include "vk_enum_string_helper.h"

// FIXME/TODO: Make sure this layer is thread-safe!


// The following is for logging error messages:
static std::unordered_map<void *, layer_data *> layer_data_map;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

static const VkExtensionProperties instance_extensions[] = {
    {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_REPORT_REVISION
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);
}

static const VkLayerProperties swapchain_global_layers[] = {
    {
        "swapchain",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: swapchain",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(swapchain_global_layers),
                                   swapchain_global_layers,
                                   pCount, pProperties);
}

// This function validates a VkSurfaceKHR object:
static VkBool32 validateSurface(layer_data *my_data, VkSurfaceKHR surface, char *fn)
{
    VkBool32 skipCall = VK_FALSE;
    bool found_error = false;

    SwpSurface *pSurface = &my_data->surfaceMap[surface];
    if ((pSurface == NULL) || (pSurface->surface != surface)) {
        found_error = true;
    } else {
#if !defined(__ANDROID__)
        VkIcdSurfaceBase *pIcdSurface = (VkIcdSurfaceBase *) surface;
        if (pSurface->platform != pIcdSurface->platform) {
            found_error = true;
        }
#else  // !defined(__ANDROID__)
        if (pSurface->platform != VK_ICD_WSI_PLATFORM_ANDROID) {
            found_error = true;
        }
#endif // !defined(__ANDROID__)
    }
    if (found_error) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_SURFACE_KHR, surface, "VkSurfaceKHR",
                              SWAPCHAIN_INVALID_HANDLE,
                              "%s() called with an invalid surface object.",
                              fn);
    }
    return skipCall;
}

static void createDeviceRegisterExtensions(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i;
    layer_data *my_device_data   = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    VkLayerDispatchTable *pDisp  = my_device_data->device_dispatch_table;
    PFN_vkGetDeviceProcAddr gpa  = pDisp->GetDeviceProcAddr;

    pDisp->CreateSwapchainKHR    = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR   = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR   = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR       = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");

    SwpPhysicalDevice *pPhysicalDevice = &my_instance_data->physicalDeviceMap[physicalDevice];
    if (pPhysicalDevice) {
        my_device_data->deviceMap[device].pPhysicalDevice = pPhysicalDevice;
        pPhysicalDevice->pDevice = &my_device_data->deviceMap[device];
    } else {
        log_msg(my_instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                (uint64_t)physicalDevice , __LINE__, SWAPCHAIN_INVALID_HANDLE, "Swapchain",
                "vkCreateDevice() called with a non-valid VkPhysicalDevice.");
    }
    my_device_data->deviceMap[device].device = device;
    my_device_data->deviceMap[device].deviceSwapchainExtensionEnabled = false;

    // Record whether the WSI device extension was enabled for this VkDevice.
    // No need to check if the extension was advertised by
    // vkEnumerateDeviceExtensionProperties(), since the loader handles that.
    for (i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {

            my_device_data->deviceMap[device].deviceSwapchainExtensionEnabled = true;
        }
    }
}

static void createInstanceRegisterExtensions(const VkInstanceCreateInfo* pCreateInfo, VkInstance instance)
{
    uint32_t i;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable *pDisp  = my_data->instance_dispatch_table;
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    pDisp->CreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR) gpa(instance, "vkCreateAndroidSurfaceKHR");
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    pDisp->CreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR) gpa(instance, "vkCreateMirSurfaceKHR");
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    pDisp->CreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR) gpa(instance, "vkCreateWaylandSurfaceKHR");
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
    pDisp->CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) gpa(instance, "vkCreateWin32SurfaceKHR");
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    pDisp->CreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR) gpa(instance, "vkCreateXcbSurfaceKHR");
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    pDisp->CreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR) gpa(instance, "vkCreateXlibSurfaceKHR");
#endif // VK_USE_PLATFORM_XLIB_KHR
    pDisp->DestroySurfaceKHR = (PFN_vkDestroySurfaceKHR) gpa(instance, "vkDestroySurfaceKHR");
    pDisp->GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    pDisp->GetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pDisp->GetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    pDisp->GetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) gpa(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

    // Remember this instance, and whether the VK_KHR_surface extension
    // was enabled for it:
    my_data->instanceMap[instance].instance = instance;
    my_data->instanceMap[instance].surfaceExtensionEnabled = false;

    // Record whether the WSI instance extension was enabled for this
    // VkInstance.  No need to check if the extension was advertised by
    // vkEnumerateInstanceExtensionProperties(), since the loader handles that.
    for (i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {

            my_data->instanceMap[instance].surfaceExtensionEnabled = true;
        }
    }
}


#include "vk_dispatch_table_helper.h"
static void initSwapchain(layer_data *my_data, const VkAllocationCallbacks *pAllocator)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    VkDebugReportCallbackEXT callback;

    // Initialize Swapchain options:
    report_flags = getLayerOptionFlags("SwapchainReportFlags", 0);
    getLayerOptionEnum("SwapchainDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        // Turn on logging, since it was requested:
        option_str = getLayerOption("SwapchainLogFilename");
        log_output = getLayerLogOutput(option_str, "Swapchain");
        VkDebugReportCallbackCreateInfoEXT dbgInfo;
        memset(&dbgInfo, 0, sizeof(dbgInfo));
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgInfo.pfnCallback = log_callback;
        dbgInfo.pUserData = log_output;
        dbgInfo.flags = report_flags;
        layer_create_msg_callback(my_data->report_data,
                                  &dbgInfo,
                                  pAllocator,
                                  &callback);
        my_data->logging_callback.push_back(callback);
    }
    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        VkDebugReportCallbackCreateInfoEXT dbgInfo;
        memset(&dbgInfo, 0, sizeof(dbgInfo));
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgInfo.pfnCallback = win32_debug_output_msg;
        dbgInfo.pUserData = log_output;
        dbgInfo.flags = report_flags;
        layer_create_msg_callback(my_data->report_data, &dbgInfo, pAllocator, &callback);
        my_data->logging_callback.push_back(callback);
    }
}

static const char *surfaceTransformStr(VkSurfaceTransformFlagBitsKHR value)
{

    // TODO: parse flags and print out one at a time
    // Return a string corresponding to the value:
    return string_VkSurfaceTransformFlagBitsKHR(value);
}

static const char *presentModeStr(VkPresentModeKHR value)
{
    static std::string presentModeStrings[] = {
        "VK_PRESENT_MODE_IMMEDIATE_KHR",
        "VK_PRESENT_MODE_MAILBOX_KHR",
        "VK_PRESENT_MODE_FIFO_KHR",
        "Out-of-Range Value"};

    // Deal with a out-of-range value:
    switch (value) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
    case VK_PRESENT_MODE_MAILBOX_KHR:
    case VK_PRESENT_MODE_FIFO_KHR:
        break;
    default:
        value = (VkPresentModeKHR) (VK_PRESENT_MODE_FIFO_KHR + 1);
        break;
    }

    // Return a string corresponding to the value:
    return presentModeStrings[value].c_str();
}


VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    // Call down the call chain:
    VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
    VkResult result = pTable->CreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result == VK_SUCCESS) {
        // Since it succeeded, do layer-specific work:
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   pTable,
                                   *pInstance,
                                   pCreateInfo->enabledExtensionNameCount,
                                   pCreateInfo->ppEnabledExtensionNames);
        // Call the following function after my_data is initialized:
        createInstanceRegisterExtensions(pCreateInfo, *pInstance);
        initSwapchain(my_data, pAllocator);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if (!pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        my_data->instance_dispatch_table->DestroyInstance(instance, pAllocator);

        // Clean up logging callback, if any
        while (my_data->logging_callback.size() > 0) {
            VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
            layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
            my_data->logging_callback.pop_back();
        }
        layer_debug_report_destroy_instance(my_data->report_data);
    }

    // Regardless of skipCall value, do some internal cleanup:
    if (pInstance) {
        // Delete all of the SwpPhysicalDevice's and the SwpInstance associated
        // with this instance:
        for (auto it = pInstance->physicalDevices.begin() ;
             it != pInstance->physicalDevices.end() ; it++) {

            // Free memory that was allocated for/by this SwpPhysicalDevice:
            SwpPhysicalDevice *pPhysicalDevice = it->second;
            free(pPhysicalDevice->pSurfaceFormats);
            free(pPhysicalDevice->pPresentModes);

            // Erase the SwpPhysicalDevice's from the my_data->physicalDeviceMap (which
            // are simply pointed to by the SwpInstance):
            my_data->physicalDeviceMap.erase(it->second->physicalDevice);
        }
        my_data->instanceMap.erase(instance);
    }
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
}

#ifdef VK_USE_PLATFORM_ANDROID_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateAndroidSurfaceKHR(
    VkInstance                                  instance,
    ANativeWindow*                              window,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if (!pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_INSTANCE,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateAndroidSurfaceKHR(
                instance, window, pAllocator, pSurface);

        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].platform = VK_ICD_WSI_PLATFORM_ANDROID;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
            skipCall |= validateSurface(my_data, *pSurface, (char *) __FUNCTION__);
        }
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}
#endif // VK_USE_PLATFORM_ANDROID_KHR

#ifdef VK_USE_PLATFORM_MIR_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateMirSurfaceKHR(
    VkInstance                                  instance,
    MirConnection*                              connection,
    MirSurface*                                 mirSurface,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if (!pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_INSTANCE,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateMirSurfaceKHR(
                instance, connection, mirSurface, pAllocator, pSurface);

        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].platform = VK_ICD_WSI_PLATFORM_MIR;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
            skipCall |= validateSurface(my_data, *pSurface, (char *) __FUNCTION__);
        }
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}
#endif // VK_USE_PLATFORM_MIR_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateWaylandSurfaceKHR(
    VkInstance                                  instance,
    struct wl_display*                          display,
    struct wl_surface*                          surface,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if (!pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_INSTANCE,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateWaylandSurfaceKHR(
                instance, display, surface, pAllocator, pSurface);

        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].platform = VK_ICD_WSI_PLATFORM_WAYLAND;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
            skipCall |= validateSurface(my_data, *pSurface, (char *) __FUNCTION__);
        }
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}
#endif // VK_USE_PLATFORM_WAYLAND_KHR

#ifdef VK_USE_PLATFORM_WIN32_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(
    VkInstance                                  instance,
    HINSTANCE                                   hinstance,
    HWND                                        hwnd,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if (!pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_INSTANCE,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateWin32SurfaceKHR(
                instance, hinstance, hwnd, pAllocator, pSurface);

        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].platform = VK_ICD_WSI_PLATFORM_WIN32;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
            skipCall |= validateSurface(my_data, *pSurface, (char *) __FUNCTION__);
        }
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}
#endif // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateXcbSurfaceKHR(
    VkInstance                                  instance,
    xcb_connection_t*                           connection,
    xcb_window_t                                window,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if (!pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_INSTANCE,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateXcbSurfaceKHR(
                instance, connection, window, pAllocator, pSurface);

        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].platform = VK_ICD_WSI_PLATFORM_XCB;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
            skipCall |= validateSurface(my_data, *pSurface, (char *) __FUNCTION__);
        }
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}
#endif // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateXlibSurfaceKHR(
    VkInstance                                  instance,
    Display*                                    dpy,
    Window                                      window,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if (!pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_INSTANCE,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->CreateXlibSurfaceKHR(
                instance, dpy, window, pAllocator, pSurface);

        if ((result == VK_SUCCESS) && pInstance && pSurface) {
            // Record the VkSurfaceKHR returned by the ICD:
            my_data->surfaceMap[*pSurface].surface = *pSurface;
            my_data->surfaceMap[*pSurface].pInstance = pInstance;
            my_data->surfaceMap[*pSurface].platform = VK_ICD_WSI_PLATFORM_XLIB;
            // Point to the associated SwpInstance:
            pInstance->surfaces[*pSurface] = &my_data->surfaceMap[*pSurface];
            skipCall |= validateSurface(my_data, *pSurface, (char *) __FUNCTION__);
        }
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}
#endif // VK_USE_PLATFORM_XLIB_KHR

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR(VkInstance  instance, VkSurfaceKHR  surface, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if (!pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_INSTANCE,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Validate that a valid VkSurfaceKHR was used:
        skipCall |= validateSurface(my_data, surface, (char *) __FUNCTION__);
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        my_data->instance_dispatch_table->DestroySurfaceKHR(
                instance, surface, pAllocator);
    }

    // Regardless of skipCall value, do some internal cleanup:
    SwpSurface *pSurface = &my_data->surfaceMap[surface];
    if (pSurface && pSurface->pInstance) {
        pSurface->pInstance->surfaces.erase(surface);
    }
    my_data->surfaceMap.erase(surface);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &(my_data->instanceMap[instance]);
    if (!pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->EnumeratePhysicalDevices(
                instance, pPhysicalDeviceCount, pPhysicalDevices);

        if ((result == VK_SUCCESS) && pInstance && pPhysicalDevices &&
            (*pPhysicalDeviceCount > 0)) {
            // Record the VkPhysicalDevices returned by the ICD:
            SwpInstance *pInstance = &(my_data->instanceMap[instance]);
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                my_data->physicalDeviceMap[pPhysicalDevices[i]].physicalDevice =
                    pPhysicalDevices[i];
                my_data->physicalDeviceMap[pPhysicalDevices[i]].pInstance = pInstance;
                my_data->physicalDeviceMap[pPhysicalDevices[i]].pDevice = NULL;
                my_data->physicalDeviceMap[pPhysicalDevices[i]].gotSurfaceCapabilities = false;
                my_data->physicalDeviceMap[pPhysicalDevices[i]].surfaceFormatCount = 0;
                my_data->physicalDeviceMap[pPhysicalDevices[i]].pSurfaceFormats = NULL;
                my_data->physicalDeviceMap[pPhysicalDevices[i]].presentModeCount = 0;
                my_data->physicalDeviceMap[pPhysicalDevices[i]].pPresentModes = NULL;
                // Point to the associated SwpInstance:
                pInstance->physicalDevices[pPhysicalDevices[i]] =
                    &my_data->physicalDeviceMap[pPhysicalDevices[i]];
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    // Validate that a valid VkPhysicalDevice was used:
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
    if (!pPhysicalDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                            physicalDevice, "VkPhysicalDevice");
    }

    if (VK_TRUE == skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    // Call down the call chain:
    result = my_device_data->device_dispatch_table->CreateDevice(
            physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS) {
        // Since it succeeded, do layer-specific work:
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
        my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        createDeviceRegisterExtensions(physicalDevice, pCreateInfo, *pDevice);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    // Validate that a valid VkDevice was used:
    SwpDevice *pDevice = &my_data->deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                            device,
                                            "VkDevice");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        my_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    }

    // Regardless of skipCall value, do some internal cleanup:
    if (pDevice) {
        // Delete the SwpDevice associated with this device:
        if (pDevice->pPhysicalDevice) {
            pDevice->pPhysicalDevice->pDevice = NULL;
        }
        if (!pDevice->swapchains.empty()) {
            LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                      SWAPCHAIN_DEL_DEVICE_BEFORE_SWAPCHAINS,
                      "%s() called before all of its associated "
                      "VkSwapchainKHRs were destroyed.",
                      __FUNCTION__);
            // Empty and then delete all SwpSwapchain's
            for (auto it = pDevice->swapchains.begin() ;
                 it != pDevice->swapchains.end() ; it++) {
                // Delete all SwpImage's
                it->second->images.clear();
            }
            pDevice->swapchains.clear();
        }
        my_data->deviceMap.erase(device);
    }
    delete my_data->device_dispatch_table;
    layer_data_map.erase(key);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    VkSurfaceKHR surface,
    VkBool32* pSupported)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    // Validate that a valid VkPhysicalDevice was used, and that the instance
    // extension was enabled:
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
    if (!pPhysicalDevice || !pPhysicalDevice->pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                            physicalDevice,
                                            "VkPhysicalDevice");
    } else if (!pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_SURFACE_EXTENSION_NAME);
    }
    skipCall |= validateSurface(my_data, surface, (char *) __FUNCTION__);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfaceSupportKHR(
                physicalDevice, queueFamilyIndex, surface,
                pSupported);

        if ((result == VK_SUCCESS) && pSupported && pPhysicalDevice) {
            // Record the result of this query:
            pPhysicalDevice->queueFamilyIndexSupport[queueFamilyIndex] =
                *pSupported;
            // TODO: We need to compare this with the actual queue used for
            // presentation, to ensure it was advertised to the application as
            // supported for presentation.
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    VkSurfaceCapabilitiesKHR* pSurfaceCapabilities)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    // Validate that a valid VkPhysicalDevice was used, and that the instance
    // extension was enabled:
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
    if (!pPhysicalDevice || !pPhysicalDevice->pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                            physicalDevice,
                                            "VkPhysicalDevice");
    } else if (!pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_SURFACE_EXTENSION_NAME);
    }
    skipCall |= validateSurface(my_data, surface, (char *) __FUNCTION__);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfaceCapabilitiesKHR(
                physicalDevice, surface, pSurfaceCapabilities);

        if ((result == VK_SUCCESS) && pPhysicalDevice) {
            pPhysicalDevice->gotSurfaceCapabilities = true;
// FIXME: NEED TO COPY THIS DATA, BECAUSE pSurfaceCapabilities POINTS TO APP-ALLOCATED DATA
            pPhysicalDevice->surfaceCapabilities = *pSurfaceCapabilities;
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    uint32_t* pCount,
    VkSurfaceFormatKHR* pSurfaceFormats)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    // Validate that a valid VkPhysicalDevice was used, and that the instance
    // extension was enabled:
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
    if (!pPhysicalDevice || !pPhysicalDevice->pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                            physicalDevice,
                                            "VkPhysicalDevice");
    } else if (!pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_SURFACE_EXTENSION_NAME);
    }
    skipCall |= validateSurface(my_data, surface, (char *) __FUNCTION__);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfaceFormatsKHR(
                physicalDevice, surface, pCount, pSurfaceFormats);

        if ((result == VK_SUCCESS) && pPhysicalDevice && pSurfaceFormats && pCount &&
            (*pCount > 0)) {
            pPhysicalDevice->surfaceFormatCount = *pCount;
            pPhysicalDevice->pSurfaceFormats = (VkSurfaceFormatKHR *)
                malloc(*pCount * sizeof(VkSurfaceFormatKHR));
            if (pPhysicalDevice->pSurfaceFormats) {
                for (uint32_t i = 0 ; i < *pCount ; i++) {
                    pPhysicalDevice->pSurfaceFormats[i] = pSurfaceFormats[i];
                }
            } else {
                pPhysicalDevice->surfaceFormatCount = 0;
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice physicalDevice,
    VkSurfaceKHR surface,
    uint32_t* pCount,
    VkPresentModeKHR* pPresentModes)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    // Validate that a valid VkPhysicalDevice was used, and that the instance
    // extension was enabled:
    SwpPhysicalDevice *pPhysicalDevice = &my_data->physicalDeviceMap[physicalDevice];
    if (!pPhysicalDevice || !pPhysicalDevice->pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                            physicalDevice,
                                            "VkPhysicalDevice");
    } else if (!pPhysicalDevice->pInstance->surfaceExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkInstance.",
                              __FUNCTION__, VK_KHR_SURFACE_EXTENSION_NAME);
    }
    skipCall |= validateSurface(my_data, surface, (char *) __FUNCTION__);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->instance_dispatch_table->GetPhysicalDeviceSurfacePresentModesKHR(
                physicalDevice, surface, pCount, pPresentModes);

        if ((result == VK_SUCCESS) && pPhysicalDevice && pPresentModes && pCount &&
            (*pCount > 0)) {
            pPhysicalDevice->presentModeCount = *pCount;
            pPhysicalDevice->pPresentModes = (VkPresentModeKHR *)
                malloc(*pCount * sizeof(VkPresentModeKHR));
            if (pPhysicalDevice->pSurfaceFormats) {
                for (uint32_t i = 0 ; i < *pCount ; i++) {
                    pPhysicalDevice->pPresentModes[i] = pPresentModes[i];
                }
            } else {
                pPhysicalDevice->presentModeCount = 0;
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

// This function does the up-front validation work for vkCreateSwapchainKHR(),
// and returns VK_TRUE if a logging callback indicates that the call down the
// chain should be skipped:
static VkBool32 validateCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, VkSwapchainKHR* pSwapchain)
{
// TODO: Validate cases of re-creating a swapchain (the current code
// assumes a new swapchain is being created).
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    char fn[] = "vkCreateSwapchainKHR";

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &my_data->deviceMap[device];
    if (!pDevice) {
        return LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                         SWAPCHAIN_INVALID_HANDLE,
                         "%s() called with a non-valid %s.",
                         fn, "VkDevice");

    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        return LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                         SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                         "%s() called even though the %s extension was not enabled for this VkDevice.",
                         fn, VK_KHR_SWAPCHAIN_EXTENSION_NAME );
    }

    // Validate pCreateInfo with the results for previous queries:
    if (!pDevice->pPhysicalDevice && !pDevice->pPhysicalDevice->gotSurfaceCapabilities) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                              "%s() called before calling "
                              "vkGetPhysicalDeviceSurfaceCapabilitiesKHR().",
                              fn);
    } else {
        // Validate pCreateInfo->surface, to ensure it is valid (Note: in order
        // to validate, we must lookup layer_data based on the VkInstance
        // associated with this VkDevice):
        SwpPhysicalDevice *pPhysicalDevice = pDevice->pPhysicalDevice;
        SwpInstance *pInstance = pPhysicalDevice->pInstance;
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(pInstance->instance), layer_data_map);
        skipCall |= validateSurface(my_instance_data,
                                    pCreateInfo->surface,
                                    (char *) "vkCreateSwapchainKHR");
        // Validate pCreateInfo->minImageCount against
        // VkSurfaceCapabilitiesKHR::{min|max}ImageCount:
        VkSurfaceCapabilitiesKHR *pCapabilities = &pDevice->pPhysicalDevice->surfaceCapabilities;
        if ((pCreateInfo->minImageCount < pCapabilities->minImageCount) ||
            ((pCapabilities->maxImageCount > 0) &&
             (pCreateInfo->minImageCount > pCapabilities->maxImageCount))) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_MIN_IMG_COUNT,
                                  "%s() called with pCreateInfo->minImageCount "
                                  "= %d, which is outside the bounds returned "
                                  "by vkGetPhysicalDeviceSurfaceCapabilitiesKHR() (i.e. "
                                  "minImageCount = %d, maxImageCount = %d).",
                                  fn,
                                  pCreateInfo->minImageCount,
                                  pCapabilities->minImageCount,
                                  pCapabilities->maxImageCount);
        }
        // Validate pCreateInfo->imageExtent against
        // VkSurfaceCapabilitiesKHR::{current|min|max}ImageExtent:
        if ((pCapabilities->currentExtent.width == -1) &&
            ((pCreateInfo->imageExtent.width < pCapabilities->minImageExtent.width) ||
             (pCreateInfo->imageExtent.width > pCapabilities->maxImageExtent.width) ||
             (pCreateInfo->imageExtent.height < pCapabilities->minImageExtent.height) ||
             (pCreateInfo->imageExtent.height > pCapabilities->maxImageExtent.height))) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_OUT_OF_BOUNDS_EXTENTS,
                                  "%s() called with pCreateInfo->imageExtent = "
                                  "(%d,%d), which is outside the bounds "
                                  "returned by vkGetPhysicalDeviceSurfaceCapabilitiesKHR(): "
                                  "currentExtent = (%d,%d), minImageExtent = "
                                  "(%d,%d), maxImageExtent = (%d,%d).",
                                  fn,
                                  pCreateInfo->imageExtent.width,
                                  pCreateInfo->imageExtent.height,
                                  pCapabilities->currentExtent.width,
                                  pCapabilities->currentExtent.height,
                                  pCapabilities->minImageExtent.width,
                                  pCapabilities->minImageExtent.height,
                                  pCapabilities->maxImageExtent.width,
                                  pCapabilities->maxImageExtent.height);
        }
        if ((pCapabilities->currentExtent.width != -1) &&
            ((pCreateInfo->imageExtent.width != pCapabilities->currentExtent.width) ||
             (pCreateInfo->imageExtent.height != pCapabilities->currentExtent.height))) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_EXTENTS_NO_MATCH_WIN,
                                  "%s() called with pCreateInfo->imageExtent = "
                                  "(%d,%d), which is not equal to the "
                                  "currentExtent = (%d,%d) returned by "
                                  "vkGetPhysicalDeviceSurfaceCapabilitiesKHR().",
                                  fn,
                                  pCreateInfo->imageExtent.width,
                                  pCreateInfo->imageExtent.height,
                                  pCapabilities->currentExtent.width,
                                  pCapabilities->currentExtent.height);
        }
        // Validate pCreateInfo->preTransform against
        // VkSurfaceCapabilitiesKHR::supportedTransforms:
        if (!((pCreateInfo->preTransform) & pCapabilities->supportedTransforms)) {
            // This is an error situation; one for which we'd like to give
            // the developer a helpful, multi-line error message.  Build it
            // up a little at a time, and then log it:
            std::string errorString = "";
            char str[1024];
            // Here's the first part of the message:
            sprintf(str, "%s() called with a non-supported "
                    "pCreateInfo->preTransform (i.e. %s).  "
                    "Supported values are:\n",
                    fn,
                    surfaceTransformStr(pCreateInfo->preTransform));
            errorString += str;
            for (int i = 0; i < 32; i++) {
                // Build up the rest of the message:
                if ((1 << i) & pCapabilities->supportedTransforms) {
                    const char *newStr =
                        surfaceTransformStr((VkSurfaceTransformFlagBitsKHR) (1 << i));
                    sprintf(str, "    %s\n", newStr);
                    errorString += str;
                }
            }
            // Log the message that we've built up:
            skipCall |= debug_report_log_msg(my_data->report_data,
                                             VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                             VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                             (uint64_t) device, __LINE__,
                                             SWAPCHAIN_CREATE_SWAP_BAD_PRE_TRANSFORM,
                                             LAYER_NAME,
                                             errorString.c_str());
        }
        // Validate pCreateInfo->imageArraySize against
        // VkSurfaceCapabilitiesKHR::maxImageArraySize:
        if (pCreateInfo->imageArrayLayers > pCapabilities->maxImageArrayLayers) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_IMG_ARRAY_SIZE,
                                  "%s() called with a non-supported "
                                  "pCreateInfo->imageArraySize (i.e. %d).  "
                                  "Maximum value is %d.",
                                  fn,
                                  pCreateInfo->imageArrayLayers,
                                  pCapabilities->maxImageArrayLayers);
        }
        // Validate pCreateInfo->imageUsage against
        // VkSurfaceCapabilitiesKHR::supportedUsageFlags:
        if (pCreateInfo->imageUsage &&
            (pCreateInfo->imageUsage !=
             (pCreateInfo->imageUsage & pCapabilities->supportedUsageFlags))) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_IMG_USAGE_FLAGS,
                                  "%s() called with a non-supported "
                                  "pCreateInfo->imageUsage (i.e. 0x%08x)."
                                  "  Supported flag bits are 0x%08x.",
                                  fn,
                                  pCreateInfo->imageUsage,
                                  pCapabilities->supportedUsageFlags);
        }
    }
    if (!pDevice->pPhysicalDevice && !pDevice->pPhysicalDevice->surfaceFormatCount) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                              "%s() called before calling "
                              "vkGetPhysicalDeviceSurfaceFormatsKHR().",
                              fn);
    } else {
        // Validate pCreateInfo->imageFormat against
        // VkSurfaceFormatKHR::format:
        bool foundFormat = false;
        bool foundColorSpace = false;
        bool foundMatch = false;
        for (uint32_t i = 0 ; i < pDevice->pPhysicalDevice->surfaceFormatCount ; i++) {
            if (pCreateInfo->imageFormat == pDevice->pPhysicalDevice->pSurfaceFormats[i].format) {
                // Validate pCreateInfo->imageColorSpace against
                // VkSurfaceFormatKHR::colorSpace:
                foundFormat = true;
                if (pCreateInfo->imageColorSpace == pDevice->pPhysicalDevice->pSurfaceFormats[i].colorSpace) {
                    foundMatch = true;
                    break;
                }
            } else {
                if (pCreateInfo->imageColorSpace == pDevice->pPhysicalDevice->pSurfaceFormats[i].colorSpace) {
                    foundColorSpace = true;
                }
            }
        }
        if (!foundMatch) {
            if (!foundFormat) {
                if (!foundColorSpace) {
                    skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device,
                                          "VkDevice",
                                          SWAPCHAIN_CREATE_SWAP_BAD_IMG_FMT_CLR_SP,
                                          "%s() called with neither a "
                                          "supported pCreateInfo->imageFormat "
                                          "(i.e. %d) nor a supported "
                                          "pCreateInfo->imageColorSpace "
                                          "(i.e. %d).",
                                          fn,
                                          pCreateInfo->imageFormat,
                                          pCreateInfo->imageColorSpace);
                } else {
                    skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device,
                                          "VkDevice",
                                          SWAPCHAIN_CREATE_SWAP_BAD_IMG_FORMAT,
                                          "%s() called with a non-supported "
                                          "pCreateInfo->imageFormat (i.e. %d).",
                                          fn, pCreateInfo->imageFormat);
                }
            } else if (!foundColorSpace) {
                skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                      SWAPCHAIN_CREATE_SWAP_BAD_IMG_COLOR_SPACE,
                                      "%s() called with a non-supported "
                                      "pCreateInfo->imageColorSpace (i.e. %d).",
                                      fn, pCreateInfo->imageColorSpace);
            }
        }
    }
    if (!pDevice->pPhysicalDevice && !pDevice->pPhysicalDevice->presentModeCount) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                              "%s() called before calling "
                              "vkGetPhysicalDeviceSurfacePresentModesKHR().",
                              fn);
    } else {
        // Validate pCreateInfo->presentMode against
        // vkGetPhysicalDeviceSurfacePresentModesKHR():
        bool foundMatch = false;
        for (uint32_t i = 0 ; i < pDevice->pPhysicalDevice->presentModeCount ; i++) {
            if (pDevice->pPhysicalDevice->pPresentModes[i] == pCreateInfo->presentMode) {
                foundMatch = true;
                break;
            }
        }
        if (!foundMatch) {
            skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_PRESENT_MODE,
                                  "%s() called with a non-supported "
                                  "pCreateInfo->presentMode (i.e. %s).",
                                  fn,
                                  presentModeStr(pCreateInfo->presentMode));
        }
    }

    // TODO: Validate the following values:
    // - pCreateInfo->sharingMode
    // - pCreateInfo->queueFamilyIndexCount
    // - pCreateInfo->pQueueFamilyIndices
    // - pCreateInfo->oldSwapchain

    return skipCall;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkSwapchainKHR* pSwapchain)
{
    VkResult result = VK_SUCCESS;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkBool32 skipCall = validateCreateSwapchainKHR(device, pCreateInfo,
                                                   pSwapchain);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->device_dispatch_table->CreateSwapchainKHR(
                device, pCreateInfo, pAllocator, pSwapchain);

        if (result == VK_SUCCESS) {
            // Remember the swapchain's handle, and link it to the device:
            SwpDevice *pDevice = &my_data->deviceMap[device];

            my_data->swapchainMap[*pSwapchain].swapchain = *pSwapchain;
            pDevice->swapchains[*pSwapchain] =
                &my_data->swapchainMap[*pSwapchain];
            my_data->swapchainMap[*pSwapchain].pDevice = pDevice;
            my_data->swapchainMap[*pSwapchain].imageCount = 0;
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    const VkAllocationCallbacks* pAllocator)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &my_data->deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                            device,
                                            "VkDevice");
    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkDevice.",
                              __FUNCTION__, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    // Regardless of skipCall value, do some internal cleanup:
    SwpSwapchain *pSwapchain = &my_data->swapchainMap[swapchain];
    if (pSwapchain) {
        // Delete the SwpSwapchain associated with this swapchain:
        if (pSwapchain->pDevice) {
            pSwapchain->pDevice->swapchains.erase(swapchain);
            if (device != pSwapchain->pDevice->device) {
                LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                          SWAPCHAIN_DESTROY_SWAP_DIFF_DEVICE,
                          "%s() called with a different VkDevice than the "
                          "VkSwapchainKHR was created with.",
                          __FUNCTION__);
            }
        }
        if (pSwapchain->imageCount) {
            pSwapchain->images.clear();
        }
        my_data->swapchainMap.erase(swapchain);
    } else {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                            swapchain,
                                            "VkSwapchainKHR");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        my_data->device_dispatch_table->DestroySwapchainKHR(device, swapchain, pAllocator);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pCount, VkImage* pSwapchainImages)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &my_data->deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                            device,
                                            "VkDevice");
    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkDevice.",
                              __FUNCTION__, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    SwpSwapchain *pSwapchain = &my_data->swapchainMap[swapchain];
    if (!pSwapchain) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                            swapchain.handle,
                                            "VkSwapchainKHR");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->device_dispatch_table->GetSwapchainImagesKHR(
                device, swapchain, pCount, pSwapchainImages);

// TBD: Should we validate that this function was called once with
// pSwapchainImages set to NULL (and record pCount at that time), and then
// called again with a non-NULL pSwapchainImages?
        if ((result == VK_SUCCESS) && pSwapchain &&pSwapchainImages &&
            pCount && (*pCount > 0)) {
            // Record the images and their state:
            if (pSwapchain) {
                pSwapchain->imageCount = *pCount;
                for (uint32_t i = 0 ; i < *pCount ; i++) {
                    pSwapchain->images[i].image = pSwapchainImages[i];
                    pSwapchain->images[i].pSwapchain = pSwapchain;
                    pSwapchain->images[i].ownedByApp = false;
                }
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint64_t timeout,
    VkSemaphore semaphore,
    VkFence fence,
    uint32_t* pImageIndex)
{
// TODO: Record/update the state of the swapchain, in case an error occurs
// (e.g. VK_ERROR_OUT_OF_DATE_KHR).
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &my_data->deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                            device,
                                            "VkDevice");
    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the %s extension was not enabled for this VkDevice.",
                              __FUNCTION__, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    // Validate that a valid VkSwapchainKHR was used:
    SwpSwapchain *pSwapchain = &my_data->swapchainMap[swapchain];
    if (!pSwapchain) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                            swapchain,
                                            "VkSwapchainKHR");
    } else {
        // Look to see if the application is trying to own too many images at
        // the same time (i.e. not leave any to display):
        uint32_t imagesOwnedByApp = 0;
        for (uint32_t i = 0 ; i < pSwapchain->imageCount ; i++) {
            if (pSwapchain->images[i].ownedByApp) {
                imagesOwnedByApp++;
            }
        }
        if (imagesOwnedByApp >= (pSwapchain->imageCount - 1)) {
            skipCall |= LOG_PERF_WARNING(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                         swapchain,
                                         "VkSwapchainKHR",
                                         SWAPCHAIN_APP_OWNS_TOO_MANY_IMAGES,
                                         "%s() called when the application "
                                         "already owns all presentable images "
                                         "in this swapchain except for the "
                                         "image currently being displayed.  "
                                         "This call to %s() cannot succeed "
                                         "unless another thread calls the "
                                         "vkQueuePresentKHR() function in "
                                         "order to release ownership of one of "
                                         "the presentable images of this "
                                         "swapchain.",
                                         __FUNCTION__, __FUNCTION__);
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->device_dispatch_table->AcquireNextImageKHR(
                device, swapchain, timeout, semaphore, fence, pImageIndex);

        if (((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)) &&
            pSwapchain) {
            // Change the state of the image (now owned by the application):
            pSwapchain->images[*pImageIndex].ownedByApp = true;
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
    VkQueue queue,
    const VkPresentInfoKHR* pPresentInfo)
{
// TODOs:
//
// - Ensure that the queue is active, and is one of the queueFamilyIndex's
//   that was returned by a previuos query.
// - Record/update the state of the swapchain, in case an error occurs
//   (e.g. VK_ERROR_OUT_OF_DATE_KHR).
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(queue), layer_data_map);

    for (uint32_t i = 0; i < pPresentInfo->swapchainCount ; i++) {
        uint32_t index = pPresentInfo->pImageIndices[i];
        SwpSwapchain *pSwapchain =
            &my_data->swapchainMap[pPresentInfo->pSwapchains[i]];
        if (pSwapchain) {
            if (!pSwapchain->pDevice->deviceSwapchainExtensionEnabled) {
                skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                      pSwapchain->pDevice, "VkDevice",
                                      SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                                      "%s() called even though the %s extension was not enabled for this VkDevice.",
                                      __FUNCTION__, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            }
            if (index >= pSwapchain->imageCount) {
                skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                      pPresentInfo->pSwapchains[i],
                                      "VkSwapchainKHR",
                                      SWAPCHAIN_INDEX_TOO_LARGE,
                                      "%s() called for an index that is too "
                                      "large (i.e. %d).  There are only %d "
                                      "images in this VkSwapchainKHR.\n",
                                      __FUNCTION__, index,
                                      pSwapchain->imageCount);
            } else {
                if (!pSwapchain->images[index].ownedByApp) {
                    skipCall |= LOG_ERROR(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                          pPresentInfo->pSwapchains[i],
                                          "VkSwapchainKHR",
                                          SWAPCHAIN_INDEX_NOT_IN_USE,
                                          "%s() returned an index (i.e. %d) "
                                          "for an image that is not owned by "
                                          "the application.",
                                          __FUNCTION__, index);
                }
            }
        } else {
            skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                                pPresentInfo->pSwapchains[i],
                                                "VkSwapchainKHR");
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = my_data->device_dispatch_table->QueuePresentKHR(queue,
                                                               pPresentInfo);

        if ((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)) {
            for (uint32_t i = 0; i < pPresentInfo->swapchainCount ; i++) {
                int index = pPresentInfo->pImageIndices[i];
                SwpSwapchain *pSwapchain =
                    &my_data->swapchainMap[pPresentInfo->pSwapchains[i]];
                if (pSwapchain) {
                    // Change the state of the image (no longer owned by the
                    // application):
                    pSwapchain->images[index].ownedByApp = false;
                }
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

static inline PFN_vkVoidFunction layer_intercept_proc(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;
    if (!strcmp(name, "CreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(name, "DestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;

    return NULL;
}
static inline PFN_vkVoidFunction layer_intercept_instance_proc(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;

    return NULL;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
        VkInstance                                      instance,
        const VkDebugReportCallbackCreateInfoEXT*       pCreateInfo,
        const VkAllocationCallbacks*                    pAllocator,
        VkDebugReportCallbackEXT*                       pMsgCallback)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult result = my_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (VK_SUCCESS == result) {
        result = layer_create_msg_callback(my_data->report_data, pCreateInfo, pAllocator, pMsgCallback);
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback, const VkAllocationCallbacks *pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(
        VkInstance                                  instance,
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT                  objType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     msgCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMsg)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    PFN_vkVoidFunction addr;
    if (device == VK_NULL_HANDLE) {
        return NULL;
    }

    layer_data *my_data;
    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp("vkGetDeviceProcAddr", funcName)) {
        VkBaseLayerObject* wrapped_dev = (VkBaseLayerObject*) device;
        my_data = get_my_data_ptr(get_dispatch_key(wrapped_dev->baseObject), layer_data_map);
        my_data->device_dispatch_table = new VkLayerDispatchTable;
        layer_initialize_dispatch_table(my_data->device_dispatch_table, wrapped_dev);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }

    addr = layer_intercept_proc(funcName);
    if (addr)
        return addr;

    my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp =  my_data->device_dispatch_table;
    if (my_data->deviceMap.size() != 0 &&
        my_data->deviceMap[device].deviceSwapchainExtensionEnabled)
    {
        if (!strcmp("vkCreateSwapchainKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateSwapchainKHR);
        if (!strcmp("vkDestroySwapchainKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkDestroySwapchainKHR);
        if (!strcmp("vkGetSwapchainImagesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetSwapchainImagesKHR);
        if (!strcmp("vkAcquireNextImageKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkAcquireNextImageKHR);
        if (!strcmp("vkQueuePresentKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkQueuePresentKHR);
    }
    {
        if (pDisp->GetDeviceProcAddr == NULL)
            return NULL;
        return pDisp->GetDeviceProcAddr(device, funcName);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    PFN_vkVoidFunction addr;
    if (instance == VK_NULL_HANDLE) {
        return NULL;
    }

    layer_data *my_data;
    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp("vkGetInstanceProcAddr", funcName)) {
        VkBaseLayerObject* wrapped_inst = (VkBaseLayerObject*) instance;
        my_data = get_my_data_ptr(get_dispatch_key(wrapped_inst->baseObject), layer_data_map);
        my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
        layer_init_instance_dispatch_table(my_data->instance_dispatch_table, wrapped_inst);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }

    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;

    addr = layer_intercept_instance_proc(funcName);
    if (addr)
        return addr;

    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
    addr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (addr) {
        return addr;
    }

    if (my_data->instanceMap.size() != 0 &&
        my_data->instanceMap[instance].surfaceExtensionEnabled)
    {
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (!strcmp("vkCreateAndroidSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateAndroidSurfaceKHR);
#endif // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (!strcmp("vkCreateMirSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateMirSurfaceKHR);
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (!strcmp("vkCreateWaylandSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateWaylandSurfaceKHR);
#endif // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (!strcmp("vkCreateWin32SurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateWin32SurfaceKHR);
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (!strcmp("vkCreateXcbSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateXcbSurfaceKHR);
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (!strcmp("vkCreateXlibSurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkCreateXlibSurfaceKHR);
#endif // VK_USE_PLATFORM_XLIB_KHR
        if (!strcmp("vkDestroySurfaceKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkDestroySurfaceKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfaceSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceSupportKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfaceFormatsKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceFormatsKHR);
        if (!strcmp("vkGetPhysicalDeviceSurfacePresentModesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfacePresentModesKHR);
    }

    if (pTable->GetInstanceProcAddr == NULL)
        return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}

