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
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <memory>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Wwrite-strings"
#endif
#include "vk_struct_size_helper.h"
#include "device_limits.h"
#include "vk_layer_config.h"
#include "vulkan/vk_debug_marker_layer.h"
#include "vk_layer_table.h"
#include "vk_layer_debug_marker_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"

struct devExts {
    bool debug_marker_enabled;
};

// This struct will be stored in a map hashed by the dispatchable object
struct layer_data {
    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerDispatchTable* device_dispatch_table;
    VkLayerInstanceDispatchTable* instance_dispatch_table;
    devExts device_extensions;
    // Track state of each instance
    unique_ptr<INSTANCE_STATE> instanceState;
    unique_ptr<PHYSICAL_DEVICE_STATE> physicalDeviceState;
    VkPhysicalDeviceFeatures actualPhysicalDeviceFeatures;
    VkPhysicalDeviceFeatures requestedPhysicalDeviceFeatures;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    // Track physical device per logical device
    VkPhysicalDevice physicalDevice;
    // Vector indices correspond to queueFamilyIndex
    vector<unique_ptr<VkQueueFamilyProperties>> queueFamilyProperties;

    layer_data() :
        report_data(nullptr),
        device_dispatch_table(nullptr),
        instance_dispatch_table(nullptr),
        device_extensions(),
        instanceState(nullptr),
        physicalDeviceState(nullptr),
        physicalDeviceProperties(),
        actualPhysicalDeviceFeatures(),
        requestedPhysicalDeviceFeatures(),
        physicalDevice()
    {};
};

static unordered_map<void *, layer_data *> layer_data_map;

static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);

// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

