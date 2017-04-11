/*
 *
 * Copyright (C) 2017 Valve Corporation
 * Copyright (C) 2017 LunarG, Inc.
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
 * Author: Arda Coskunses <arda@lunarg.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>

#include "vk_dispatch_table_helper.h"
#include "vk_lunarg_device_profile_api_layer.h"
#include "vk_device_profile_api_layer.h"
#include "threading.h"

namespace device_profile_api {

static std::unordered_map<dispatch_key, VkInstance> device_profile_api_instance_map;
static std::mutex global_lock;

static uint32_t loader_layer_if_version = CURRENT_LOADER_LAYER_INTERFACE_VERSION;

static std::unordered_map<VkPhysicalDevice, struct VkPhysicalDeviceProperties> device_profile_api_dev_data_map;
static std::unordered_map<VkPhysicalDevice, struct VkPhysicalDeviceProperties> device_profile_api_dev_org_data_map;

// device_profile_api Layer EXT APIs
typedef VkResult(VKAPI_PTR *PFN_vkGetOriginalPhysicalDeviceLimitsEXT)(VkPhysicalDevice physicalDevice,
                                                                      const VkPhysicalDeviceLimits *limits);

typedef VkResult(VKAPI_PTR *PFN_vkSetPhysicalDeviceLimitsEXT)(VkPhysicalDevice physicalDevice,
                                                              const VkPhysicalDeviceLimits *newLimits);

VKAPI_ATTR void VKAPI_CALL GetOriginalPhysicalDeviceLimitsEXT(VkPhysicalDevice physicalDevice, VkPhysicalDeviceLimits *orgLimits) {
    std::lock_guard<std::mutex> lock(global_lock);

    // search if we got the device limits for this device and stored in device_profile_api layer
    auto device_profile_api_data_it = device_profile_api_dev_org_data_map.find(physicalDevice);
    if (device_profile_api_data_it != device_profile_api_dev_org_data_map.end()) {
        memcpy( orgLimits, &(device_profile_api_dev_org_data_map[physicalDevice].limits), sizeof(VkPhysicalDeviceLimits));
    }
}

VKAPI_ATTR void VKAPI_CALL SetPhysicalDeviceLimitsEXT(VkPhysicalDevice physicalDevice,
                                                          const VkPhysicalDeviceLimits *newLimits) {
    std::lock_guard<std::mutex> lock(global_lock);

    // search if we got the device limits for this device and stored in device_profile_api layer
    auto device_profile_api_data_it = device_profile_api_dev_data_map.find(physicalDevice);
    if (device_profile_api_data_it != device_profile_api_dev_data_map.end()) {
        memcpy(&(device_profile_api_dev_data_map[physicalDevice].limits), newLimits, sizeof(VkPhysicalDeviceLimits));
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                              VkInstance *pInstance) {
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
    std::lock_guard<std::mutex> lock(global_lock);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fp_get_instance_proc_addr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fp_create_instance = (PFN_vkCreateInstance)fp_get_instance_proc_addr(NULL, "vkCreateInstance");
    if (fp_create_instance == NULL) return VK_ERROR_INITIALIZATION_FAILED;

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fp_create_instance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) return result;

    device_profile_api_instance_map[get_dispatch_key(*pInstance)] = *pInstance;
    initInstanceTable(*pInstance, fp_get_instance_proc_addr);

    layer_data *device_profile_data = GetLayerDataPtr(get_dispatch_key(*pInstance), layer_data_map);
    device_profile_data->instance = *pInstance;
    device_profile_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, device_profile_data->instance_dispatch_table, fp_get_instance_proc_addr);

    uint32_t physical_device_count = 0;
    device_profile_data->instance_dispatch_table->EnumeratePhysicalDevices(*pInstance, &physical_device_count, NULL);

    VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)malloc(sizeof(physical_devices[0]) * physical_device_count);
    result = device_profile_data->instance_dispatch_table->EnumeratePhysicalDevices(*pInstance, &physical_device_count, physical_devices);

    // Get original physical device props
    for (uint8_t i = 0; i < physical_device_count; i++) {
        auto device_profile_api_data_it = device_profile_api_dev_org_data_map.find(physical_devices[i]);
        if (device_profile_api_data_it == device_profile_api_dev_org_data_map.end()) {
            VkPhysicalDeviceProperties props;
            device_profile_data->instance_dispatch_table
                     ->GetPhysicalDeviceProperties(physical_devices[i], &props);
            device_profile_api_dev_org_data_map[physical_devices[i]] = props;
            device_profile_api_dev_data_map[physical_devices[i]] = props;
            }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,
                                                       VkPhysicalDeviceProperties *pProperties) {
    {
        std::lock_guard<std::mutex> lock(global_lock);

        // Search if we got the device limits for this device and stored in device_profile_api layer
        auto device_profile_api_data_it = device_profile_api_dev_data_map.find(physicalDevice);
        if (device_profile_api_data_it != device_profile_api_dev_data_map.end()) {
            memcpy(pProperties, &device_profile_api_dev_data_map[physicalDevice], sizeof(VkPhysicalDeviceProperties));
        }
    }
}

static const VkLayerProperties device_profile_api_LayerProps = {
    "VK_LAYER_LUNARG_device_profile_api", VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION),  // specVersion
    1,                                                                  // implementationVersion
    "LunarG device profile api Layer",
};

static const VkExtensionProperties device_profile_api_physicaldevice_extensions[] = {{
    "vkLayerdevice_profile_apiEXT", 1,
}};

template <typename T>
VkResult EnumerateProperties(uint32_t src_count, const T *src_props, uint32_t *dst_count, T *dst_props) {
    if (!dst_props || !src_props) {
        *dst_count = src_count;
        return VK_SUCCESS;
    }

    uint32_t copy_count = (*dst_count < src_count) ? *dst_count : src_count;
    memcpy(dst_props, src_props, sizeof(T) * copy_count);
    *dst_count = copy_count;

    return (copy_count == src_count) ? VK_SUCCESS : VK_INCOMPLETE;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return EnumerateProperties(1, &device_profile_api_LayerProps, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                    VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, device_profile_api_LayerProps.layerName))
        return EnumerateProperties<VkExtensionProperties>(0, NULL, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetPhysicalDeviceProcAddr(VkInstance instance, const char *name) {

    if (!strcmp(name, "vkSetPhysicalDeviceLimitsEXT")) return (PFN_vkVoidFunction)SetPhysicalDeviceLimitsEXT;
    if (!strcmp(name, "vkGetOriginalPhysicalDeviceLimitsEXT")) return (PFN_vkVoidFunction)GetOriginalPhysicalDeviceLimitsEXT;
    if (instance_dispatch_table(instance)->GetPhysicalDeviceProcAddr == NULL) return NULL;
    return instance_dispatch_table(instance)->GetPhysicalDeviceProcAddr(instance, name);

}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *name) {

    if (!strcmp(name, "vkCreateInstance")) return (PFN_vkVoidFunction)CreateInstance;
    if (!strcmp(name, "vkGetPhysicalDeviceProperties")) return (PFN_vkVoidFunction)GetPhysicalDeviceProperties;
    if (!strcmp(name, "vkGetInstanceProcAddr")) return (PFN_vkVoidFunction)GetInstanceProcAddr;
    if (!strcmp(name, "vkEnumerateInstanceExtensionProperties")) return (PFN_vkVoidFunction)EnumerateInstanceExtensionProperties;
    if (!strcmp(name, "vkEnumerateInstanceLayerProperties")) return (PFN_vkVoidFunction)EnumerateInstanceLayerProperties;
    if (!strcmp(name, "vkSetPhysicalDeviceLimitsEXT")) return (PFN_vkVoidFunction)SetPhysicalDeviceLimitsEXT;
    if (!strcmp(name, "vkGetOriginalPhysicalDeviceLimitsEXT")) return (PFN_vkVoidFunction)GetOriginalPhysicalDeviceLimitsEXT;

    assert(instance);

    if (instance_dispatch_table(instance)->GetInstanceProcAddr == NULL) return NULL;
    return instance_dispatch_table(instance)->GetInstanceProcAddr(instance, name);
}

} // namespace device_profile_api

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,
                                                                                  VkLayerProperties *pProperties) {
    return device_profile_api::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                                      VkExtensionProperties *pProperties) {
    return device_profile_api::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return device_profile_api::GetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_layerGetPhysicalDeviceProcAddr(VkInstance instance,
        const char *funcName) {
    return device_profile_api::GetPhysicalDeviceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct) {
    assert(pVersionStruct != NULL);
    assert(pVersionStruct->sType == LAYER_NEGOTIATE_INTERFACE_STRUCT);

    // Fill in the function pointers if our version is at least capable of having the structure contain them.
    if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
        pVersionStruct->pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
        pVersionStruct->pfnGetDeviceProcAddr = nullptr;
        pVersionStruct->pfnGetPhysicalDeviceProcAddr = vk_layerGetPhysicalDeviceProcAddr;
    }

    if (pVersionStruct->loaderLayerInterfaceVersion < CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        device_profile_api::loader_layer_if_version = pVersionStruct->loaderLayerInterfaceVersion;
    } else if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
    }

    return VK_SUCCESS;
}
