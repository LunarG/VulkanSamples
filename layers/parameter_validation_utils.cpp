/* Copyright (c) 2015-2017 The Khronos Group Inc.
 * Copyright (c) 2015-2017 Valve Corporation
 * Copyright (c) 2015-2017 LunarG, Inc.
 * Copyright (C) 2015-2017 Google Inc.
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
 * Author: Mark Lobodzinski <mark@LunarG.com>
 */

#define NOMINMAX

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>

#include "vk_loader_platform.h"
#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_dispatch_table_helper.h"

#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"

#include "parameter_name.h"
#include "parameter_validation.h"

// TODO: remove on NDK update (r15 will probably have proper STL impl)
#ifdef __ANDROID__
namespace std {

template <typename T>
std::string to_string(T var) {
    std::ostringstream ss;
    ss << var;
    return ss.str();
}
}  // namespace std
#endif

namespace parameter_validation {

extern std::unordered_map<std::string, void *> custom_functions;

extern bool parameter_validation_vkCreateInstance(VkInstance instance, const VkInstanceCreateInfo *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkInstance *pInstance);
extern bool parameter_validation_vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator);
extern bool parameter_validation_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkDevice *pDevice);
extern bool parameter_validation_vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator);
extern bool parameter_validation_vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool);
extern bool parameter_validation_vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                                const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                                const VkAllocationCallbacks *pAllocator,
                                                                VkDebugReportCallbackEXT *pMsgCallback);
extern bool parameter_validation_vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                                 const VkAllocationCallbacks *pAllocator);
extern bool parameter_validation_vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
                                                     const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool);

// TODO : This can be much smarter, using separate locks for separate global data
std::mutex global_lock;

static uint32_t loader_layer_if_version = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
std::unordered_map<void *, layer_data *> layer_data_map;
std::unordered_map<void *, instance_layer_data *> instance_layer_data_map;

void InitializeManualParameterValidationFunctionPointers(void);

static void init_parameter_validation(instance_layer_data *instance_data, const VkAllocationCallbacks *pAllocator) {
    layer_debug_actions(instance_data->report_data, instance_data->logging_callback, pAllocator, "lunarg_parameter_validation");
}

static const VkExtensionProperties instance_extensions[] = {{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION}};

static const VkLayerProperties global_layer = {
    "VK_LAYER_LUNARG_parameter_validation", VK_LAYER_API_VERSION, 1, "LunarG Validation Layer",
};

static const int MaxParamCheckerStringLength = 256;

static bool validate_string(debug_report_data *report_data, const char *apiName, const ParameterName &stringName,
                            const char *validateString) {
    assert(apiName != nullptr);
    assert(validateString != nullptr);

    bool skip = false;

    VkStringErrorFlags result = vk_string_validate(MaxParamCheckerStringLength, validateString);

    if (result == VK_STRING_ERROR_NONE) {
        return skip;
    } else if (result & VK_STRING_ERROR_LENGTH) {
        skip = log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                       INVALID_USAGE, LayerName, "%s: string %s exceeds max length %d", apiName, stringName.get_name().c_str(),
                       MaxParamCheckerStringLength);
    } else if (result & VK_STRING_ERROR_BAD_DATA) {
        skip = log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                       INVALID_USAGE, LayerName, "%s: string %s contains invalid characters or is badly formed", apiName,
                       stringName.get_name().c_str());
    }
    return skip;
}

static bool ValidateDeviceQueueFamily(layer_data *device_data, uint32_t queue_family, const char *cmd_name,
                                      const char *parameter_name, int32_t error_code, bool optional = false,
                                      const char *vu_note = nullptr) {
    bool skip = false;

    if (!vu_note) vu_note = validation_error_map[error_code];
    if (!optional && queue_family == VK_QUEUE_FAMILY_IGNORED) {
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        HandleToUint64(device_data->device), __LINE__, error_code, LayerName,
                        "%s: %s is VK_QUEUE_FAMILY_IGNORED, but it is required to provide a valid queue family index value. %s",
                        cmd_name, parameter_name, vu_note);
    } else if (device_data->queueFamilyIndexMap.find(queue_family) == device_data->queueFamilyIndexMap.end()) {
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        HandleToUint64(device_data->device), __LINE__, error_code, LayerName,
                        "%s: %s (= %" PRIu32
                        ") is not one of the queue families given via VkDeviceQueueCreateInfo structures when "
                        "the device was created. %s",
                        cmd_name, parameter_name, queue_family, vu_note);
    }

    return skip;
}

static bool ValidateQueueFamilies(layer_data *device_data, uint32_t queue_family_count, const uint32_t *queue_families,
                                  const char *cmd_name, const char *array_parameter_name, int32_t unique_error_code,
                                  int32_t valid_error_code, bool optional = false, const char *unique_vu_note = nullptr,
                                  const char *valid_vu_note = nullptr) {
    bool skip = false;
    if (!unique_vu_note) unique_vu_note = validation_error_map[unique_error_code];
    if (!valid_vu_note) valid_vu_note = validation_error_map[valid_error_code];
    if (queue_families) {
        std::unordered_set<uint32_t> set;
        for (uint32_t i = 0; i < queue_family_count; ++i) {
            std::string parameter_name = std::string(array_parameter_name) + "[" + std::to_string(i) + "]";

            if (set.count(queue_families[i])) {
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                                HandleToUint64(device_data->device), __LINE__, VALIDATION_ERROR_056002e8, LayerName,
                                "%s: %s (=%" PRIu32 ") is not unique within %s array. %s", cmd_name, parameter_name.c_str(),
                                queue_families[i], array_parameter_name, unique_vu_note);
            } else {
                set.insert(queue_families[i]);
                skip |= ValidateDeviceQueueFamily(device_data, queue_families[i], cmd_name, parameter_name.c_str(),
                                                  valid_error_code, optional, valid_vu_note);
            }
        }
    }
    return skip;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                                VkInstance *pInstance) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;

    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
    assert(chain_info != nullptr);
    assert(chain_info->u.pLayerInfo != nullptr);

    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);

    if (result == VK_SUCCESS) {
        InitializeManualParameterValidationFunctionPointers();
        auto my_instance_data = GetLayerDataPtr(get_dispatch_key(*pInstance), instance_layer_data_map);
        assert(my_instance_data != nullptr);

        layer_init_instance_dispatch_table(*pInstance, &my_instance_data->dispatch_table, fpGetInstanceProcAddr);
        my_instance_data->instance = *pInstance;
        my_instance_data->report_data =
            debug_report_create_instance(&my_instance_data->dispatch_table, *pInstance, pCreateInfo->enabledExtensionCount,
                                         pCreateInfo->ppEnabledExtensionNames);

        // Look for one or more debug report create info structures
        // and setup a callback(s) for each one found.
        if (!layer_copy_tmp_callbacks(pCreateInfo->pNext, &my_instance_data->num_tmp_callbacks,
                                      &my_instance_data->tmp_dbg_create_infos, &my_instance_data->tmp_callbacks)) {
            if (my_instance_data->num_tmp_callbacks > 0) {
                // Setup the temporary callback(s) here to catch early issues:
                if (layer_enable_tmp_callbacks(my_instance_data->report_data, my_instance_data->num_tmp_callbacks,
                                               my_instance_data->tmp_dbg_create_infos, my_instance_data->tmp_callbacks)) {
                    // Failure of setting up one or more of the callback.
                    // Therefore, clean up and don't use those callbacks:
                    layer_free_tmp_callbacks(my_instance_data->tmp_dbg_create_infos, my_instance_data->tmp_callbacks);
                    my_instance_data->num_tmp_callbacks = 0;
                }
            }
        }

        init_parameter_validation(my_instance_data, pAllocator);
        my_instance_data->extensions.InitFromInstanceCreateInfo(pCreateInfo);

        // Ordinarily we'd check these before calling down the chain, but none of the layer support is in place until now, if we
        // survive we can report the issue now.
        parameter_validation_vkCreateInstance(*pInstance, pCreateInfo, pAllocator, pInstance);

        if (pCreateInfo->pApplicationInfo) {
            if (pCreateInfo->pApplicationInfo->pApplicationName) {
                validate_string(my_instance_data->report_data, "vkCreateInstance",
                                "pCreateInfo->VkApplicationInfo->pApplicationName",
                                pCreateInfo->pApplicationInfo->pApplicationName);
            }

            if (pCreateInfo->pApplicationInfo->pEngineName) {
                validate_string(my_instance_data->report_data, "vkCreateInstance", "pCreateInfo->VkApplicationInfo->pEngineName",
                                pCreateInfo->pApplicationInfo->pEngineName);
            }
        }

        // Disable the tmp callbacks:
        if (my_instance_data->num_tmp_callbacks > 0) {
            layer_disable_tmp_callbacks(my_instance_data->report_data, my_instance_data->num_tmp_callbacks,
                                        my_instance_data->tmp_callbacks);
        }
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    bool skip = false;
    auto instance_data = GetLayerDataPtr(key, instance_layer_data_map);

    // Enable the temporary callback(s) here to catch vkDestroyInstance issues:
    bool callback_setup = false;
    if (instance_data->num_tmp_callbacks > 0) {
        if (!layer_enable_tmp_callbacks(instance_data->report_data, instance_data->num_tmp_callbacks,
                                        instance_data->tmp_dbg_create_infos, instance_data->tmp_callbacks)) {
            callback_setup = true;
        }
    }

    skip |= parameter_validation_vkDestroyInstance(instance, pAllocator);

    // Disable and cleanup the temporary callback(s):
    if (callback_setup) {
        layer_disable_tmp_callbacks(instance_data->report_data, instance_data->num_tmp_callbacks, instance_data->tmp_callbacks);
    }
    if (instance_data->num_tmp_callbacks > 0) {
        layer_free_tmp_callbacks(instance_data->tmp_dbg_create_infos, instance_data->tmp_callbacks);
        instance_data->num_tmp_callbacks = 0;
    }

    if (!skip) {
        instance_data->dispatch_table.DestroyInstance(instance, pAllocator);

        // Clean up logging callback, if any
        while (instance_data->logging_callback.size() > 0) {
            VkDebugReportCallbackEXT callback = instance_data->logging_callback.back();
            layer_destroy_msg_callback(instance_data->report_data, callback, pAllocator);
            instance_data->logging_callback.pop_back();
        }

        layer_debug_report_destroy_instance(instance_data->report_data);
    }

    FreeLayerDataPtr(key, instance_layer_data_map);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugReportCallbackEXT *pMsgCallback) {
    bool skip = parameter_validation_vkCreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;

    auto instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    VkResult result = instance_data->dispatch_table.CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (result == VK_SUCCESS) {
        result = layer_create_msg_callback(instance_data->report_data, false, pCreateInfo, pAllocator, pMsgCallback);
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                           const VkAllocationCallbacks *pAllocator) {
    bool skip = parameter_validation_vkDestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    if (!skip) {
        auto instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
        instance_data->dispatch_table.DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
        layer_destroy_msg_callback(instance_data->report_data, msgCallback, pAllocator);
    }
}

static bool ValidateDeviceCreateInfo(instance_layer_data *instance_data, VkPhysicalDevice physicalDevice,
                                     const VkDeviceCreateInfo *pCreateInfo) {
    bool skip = false;

    if ((pCreateInfo->enabledLayerCount > 0) && (pCreateInfo->ppEnabledLayerNames != NULL)) {
        for (size_t i = 0; i < pCreateInfo->enabledLayerCount; i++) {
            skip |= validate_string(instance_data->report_data, "vkCreateDevice", "pCreateInfo->ppEnabledLayerNames",
                                    pCreateInfo->ppEnabledLayerNames[i]);
        }
    }

    bool maint1 = false;
    bool negative_viewport = false;

    if ((pCreateInfo->enabledExtensionCount > 0) && (pCreateInfo->ppEnabledExtensionNames != NULL)) {
        for (size_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
            skip |= validate_string(instance_data->report_data, "vkCreateDevice", "pCreateInfo->ppEnabledExtensionNames",
                                    pCreateInfo->ppEnabledExtensionNames[i]);
            if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MAINTENANCE1_EXTENSION_NAME) == 0) maint1 = true;
            if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME) == 0)
                negative_viewport = true;
        }
    }

    if (maint1 && negative_viewport) {
        skip |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, VALIDATION_ERROR_056002ec, LayerName,
                        "VkDeviceCreateInfo->ppEnabledExtensionNames must not simultaneously include VK_KHR_maintenance1 and "
                        "VK_AMD_negative_viewport_height. %s",
                        validation_error_map[VALIDATION_ERROR_056002ec]);
    }

    if (pCreateInfo->pNext != NULL && pCreateInfo->pEnabledFeatures) {
        // Check for get_physical_device_properties2 struct
        struct std_header {
            VkStructureType sType;
            const void *pNext;
        };
        std_header *cur_pnext = (std_header *)pCreateInfo->pNext;
        while (cur_pnext) {
            if (VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR == cur_pnext->sType) {
                // Cannot include VkPhysicalDeviceFeatures2KHR and have non-null pEnabledFeatures
                skip |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
                                0, __LINE__, INVALID_USAGE, LayerName,
                                "VkDeviceCreateInfo->pNext includes a VkPhysicalDeviceFeatures2KHR struct when "
                                "pCreateInfo->pEnabledFeatures is non-NULL.");
                break;
            }
            cur_pnext = (std_header *)cur_pnext->pNext;
        }
    }
    if (pCreateInfo->pNext != NULL && pCreateInfo->pEnabledFeatures) {
        // Check for get_physical_device_properties2 struct
        struct std_header {
            VkStructureType sType;
            const void *pNext;
        };
        std_header *cur_pnext = (std_header *)pCreateInfo->pNext;
        while (cur_pnext) {
            if (VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR == cur_pnext->sType) {
                // Cannot include VkPhysicalDeviceFeatures2KHR and have non-null pEnabledFeatures
                skip |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
                                0, __LINE__, INVALID_USAGE, LayerName,
                                "VkDeviceCreateInfo->pNext includes a VkPhysicalDeviceFeatures2KHR struct when "
                                "pCreateInfo->pEnabledFeatures is non-NULL.");
                break;
            }
            cur_pnext = (std_header *)cur_pnext->pNext;
        }
    }

    // Validate pCreateInfo->pQueueCreateInfos
    if (pCreateInfo->pQueueCreateInfos) {
        std::unordered_set<uint32_t> set;

        for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
            const uint32_t requested_queue_family = pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex;
            if (requested_queue_family == VK_QUEUE_FAMILY_IGNORED) {
                skip |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, HandleToUint64(physicalDevice), __LINE__,
                                VALIDATION_ERROR_06c002fa, LayerName,
                                "vkCreateDevice: pCreateInfo->pQueueCreateInfos[%" PRIu32
                                "].queueFamilyIndex is "
                                "VK_QUEUE_FAMILY_IGNORED, but it is required to provide a valid queue family index value. %s",
                                i, validation_error_map[VALIDATION_ERROR_06c002fa]);
            } else if (set.count(requested_queue_family)) {
                skip |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, HandleToUint64(physicalDevice), __LINE__,
                                VALIDATION_ERROR_056002e8, LayerName,
                                "vkCreateDevice: pCreateInfo->pQueueCreateInfos[%" PRIu32 "].queueFamilyIndex (=%" PRIu32
                                ") is "
                                "not unique within pCreateInfo->pQueueCreateInfos array. %s",
                                i, requested_queue_family, validation_error_map[VALIDATION_ERROR_056002e8]);
            } else {
                set.insert(requested_queue_family);
            }

            if (pCreateInfo->pQueueCreateInfos[i].pQueuePriorities != nullptr) {
                for (uint32_t j = 0; j < pCreateInfo->pQueueCreateInfos[i].queueCount; ++j) {
                    const float queue_priority = pCreateInfo->pQueueCreateInfos[i].pQueuePriorities[j];
                    if (!(queue_priority >= 0.f) || !(queue_priority <= 1.f)) {
                        skip |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, HandleToUint64(physicalDevice), __LINE__,
                                        VALIDATION_ERROR_06c002fe, LayerName,
                                        "vkCreateDevice: pCreateInfo->pQueueCreateInfos[%" PRIu32 "].pQueuePriorities[%" PRIu32
                                        "] (=%f) is not between 0 and 1 (inclusive). %s",
                                        i, j, queue_priority, validation_error_map[VALIDATION_ERROR_06c002fe]);
                    }
                }
            }
        }
    }

    return skip;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                                              const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    // NOTE: Don't validate physicalDevice or any dispatchable object as the first parameter. We couldn't get here if it was wrong!

    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip = false;
    auto my_instance_data = GetLayerDataPtr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    assert(my_instance_data != nullptr);
    std::unique_lock<std::mutex> lock(global_lock);

    skip |= parameter_validation_vkCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

    if (pCreateInfo != NULL) skip |= ValidateDeviceCreateInfo(my_instance_data, physicalDevice, pCreateInfo);

    if (!skip) {
        VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);
        assert(chain_info != nullptr);
        assert(chain_info->u.pLayerInfo != nullptr);

        PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
        PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
        PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(my_instance_data->instance, "vkCreateDevice");
        if (fpCreateDevice == NULL) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        // Advance the link info for the next element on the chain
        chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

        lock.unlock();

        result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);

        lock.lock();

        validate_result(my_instance_data->report_data, "vkCreateDevice", {}, result);

        if (result == VK_SUCCESS) {
            layer_data *my_device_data = GetLayerDataPtr(get_dispatch_key(*pDevice), layer_data_map);
            assert(my_device_data != nullptr);

            my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
            layer_init_device_dispatch_table(*pDevice, &my_device_data->dispatch_table, fpGetDeviceProcAddr);

            my_device_data->extensions.InitFromDeviceCreateInfo(&my_instance_data->extensions, pCreateInfo);

            // Store createdevice data
            if ((pCreateInfo != nullptr) && (pCreateInfo->pQueueCreateInfos != nullptr)) {
                for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
                    my_device_data->queueFamilyIndexMap.insert(std::make_pair(pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex,
                                                                              pCreateInfo->pQueueCreateInfos[i].queueCount));
                }
            }

            // Query and save physical device limits for this device
            VkPhysicalDeviceProperties device_properties = {};
            my_instance_data->dispatch_table.GetPhysicalDeviceProperties(physicalDevice, &device_properties);
            memcpy(&my_device_data->device_limits, &device_properties.limits, sizeof(VkPhysicalDeviceLimits));
            my_device_data->physical_device = physicalDevice;
            my_device_data->device = *pDevice;

            // Save app-enabled features in this device's layer_data structure
            if (pCreateInfo->pEnabledFeatures) {
                my_device_data->physical_device_features = *pCreateInfo->pEnabledFeatures;
            } else {
                memset(&my_device_data->physical_device_features, 0, sizeof(VkPhysicalDeviceFeatures));
            }
        }
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(device);
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(key, layer_data_map);
    {
        std::unique_lock<std::mutex> lock(global_lock);
        skip |= parameter_validation_vkDestroyDevice(device, pAllocator);
    }

    if (!skip) {
        layer_debug_report_destroy_device(device);
        device_data->dispatch_table.DestroyDevice(device, pAllocator);
    }
    FreeLayerDataPtr(key, layer_data_map);
}

