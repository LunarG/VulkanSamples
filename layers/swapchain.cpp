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

#include <stdio.h>
#include <string.h>
#include <unordered_map>
#include "vk_loader_platform.h"
#include "vk_layer.h"
#include "vk_layer_config.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"

#include "swapchain.h"


// FIXME/TODO: Make sure this layer is thread-safe!


// The following is for logging error messages:
static std::unordered_map<void *, layer_data *> layer_data_map;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);


// NOTE: The following are for keeping track of info that is used for
// validating the WSI extensions.
static std::unordered_map<void *, SwpInstance>       instanceMap;
static std::unordered_map<void *, SwpPhysicalDevice> physicalDeviceMap;
static std::unordered_map<void *, SwpDevice>         deviceMap;
static std::unordered_map<uint64_t, SwpSwapchain>    swapchainMap;


static void createDeviceRegisterExtensions(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
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

    SwpPhysicalDevice *pPhysicalDevice = &physicalDeviceMap[physicalDevice];
    if (pPhysicalDevice) {
        deviceMap[device].pPhysicalDevice = pPhysicalDevice;
        pPhysicalDevice->pDevice = &deviceMap[device];
    } else {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
        LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_PHYSICAL_DEVICE,
                                physicalDevice,
                                "VkPhysicalDevice");
    }
    deviceMap[device].device = device;
    deviceMap[device].deviceSwapchainExtensionEnabled = false;
    deviceMap[device].gotSurfaceProperties = false;
    deviceMap[device].surfaceFormatCount = 0;
    deviceMap[device].pSurfaceFormats = NULL;
    deviceMap[device].presentModeCount = 0;
    deviceMap[device].pPresentModes = NULL;

    // Record whether the WSI device extension was enabled for this VkDevice.
    // No need to check if the extension was advertised by
    // vkEnumerateDeviceExtensionProperties(), since the loader handles that.
    for (i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME) == 0) {

            deviceMap[device].deviceSwapchainExtensionEnabled = true;
        }
    }
}

static void createInstanceRegisterExtensions(const VkInstanceCreateInfo* pCreateInfo, VkInstance instance)
{
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp  = instance_dispatch_table(instance);
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;
    pDisp->GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");

    // Remember this instance, and whether the VK_EXT_KHR_swapchain extension
    // was enabled for it:
    instanceMap[instance].instance = instance;
    instanceMap[instance].swapchainExtensionEnabled = false;

    // Record whether the WSI instance extension was enabled for this
    // VkInstance.  No need to check if the extension was advertised by
    // vkEnumerateInstanceExtensionProperties(), since the loader handles that.
    for (i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {

            instanceMap[instance].swapchainExtensionEnabled = true;
        }
    }
}


#include "vk_dispatch_table_helper.h"
static void initSwapchain(layer_data *my_data)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    VkDbgMsgCallback callback;

    // Initialize Swapchain options:
    report_flags = getLayerOptionFlags("SwapchainReportFlags", 0);
    getLayerOptionEnum("SwapchainDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        // Turn on logging, since it was requested:
        option_str = getLayerOption("SwapchainLogFilename");
        log_output = getLayerLogOutput(option_str, "Swapchain");
        layer_create_msg_callback(my_data->report_data, report_flags,
                                  log_callback, (void *) log_output,
                                  &callback);
        my_data->logging_callback.push_back(callback);
    }
    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        layer_create_msg_callback(my_data->report_data, report_flags, win32_debug_output_msg, NULL, &callback);
        my_data->logging_callback.push_back(callback);
    }
}

