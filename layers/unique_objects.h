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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cinttypes>

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

// All increments must be guarded by global_lock
static uint64_t global_unique_id = 1;

struct layer_data {
    VkInstance instance;

    bool wsi_enabled;
    std::unordered_map<uint64_t, uint64_t> unique_id_mapping; // Map uniqueID to actual object handle
    VkPhysicalDevice gpu;

    layer_data() : wsi_enabled(false), gpu(VK_NULL_HANDLE){};
};

struct instExts {
    bool wsi_enabled;
    bool xlib_enabled;
    bool xcb_enabled;
    bool wayland_enabled;
    bool mir_enabled;
    bool android_enabled;
    bool win32_enabled;
};

static std::unordered_map<void *, struct instExts> instanceExtMap;
static std::unordered_map<void *, layer_data *> layer_data_map;
static device_table_map unique_objects_device_table_map;
static instance_table_map unique_objects_instance_table_map;
static std::mutex global_lock; // Protect map accesses and unique_id increments

// Handle CreateInstance
static void createInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, VkInstance instance) {
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(unique_objects_instance_table_map, instance);
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;

    pDisp->DestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)gpa(instance, "vkDestroySurfaceKHR");
    pDisp->GetPhysicalDeviceSurfaceSupportKHR =
        (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    pDisp->GetPhysicalDeviceSurfaceCapabilitiesKHR =
        (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)gpa(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pDisp->GetPhysicalDeviceSurfaceFormatsKHR =
        (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)gpa(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    pDisp->GetPhysicalDeviceSurfacePresentModesKHR =
        (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)gpa(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
#ifdef VK_USE_PLATFORM_WIN32_KHR
    pDisp->CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)gpa(instance, "vkCreateWin32SurfaceKHR");
    pDisp->GetPhysicalDeviceWin32PresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    pDisp->CreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)gpa(instance, "vkCreateXcbSurfaceKHR");
    pDisp->GetPhysicalDeviceXcbPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    pDisp->CreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)gpa(instance, "vkCreateXlibSurfaceKHR");
    pDisp->GetPhysicalDeviceXlibPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    pDisp->CreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR)gpa(instance, "vkCreateMirSurfaceKHR");
    pDisp->GetPhysicalDeviceMirPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    pDisp->CreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)gpa(instance, "vkCreateWaylandSurfaceKHR");
    pDisp->GetPhysicalDeviceWaylandPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif //  VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    pDisp->CreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)gpa(instance, "vkCreateAndroidSurfaceKHR");
#endif // VK_USE_PLATFORM_ANDROID_KHR

    instanceExtMap[pDisp] = {};
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].wsi_enabled = true;
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].xlib_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].xcb_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].wayland_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].mir_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].android_enabled = true;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].win32_enabled = true;
#endif
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
    initInstanceTable(*pInstance, fpGetInstanceProcAddr, unique_objects_instance_table_map);

    createInstanceRegisterExtensions(pCreateInfo, *pInstance);

    return result;
}

void explicit_DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(instance);
    get_dispatch_table(unique_objects_instance_table_map, instance)->DestroyInstance(instance, pAllocator);
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
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            my_device_data->wsi_enabled = true;
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

    // Setup layer's device dispatch table
    initDeviceTable(*pDevice, fpGetDeviceProcAddr, unique_objects_device_table_map);

    createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    // Set gpu for this device in order to get at any objects mapped at instance level
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    my_device_data->gpu = gpu;

    return result;
}

void explicit_DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(device);
    get_dispatch_table(unique_objects_device_table_map, device)->DestroyDevice(device, pAllocator);
    layer_data_map.erase(key);
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

} // namespace unique_objects