bool pv_vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    skip |=
        ValidateDeviceQueueFamily(device_data, queueFamilyIndex, "vkGetDeviceQueue", "queueFamilyIndex", VALIDATION_ERROR_29600300);
    const auto &queue_data = device_data->queueFamilyIndexMap.find(queueFamilyIndex);
    if (queue_data != device_data->queueFamilyIndexMap.end() && queue_data->second <= queueIndex) {
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        HandleToUint64(device), __LINE__, VALIDATION_ERROR_29600302, LayerName,
                        "vkGetDeviceQueue: queueIndex (=%" PRIu32
                        ") is not less than the number of queues requested from "
                        "queueFamilyIndex (=%" PRIu32 ") when the device was created (i.e. is not less than %" PRIu32 "). %s",
                        queueIndex, queueFamilyIndex, queue_data->second, validation_error_map[VALIDATION_ERROR_29600302]);
    }
    return skip;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool) {
    layer_data *local_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip = false;
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    std::unique_lock<std::mutex> lock(global_lock);

    skip |= ValidateDeviceQueueFamily(local_data, pCreateInfo->queueFamilyIndex, "vkCreateCommandPool",
                                      "pCreateInfo->queueFamilyIndex", VALIDATION_ERROR_02c0004e);

    skip |= parameter_validation_vkCreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);

    lock.unlock();
    if (!skip) {
        result = local_data->dispatch_table.CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    skip |= parameter_validation_vkCreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    if (pCreateInfo != nullptr) {
        // If queryType is VK_QUERY_TYPE_PIPELINE_STATISTICS, pipelineStatistics must be a valid combination of
        // VkQueryPipelineStatisticFlagBits values
        if ((pCreateInfo->queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS) && (pCreateInfo->pipelineStatistics != 0) &&
            ((pCreateInfo->pipelineStatistics & (~AllVkQueryPipelineStatisticFlagBits)) != 0)) {
            skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_11c00630, LayerName,
                            "vkCreateQueryPool(): if pCreateInfo->queryType is "
                            "VK_QUERY_TYPE_PIPELINE_STATISTICS, pCreateInfo->pipelineStatistics must be "
                            "a valid combination of VkQueryPipelineStatisticFlagBits values. %s",
                            validation_error_map[VALIDATION_ERROR_11c00630]);
        }
    }
    if (!skip) {
        result = device_data->dispatch_table.CreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
    }
    return result;
}

bool pv_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                       VkBuffer *pBuffer) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    if (pCreateInfo != nullptr) {
        // Buffer size must be greater than 0 (error 00663)
        skip |=
            ValidateGreaterThan(report_data, "vkCreateBuffer", "pCreateInfo->size", static_cast<uint32_t>(pCreateInfo->size), 0u);

        // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
        if (pCreateInfo->sharingMode == VK_SHARING_MODE_CONCURRENT) {
            // If sharingMode is VK_SHARING_MODE_CONCURRENT, queueFamilyIndexCount must be greater than 1
            if (pCreateInfo->queueFamilyIndexCount <= 1) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_01400724, LayerName,
                                "vkCreateBuffer: if pCreateInfo->sharingMode is VK_SHARING_MODE_CONCURRENT, "
                                "pCreateInfo->queueFamilyIndexCount must be greater than 1. %s",
                                validation_error_map[VALIDATION_ERROR_01400724]);
            }

            // If sharingMode is VK_SHARING_MODE_CONCURRENT, pQueueFamilyIndices must be a pointer to an array of
            // queueFamilyIndexCount uint32_t values
            if (pCreateInfo->pQueueFamilyIndices == nullptr) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_01400722, LayerName,
                                "vkCreateBuffer: if pCreateInfo->sharingMode is VK_SHARING_MODE_CONCURRENT, "
                                "pCreateInfo->pQueueFamilyIndices must be a pointer to an array of "
                                "pCreateInfo->queueFamilyIndexCount uint32_t values. %s",
                                validation_error_map[VALIDATION_ERROR_01400722]);
            } else {
                // TODO: Not in the spec VUs. Probably missing -- KhronosGroup/Vulkan-Docs#501. Update error codes when resolved.
                skip |= ValidateQueueFamilies(device_data, pCreateInfo->queueFamilyIndexCount, pCreateInfo->pQueueFamilyIndices,
                                              "vkCreateBuffer", "pCreateInfo->pQueueFamilyIndices", INVALID_USAGE, INVALID_USAGE,
                                              false, "", "");
            }
        }

        // If flags contains VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT or VK_BUFFER_CREATE_SPARSE_ALIASED_BIT, it must also contain
        // VK_BUFFER_CREATE_SPARSE_BINDING_BIT
        if (((pCreateInfo->flags & (VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT | VK_BUFFER_CREATE_SPARSE_ALIASED_BIT)) != 0) &&
            ((pCreateInfo->flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT) != VK_BUFFER_CREATE_SPARSE_BINDING_BIT)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_0140072c, LayerName,
                            "vkCreateBuffer: if pCreateInfo->flags contains VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT or "
                            "VK_BUFFER_CREATE_SPARSE_ALIASED_BIT, it must also contain VK_BUFFER_CREATE_SPARSE_BINDING_BIT. %s",
                            validation_error_map[VALIDATION_ERROR_0140072c]);
        }
    }

    return skip;
}