static const char *surfaceTransformStr(VkSurfaceTransformKHR value)
{
    static std::string surfaceTransformStrings[] = {
        "VK_SURFACE_TRANSFORM_NONE_KHR",
        "VK_SURFACE_TRANSFORM_ROT90_KHR",
        "VK_SURFACE_TRANSFORM_ROT180_KHR",
        "VK_SURFACE_TRANSFORM_ROT270_KHR",
        "VK_SURFACE_TRANSFORM_HMIRROR_KHR",
        "VK_SURFACE_TRANSFORM_HMIRROR_ROT90_KHR",
        "VK_SURFACE_TRANSFORM_HMIRROR_ROT180_KHR",
        "VK_SURFACE_TRANSFORM_HMIRROR_ROT270_KHR",
        "Out-of-Range Value"};

    // Deal with a out-of-range value:
    switch (value) {
    case VK_SURFACE_TRANSFORM_NONE_KHR:
    case VK_SURFACE_TRANSFORM_ROT90_KHR:
    case VK_SURFACE_TRANSFORM_ROT180_KHR:
    case VK_SURFACE_TRANSFORM_ROT270_KHR:
    case VK_SURFACE_TRANSFORM_HMIRROR_KHR:
    case VK_SURFACE_TRANSFORM_HMIRROR_ROT90_KHR:
    case VK_SURFACE_TRANSFORM_HMIRROR_ROT180_KHR:
    case VK_SURFACE_TRANSFORM_HMIRROR_ROT270_KHR:
        break;
    default:
        value =
            (VkSurfaceTransformKHR) (VK_SURFACE_TRANSFORM_HMIRROR_ROT270_KHR + 1);
        break;
    }

    // Return a string corresponding to the value:
    return surfaceTransformStrings[value].c_str();
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


VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, VkInstance* pInstance)
{
    // Call down the call chain:
    VkResult result = instance_dispatch_table(*pInstance)->CreateInstance(pCreateInfo, pInstance);
    if (result == VK_SUCCESS) {
        // Since it succeeded, do layer-specific work:
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   instance_dispatch_table(*pInstance),
                                   *pInstance,
                                   pCreateInfo->extensionCount,
                                   pCreateInfo->ppEnabledExtensionNames);
        // Call the following function after my_data is initialized:
        createInstanceRegisterExtensions(pCreateInfo, *pInstance);
        initSwapchain(my_data);
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkDestroyInstance(VkInstance instance)
{
    VkBool32 skipCall = VK_FALSE;

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &instanceMap[instance];
    if (!pInstance) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_INSTANCE,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        dispatch_key key = get_dispatch_key(instance);
        VkLayerInstanceDispatchTable *pDisp = instance_dispatch_table(instance);
        pDisp->DestroyInstance(instance);

        // Clean up logging callback, if any
        layer_data *my_data = get_my_data_ptr(key, layer_data_map);
        while (my_data->logging_callback.size() > 0) {
            VkDbgMsgCallback callback = my_data->logging_callback.back();
            layer_destroy_msg_callback(my_data->report_data, callback);
            my_data->logging_callback.pop_back();
        }
        layer_debug_report_destroy_instance(my_data->report_data);
        layer_data_map.erase(key);

        destroy_instance_dispatch_table(key);
    }

    // Regardless of skipCall value, do some internal cleanup:
    if (pInstance) {
        // Delete all of the SwpPhysicalDevice's and the SwpInstance associated
        // with this instance:
        for (auto it = pInstance->physicalDevices.begin() ;
             it != pInstance->physicalDevices.end() ; it++) {
            // Erase the SwpPhysicalDevice's from the physicalDeviceMap (which
            // are simply pointed to by the SwpInstance):
            physicalDeviceMap.erase(it->second->physicalDevice);
        }
        instanceMap.erase(instance);
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;

    // Validate that a valid VkInstance was used:
    SwpInstance *pInstance = &instanceMap[instance];
    if (!pInstance) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_INSTANCE,
                                            instance,
                                            "VkInstance");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = instance_dispatch_table(instance)->EnumeratePhysicalDevices(
                instance, pPhysicalDeviceCount, pPhysicalDevices);

        if ((result == VK_SUCCESS) && pInstance && pPhysicalDevices &&
            (*pPhysicalDeviceCount > 0)) {
            // Record the VkPhysicalDevices returned by the ICD:
            SwpInstance *pInstance = &instanceMap[instance];
            for (int i = 0; i < *pPhysicalDeviceCount; i++) {
                physicalDeviceMap[pPhysicalDevices[i]].physicalDevice =
                    pPhysicalDevices[i];
                physicalDeviceMap[pPhysicalDevices[i]].pInstance = pInstance;
                physicalDeviceMap[pPhysicalDevices[i]].pDevice = NULL;
                // Point to the associated SwpInstance:
                pInstance->physicalDevices[pPhysicalDevices[i]] =
                    &physicalDeviceMap[pPhysicalDevices[i]];
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    // Validate that a valid VkPhysicalDevice was used:
    SwpPhysicalDevice *pPhysicalDevice = &physicalDeviceMap[physicalDevice];
    if (!pPhysicalDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_PHYSICAL_DEVICE,
                                            physicalDevice,
                                            "VkPhysicalDevice");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = device_dispatch_table(*pDevice)->CreateDevice(
                physicalDevice, pCreateInfo, pDevice);
        if (result == VK_SUCCESS) {
            // Since it succeeded, do layer-specific work:
            my_data->report_data = layer_debug_report_create_device(my_data->report_data, *pDevice);
            createDeviceRegisterExtensions(physicalDevice, pCreateInfo,
                                           *pDevice);
        }
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT void VKAPI vkDestroyDevice(VkDevice device)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used:
    SwpDevice *pDevice = &deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_DEVICE,
                                            device,
                                            "VkDevice");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        dispatch_key key = get_dispatch_key(device);
        VkLayerDispatchTable *pDisp  =  device_dispatch_table(device);
        pDisp->DestroyDevice(device);
        destroy_device_dispatch_table(key);
    }

    // Regardless of skipCall value, do some internal cleanup:
    if (pDevice) {
        // Delete the SwpDevice associated with this device:
        if (pDevice->pPhysicalDevice) {
            pDevice->pPhysicalDevice->pDevice = NULL;
        }
        if (deviceMap[device].pSurfaceFormats) {
            free(deviceMap[device].pSurfaceFormats);
        }
        if (deviceMap[device].pPresentModes) {
            free(deviceMap[device].pPresentModes);
        }
        if (!pDevice->swapchains.empty()) {
            LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
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
        deviceMap.erase(device);
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, const VkSurfaceDescriptionKHR* pSurfaceDescription, VkBool32* pSupported)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    // Validate that a valid VkPhysicalDevice was used, and that the instance
    // extension was enabled:
    SwpPhysicalDevice *pPhysicalDevice = &physicalDeviceMap[physicalDevice];
    if (!pPhysicalDevice || !pPhysicalDevice->pInstance) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_PHYSICAL_DEVICE,
                                            physicalDevice,
                                            "VkPhysicalDevice");
    } else if (!pPhysicalDevice->pInstance->swapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_INSTANCE,
                              pPhysicalDevice->pInstance,
                              "VkInstance",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the "
                              VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME,
                              "extension was not enabled for this VkInstance.",
                              __FUNCTION__);
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = instance_dispatch_table(physicalDevice)->GetPhysicalDeviceSurfaceSupportKHR(
                physicalDevice, queueFamilyIndex, pSurfaceDescription,
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
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetSurfacePropertiesKHR(VkDevice device, const VkSurfaceDescriptionKHR* pSurfaceDescription, VkSurfacePropertiesKHR* pSurfaceProperties)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_DEVICE,
                                            device,
                                            "VkDevice");
    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the "
                              VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME,
                              "extension was not enabled for this VkDevice.",
                              __FUNCTION__);
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = device_dispatch_table(device)->GetSurfacePropertiesKHR(
                device, pSurfaceDescription, pSurfaceProperties);

        if ((result == VK_SUCCESS) && pDevice) {
            pDevice->gotSurfaceProperties = true;
            pDevice->surfaceProperties = *pSurfaceProperties;
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetSurfaceFormatsKHR(VkDevice device, const VkSurfaceDescriptionKHR* pSurfaceDescription, uint32_t* pCount, VkSurfaceFormatKHR* pSurfaceFormats)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_DEVICE,
                                            device,
                                            "VkDevice");
    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the "
                              VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME,
                              "extension was not enabled for this VkDevice.",
                              __FUNCTION__);
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = device_dispatch_table(device)->GetSurfaceFormatsKHR(
                device, pSurfaceDescription, pCount, pSurfaceFormats);

        if ((result == VK_SUCCESS) && pDevice && pSurfaceFormats && pCount &&
            (*pCount > 0)) {
            pDevice->surfaceFormatCount = *pCount;
            pDevice->pSurfaceFormats = (VkSurfaceFormatKHR *)
                malloc(*pCount * sizeof(VkSurfaceFormatKHR));
            if (pDevice->pSurfaceFormats) {
                for (int i = 0 ; i < *pCount ; i++) {
                    pDevice->pSurfaceFormats[i] = pSurfaceFormats[i];
                }
            } else {
                pDevice->surfaceFormatCount = 0;
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetSurfacePresentModesKHR(VkDevice device, const VkSurfaceDescriptionKHR* pSurfaceDescription, uint32_t* pCount, VkPresentModeKHR* pPresentModes)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_DEVICE,
                                            device,
                                            "VkDevice");
    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the "
                              VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME,
                              "extension was not enabled for this VkDevice.",
                              __FUNCTION__);
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = device_dispatch_table(device)->GetSurfacePresentModesKHR(
                device, pSurfaceDescription, pCount, pPresentModes);

        if ((result == VK_SUCCESS) && pDevice && pPresentModes && pCount &&
            (*pCount > 0)) {
            pDevice->presentModeCount = *pCount;
            pDevice->pPresentModes = (VkPresentModeKHR *)
                malloc(*pCount * sizeof(VkPresentModeKHR));
            if (pDevice->pSurfaceFormats) {
                for (int i = 0 ; i < *pCount ; i++) {
                    pDevice->pPresentModes[i] = pPresentModes[i];
                }
            } else {
                pDevice->presentModeCount = 0;
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
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
    SwpDevice *pDevice = &deviceMap[device];
    if (!pDevice) {
        return LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                         SWAPCHAIN_INVALID_HANDLE,
                         "%s() called with a non-valid %s.",
                         fn, "VkDevice");

    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        return LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                         SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                         "%s() called even though the "
                         VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME,
                         "extension was not enabled for this VkDevice.",
                         fn);
    }

    // Validate pCreateInfo with the results for previous queries:
    if (!pDevice->gotSurfaceProperties) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                              "%s() called before calling "
                              "vkGetSurfacePropertiesKHR().",
                              fn);
    } else {
        // Validate pCreateInfo->minImageCount against
        // VkSurfacePropertiesKHR::{min|max}ImageCount:
        VkSurfacePropertiesKHR *pProps = &pDevice->surfaceProperties;
        if ((pCreateInfo->minImageCount < pProps->minImageCount) ||
            ((pProps->maxImageCount > 0) &&
             (pCreateInfo->minImageCount > pProps->maxImageCount))) {
            skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_MIN_IMG_COUNT,
                                  "%s() called with pCreateInfo->minImageCount "
                                  "= %d, which is outside the bounds returned "
                                  "by vkGetSurfacePropertiesKHR() (i.e. "
                                  "minImageCount = %d, maxImageCount = %d).",
                                  fn,
                                  pCreateInfo->minImageCount,
                                  pProps->minImageCount,
                                  pProps->maxImageCount);
        }
        // Validate pCreateInfo->imageExtent against
        // VkSurfacePropertiesKHR::{current|min|max}ImageExtent:
        if ((pProps->currentExtent.width == -1) &&
            ((pCreateInfo->imageExtent.width < pProps->minImageExtent.width) ||
             (pCreateInfo->imageExtent.width > pProps->maxImageExtent.width) ||
             (pCreateInfo->imageExtent.height < pProps->minImageExtent.height) ||
             (pCreateInfo->imageExtent.height > pProps->maxImageExtent.height))) {
            skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_OUT_OF_BOUNDS_EXTENTS,
                                  "%s() called with pCreateInfo->imageExtent = "
                                  "(%d,%d), which is outside the bounds "
                                  "returned by vkGetSurfacePropertiesKHR(): "
                                  "currentExtent = (%d,%d), minImageExtent = "
                                  "(%d,%d), maxImageExtent = (%d,%d).",
                                  fn,
                                  pCreateInfo->imageExtent.width,
                                  pCreateInfo->imageExtent.height,
                                  pProps->currentExtent.width,
                                  pProps->currentExtent.height,
                                  pProps->minImageExtent.width,
                                  pProps->minImageExtent.height,
                                  pProps->maxImageExtent.width,
                                  pProps->maxImageExtent.height);
        }
        if ((pProps->currentExtent.width != -1) &&
            ((pCreateInfo->imageExtent.width != pProps->currentExtent.width) ||
             (pCreateInfo->imageExtent.height != pProps->currentExtent.height))) {
            skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_EXTENTS_NO_MATCH_WIN,
                                  "%s() called with pCreateInfo->imageExtent = "
                                  "(%d,%d), which is not equal to the "
                                  "currentExtent = (%d,%d) returned by "
                                  "vkGetSurfacePropertiesKHR().",
                                  fn,
                                  pCreateInfo->imageExtent.width,
                                  pCreateInfo->imageExtent.height,
                                  pProps->currentExtent.width,
                                  pProps->currentExtent.height);
        }
        // Validate pCreateInfo->preTransform against
        // VkSurfacePropertiesKHR::supportedTransforms:
        if (!((1 << pCreateInfo->preTransform) & pProps->supportedTransforms)) {
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
            for (int i = VK_SURFACE_TRANSFORM_NONE_KHR ;
                 i < VK_SURFACE_TRANSFORM_INHERIT_KHR ; i++) {
                // Build up the rest of the message:
                if ((1 << i) & pProps->supportedTransforms) {
                    const char *newStr =
                        surfaceTransformStr((VkSurfaceTransformKHR) (1 << i));
                    sprintf(str, "    %s\n", newStr);
                    errorString += str;
                }
            }
            // Log the message that we've built up:
            skipCall |= debug_report_log_msg(my_data->report_data,
                                             VK_DBG_REPORT_ERROR_BIT,
                                             VK_OBJECT_TYPE_DEVICE,
                                             (uint64_t) device, 0, 
                                             SWAPCHAIN_CREATE_SWAP_BAD_PRE_TRANSFORM,
                                             LAYER_NAME,
                                             errorString.c_str());
        }
        // Validate pCreateInfo->imageArraySize against
        // VkSurfacePropertiesKHR::maxImageArraySize:
        if (pCreateInfo->imageArraySize <= pProps->maxImageArraySize) {
            skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_IMG_ARRAY_SIZE,
                                  "%s() called with a non-supported "
                                  "pCreateInfo->imageArraySize (i.e. %d).  "
                                  "Maximum value is %d.",
                                  fn,
                                  pCreateInfo->imageArraySize,
                                  pProps->maxImageArraySize);
        }
        // Validate pCreateInfo->imageUsageFlags against
        // VkSurfacePropertiesKHR::supportedUsageFlags:
        if (pCreateInfo->imageUsageFlags &&
            (pCreateInfo->imageUsageFlags !=
             (pCreateInfo->imageUsageFlags & pProps->supportedUsageFlags))) {
            skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_IMG_USAGE_FLAGS,
                                  "%s() called with a non-supported "
                                  "pCreateInfo->imageUsageFlags (i.e. 0x%08x)."
                                  "  Supported flag bits are 0x%08x.",
                                  fn,
                                  pCreateInfo->imageUsageFlags,
                                  pProps->supportedUsageFlags);
        }
    }
    if (!pDevice->surfaceFormatCount) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                              "%s() called before calling "
                              "vkGetSurfaceFormatsKHR().",
                              fn);
    } else {
        // Validate pCreateInfo->imageFormat against
        // VkSurfaceFormatKHR::format:
        bool foundFormat = false;
        bool foundColorSpace = false;
        bool foundMatch = false;
        for (int i = 0 ; i < pDevice->surfaceFormatCount ; i++) {
            if (pCreateInfo->imageFormat == pDevice->pSurfaceFormats[i].format) {
                // Validate pCreateInfo->imageColorSpace against
                // VkSurfaceFormatKHR::colorSpace:
                foundFormat = true;
                if (pCreateInfo->imageColorSpace == pDevice->pSurfaceFormats[i].colorSpace) {
                    foundMatch = true;
                    break;
                }
            } else {
                if (pCreateInfo->imageColorSpace == pDevice->pSurfaceFormats[i].colorSpace) {
                    foundColorSpace = true;
                }
            }
        }
        if (!foundMatch) {
            if (!foundFormat) {
                if (!foundColorSpace) {
                    skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device,
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
                    skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device,
                                          "VkDevice",
                                          SWAPCHAIN_CREATE_SWAP_BAD_IMG_FORMAT,
                                          "%s() called with a non-supported "
                                          "pCreateInfo->imageFormat (i.e. %d).",
                                          fn, pCreateInfo->imageFormat);
                }
            } else if (!foundColorSpace) {
                skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                                      SWAPCHAIN_CREATE_SWAP_BAD_IMG_COLOR_SPACE,
                                      "%s() called with a non-supported "
                                      "pCreateInfo->imageColorSpace (i.e. %d).",
                                      fn, pCreateInfo->imageColorSpace);
            }
        }
    }
    if (!pDevice->presentModeCount) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                              SWAPCHAIN_CREATE_SWAP_WITHOUT_QUERY,
                              "%s() called before calling "
                              "vkGetSurfacePresentModesKHR().",
                              fn);
    } else {
        // Validate pCreateInfo->presentMode against
        // vkGetSurfacePresentModesKHR():
        bool foundMatch = false;
        for (int i = 0 ; i < pDevice->presentModeCount ; i++) {
            if (pDevice->pPresentModes[i] == pCreateInfo->presentMode) {
                foundMatch = true;
                break;
            }
        }
        if (!foundMatch) {
            skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                                  SWAPCHAIN_CREATE_SWAP_BAD_PRESENT_MODE,
                                  "%s() called with a non-supported "
                                  "pCreateInfo->presentMode (i.e. %s).",
                                  fn,
                                  presentModeStr(pCreateInfo->presentMode));
        }
    }

    // TODO: Validate the following values:
    // - pCreateInfo->sharingMode
    // - pCreateInfo->queueFamilyCount
    // - pCreateInfo->pQueueFamilyIndices
    // - pCreateInfo->oldSwapchain

    return skipCall;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, VkSwapchainKHR* pSwapchain)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = validateCreateSwapchainKHR(device, pCreateInfo,
                                                   pSwapchain);

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = device_dispatch_table(device)->CreateSwapchainKHR(
                device, pCreateInfo, pSwapchain);

        if (result == VK_SUCCESS) {
            // Remember the swapchain's handle, and link it to the device:
            SwpDevice *pDevice = &deviceMap[device];

            swapchainMap[pSwapchain->handle].swapchain = *pSwapchain;
            pDevice->swapchains[pSwapchain->handle] =
                &swapchainMap[pSwapchain->handle];
            swapchainMap[pSwapchain->handle].pDevice = pDevice;
            swapchainMap[pSwapchain->handle].imageCount = 0;
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_DEVICE,
                                            device,
                                            "VkDevice");
    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the "
                              VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME,
                              "extension was not enabled for this VkDevice.",
                              __FUNCTION__);
    }

    // Regardless of skipCall value, do some internal cleanup:
    SwpSwapchain *pSwapchain = &swapchainMap[swapchain.handle];
    if (pSwapchain) {
        // Delete the SwpSwapchain associated with this swapchain:
        if (pSwapchain->pDevice) {
            pSwapchain->pDevice->swapchains.erase(swapchain.handle);
            if (device != pSwapchain->pDevice->device) {
                LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                          SWAPCHAIN_DESTROY_SWAP_DIFF_DEVICE,
                          "%s() called with a different VkDevice than the "
                          "VkSwapchainKHR was created with.",
                          __FUNCTION__);
            }
        }
        if (pSwapchain->imageCount) {
            pSwapchain->images.clear();
        }
        swapchainMap.erase(swapchain.handle);
    } else {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_SWAPCHAIN_KHR,
                                            swapchain.handle,
                                            "VkSwapchainKHR");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        VkResult result = device_dispatch_table(device)->DestroySwapchainKHR(device, swapchain);
        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pCount, VkImage* pSwapchainImages)
{
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_DEVICE,
                                            device,
                                            "VkDevice");
    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the "
                              VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME,
                              "extension was not enabled for this VkDevice.",
                              __FUNCTION__);
    }
    SwpSwapchain *pSwapchain = &swapchainMap[swapchain.handle];
    if (!pSwapchain) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_SWAPCHAIN_KHR,
                                            swapchain.handle,
                                            "VkSwapchainKHR");
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = device_dispatch_table(device)->GetSwapchainImagesKHR(
                device, swapchain, pCount, pSwapchainImages);