static void init_device_limits(layer_data *my_data, const VkAllocationCallbacks *pAllocator)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    VkDebugReportCallbackEXT callback;
    // initialize DeviceLimits options
    report_flags = getLayerOptionFlags("DeviceLimitsReportFlags", 0);
    getLayerOptionEnum("DeviceLimitsDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("DeviceLimitsLogFilename");
        log_output = getLayerLogOutput(option_str, "DeviceLimits");
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.flags = report_flags;
        dbgCreateInfo.pfnCallback = log_callback;
        dbgCreateInfo.pUserData = (void *) log_output;
        layer_create_msg_callback(my_data->report_data, &dbgCreateInfo, pAllocator, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (debug_action & VK_DBG_LAYER_ACTION_DEBUG_OUTPUT) {
        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.flags = report_flags;
        dbgCreateInfo.pfnCallback = win32_debug_output_msg;
        dbgCreateInfo.pUserData = NULL;
        layer_create_msg_callback(my_data->report_data, &dbgCreateInfo, pAllocator, &callback);
        my_data->logging_callback.push_back(callback);
    }

    if (!globalLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???  One
        // suggestion is to call this during vkCreateInstance(), and then we
        // can clean it up during vkDestroyInstance().  However, that requires
        // that the layer have per-instance locks.  We need to come back and
        // address this soon.
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }
}

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

static const VkLayerProperties dl_global_layers[] = {
    {
        "VK_LAYER_LUNARG_device_limits",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: Device Limits",
    }
};

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(dl_global_layers),
                                   dl_global_layers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
    VkResult result = pTable->CreateInstance(pCreateInfo, pAllocator, pInstance);

    if (result == VK_SUCCESS) {
        my_data->report_data = debug_report_create_instance(
                                   pTable,
                                   *pInstance,
                                   pCreateInfo->enabledExtensionCount,
                                   pCreateInfo->ppEnabledExtensionNames);

        init_device_limits(my_data, pAllocator);
        my_data->instanceState = unique_ptr<INSTANCE_STATE>(new INSTANCE_STATE());
    }
    return result;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
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

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    if (my_data->instanceState) {
        // For this instance, flag when vkEnumeratePhysicalDevices goes to QUERY_COUNT and then QUERY_DETAILS
        if (NULL == pPhysicalDevices) {
            my_data->instanceState->vkEnumeratePhysicalDevicesState = QUERY_COUNT;
        } else {
            if (UNCALLED == my_data->instanceState->vkEnumeratePhysicalDevicesState) {
                // Flag error here, shouldn't be calling this without having queried count
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, 0, __LINE__, DEVLIMITS_MUST_QUERY_COUNT, "DL",
                    "Invalid call sequence to vkEnumeratePhysicalDevices() w/ non-NULL pPhysicalDevices. You should first call vkEnumeratePhysicalDevices() w/ NULL pPhysicalDevices to query pPhysicalDeviceCount.");
            } // TODO : Could also flag a warning if re-calling this function in QUERY_DETAILS state
            else if (my_data->instanceState->physicalDevicesCount != *pPhysicalDeviceCount) {
                // TODO: Having actual count match count from app is not a requirement, so this can be a warning
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_WARN_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                    "Call to vkEnumeratePhysicalDevices() w/ pPhysicalDeviceCount value %u, but actual count supported by this instance is %u.", *pPhysicalDeviceCount, my_data->instanceState->physicalDevicesCount);
            }
            my_data->instanceState->vkEnumeratePhysicalDevicesState = QUERY_DETAILS;
        }
        if (skipCall)
            return VK_ERROR_VALIDATION_FAILED_EXT;
        VkResult result = my_data->instance_dispatch_table->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
        if (NULL == pPhysicalDevices) {
            my_data->instanceState->physicalDevicesCount = *pPhysicalDeviceCount;
        } else { // Save physical devices
            for (uint32_t i=0; i < *pPhysicalDeviceCount; i++) {
                layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(pPhysicalDevices[i]), layer_data_map);
                phy_dev_data->physicalDeviceState = unique_ptr<PHYSICAL_DEVICE_STATE>(new PHYSICAL_DEVICE_STATE());
                // Init actual features for each physical device
                my_data->instance_dispatch_table->GetPhysicalDeviceFeatures(pPhysicalDevices[i], &(phy_dev_data->actualPhysicalDeviceFeatures));
                my_data->instance_dispatch_table->GetPhysicalDeviceProperties(pPhysicalDevices[i], &(phy_dev_data->physicalDeviceProperties));
            }
        }
        return result;
    } else {
        log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, 0, __LINE__, DEVLIMITS_INVALID_INSTANCE, "DL",
            "Invalid instance (%#" PRIxLEAST64 ") passed into vkEnumeratePhysicalDevices().", (uint64_t)instance);
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures)
{
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceFeaturesState = QUERY_DETAILS;
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties)
{
    get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map)->instance_dispatch_table->GetPhysicalDeviceFormatProperties(
                                     physicalDevice, format, pFormatProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
{
    return get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map)->instance_dispatch_table->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties)
{
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceProperties(physicalDevice, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pCount, VkQueueFamilyProperties* pQueueFamilyProperties)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    if (phy_dev_data->physicalDeviceState) {
        if (NULL == pQueueFamilyProperties) {
            phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_COUNT;
        } else {
            // Verify that for each physical device, this function is called first with NULL pQueueFamilyProperties ptr in order to get count
            if (UNCALLED == phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
                skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_MUST_QUERY_COUNT, "DL",
                    "Invalid call sequence to vkGetPhysicalDeviceQueueFamilyProperties() w/ non-NULL pQueueFamilyProperties. You should first call vkGetPhysicalDeviceQueueFamilyProperties() w/ NULL pQueueFamilyProperties to query pCount.");
            }
            // Then verify that pCount that is passed in on second call matches what was returned
            if (phy_dev_data->physicalDeviceState->queueFamilyPropertiesCount != *pCount) {

                // TODO: this is not a requirement of the Valid Usage section for vkGetPhysicalDeviceQueueFamilyProperties, so provide as warning
                skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_WARN_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                    "Call to vkGetPhysicalDeviceQueueFamilyProperties() w/ pCount value %u, but actual count supported by this physicalDevice is %u.", *pCount, phy_dev_data->physicalDeviceState->queueFamilyPropertiesCount);
            }
            phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_DETAILS;
        }
        if (skipCall)
            return;
        phy_dev_data->instance_dispatch_table->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pCount, pQueueFamilyProperties);
        if (NULL == pQueueFamilyProperties) {
            phy_dev_data->physicalDeviceState->queueFamilyPropertiesCount = *pCount;
        } else { // Save queue family properties
            phy_dev_data->queueFamilyProperties.reserve(*pCount);
            for (uint32_t i=0; i < *pCount; i++) {
                phy_dev_data->queueFamilyProperties.emplace_back(new VkQueueFamilyProperties(pQueueFamilyProperties[i]));
            }
        }
        return;
    } else {
        log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_PHYSICAL_DEVICE, "DL",
            "Invalid physicalDevice (%#" PRIxLEAST64 ") passed into vkGetPhysicalDeviceQueueFamilyProperties().", (uint64_t)physicalDevice);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map)->instance_dispatch_table->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pNumProperties, VkSparseImageFormatProperties* pProperties)
{
    get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map)->instance_dispatch_table->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pNumProperties, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(
    VkCommandBuffer                         commandBuffer,
    uint32_t                                firstViewport,
    uint32_t                                viewportCount,
    const VkViewport*                       pViewports)
{
    VkBool32 skipCall = VK_FALSE;
    /* TODO: Verify viewportCount < maxViewports from VkPhysicalDeviceLimits */
    if (VK_FALSE == skipCall) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
        my_data->device_dispatch_table->CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(
    VkCommandBuffer                         commandBuffer,
    uint32_t                                firstScissor,
    uint32_t                                scissorCount,
    const VkRect2D*                         pScissors)
{
    VkBool32 skipCall = VK_FALSE;
    /* TODO: Verify scissorCount < maxViewports from VkPhysicalDeviceLimits */
    /* TODO: viewportCount and scissorCount must match at draw time */
    if (VK_FALSE == skipCall) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
        my_data->device_dispatch_table->CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
    }
}

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i;
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    my_data->device_extensions.debug_marker_enabled = false;

    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], DEBUG_MARKER_EXTENSION_NAME) == 0) {
            /* Found a matching extension name, mark it enabled and init dispatch table*/
            initDebugMarkerTable(device);
            my_data->device_extensions.debug_marker_enabled = true;
        }

    }
}