bool pv_vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                      VkImage *pImage) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    if (pCreateInfo != nullptr) {
        if ((device_data->physical_device_features.textureCompressionETC2 == false) &&
            FormatIsCompressed_ETC2_EAC(pCreateInfo->format)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            DEVICE_FEATURE, LayerName,
                            "vkCreateImage(): Attempting to create VkImage with format %s. The textureCompressionETC2 feature is "
                            "not enabled: neither ETC2 nor EAC formats can be used to create images.",
                            string_VkFormat(pCreateInfo->format));
        }

        if ((device_data->physical_device_features.textureCompressionASTC_LDR == false) &&
            FormatIsCompressed_ASTC_LDR(pCreateInfo->format)) {
            skip |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        DEVICE_FEATURE, LayerName,
                        "vkCreateImage(): Attempting to create VkImage with format %s. The textureCompressionASTC_LDR feature is "
                        "not enabled: ASTC formats cannot be used to create images.",
                        string_VkFormat(pCreateInfo->format));
        }

        if ((device_data->physical_device_features.textureCompressionBC == false) && FormatIsCompressed_BC(pCreateInfo->format)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            DEVICE_FEATURE, LayerName,
                            "vkCreateImage(): Attempting to create VkImage with format %s. The textureCompressionBC feature is "
                            "not enabled: BC compressed formats cannot be used to create images.",
                            string_VkFormat(pCreateInfo->format));
        }

        // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
        if (pCreateInfo->sharingMode == VK_SHARING_MODE_CONCURRENT) {
            // If sharingMode is VK_SHARING_MODE_CONCURRENT, queueFamilyIndexCount must be greater than 1
            if (pCreateInfo->queueFamilyIndexCount <= 1) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_09e0075c, LayerName,
                                "vkCreateImage(): if pCreateInfo->sharingMode is VK_SHARING_MODE_CONCURRENT, "
                                "pCreateInfo->queueFamilyIndexCount must be greater than 1. %s",
                                validation_error_map[VALIDATION_ERROR_09e0075c]);
            }

            // If sharingMode is VK_SHARING_MODE_CONCURRENT, pQueueFamilyIndices must be a pointer to an array of
            // queueFamilyIndexCount uint32_t values
            if (pCreateInfo->pQueueFamilyIndices == nullptr) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_09e0075a, LayerName,
                                "vkCreateImage(): if pCreateInfo->sharingMode is VK_SHARING_MODE_CONCURRENT, "
                                "pCreateInfo->pQueueFamilyIndices must be a pointer to an array of "
                                "pCreateInfo->queueFamilyIndexCount uint32_t values. %s",
                                validation_error_map[VALIDATION_ERROR_09e0075a]);
            } else {
                // TODO: Not in the spec VUs. Probably missing -- KhronosGroup/Vulkan-Docs#501. Update error codes when resolved.
                skip |= ValidateQueueFamilies(device_data, pCreateInfo->queueFamilyIndexCount, pCreateInfo->pQueueFamilyIndices,
                                              "vkCreateImage", "pCreateInfo->pQueueFamilyIndices", INVALID_USAGE, INVALID_USAGE,
                                              false, "", "");
            }
        }

        // width, height, and depth members of extent must be greater than 0
        skip |= ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->extent.width", pCreateInfo->extent.width, 0u);
        skip |= ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->extent.height", pCreateInfo->extent.height, 0u);
        skip |= ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->extent.depth", pCreateInfo->extent.depth, 0u);

        // mipLevels must be greater than 0
        skip |= ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->mipLevels", pCreateInfo->mipLevels, 0u);

        // arrayLayers must be greater than 0
        skip |= ValidateGreaterThan(report_data, "vkCreateImage", "pCreateInfo->arrayLayers", pCreateInfo->arrayLayers, 0u);

        // If imageType is VK_IMAGE_TYPE_1D, both extent.height and extent.depth must be 1
        if ((pCreateInfo->imageType == VK_IMAGE_TYPE_1D) && (pCreateInfo->extent.height != 1) && (pCreateInfo->extent.depth != 1)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_09e00778, LayerName,
                            "vkCreateImage(): if pCreateInfo->imageType is VK_IMAGE_TYPE_1D, both "
                            "pCreateInfo->extent.height and pCreateInfo->extent.depth must be 1. %s",
                            validation_error_map[VALIDATION_ERROR_09e00778]);
        }

        if (pCreateInfo->imageType == VK_IMAGE_TYPE_2D) {
            // If imageType is VK_IMAGE_TYPE_2D and flags contains VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, extent.width and
            // extent.height must be equal
            if ((pCreateInfo->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
                (pCreateInfo->extent.width != pCreateInfo->extent.height)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_09e00774, LayerName,
                                "vkCreateImage(): if pCreateInfo->imageType is VK_IMAGE_TYPE_2D and "
                                "pCreateInfo->flags contains VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, "
                                "pCreateInfo->extent.width and pCreateInfo->extent.height must be equal. %s",
                                validation_error_map[VALIDATION_ERROR_09e00774]);
            }

            if (pCreateInfo->extent.depth != 1) {
                skip |= log_msg(
                    report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    VALIDATION_ERROR_09e0077a, LayerName,
                    "vkCreateImage(): if pCreateInfo->imageType is VK_IMAGE_TYPE_2D, pCreateInfo->extent.depth must be 1. %s",
                    validation_error_map[VALIDATION_ERROR_09e0077a]);
            }
        }

        // mipLevels must be less than or equal to floor(log2(max(extent.width,extent.height,extent.depth)))+1
        uint32_t maxDim = std::max(std::max(pCreateInfo->extent.width, pCreateInfo->extent.height), pCreateInfo->extent.depth);
        if (pCreateInfo->mipLevels > (floor(log2(maxDim)) + 1)) {
            skip |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_09e0077c, LayerName,
                        "vkCreateImage(): pCreateInfo->mipLevels must be less than or equal to "
                        "floor(log2(max(pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth)))+1. %s",
                        validation_error_map[VALIDATION_ERROR_09e0077c]);
        }

        // If flags contains VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT or VK_IMAGE_CREATE_SPARSE_ALIASED_BIT, it must also contain
        // VK_IMAGE_CREATE_SPARSE_BINDING_BIT
        if (((pCreateInfo->flags & (VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_ALIASED_BIT)) != 0) &&
            ((pCreateInfo->flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) != VK_IMAGE_CREATE_SPARSE_BINDING_BIT)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_09e007b6, LayerName,
                            "vkCreateImage: if pCreateInfo->flags contains VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT or "
                            "VK_IMAGE_CREATE_SPARSE_ALIASED_BIT, it must also contain VK_IMAGE_CREATE_SPARSE_BINDING_BIT. %s",
                            validation_error_map[VALIDATION_ERROR_09e007b6]);
        }

        // Check for combinations of attributes that are incompatible with having VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT set
        if ((pCreateInfo->flags & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT) != 0) {
            // Linear tiling is unsupported
            if (VK_IMAGE_TILING_LINEAR == pCreateInfo->tiling) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                INVALID_USAGE, LayerName,
                                "vkCreateImage: if pCreateInfo->flags contains VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT "
                                "then image tiling of VK_IMAGE_TILING_LINEAR is not supported");
            }

            // Sparse 1D image isn't valid
            if (VK_IMAGE_TYPE_1D == pCreateInfo->imageType) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_09e00794, LayerName,
                                "vkCreateImage: cannot specify VK_IMAGE_CREATE_SPARSE_BINDING_BIT for 1D image. %s",
                                validation_error_map[VALIDATION_ERROR_09e00794]);
            }

            // Sparse 2D image when device doesn't support it
            if ((VK_FALSE == device_data->physical_device_features.sparseResidencyImage2D) &&
                (VK_IMAGE_TYPE_2D == pCreateInfo->imageType)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_09e00796, LayerName,
                                "vkCreateImage: cannot specify VK_IMAGE_CREATE_SPARSE_BINDING_BIT for 2D image if corresponding "
                                "feature is not enabled on the device. %s",
                                validation_error_map[VALIDATION_ERROR_09e00796]);
            }

            // Sparse 3D image when device doesn't support it
            if ((VK_FALSE == device_data->physical_device_features.sparseResidencyImage3D) &&
                (VK_IMAGE_TYPE_3D == pCreateInfo->imageType)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_09e00798, LayerName,
                                "vkCreateImage: cannot specify VK_IMAGE_CREATE_SPARSE_BINDING_BIT for 3D image if corresponding "
                                "feature is not enabled on the device. %s",
                                validation_error_map[VALIDATION_ERROR_09e00798]);
            }

            // Multi-sample 2D image when device doesn't support it
            if (VK_IMAGE_TYPE_2D == pCreateInfo->imageType) {
                if ((VK_FALSE == device_data->physical_device_features.sparseResidency2Samples) &&
                    (VK_SAMPLE_COUNT_2_BIT == pCreateInfo->samples)) {
                    skip |= log_msg(
                        report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_09e0079a, LayerName,
                        "vkCreateImage: cannot specify VK_IMAGE_CREATE_SPARSE_BINDING_BIT for 2-sample image if corresponding "
                        "feature is not enabled on the device. %s",
                        validation_error_map[VALIDATION_ERROR_09e0079a]);
                } else if ((VK_FALSE == device_data->physical_device_features.sparseResidency4Samples) &&
                           (VK_SAMPLE_COUNT_4_BIT == pCreateInfo->samples)) {
                    skip |= log_msg(
                        report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_09e0079c, LayerName,
                        "vkCreateImage: cannot specify VK_IMAGE_CREATE_SPARSE_BINDING_BIT for 4-sample image if corresponding "
                        "feature is not enabled on the device. %s",
                        validation_error_map[VALIDATION_ERROR_09e0079c]);
                } else if ((VK_FALSE == device_data->physical_device_features.sparseResidency8Samples) &&
                           (VK_SAMPLE_COUNT_8_BIT == pCreateInfo->samples)) {
                    skip |= log_msg(
                        report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_09e0079e, LayerName,
                        "vkCreateImage: cannot specify VK_IMAGE_CREATE_SPARSE_BINDING_BIT for 8-sample image if corresponding "
                        "feature is not enabled on the device. %s",
                        validation_error_map[VALIDATION_ERROR_09e0079e]);
                } else if ((VK_FALSE == device_data->physical_device_features.sparseResidency16Samples) &&
                           (VK_SAMPLE_COUNT_16_BIT == pCreateInfo->samples)) {
                    skip |= log_msg(
                        report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_09e007a0, LayerName,
                        "vkCreateImage: cannot specify VK_IMAGE_CREATE_SPARSE_BINDING_BIT for 16-sample image if corresponding "
                        "feature is not enabled on the device. %s",
                        validation_error_map[VALIDATION_ERROR_09e007a0]);
                }
            }
        }
    }
    return skip;
}

bool pv_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                          VkImageView *pView) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    if (pCreateInfo != nullptr) {
        if ((pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_1D) || (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_2D)) {
            if ((pCreateInfo->subresourceRange.layerCount != 1) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, 1,
                                LayerName,
                                "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_%dD, "
                                "pCreateInfo->subresourceRange.layerCount must be 1",
                                ((pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_1D) ? 1 : 2));
            }
        } else if ((pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY) ||
                   (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY)) {
            if ((pCreateInfo->subresourceRange.layerCount < 1) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, 1,
                                LayerName,
                                "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_%dD_ARRAY, "
                                "pCreateInfo->subresourceRange.layerCount must be >= 1",
                                ((pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY) ? 1 : 2));
            }
        } else if (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_CUBE) {
            if ((pCreateInfo->subresourceRange.layerCount != 6) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, 1,
                                LayerName,
                                "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_CUBE, "
                                "pCreateInfo->subresourceRange.layerCount must be 6");
            }
        } else if (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
            if (((pCreateInfo->subresourceRange.layerCount == 0) || ((pCreateInfo->subresourceRange.layerCount % 6) != 0)) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, 1,
                                LayerName,
                                "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_CUBE_ARRAY, "
                                "pCreateInfo->subresourceRange.layerCount must be a multiple of 6");
            }
            if (!device_data->physical_device_features.imageCubeArray) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, 1,
                                LayerName, "vkCreateImageView: Device feature imageCubeArray not enabled.");
            }
        } else if (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_3D) {
            if (pCreateInfo->subresourceRange.baseArrayLayer != 0) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, 1,
                                LayerName,
                                "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_3D, "
                                "pCreateInfo->subresourceRange.baseArrayLayer must be 0");
            }

            if ((pCreateInfo->subresourceRange.layerCount != 1) &&
                (pCreateInfo->subresourceRange.layerCount != VK_REMAINING_ARRAY_LAYERS)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, 1,
                                LayerName,
                                "vkCreateImageView: if pCreateInfo->viewType is VK_IMAGE_TYPE_3D, "
                                "pCreateInfo->subresourceRange.layerCount must be 1");
            }
        }
    }
    return skip;
}

