/*
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015 Google Inc.
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
 * Author: Courtney Goeltzenleuchter <courtney@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Tony Barbour <tony@LunarG.com>
 * Author: Chia-I Wu <olv@lunarg.com>
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "vk_loader_platform.h"
#include "loader.h"
#include "debug_report.h"
#include "wsi.h"
#include "vk_loader_extensions.h"
#include "gpa_helper.h"

// Trampoline entrypoints are in this file for core Vulkan commands

// Get an instance level or global level entry point address.
// @param instance
// @param pName
// @return
//    If instance == NULL returns a global level functions only
//    If instance is valid returns a trampoline entry point for all dispatchable Vulkan
//    functions both core and extensions.
LOADER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *pName) {
    void *addr;

    addr = globalGetProcAddr(pName);
    if (instance == VK_NULL_HANDLE) {
        // Get entrypoint addresses that are global (no dispatchable object)

        return addr;
    } else {
        // If a global entrypoint return NULL
        if (addr) return NULL;
    }

    struct loader_instance *ptr_instance = loader_get_instance(instance);
    if (ptr_instance == NULL) return NULL;
    // Return trampoline code for non-global entrypoints including any extensions.
    // Device extensions are returned if a layer or ICD supports the extension.
    // Instance extensions are returned if the extension is enabled and the
    // loader or someone else supports the extension
    return trampolineGetProcAddr(ptr_instance, pName);
}

// Get a device level or global level entry point address.
// @param device
// @param pName
// @return
//    If device is valid, returns a device relative entry point for device level
//    entry points both core and extensions.
//    Device relative means call down the device chain.
LOADER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice device, const char *pName) {
    void *addr;

    // For entrypoints that loader must handle (ie non-dispatchable or create object)
    // make sure the loader entrypoint is returned
    addr = loader_non_passthrough_gdpa(pName);
    if (addr) {
        return addr;
    }

    // Although CreateDevice is on device chain it's dispatchable object isn't
    // a VkDevice or child of VkDevice so return NULL.
    if (!strcmp(pName, "CreateDevice")) return NULL;

    // Return the dispatch table entrypoint for the fastest case
    const VkLayerDispatchTable *disp_table = *(VkLayerDispatchTable **)device;
    if (disp_table == NULL) return NULL;

    addr = loader_lookup_device_dispatch_table(disp_table, pName);
    if (addr) return addr;

    if (disp_table->GetDeviceProcAddr == NULL) return NULL;
    return disp_table->GetDeviceProcAddr(device, pName);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName,
                                                                                    uint32_t *pPropertyCount,
                                                                                    VkExtensionProperties *pProperties) {
    struct loader_extension_list *global_ext_list = NULL;
    struct loader_layer_list instance_layers;
    struct loader_extension_list local_ext_list;
    struct loader_icd_tramp_list icd_tramp_list;
    uint32_t copy_size;
    VkResult res = VK_SUCCESS;

    tls_instance = NULL;
    memset(&local_ext_list, 0, sizeof(local_ext_list));
    memset(&instance_layers, 0, sizeof(instance_layers));
    loader_platform_thread_once(&once_init, loader_initialize);

    // Get layer libraries if needed
    if (pLayerName && strlen(pLayerName) != 0) {
        if (vk_string_validate(MaxLoaderStringLength, pLayerName) != VK_STRING_ERROR_NONE) {
            assert(VK_FALSE &&
                   "vkEnumerateInstanceExtensionProperties:  "
                   "pLayerName is too long or is badly formed");
            res = VK_ERROR_EXTENSION_NOT_PRESENT;
            goto out;
        }

        loader_layer_scan(NULL, &instance_layers);
        for (uint32_t i = 0; i < instance_layers.count; i++) {
            struct loader_layer_properties *props = &instance_layers.list[i];
            if (strcmp(props->info.layerName, pLayerName) == 0) {
                global_ext_list = &props->instance_extension_list;
                break;
            }
        }
    } else {
        // Scan/discover all ICD libraries
        memset(&icd_tramp_list, 0, sizeof(struct loader_icd_tramp_list));
        res = loader_icd_scan(NULL, &icd_tramp_list);
        if (VK_SUCCESS != res) {
            goto out;
        }
        // Get extensions from all ICD's, merge so no duplicates
        res = loader_get_icd_loader_instance_extensions(NULL, &icd_tramp_list, &local_ext_list);
        if (VK_SUCCESS != res) {
            goto out;
        }
        loader_scanned_icd_clear(NULL, &icd_tramp_list);

        // Append implicit layers.
        loader_implicit_layer_scan(NULL, &instance_layers);
        for (uint32_t i = 0; i < instance_layers.count; i++) {
            struct loader_extension_list *ext_list = &instance_layers.list[i].instance_extension_list;
            loader_add_to_ext_list(NULL, &local_ext_list, ext_list->count, ext_list->list);
        }

        global_ext_list = &local_ext_list;
    }

    if (global_ext_list == NULL) {
        res = VK_ERROR_LAYER_NOT_PRESENT;
        goto out;
    }

    if (pProperties == NULL) {
        *pPropertyCount = global_ext_list->count;
        goto out;
    }

    copy_size = *pPropertyCount < global_ext_list->count ? *pPropertyCount : global_ext_list->count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i], &global_ext_list->list[i], sizeof(VkExtensionProperties));
    }
    *pPropertyCount = copy_size;

    if (copy_size < global_ext_list->count) {
        res = VK_INCOMPLETE;
        goto out;
    }

out:

    loader_destroy_generic_list(NULL, (struct loader_generic_list *)&local_ext_list);
    loader_delete_layer_properties(NULL, &instance_layers);
    return res;
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
                                                                                VkLayerProperties *pProperties) {
    VkResult result = VK_SUCCESS;
    struct loader_layer_list instance_layer_list;
    tls_instance = NULL;

    loader_platform_thread_once(&once_init, loader_initialize);

    uint32_t copy_size;

    // Get layer libraries
    memset(&instance_layer_list, 0, sizeof(instance_layer_list));
    loader_layer_scan(NULL, &instance_layer_list);

    if (pProperties == NULL) {
        *pPropertyCount = instance_layer_list.count;
        goto out;
    }

    copy_size = (*pPropertyCount < instance_layer_list.count) ? *pPropertyCount : instance_layer_list.count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i], &instance_layer_list.list[i].info, sizeof(VkLayerProperties));
    }

    *pPropertyCount = copy_size;

    if (copy_size < instance_layer_list.count) {
        result = VK_INCOMPLETE;
        goto out;
    }

out:

    loader_delete_layer_properties(NULL, &instance_layer_list);
    return result;
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    struct loader_instance *ptr_instance = NULL;
    VkInstance created_instance = VK_NULL_HANDLE;
    bool loaderLocked = false;
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    loader_platform_thread_once(&once_init, loader_initialize);

    // Fail if the requested Vulkan apiVersion is > 1.0 since the loader only supports 1.0.
    // Having pCreateInfo == NULL, pCreateInfo->pApplication == NULL, or
    // pCreateInfo->pApplicationInfo->apiVersion == 0 all indicate that the application is
    // only requesting a 1.0 instance, which this loader will always support.
    uint32_t loader_major_version = 1;
    uint32_t loader_minor_version = 0;
    if (NULL != pCreateInfo && NULL != pCreateInfo->pApplicationInfo &&
        pCreateInfo->pApplicationInfo->apiVersion >= VK_MAKE_VERSION(loader_major_version, loader_minor_version + 1, 0)) {
        loader_log(ptr_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "vkCreateInstance:  Called with invalid API version %d.%d.  Loader only supports %d.%d",
                   VK_VERSION_MAJOR(pCreateInfo->pApplicationInfo->apiVersion),
                   VK_VERSION_MINOR(pCreateInfo->pApplicationInfo->apiVersion), loader_major_version, loader_minor_version);
        res = VK_ERROR_INCOMPATIBLE_DRIVER;
        goto out;
    }

#if (DEBUG_DISABLE_APP_ALLOCATORS == 1)
    {
#else
    if (pAllocator) {
        ptr_instance = (struct loader_instance *)pAllocator->pfnAllocation(pAllocator->pUserData, sizeof(struct loader_instance),
                                                                           sizeof(int *), VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
    } else {
#endif
        ptr_instance = (struct loader_instance *)malloc(sizeof(struct loader_instance));
    }

    VkInstanceCreateInfo ici = *pCreateInfo;

    if (ptr_instance == NULL) {
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto out;
    }

    tls_instance = ptr_instance;
    loader_platform_thread_lock_mutex(&loader_lock);
    loaderLocked = true;
    memset(ptr_instance, 0, sizeof(struct loader_instance));
    if (pAllocator) {
        ptr_instance->alloc_callbacks = *pAllocator;
    }

    // Look for one or more debug report create info structures
    // and setup a callback(s) for each one found.
    ptr_instance->num_tmp_callbacks = 0;
    ptr_instance->tmp_dbg_create_infos = NULL;
    ptr_instance->tmp_callbacks = NULL;
    if (util_CopyDebugReportCreateInfos(pCreateInfo->pNext, pAllocator, &ptr_instance->num_tmp_callbacks,
                                        &ptr_instance->tmp_dbg_create_infos, &ptr_instance->tmp_callbacks)) {
        // One or more were found, but allocation failed.  Therefore, clean up
        // and fail this function:
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto out;
    } else if (ptr_instance->num_tmp_callbacks > 0) {
        // Setup the temporary callback(s) here to catch early issues:
        if (util_CreateDebugReportCallbacks(ptr_instance, pAllocator, ptr_instance->num_tmp_callbacks,
                                            ptr_instance->tmp_dbg_create_infos, ptr_instance->tmp_callbacks)) {
            // Failure of setting up one or more of the callback.  Therefore,
            // clean up and fail this function:
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            goto out;
        }
    }

    // Due to implicit layers need to get layer list even if
    // enabledLayerCount == 0 and VK_INSTANCE_LAYERS is unset. For now always
    // get layer list via loader_layer_scan().
    memset(&ptr_instance->instance_layer_list, 0, sizeof(ptr_instance->instance_layer_list));
    loader_layer_scan(ptr_instance, &ptr_instance->instance_layer_list);

    // Validate the app requested layers to be enabled
    if (pCreateInfo->enabledLayerCount > 0) {
        res = loader_validate_layers(ptr_instance, pCreateInfo->enabledLayerCount, pCreateInfo->ppEnabledLayerNames,
                                     &ptr_instance->instance_layer_list);
        if (res != VK_SUCCESS) {
            goto out;
        }
    }

    // Scan/discover all ICD libraries
    memset(&ptr_instance->icd_tramp_list, 0, sizeof(ptr_instance->icd_tramp_list));
    res = loader_icd_scan(ptr_instance, &ptr_instance->icd_tramp_list);
    if (res != VK_SUCCESS) {
        goto out;
    }

    // Get extensions from all ICD's, merge so no duplicates, then validate
    res = loader_get_icd_loader_instance_extensions(ptr_instance, &ptr_instance->icd_tramp_list, &ptr_instance->ext_list);
    if (res != VK_SUCCESS) {
        goto out;
    }
    res = loader_validate_instance_extensions(ptr_instance, &ptr_instance->ext_list, &ptr_instance->instance_layer_list, &ici);
    if (res != VK_SUCCESS) {
        goto out;
    }

    ptr_instance->disp = loader_instance_heap_alloc(ptr_instance, sizeof(struct loader_instance_dispatch_table),
                                                    VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
    if (ptr_instance->disp == NULL) {
        loader_log(ptr_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "vkCreateInstance:  Failed to allocate Loader's full Instance dispatch table.");
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto out;
    }
    memcpy(&ptr_instance->disp->layer_inst_disp, &instance_disp, sizeof(instance_disp));

    ptr_instance->next = loader.instances;
    loader.instances = ptr_instance;

    // Activate any layers on instance chain
    res = loader_enable_instance_layers(ptr_instance, &ici, &ptr_instance->instance_layer_list);
    if (res != VK_SUCCESS) {
        goto out;
    }

    created_instance = (VkInstance)ptr_instance;
    res = loader_create_instance_chain(&ici, pAllocator, ptr_instance, &created_instance);

    if (res == VK_SUCCESS) {
        memset(ptr_instance->enabled_known_extensions.padding, 0, sizeof(uint64_t) * 4);

        wsi_create_instance(ptr_instance, &ici);
        debug_report_create_instance(ptr_instance, &ici);
        extensions_create_instance(ptr_instance, &ici);

        *pInstance = created_instance;

        // Finally have the layers in place and everyone has seen
        // the CreateInstance command go by. This allows the layer's
        // GetInstanceProcAddr functions to return valid extension functions
        // if enabled.
        loader_activate_instance_layer_extensions(ptr_instance, *pInstance);
    }

out:

    if (NULL != ptr_instance) {
        if (res != VK_SUCCESS) {
            if (NULL != ptr_instance->next) {
                loader.instances = ptr_instance->next;
            }
            if (NULL != ptr_instance->disp) {
                loader_instance_heap_free(ptr_instance, ptr_instance->disp);
            }
            if (ptr_instance->num_tmp_callbacks > 0) {
                util_DestroyDebugReportCallbacks(ptr_instance, pAllocator, ptr_instance->num_tmp_callbacks,
                                                 ptr_instance->tmp_callbacks);
                util_FreeDebugReportCreateInfos(pAllocator, ptr_instance->tmp_dbg_create_infos, ptr_instance->tmp_callbacks);
            }

            if (NULL != ptr_instance->expanded_activated_layer_list.list) {
                loader_deactivate_layers(ptr_instance, NULL, &ptr_instance->expanded_activated_layer_list);
            }
            if (NULL != ptr_instance->app_activated_layer_list.list) {
                loader_destroy_layer_list(ptr_instance, NULL, &ptr_instance->app_activated_layer_list);
            }

            loader_delete_layer_properties(ptr_instance, &ptr_instance->instance_layer_list);
            loader_scanned_icd_clear(ptr_instance, &ptr_instance->icd_tramp_list);
            loader_destroy_generic_list(ptr_instance, (struct loader_generic_list *)&ptr_instance->ext_list);

            loader_instance_heap_free(ptr_instance, ptr_instance);
        } else {
            // Remove temporary debug_report callback
            util_DestroyDebugReportCallbacks(ptr_instance, pAllocator, ptr_instance->num_tmp_callbacks,
                                             ptr_instance->tmp_callbacks);
        }

        if (loaderLocked) {
            loader_platform_thread_unlock_mutex(&loader_lock);
        }
    }

    return res;
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    const VkLayerInstanceDispatchTable *disp;
    struct loader_instance *ptr_instance = NULL;
    bool callback_setup = false;

    if (instance == VK_NULL_HANDLE) {
        return;
    }

    disp = loader_get_instance_layer_dispatch(instance);

    loader_platform_thread_lock_mutex(&loader_lock);

    ptr_instance = loader_get_instance(instance);

    if (pAllocator) {
        ptr_instance->alloc_callbacks = *pAllocator;
    }

    if (ptr_instance->num_tmp_callbacks > 0) {
        // Setup the temporary callback(s) here to catch cleanup issues:
        if (!util_CreateDebugReportCallbacks(ptr_instance, pAllocator, ptr_instance->num_tmp_callbacks,
                                             ptr_instance->tmp_dbg_create_infos, ptr_instance->tmp_callbacks)) {
            callback_setup = true;
        }
    }

    disp->DestroyInstance(instance, pAllocator);

    if (NULL != ptr_instance->expanded_activated_layer_list.list) {
        loader_deactivate_layers(ptr_instance, NULL, &ptr_instance->expanded_activated_layer_list);
    }
    if (NULL != ptr_instance->app_activated_layer_list.list) {
        loader_destroy_layer_list(ptr_instance, NULL, &ptr_instance->app_activated_layer_list);
    }

    if (ptr_instance->phys_devs_tramp) {
        for (uint32_t i = 0; i < ptr_instance->phys_dev_count_tramp; i++) {
            loader_instance_heap_free(ptr_instance, ptr_instance->phys_devs_tramp[i]);
        }
        loader_instance_heap_free(ptr_instance, ptr_instance->phys_devs_tramp);
    }

    if (ptr_instance->phys_dev_groups_tramp) {
        for (uint32_t i = 0; i < ptr_instance->phys_dev_group_count_tramp; i++) {
            loader_instance_heap_free(ptr_instance, ptr_instance->phys_dev_groups_tramp[i]);
        }
        loader_instance_heap_free(ptr_instance, ptr_instance->phys_dev_groups_tramp);
    }

    if (callback_setup) {
        util_DestroyDebugReportCallbacks(ptr_instance, pAllocator, ptr_instance->num_tmp_callbacks, ptr_instance->tmp_callbacks);
        util_FreeDebugReportCreateInfos(pAllocator, ptr_instance->tmp_dbg_create_infos, ptr_instance->tmp_callbacks);
    }
    loader_instance_heap_free(ptr_instance, ptr_instance->disp);
    loader_instance_heap_free(ptr_instance, ptr_instance);
    loader_platform_thread_unlock_mutex(&loader_lock);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                                        VkPhysicalDevice *pPhysicalDevices) {
    VkResult res = VK_SUCCESS;
    uint32_t count;
    uint32_t i;
    struct loader_instance *inst;

    loader_platform_thread_lock_mutex(&loader_lock);

    inst = loader_get_instance(instance);
    if (NULL == inst) {
        res = VK_ERROR_INITIALIZATION_FAILED;
        goto out;
    }

    if (NULL == pPhysicalDeviceCount) {
        loader_log(inst, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "vkEnumeratePhysicalDevices: Received NULL pointer for physical device count return value.");
        res = VK_ERROR_INITIALIZATION_FAILED;
        goto out;
    }

    // Setup the trampoline loader physical devices.  This will actually
    // call down and setup the terminator loader physical devices during the
    // process.
    VkResult setup_res = setupLoaderTrampPhysDevs(instance);
    if (setup_res != VK_SUCCESS && setup_res != VK_INCOMPLETE) {
        res = setup_res;
        goto out;
    }

    count = inst->phys_dev_count_tramp;

    // Wrap the PhysDev object for loader usage, return wrapped objects
    if (NULL != pPhysicalDevices) {
        if (inst->phys_dev_count_tramp > *pPhysicalDeviceCount) {
            loader_log(inst, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, 0,
                       "vkEnumeratePhysicalDevices: Trimming device count down"
                       " by application request from %d to %d physical devices",
                       inst->phys_dev_count_tramp, *pPhysicalDeviceCount);
            count = *pPhysicalDeviceCount;
            res = VK_INCOMPLETE;
        }
        for (i = 0; i < count; i++) {
            pPhysicalDevices[i] = (VkPhysicalDevice)inst->phys_devs_tramp[i];
        }
    }

    *pPhysicalDeviceCount = count;

out:

    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
                                                                     VkPhysicalDeviceFeatures *pFeatures) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev = loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceFeatures(unwrapped_phys_dev, pFeatures);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                                             VkFormatProperties *pFormatInfo) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_pd = loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceFormatProperties(unwrapped_pd, format, pFormatInfo);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage,
    VkImageCreateFlags flags, VkImageFormatProperties *pImageFormatProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev = loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    return disp->GetPhysicalDeviceImageFormatProperties(unwrapped_phys_dev, format, type, tiling, usage, flags,
                                                        pImageFormatProperties);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,
                                                                       VkPhysicalDeviceProperties *pProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev = loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceProperties(unwrapped_phys_dev, pProperties);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice,
                                                                                  uint32_t *pQueueFamilyPropertyCount,
                                                                                  VkQueueFamilyProperties *pQueueProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev = loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceQueueFamilyProperties(unwrapped_phys_dev, pQueueFamilyPropertyCount, pQueueProperties);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice,
                                                                             VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev = loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);
    disp->GetPhysicalDeviceMemoryProperties(unwrapped_phys_dev, pMemoryProperties);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    VkResult res;
    struct loader_physical_device_tramp *phys_dev = NULL;
    struct loader_device *dev = NULL;
    struct loader_instance *inst = NULL;

    assert(pCreateInfo->queueCreateInfoCount >= 1);

    loader_platform_thread_lock_mutex(&loader_lock);

    phys_dev = (struct loader_physical_device_tramp *)physicalDevice;
    inst = (struct loader_instance *)phys_dev->this_instance;

    // Get the physical device (ICD) extensions
    struct loader_extension_list icd_exts;
    icd_exts.list = NULL;
    res = loader_init_generic_list(inst, (struct loader_generic_list *)&icd_exts, sizeof(VkExtensionProperties));
    if (VK_SUCCESS != res) {
        loader_log(inst, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0, "vkCreateDevice:  Failed to create ICD extension list");
        goto out;
    }

    res = loader_add_device_extensions(inst, inst->disp->layer_inst_disp.EnumerateDeviceExtensionProperties, phys_dev->phys_dev,
                                       "Unknown", &icd_exts);
    if (res != VK_SUCCESS) {
        loader_log(inst, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0, "vkCreateDevice:  Failed to add extensions to list");
        goto out;
    }

    // Make sure requested extensions to be enabled are supported
    res = loader_validate_device_extensions(phys_dev, &inst->expanded_activated_layer_list, &icd_exts, pCreateInfo);
    if (res != VK_SUCCESS) {
        loader_log(inst, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0, "vkCreateDevice:  Failed to validate extensions in list");
        goto out;
    }

    dev = loader_create_logical_device(inst, pAllocator);
    if (dev == NULL) {
        res = VK_ERROR_OUT_OF_HOST_MEMORY;
        goto out;
    }

    // Copy the application enabled instance layer list into the device
    if (NULL != inst->app_activated_layer_list.list) {
        dev->app_activated_layer_list.capacity = inst->app_activated_layer_list.capacity;
        dev->app_activated_layer_list.count = inst->app_activated_layer_list.count;
        dev->app_activated_layer_list.list =
            loader_device_heap_alloc(dev, inst->app_activated_layer_list.capacity, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
        if (dev->app_activated_layer_list.list == NULL) {
            loader_log(inst, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                       "vkCreateDevice:  Failed to allocate application activated layer list of size %d.",
                       inst->app_activated_layer_list.capacity);
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            goto out;
        }
        memcpy(dev->app_activated_layer_list.list, inst->app_activated_layer_list.list,
               sizeof(*dev->app_activated_layer_list.list) * dev->app_activated_layer_list.count);
    } else {
        dev->app_activated_layer_list.capacity = 0;
        dev->app_activated_layer_list.count = 0;
        dev->app_activated_layer_list.list = NULL;
    }

    // Copy the expanded enabled instance layer list into the device
    if (NULL != inst->expanded_activated_layer_list.list) {
        dev->expanded_activated_layer_list.capacity = inst->expanded_activated_layer_list.capacity;
        dev->expanded_activated_layer_list.count = inst->expanded_activated_layer_list.count;
        dev->expanded_activated_layer_list.list =
            loader_device_heap_alloc(dev, inst->expanded_activated_layer_list.capacity, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
        if (dev->expanded_activated_layer_list.list == NULL) {
            loader_log(inst, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                       "vkCreateDevice:  Failed to allocate expanded activated layer list of size %d.",
                       inst->expanded_activated_layer_list.capacity);
            res = VK_ERROR_OUT_OF_HOST_MEMORY;
            goto out;
        }
        memcpy(dev->expanded_activated_layer_list.list, inst->expanded_activated_layer_list.list,
               sizeof(*dev->expanded_activated_layer_list.list) * dev->expanded_activated_layer_list.count);
    } else {
        dev->expanded_activated_layer_list.capacity = 0;
        dev->expanded_activated_layer_list.count = 0;
        dev->expanded_activated_layer_list.list = NULL;
    }

    res = loader_create_device_chain(phys_dev, pCreateInfo, pAllocator, inst, dev);
    if (res != VK_SUCCESS) {
        loader_log(inst, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0, "vkCreateDevice:  Failed to create device chain.");
        goto out;
    }

    *pDevice = dev->chain_device;

    // Initialize any device extension dispatch entry's from the instance list
    loader_init_dispatch_dev_ext(inst, dev);

    // Initialize WSI device extensions as part of core dispatch since loader
    // has dedicated trampoline code for these*/
    loader_init_device_extension_dispatch_table(&dev->loader_dispatch, dev->loader_dispatch.core_dispatch.GetDeviceProcAddr,
                                                *pDevice);

