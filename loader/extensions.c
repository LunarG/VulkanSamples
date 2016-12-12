/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
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
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vk_loader_platform.h"
#include "loader.h"
#include "extensions.h"
#include <vulkan/vk_icd.h>
#include "wsi.h"

// Definitions for the VK_EXT_debug_marker extension commands which
// need to have a terminator function

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectTagEXT(
    VkDevice device, VkDebugMarkerObjectTagInfoEXT *pTagInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    // If this is a physical device, we have to replace it with the proper one
    // for the next call.
    if (pTagInfo->objectType ==
        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {
        struct loader_physical_device_tramp *phys_dev_tramp =
            (struct loader_physical_device_tramp *)(uintptr_t)pTagInfo->object;
        pTagInfo->object = (uint64_t)(uintptr_t)phys_dev_tramp->phys_dev;
    }
    return disp->DebugMarkerSetObjectTagEXT(device, pTagInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_DebugMarkerSetObjectTagEXT(
    VkDevice device, VkDebugMarkerObjectTagInfoEXT *pTagInfo) {
    uint32_t icd_index = 0;
    struct loader_device *dev;
    struct loader_icd_term *icd_term =
        loader_get_icd_and_device(device, &dev, &icd_index);
    if (NULL != icd_term && NULL != icd_term->DebugMarkerSetObjectTagEXT) {
        // If this is a physical device, we have to replace it with the proper
        // one for the next call.
        if (pTagInfo->objectType ==
            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {
            struct loader_physical_device_term *phys_dev_term =
                (struct loader_physical_device_term *)(uintptr_t)
                    pTagInfo->object;
            pTagInfo->object = (uint64_t)(uintptr_t)phys_dev_term->phys_dev;

            // If this is a KHR_surface, and the ICD has created its own, we
            // have to replace it with the proper one for the next call.
        } else if (pTagInfo->objectType ==
                   VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT) {
            if (NULL != icd_term && NULL != icd_term->CreateSwapchainKHR) {
                VkIcdSurface *icd_surface =
                    (VkIcdSurface *)(uintptr_t)pTagInfo->object;
                if (NULL != icd_surface->real_icd_surfaces) {
                    pTagInfo->object =
                        (uint64_t)icd_surface->real_icd_surfaces[icd_index];
                }
            }
        }
        return icd_term->DebugMarkerSetObjectTagEXT(device, pTagInfo);
    } else {
        return VK_SUCCESS;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectNameEXT(
    VkDevice device, VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    // If this is a physical device, we have to replace it with the proper one
    // for the next call.
    if (pNameInfo->objectType ==
        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {
        struct loader_physical_device_tramp *phys_dev_tramp =
            (struct loader_physical_device_tramp *)(uintptr_t)pNameInfo->object;
        pNameInfo->object = (uint64_t)(uintptr_t)phys_dev_tramp->phys_dev;
    }
    return disp->DebugMarkerSetObjectNameEXT(device, pNameInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL terminator_DebugMarkerSetObjectNameEXT(
    VkDevice device, VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    uint32_t icd_index = 0;
    struct loader_device *dev;
    struct loader_icd_term *icd_term =
        loader_get_icd_and_device(device, &dev, &icd_index);
    if (NULL != icd_term && NULL != icd_term->DebugMarkerSetObjectNameEXT) {
        // If this is a physical device, we have to replace it with the proper
        // one for the next call.
        if (pNameInfo->objectType ==
            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT) {
            struct loader_physical_device_term *phys_dev_term =
                (struct loader_physical_device_term *)(uintptr_t)
                    pNameInfo->object;
            pNameInfo->object = (uint64_t)(uintptr_t)phys_dev_term->phys_dev;

            // If this is a KHR_surface, and the ICD has created its own, we
            // have to replace it with the proper one for the next call.
        } else if (pNameInfo->objectType ==
                   VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT) {
            if (NULL != icd_term && NULL != icd_term->CreateSwapchainKHR) {
                VkIcdSurface *icd_surface =
                    (VkIcdSurface *)(uintptr_t)pNameInfo->object;
                if (NULL != icd_surface->real_icd_surfaces) {
                    pNameInfo->object =
                        (uint64_t)(
                            uintptr_t)icd_surface->real_icd_surfaces[icd_index];
                }
            }
        }
        return icd_term->DebugMarkerSetObjectNameEXT(device, pNameInfo);
    } else {
        return VK_SUCCESS;
    }
}

// Definitions for the VK_NV_external_memory_capabilities extension

LOADER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkGetPhysicalDeviceExternalImageFormatPropertiesNV(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_dispatch(physicalDevice);

    return disp->GetPhysicalDeviceExternalImageFormatPropertiesNV(
        unwrapped_phys_dev, format, type, tiling, usage, flags,
        externalHandleType, pExternalImageFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceExternalImageFormatPropertiesNV(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;

    if (!icd_term->GetPhysicalDeviceExternalImageFormatPropertiesNV) {
        if (externalHandleType) {
            return VK_ERROR_FORMAT_NOT_SUPPORTED;
        }

        if (!icd_term->GetPhysicalDeviceImageFormatProperties) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        pExternalImageFormatProperties->externalMemoryFeatures = 0;
        pExternalImageFormatProperties->exportFromImportedHandleTypes = 0;
        pExternalImageFormatProperties->compatibleHandleTypes = 0;

        return icd_term->GetPhysicalDeviceImageFormatProperties(
            phys_dev_term->phys_dev, format, type, tiling, usage, flags,
            &pExternalImageFormatProperties->imageFormatProperties);
    }

    return icd_term->GetPhysicalDeviceExternalImageFormatPropertiesNV(
        phys_dev_term->phys_dev, format, type, tiling, usage, flags,
        externalHandleType, pExternalImageFormatProperties);
}

// Definitions for the VK_AMD_draw_indirect_count extension

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectCountAMD(
    VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
    VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
    uint32_t stride) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdDrawIndirectCountAMD(commandBuffer, buffer, offset, countBuffer,
                                  countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirectCountAMD(
    VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
    VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
    uint32_t stride) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdDrawIndexedIndirectCountAMD(commandBuffer, buffer, offset,
                                         countBuffer, countBufferOffset,
                                         maxDrawCount, stride);
}

#ifdef VK_USE_PLATFORM_WIN32_KHR

// Definitions for the VK_NV_external_memory_win32 extension

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleNV(
    VkDevice device, VkDeviceMemory memory,
    VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE *pHandle) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->GetMemoryWin32HandleNV(device, memory, handleType, pHandle);
}

#endif // VK_USE_PLATFORM_WIN32_KHR

// Definitions for the VK_NVX_device_generated_commands

VKAPI_ATTR void VKAPI_CALL vkCmdProcessCommandsNVX(
    VkCommandBuffer commandBuffer,
    const VkCmdProcessCommandsInfoNVX *pProcessCommandsInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdProcessCommandsNVX(commandBuffer, pProcessCommandsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdReserveSpaceForCommandsNVX(
    VkCommandBuffer commandBuffer,
    const VkCmdReserveSpaceForCommandsInfoNVX *pReserveSpaceInfo) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(commandBuffer);
    disp->CmdReserveSpaceForCommandsNVX(commandBuffer, pReserveSpaceInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateIndirectCommandsLayoutNVX(
    VkDevice device, const VkIndirectCommandsLayoutCreateInfoNVX *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkIndirectCommandsLayoutNVX *pIndirectCommandsLayout) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->CreateIndirectCommandsLayoutNVX(
        device, pCreateInfo, pAllocator, pIndirectCommandsLayout);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyIndirectCommandsLayoutNVX(
    VkDevice device, VkIndirectCommandsLayoutNVX indirectCommandsLayout,
    const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    disp->DestroyIndirectCommandsLayoutNVX(device, indirectCommandsLayout,
                                           pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateObjectTableNVX(
    VkDevice device, const VkObjectTableCreateInfoNVX *pCreateInfo,
    const VkAllocationCallbacks *pAllocator, VkObjectTableNVX *pObjectTable) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->CreateObjectTableNVX(device, pCreateInfo, pAllocator,
                                      pObjectTable);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyObjectTableNVX(VkDevice device, VkObjectTableNVX objectTable,
                        const VkAllocationCallbacks *pAllocator) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    disp->DestroyObjectTableNVX(device, objectTable, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkRegisterObjectsNVX(
    VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount,
    const VkObjectTableEntryNVX *const *ppObjectTableEntries,
    const uint32_t *pObjectIndices) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->RegisterObjectsNVX(device, objectTable, objectCount,
                                    ppObjectTableEntries, pObjectIndices);
}

VKAPI_ATTR VkResult VKAPI_CALL vkUnregisterObjectsNVX(
    VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount,
    const VkObjectEntryTypeNVX *pObjectEntryTypes,
    const uint32_t *pObjectIndices) {
    const VkLayerDispatchTable *disp = loader_get_dispatch(device);
    return disp->UnregisterObjectsNVX(device, objectTable, objectCount,
                                      pObjectEntryTypes, pObjectIndices);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX(
    VkPhysicalDevice physicalDevice,
    VkDeviceGeneratedCommandsFeaturesNVX *pFeatures,
    VkDeviceGeneratedCommandsLimitsNVX *pLimits) {
    const VkLayerInstanceDispatchTable *disp;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_dispatch(physicalDevice);
    disp->GetPhysicalDeviceGeneratedCommandsPropertiesNVX(unwrapped_phys_dev,
                                                          pFeatures, pLimits);
}

VKAPI_ATTR void VKAPI_CALL
terminator_GetPhysicalDeviceGeneratedCommandsPropertiesNVX(
    VkPhysicalDevice physicalDevice,
    VkDeviceGeneratedCommandsFeaturesNVX *pFeatures,
    VkDeviceGeneratedCommandsLimitsNVX *pLimits) {
    struct loader_physical_device_term *phys_dev_term =
        (struct loader_physical_device_term *)physicalDevice;
    struct loader_icd_term *icd_term = phys_dev_term->this_icd_term;
    if (NULL == icd_term->GetPhysicalDeviceGeneratedCommandsPropertiesNVX) {
        loader_log(icd_term->this_instance, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
                   "ICD associated with VkPhysicalDevice does not support "
                   "vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX");
    }
    icd_term->GetPhysicalDeviceGeneratedCommandsPropertiesNVX(
        phys_dev_term->phys_dev, pFeatures, pLimits);
}

// GPA helpers for non-KHR extensions

bool extension_instance_gpa(struct loader_instance *ptr_instance,
                            const char *name, void **addr) {
    *addr = NULL;

    // Definitions for the VK_EXT_debug_marker extension commands which
    // need to have a terminator function.  Since these are device
    // commands, we always need to return a valid value for them.

    if (!strcmp("vkDebugMarkerSetObjectTagEXT", name)) {
        *addr = (void *)vkDebugMarkerSetObjectTagEXT;
        return true;
    }
    if (!strcmp("vkDebugMarkerSetObjectNameEXT", name)) {
        *addr = (void *)vkDebugMarkerSetObjectNameEXT;
        return true;
    }

    // Functions for the VK_NV_external_memory_capabilities extension

    if (!strcmp("vkGetPhysicalDeviceExternalImageFormatPropertiesNV", name)) {
        *addr = (ptr_instance->enabled_known_extensions
                     .nv_external_memory_capabilities == 1)
                    ? (void *)vkGetPhysicalDeviceExternalImageFormatPropertiesNV
                    : NULL;
        return true;
    }

    // Functions for the VK_AMD_draw_indirect_count extension

    if (!strcmp("vkCmdDrawIndirectCountAMD", name)) {
        *addr = (void *)vkCmdDrawIndirectCountAMD;
        return true;
    }

    if (!strcmp("vkCmdDrawIndexedIndirectCountAMD", name)) {
        *addr = (void *)vkCmdDrawIndexedIndirectCountAMD;
        return true;
    }

#ifdef VK_USE_PLATFORM_WIN32_KHR

    // Functions for the VK_NV_external_memory_win32 extension

    if (!strcmp("vkGetMemoryWin32HandleNV", name)) {
        *addr = (void *)vkGetMemoryWin32HandleNV;
        return true;
    }

#endif // VK_USE_PLATFORM_WIN32_KHR

    // Functions for the VK_NVX_device_generated_commands extension

    if (!strcmp("vkCmdProcessCommandsNVX", name)) {
        *addr = (void *)vkCmdProcessCommandsNVX;
        return true;
    }
    if (!strcmp("vkCmdReserveSpaceForCommandsNVX", name)) {
        *addr = (void *)vkCmdReserveSpaceForCommandsNVX;
        return true;
    }
    if (!strcmp("vkCreateIndirectCommandsLayoutNVX", name)) {
        *addr = (void *)vkCreateIndirectCommandsLayoutNVX;
        return true;
    }
    if (!strcmp("vkDestroyIndirectCommandsLayoutNVX", name)) {
        *addr = (void *)vkDestroyIndirectCommandsLayoutNVX;
        return true;
    }
    if (!strcmp("vkCreateObjectTableNVX", name)) {
        *addr = (void *)vkCreateObjectTableNVX;
        return true;
    }
    if (!strcmp("vkDestroyObjectTableNVX", name)) {
        *addr = (void *)vkDestroyObjectTableNVX;
        return true;
    }
    if (!strcmp("vkRegisterObjectsNVX", name)) {
        *addr = (void *)vkRegisterObjectsNVX;
        return true;
    }
    if (!strcmp("vkUnregisterObjectsNVX", name)) {
        *addr = (void *)vkUnregisterObjectsNVX;
        return true;
    }
    if (!strcmp("vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX", name)) {
        *addr = (void *)vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX;
        return true;
    }

    return false;
}

void extensions_create_instance(struct loader_instance *ptr_instance,
                                const VkInstanceCreateInfo *pCreateInfo) {
    ptr_instance->enabled_known_extensions.nv_external_memory_capabilities = 0;

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                   VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME) == 0) {
            ptr_instance->enabled_known_extensions
                .nv_external_memory_capabilities = 1;
            return;
        }
    }
}