bool pv_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                  const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                                  VkPipeline *pPipelines) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    if (pCreateInfos != nullptr) {
        for (uint32_t i = 0; i < createInfoCount; ++i) {
            // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
            if (pCreateInfos[i].pVertexInputState != nullptr) {
                auto const &vertex_input_state = pCreateInfos[i].pVertexInputState;
                for (uint32_t d = 0; d < vertex_input_state->vertexBindingDescriptionCount; ++d) {
                    auto const &vertex_bind_desc = vertex_input_state->pVertexBindingDescriptions[d];
                    if (vertex_bind_desc.binding >= device_data->device_limits.maxVertexInputBindings) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_14c004d4, LayerName,
                                        "vkCreateGraphicsPipelines: parameter "
                                        "pCreateInfos[%u].pVertexInputState->pVertexBindingDescriptions[%u].binding (%u) is "
                                        "greater than or equal to VkPhysicalDeviceLimits::maxVertexInputBindings (%u). %s",
                                        i, d, vertex_bind_desc.binding, device_data->device_limits.maxVertexInputBindings,
                                        validation_error_map[VALIDATION_ERROR_14c004d4]);
                    }

                    if (vertex_bind_desc.stride > device_data->device_limits.maxVertexInputBindingStride) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_14c004d6, LayerName,
                                        "vkCreateGraphicsPipelines: parameter "
                                        "pCreateInfos[%u].pVertexInputState->pVertexBindingDescriptions[%u].stride (%u) is greater "
                                        "than VkPhysicalDeviceLimits::maxVertexInputBindingStride (%u). %s",
                                        i, d, vertex_bind_desc.stride, device_data->device_limits.maxVertexInputBindingStride,
                                        validation_error_map[VALIDATION_ERROR_14c004d6]);
                    }
                }

                for (uint32_t d = 0; d < vertex_input_state->vertexAttributeDescriptionCount; ++d) {
                    auto const &vertex_attrib_desc = vertex_input_state->pVertexAttributeDescriptions[d];
                    if (vertex_attrib_desc.location >= device_data->device_limits.maxVertexInputAttributes) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_14a004d8, LayerName,
                                        "vkCreateGraphicsPipelines: parameter "
                                        "pCreateInfos[%u].pVertexInputState->pVertexAttributeDescriptions[%u].location (%u) is "
                                        "greater than or equal to VkPhysicalDeviceLimits::maxVertexInputAttributes (%u). %s",
                                        i, d, vertex_attrib_desc.location, device_data->device_limits.maxVertexInputAttributes,
                                        validation_error_map[VALIDATION_ERROR_14a004d8]);
                    }

                    if (vertex_attrib_desc.binding >= device_data->device_limits.maxVertexInputBindings) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_14a004da, LayerName,
                                        "vkCreateGraphicsPipelines: parameter "
                                        "pCreateInfos[%u].pVertexInputState->pVertexAttributeDescriptions[%u].binding (%u) is "
                                        "greater than or equal to VkPhysicalDeviceLimits::maxVertexInputBindings (%u). %s",
                                        i, d, vertex_attrib_desc.binding, device_data->device_limits.maxVertexInputBindings,
                                        validation_error_map[VALIDATION_ERROR_14a004da]);
                    }

                    if (vertex_attrib_desc.offset > device_data->device_limits.maxVertexInputAttributeOffset) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_14a004dc, LayerName,
                                        "vkCreateGraphicsPipelines: parameter "
                                        "pCreateInfos[%u].pVertexInputState->pVertexAttributeDescriptions[%u].offset (%u) is "
                                        "greater than VkPhysicalDeviceLimits::maxVertexInputAttributeOffset (%u). %s",
                                        i, d, vertex_attrib_desc.offset, device_data->device_limits.maxVertexInputAttributeOffset,
                                        validation_error_map[VALIDATION_ERROR_14a004dc]);
                    }
                }
            }

            if (pCreateInfos[i].pStages != nullptr) {
                bool has_control = false;
                bool has_eval = false;

                for (uint32_t stage_index = 0; stage_index < pCreateInfos[i].stageCount; ++stage_index) {
                    if (pCreateInfos[i].pStages[stage_index].stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
                        has_control = true;
                    } else if (pCreateInfos[i].pStages[stage_index].stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
                        has_eval = true;
                    }
                }

                // pTessellationState is ignored without both tessellation control and tessellation evaluation shaders stages
                if (has_control && has_eval) {
                    if (pCreateInfos[i].pTessellationState == nullptr) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_096005b6, LayerName,
                                        "vkCreateGraphicsPipelines: if pCreateInfos[%d].pStages includes a tessellation control "
                                        "shader stage and a tessellation evaluation shader stage, "
                                        "pCreateInfos[%d].pTessellationState must not be NULL. %s",
                                        i, i, validation_error_map[VALIDATION_ERROR_096005b6]);
                    } else {
                        skip |= validate_struct_pnext(
                            report_data, "vkCreateGraphicsPipelines",
                            ParameterName("pCreateInfos[%i].pTessellationState->pNext", ParameterName::IndexVector{i}), NULL,
                            pCreateInfos[i].pTessellationState->pNext, 0, NULL, GeneratedHeaderVersion, VALIDATION_ERROR_0961c40d);

                        skip |= validate_reserved_flags(
                            report_data, "vkCreateGraphicsPipelines",
                            ParameterName("pCreateInfos[%i].pTessellationState->flags", ParameterName::IndexVector{i}),
                            pCreateInfos[i].pTessellationState->flags, VALIDATION_ERROR_10809005);

                        if (pCreateInfos[i].pTessellationState->sType !=
                            VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO) {
                            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                            __LINE__, VALIDATION_ERROR_1082b00b, LayerName,
                                            "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pTessellationState->sType must "
                                            "be VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO. %s",
                                            i, validation_error_map[VALIDATION_ERROR_1082b00b]);
                        }

                        if (pCreateInfos[i].pTessellationState->patchControlPoints == 0 ||
                            pCreateInfos[i].pTessellationState->patchControlPoints >
                                device_data->device_limits.maxTessellationPatchSize) {
                            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                            __LINE__, VALIDATION_ERROR_1080097c, LayerName,
                                            "vkCreateGraphicsPipelines: invalid parameter "
                                            "pCreateInfos[%d].pTessellationState->patchControlPoints value %u. patchControlPoints "
                                            "should be >0 and <=%u. %s",
                                            i, pCreateInfos[i].pTessellationState->patchControlPoints,
                                            device_data->device_limits.maxTessellationPatchSize,
                                            validation_error_map[VALIDATION_ERROR_1080097c]);
                        }
                    }
                }
            }

            // pViewportState, pMultisampleState, pDepthStencilState, and pColorBlendState ignored when rasterization is disabled
            if ((pCreateInfos[i].pRasterizationState != nullptr) &&
                (pCreateInfos[i].pRasterizationState->rasterizerDiscardEnable == VK_FALSE)) {
                if (pCreateInfos[i].pViewportState == nullptr) {
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, VALIDATION_ERROR_096005dc, LayerName,
                                    "vkCreateGraphicsPipelines: if pCreateInfos[%d].pRasterizationState->rasterizerDiscardEnable "
                                    "is VK_FALSE, pCreateInfos[%d].pViewportState must be a pointer to a valid "
                                    "VkPipelineViewportStateCreateInfo structure. %s",
                                    i, i, validation_error_map[VALIDATION_ERROR_096005dc]);
                } else {
                    if (pCreateInfos[i].pViewportState->scissorCount != pCreateInfos[i].pViewportState->viewportCount) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_10c00988, LayerName,
                                        "Graphics Pipeline viewport count (%u) must match scissor count (%u). %s",
                                        pCreateInfos[i].pViewportState->viewportCount, pCreateInfos[i].pViewportState->scissorCount,
                                        validation_error_map[VALIDATION_ERROR_10c00988]);
                    }

                    skip |= validate_struct_pnext(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pViewportState->pNext", ParameterName::IndexVector{i}), NULL,
                        pCreateInfos[i].pViewportState->pNext, 0, NULL, GeneratedHeaderVersion, VALIDATION_ERROR_10c1c40d);

                    skip |= validate_reserved_flags(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pViewportState->flags", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pViewportState->flags, VALIDATION_ERROR_10c09005);

                    if (pCreateInfos[i].pViewportState->sType != VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, INVALID_STRUCT_STYPE, LayerName,
                                        "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pViewportState->sType must be "
                                        "VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO",
                                        i);
                    }

                    if (device_data->physical_device_features.multiViewport == false) {
                        if (pCreateInfos[i].pViewportState->viewportCount != 1) {
                            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                            __LINE__, VALIDATION_ERROR_10c00980, LayerName,
                                            "vkCreateGraphicsPipelines: The multiViewport feature is not enabled, so "
                                            "pCreateInfos[%d].pViewportState->viewportCount must be 1 but is %d. %s",
                                            i, pCreateInfos[i].pViewportState->viewportCount,
                                            validation_error_map[VALIDATION_ERROR_10c00980]);
                        }
                        if (pCreateInfos[i].pViewportState->scissorCount != 1) {
                            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                            __LINE__, VALIDATION_ERROR_10c00982, LayerName,
                                            "vkCreateGraphicsPipelines: The multiViewport feature is not enabled, so "
                                            "pCreateInfos[%d].pViewportState->scissorCount must be 1 but is %d. %s",
                                            i, pCreateInfos[i].pViewportState->scissorCount,
                                            validation_error_map[VALIDATION_ERROR_10c00982]);
                        }
                    } else {
                        if ((pCreateInfos[i].pViewportState->viewportCount < 1) ||
                            (pCreateInfos[i].pViewportState->viewportCount > device_data->device_limits.maxViewports)) {
                            skip |=
                                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_10c00984, LayerName,
                                        "vkCreateGraphicsPipelines: multiViewport feature is enabled; "
                                        "pCreateInfos[%d].pViewportState->viewportCount is %d but must be between 1 and "
                                        "maxViewports (%d), inclusive. %s",
                                        i, pCreateInfos[i].pViewportState->viewportCount, device_data->device_limits.maxViewports,
                                        validation_error_map[VALIDATION_ERROR_10c00984]);
                        }
                        if ((pCreateInfos[i].pViewportState->scissorCount < 1) ||
                            (pCreateInfos[i].pViewportState->scissorCount > device_data->device_limits.maxViewports)) {
                            skip |=
                                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_10c00986, LayerName,
                                        "vkCreateGraphicsPipelines: multiViewport feature is enabled; "
                                        "pCreateInfos[%d].pViewportState->scissorCount is %d but must be between 1 and "
                                        "maxViewports (%d), inclusive. %s",
                                        i, pCreateInfos[i].pViewportState->scissorCount, device_data->device_limits.maxViewports,
                                        validation_error_map[VALIDATION_ERROR_10c00986]);
                        }
                    }

                    if (pCreateInfos[i].pDynamicState != nullptr) {
                        bool has_dynamic_viewport = false;
                        bool has_dynamic_scissor = false;

                        for (uint32_t state_index = 0; state_index < pCreateInfos[i].pDynamicState->dynamicStateCount;
                             ++state_index) {
                            if (pCreateInfos[i].pDynamicState->pDynamicStates[state_index] == VK_DYNAMIC_STATE_VIEWPORT) {
                                has_dynamic_viewport = true;
                            } else if (pCreateInfos[i].pDynamicState->pDynamicStates[state_index] == VK_DYNAMIC_STATE_SCISSOR) {
                                has_dynamic_scissor = true;
                            }
                        }

                        // If no element of the pDynamicStates member of pDynamicState is VK_DYNAMIC_STATE_VIEWPORT, the pViewports
                        // member of pViewportState must be a pointer to an array of pViewportState->viewportCount VkViewport
                        // structures
                        if (!has_dynamic_viewport && (pCreateInfos[i].pViewportState->pViewports == nullptr)) {
                            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                            __LINE__, VALIDATION_ERROR_096005d6, LayerName,
                                            "vkCreateGraphicsPipelines: if pCreateInfos[%d].pDynamicState->pDynamicStates does not "
                                            "contain VK_DYNAMIC_STATE_VIEWPORT, pCreateInfos[%d].pViewportState->pViewports must "
                                            "not be NULL. %s",
                                            i, i, validation_error_map[VALIDATION_ERROR_096005d6]);
                        }

                        // If no element of the pDynamicStates member of pDynamicState is VK_DYNAMIC_STATE_SCISSOR, the pScissors
                        // member
                        // of pViewportState must be a pointer to an array of pViewportState->scissorCount VkRect2D structures
                        if (!has_dynamic_scissor && (pCreateInfos[i].pViewportState->pScissors == nullptr)) {
                            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                            __LINE__, VALIDATION_ERROR_096005d8, LayerName,
                                            "vkCreateGraphicsPipelines: if pCreateInfos[%d].pDynamicState->pDynamicStates does not "
                                            "contain VK_DYNAMIC_STATE_SCISSOR, pCreateInfos[%d].pViewportState->pScissors must not "
                                            "be NULL. %s",
                                            i, i, validation_error_map[VALIDATION_ERROR_096005d8]);
                        }
                    }
                }

                if (pCreateInfos[i].pMultisampleState == nullptr) {
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, VALIDATION_ERROR_096005de, LayerName,
                                    "vkCreateGraphicsPipelines: if pCreateInfos[%d].pRasterizationState->rasterizerDiscardEnable "
                                    "is VK_FALSE, pCreateInfos[%d].pMultisampleState must not be NULL. %s",
                                    i, i, validation_error_map[VALIDATION_ERROR_096005de]);
                } else {
                    skip |= validate_struct_pnext(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pMultisampleState->pNext", ParameterName::IndexVector{i}), NULL,
                        pCreateInfos[i].pMultisampleState->pNext, 0, NULL, GeneratedHeaderVersion, VALIDATION_ERROR_1001c40d);

                    skip |= validate_reserved_flags(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pMultisampleState->flags", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pMultisampleState->flags, VALIDATION_ERROR_10009005);

                    skip |= validate_bool32(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pMultisampleState->sampleShadingEnable", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pMultisampleState->sampleShadingEnable);

                    skip |= validate_array(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pMultisampleState->rasterizationSamples", ParameterName::IndexVector{i}),
                        ParameterName("pCreateInfos[%i].pMultisampleState->pSampleMask", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pMultisampleState->rasterizationSamples, pCreateInfos[i].pMultisampleState->pSampleMask,
                        true, false, VALIDATION_ERROR_UNDEFINED, VALIDATION_ERROR_UNDEFINED);

                    skip |= validate_bool32(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pMultisampleState->alphaToCoverageEnable", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pMultisampleState->alphaToCoverageEnable);

                    skip |= validate_bool32(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pMultisampleState->alphaToOneEnable", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pMultisampleState->alphaToOneEnable);

                    if (pCreateInfos[i].pMultisampleState->sType != VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, INVALID_STRUCT_STYPE, LayerName,
                                        "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pMultisampleState->sType must be "
                                        "VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO",
                                        i);
                    }
                }

                // TODO: Conditional NULL check based on subpass depth/stencil attachment
                if (pCreateInfos[i].pDepthStencilState != nullptr) {
                    skip |= validate_struct_pnext(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->pNext", ParameterName::IndexVector{i}), NULL,
                        pCreateInfos[i].pDepthStencilState->pNext, 0, NULL, GeneratedHeaderVersion, VALIDATION_ERROR_0f61c40d);

                    skip |= validate_reserved_flags(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->flags", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pDepthStencilState->flags, VALIDATION_ERROR_0f609005);

                    skip |= validate_bool32(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->depthTestEnable", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pDepthStencilState->depthTestEnable);

                    skip |= validate_bool32(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->depthWriteEnable", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pDepthStencilState->depthWriteEnable);

                    skip |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->depthCompareOp", ParameterName::IndexVector{i}),
                        "VkCompareOp", AllVkCompareOpEnums, pCreateInfos[i].pDepthStencilState->depthCompareOp,
                        VALIDATION_ERROR_0f604001);

                    skip |= validate_bool32(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->depthBoundsTestEnable", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pDepthStencilState->depthBoundsTestEnable);

                    skip |= validate_bool32(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->stencilTestEnable", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pDepthStencilState->stencilTestEnable);

                    skip |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->front.failOp", ParameterName::IndexVector{i}),
                        "VkStencilOp", AllVkStencilOpEnums, pCreateInfos[i].pDepthStencilState->front.failOp,
                        VALIDATION_ERROR_13a08601);

                    skip |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->front.passOp", ParameterName::IndexVector{i}),
                        "VkStencilOp", AllVkStencilOpEnums, pCreateInfos[i].pDepthStencilState->front.passOp,
                        VALIDATION_ERROR_13a27801);

                    skip |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->front.depthFailOp", ParameterName::IndexVector{i}),
                        "VkStencilOp", AllVkStencilOpEnums, pCreateInfos[i].pDepthStencilState->front.depthFailOp,
                        VALIDATION_ERROR_13a04201);

                    skip |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->front.compareOp", ParameterName::IndexVector{i}),
                        "VkCompareOp", AllVkCompareOpEnums, pCreateInfos[i].pDepthStencilState->front.compareOp,
                        VALIDATION_ERROR_0f604001);

                    skip |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->back.failOp", ParameterName::IndexVector{i}),
                        "VkStencilOp", AllVkStencilOpEnums, pCreateInfos[i].pDepthStencilState->back.failOp,
                        VALIDATION_ERROR_13a08601);

                    skip |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->back.passOp", ParameterName::IndexVector{i}),
                        "VkStencilOp", AllVkStencilOpEnums, pCreateInfos[i].pDepthStencilState->back.passOp,
                        VALIDATION_ERROR_13a27801);

                    skip |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->back.depthFailOp", ParameterName::IndexVector{i}),
                        "VkStencilOp", AllVkStencilOpEnums, pCreateInfos[i].pDepthStencilState->back.depthFailOp,
                        VALIDATION_ERROR_13a04201);

                    skip |= validate_ranged_enum(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pDepthStencilState->back.compareOp", ParameterName::IndexVector{i}),
                        "VkCompareOp", AllVkCompareOpEnums, pCreateInfos[i].pDepthStencilState->back.compareOp,
                        VALIDATION_ERROR_0f604001);

                    if (pCreateInfos[i].pDepthStencilState->sType != VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, INVALID_STRUCT_STYPE, LayerName,
                                        "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pDepthStencilState->sType must be "
                                        "VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO",
                                        i);
                    }
                }

                // TODO: Conditional NULL check based on subpass color attachment
                if (pCreateInfos[i].pColorBlendState != nullptr) {
                    skip |= validate_struct_pnext(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pColorBlendState->pNext", ParameterName::IndexVector{i}), NULL,
                        pCreateInfos[i].pColorBlendState->pNext, 0, NULL, GeneratedHeaderVersion, VALIDATION_ERROR_0f41c40d);

                    skip |= validate_reserved_flags(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pColorBlendState->flags", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pColorBlendState->flags, VALIDATION_ERROR_0f409005);

                    skip |= validate_bool32(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pColorBlendState->logicOpEnable", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pColorBlendState->logicOpEnable);

                    skip |= validate_array(
                        report_data, "vkCreateGraphicsPipelines",
                        ParameterName("pCreateInfos[%i].pColorBlendState->attachmentCount", ParameterName::IndexVector{i}),
                        ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments", ParameterName::IndexVector{i}),
                        pCreateInfos[i].pColorBlendState->attachmentCount, pCreateInfos[i].pColorBlendState->pAttachments, false,
                        true, VALIDATION_ERROR_UNDEFINED, VALIDATION_ERROR_UNDEFINED);

                    if (pCreateInfos[i].pColorBlendState->pAttachments != NULL) {
                        for (uint32_t attachmentIndex = 0; attachmentIndex < pCreateInfos[i].pColorBlendState->attachmentCount;
                             ++attachmentIndex) {
                            skip |= validate_bool32(report_data, "vkCreateGraphicsPipelines",
                                                    ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].blendEnable",
                                                                  ParameterName::IndexVector{i, attachmentIndex}),
                                                    pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].blendEnable);

                            skip |= validate_ranged_enum(
                                report_data, "vkCreateGraphicsPipelines",
                                ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].srcColorBlendFactor",
                                              ParameterName::IndexVector{i, attachmentIndex}),
                                "VkBlendFactor", AllVkBlendFactorEnums,
                                pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].srcColorBlendFactor,
                                VALIDATION_ERROR_0f22cc01);

                            skip |= validate_ranged_enum(
                                report_data, "vkCreateGraphicsPipelines",
                                ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].dstColorBlendFactor",
                                              ParameterName::IndexVector{i, attachmentIndex}),
                                "VkBlendFactor", AllVkBlendFactorEnums,
                                pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].dstColorBlendFactor,
                                VALIDATION_ERROR_0f207001);

                            skip |= validate_ranged_enum(
                                report_data, "vkCreateGraphicsPipelines",
                                ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].colorBlendOp",
                                              ParameterName::IndexVector{i, attachmentIndex}),
                                "VkBlendOp", AllVkBlendOpEnums,
                                pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].colorBlendOp,
                                VALIDATION_ERROR_0f202001);

                            skip |= validate_ranged_enum(
                                report_data, "vkCreateGraphicsPipelines",
                                ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].srcAlphaBlendFactor",
                                              ParameterName::IndexVector{i, attachmentIndex}),
                                "VkBlendFactor", AllVkBlendFactorEnums,
                                pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].srcAlphaBlendFactor,
                                VALIDATION_ERROR_0f22c601);

                            skip |= validate_ranged_enum(
                                report_data, "vkCreateGraphicsPipelines",
                                ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].dstAlphaBlendFactor",
                                              ParameterName::IndexVector{i, attachmentIndex}),
                                "VkBlendFactor", AllVkBlendFactorEnums,
                                pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].dstAlphaBlendFactor,
                                VALIDATION_ERROR_0f206a01);

                            skip |= validate_ranged_enum(
                                report_data, "vkCreateGraphicsPipelines",
                                ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].alphaBlendOp",
                                              ParameterName::IndexVector{i, attachmentIndex}),
                                "VkBlendOp", AllVkBlendOpEnums,
                                pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].alphaBlendOp,
                                VALIDATION_ERROR_0f200801);

                            skip |=
                                validate_flags(report_data, "vkCreateGraphicsPipelines",
                                               ParameterName("pCreateInfos[%i].pColorBlendState->pAttachments[%i].colorWriteMask",
                                                             ParameterName::IndexVector{i, attachmentIndex}),
                                               "VkColorComponentFlagBits", AllVkColorComponentFlagBits,
                                               pCreateInfos[i].pColorBlendState->pAttachments[attachmentIndex].colorWriteMask,
                                               false, false, VALIDATION_ERROR_0f202201);
                        }
                    }

                    if (pCreateInfos[i].pColorBlendState->sType != VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, INVALID_STRUCT_STYPE, LayerName,
                                        "vkCreateGraphicsPipelines: parameter pCreateInfos[%d].pColorBlendState->sType must be "
                                        "VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO",
                                        i);
                    }

                    // If logicOpEnable is VK_TRUE, logicOp must be a valid VkLogicOp value
                    if (pCreateInfos[i].pColorBlendState->logicOpEnable == VK_TRUE) {
                        skip |= validate_ranged_enum(
                            report_data, "vkCreateGraphicsPipelines",
                            ParameterName("pCreateInfos[%i].pColorBlendState->logicOp", ParameterName::IndexVector{i}), "VkLogicOp",
                            AllVkLogicOpEnums, pCreateInfos[i].pColorBlendState->logicOp, VALIDATION_ERROR_0f4004be);
                    }
                }
            }
        }

        if (pCreateInfos != nullptr) {
            if (pCreateInfos->flags & VK_PIPELINE_CREATE_DERIVATIVE_BIT) {
                if (pCreateInfos->basePipelineIndex != -1) {
                    if (pCreateInfos->basePipelineHandle != VK_NULL_HANDLE) {
                        skip |= log_msg(
                            report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_096005a8, LayerName,
                            "vkCreateGraphicsPipelines parameter, pCreateInfos->basePipelineHandle, must be VK_NULL_HANDLE if "
                            "pCreateInfos->flags "
                            "contains the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag and pCreateInfos->basePipelineIndex is not -1. %s",
                            validation_error_map[VALIDATION_ERROR_096005a8]);
                    }
                }

                if (pCreateInfos->basePipelineHandle != VK_NULL_HANDLE) {
                    if (pCreateInfos->basePipelineIndex != -1) {
                        skip |= log_msg(
                            report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_096005aa, LayerName,
                            "vkCreateGraphicsPipelines parameter, pCreateInfos->basePipelineIndex, must be -1 if "
                            "pCreateInfos->flags "
                            "contains the VK_PIPELINE_CREATE_DERIVATIVE_BIT flag and pCreateInfos->basePipelineHandle is not "
                            "VK_NULL_HANDLE. %s",
                            validation_error_map[VALIDATION_ERROR_096005aa]);
                    }
                }
            }

            if (pCreateInfos->pRasterizationState != nullptr) {
                if (pCreateInfos->pRasterizationState->cullMode & ~VK_CULL_MODE_FRONT_AND_BACK) {
                    skip |= log_msg(
                        report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        UNRECOGNIZED_VALUE, LayerName,
                        "vkCreateGraphicsPipelines parameter, VkCullMode pCreateInfos->pRasterizationState->cullMode, is an "
                        "unrecognized enumerator");
                }

                if ((pCreateInfos->pRasterizationState->polygonMode != VK_POLYGON_MODE_FILL) &&
                    (device_data->physical_device_features.fillModeNonSolid == false)) {
                    skip |= log_msg(
                        report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        DEVICE_FEATURE, LayerName,
                        "vkCreateGraphicsPipelines parameter, VkPolygonMode pCreateInfos->pRasterizationState->polygonMode cannot "
                        "be "
                        "VK_POLYGON_MODE_POINT or VK_POLYGON_MODE_LINE if VkPhysicalDeviceFeatures->fillModeNonSolid is false.");
                }
            }

            size_t i = 0;
            for (size_t j = 0; j < pCreateInfos[i].stageCount; j++) {
                skip |= validate_string(device_data->report_data, "vkCreateGraphicsPipelines",
                                        ParameterName("pCreateInfos[%i].pStages[%i].pName", ParameterName::IndexVector{i, j}),
                                        pCreateInfos[i].pStages[j].pName);
            }
        }
    }

    return skip;
}

