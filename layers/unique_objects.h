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
 * Author: Tobin Ehlis <tobine@google.com>
 */

#include "vk_loader_platform.h"
#include "vulkan/vulkan.h"

#include <cinttypes>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unordered_map>
#include <vector>
#include <mutex>

#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"
#include "vk_safe_struct.h"
#include "vk_layer_utils.h"

namespace unique_objects {

// The display-server-specific WSI extensions are handled explicitly
static const char *kUniqueObjectsSupportedInstanceExtensions =
#ifdef VK_USE_PLATFORM_XLIB_KHR
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
    VK_KHR_MIR_SURFACE_EXTENSION_NAME
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
    VK_EXT_DEBUG_MARKER_EXTENSION_NAME
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    VK_KHR_DISPLAY_EXTENSION_NAME
    VK_KHR_SURFACE_EXTENSION_NAME;

static const char *kUniqueObjectsSupportedDeviceExtensions =
    VK_AMD_RASTERIZATION_ORDER_EXTENSION_NAME
    VK_AMD_SHADER_TRINARY_MINMAX_EXTENSION_NAME
    VK_AMD_SHADER_EXPLICIT_VERTEX_PARAMETER_EXTENSION_NAME
    VK_AMD_GCN_SHADER_EXTENSION_NAME
    VK_IMG_FILTER_CUBIC_EXTENSION_NAME
    VK_IMG_FORMAT_PVRTC_EXTENSION_NAME
    VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME
    VK_NV_DEDICATED_ALLOCATION_EXTENSION_NAME
    VK_NV_GLSL_SHADER_EXTENSION_NAME;

// All increments must be guarded by global_lock
static uint64_t global_unique_id = 1;

struct layer_data {
    VkInstance instance;

    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;

    // The following are for keeping track of the temporary callbacks that can
    // be used in vkCreateInstance and vkDestroyInstance:
    uint32_t num_tmp_callbacks;
    VkDebugReportCallbackCreateInfoEXT *tmp_dbg_create_infos;
    VkDebugReportCallbackEXT *tmp_callbacks;

    bool wsi_enabled;
    std::unordered_map<uint64_t, uint64_t> unique_id_mapping; // Map uniqueID to actual object handle
    VkPhysicalDevice gpu;

    layer_data() : wsi_enabled(false), gpu(VK_NULL_HANDLE){};
};

struct instance_extension_enables {
    bool wsi_enabled;
    bool xlib_enabled;
    bool xcb_enabled;
    bool wayland_enabled;
    bool mir_enabled;
    bool android_enabled;
    bool win32_enabled;
    bool display_enabled;
};

static std::unordered_map<void *, struct instance_extension_enables> instanceExtMap;
static std::unordered_map<void *, layer_data *> layer_data_map;
static device_table_map unique_objects_device_table_map;
static instance_table_map unique_objects_instance_table_map;
static std::mutex global_lock; // Protect map accesses and unique_id increments

struct GenericHeader {
    VkStructureType sType;
    void *pNext;
};

template <typename T> bool ContainsExtStruct(const T *target, VkStructureType ext_type) {
    assert(target != nullptr);

    const GenericHeader *ext_struct = reinterpret_cast<const GenericHeader *>(target->pNext);

    while (ext_struct != nullptr) {
        if (ext_struct->sType == ext_type) {
            return true;
        }

        ext_struct = reinterpret_cast<const GenericHeader *>(ext_struct->pNext);
    }

    return false;
}

static void init_unique_objects(layer_data *my_data, const VkAllocationCallbacks *pAllocator) {
    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "google_unique_objects");
}

// Handle CreateInstance
static void checkInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, VkInstance instance) {
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(unique_objects_instance_table_map, instance);

    instanceExtMap[pDisp] = {};

    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {

        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].wsi_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].display_enabled = true;
        }
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].xlib_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].xcb_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].wayland_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].mir_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].android_enabled = true;
        }
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0) {
            instanceExtMap[pDisp].win32_enabled = true;
        }