// TBD: Should we validate that this function was called once with
// pSwapchainImages set to NULL (and record pCount at that time), and then
// called again with a non-NULL pSwapchainImages?
        if ((result == VK_SUCCESS) && pSwapchain &&pSwapchainImages &&
            pCount && (*pCount > 0)) {
            // Record the images and their state:
            if (pSwapchain) {
                pSwapchain->imageCount = *pCount;
                for (int i = 0 ; i < *pCount ; i++) {
                    pSwapchain->images[i].image = pSwapchainImages[i];
                    pSwapchain->images[i].pSwapchain = pSwapchain;
                    pSwapchain->images[i].ownedByApp = false;
                }
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VkResult VKAPI vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, uint32_t* pImageIndex)
{
// TODO: Record/update the state of the swapchain, in case an error occurs
// (e.g. VK_ERROR_OUT_OF_DATE_KHR).
    VkResult result = VK_SUCCESS;
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    // Validate that a valid VkDevice was used, and that the device
    // extension was enabled:
    SwpDevice *pDevice = &deviceMap[device];
    if (!pDevice) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_DEVICE,
                                            device,
                                            "VkDevice");
    } else if (!pDevice->deviceSwapchainExtensionEnabled) {
        skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE, device, "VkDevice",
                              SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                              "%s() called even though the "
                              VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME,
                              "extension was not enabled for this VkDevice.",
                              __FUNCTION__);
    }
    // Validate that a valid VkSwapchainKHR was used:
    SwpSwapchain *pSwapchain = &swapchainMap[swapchain.handle];
    if (!pSwapchain) {
        skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_SWAPCHAIN_KHR,
                                            swapchain.handle,
                                            "VkSwapchainKHR");
    } else {
        // Look to see if the application is trying to own too many images at
        // the same time (i.e. not leave any to display):
        int imagesOwnedByApp = 0;
        for (int i = 0 ; i < pSwapchain->imageCount ; i++) {
            if (pSwapchain->images[i].ownedByApp) {
                imagesOwnedByApp++;
            }
        }
        if (imagesOwnedByApp >= (pSwapchain->imageCount - 1)) {
            skipCall |= LOG_PERF_WARNING(VK_OBJECT_TYPE_SWAPCHAIN_KHR,
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
        result = device_dispatch_table(device)->AcquireNextImageKHR(
                device, swapchain, timeout, semaphore, pImageIndex);

        if (((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)) &&
            pSwapchain) {
            // Change the state of the image (now owned by the application):
            pSwapchain->images[*pImageIndex].ownedByApp = true;
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueuePresentKHR(VkQueue queue, VkPresentInfoKHR* pPresentInfo)
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

    for (int i = 0; i < pPresentInfo->swapchainCount ; i++) {
        int index = pPresentInfo->imageIndices[i];
        SwpSwapchain *pSwapchain =
            &swapchainMap[pPresentInfo->swapchains[i].handle];
        if (pSwapchain) {
            if (!pSwapchain->pDevice->deviceSwapchainExtensionEnabled) {
                skipCall |= LOG_ERROR(VK_OBJECT_TYPE_DEVICE,
                                      pSwapchain->pDevice, "VkDevice",
                                      SWAPCHAIN_EXT_NOT_ENABLED_BUT_USED,
                                      "%s() called even though the "
                                      VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME,
                                      "extension was not enabled for this "
                                      "VkDevice.",
                                      __FUNCTION__);
            }
            if (index >= pSwapchain->imageCount) {
                skipCall |= LOG_ERROR(VK_OBJECT_TYPE_SWAPCHAIN_KHR,
                                      pPresentInfo->swapchains[i].handle,
                                      "VkSwapchainKHR",
                                      SWAPCHAIN_INDEX_TOO_LARGE,
                                      "%s() called for an index that is too "
                                      "large (i.e. %d).  There are only %d "
                                      "images in this VkSwapchainKHR.\n",
                                      __FUNCTION__, index,
                                      pSwapchain->imageCount);
            } else {
                if (!pSwapchain->images[index].ownedByApp) {
                    skipCall |= LOG_ERROR(VK_OBJECT_TYPE_SWAPCHAIN_KHR,
                                          pPresentInfo->swapchains[i].handle,
                                          "VkSwapchainKHR",
                                          SWAPCHAIN_INDEX_NOT_IN_USE,
                                          "%s() returned an index (i.e. %d) "
                                          "for an image that is not owned by "
                                          "the application.",
                                          __FUNCTION__, index);
                }
            }
        } else {
            skipCall |= LOG_ERROR_NON_VALID_OBJ(VK_OBJECT_TYPE_SWAPCHAIN_KHR,
                                                pPresentInfo->swapchains[i].handle,
                                                "VkSwapchainKHR");
        }
    }

    if (VK_FALSE == skipCall) {
        // Call down the call chain:
        result = device_dispatch_table(queue)->QueuePresentKHR(queue,
                                                               pPresentInfo);

        if ((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR)) {
            for (int i = 0; i < pPresentInfo->swapchainCount ; i++) {
                int index = pPresentInfo->imageIndices[i];
                SwpSwapchain *pSwapchain =
                    &swapchainMap[pPresentInfo->swapchains[i].handle];
                if (pSwapchain) {
                    // Change the state of the image (no longer owned by the
                    // application):
                    pSwapchain->images[index].ownedByApp = false;
                }
            }
        }

        return result;
    }
    return VK_ERROR_VALIDATION_FAILED;
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

VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(VkInstance instance, VkFlags msgFlags, const PFN_vkDbgMsgCallback pfnMsgCallback, void* pUserData, VkDbgMsgCallback* pMsgCallback)
{
    VkResult result = instance_dispatch_table(instance)->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    if (VK_SUCCESS == result) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        result = layer_create_msg_callback(my_data->report_data, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgDestroyMsgCallback(VkInstance instance, VkDbgMsgCallback msgCallback)
{
    VkResult result = instance_dispatch_table(instance)->DbgDestroyMsgCallback(instance, msgCallback);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(my_data->report_data, msgCallback);
    return result;
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    PFN_vkVoidFunction addr;
    if (device == VK_NULL_HANDLE) {
        return NULL;
    }

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp("vkGetDeviceProcAddr", funcName)) {
        initDeviceTable((const VkBaseLayerObject *) device);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }

    addr = layer_intercept_proc(funcName);
    if (addr)
        return addr;

    VkLayerDispatchTable *pDisp =  device_dispatch_table(device);
    if (deviceMap.size() != 0 &&
        deviceMap[pDisp].deviceSwapchainExtensionEnabled)
    {
        if (!strcmp("vkGetSurfacePropertiesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetSurfacePropertiesKHR);
        if (!strcmp("vkGetSurfaceFormatsKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetSurfaceFormatsKHR);
        if (!strcmp("vkGetSurfacePresentModesKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetSurfacePresentModesKHR);
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

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    PFN_vkVoidFunction addr;
    if (instance == VK_NULL_HANDLE) {
        return NULL;
    }

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp("vkGetInstanceProcAddr", funcName)) {
        initInstanceTable((const VkBaseLayerObject *) instance);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }

    addr = layer_intercept_instance_proc(funcName);
    if (addr)
        return addr;

    VkLayerInstanceDispatchTable* pTable = instance_dispatch_table(instance);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    addr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (addr) {
        return addr;
    }

    if (instanceMap.size() != 0 &&
        instanceMap[instance].swapchainExtensionEnabled)
    {
        if (!strcmp("vkGetPhysicalDeviceSurfaceSupportKHR", funcName))
            return reinterpret_cast<PFN_vkVoidFunction>(vkGetPhysicalDeviceSurfaceSupportKHR);
    }

    if (pTable->GetInstanceProcAddr == NULL)
        return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}

