/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
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
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan     <mike@LunarG.com>
 * Author: Tobin Ehlis      <tobin@lunarg.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <memory>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Wwrite-strings"
#endif
#include "vk_struct_size_helper.h"
#include "device_limits.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_enum_validate_helper.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"

namespace device_limits {

// This struct will be stored in a map hashed by the dispatchable object
struct layer_data {
    VkInstance instance;

    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerDispatchTable *device_dispatch_table;
    VkLayerInstanceDispatchTable *instance_dispatch_table;
    // Track state of each instance
    unique_ptr<INSTANCE_STATE> instanceState;
    unique_ptr<PHYSICAL_DEVICE_STATE> physicalDeviceState;
    VkPhysicalDeviceFeatures actualPhysicalDeviceFeatures;
    VkPhysicalDeviceFeatures requestedPhysicalDeviceFeatures;

    // Track physical device per logical device
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    // Vector indices correspond to queueFamilyIndex
    vector<unique_ptr<VkQueueFamilyProperties>> queueFamilyProperties;

    layer_data()
        : report_data(nullptr), device_dispatch_table(nullptr), instance_dispatch_table(nullptr), instanceState(nullptr),
          physicalDeviceState(nullptr), actualPhysicalDeviceFeatures(), requestedPhysicalDeviceFeatures(), physicalDevice(){};
};

static unordered_map<void *, layer_data *> layer_data_map;

// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

static void init_device_limits(layer_data *my_data, const VkAllocationCallbacks *pAllocator) {

    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "lunarg_device_limits");

    if (!globalLockInitialized) {
        // TODO/TBD: Need to delete this mutex sometime.  How???  One
        // suggestion is to call this during vkCreateInstance(), and then we
        // can clean it up during vkDestroyInstance().  However, that requires
        // that the layer have per-instance locks.  We need to come back and
        // address this soon.
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

static const VkExtensionProperties instance_extensions[] = {{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION}};

static const VkLayerProperties global_layer = {
    "VK_LAYER_LUNARG_device_limits", VK_LAYER_API_VERSION, 1, "LunarG Validation Layer",
};

VKAPI_ATTR VkResult VKAPI_CALL
CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
        return result;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    my_data->instance = *pInstance;
    my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, my_data->instance_dispatch_table, fpGetInstanceProcAddr);

    my_data->report_data = debug_report_create_instance(my_data->instance_dispatch_table, *pInstance,
                                                        pCreateInfo->enabledExtensionCount, pCreateInfo->ppEnabledExtensionNames);

    init_device_limits(my_data, pAllocator);
    my_data->instanceState = unique_ptr<INSTANCE_STATE>(new INSTANCE_STATE());

