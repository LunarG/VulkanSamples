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
#include "vk_debug_marker_layer.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "vk_loader_platform.h"
#include "vk_layer_msg.h"
#include "vk_layer_table.h"
#include "vk_layer_debug_marker_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"

struct layer_data {
    debug_report_data *report_data;
    // TODO: put instance data here
    VkDbgMsgCallback logging_callback;

    layer_data() :
        report_data(nullptr),
        logging_callback(nullptr)
    {};
};

static std::unordered_map<void *, layer_data *> layer_data_map;
static device_table_map device_limits_device_table_map;
static instance_table_map device_limits_instance_table_map;

// Track state of each instance
unordered_map<VkInstance,       unique_ptr<INSTANCE_STATE>>                                   instanceMap;
unordered_map<VkPhysicalDevice, unique_ptr<PHYSICAL_DEVICE_STATE>>                            physicalDeviceMap;
unordered_map<VkPhysicalDevice, unordered_map<uint32_t, unique_ptr<VkQueueFamilyProperties>>> queueFamilyPropertiesMap;
unordered_map<VkPhysicalDevice, unique_ptr<VkPhysicalDeviceFeatures>>                         physicalDeviceFeaturesMap;
unordered_map<VkPhysicalDevice, unique_ptr<VkPhysicalDeviceProperties>>                       physicalDevicePropertiesMap;
unordered_map<VkDevice,         VkPhysicalDevice>                                             deviceMap;

struct devExts {
    bool debug_marker_enabled;
};

static std::unordered_map<void *, struct devExts> deviceExtMap;

static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);

// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

debug_report_data *mdd(void* object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MDD: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, my_data);
#endif
    return my_data->report_data;
}

debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MID: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, my_data);
#endif
    return my_data->report_data;
}

static void init_device_limits(layer_data *my_data)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    // initialize DeviceLimits options
    report_flags = getLayerOptionFlags("DeviceLimitsReportFlags", 0);
    getLayerOptionEnum("DeviceLimitsDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("DeviceLimitsLogFilename");
        log_output = getLayerLogOutput(option_str, "DeviceLimits");
        layer_create_msg_callback(my_data->report_data, report_flags, log_callback, (void *) log_output, &my_data->logging_callback);
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

VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, VkInstance* pInstance)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(device_limits_instance_table_map,*pInstance);
    VkResult result = pTable->CreateInstance(pCreateInfo, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   pTable,
                                   *pInstance,
                                   pCreateInfo->extensionCount,
                                   pCreateInfo->ppEnabledExtensionNames);

        init_device_limits(my_data);
        instanceMap[*pInstance] = unique_ptr<INSTANCE_STATE>(new INSTANCE_STATE());
    }
    return result;
}