#endif

        // Check for recognized instance extensions
        layer_data *instance_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
        if (!white_list(pCreateInfo->ppEnabledExtensionNames[i], kUniqueObjectsSupportedInstanceExtensions)) {
            log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    0, "UniqueObjects",
                    "Instance Extension %s is not supported by this layer.  Using this extension may adversely affect "
                    "validation results and/or produce undefined behavior.",
                    pCreateInfo->ppEnabledExtensionNames[i]);
        }
    }
}

VkResult explicit_CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                 VkInstance *pInstance) {
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
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    my_data->instance = *pInstance;
    VkLayerInstanceDispatchTable *pTable = initInstanceTable(*pInstance, fpGetInstanceProcAddr, unique_objects_instance_table_map);

    my_data->instance = *pInstance;
    my_data->report_data = debug_report_create_instance(pTable, *pInstance, pCreateInfo->enabledExtensionCount,
        pCreateInfo->ppEnabledExtensionNames);

    // Set up temporary debug callbacks to output messages at CreateInstance-time
    if (!layer_copy_tmp_callbacks(pCreateInfo->pNext, &my_data->num_tmp_callbacks, &my_data->tmp_dbg_create_infos,
                                  &my_data->tmp_callbacks)) {
        if (my_data->num_tmp_callbacks > 0) {
            if (layer_enable_tmp_callbacks(my_data->report_data, my_data->num_tmp_callbacks, my_data->tmp_dbg_create_infos,
                                           my_data->tmp_callbacks)) {
                layer_free_tmp_callbacks(my_data->tmp_dbg_create_infos, my_data->tmp_callbacks);
                my_data->num_tmp_callbacks = 0;
            }
        }
    }

    init_unique_objects(my_data, pAllocator);
    checkInstanceRegisterExtensions(pCreateInfo, *pInstance);

    // Disable and free tmp callbacks, no longer necessary
    if (my_data->num_tmp_callbacks > 0) {
        layer_disable_tmp_callbacks(my_data->report_data, my_data->num_tmp_callbacks, my_data->tmp_callbacks);
        layer_free_tmp_callbacks(my_data->tmp_dbg_create_infos, my_data->tmp_callbacks);
        my_data->num_tmp_callbacks = 0;
    }

    return result;
}

void explicit_DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(unique_objects_instance_table_map, instance);
    instanceExtMap.erase(pDisp);
    pDisp->DestroyInstance(instance, pAllocator);

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    layer_data_map.erase(key);
}

// Handle CreateDevice
static void createDeviceRegisterExtensions(const VkDeviceCreateInfo *pCreateInfo, VkDevice device) {
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp = get_dispatch_table(unique_objects_device_table_map, device);
    PFN_vkGetDeviceProcAddr gpa = pDisp->GetDeviceProcAddr;
    pDisp->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR = (PFN_vkQueuePresentKHR)gpa(device, "vkQueuePresentKHR");
    my_device_data->wsi_enabled = false;

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            my_device_data->wsi_enabled = true;
        }
        // Check for recognized device extensions
        if (!white_list(pCreateInfo->ppEnabledExtensionNames[i], kUniqueObjectsSupportedDeviceExtensions)) {
            log_msg(my_device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                    __LINE__, 0, "UniqueObjects",
                    "Device Extension %s is not supported by this layer.  Using this extension may adversely affect "
                    "validation results and/or produce undefined behavior.",
                    pCreateInfo->ppEnabledExtensionNames[i]);
        }
    }
}

VkResult explicit_CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                               VkDevice *pDevice) {
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(my_instance_data->instance, "vkCreateDevice");
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
    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);

    // Setup layer's device dispatch table
    initDeviceTable(*pDevice, fpGetDeviceProcAddr, unique_objects_device_table_map);

    createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    // Set gpu for this device in order to get at any objects mapped at instance level

    my_device_data->gpu = gpu;

    return result;
}