    return VK_SUCCESS;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VKAPI_ATTR void VKAPI_CALL
DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyInstance(instance, pAllocator);

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
    if (layer_data_map.empty()) {
        // Release mutex when destroying last instance.
        loader_platform_thread_delete_mutex(&globalLock);
        globalLockInitialized = 0;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices) {
    bool skipCall = false;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    if (my_data->instanceState) {
        // For this instance, flag when vkEnumeratePhysicalDevices goes to QUERY_COUNT and then QUERY_DETAILS
        if (NULL == pPhysicalDevices) {
            my_data->instanceState->vkEnumeratePhysicalDevicesState = QUERY_COUNT;
        } else {
            if (UNCALLED == my_data->instanceState->vkEnumeratePhysicalDevicesState) {
                // Flag warning here. You can call this without having queried the count, but it may not be
                // robust on platforms with multiple physical devices.
                skipCall |=
                    log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, 0,
                            __LINE__, DEVLIMITS_MISSING_QUERY_COUNT, "DL",
                            "Call sequence has vkEnumeratePhysicalDevices() w/ non-NULL pPhysicalDevices. You should first "
                            "call vkEnumeratePhysicalDevices() w/ NULL pPhysicalDevices to query pPhysicalDeviceCount.");
            } // TODO : Could also flag a warning if re-calling this function in QUERY_DETAILS state
            else if (my_data->instanceState->physicalDevicesCount != *pPhysicalDeviceCount) {
                // Having actual count match count from app is not a requirement, so this can be a warning
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                                    "Call to vkEnumeratePhysicalDevices() w/ pPhysicalDeviceCount value %u, but actual count "
                                    "supported by this instance is %u.",
                                    *pPhysicalDeviceCount, my_data->instanceState->physicalDevicesCount);
            }
            my_data->instanceState->vkEnumeratePhysicalDevicesState = QUERY_DETAILS;
        }
        if (skipCall)
            return VK_ERROR_VALIDATION_FAILED_EXT;
        VkResult result =
            my_data->instance_dispatch_table->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
        if (NULL == pPhysicalDevices) {
            my_data->instanceState->physicalDevicesCount = *pPhysicalDeviceCount;
        } else { // Save physical devices
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(pPhysicalDevices[i]), layer_data_map);
                phy_dev_data->physicalDeviceState = unique_ptr<PHYSICAL_DEVICE_STATE>(new PHYSICAL_DEVICE_STATE());
                // Init actual features for each physical device
                my_data->instance_dispatch_table->GetPhysicalDeviceFeatures(pPhysicalDevices[i],
                                                                            &(phy_dev_data->actualPhysicalDeviceFeatures));
            }
        }
        return result;
    } else {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, 0, __LINE__,
                DEVLIMITS_INVALID_INSTANCE, "DL", "Invalid instance (0x%" PRIxLEAST64 ") passed into vkEnumeratePhysicalDevices().",
                (uint64_t)instance);
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VKAPI_ATTR void VKAPI_CALL
GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures) {
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceFeaturesState = QUERY_DETAILS;
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL
GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties) {
    get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map)
        ->instance_dispatch_table->GetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
GetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling,
                                       VkImageUsageFlags usage, VkImageCreateFlags flags,
                                       VkImageFormatProperties *pImageFormatProperties) {
    return get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map)
        ->instance_dispatch_table->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags,
                                                                          pImageFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL
GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceProperties(physicalDevice, pProperties);
}

VKAPI_ATTR void VKAPI_CALL
GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                       VkQueueFamilyProperties *pQueueFamilyProperties) {
    bool skipCall = false;
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    if (phy_dev_data->physicalDeviceState) {
        if (NULL == pQueueFamilyProperties) {
            phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_COUNT;
        } else {
            // Verify that for each physical device, this function is called first with NULL pQueueFamilyProperties ptr in order to
            // get count
            if (UNCALLED == phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
                skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_MISSING_QUERY_COUNT, "DL",
                                    "Call sequence has vkGetPhysicalDeviceQueueFamilyProperties() w/ non-NULL "
                                    "pQueueFamilyProperties. You should first call vkGetPhysicalDeviceQueueFamilyProperties() w/ "
                                    "NULL pQueueFamilyProperties to query pCount.");
            }
            // Then verify that pCount that is passed in on second call matches what was returned
            if (phy_dev_data->physicalDeviceState->queueFamilyPropertiesCount != *pCount) {

                // TODO: this is not a requirement of the Valid Usage section for vkGetPhysicalDeviceQueueFamilyProperties, so
                // provide as warning
                skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                                    "Call to vkGetPhysicalDeviceQueueFamilyProperties() w/ pCount value %u, but actual count "
                                    "supported by this physicalDevice is %u.",
                                    *pCount, phy_dev_data->physicalDeviceState->queueFamilyPropertiesCount);
            }
            phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_DETAILS;
        }
        if (skipCall)
            return;
        phy_dev_data->instance_dispatch_table->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pCount,
                                                                                      pQueueFamilyProperties);
        if (NULL == pQueueFamilyProperties) {
            phy_dev_data->physicalDeviceState->queueFamilyPropertiesCount = *pCount;
        } else { // Save queue family properties
            phy_dev_data->queueFamilyProperties.reserve(*pCount);
            for (uint32_t i = 0; i < *pCount; i++) {
                phy_dev_data->queueFamilyProperties.emplace_back(new VkQueueFamilyProperties(pQueueFamilyProperties[i]));
            }
        }
        return;
    } else {
        log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
                __LINE__, DEVLIMITS_INVALID_PHYSICAL_DEVICE, "DL",
                "Invalid physicalDevice (0x%" PRIxLEAST64 ") passed into vkGetPhysicalDeviceQueueFamilyProperties().",
                (uint64_t)physicalDevice);
    }
}