/* hook DestroyInstance to remove tableInstanceMap entry */
VK_LAYER_EXPORT void VKAPI vkDestroyInstance(VkInstance instance)
{
    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(device_limits_instance_table_map, instance);
    pTable->DestroyInstance(instance);

    // Clean up logging callback, if any
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    if (my_data->logging_callback) {
        layer_destroy_msg_callback(my_data->report_data, my_data->logging_callback);
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    layer_data_map.erase(pTable);
    instanceMap.erase(instance);
    device_limits_instance_table_map.erase(key);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    auto it = instanceMap.find(instance);
    if (it != instanceMap.end()) {
        // For this instance, flag when vkEnumeratePhysicalDevices goes to QUERY_COUNT and then QUERY_DETAILS
        if (NULL == pPhysicalDevices) {
            it->second->vkEnumeratePhysicalDevicesState = QUERY_COUNT;
        } else {
            if (UNCALLED == it->second->vkEnumeratePhysicalDevicesState) {
                // Flag error here, shouldn't be calling this without having queried count
                log_msg(mid(instance), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_INSTANCE, 0, 0, DEVLIMITS_MUST_QUERY_COUNT, "DL",
                    "Invalid call sequence to vkEnumeratePhysicalDevices() w/ non-NULL pPhysicalDevices. You should first call vkEnumeratePhysicalDevices() w/ NULL pPhysicalDevices to query pPhysicalDeviceCount.");
            } // TODO : Could also flag a warning if re-calling this function in QUERY_DETAILS state
            else if (it->second->physicalDevicesCount != *pPhysicalDeviceCount) {
                log_msg(mid(instance), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_COUNT_MISMATCH, "DL",
                    "Call to vkEnumeratePhysicalDevices() w/ pPhysicalDeviceCount value %u, but actual count supported by this instance is %u.", *pPhysicalDeviceCount, it->second->physicalDevicesCount);
            }
            it->second->vkEnumeratePhysicalDevicesState = QUERY_DETAILS;
        }
        VkResult result = get_dispatch_table(device_limits_instance_table_map,instance)->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
        if (NULL == pPhysicalDevices) {
            it->second->physicalDevicesCount = *pPhysicalDeviceCount;
        } else { // Save physical devices
            for (uint32_t i=0; i < *pPhysicalDeviceCount; i++) {
                physicalDeviceMap[pPhysicalDevices[i]] = unique_ptr<PHYSICAL_DEVICE_STATE>(new PHYSICAL_DEVICE_STATE());
            }
        }
        return result;
    } else {
        log_msg(mid(instance), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_INSTANCE, 0, 0, DEVLIMITS_INVALID_INSTANCE, "DL",
            "Invalid instance (%#" PRIxLEAST64 ") passed into vkEnumeratePhysicalDevices().", instance);
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures)
{
    VkResult result = VK_SUCCESS;
    auto it = physicalDeviceMap.find(physicalDevice);
    if (it != physicalDeviceMap.end()) {
        result = get_dispatch_table(device_limits_instance_table_map, physicalDevice)->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);
        // Save Features
        if (VK_SUCCESS == result) {
            physicalDeviceFeaturesMap[physicalDevice] =
                unique_ptr<VkPhysicalDeviceFeatures>(new VkPhysicalDeviceFeatures(*pFeatures));
        }
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties)
{
    VkResult result = get_dispatch_table(device_limits_instance_table_map, physicalDevice)->GetPhysicalDeviceFormatProperties(
                                         physicalDevice, format, pFormatProperties);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
{
    VkResult result = get_dispatch_table(device_limits_instance_table_map, physicalDevice)->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties)
{
    VkResult result = VK_SUCCESS;
    auto it = physicalDeviceMap.find(physicalDevice);
    if (it != physicalDeviceMap.end()) {
        result = get_dispatch_table(device_limits_instance_table_map, physicalDevice)->
                                    GetPhysicalDeviceProperties(physicalDevice, pProperties);
        if (VK_SUCCESS == result) {
            // Save Properties
            physicalDevicePropertiesMap[physicalDevice] =
                unique_ptr<VkPhysicalDeviceProperties>(new VkPhysicalDeviceProperties(*pProperties));
        }
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pCount, VkQueueFamilyProperties* pQueueFamilyProperties)
{
    auto it = physicalDeviceMap.find(physicalDevice);
    if (it != physicalDeviceMap.end()) {
        if (NULL == pQueueFamilyProperties) {
            it->second->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_COUNT;
        } else {
            // Verify that for each physical device, this function is called first with NULL pQueueFamilyProperties ptr in order to get count
            if (UNCALLED == it->second->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
                log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_MUST_QUERY_COUNT, "DL",
                    "Invalid call sequence to vkGetPhysicalDeviceQueueFamilyProperties() w/ non-NULL pQueueFamilyProperties. You should first call vkGetPhysicalDeviceQueueFamilyProperties() w/ NULL pQueueFamilyProperties to query pCount.");
            }
            if (it->second->queueFamilyPropertiesCount != *pCount) {
                log_msg(mdd(physicalDevice), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_COUNT_MISMATCH, "DL",
                    "Call to vkGetPhysicalDeviceQueueFamilyProperties() w/ pCount value %u, but actual count supported by this physicalDevice is %u.", *pCount, it->second->queueFamilyPropertiesCount);
            }
            it->second->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_DETAILS;
        }
        // Then verify that pCount that is passed in on second call matches what was returned
        VkResult result = get_dispatch_table(device_limits_instance_table_map, physicalDevice)->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pCount, pQueueFamilyProperties);
        if (NULL == pQueueFamilyProperties) {
            it->second->queueFamilyPropertiesCount = *pCount;
        } else { // Save queue family properties
            for (uint32_t i=0; i < *pCount; i++) {
                queueFamilyPropertiesMap[physicalDevice][i] = unique_ptr<VkQueueFamilyProperties>(new VkQueueFamilyProperties(pQueueFamilyProperties[i]));
            }
        }
        return result;
    } else {
        log_msg(mdd(physicalDevice), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_INVALID_PHYSICAL_DEVICE, "DL",
            "Invalid physicalDevice (%#" PRIxLEAST64 ") passed into vkGetPhysicalDeviceQueueFamilyProperties().", physicalDevice);
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    VkResult result = get_dispatch_table(device_limits_instance_table_map, physicalDevice)->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, uint32_t samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pNumProperties, VkSparseImageFormatProperties* pProperties)
{
    VkResult result = get_dispatch_table(device_limits_instance_table_map, physicalDevice)->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pNumProperties, pProperties);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdSetViewport(
    VkCmdBuffer                         cmdBuffer,
    uint32_t                            viewportCount,
    const VkViewport*                   pViewports)
{
    VkBool32 skipCall = VK_FALSE;
    /* TODO: Verify viewportCount < maxViewports from VkPhysicalDeviceLimits */
    if (VK_FALSE == skipCall) {
        get_dispatch_table(device_limits_device_table_map, cmdBuffer)->CmdSetViewport(cmdBuffer, viewportCount, pViewports);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdSetScissor(
    VkCmdBuffer                         cmdBuffer,
    uint32_t                            scissorCount,
    const VkRect2D*                     pScissors)
{
    VkBool32 skipCall = VK_FALSE;
    /* TODO: Verify scissorCount < maxViewports from VkPhysicalDeviceLimits */
    /* TODO: viewportCount and scissorCount must match at draw time */
    if (VK_FALSE == skipCall) {
        get_dispatch_table(device_limits_device_table_map, cmdBuffer)->CmdSetScissor(cmdBuffer, scissorCount, pScissors);
    }
}

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i;
    VkLayerDispatchTable *pDisp =  get_dispatch_table(device_limits_device_table_map, device);
    deviceExtMap[pDisp].debug_marker_enabled = false;

    for (i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], DEBUG_MARKER_EXTENSION_NAME) == 0) {
            /* Found a matching extension name, mark it enabled and init dispatch table*/
            initDebugMarkerTable(device);
            deviceExtMap[pDisp].debug_marker_enabled = true;
        }

    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    // First check is app has actually requested queueFamilyProperties
    auto it = physicalDeviceMap.find(gpu);
    if (it == physicalDeviceMap.end()) {
        log_msg(mdd(gpu), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_MUST_QUERY_COUNT, "DL",
            "Invalid call to vkCreateDevice() w/o first calling vkEnumeratePhysicalDevices().");
    } else if (QUERY_DETAILS != it->second->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
        log_msg(mdd(gpu), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
            "Invalid call to vkCreateDevice() w/o first calling vkGetPhysicalDeviceQueueFamilyProperties().");
    } else {
        // Check that the requested queue properties are valid
        for (uint32_t i=0; i<pCreateInfo->queueRecordCount; i++) {
            uint32_t requestedIndex = pCreateInfo->pRequestedQueues[i].queueFamilyIndex;
            auto qfp_it = queueFamilyPropertiesMap[gpu].find(requestedIndex);
            if (qfp_it == queueFamilyPropertiesMap[gpu].end()) { // requested index is out of bounds for this physical device
                log_msg(mdd(gpu), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
                    "Invalid queue create request in vkCreateDevice(). Invalid queueFamilyIndex %u requested.", requestedIndex);
            } else if (pCreateInfo->pRequestedQueues[i].queueCount > qfp_it->second->queueCount) {
                log_msg(mdd(gpu), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
                    "Invalid queue create request in vkCreateDevice(). QueueFamilyIndex %u only has %u queues, but requested queueCount is %u.", requestedIndex, qfp_it->second->queueCount, pCreateInfo->pRequestedQueues[i].queueCount);
            }
        }
    }
    VkLayerDispatchTable *pDeviceTable = get_dispatch_table(device_limits_device_table_map, *pDevice);
    VkResult result = pDeviceTable->CreateDevice(gpu, pCreateInfo, pDevice);
    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
        VkLayerDispatchTable *pTable = get_dispatch_table(device_limits_device_table_map, *pDevice);
        layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
        my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        createDeviceRegisterExtensions(pCreateInfo, *pDevice);
        deviceMap[*pDevice] = gpu;
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkDestroyDevice(VkDevice device)
{
    // Free device lifetime allocations
    dispatch_key key = get_dispatch_key(device);
    VkLayerDispatchTable *pDisp =  get_dispatch_table(device_limits_device_table_map, device);
    pDisp->DestroyDevice(device);
    deviceExtMap.erase(pDisp);
    device_limits_device_table_map.erase(key);
    tableDebugMarkerMap.erase(pDisp);
}

static const VkLayerProperties ds_global_layers[] = {
    {
        "DeviceLimits",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: DeviceLimits",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* DeviceLimits does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(ds_global_layers),
                                   ds_global_layers,
                                   pCount, pProperties);
}

static const VkExtensionProperties ds_device_extensions[] = {
    {
        DEBUG_MARKER_EXTENSION_NAME,
        VK_MAKE_VERSION(0, 1, 0),
    }
};

static const VkLayerProperties ds_device_layers[] = {
    {
        "DeviceLimits",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: DeviceLimits",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* Device Limits does not have any physical device extensions */
    return util_GetExtensionProperties(ARRAY_SIZE(ds_device_extensions), ds_device_extensions,
                                       pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* Device Limits' physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(ds_device_layers), ds_device_layers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    VkPhysicalDevice gpu = deviceMap[device];
    auto qfp_it = queueFamilyPropertiesMap[gpu].find(queueFamilyIndex);
    if (qfp_it == queueFamilyPropertiesMap[gpu].end()) { // requested index is out of bounds for this physical device
        log_msg(mdd(gpu), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
            "Invalid queueFamilyIndex %u requested in vkGetDeviceQueue().", queueFamilyIndex);
    } else if (queueIndex >= qfp_it->second->queueCount) {
        log_msg(mdd(gpu), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PHYSICAL_DEVICE, 0, 0, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
            "Invalid queue request in vkGetDeviceQueue(). QueueFamilyIndex %u only has %u queues, but requested queueIndex is %u.", queueFamilyIndex, qfp_it->second->queueCount, queueIndex);
    }
    VkResult result = get_dispatch_table(device_limits_device_table_map, device)->GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(
    VkDevice                 device,
    const VkImageCreateInfo *pCreateInfo,
    VkImage                 *pImage)
{
    VkBool32                skipCall       = VK_FALSE;
    VkResult                result         = VK_ERROR_VALIDATION_FAILED;
    VkPhysicalDevice        physicalDevice = deviceMap[device];
    VkImageType             type;
    VkImageFormatProperties ImageFormatProperties = {0};

    // Internal call to get format info.  Still goes through layers, could potentially go directly to ICD.
    get_dispatch_table(device_limits_instance_table_map, physicalDevice)->GetPhysicalDeviceImageFormatProperties(
                       physicalDevice, pCreateInfo->format, pCreateInfo->imageType, pCreateInfo->tiling,
                       pCreateInfo->usage, pCreateInfo->flags, &ImageFormatProperties);

    auto pdp_it = physicalDevicePropertiesMap.find(physicalDevice);
    if (pdp_it == physicalDevicePropertiesMap.end()) {
        skipCall = log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_IMAGE, (uint64_t)pImage, 0,
                       DEVLIMITS_MUST_QUERY_PROPERTIES, "DL",
                       "CreateImage called before querying device properties ");
    }
    uint32_t imageGranularity = pdp_it->second->limits.bufferImageGranularity;
    imageGranularity = imageGranularity == 1 ? 0 : imageGranularity;

    if ((pCreateInfo->extent.depth  > ImageFormatProperties.maxExtent.depth)  ||
        (pCreateInfo->extent.width  > ImageFormatProperties.maxExtent.width)  ||
        (pCreateInfo->extent.height > ImageFormatProperties.maxExtent.height)) {
        skipCall = log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE, (uint64_t)pImage, 0,
                       DEVLIMITS_LIMITS_VIOLATION, "DL",
                       "CreateImage extents exceed allowable limits for format: "
                       "Width = %d Height = %d Depth = %d:  Limits for Width = %d Height = %d Depth = %d for format %s.",
                       pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth,
                       ImageFormatProperties.maxExtent.width, ImageFormatProperties.maxExtent.height, ImageFormatProperties.maxExtent.depth,
                       string_VkFormat(pCreateInfo->format));

    }

    uint64_t totalSize = ((uint64_t)pCreateInfo->extent.width               *
                          (uint64_t)pCreateInfo->extent.height              *
                          (uint64_t)pCreateInfo->extent.depth               *
                          (uint64_t)pCreateInfo->arraySize                  *
                          (uint64_t)pCreateInfo->samples                    *
                          (uint64_t)vk_format_get_size(pCreateInfo->format) +
                          (uint64_t)imageGranularity ) & ~(uint64_t)imageGranularity;

    if (totalSize > ImageFormatProperties.maxResourceSize) {
        skipCall = log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE, (uint64_t)pImage, 0,
                       DEVLIMITS_LIMITS_VIOLATION, "DL",
                       "CreateImage resource size exceeds allowable maximum "
                       "Image resource size = %#" PRIxLEAST64 ", maximum resource size = %#" PRIxLEAST64 " ",
                       totalSize, ImageFormatProperties.maxResourceSize);
    }
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(device_limits_device_table_map, device)->CreateImage(device, pCreateInfo, pImage);
    }
    return result;
}


VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(
    VkInstance                          instance,
    VkFlags                             msgFlags,
    const PFN_vkDbgMsgCallback          pfnMsgCallback,
    void*                               pUserData,
    VkDbgMsgCallback*                   pMsgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(device_limits_instance_table_map, instance);
    VkResult res = pTable->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    if (VK_SUCCESS == res) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        res = layer_create_msg_callback(my_data->report_data, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    }
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgDestroyMsgCallback(
    VkInstance                          instance,
    VkDbgMsgCallback                    msgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(device_limits_instance_table_map, instance);
    VkResult res = pTable->DbgDestroyMsgCallback(instance, msgCallback);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(my_data->report_data, msgCallback);
    return res;
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(VkDevice dev, const char* funcName)
{
    if (dev == NULL)
        return NULL;

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        initDeviceTable(device_limits_device_table_map, (const VkBaseLayerObject *) dev);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    /*if (!strcmp(funcName, "vkQueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;*/
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (PFN_vkVoidFunction) vkGetDeviceQueue;
/*    if (!strcmp(funcName, "vkDestroyFence"))
        return (PFN_vkVoidFunction) vkDestroyFence;
    if (!strcmp(funcName, "vkDestroySemaphore"))
        return (PFN_vkVoidFunction) vkDestroySemaphore;
    if (!strcmp(funcName, "vkDestroyEvent"))
        return (PFN_vkVoidFunction) vkDestroyEvent;
    if (!strcmp(funcName, "vkDestroyQueryPool"))
        return (PFN_vkVoidFunction) vkDestroyQueryPool;
    if (!strcmp(funcName, "vkDestroyBuffer"))
        return (PFN_vkVoidFunction) vkDestroyBuffer;
    if (!strcmp(funcName, "vkDestroyBufferView"))
        return (PFN_vkVoidFunction) vkDestroyBufferView;
    if (!strcmp(funcName, "vkDestroyImage"))
        return (PFN_vkVoidFunction) vkDestroyImage;
    if (!strcmp(funcName, "vkDestroyImageView"))
        return (PFN_vkVoidFunction) vkDestroyImageView;
    if (!strcmp(funcName, "vkDestroyShaderModule"))
        return (PFN_vkVoidFunction) vkDestroyShaderModule;
    if (!strcmp(funcName, "vkDestroyShader"))
        return (PFN_vkVoidFunction) vkDestroyShader;
    if (!strcmp(funcName, "vkDestroyPipeline"))
        return (PFN_vkVoidFunction) vkDestroyPipeline;
    if (!strcmp(funcName, "vkDestroyPipelineLayout"))
        return (PFN_vkVoidFunction) vkDestroyPipelineLayout;
    if (!strcmp(funcName, "vkDestroySampler"))
        return (PFN_vkVoidFunction) vkDestroySampler;
    if (!strcmp(funcName, "vkDestroyDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorSetLayout;
    if (!strcmp(funcName, "vkDestroyDescriptorPool"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorPool;
    if (!strcmp(funcName, "vkDestroyCommandBuffer"))
        return (PFN_vkVoidFunction) vkDestroyCommandBuffer;
    if (!strcmp(funcName, "vkDestroyFramebuffer"))
        return (PFN_vkVoidFunction) vkDestroyFramebuffer;
    if (!strcmp(funcName, "vkDestroyRenderPass"))
        return (PFN_vkVoidFunction) vkDestroyRenderPass;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
*/
    if (!strcmp(funcName, "vkCreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
/*
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "CreatePipelineCache"))
        return (PFN_vkVoidFunction) vkCreatePipelineCache;
    if (!strcmp(funcName, "DestroyPipelineCache"))
        return (PFN_vkVoidFunction) vkDestroyPipelineCache;
    if (!strcmp(funcName, "GetPipelineCacheSize"))
        return (PFN_vkVoidFunction) vkGetPipelineCacheSize;
    if (!strcmp(funcName, "GetPipelineCacheData"))
        return (PFN_vkVoidFunction) vkGetPipelineCacheData;
    if (!strcmp(funcName, "MergePipelineCaches"))
        return (PFN_vkVoidFunction) vkMergePipelineCaches;
    if (!strcmp(funcName, "vkCreateGraphicsPipelines"))
        return (PFN_vkVoidFunction) vkCreateGraphicsPipelines;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (PFN_vkVoidFunction) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkCreateDescriptorSetLayout;
    if (!strcmp(funcName, "vkCreatePipelineLayout"))
        return (PFN_vkVoidFunction) vkCreatePipelineLayout;
    if (!strcmp(funcName, "vkCreateDescriptorPool"))
        return (PFN_vkVoidFunction) vkCreateDescriptorPool;
    if (!strcmp(funcName, "vkResetDescriptorPool"))
        return (PFN_vkVoidFunction) vkResetDescriptorPool;
    if (!strcmp(funcName, "vkAllocDescriptorSets"))
        return (PFN_vkVoidFunction) vkAllocDescriptorSets;
    if (!strcmp(funcName, "vkUpdateDescriptorSets"))
        return (PFN_vkVoidFunction) vkUpdateDescriptorSets;
    if (!strcmp(funcName, "vkCreateCommandBuffer"))
        return (PFN_vkVoidFunction) vkCreateCommandBuffer;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (PFN_vkVoidFunction) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (PFN_vkVoidFunction) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (PFN_vkVoidFunction) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (PFN_vkVoidFunction) vkCmdBindPipeline;
    if (!strcmp(funcName, "vkCmdSetViewport"))
        return (PFN_vkVoidFunction) vkCmdSetViewport;
    if (!strcmp(funcName, "vkCmdBindDescriptorSets"))
        return (PFN_vkVoidFunction) vkCmdBindDescriptorSets;
    if (!strcmp(funcName, "vkCmdBindVertexBuffers"))
        return (PFN_vkVoidFunction) vkCmdBindVertexBuffers;
    if (!strcmp(funcName, "vkCmdBindIndexBuffer"))
        return (PFN_vkVoidFunction) vkCmdBindIndexBuffer;
    if (!strcmp(funcName, "vkCmdDraw"))
        return (PFN_vkVoidFunction) vkCmdDraw;
    if (!strcmp(funcName, "vkCmdDrawIndexed"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexed;
    if (!strcmp(funcName, "vkCmdDrawIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndirect;
    if (!strcmp(funcName, "vkCmdDrawIndexedIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexedIndirect;
    if (!strcmp(funcName, "vkCmdDispatch"))
        return (PFN_vkVoidFunction) vkCmdDispatch;
    if (!strcmp(funcName, "vkCmdDispatchIndirect"))
        return (PFN_vkVoidFunction) vkCmdDispatchIndirect;
    if (!strcmp(funcName, "vkCmdCopyBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyBuffer;
    if (!strcmp(funcName, "vkCmdCopyImage"))
        return (PFN_vkVoidFunction) vkCmdCopyImage;
    if (!strcmp(funcName, "vkCmdCopyBufferToImage"))
        return (PFN_vkVoidFunction) vkCmdCopyBufferToImage;
    if (!strcmp(funcName, "vkCmdCopyImageToBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyImageToBuffer;
    if (!strcmp(funcName, "vkCmdUpdateBuffer"))
        return (PFN_vkVoidFunction) vkCmdUpdateBuffer;
    if (!strcmp(funcName, "vkCmdFillBuffer"))
        return (PFN_vkVoidFunction) vkCmdFillBuffer;
    if (!strcmp(funcName, "vkCmdClearColorImage"))
        return (PFN_vkVoidFunction) vkCmdClearColorImage;
    if (!strcmp(funcName, "vkCmdClearDepthStencilImage"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilImage;
    if (!strcmp(funcName, "vkCmdClearColorAttachment"))
        return (PFN_vkVoidFunction) vkCmdClearColorAttachment;
    if (!strcmp(funcName, "vkCmdClearDepthStencilAttachment"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilAttachment;
    if (!strcmp(funcName, "vkCmdResolveImage"))
        return (PFN_vkVoidFunction) vkCmdResolveImage;
    if (!strcmp(funcName, "vkCmdSetEvent"))
        return (PFN_vkVoidFunction) vkCmdSetEvent;
    if (!strcmp(funcName, "vkCmdResetEvent"))
        return (PFN_vkVoidFunction) vkCmdResetEvent;
    if (!strcmp(funcName, "vkCmdWaitEvents"))
        return (PFN_vkVoidFunction) vkCmdWaitEvents;
    if (!strcmp(funcName, "vkCmdPipelineBarrier"))
        return (PFN_vkVoidFunction) vkCmdPipelineBarrier;
    if (!strcmp(funcName, "vkCmdBeginQuery"))
        return (PFN_vkVoidFunction) vkCmdBeginQuery;
    if (!strcmp(funcName, "vkCmdEndQuery"))
        return (PFN_vkVoidFunction) vkCmdEndQuery;
    if (!strcmp(funcName, "vkCmdResetQueryPool"))
        return (PFN_vkVoidFunction) vkCmdResetQueryPool;
    if (!strcmp(funcName, "vkCmdWriteTimestamp"))
        return (PFN_vkVoidFunction) vkCmdWriteTimestamp;
    if (!strcmp(funcName, "vkCreateFramebuffer"))
        return (PFN_vkVoidFunction) vkCreateFramebuffer;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;
    if (!strcmp(funcName, "vkCmdBeginRenderPass"))
        return (PFN_vkVoidFunction) vkCmdBeginRenderPass;
    if (!strcmp(funcName, "vkCmdNextSubpass"))
        return (PFN_vkVoidFunction) vkCmdNextSubpass;
    if (!strcmp(funcName, "vkCmdEndRenderPass"))
        return (PFN_vkVoidFunction) vkCmdEndRenderPass;*/

    VkLayerDispatchTable* pTable = get_dispatch_table(device_limits_device_table_map, dev);
    if (deviceExtMap.size() == 0 || deviceExtMap[pTable].debug_marker_enabled)
    {
//        if (!strcmp(funcName, "vkCmdDbgMarkerBegin"))
//            return (PFN_vkVoidFunction) vkCmdDbgMarkerBegin;
//        if (!strcmp(funcName, "vkCmdDbgMarkerEnd"))
//            return (PFN_vkVoidFunction) vkCmdDbgMarkerEnd;
//        if (!strcmp(funcName, "vkDbgSetObjectTag"))
//            return (void*) vkDbgSetObjectTag;
//        if (!strcmp(funcName, "vkDbgSetObjectName"))
//            return (void*) vkDbgSetObjectName;
    }
    {
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(dev, funcName);
    }
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    PFN_vkVoidFunction fptr;
    if (instance == NULL)
        return NULL;

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        initInstanceTable(device_limits_instance_table_map, (const VkBaseLayerObject *) instance);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }
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
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceSparseImageFormatProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceSparseImageFormatProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    fptr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (fptr)
        return fptr;

    {
        VkLayerInstanceDispatchTable* pTable = get_dispatch_table(device_limits_instance_table_map, instance);
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(instance, funcName);
    }
}