bool pv_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                 const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                                 VkPipeline *pPipelines) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    for (uint32_t i = 0; i < createInfoCount; i++) {
        skip |= validate_string(device_data->report_data, "vkCreateComputePipelines",
                                ParameterName("pCreateInfos[%i].stage.pName", ParameterName::IndexVector{i}),
                                pCreateInfos[i].stage.pName);
    }

    return skip;
}

bool pv_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                        VkSampler *pSampler) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    if (pCreateInfo != nullptr) {
        if ((device_data->physical_device_features.samplerAnisotropy == false) && (pCreateInfo->maxAnisotropy != 1.0)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            DEVICE_FEATURE, LayerName,
                            "vkCreateSampler(): The samplerAnisotropy feature was not enabled at device-creation time, so the "
                            "maxAnisotropy member of the VkSamplerCreateInfo structure must be 1.0 but is %f.",
                            pCreateInfo->maxAnisotropy);
        }

        // If compareEnable is VK_TRUE, compareOp must be a valid VkCompareOp value
        if (pCreateInfo->compareEnable == VK_TRUE) {
            skip |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->compareOp", "VkCompareOp",
                                         AllVkCompareOpEnums, pCreateInfo->compareOp, VALIDATION_ERROR_12600870);
        }

        // If any of addressModeU, addressModeV or addressModeW are VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, borderColor must be a
        // valid VkBorderColor value
        if ((pCreateInfo->addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER) ||
            (pCreateInfo->addressModeV == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER) ||
            (pCreateInfo->addressModeW == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER)) {
            skip |= validate_ranged_enum(report_data, "vkCreateSampler", "pCreateInfo->borderColor", "VkBorderColor",
                                         AllVkBorderColorEnums, pCreateInfo->borderColor, VALIDATION_ERROR_1260086c);
        }

        // If any of addressModeU, addressModeV or addressModeW are VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE, the
        // VK_KHR_sampler_mirror_clamp_to_edge extension must be enabled
        if (!device_data->extensions.vk_khr_sampler_mirror_clamp_to_edge &&
            ((pCreateInfo->addressModeU == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE) ||
             (pCreateInfo->addressModeV == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE) ||
             (pCreateInfo->addressModeW == VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE))) {
            skip |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_1260086e, LayerName,
                        "vkCreateSampler(): A VkSamplerAddressMode value is set to VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE "
                        "but the VK_KHR_sampler_mirror_clamp_to_edge extension has not been enabled. %s",
                        validation_error_map[VALIDATION_ERROR_1260086e]);
        }
    }

    return skip;
}