VKAPI_ATTR void VKAPI_CALL
GetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
    get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map)
        ->instance_dispatch_table->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

VKAPI_ATTR void VKAPI_CALL
GetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
                                             VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling,
                                             uint32_t *pNumProperties, VkSparseImageFormatProperties *pProperties) {
    get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map)
        ->instance_dispatch_table->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage,
                                                                                tiling, pNumProperties, pProperties);
}

VKAPI_ATTR void VKAPI_CALL
CmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports) {
    bool skipCall = false;
    /* TODO: Verify viewportCount < maxViewports from VkPhysicalDeviceLimits */
    if (!skipCall) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
        my_data->device_dispatch_table->CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
    }
}

VKAPI_ATTR void VKAPI_CALL
CmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors) {
    bool skipCall = false;
    /* TODO: Verify scissorCount < maxViewports from VkPhysicalDeviceLimits */
    /* TODO: viewportCount and scissorCount must match at draw time */
    if (!skipCall) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
        my_data->device_dispatch_table->CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
    }
}

// Verify that features have been queried and verify that requested features are available
static bool validate_features_request(layer_data *phy_dev_data) {
    bool skipCall = false;
    // Verify that all of the requested features are available
    // Get ptrs into actual and requested structs and if requested is 1 but actual is 0, request is invalid
    VkBool32 *actual = (VkBool32 *)&(phy_dev_data->actualPhysicalDeviceFeatures);
    VkBool32 *requested = (VkBool32 *)&(phy_dev_data->requestedPhysicalDeviceFeatures);
    // TODO : This is a nice, compact way to loop through struct, but a bad way to report issues
    //  Need to provide the struct member name with the issue. To do that seems like we'll
    //  have to loop through each struct member which should be done w/ codegen to keep in synch.
    uint32_t errors = 0;
    uint32_t totalBools = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
    for (uint32_t i = 0; i < totalBools; i++) {
        if (requested[i] > actual[i]) {
            skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_FEATURE_REQUESTED,
                                "DL", "While calling vkCreateDevice(), requesting feature #%u in VkPhysicalDeviceFeatures struct, "
                                      "which is not available on this device.",
                                i);
            errors++;
        }
    }
    if (errors && (UNCALLED == phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceFeaturesState)) {
        // If user didn't request features, notify them that they should
        // TODO: Verify this against the spec. I believe this is an invalid use of the API and should return an error
        skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_FEATURE_REQUESTED, "DL",
                            "You requested features that are unavailable on this device. You should first query feature "
                            "availability by calling vkGetPhysicalDeviceFeatures().");
    }
    return skipCall;
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo *pCreateInfo,
             const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    bool skipCall = false;
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    // First check is app has actually requested queueFamilyProperties
    if (!phy_dev_data->physicalDeviceState) {
        skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_MUST_QUERY_COUNT, "DL",
                            "Invalid call to vkCreateDevice() w/o first calling vkEnumeratePhysicalDevices().");
    } else if (QUERY_DETAILS != phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
        // TODO: This is not called out as an invalid use in the spec so make more informative recommendation.
        skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST,
                            "DL", "Call to vkCreateDevice() w/o first calling vkGetPhysicalDeviceQueueFamilyProperties().");
    } else {
        // Check that the requested queue properties are valid
        for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
            uint32_t requestedIndex = pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex;
            if (phy_dev_data->queueFamilyProperties.size() <=
                requestedIndex) { // requested index is out of bounds for this physical device
                skipCall |= log_msg(
                    phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
                    __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
                    "Invalid queue create request in vkCreateDevice(). Invalid queueFamilyIndex %u requested.", requestedIndex);
            } else if (pCreateInfo->pQueueCreateInfos[i].queueCount >
                       phy_dev_data->queueFamilyProperties[requestedIndex]->queueCount) {
                skipCall |=
                    log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST,
                            "DL", "Invalid queue create request in vkCreateDevice(). QueueFamilyIndex %u only has %u queues, but "
                                  "requested queueCount is %u.",
                            requestedIndex, phy_dev_data->queueFamilyProperties[requestedIndex]->queueCount,
                            pCreateInfo->pQueueCreateInfos[i].queueCount);
            }
        }
    }
    // Check that any requested features are available
    if (pCreateInfo->pEnabledFeatures) {
        phy_dev_data->requestedPhysicalDeviceFeatures = *(pCreateInfo->pEnabledFeatures);
        skipCall |= validate_features_request(phy_dev_data);
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(phy_dev_data->instance, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    my_device_data->device_dispatch_table = new VkLayerDispatchTable;
    layer_init_device_dispatch_table(*pDevice, my_device_data->device_dispatch_table, fpGetDeviceProcAddr);
    my_device_data->report_data = layer_debug_report_create_device(phy_dev_data->report_data, *pDevice);
    my_device_data->physicalDevice = gpu;

    // Get physical device properties for this device
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceProperties(gpu, &(my_device_data->physicalDeviceProperties));
    return result;
}

VKAPI_ATTR void VKAPI_CALL
DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    // Free device lifetime allocations
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_device_data = get_my_data_ptr(key, layer_data_map);
    my_device_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    delete my_device_data->device_dispatch_table;
    layer_data_map.erase(key);
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                 const VkAllocationCallbacks *pAllocator,
                 VkRenderPass *pRenderPass) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    uint32_t max_color_attachments = dev_data->physicalDeviceProperties.limits.maxColorAttachments;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        if (pCreateInfo->pSubpasses[i].colorAttachmentCount > max_color_attachments) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                 reinterpret_cast<uint64_t>(device), __LINE__, DEVLIMITS_INVALID_ATTACHMENT_COUNT, "DL",
                                 "Cannot create a render pass with %d color attachments. Max is %d.",
                                 pCreateInfo->pSubpasses[i].colorAttachmentCount, max_color_attachments);
        }
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    return dev_data->device_dispatch_table->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
                  const VkAllocationCallbacks *pAllocator,
                  VkCommandPool *pCommandPool) {
    // TODO : Verify that requested QueueFamilyIndex for this pool exists
    VkResult result = get_my_data_ptr(get_dispatch_key(device), layer_data_map)
                          ->device_dispatch_table->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
    return result;
}