// Verify that features have been queried and verify that requested features are available
static VkBool32 validate_features_request(layer_data *phy_dev_data)
{
    VkBool32 skipCall = VK_FALSE;
    // Verify that all of the requested features are available
    // Get ptrs into actual and requested structs and if requested is 1 but actual is 0, request is invalid
    VkBool32* actual = (VkBool32*)&(phy_dev_data->actualPhysicalDeviceFeatures);
    VkBool32* requested = (VkBool32*)&(phy_dev_data->requestedPhysicalDeviceFeatures);
    // TODO : This is a nice, compact way to loop through struct, but a bad way to report issues
    //  Need to provide the struct member name with the issue. To do that seems like we'll
    //  have to loop through each struct member which should be done w/ codegen to keep in synch.
    uint32_t errors = 0;
    uint32_t totalBools = sizeof(VkPhysicalDeviceFeatures)/sizeof(VkBool32);
    for (uint32_t i = 0; i < totalBools; i++) {
        if (requested[i] > actual[i]) {
            skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_FEATURE_REQUESTED, "DL",
                "While calling vkCreateDevice(), requesting feature #%u in VkPhysicalDeviceFeatures struct, which is not available on this device.", i);
            errors++;
        }
    }
    if (errors && (UNCALLED == phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceFeaturesState)) {
        // If user didn't request features, notify them that they should
        // TODO: Verify this against the spec. I believe this is an invalid use of the API and should return an error
        skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_WARN_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_FEATURE_REQUESTED, "DL",
                "You requested features that are unavailable on this device. You should first query feature availability by calling vkGetPhysicalDeviceFeatures().");
    }
    return skipCall;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    // First check is app has actually requested queueFamilyProperties
    if (!phy_dev_data->physicalDeviceState) {
        skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_MUST_QUERY_COUNT, "DL",
            "Invalid call to vkCreateDevice() w/o first calling vkEnumeratePhysicalDevices().");
    } else if (QUERY_DETAILS != phy_dev_data->physicalDeviceState->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
        // TODO: This is not called out as an invalid use in the spec so make more informative recommendation.
        skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_WARN_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
            "Call to vkCreateDevice() w/o first calling vkGetPhysicalDeviceQueueFamilyProperties().");
    } else {
        // Check that the requested queue properties are valid
        for (uint32_t i=0; i<pCreateInfo->queueCreateInfoCount; i++) {
            uint32_t requestedIndex = pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex;
            if (phy_dev_data->queueFamilyProperties.size() <= requestedIndex) { // requested index is out of bounds for this physical device
                skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
                    "Invalid queue create request in vkCreateDevice(). Invalid queueFamilyIndex %u requested.", requestedIndex);
            } else if (pCreateInfo->pQueueCreateInfos[i].queueCount > phy_dev_data->queueFamilyProperties[requestedIndex]->queueCount) {
                skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
                    "Invalid queue create request in vkCreateDevice(). QueueFamilyIndex %u only has %u queues, but requested queueCount is %u.", requestedIndex, phy_dev_data->queueFamilyProperties[requestedIndex]->queueCount, pCreateInfo->pQueueCreateInfos[i].queueCount);
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

    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    VkResult result = my_device_data->device_dispatch_table->CreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS) {
        my_device_data->report_data = layer_debug_report_create_device(phy_dev_data->report_data, *pDevice);
        createDeviceRegisterExtensions(pCreateInfo, *pDevice);
        my_device_data->physicalDevice = gpu;
    }
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator)
{
    // Free device lifetime allocations
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_device_data = get_my_data_ptr(key, layer_data_map);
    my_device_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    tableDebugMarkerMap.erase(key);
    delete my_device_data->device_dispatch_table;
    layer_data_map.erase(key);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool)
{
    // TODO : Verify that requested QueueFamilyIndex for this pool exists
    VkResult result = get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->DestroyCommandPool(device, commandPool, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags)
{
    VkResult result = get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->ResetCommandPool(device, commandPool, flags);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pCreateInfo, VkCommandBuffer* pCommandBuffer)
{
    VkResult result = get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->AllocateCommandBuffers(device, pCreateInfo, pCommandBuffer);
    return result;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t count, const VkCommandBuffer* pCommandBuffers)
{
    get_my_data_ptr(get_dispatch_key(device), layer_data_map)->device_dispatch_table->FreeCommandBuffers(device, commandPool, count, pCommandBuffers);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkPhysicalDevice gpu = dev_data->physicalDevice;
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    if (queueFamilyIndex >= phy_dev_data->queueFamilyProperties.size()) { // requested index is out of bounds for this physical device
        skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
            "Invalid queueFamilyIndex %u requested in vkGetDeviceQueue().", queueFamilyIndex);
    } else if (queueIndex >= phy_dev_data->queueFamilyProperties[queueFamilyIndex]->queueCount) {
        skipCall |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
            "Invalid queue request in vkGetDeviceQueue(). QueueFamilyIndex %u only has %u queues, but requested queueIndex is %u.", queueFamilyIndex, phy_dev_data->queueFamilyProperties[queueFamilyIndex]->queueCount, queueIndex);
    }
    if (skipCall)
        return;
    dev_data->device_dispatch_table->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize dstOffset,
    VkDeviceSize dataSize,
    const uint32_t* pData)
{
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    // dstOffset is the byte offset into the buffer to start updating and must be a multiple of 4.
    if (dstOffset & 3) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
        if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "DL",
        "vkCmdUpdateBuffer parameter, VkDeviceSize dstOffset, is not a multiple of 4")) {
            return;
        }
    }

    // dataSize is the number of bytes to update, which must be a multiple of 4.
    if (dataSize & 3) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
        if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "DL",
        "vkCmdUpdateBuffer parameter, VkDeviceSize dataSize, is not a multiple of 4")) {
            return;
        }
    }

    dev_data->device_dispatch_table->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(
    VkCommandBuffer commandBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize dstOffset,
    VkDeviceSize size,
    uint32_t data)
{
    layer_data *dev_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    // dstOffset is the byte offset into the buffer to start filling and must be a multiple of 4.
    if (dstOffset & 3) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
        if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "DL",
        "vkCmdFillBuffer parameter, VkDeviceSize dstOffset, is not a multiple of 4")) {
            return;
        }
    }

    // size is the number of bytes to fill, which must be a multiple of 4.
    if (size & 3) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
        if (log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, 1, "DL",
        "vkCmdFillBuffer parameter, VkDeviceSize size, is not a multiple of 4")) {
            return;
        }
    }

    dev_data->device_dispatch_table->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(
        VkInstance                                      instance,
        const VkDebugReportCallbackCreateInfoEXT*       pCreateInfo,
        const VkAllocationCallbacks*                    pAllocator,
        VkDebugReportCallbackEXT*                       pMsgCallback)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult res = my_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (VK_SUCCESS == res) {
        res = layer_create_msg_callback(my_data->report_data, pCreateInfo, pAllocator, pMsgCallback);
    }
    return res;
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(
        VkInstance                                  instance,
        VkDebugReportCallbackEXT                 msgCallback,
        const VkAllocationCallbacks*                pAllocator)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
}