bool pv_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                    const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayout) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    if ((pCreateInfo != nullptr) && (pCreateInfo->pBindings != nullptr)) {
        for (uint32_t i = 0; i < pCreateInfo->bindingCount; ++i) {
            if (pCreateInfo->pBindings[i].descriptorCount != 0) {
                // If descriptorType is VK_DESCRIPTOR_TYPE_SAMPLER or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, and descriptorCount
                // is not 0 and pImmutableSamplers is not NULL, pImmutableSamplers must be a pointer to an array of descriptorCount
                // valid VkSampler handles
                if (((pCreateInfo->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) ||
                     (pCreateInfo->pBindings[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) &&
                    (pCreateInfo->pBindings[i].pImmutableSamplers != nullptr)) {
                    for (uint32_t descriptor_index = 0; descriptor_index < pCreateInfo->pBindings[i].descriptorCount;
                         ++descriptor_index) {
                        if (pCreateInfo->pBindings[i].pImmutableSamplers[descriptor_index] == VK_NULL_HANDLE) {
                            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                            __LINE__, REQUIRED_PARAMETER, LayerName,
                                            "vkCreateDescriptorSetLayout: required parameter "
                                            "pCreateInfo->pBindings[%d].pImmutableSamplers[%d]"
                                            " specified as VK_NULL_HANDLE",
                                            i, descriptor_index);
                        }
                    }
                }

                // If descriptorCount is not 0, stageFlags must be a valid combination of VkShaderStageFlagBits values
                if ((pCreateInfo->pBindings[i].stageFlags != 0) &&
                    ((pCreateInfo->pBindings[i].stageFlags & (~AllVkShaderStageFlagBits)) != 0)) {
                    skip |= log_msg(
                        report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_04e00236, LayerName,
                        "vkCreateDescriptorSetLayout(): if pCreateInfo->pBindings[%d].descriptorCount is not 0, "
                        "pCreateInfo->pBindings[%d].stageFlags must be a valid combination of VkShaderStageFlagBits values. %s",
                        i, i, validation_error_map[VALIDATION_ERROR_04e00236]);
                }
            }
        }
    }

    return skip;
}

bool pv_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount,
                             const VkDescriptorSet *pDescriptorSets) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    // This is an array of handles, where the elements are allowed to be VK_NULL_HANDLE, and does not require any validation beyond
    // validate_array()
    skip |= validate_array(report_data, "vkFreeDescriptorSets", "descriptorSetCount", "pDescriptorSets", descriptorSetCount,
                           pDescriptorSets, true, true, VALIDATION_ERROR_UNDEFINED, VALIDATION_ERROR_UNDEFINED);
    return skip;
}

bool pv_vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites,
                               uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    if (pDescriptorWrites != NULL) {
        for (uint32_t i = 0; i < descriptorWriteCount; ++i) {
            // descriptorCount must be greater than 0
            if (pDescriptorWrites[i].descriptorCount == 0) {
                skip |=
                    log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_15c0441b, LayerName,
                            "vkUpdateDescriptorSets(): parameter pDescriptorWrites[%d].descriptorCount must be greater than 0. %s",
                            i, validation_error_map[VALIDATION_ERROR_15c0441b]);
            }

            // dstSet must be a valid VkDescriptorSet handle
            skip |= validate_required_handle(report_data, "vkUpdateDescriptorSets",
                                             ParameterName("pDescriptorWrites[%i].dstSet", ParameterName::IndexVector{i}),
                                             pDescriptorWrites[i].dstSet);

            if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
                // If descriptorType is VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                // pImageInfo must be a pointer to an array of descriptorCount valid VkDescriptorImageInfo structures
                if (pDescriptorWrites[i].pImageInfo == nullptr) {
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, VALIDATION_ERROR_15c00284, LayerName,
                                    "vkUpdateDescriptorSets(): if pDescriptorWrites[%d].descriptorType is "
                                    "VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, "
                                    "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or "
                                    "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, pDescriptorWrites[%d].pImageInfo must not be NULL. %s",
                                    i, i, validation_error_map[VALIDATION_ERROR_15c00284]);
                } else if (pDescriptorWrites[i].descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
                    // If descriptorType is VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE or VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, the imageView and imageLayout
                    // members of any given element of pImageInfo must be a valid VkImageView and VkImageLayout, respectively
                    for (uint32_t descriptor_index = 0; descriptor_index < pDescriptorWrites[i].descriptorCount;
                         ++descriptor_index) {
                        skip |= validate_required_handle(report_data, "vkUpdateDescriptorSets",
                                                         ParameterName("pDescriptorWrites[%i].pImageInfo[%i].imageView",
                                                                       ParameterName::IndexVector{i, descriptor_index}),
                                                         pDescriptorWrites[i].pImageInfo[descriptor_index].imageView);
                        skip |= validate_ranged_enum(report_data, "vkUpdateDescriptorSets",
                                                     ParameterName("pDescriptorWrites[%i].pImageInfo[%i].imageLayout",
                                                                   ParameterName::IndexVector{i, descriptor_index}),
                                                     "VkImageLayout", AllVkImageLayoutEnums,
                                                     pDescriptorWrites[i].pImageInfo[descriptor_index].imageLayout,
                                                     VALIDATION_ERROR_UNDEFINED);
                    }
                }
            } else if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                // If descriptorType is VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC or VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, pBufferInfo must be a
                // pointer to an array of descriptorCount valid VkDescriptorBufferInfo structures
                if (pDescriptorWrites[i].pBufferInfo == nullptr) {
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, VALIDATION_ERROR_15c00288, LayerName,
                                    "vkUpdateDescriptorSets(): if pDescriptorWrites[%d].descriptorType is "
                                    "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, "
                                    "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC or VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, "
                                    "pDescriptorWrites[%d].pBufferInfo must not be NULL. %s",
                                    i, i, validation_error_map[VALIDATION_ERROR_15c00288]);
                } else {
                    for (uint32_t descriptorIndex = 0; descriptorIndex < pDescriptorWrites[i].descriptorCount; ++descriptorIndex) {
                        skip |= validate_required_handle(report_data, "vkUpdateDescriptorSets",
                                                         ParameterName("pDescriptorWrites[%i].pBufferInfo[%i].buffer",
                                                                       ParameterName::IndexVector{i, descriptorIndex}),
                                                         pDescriptorWrites[i].pBufferInfo[descriptorIndex].buffer);
                    }
                }
            } else if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)) {
                // If descriptorType is VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER or VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
                // pTexelBufferView must be a pointer to an array of descriptorCount valid VkBufferView handles
                if (pDescriptorWrites[i].pTexelBufferView == nullptr) {
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, VALIDATION_ERROR_15c00286, LayerName,
                                    "vkUpdateDescriptorSets(): if pDescriptorWrites[%d].descriptorType is "
                                    "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER or VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, "
                                    "pDescriptorWrites[%d].pTexelBufferView must not be NULL. %s",
                                    i, i, validation_error_map[VALIDATION_ERROR_15c00286]);
                } else {
                    for (uint32_t descriptor_index = 0; descriptor_index < pDescriptorWrites[i].descriptorCount;
                         ++descriptor_index) {
                        skip |= validate_required_handle(report_data, "vkUpdateDescriptorSets",
                                                         ParameterName("pDescriptorWrites[%i].pTexelBufferView[%i]",
                                                                       ParameterName::IndexVector{i, descriptor_index}),
                                                         pDescriptorWrites[i].pTexelBufferView[descriptor_index]);
                    }
                }
            }

            if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
                (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)) {
                VkDeviceSize uniformAlignment = device_data->device_limits.minUniformBufferOffsetAlignment;
                for (uint32_t j = 0; j < pDescriptorWrites[i].descriptorCount; j++) {
                    if (pDescriptorWrites[i].pBufferInfo != NULL) {
                        if (SafeModulo(pDescriptorWrites[i].pBufferInfo[j].offset, uniformAlignment) != 0) {
                            skip |= log_msg(
                                device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, VALIDATION_ERROR_15c0028e, LayerName,
                                "vkUpdateDescriptorSets(): pDescriptorWrites[%d].pBufferInfo[%d].offset (0x%" PRIxLEAST64
                                ") must be a multiple of device limit minUniformBufferOffsetAlignment 0x%" PRIxLEAST64 ". %s",
                                i, j, pDescriptorWrites[i].pBufferInfo[j].offset, uniformAlignment,
                                validation_error_map[VALIDATION_ERROR_15c0028e]);
                        }
                    }
                }
            } else if ((pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                       (pDescriptorWrites[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) {
                VkDeviceSize storageAlignment = device_data->device_limits.minStorageBufferOffsetAlignment;
                for (uint32_t j = 0; j < pDescriptorWrites[i].descriptorCount; j++) {
                    if (pDescriptorWrites[i].pBufferInfo != NULL) {
                        if (SafeModulo(pDescriptorWrites[i].pBufferInfo[j].offset, storageAlignment) != 0) {
                            skip |= log_msg(
                                device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, VALIDATION_ERROR_15c00290, LayerName,
                                "vkUpdateDescriptorSets(): pDescriptorWrites[%d].pBufferInfo[%d].offset (0x%" PRIxLEAST64
                                ") must be a multiple of device limit minStorageBufferOffsetAlignment 0x%" PRIxLEAST64 ". %s",
                                i, j, pDescriptorWrites[i].pBufferInfo[j].offset, storageAlignment,
                                validation_error_map[VALIDATION_ERROR_15c00290]);
                        }
                    }
                }
            }
        }
    }
    return skip;
}

bool pv_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                           VkRenderPass *pRenderPass) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    uint32_t max_color_attachments = device_data->device_limits.maxColorAttachments;

    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        if (pCreateInfo->pAttachments[i].format == VK_FORMAT_UNDEFINED) {
            std::stringstream ss;
            ss << "vkCreateRenderPass: pCreateInfo->pAttachments[" << i << "].format is VK_FORMAT_UNDEFINED. "
               << validation_error_map[VALIDATION_ERROR_00809201];
            skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_00809201, "IMAGE", "%s", ss.str().c_str());
        }
        if (pCreateInfo->pAttachments[i].finalLayout == VK_IMAGE_LAYOUT_UNDEFINED ||
            pCreateInfo->pAttachments[i].finalLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
            skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_00800696, "DL",
                            "pCreateInfo->pAttachments[%d].finalLayout must not be VK_IMAGE_LAYOUT_UNDEFINED or "
                            "VK_IMAGE_LAYOUT_PREINITIALIZED. %s",
                            i, validation_error_map[VALIDATION_ERROR_00800696]);
        }
    }

    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        if (pCreateInfo->pSubpasses[i].colorAttachmentCount > max_color_attachments) {
            skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_1400069a, "DL",
                            "Cannot create a render pass with %d color attachments. Max is %d. %s",
                            pCreateInfo->pSubpasses[i].colorAttachmentCount, max_color_attachments,
                            validation_error_map[VALIDATION_ERROR_1400069a]);
        }
    }
    return skip;
}

bool pv_vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                             const VkCommandBuffer *pCommandBuffers) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    // This is an array of handles, where the elements are allowed to be VK_NULL_HANDLE, and does not require any validation beyond
    // validate_array()
    skip |= validate_array(report_data, "vkFreeCommandBuffers", "commandBufferCount", "pCommandBuffers", commandBufferCount,
                           pCommandBuffers, true, true, VALIDATION_ERROR_UNDEFINED, VALIDATION_ERROR_UNDEFINED);
    return skip;
}

bool pv_vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    debug_report_data *report_data = device_data->report_data;
    const VkCommandBufferInheritanceInfo *pInfo = pBeginInfo->pInheritanceInfo;

    // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
    // TODO: pBeginInfo->pInheritanceInfo must not be NULL if commandBuffer is a secondary command buffer
    skip |= validate_struct_type(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo",
                                 "VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO", pBeginInfo->pInheritanceInfo,
                                 VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, false, VALIDATION_ERROR_UNDEFINED);

    if (pBeginInfo->pInheritanceInfo != NULL) {
        skip |=
            validate_struct_pnext(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo->pNext", NULL,
                                  pBeginInfo->pInheritanceInfo->pNext, 0, NULL, GeneratedHeaderVersion, VALIDATION_ERROR_0281c40d);

        skip |= validate_bool32(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo->occlusionQueryEnable",
                                pBeginInfo->pInheritanceInfo->occlusionQueryEnable);

        // TODO: This only needs to be validated when the inherited queries feature is enabled
        // skip |= validate_flags(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo->queryFlags",
        // "VkQueryControlFlagBits", AllVkQueryControlFlagBits, pBeginInfo->pInheritanceInfo->queryFlags, false);

        // TODO: This must be 0 if the pipeline statistics queries feature is not enabled
        skip |= validate_flags(report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo->pipelineStatistics",
                               "VkQueryPipelineStatisticFlagBits", AllVkQueryPipelineStatisticFlagBits,
                               pBeginInfo->pInheritanceInfo->pipelineStatistics, false, false, VALIDATION_ERROR_UNDEFINED);
    }

    if (pInfo != NULL) {
        if ((device_data->physical_device_features.inheritedQueries == VK_FALSE) && (pInfo->occlusionQueryEnable != VK_FALSE)) {
            skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            HandleToUint64(commandBuffer), __LINE__, VALIDATION_ERROR_02a00070, LayerName,
                            "Cannot set inherited occlusionQueryEnable in vkBeginCommandBuffer() when device does not support "
                            "inheritedQueries. %s",
                            validation_error_map[VALIDATION_ERROR_02a00070]);
        }
        if ((device_data->physical_device_features.inheritedQueries != VK_FALSE) && (pInfo->occlusionQueryEnable != VK_FALSE)) {
            skip |= validate_flags(device_data->report_data, "vkBeginCommandBuffer", "pBeginInfo->pInheritanceInfo->queryFlags",
                                   "VkQueryControlFlagBits", AllVkQueryControlFlagBits, pInfo->queryFlags, false, false,
                                   VALIDATION_ERROR_02a00072);
        }
    }

    return skip;
}