VKAPI_ATTR void VKAPI_CALL
DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)
        ->device_dispatch_table->DestroyCommandPool(device, commandPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL
ResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
    VkResult result = get_my_data_ptr(get_dispatch_key(device), layer_data_map)
                          ->device_dispatch_table->ResetCommandPool(device, commandPool, flags);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pCreateInfo, VkCommandBuffer *pCommandBuffer) {
    VkResult result = get_my_data_ptr(get_dispatch_key(device), layer_data_map)
                          ->device_dispatch_table->AllocateCommandBuffers(device, pCreateInfo, pCommandBuffer);
    return result;
}

VKAPI_ATTR void VKAPI_CALL
FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count, const VkCommandBuffer *pCommandBuffers) {
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)
        ->device_dispatch_table->FreeCommandBuffers(device, commandPool, count, pCommandBuffers);
}

VKAPI_ATTR VkResult VKAPI_CALL
BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    bool skipCall = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(dev_data->physicalDevice), layer_data_map);
    const VkCommandBufferInheritanceInfo *pInfo = pBeginInfo->pInheritanceInfo;
    if (phy_dev_data->actualPhysicalDeviceFeatures.inheritedQueries == VK_FALSE && pInfo && pInfo->occlusionQueryEnable != VK_FALSE) {
        skipCall |= log_msg(
            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
            reinterpret_cast<uint64_t>(commandBuffer), __LINE__, DEVLIMITS_INVALID_INHERITED_QUERY, "DL",
            "Cannot set inherited occlusionQueryEnable in vkBeginCommandBuffer() when device does not support inheritedQueries.");
    }
    if (phy_dev_data->actualPhysicalDeviceFeatures.inheritedQueries != VK_FALSE && pInfo && pInfo->occlusionQueryEnable != VK_FALSE &&
        !validate_VkQueryControlFlagBits(VkQueryControlFlagBits(pInfo->queryFlags))) {
        skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(commandBuffer), __LINE__, DEVLIMITS_INVALID_INHERITED_QUERY, "DL",
                            "Cannot enable in occlusion queries in vkBeginCommandBuffer() and set queryFlags to %d which is not a "
                            "valid combination of VkQueryControlFlagBits.",
                            pInfo->queryFlags);
    }
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    if (!skipCall)
        result = dev_data->device_dispatch_table->BeginCommandBuffer(commandBuffer, pBeginInfo);
    return result;
}