VK_LAYER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(
        VkInstance                                  instance,
        VkDebugReportFlagsEXT                       flags,
        VkDebugReportObjectTypeEXT               objType,
        uint64_t                                    object,
        size_t                                      location,
        int32_t                                     msgCode,
        const char*                                 pLayerPrefix,
        const char*                                 pMsg)
{
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char* funcName)
{
    if (dev == NULL)
        return NULL;

    layer_data *my_data;
    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        VkBaseLayerObject* wrapped_dev = (VkBaseLayerObject*) dev;
        my_data = get_my_data_ptr(get_dispatch_key(wrapped_dev->baseObject), layer_data_map);
        my_data->device_dispatch_table = new VkLayerDispatchTable;
        layer_initialize_dispatch_table(my_data->device_dispatch_table, wrapped_dev);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }
    my_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (PFN_vkVoidFunction) vkGetDeviceQueue;
    if (!strcmp(funcName, "CreateCommandPool"))
        return (PFN_vkVoidFunction) vkCreateCommandPool;
    if (!strcmp(funcName, "DestroyCommandPool"))
        return (PFN_vkVoidFunction) vkDestroyCommandPool;
    if (!strcmp(funcName, "ResetCommandPool"))
        return (PFN_vkVoidFunction) vkResetCommandPool;
    if (!strcmp(funcName, "vkAllocateCommandBuffers"))
        return (PFN_vkVoidFunction) vkAllocateCommandBuffers;
    if (!strcmp(funcName, "vkFreeCommandBuffers"))
        return (PFN_vkVoidFunction) vkFreeCommandBuffers;
    if (!strcmp(funcName, "vkCmdUpdateBuffer"))
        return (PFN_vkVoidFunction) vkCmdUpdateBuffer;
    if (!strcmp(funcName, "vkCmdFillBuffer"))
        return (PFN_vkVoidFunction) vkCmdFillBuffer;

    VkLayerDispatchTable* pTable = my_data->device_dispatch_table;
    {
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(dev, funcName);
    }
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    PFN_vkVoidFunction fptr;
    if (instance == NULL)
        return NULL;

    layer_data *my_data;
    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        VkBaseLayerObject* wrapped_inst = (VkBaseLayerObject*) instance;
        my_data = get_my_data_ptr(get_dispatch_key(wrapped_inst->baseObject), layer_data_map);
        my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
        layer_init_instance_dispatch_table(my_data->instance_dispatch_table, wrapped_inst);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }
    my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkEnumeratePhysicalDevices"))
        return (PFN_vkVoidFunction) vkEnumeratePhysicalDevices;
    if (!strcmp(funcName, "vkGetPhysicalDeviceFeatures"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFeatures;
    if (!strcmp(funcName, "vkGetPhysicalDeviceFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceFormatProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceImageFormatProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceQueueFamilyProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceQueueFamilyProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceMemoryProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceMemoryProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceSparseImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceSparseImageFormatProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;

    fptr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (fptr)
        return fptr;

    {
        VkLayerInstanceDispatchTable* pTable = my_data->instance_dispatch_table;
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(instance, funcName);
    }
}