bool pv_vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                         const VkViewport *pViewports) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);

    skip |= validate_array(device_data->report_data, "vkCmdSetViewport", "viewportCount", "pViewports", viewportCount, pViewports,
                           true, true, VALIDATION_ERROR_UNDEFINED, VALIDATION_ERROR_UNDEFINED);

    if (viewportCount > 0 && pViewports != nullptr) {
        const VkPhysicalDeviceLimits &limits = device_data->device_limits;
        for (uint32_t viewportIndex = 0; viewportIndex < viewportCount; ++viewportIndex) {
            const VkViewport &viewport = pViewports[viewportIndex];

            if (device_data->physical_device_features.multiViewport == false) {
                if (viewportCount != 1) {
                    skip |= log_msg(
                        device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, DEVICE_FEATURE, LayerName,
                        "vkCmdSetViewport(): The multiViewport feature is not enabled, so viewportCount must be 1 but is %d.",
                        viewportCount);
                }
                if (firstViewport != 0) {
                    skip |= log_msg(
                        device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, DEVICE_FEATURE, LayerName,
                        "vkCmdSetViewport(): The multiViewport feature is not enabled, so firstViewport must be 0 but is %d.",
                        firstViewport);
                }
            }

            if (viewport.width <= 0 || viewport.width > limits.maxViewportDimensions[0]) {
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                __LINE__, VALIDATION_ERROR_15000996, LayerName,
                                "vkCmdSetViewport %d: width (%f) exceeds permitted bounds (0,%u). %s", viewportIndex,
                                viewport.width, limits.maxViewportDimensions[0], validation_error_map[VALIDATION_ERROR_15000996]);
            }

            if (device_data->extensions.vk_amd_negative_viewport_height || device_data->extensions.vk_khr_maintenance1) {
                // Check lower bound against negative viewport height instead of zero
                if (viewport.height <= -(static_cast<int32_t>(limits.maxViewportDimensions[1])) ||
                    (viewport.height > limits.maxViewportDimensions[1])) {
                    skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, VALIDATION_ERROR_1500099a, LayerName,
                                    "vkCmdSetViewport %d: height (%f) exceeds permitted bounds (-%u,%u). %s", viewportIndex,
                                    viewport.height, limits.maxViewportDimensions[1], limits.maxViewportDimensions[1],
                                    validation_error_map[VALIDATION_ERROR_1500099a]);
                }
            } else {
                if ((viewport.height <= 0) || (viewport.height > limits.maxViewportDimensions[1])) {
                    skip |=
                        log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                __LINE__, VALIDATION_ERROR_15000998, LayerName,
                                "vkCmdSetViewport %d: height (%f) exceeds permitted bounds (0,%u). %s", viewportIndex,
                                viewport.height, limits.maxViewportDimensions[1], validation_error_map[VALIDATION_ERROR_15000998]);
                }
            }

            if (viewport.x < limits.viewportBoundsRange[0] || viewport.x > limits.viewportBoundsRange[1]) {
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                __LINE__, VALIDATION_ERROR_1500099e, LayerName,
                                "vkCmdSetViewport %d: x (%f) exceeds permitted bounds (%f,%f). %s", viewportIndex, viewport.x,
                                limits.viewportBoundsRange[0], limits.viewportBoundsRange[1],
                                validation_error_map[VALIDATION_ERROR_1500099e]);
            }

            if (viewport.y < limits.viewportBoundsRange[0] || viewport.y > limits.viewportBoundsRange[1]) {
                skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                __LINE__, VALIDATION_ERROR_1500099e, LayerName,
                                "vkCmdSetViewport %d: y (%f) exceeds permitted bounds (%f,%f). %s", viewportIndex, viewport.y,
                                limits.viewportBoundsRange[0], limits.viewportBoundsRange[1],
                                validation_error_map[VALIDATION_ERROR_1500099e]);
            }

            if (viewport.x + viewport.width > limits.viewportBoundsRange[1]) {
                skip |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_150009a0, LayerName,
                            "vkCmdSetViewport %d: x (%f) + width (%f) exceeds permitted bound (%f). %s", viewportIndex, viewport.x,
                            viewport.width, limits.viewportBoundsRange[1], validation_error_map[VALIDATION_ERROR_150009a0]);
            }

            if (viewport.y + viewport.height > limits.viewportBoundsRange[1]) {
                skip |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_150009a2, LayerName,
                            "vkCmdSetViewport %d: y (%f) + height (%f) exceeds permitted bound (%f). %s", viewportIndex, viewport.y,
                            viewport.height, limits.viewportBoundsRange[1], validation_error_map[VALIDATION_ERROR_150009a2]);
            }
        }
    }
    return skip;
}

bool pv_vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    if (device_data->physical_device_features.multiViewport == false) {
        if (scissorCount != 1) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            DEVICE_FEATURE, LayerName,
                            "vkCmdSetScissor(): The multiViewport feature is not enabled, so scissorCount must be 1 but is %d.",
                            scissorCount);
        }
        if (firstScissor != 0) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            DEVICE_FEATURE, LayerName,
                            "vkCmdSetScissor(): The multiViewport feature is not enabled, so firstScissor must be 0 but is %d.",
                            firstScissor);
        }
    }

    for (uint32_t scissorIndex = 0; scissorIndex < scissorCount; ++scissorIndex) {
        const VkRect2D &pScissor = pScissors[scissorIndex];

        if (pScissor.offset.x < 0) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_1d8004a6, LayerName, "vkCmdSetScissor %d: offset.x (%d) must not be negative. %s",
                            scissorIndex, pScissor.offset.x, validation_error_map[VALIDATION_ERROR_1d8004a6]);
        } else if (static_cast<int32_t>(pScissor.extent.width) > (INT_MAX - pScissor.offset.x)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_1d8004a8, LayerName,
                            "vkCmdSetScissor %d: adding offset.x (%d) and extent.width (%u) will overflow. %s", scissorIndex,
                            pScissor.offset.x, pScissor.extent.width, validation_error_map[VALIDATION_ERROR_1d8004a8]);
        }

        if (pScissor.offset.y < 0) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_1d8004a6, LayerName, "vkCmdSetScissor %d: offset.y (%d) must not be negative. %s",
                            scissorIndex, pScissor.offset.y, validation_error_map[VALIDATION_ERROR_1d8004a6]);
        } else if (static_cast<int32_t>(pScissor.extent.height) > (INT_MAX - pScissor.offset.y)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            VALIDATION_ERROR_1d8004aa, LayerName,
                            "vkCmdSetScissor %d: adding offset.y (%d) and extent.height (%u) will overflow. %s", scissorIndex,
                            pScissor.offset.y, pScissor.extent.height, validation_error_map[VALIDATION_ERROR_1d8004aa]);
        }
    }
    return skip;
}

bool pv_vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                  uint32_t firstInstance) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    if (vertexCount == 0) {
        // TODO: Verify against Valid Usage section. I don't see a non-zero vertexCount listed, may need to add that and make
        // this an error or leave as is.
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, REQUIRED_PARAMETER, LayerName, "vkCmdDraw parameter, uint32_t vertexCount, is 0");
    }

    if (instanceCount == 0) {
        // TODO: Verify against Valid Usage section. I don't see a non-zero instanceCount listed, may need to add that and make
        // this an error or leave as is.
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, REQUIRED_PARAMETER, LayerName, "vkCmdDraw parameter, uint32_t instanceCount, is 0");
    }
    return skip;
}

bool pv_vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);

    if (!device_data->physical_device_features.multiDrawIndirect && ((count > 1))) {
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, DEVICE_FEATURE, LayerName,
                        "CmdDrawIndirect(): Device feature multiDrawIndirect disabled: count must be 0 or 1 but is %d", count);
    }
    return skip;
}

bool pv_vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count,
                                 uint32_t stride) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    if (!device_data->physical_device_features.multiDrawIndirect && ((count > 1))) {
        skip |=
            log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    DEVICE_FEATURE, LayerName,
                    "CmdDrawIndexedIndirect(): Device feature multiDrawIndirect disabled: count must be 0 or 1 but is %d", count);
    }
    return skip;
}

bool pv_vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                       VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy *pRegions) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);

    if (pRegions != nullptr) {
        if ((pRegions->srcSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            skip |= log_msg(
                device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                VALIDATION_ERROR_0a600c01, LayerName,
                "vkCmdCopyImage() parameter, VkImageAspect pRegions->srcSubresource.aspectMask, is an unrecognized enumerator. %s",
                validation_error_map[VALIDATION_ERROR_0a600c01]);
        }
        if ((pRegions->dstSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            skip |= log_msg(
                device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                VALIDATION_ERROR_0a600c01, LayerName,
                "vkCmdCopyImage() parameter, VkImageAspect pRegions->dstSubresource.aspectMask, is an unrecognized enumerator. %s",
                validation_error_map[VALIDATION_ERROR_0a600c01]);
        }
    }
    return skip;
}

bool pv_vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                       VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);

    if (pRegions != nullptr) {
        if ((pRegions->srcSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            skip |= log_msg(
                device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                UNRECOGNIZED_VALUE, LayerName,
                "vkCmdBlitImage() parameter, VkImageAspect pRegions->srcSubresource.aspectMask, is an unrecognized enumerator");
        }
        if ((pRegions->dstSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            skip |= log_msg(
                device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                UNRECOGNIZED_VALUE, LayerName,
                "vkCmdBlitImage() parameter, VkImageAspect pRegions->dstSubresource.aspectMask, is an unrecognized enumerator");
        }
    }
    return skip;
}

bool pv_vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout,
                               uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);

    if (pRegions != nullptr) {
        if ((pRegions->imageSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                      VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            skip |= log_msg(
                device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                UNRECOGNIZED_VALUE, LayerName,
                "vkCmdCopyBufferToImage() parameter, VkImageAspect pRegions->imageSubresource.aspectMask, is an unrecognized "
                "enumerator");
        }
    }
    return skip;
}

bool pv_vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer,
                               uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);

    if (pRegions != nullptr) {
        if ((pRegions->imageSubresource.aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT |
                                                      VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_METADATA_BIT)) == 0) {
            log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    UNRECOGNIZED_VALUE, LayerName,
                    "vkCmdCopyImageToBuffer parameter, VkImageAspect pRegions->imageSubresource.aspectMask, is an unrecognized "
                    "enumerator");
        }
    }
    return skip;
}

bool pv_vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize,
                          const void *pData) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);

    if (dstOffset & 3) {
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, VALIDATION_ERROR_1e400048, LayerName,
                        "vkCmdUpdateBuffer() parameter, VkDeviceSize dstOffset (0x%" PRIxLEAST64 "), is not a multiple of 4. %s",
                        dstOffset, validation_error_map[VALIDATION_ERROR_1e400048]);
    }

    if ((dataSize <= 0) || (dataSize > 65536)) {
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, VALIDATION_ERROR_1e40004a, LayerName,
                        "vkCmdUpdateBuffer() parameter, VkDeviceSize dataSize (0x%" PRIxLEAST64
                        "), must be greater than zero and less than or equal to 65536. %s",
                        dataSize, validation_error_map[VALIDATION_ERROR_1e40004a]);
    } else if (dataSize & 3) {
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, VALIDATION_ERROR_1e40004c, LayerName,
                        "vkCmdUpdateBuffer() parameter, VkDeviceSize dataSize (0x%" PRIxLEAST64 "), is not a multiple of 4. %s",
                        dataSize, validation_error_map[VALIDATION_ERROR_1e40004c]);
    }
    return skip;
}