out:

    // Failure cleanup
    if (VK_SUCCESS != res) {
        if (NULL != dev) {
            loader_destroy_logical_device(inst, dev, pAllocator);
        }
    }

    if (NULL != icd_exts.list) {
        loader_destroy_generic_list(inst, (struct loader_generic_list *)&icd_exts);
    }
    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;
    struct loader_device *dev;

    if (device == VK_NULL_HANDLE) {
        return;
    }

    loader_platform_thread_lock_mutex(&loader_lock);

    struct loader_icd_term *icd_term = loader_get_icd_and_device(device, &dev, NULL);
    const struct loader_instance *inst = icd_term->this_instance;
    disp = loader_get_dispatch(device);

    disp->DestroyDevice(device, pAllocator);
    dev->chain_device = NULL;
    dev->icd_device = NULL;
    loader_remove_logical_device(inst, icd_term, dev, pAllocator);

    loader_platform_thread_unlock_mutex(&loader_lock);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                  const char *pLayerName, uint32_t *pPropertyCount,
                                                                                  VkExtensionProperties *pProperties) {
    VkResult res = VK_SUCCESS;
    struct loader_physical_device_tramp *phys_dev;
    phys_dev = (struct loader_physical_device_tramp *)physicalDevice;

    loader_platform_thread_lock_mutex(&loader_lock);

    // If pLayerName == NULL, then querying ICD extensions, pass this call
    // down the instance chain which will terminate in the ICD. This allows
    // layers to filter the extensions coming back up the chain.
    // If pLayerName != NULL then get layer extensions from manifest file.
    if (pLayerName == NULL || strlen(pLayerName) == 0) {
        const VkLayerInstanceDispatchTable *disp;

        disp = loader_get_instance_layer_dispatch(physicalDevice);
        res = disp->EnumerateDeviceExtensionProperties(phys_dev->phys_dev, NULL, pPropertyCount, pProperties);
    } else {
        uint32_t count;
        uint32_t copy_size;
        const struct loader_instance *inst = phys_dev->this_instance;
        struct loader_device_extension_list *dev_ext_list = NULL;
        struct loader_device_extension_list local_ext_list;
        memset(&local_ext_list, 0, sizeof(local_ext_list));
        if (vk_string_validate(MaxLoaderStringLength, pLayerName) == VK_STRING_ERROR_NONE) {
            for (uint32_t i = 0; i < inst->instance_layer_list.count; i++) {
                struct loader_layer_properties *props = &inst->instance_layer_list.list[i];
                if (strcmp(props->info.layerName, pLayerName) == 0) {
                    dev_ext_list = &props->device_extension_list;
                }
            }

            count = (dev_ext_list == NULL) ? 0 : dev_ext_list->count;
            if (pProperties == NULL) {
                *pPropertyCount = count;
                loader_destroy_generic_list(inst, (struct loader_generic_list *)&local_ext_list);
                loader_platform_thread_unlock_mutex(&loader_lock);
                return VK_SUCCESS;
            }

            copy_size = *pPropertyCount < count ? *pPropertyCount : count;
            for (uint32_t i = 0; i < copy_size; i++) {
                memcpy(&pProperties[i], &dev_ext_list->list[i].props, sizeof(VkExtensionProperties));
            }
            *pPropertyCount = copy_size;

            loader_destroy_generic_list(inst, (struct loader_generic_list *)&local_ext_list);
            if (copy_size < count) {
                loader_platform_thread_unlock_mutex(&loader_lock);
                return VK_INCOMPLETE;
            }
        } else {
            loader_log(inst, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                       "vkEnumerateDeviceExtensionProperties:  pLayerName "
                       "is too long or is badly formed");
            loader_platform_thread_unlock_mutex(&loader_lock);
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
                                                                              uint32_t *pPropertyCount,
                                                                              VkLayerProperties *pProperties) {
    uint32_t copy_size;
    struct loader_physical_device_tramp *phys_dev;
    struct loader_layer_list *enabled_layers, layers_list;
    memset(&layers_list, 0, sizeof(layers_list));
    loader_platform_thread_lock_mutex(&loader_lock);

    // Don't dispatch this call down the instance chain, want all device layers
    // enumerated and instance chain may not contain all device layers
    // TODO re-evaluate the above statement we maybe able to start calling
    // down the chain

    phys_dev = (struct loader_physical_device_tramp *)physicalDevice;
    const struct loader_instance *inst = phys_dev->this_instance;

    uint32_t count = inst->app_activated_layer_list.count;
    if (count == 0 || pProperties == NULL) {
        *pPropertyCount = count;
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_SUCCESS;
    }
    enabled_layers = (struct loader_layer_list *)&inst->app_activated_layer_list;

    copy_size = (*pPropertyCount < count) ? *pPropertyCount : count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i], &(enabled_layers->list[i].info), sizeof(VkLayerProperties));
    }
    *pPropertyCount = copy_size;

    if (copy_size < count) {
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_INCOMPLETE;
    }

    loader_platform_thread_unlock_mutex(&loader_lock);
    return VK_SUCCESS;
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(VkDevice device, uint32_t queueNodeIndex, uint32_t queueIndex,
                                                          VkQueue *pQueue) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    loader_set_dispatch(*pQueue, disp);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits,
                                                           VkFence fence) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueSubmit(queue, submitCount, pSubmits, fence);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(VkQueue queue) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueWaitIdle(queue);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(VkDevice device) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DeviceWaitIdle(device);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
                                                              const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeMemory(VkDevice device, VkDeviceMemory mem,
                                                      const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->FreeMemory(device, mem, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset,
                                                         VkDeviceSize size, VkFlags flags, void **ppData) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->MapMemory(device, mem, offset, size, flags, ppData);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(VkDevice device, VkDeviceMemory mem) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->UnmapMemory(device, mem);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                                       const VkMappedMemoryRange *pMemoryRanges) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->FlushMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                                            const VkMappedMemoryRange *pMemoryRanges) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->InvalidateMappedMemoryRanges(device, memoryRangeCount, pMemoryRanges);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory,
                                                                     VkDeviceSize *pCommittedMemoryInBytes) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory mem,
                                                                VkDeviceSize offset) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->BindBufferMemory(device, buffer, mem, offset);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory mem,
                                                               VkDeviceSize offset) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->BindImageMemory(device, image, mem, offset);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer,
                                                                       VkMemoryRequirements *pMemoryRequirements) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(VkDevice device, VkImage image,
                                                                      VkMemoryRequirements *pMemoryRequirements) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetImageMemoryRequirements(device, image, pMemoryRequirements);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL
vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount,
                                   VkSparseImageMemoryRequirements *pSparseMemoryRequirements) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetImageSparseMemoryRequirements(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage,
    VkImageTiling tiling, uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev = loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_layer_dispatch(physicalDevice);

    disp->GetPhysicalDeviceSparseImageFormatProperties(unwrapped_phys_dev, format, type, samples, usage, tiling, pPropertyCount,
                                                       pProperties);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount,
                                                               const VkBindSparseInfo *pBindInfo, VkFence fence) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo,
                                                           const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateFence(device, pCreateInfo, pAllocator, pFence);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyFence(device, fence, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetFences(device, fenceCount, pFences);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(VkDevice device, VkFence fence) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetFenceStatus(device, fence);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences,
                                                             VkBool32 waitAll, uint64_t timeout) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->WaitForFences(device, fenceCount, pFences, waitAll, timeout);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo,
                                                               const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(VkDevice device, VkSemaphore semaphore,
                                                            const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroySemaphore(device, semaphore, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo,
                                                           const VkAllocationCallbacks *pAllocator, VkEvent *pEvent) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateEvent(device, pCreateInfo, pAllocator, pEvent);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyEvent(device, event, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus(VkDevice device, VkEvent event) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetEventStatus(device, event);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(VkDevice device, VkEvent event) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->SetEvent(device, event);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent(VkDevice device, VkEvent event) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetEvent(device, event);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo,
                                                               const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool,
                                                            const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyQueryPool(device, queryPool, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery,
                                                                   uint32_t queryCount, size_t dataSize, void *pData,
                                                                   VkDeviceSize stride, VkQueryResultFlags flags) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(VkDevice device, VkBuffer buffer,
                                                         const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyBuffer(device, buffer, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo,
                                                                const VkAllocationCallbacks *pAllocator, VkBufferView *pView) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateBufferView(device, pCreateInfo, pAllocator, pView);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBufferView(VkDevice device, VkBufferView bufferView,
                                                             const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyBufferView(device, bufferView, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
                                                           const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateImage(device, pCreateInfo, pAllocator, pImage);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyImage(device, image, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout(VkDevice device, VkImage image,
                                                                     const VkImageSubresource *pSubresource,
                                                                     VkSubresourceLayout *pLayout) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                                                               const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateImageView(device, pCreateInfo, pAllocator, pView);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(VkDevice device, VkImageView imageView,
                                                            const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyImageView(device, imageView, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo,
                                                                  const VkAllocationCallbacks *pAllocator,
                                                                  VkShaderModule *pShader) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateShaderModule(device, pCreateInfo, pAllocator, pShader);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule,
                                                               const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyShaderModule(device, shaderModule, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo,
                                                                   const VkAllocationCallbacks *pAllocator,
                                                                   VkPipelineCache *pPipelineCache) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache,
                                                                const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyPipelineCache(device, pipelineCache, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache,
                                                                    size_t *pDataSize, void *pData) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache,
                                                                   uint32_t srcCacheCount, const VkPipelineCache *pSrcCaches) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache,
                                                                       uint32_t createInfoCount,
                                                                       const VkGraphicsPipelineCreateInfo *pCreateInfos,
                                                                       const VkAllocationCallbacks *pAllocator,
                                                                       VkPipeline *pPipelines) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache,
                                                                      uint32_t createInfoCount,
                                                                      const VkComputePipelineCreateInfo *pCreateInfos,
                                                                      const VkAllocationCallbacks *pAllocator,
                                                                      VkPipeline *pPipelines) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(VkDevice device, VkPipeline pipeline,
                                                           const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyPipeline(device, pipeline, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo,
                                                                    const VkAllocationCallbacks *pAllocator,
                                                                    VkPipelineLayout *pPipelineLayout) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout,
                                                                 const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo,
                                                             const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateSampler(device, pCreateInfo, pAllocator, pSampler);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySampler(VkDevice device, VkSampler sampler,
                                                          const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroySampler(device, sampler, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(VkDevice device,
                                                                         const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                                         const VkAllocationCallbacks *pAllocator,
                                                                         VkDescriptorSetLayout *pSetLayout) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
                                                                      const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo,
                                                                    const VkAllocationCallbacks *pAllocator,
                                                                    VkDescriptorPool *pDescriptorPool) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                                 const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyDescriptorPool(device, descriptorPool, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                                   VkDescriptorPoolResetFlags flags) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetDescriptorPool(device, descriptorPool, flags);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(VkDevice device,
                                                                      const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                                      VkDescriptorSet *pDescriptorSets) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool,
                                                                  uint32_t descriptorSetCount,
                                                                  const VkDescriptorSet *pDescriptorSets) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->FreeDescriptorSets(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount,
                                                                const VkWriteDescriptorSet *pDescriptorWrites,
                                                                uint32_t descriptorCopyCount,
                                                                const VkCopyDescriptorSet *pDescriptorCopies) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo,
                                                                 const VkAllocationCallbacks *pAllocator,
                                                                 VkFramebuffer *pFramebuffer) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer,
                                                              const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyFramebuffer(device, framebuffer, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                                                const VkAllocationCallbacks *pAllocator,
                                                                VkRenderPass *pRenderPass) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass,
                                                             const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyRenderPass(device, renderPass, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass,
                                                                    VkExtent2D *pGranularity) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetRenderAreaGranularity(device, renderPass, pGranularity);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
                                                                 const VkAllocationCallbacks *pAllocator,
                                                                 VkCommandPool *pCommandPool) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool,
                                                              const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyCommandPool(device, commandPool, pAllocator);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(VkDevice device, VkCommandPool commandPool,
                                                                VkCommandPoolResetFlags flags) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetCommandPool(device, commandPool, flags);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(VkDevice device,
                                                                      const VkCommandBufferAllocateInfo *pAllocateInfo,
                                                                      VkCommandBuffer *pCommandBuffers) {
    const VkLayerDispatchTable *disp;
    VkResult res;

    disp = loader_get_dispatch(device);

    res = disp->AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);
    if (res == VK_SUCCESS) {
        for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
            if (pCommandBuffers[i]) {
                loader_init_dispatch(pCommandBuffers[i], disp);
            }
        }
    }

    return res;
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool,
                                                              uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(VkCommandBuffer commandBuffer,
                                                                  const VkCommandBufferBeginInfo *pBeginInfo) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    return disp->BeginCommandBuffer(commandBuffer, pBeginInfo);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(VkCommandBuffer commandBuffer) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    return disp->EndCommandBuffer(commandBuffer);
}

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    return disp->ResetCommandBuffer(commandBuffer, flags);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                                           VkPipeline pipeline) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport,
                                                          uint32_t viewportCount, const VkViewport *pViewports) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor,
                                                         uint32_t scissorCount, const VkRect2D *pScissors) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetLineWidth(commandBuffer, lineWidth);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor,
                                                           float depthBiasClamp, float depthBiasSlopeFactor) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetBlendConstants(commandBuffer, blendConstants);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds,
                                                             float maxDepthBounds) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask,
                                                                    uint32_t compareMask) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask,
                                                                  uint32_t writeMask) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask,
                                                                  uint32_t reference) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetStencilReference(commandBuffer, faceMask, reference);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer,
                                                                 VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout,
                                                                 uint32_t firstSet, uint32_t descriptorSetCount,
                                                                 const VkDescriptorSet *pDescriptorSets,
                                                                 uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets,
                                dynamicOffsetCount, pDynamicOffsets);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                              VkIndexType indexType) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding,
                                                                uint32_t bindingCount, const VkBuffer *pBuffers,
                                                                const VkDeviceSize *pOffsets) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
                                                   uint32_t firstVertex, uint32_t firstInstance) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount,
                                                          uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                                                          uint32_t firstInstance) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                           uint32_t drawCount, uint32_t stride) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdDrawIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer,
                                                                  VkDeviceSize offset, uint32_t drawCount, uint32_t stride) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdDrawIndexedIndirect(commandBuffer, buffer, offset, drawCount, stride);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdDispatch(commandBuffer, x, y, z);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer,
                                                               VkDeviceSize offset) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdDispatchIndirect(commandBuffer, buffer, offset);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
                                                         uint32_t regionCount, const VkBufferCopy *pRegions) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage,
                                                        VkImageLayout srcImageLayout, VkImage dstImage,
                                                        VkImageLayout dstImageLayout, uint32_t regionCount,
                                                        const VkImageCopy *pRegions) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage,
                                                        VkImageLayout srcImageLayout, VkImage dstImage,
                                                        VkImageLayout dstImageLayout, uint32_t regionCount,
                                                        const VkImageBlit *pRegions, VkFilter filter) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                                                VkImageLayout dstImageLayout, uint32_t regionCount,
                                                                const VkBufferImageCopy *pRegions) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage,
                                                                VkImageLayout srcImageLayout, VkBuffer dstBuffer,
                                                                uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer,
                                                           VkDeviceSize dstOffset, VkDeviceSize dataSize, const void *pData) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                                         VkDeviceSize size, uint32_t data) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image,
                                                              VkImageLayout imageLayout, const VkClearColorValue *pColor,
                                                              uint32_t rangeCount, const VkImageSubresourceRange *pRanges) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image,
                                                                     VkImageLayout imageLayout,
                                                                     const VkClearDepthStencilValue *pDepthStencil,
                                                                     uint32_t rangeCount, const VkImageSubresourceRange *pRanges) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                                               const VkClearAttachment *pAttachments, uint32_t rectCount,
                                                               const VkClearRect *pRects) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage,
                                                           VkImageLayout srcImageLayout, VkImage dstImage,
                                                           VkImageLayout dstImageLayout, uint32_t regionCount,
                                                           const VkImageResolve *pRegions) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event,
                                                       VkPipelineStageFlags stageMask) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdSetEvent(commandBuffer, event, stageMask);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event,
                                                         VkPipelineStageFlags stageMask) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdResetEvent(commandBuffer, event, stageMask);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents,
                                                         VkPipelineStageFlags sourceStageMask, VkPipelineStageFlags dstStageMask,
                                                         uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                                         uint32_t bufferMemoryBarrierCount,
                                                         const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                                         uint32_t imageMemoryBarrierCount,
                                                         const VkImageMemoryBarrier *pImageMemoryBarriers) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdWaitEvents(commandBuffer, eventCount, pEvents, sourceStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers,
                        bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                                              VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                                              uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                                              uint32_t bufferMemoryBarrierCount,
                                                              const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                                              uint32_t imageMemoryBarrierCount,
                                                              const VkImageMemoryBarrier *pImageMemoryBarriers) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers,
                             bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot,
                                                         VkFlags flags) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdBeginQuery(commandBuffer, queryPool, slot, flags);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdEndQuery(commandBuffer, queryPool, slot);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool,
                                                             uint32_t firstQuery, uint32_t queryCount) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage,
                                                             VkQueryPool queryPool, uint32_t slot) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, slot);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool,
                                                                   uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer,
                                                                   VkDeviceSize dstOffset, VkDeviceSize stride, VkFlags flags) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout,
                                                            VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size,
                                                            const void *pValues) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(VkCommandBuffer commandBuffer,
                                                              const VkRenderPassBeginInfo *pRenderPassBegin,
                                                              VkSubpassContents contents) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdNextSubpass(commandBuffer, contents);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(VkCommandBuffer commandBuffer) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdEndRenderPass(commandBuffer);
}

LOADER_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBuffersCount,
                                                              const VkCommandBuffer *pCommandBuffers) {
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(commandBuffer);

    disp->CmdExecuteCommands(commandBuffer, commandBuffersCount, pCommandBuffers);
}