void explicit_DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(device);
    layer_debug_report_destroy_device(device);
    get_dispatch_table(unique_objects_device_table_map, device)->DestroyDevice(device, pAllocator);
    layer_data_map.erase(key);
}

VkResult explicit_AllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
                                 const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    const VkMemoryAllocateInfo *input_allocate_info = pAllocateInfo;
    std::unique_ptr<safe_VkMemoryAllocateInfo> safe_allocate_info;
    std::unique_ptr<safe_VkDedicatedAllocationMemoryAllocateInfoNV> safe_dedicated_allocate_info;
    layer_data *my_map_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    if ((pAllocateInfo != nullptr) &&
        ContainsExtStruct(pAllocateInfo, VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV)) {
        // Assuming there is only one extension struct of this type in the list for now
        safe_dedicated_allocate_info =
            std::unique_ptr<safe_VkDedicatedAllocationMemoryAllocateInfoNV>(new safe_VkDedicatedAllocationMemoryAllocateInfoNV);
        safe_allocate_info = std::unique_ptr<safe_VkMemoryAllocateInfo>(new safe_VkMemoryAllocateInfo);

        safe_allocate_info->initialize(pAllocateInfo);
        input_allocate_info = reinterpret_cast<const VkMemoryAllocateInfo *>(safe_allocate_info.get());

        const GenericHeader *orig_pnext = reinterpret_cast<const GenericHeader *>(pAllocateInfo->pNext);
        GenericHeader *input_pnext = reinterpret_cast<GenericHeader *>(safe_allocate_info.get());
        while (orig_pnext != nullptr) {
            if (orig_pnext->sType == VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV) {
                safe_dedicated_allocate_info->initialize(
                    reinterpret_cast<const VkDedicatedAllocationMemoryAllocateInfoNV *>(orig_pnext));

                std::unique_lock<std::mutex> lock(global_lock);

                if (safe_dedicated_allocate_info->buffer != VK_NULL_HANDLE) {
                    uint64_t local_buffer = reinterpret_cast<uint64_t &>(safe_dedicated_allocate_info->buffer);
                    safe_dedicated_allocate_info->buffer =
                        reinterpret_cast<VkBuffer &>(my_map_data->unique_id_mapping[local_buffer]);
                }

                if (safe_dedicated_allocate_info->image != VK_NULL_HANDLE) {
                    uint64_t local_image = reinterpret_cast<uint64_t &>(safe_dedicated_allocate_info->image);
                    safe_dedicated_allocate_info->image = reinterpret_cast<VkImage &>(my_map_data->unique_id_mapping[local_image]);
                }

                lock.unlock();

                input_pnext->pNext = reinterpret_cast<GenericHeader *>(safe_dedicated_allocate_info.get());
                input_pnext = reinterpret_cast<GenericHeader *>(input_pnext->pNext);
            } else {
                // TODO: generic handling of pNext copies
            }

            orig_pnext = reinterpret_cast<const GenericHeader *>(orig_pnext->pNext);
        }
    }

    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)
                          ->AllocateMemory(device, input_allocate_info, pAllocator, pMemory);

    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        uint64_t unique_id = global_unique_id++;
        my_map_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(*pMemory);
        *pMemory = reinterpret_cast<VkDeviceMemory &>(unique_id);
    }

    return result;
}

