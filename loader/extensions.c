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

// Definitions for EXT_debug_marker extension

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectTagEXT(
    VkDevice device, VkDebugMarkerObjectTagInfoEXT *pTagInfo) {
    struct loader_dev_dispatch_table *disp = loader_get_dev_dispatch(device);
    if (0 == disp->enabled_known_extensions.ext_debug_marker ||
        NULL == disp->core_dispatch.DebugMarkerSetObjectTagEXT) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    } else {
        return disp->core_dispatch.DebugMarkerSetObjectTagEXT(device, pTagInfo);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectNameEXT(
    VkDevice device, VkDebugMarkerObjectNameInfoEXT *pNameInfo) {
    struct loader_dev_dispatch_table *disp = loader_get_dev_dispatch(device);
    if (0 == disp->enabled_known_extensions.ext_debug_marker ||
        NULL == disp->core_dispatch.DebugMarkerSetObjectNameEXT) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    } else {
        return disp->core_dispatch.DebugMarkerSetObjectNameEXT(device,
                                                               pNameInfo);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerBeginEXT(
    VkCommandBuffer commandBuffer, VkDebugMarkerMarkerInfoEXT *pMarkerInfo) {
    struct loader_dev_dispatch_table *disp =
        loader_get_dev_dispatch(commandBuffer);
    if (1 == disp->enabled_known_extensions.ext_debug_marker &&
        NULL != disp->core_dispatch.CmdDebugMarkerBeginEXT) {
        disp->core_dispatch.CmdDebugMarkerBeginEXT(commandBuffer, pMarkerInfo);
    }
}

VKAPI_ATTR void VKAPI_CALL
vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) {
    struct loader_dev_dispatch_table *disp =
        loader_get_dev_dispatch(commandBuffer);
    if (1 == disp->enabled_known_extensions.ext_debug_marker &&
        NULL != disp->core_dispatch.CmdDebugMarkerEndEXT) {
        disp->core_dispatch.CmdDebugMarkerEndEXT(commandBuffer);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerInsertEXT(
    VkCommandBuffer commandBuffer, VkDebugMarkerMarkerInfoEXT *pMarkerInfo) {
    struct loader_dev_dispatch_table *disp =
        loader_get_dev_dispatch(commandBuffer);
    if (1 == disp->enabled_known_extensions.ext_debug_marker &&
        NULL != disp->core_dispatch.CmdDebugMarkerInsertEXT) {
        disp->core_dispatch.CmdDebugMarkerInsertEXT(commandBuffer, pMarkerInfo);
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
    struct loader_physical_device_tramp *phys_dev =
        (struct loader_physical_device_tramp *)physicalDevice;
    struct loader_instance *inst = phys_dev->this_instance;
    VkPhysicalDevice unwrapped_phys_dev =
        loader_unwrap_physical_device(physicalDevice);
    disp = loader_get_instance_dispatch(physicalDevice);

    if (0 == inst->enabled_known_extensions.nv_external_memory_capabilities ||
        NULL == disp->GetPhysicalDeviceExternalImageFormatPropertiesNV) {
        loader_log(
            inst, VK_DEBUG_REPORT_ERROR_BIT_EXT, 0,
            "vkGetPhysicalDeviceExternalImageFormatPropertiesNV called without"
            " NV_external_memory_capabilities extension being enabled");
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    } else {
        return disp->GetPhysicalDeviceExternalImageFormatPropertiesNV(
            unwrapped_phys_dev, format, type, tiling, usage, flags,
            externalHandleType, pExternalImageFormatProperties);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
terminator_GetPhysicalDeviceExternalImageFormatPropertiesNV(
    VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkExternalMemoryHandleTypeFlagsNV externalHandleType,
    VkExternalImageFormatPropertiesNV *pExternalImageFormatProperties) {
    struct loader_physical_device *phys_dev =
        (struct loader_physical_device *)physicalDevice;
    struct loader_icd *icd = phys_dev->this_icd;

    if (!icd->GetPhysicalDeviceExternalImageFormatPropertiesNV) {
        if (externalHandleType) {
            return VK_ERROR_FORMAT_NOT_SUPPORTED;
        }

        if (!icd->GetPhysicalDeviceImageFormatProperties) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        pExternalImageFormatProperties->externalMemoryFeatures = 0;
        pExternalImageFormatProperties->exportFromImportedHandleTypes = 0;
        pExternalImageFormatProperties->compatibleHandleTypes = 0;

        return icd->GetPhysicalDeviceImageFormatProperties(
            phys_dev->phys_dev, format, type, tiling, usage, flags,
            &pExternalImageFormatProperties->imageFormatProperties);
    }

    return icd->GetPhysicalDeviceExternalImageFormatPropertiesNV(
        phys_dev->phys_dev, format, type, tiling, usage, flags,
        externalHandleType, pExternalImageFormatProperties);
}

// Definitions for the VK_AMD_draw_indirect_count extension

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectCountAMD(
    VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
    VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
    uint32_t stride) {
    struct loader_dev_dispatch_table *disp =
        loader_get_dev_dispatch(commandBuffer);
    if (1 == disp->enabled_known_extensions.amd_draw_indirect_count &&
        NULL != disp->core_dispatch.CmdDrawIndirectCountAMD) {
        disp->core_dispatch.CmdDrawIndirectCountAMD(
            commandBuffer, buffer, offset, countBuffer, countBufferOffset,
            maxDrawCount, stride);
    }
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirectCountAMD(
    VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
    VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
    uint32_t stride) {
    struct loader_dev_dispatch_table *disp =
        loader_get_dev_dispatch(commandBuffer);
    if (1 == disp->enabled_known_extensions.amd_draw_indirect_count &&
        NULL != disp->core_dispatch.CmdDrawIndexedIndirectCountAMD) {
        disp->core_dispatch.CmdDrawIndexedIndirectCountAMD(
            commandBuffer, buffer, offset, countBuffer, countBufferOffset,
            maxDrawCount, stride);
    }
}

#ifdef VK_USE_PLATFORM_WIN32_KHR

// Definitions for the VK_NV_external_memory_win32 extension

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryWin32HandleNV(
    VkDevice device, VkDeviceMemory memory,
    VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE *pHandle) {
    struct loader_dev_dispatch_table *disp = loader_get_dev_dispatch(device);
    if (0 == disp->enabled_known_extensions.nv_external_memory_win32 ||
        NULL == disp->core_dispatch.GetMemoryWin32HandleNV) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    } else {
        return disp->core_dispatch.GetMemoryWin32HandleNV(device, memory,
                                                          handleType, pHandle);
    }
}

#endif // VK_USE_PLATFORM_WIN32_KHR

// GPA helpers for non-KHR extensions

bool extension_instance_gpa(struct loader_instance *ptr_instance,
                            const char *name, void **addr) {
    *addr = NULL;

    // Functions for the EXT_debug_marker extension

    if (!strcmp("vkDebugMarkerSetObjectTagEXT", name)) {
        *addr = (void *)vkDebugMarkerSetObjectTagEXT;
        return true;
    }
    if (!strcmp("vkDebugMarkerSetObjectNameEXT", name)) {
        *addr = (void *)vkDebugMarkerSetObjectNameEXT;
        return true;
    }
    if (!strcmp("vkCmdDebugMarkerBeginEXT", name)) {
        *addr = (void *)vkCmdDebugMarkerBeginEXT;
        return true;
    }
    if (!strcmp("vkCmdDebugMarkerEndEXT", name)) {
        *addr = (void *)vkCmdDebugMarkerEndEXT;
        return true;
    }
    if (!strcmp("vkCmdDebugMarkerInsertEXT", name)) {
        *addr = (void *)vkCmdDebugMarkerInsertEXT;
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

void extensions_create_device(struct loader_device *dev,
                              const VkDeviceCreateInfo *pCreateInfo) {
    dev->loader_dispatch.enabled_known_extensions.ext_debug_marker = 0;
    dev->loader_dispatch.enabled_known_extensions.amd_draw_indirect_count = 0;
    dev->loader_dispatch.enabled_known_extensions.nv_external_memory_win32 = 0;

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                   VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
            dev->loader_dispatch.enabled_known_extensions.ext_debug_marker = 1;
            return;
        }

        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                   VK_AMD_EXTENSION_DRAW_INDIRECT_COUNT_EXTENSION_NAME) == 0) {
            dev->loader_dispatch.enabled_known_extensions
                .amd_draw_indirect_count = 1;
            return;
        }

#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i],
                   VK_NV_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) == 0) {
            dev->loader_dispatch.enabled_known_extensions
                .nv_external_memory_win32 = 1;
            return;
        }
#endif // VK_USE_PLATFORM_WIN32_KHR
    }
}