bool pv_vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size,
                        uint32_t data) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);

    if (dstOffset & 3) {
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, VALIDATION_ERROR_1b400032, LayerName,
                        "vkCmdFillBuffer() parameter, VkDeviceSize dstOffset (0x%" PRIxLEAST64 "), is not a multiple of 4. %s",
                        dstOffset, validation_error_map[VALIDATION_ERROR_1b400032]);
    }

    if (size != VK_WHOLE_SIZE) {
        if (size <= 0) {
            skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_1b400034, LayerName,
                            "vkCmdFillBuffer() parameter, VkDeviceSize size (0x%" PRIxLEAST64 "), must be greater than zero. %s",
                            size, validation_error_map[VALIDATION_ERROR_1b400034]);
        } else if (size & 3) {
            skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_1b400038, LayerName,
                            "vkCmdFillBuffer() parameter, VkDeviceSize size (0x%" PRIxLEAST64 "), is not a multiple of 4. %s", size,
                            validation_error_map[VALIDATION_ERROR_1b400038]);
        }
    }
    return skip;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                      VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName,
                                                                    uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {
    // Parameter_validation does not have any physical device extensions
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(0, NULL, pPropertyCount, pProperties);

    instance_layer_data *local_data = GetLayerDataPtr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    bool skip =
        validate_array(local_data->report_data, "vkEnumerateDeviceExtensionProperties", "pPropertyCount", "pProperties",
                       pPropertyCount, pProperties, true, false, false, VALIDATION_ERROR_UNDEFINED, VALIDATION_ERROR_2761f401);
    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;

    return local_data->dispatch_table.EnumerateDeviceExtensionProperties(physicalDevice, NULL, pPropertyCount, pProperties);
}

static bool require_device_extension(layer_data *device_data, bool flag, char const *function_name, char const *extension_name) {
    if (!flag) {
        return log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                       __LINE__, EXTENSION_NOT_ENABLED, LayerName,
                       "%s() called even though the %s extension was not enabled for this VkDevice.", function_name,
                       extension_name);
    }

    return false;
}

bool pv_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                             VkSwapchainKHR *pSwapchain) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    debug_report_data *report_data = device_data->report_data;

    if (pCreateInfo != nullptr) {
        if ((device_data->physical_device_features.textureCompressionETC2 == false) &&
            FormatIsCompressed_ETC2_EAC(pCreateInfo->imageFormat)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            DEVICE_FEATURE, LayerName,
                            "vkCreateSwapchainKHR(): Attempting to create swapchain VkImage with format %s. The "
                            "textureCompressionETC2 feature is not enabled: neither ETC2 nor EAC formats can be used to create "
                            "images.",
                            string_VkFormat(pCreateInfo->imageFormat));
        }

        if ((device_data->physical_device_features.textureCompressionASTC_LDR == false) &&
            FormatIsCompressed_ASTC_LDR(pCreateInfo->imageFormat)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            DEVICE_FEATURE, LayerName,
                            "vkCreateSwapchainKHR(): Attempting to create swapchain VkImage with format %s. The "
                            "textureCompressionASTC_LDR feature is not enabled: ASTC formats cannot be used to create images.",
                            string_VkFormat(pCreateInfo->imageFormat));
        }

        if ((device_data->physical_device_features.textureCompressionBC == false) &&
            FormatIsCompressed_BC(pCreateInfo->imageFormat)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                            DEVICE_FEATURE, LayerName,
                            "vkCreateSwapchainKHR(): Attempting to create swapchain VkImage with format %s. The "
                            "textureCompressionBC feature is not enabled: BC compressed formats cannot be used to create images.",
                            string_VkFormat(pCreateInfo->imageFormat));
        }

        // Validation for parameters excluded from the generated validation code due to a 'noautovalidity' tag in vk.xml
        if (pCreateInfo->imageSharingMode == VK_SHARING_MODE_CONCURRENT) {
            // If imageSharingMode is VK_SHARING_MODE_CONCURRENT, queueFamilyIndexCount must be greater than 1
            if (pCreateInfo->queueFamilyIndexCount <= 1) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_146009fc, LayerName,
                                "vkCreateSwapchainKHR(): if pCreateInfo->imageSharingMode is VK_SHARING_MODE_CONCURRENT, "
                                "pCreateInfo->queueFamilyIndexCount must be greater than 1. %s",
                                validation_error_map[VALIDATION_ERROR_146009fc]);
            }

            // If imageSharingMode is VK_SHARING_MODE_CONCURRENT, pQueueFamilyIndices must be a pointer to an array of
            // queueFamilyIndexCount uint32_t values
            if (pCreateInfo->pQueueFamilyIndices == nullptr) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                                VALIDATION_ERROR_146009fa, LayerName,
                                "vkCreateSwapchainKHR(): if pCreateInfo->imageSharingMode is VK_SHARING_MODE_CONCURRENT, "
                                "pCreateInfo->pQueueFamilyIndices must be a pointer to an array of "
                                "pCreateInfo->queueFamilyIndexCount uint32_t values. %s",
                                validation_error_map[VALIDATION_ERROR_146009fa]);
            } else {
                // TODO: Not in the spec VUs. Probably missing -- KhronosGroup/Vulkan-Docs#501. Update error codes when resolved.
                skip |= ValidateQueueFamilies(device_data, pCreateInfo->queueFamilyIndexCount, pCreateInfo->pQueueFamilyIndices,
                                              "vkCreateSwapchainKHR", "pCreateInfo->pQueueFamilyIndices", INVALID_USAGE,
                                              INVALID_USAGE, false, "", "");
            }
        }

        // imageArrayLayers must be greater than 0
        skip |= ValidateGreaterThan(report_data, "vkCreateSwapchainKHR", "pCreateInfo->imageArrayLayers",
                                    pCreateInfo->imageArrayLayers, 0u);
    }

    return skip;
}

bool pv_vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(queue), layer_data_map);

    if (pPresentInfo && pPresentInfo->pNext) {
        // Verify ext struct
        struct std_header {
            VkStructureType sType;
            const void *pNext;
        };
        std_header *pnext = (std_header *)pPresentInfo->pNext;
        while (pnext) {
            if (VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR == pnext->sType) {
                // TODO: This and all other pNext extension dependencies should be added to code-generation
                skip |= require_device_extension(device_data, device_data->extensions.vk_khr_incremental_present,
                                                 "vkQueuePresentKHR", VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME);
                VkPresentRegionsKHR *present_regions = (VkPresentRegionsKHR *)pnext;
                if (present_regions->swapchainCount != pPresentInfo->swapchainCount) {
                    skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, INVALID_USAGE, LayerName,
                                    "QueuePresentKHR(): pPresentInfo->swapchainCount has a value of %i"
                                    " but VkPresentRegionsKHR extension swapchainCount is %i. These values must be equal.",
                                    pPresentInfo->swapchainCount, present_regions->swapchainCount);
                }
                skip |= validate_struct_pnext(device_data->report_data, "QueuePresentKHR", "pCreateInfo->pNext->pNext", NULL,
                                              present_regions->pNext, 0, NULL, GeneratedHeaderVersion, VALIDATION_ERROR_1121c40d);
                skip |= validate_array(device_data->report_data, "QueuePresentKHR", "pCreateInfo->pNext->swapchainCount",
                                       "pCreateInfo->pNext->pRegions", present_regions->swapchainCount, present_regions->pRegions,
                                       true, false, VALIDATION_ERROR_UNDEFINED, VALIDATION_ERROR_UNDEFINED);
                for (uint32_t i = 0; i < present_regions->swapchainCount; ++i) {
                    skip |=
                        validate_array(device_data->report_data, "QueuePresentKHR", "pCreateInfo->pNext->pRegions[].rectangleCount",
                                       "pCreateInfo->pNext->pRegions[].pRectangles", present_regions->pRegions[i].rectangleCount,
                                       present_regions->pRegions[i].pRectangles, true, false, VALIDATION_ERROR_UNDEFINED,
                                       VALIDATION_ERROR_UNDEFINED);
                }
            }
            pnext = (std_header *)pnext->pNext;
        }
    }

    return skip;
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
bool pv_vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo,
                                const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    auto device_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    bool skip = false;

    if (pCreateInfo->hwnd == nullptr) {
        skip |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, VALIDATION_ERROR_15a00a38, LayerName,
                        "vkCreateWin32SurfaceKHR(): hwnd must be a valid Win32 HWND but hwnd is NULL. %s",
                        validation_error_map[VALIDATION_ERROR_15a00a38]);
    }

    return skip;
}
#endif  // VK_USE_PLATFORM_WIN32_KHR

bool pv_vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    auto device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    if (pNameInfo->pObjectName) {
        device_data->report_data->debugObjectNameMap->insert(
            std::make_pair<uint64_t, std::string>((uint64_t &&) pNameInfo->object, pNameInfo->pObjectName));
    } else {
        device_data->report_data->debugObjectNameMap->erase(pNameInfo->object);
    }
    return false;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char *funcName) {
    const auto item = name_to_funcptr_map.find(funcName);
    if (item != name_to_funcptr_map.end()) {
        return reinterpret_cast<PFN_vkVoidFunction>(item->second);
    }

    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    const auto &table = device_data->dispatch_table;
    if (!table.GetDeviceProcAddr) return nullptr;
    return table.GetDeviceProcAddr(device, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    const auto item = name_to_funcptr_map.find(funcName);
    if (item != name_to_funcptr_map.end()) {
        return reinterpret_cast<PFN_vkVoidFunction>(item->second);
    }

    auto instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    auto &table = instance_data->dispatch_table;
    if (!table.GetInstanceProcAddr) return nullptr;
    return table.GetInstanceProcAddr(instance, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetPhysicalDeviceProcAddr(VkInstance instance, const char *funcName) {
    assert(instance);
    auto instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);

    if (!instance_data->dispatch_table.GetPhysicalDeviceProcAddr) return nullptr;
    return instance_data->dispatch_table.GetPhysicalDeviceProcAddr(instance, funcName);
}

// If additional validation is needed outside of the generated checks, a manual routine can be added to this file
// and the address filled in here. The autogenerated source will call these routines if the pointers are not NULL.
void InitializeManualParameterValidationFunctionPointers(void) {
    custom_functions["vkGetDeviceQueue"] = (void*)pv_vkGetDeviceQueue;
    custom_functions["vkCreateBuffer"] = (void*)pv_vkCreateBuffer;
    custom_functions["vkCreateImage"] = (void*)pv_vkCreateImage;
    custom_functions["vkCreateImageView"] = (void*)pv_vkCreateImageView;
    custom_functions["vkCreateGraphicsPipelines"] = (void*)pv_vkCreateGraphicsPipelines;
    custom_functions["vkCreateComputePipelines"] = (void*)pv_vkCreateComputePipelines;
    custom_functions["vkCreateSampler"] = (void*)pv_vkCreateSampler;
    custom_functions["vkCreateDescriptorSetLayout"] = (void*)pv_vkCreateDescriptorSetLayout;
    custom_functions["vkFreeDescriptorSets"] = (void*)pv_vkFreeDescriptorSets;
    custom_functions["vkUpdateDescriptorSets"] = (void*)pv_vkUpdateDescriptorSets;
    custom_functions["vkCreateRenderPass"] = (void*)pv_vkCreateRenderPass;
    custom_functions["vkBeginCommandBuffer"] = (void*)pv_vkBeginCommandBuffer;
    custom_functions["vkCmdSetViewport"] = (void*)pv_vkCmdSetViewport;
    custom_functions["vkCmdSetScissor"] = (void*)pv_vkCmdSetScissor;
    custom_functions["vkCmdDraw"] = (void*)pv_vkCmdDraw;
    custom_functions["vkCmdDrawIndirect"] = (void*)pv_vkCmdDrawIndirect;
    custom_functions["vkCmdDrawIndexedIndirect"] = (void*)pv_vkCmdDrawIndexedIndirect;
    custom_functions["vkCmdCopyImage"] = (void*)pv_vkCmdCopyImage;
    custom_functions["vkCmdBlitImage"] = (void*)pv_vkCmdBlitImage;
    custom_functions["vkCmdCopyBufferToImage"] = (void*)pv_vkCmdCopyBufferToImage;
    custom_functions["vkCmdCopyImageToBuffer"] = (void*)pv_vkCmdCopyImageToBuffer;
    custom_functions["vkCmdUpdateBuffer"] = (void*)pv_vkCmdUpdateBuffer;
    custom_functions["vkCmdFillBuffer"] = (void*)pv_vkCmdFillBuffer;
    custom_functions["vkCreateSwapchainKHR"] = (void*)pv_vkCreateSwapchainKHR;
    custom_functions["vkQueuePresentKHR"] = (void*)pv_vkQueuePresentKHR;
}

}  // namespace parameter_validation

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                                      VkExtensionProperties *pProperties) {
    return parameter_validation::vkEnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,
                                                                                  VkLayerProperties *pProperties) {
    return parameter_validation::vkEnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                                VkLayerProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return parameter_validation::vkEnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return parameter_validation::vkEnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return parameter_validation::vkGetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return parameter_validation::vkGetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_layerGetPhysicalDeviceProcAddr(VkInstance instance,
                                                                                           const char *funcName) {
    return parameter_validation::vkGetPhysicalDeviceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT bool pv_vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct) {
    assert(pVersionStruct != NULL);
    assert(pVersionStruct->sType == LAYER_NEGOTIATE_INTERFACE_STRUCT);

    // Fill in the function pointers if our version is at least capable of having the structure contain them.
    if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
        pVersionStruct->pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
        pVersionStruct->pfnGetDeviceProcAddr = vkGetDeviceProcAddr;
        pVersionStruct->pfnGetPhysicalDeviceProcAddr = vk_layerGetPhysicalDeviceProcAddr;
    }

    if (pVersionStruct->loaderLayerInterfaceVersion < CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        parameter_validation::loader_layer_if_version = pVersionStruct->loaderLayerInterfaceVersion;
    } else if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
    }

    return VK_SUCCESS;
}