VkResult explicit_CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                         const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                                         VkPipeline *pPipelines) {
    // STRUCT USES:{'pipelineCache': 'VkPipelineCache', 'pCreateInfos[createInfoCount]': {'stage': {'module': 'VkShaderModule'},
    // 'layout': 'VkPipelineLayout', 'basePipelineHandle': 'VkPipeline'}}
    // LOCAL DECLS:{'pCreateInfos': 'VkComputePipelineCreateInfo*'}
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    safe_VkComputePipelineCreateInfo *local_pCreateInfos = NULL;
    if (pCreateInfos) {
        std::lock_guard<std::mutex> lock(global_lock);
        local_pCreateInfos = new safe_VkComputePipelineCreateInfo[createInfoCount];
        for (uint32_t idx0 = 0; idx0 < createInfoCount; ++idx0) {
            local_pCreateInfos[idx0].initialize(&pCreateInfos[idx0]);
            if (pCreateInfos[idx0].basePipelineHandle) {
                local_pCreateInfos[idx0].basePipelineHandle =
                    (VkPipeline)my_device_data
                        ->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].basePipelineHandle)];
            }
            if (pCreateInfos[idx0].layout) {
                local_pCreateInfos[idx0].layout =
                    (VkPipelineLayout)
                        my_device_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].layout)];
            }
            if (pCreateInfos[idx0].stage.module) {
                local_pCreateInfos[idx0].stage.module =
                    (VkShaderModule)
                        my_device_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].stage.module)];
            }
        }
    }
    if (pipelineCache) {
        std::lock_guard<std::mutex> lock(global_lock);
        pipelineCache = (VkPipelineCache)my_device_data->unique_id_mapping[reinterpret_cast<uint64_t &>(pipelineCache)];
    }

    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)
                          ->CreateComputePipelines(device, pipelineCache, createInfoCount,
                                                   (const VkComputePipelineCreateInfo *)local_pCreateInfos, pAllocator, pPipelines);
    delete[] local_pCreateInfos;
    if (VK_SUCCESS == result) {
        uint64_t unique_id = 0;
        std::lock_guard<std::mutex> lock(global_lock);
        for (uint32_t i = 0; i < createInfoCount; ++i) {
            unique_id = global_unique_id++;
            my_device_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(pPipelines[i]);
            pPipelines[i] = reinterpret_cast<VkPipeline &>(unique_id);
        }
    }
    return result;
}

VkResult explicit_CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                          const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                                          VkPipeline *pPipelines) {
    // STRUCT USES:{'pipelineCache': 'VkPipelineCache', 'pCreateInfos[createInfoCount]': {'layout': 'VkPipelineLayout',
    // 'pStages[stageCount]': {'module': 'VkShaderModule'}, 'renderPass': 'VkRenderPass', 'basePipelineHandle': 'VkPipeline'}}
    // LOCAL DECLS:{'pCreateInfos': 'VkGraphicsPipelineCreateInfo*'}
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    safe_VkGraphicsPipelineCreateInfo *local_pCreateInfos = NULL;
    if (pCreateInfos) {
        local_pCreateInfos = new safe_VkGraphicsPipelineCreateInfo[createInfoCount];
        std::lock_guard<std::mutex> lock(global_lock);
        for (uint32_t idx0 = 0; idx0 < createInfoCount; ++idx0) {
            local_pCreateInfos[idx0].initialize(&pCreateInfos[idx0]);
            if (pCreateInfos[idx0].basePipelineHandle) {
                local_pCreateInfos[idx0].basePipelineHandle =
                    (VkPipeline)my_device_data
                        ->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].basePipelineHandle)];
            }
            if (pCreateInfos[idx0].layout) {
                local_pCreateInfos[idx0].layout =
                    (VkPipelineLayout)
                        my_device_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].layout)];
            }
            if (pCreateInfos[idx0].pStages) {
                for (uint32_t idx1 = 0; idx1 < pCreateInfos[idx0].stageCount; ++idx1) {
                    if (pCreateInfos[idx0].pStages[idx1].module) {
                        local_pCreateInfos[idx0].pStages[idx1].module =
                            (VkShaderModule)my_device_data
                                ->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].pStages[idx1].module)];
                    }
                }
            }
            if (pCreateInfos[idx0].renderPass) {
                local_pCreateInfos[idx0].renderPass =
                    (VkRenderPass)
                        my_device_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfos[idx0].renderPass)];
            }
        }
    }
    if (pipelineCache) {
        std::lock_guard<std::mutex> lock(global_lock);
        pipelineCache = (VkPipelineCache)my_device_data->unique_id_mapping[reinterpret_cast<uint64_t &>(pipelineCache)];
    }

    VkResult result =
        get_dispatch_table(unique_objects_device_table_map, device)
            ->CreateGraphicsPipelines(device, pipelineCache, createInfoCount,
                                      (const VkGraphicsPipelineCreateInfo *)local_pCreateInfos, pAllocator, pPipelines);
    delete[] local_pCreateInfos;
    if (VK_SUCCESS == result) {
        uint64_t unique_id = 0;
        std::lock_guard<std::mutex> lock(global_lock);
        for (uint32_t i = 0; i < createInfoCount; ++i) {
            unique_id = global_unique_id++;
            my_device_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(pPipelines[i]);
            pPipelines[i] = reinterpret_cast<VkPipeline &>(unique_id);
        }
    }
    return result;
}