VKAPI_ATTR void VKAPI_CALL
GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) {
    bool skipCall = false;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkPhysicalDevice gpu = dev_data->physicalDevice;
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    if (queueFamilyIndex >=
        phy_dev_data->queueFamilyProperties.size()) { // requested index is out of bounds for this physical device
        skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST,
                            "DL", "Invalid queueFamilyIndex %u requested in vkGetDeviceQueue().", queueFamilyIndex);
    } else if (queueIndex >= phy_dev_data->queueFamilyProperties[queueFamilyIndex]->queueCount) {
        skipCall |= log_msg(
            phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__,
            DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
            "Invalid queue request in vkGetDeviceQueue(). QueueFamilyIndex %u only has %u queues, but requested queueIndex is %u.",
            queueFamilyIndex, phy_dev_data->queueFamilyProperties[queueFamilyIndex]->queueCount, queueIndex);
    }
    if (!skipCall)
        dev_data->device_dispatch_table->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

VKAPI_ATTR void VKAPI_CALL
UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites,
                       uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies) {
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skipCall = false;

    for (uint32_t i = 0; i < descriptorWriteCount; i++) {
        if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
            (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)) {
            VkDeviceSize uniformAlignment = dev_data->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
            for (uint32_t j = 0; j < pDescriptorWrites[i].descriptorCount; j++) {
                if (vk_safe_modulo(pDescriptorWrites[i].pBufferInfo[j].offset, uniformAlignment) != 0) {
                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__,
                                        DEVLIMITS_INVALID_UNIFORM_BUFFER_OFFSET, "DL",
                                        "vkUpdateDescriptorSets(): pDescriptorWrites[%d].pBufferInfo[%d].offset (0x%" PRIxLEAST64
                                        ") must be a multiple of device limit minUniformBufferOffsetAlignment 0x%" PRIxLEAST64,
                                        i, j, pDescriptorWrites[i].pBufferInfo[j].offset, uniformAlignment);
                }
            }
        } else if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                   (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
            VkDeviceSize storageAlignment = dev_data->physicalDeviceProperties.limits.minStorageBufferOffsetAlignment;
            for (uint32_t j = 0; j < pDescriptorWrites[i].descriptorCount; j++) {
                if (vk_safe_modulo(pDescriptorWrites[i].pBufferInfo[j].offset, storageAlignment) != 0) {
                    skipCall |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__,
                                        DEVLIMITS_INVALID_STORAGE_BUFFER_OFFSET, "DL",
                                        "vkUpdateDescriptorSets(): pDescriptorWrites[%d].pBufferInfo[%d].offset (0x%" PRIxLEAST64
                                        ") must be a multiple of device limit minStorageBufferOffsetAlignment 0x%" PRIxLEAST64,
                                        i, j, pDescriptorWrites[i].pBufferInfo[j].offset, storageAlignment);
                }
            }
        }
    }
    if (!skipCall) {
        dev_data->device_dispatch_table->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                                                              pDescriptorCopies);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                             const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult res = my_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (VK_SUCCESS == res) {
        res = layer_create_msg_callback(my_data->report_data, false, pCreateInfo, pAllocator, pMsgCallback);
    }
    return res;
}