VkResult explicit_CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo,
                                     const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    layer_data *my_map_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    safe_VkSwapchainCreateInfoKHR *local_pCreateInfo = NULL;
    if (pCreateInfo) {
        std::lock_guard<std::mutex> lock(global_lock);
        local_pCreateInfo = new safe_VkSwapchainCreateInfoKHR(pCreateInfo);
        local_pCreateInfo->oldSwapchain =
            (VkSwapchainKHR)my_map_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfo->oldSwapchain)];
        // Need to pull surface mapping from the instance-level map
        layer_data *instance_data = get_my_data_ptr(get_dispatch_key(my_map_data->gpu), layer_data_map);
        local_pCreateInfo->surface =
            (VkSurfaceKHR)instance_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pCreateInfo->surface)];
    }

    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)
                          ->CreateSwapchainKHR(device, (const VkSwapchainCreateInfoKHR *)local_pCreateInfo, pAllocator, pSwapchain);
    if (local_pCreateInfo)
        delete local_pCreateInfo;
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        uint64_t unique_id =global_unique_id++;
        my_map_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(*pSwapchain);
        *pSwapchain = reinterpret_cast<VkSwapchainKHR &>(unique_id);
    }
    return result;
}

VkResult explicit_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount,
                                        VkImage *pSwapchainImages) {
    // UNWRAP USES:
    //  0 : swapchain,VkSwapchainKHR, pSwapchainImages,VkImage
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    if (VK_NULL_HANDLE != swapchain) {
        std::lock_guard<std::mutex> lock(global_lock);
        swapchain = (VkSwapchainKHR)my_device_data->unique_id_mapping[reinterpret_cast<uint64_t &>(swapchain)];
    }
    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)
                          ->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    // TODO : Need to add corresponding code to delete these images
    if (VK_SUCCESS == result) {
        if ((*pSwapchainImageCount > 0) && pSwapchainImages) {
            uint64_t unique_id = 0;
            std::lock_guard<std::mutex> lock(global_lock);
            for (uint32_t i = 0; i < *pSwapchainImageCount; ++i) {
                unique_id = global_unique_id++;
                my_device_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(pSwapchainImages[i]);
                pSwapchainImages[i] = reinterpret_cast<VkImage &>(unique_id);
            }
        }
    }
    return result;
}

 #ifndef __ANDROID__
VkResult explicit_GetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkDisplayPropertiesKHR* pProperties)
{
    layer_data *my_map_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    safe_VkDisplayPropertiesKHR* local_pProperties = NULL;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        if (pProperties) {
            local_pProperties = new safe_VkDisplayPropertiesKHR[*pPropertyCount];
            for (uint32_t idx0=0; idx0<*pPropertyCount; ++idx0) {
                local_pProperties[idx0].initialize(&pProperties[idx0]);
                if (pProperties[idx0].display) {
                    local_pProperties[idx0].display = (VkDisplayKHR)my_map_data->unique_id_mapping[reinterpret_cast<const uint64_t &>(pProperties[idx0].display)];
                }
            }
        }
    }

    VkResult result = get_dispatch_table(unique_objects_instance_table_map, physicalDevice)->GetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, pPropertyCount, ( VkDisplayPropertiesKHR*)local_pProperties);
    if (result == VK_SUCCESS && pProperties)
    {
        for (uint32_t idx0=0; idx0<*pPropertyCount; ++idx0) {
            std::lock_guard<std::mutex> lock(global_lock);

            uint64_t unique_id = global_unique_id++;
            my_map_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(local_pProperties[idx0].display);
            pProperties[idx0].display = reinterpret_cast<VkDisplayKHR&>(unique_id);
            pProperties[idx0].displayName = local_pProperties[idx0].displayName;
            pProperties[idx0].physicalDimensions = local_pProperties[idx0].physicalDimensions;
            pProperties[idx0].physicalResolution = local_pProperties[idx0].physicalResolution;
            pProperties[idx0].supportedTransforms = local_pProperties[idx0].supportedTransforms;
            pProperties[idx0].planeReorderPossible = local_pProperties[idx0].planeReorderPossible;
            pProperties[idx0].persistentContent = local_pProperties[idx0].persistentContent;
        }
    }
    if (local_pProperties)
        delete[] local_pProperties;
    return result;
}

VkResult explicit_GetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice, uint32_t planeIndex, uint32_t* pDisplayCount, VkDisplayKHR* pDisplays)
{
    layer_data *my_map_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    VkResult result = get_dispatch_table(unique_objects_instance_table_map, physicalDevice)->GetDisplayPlaneSupportedDisplaysKHR(physicalDevice, planeIndex, pDisplayCount, pDisplays);
    if (VK_SUCCESS == result) {
        if ((*pDisplayCount > 0) && pDisplays) {
            std::lock_guard<std::mutex> lock(global_lock);
            for (uint32_t i = 0; i < *pDisplayCount; i++) {
		    auto it = my_map_data->unique_id_mapping.find(reinterpret_cast<const uint64_t &> (pDisplays[i]));
                assert (it !=  my_map_data->unique_id_mapping.end());
                pDisplays[i] = reinterpret_cast<VkDisplayKHR&> (it->second);
            }
        }
    }
    return result;
}


VkResult explicit_GetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t* pPropertyCount, VkDisplayModePropertiesKHR* pProperties)
{
    layer_data *my_map_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    safe_VkDisplayModePropertiesKHR* local_pProperties = NULL;
    {
        std::lock_guard<std::mutex> lock(global_lock);
        display = (VkDisplayKHR)my_map_data->unique_id_mapping[reinterpret_cast<uint64_t &>(display)];
        if (pProperties) {
            local_pProperties = new safe_VkDisplayModePropertiesKHR[*pPropertyCount];
            for (uint32_t idx0=0; idx0<*pPropertyCount; ++idx0) {
                local_pProperties[idx0].initialize(&pProperties[idx0]);
            }
        }
    }

    VkResult result = get_dispatch_table(unique_objects_instance_table_map, physicalDevice)->GetDisplayModePropertiesKHR(physicalDevice, display, pPropertyCount, ( VkDisplayModePropertiesKHR*)local_pProperties);
    if (result == VK_SUCCESS && pProperties)
    {
        for (uint32_t idx0=0; idx0<*pPropertyCount; ++idx0) {
            std::lock_guard<std::mutex> lock(global_lock);

            uint64_t unique_id = global_unique_id++;
            my_map_data->unique_id_mapping[unique_id] = reinterpret_cast<uint64_t &>(local_pProperties[idx0].displayMode);
            pProperties[idx0].displayMode = reinterpret_cast<VkDisplayModeKHR&>(unique_id);
            pProperties[idx0].parameters.visibleRegion.width = local_pProperties[idx0].parameters.visibleRegion.width;
            pProperties[idx0].parameters.visibleRegion.height = local_pProperties[idx0].parameters.visibleRegion.height;
            pProperties[idx0].parameters.refreshRate = local_pProperties[idx0].parameters.refreshRate;
        }
    }
    if (local_pProperties)
        delete[] local_pProperties;
    return result;
}
#endif
} // namespace unique_objects