VKAPI_ATTR void VKAPI_CALL
DestroyDebugReportCallbackEXT(VkInstance instance,
                              VkDebugReportCallbackEXT msgCallback,
                              const VkAllocationCallbacks *pAllocator) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                      size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix,
                                                            pMsg);
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL
EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                   const char *pLayerName, uint32_t *pCount,
                                   VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(0, nullptr, pCount, pProperties);

    assert(physicalDevice);

    dispatch_key key = get_dispatch_key(physicalDevice);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    return my_data->instance_dispatch_table->EnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pCount, pProperties);
}

static PFN_vkVoidFunction
intercept_core_instance_command(const char *name);

static PFN_vkVoidFunction
intercept_core_device_command(const char *name);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
GetDeviceProcAddr(VkDevice dev, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_device_command(funcName);
    if (proc)
        return proc;

    assert(dev);

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);
    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    {
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(dev, funcName);
    }
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_instance_command(funcName);
    if (!proc)
        intercept_core_device_command(funcName);
    if (proc)
        return proc;

    layer_data *my_data;

    assert(instance);
    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    proc = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (proc)
        return proc;

    {
        VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(instance, funcName);
    }
}

static PFN_vkVoidFunction
intercept_core_instance_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_instance_commands[] = {
        { "vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr) },
        { "vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr) },
        { "vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance) },
        { "vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance) },
        { "vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(CreateDevice) },
        { "vkEnumeratePhysicalDevices", reinterpret_cast<PFN_vkVoidFunction>(EnumeratePhysicalDevices) },
        { "vkGetPhysicalDeviceFeatures", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceFeatures) },
        { "vkGetPhysicalDeviceFormatProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceFormatProperties) },
        { "vkGetPhysicalDeviceImageFormatProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceImageFormatProperties) },
        { "vkGetPhysicalDeviceProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceProperties) },
        { "vkGetPhysicalDeviceQueueFamilyProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceQueueFamilyProperties) },
        { "vkGetPhysicalDeviceMemoryProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceMemoryProperties) },
        { "vkGetPhysicalDeviceSparseImageFormatProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSparseImageFormatProperties) },
        { "vkEnumerateInstanceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceLayerProperties) },
        { "vkEnumerateDeviceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceLayerProperties) },
        { "vkEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceExtensionProperties) },
        { "vkEnumerateDeviceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceExtensionProperties) },
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_instance_commands); i++) {
        if (!strcmp(core_instance_commands[i].name, name))
            return core_instance_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction
intercept_core_device_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_device_commands[] = {
        { "vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr) },
        { "vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(DestroyDevice) },
        { "vkGetDeviceQueue", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceQueue) },
        { "vkCreateRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CreateRenderPass) },
        { "vkCreateCommandPool", reinterpret_cast<PFN_vkVoidFunction>(CreateCommandPool) },
        { "vkDestroyCommandPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyCommandPool) },
        { "vkResetCommandPool", reinterpret_cast<PFN_vkVoidFunction>(ResetCommandPool) },
        { "vkAllocateCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(AllocateCommandBuffers) },
        { "vkFreeCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(FreeCommandBuffers) },
        { "vkBeginCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(BeginCommandBuffer) },
        { "vkUpdateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(UpdateDescriptorSets) },
        { "vkCmdSetScissor", reinterpret_cast<PFN_vkVoidFunction>(CmdSetScissor) },
        { "vkCmdSetViewport", reinterpret_cast<PFN_vkVoidFunction>(CmdSetViewport) },
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_device_commands); i++) {
        if (!strcmp(core_device_commands[i].name, name))
            return core_device_commands[i].proc;
    }

    return nullptr;
}

} // namespace device_limits

// vk_layer_logging.h expects these to be defined

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                               const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {
    return device_limits::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDebugReportCallbackEXT(VkInstance instance,
                                VkDebugReportCallbackEXT msgCallback,
                                const VkAllocationCallbacks *pAllocator) {
    device_limits::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                        size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    device_limits::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

// loader-layer interface v0, just wrappers since there is only a layer

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return device_limits::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return device_limits::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties) {
    return device_limits::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return device_limits::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return device_limits::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return device_limits::GetInstanceProcAddr(instance, funcName);
}
