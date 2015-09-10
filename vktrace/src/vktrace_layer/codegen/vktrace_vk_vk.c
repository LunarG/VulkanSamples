/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
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

#include "vktrace_platform.h"
#include "vktrace_common.h"
#include "vktrace_lib_helpers.h"
#include "vktrace_vk_vk.h"
#include "vktrace_vk_vk_debug_report_lunarg.h"
#include "vktrace_vk_vk_debug_marker_lunarg.h"
#include "vktrace_vk_vk_ext_khr_swapchain.h"
#include "vktrace_vk_vk_ext_khr_device_swapchain.h"
#include "vktrace_interconnect.h"
#include "vktrace_filelike.h"
#include "vk_struct_size_helper.h"
#ifdef PLATFORM_LINUX
#include <pthread.h>
#endif
#include "vktrace_trace_packet_utils.h"
#include <stdio.h>

#ifdef WIN32
INIT_ONCE gInitOnce = INIT_ONCE_STATIC_INIT;
#elif defined(PLATFORM_LINUX)
pthread_once_t gInitOnce = PTHREAD_ONCE_INIT;
#endif

void send_vk_api_version_packet()
{
    packet_vkApiVersion* pPacket;
    vktrace_trace_packet_header* pHeader;
    pHeader = vktrace_create_trace_packet(VKTRACE_TID_VULKAN, VKTRACE_TPI_VK_vkApiVersion, sizeof(packet_vkApiVersion), 0);
    pPacket = interpret_body_as_vkApiVersion(pHeader);
    pPacket->version = VK_API_VERSION;
    vktrace_set_packet_entrypoint_end_time(pHeader);
    FINISH_TRACE_PACKET();
}

extern VKTRACE_CRITICAL_SECTION g_memInfoLock;
void InitTracer(void)
{
    const char *ipAddr = vktrace_get_global_var("VKTRACE_LIB_IPADDR");
    if (ipAddr == NULL)
        ipAddr = "127.0.0.1";
    gMessageStream = vktrace_MessageStream_create(FALSE, ipAddr, VKTRACE_BASE_PORT + VKTRACE_TID_VULKAN);
    vktrace_trace_set_trace_file(vktrace_FileLike_create_msg(gMessageStream));
    vktrace_tracelog_set_tracer_id(VKTRACE_TID_VULKAN);
    vktrace_create_critical_section(&g_memInfoLock);
    send_vk_api_version_packet();
}

// __HOOKED_vkCreateInstance is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyInstance(
    VkInstance instance)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyInstance* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyInstance, 0);
    g_instTable.DestroyInstance(instance);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyInstance(pHeader);
    pPacket->instance = instance;
    FINISH_TRACE_PACKET();
}

// __HOOKED_vkEnumeratePhysicalDevices is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceFeatures* pFeatures)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceFeatures* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceFeatures, sizeof(VkPhysicalDeviceFeatures));
    result = g_instTable.GetPhysicalDeviceFeatures(physicalDevice, pFeatures);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPhysicalDeviceFeatures(pHeader);
    pPacket->physicalDevice = physicalDevice;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFeatures), sizeof(VkPhysicalDeviceFeatures), pFeatures);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pFeatures));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkFormatProperties* pFormatProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceFormatProperties* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceFormatProperties, sizeof(VkFormatProperties));
    result = g_instTable.GetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPhysicalDeviceFormatProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    pPacket->format = format;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFormatProperties), sizeof(VkFormatProperties), pFormatProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pFormatProperties));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkImageCreateFlags flags,
    VkImageFormatProperties* pImageFormatProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceImageFormatProperties* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceImageFormatProperties, sizeof(VkImageFormatProperties));
    result = g_instTable.GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPhysicalDeviceImageFormatProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    pPacket->format = format;
    pPacket->type = type;
    pPacket->tiling = tiling;
    pPacket->usage = usage;
    pPacket->flags = flags;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pImageFormatProperties), sizeof(VkImageFormatProperties), pImageFormatProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pImageFormatProperties));
    FINISH_TRACE_PACKET();
    return result;
}

#if 0  // TODO move this to manually written
// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT PFN_vkVoidFunction VKAPI __HOOKED_vkGetInstanceProcAddr(
    VkInstance instance,
    const char* pName)
{
    vktrace_trace_packet_header* pHeader;
    PFN_vkVoidFunction result;
    packet_vkGetInstanceProcAddr* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetInstanceProcAddr, ((pName != NULL) ? strlen(pName) + 1 : 0));
    if (strcmp(pName, "vkGetPhysicalDeviceSurfaceSupportKHR") == 0) {
        real_vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)real_vkGetInstanceProcAddr(instance, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkGetPhysicalDeviceSurfaceSupportKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkGetPhysicalDeviceSurfaceSupportKHR;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkDbgCreateMsgCallback") == 0) {
        real_vkDbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback)real_vkGetInstanceProcAddr(instance, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkDbgCreateMsgCallback != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkDbgCreateMsgCallback;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkDbgDestroyMsgCallback") == 0) {
        real_vkDbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback)real_vkGetInstanceProcAddr(instance, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkDbgDestroyMsgCallback != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkDbgDestroyMsgCallback;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkCmdDbgMarkerBegin") == 0) {
        real_vkCmdDbgMarkerBegin = (PFN_vkCmdDbgMarkerBegin)real_vkGetInstanceProcAddr(instance, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkCmdDbgMarkerBegin != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkCmdDbgMarkerBegin;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkCmdDbgMarkerEnd") == 0) {
        real_vkCmdDbgMarkerEnd = (PFN_vkCmdDbgMarkerEnd)real_vkGetInstanceProcAddr(instance, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkCmdDbgMarkerEnd != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkCmdDbgMarkerEnd;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkDbgSetObjectTag") == 0) {
        real_vkDbgSetObjectTag = (PFN_vkDbgSetObjectTag)real_vkGetInstanceProcAddr(instance, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkDbgSetObjectTag != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkDbgSetObjectTag;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkDbgSetObjectName") == 0) {
        real_vkDbgSetObjectName = (PFN_vkDbgSetObjectName)real_vkGetInstanceProcAddr(instance, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkDbgSetObjectName != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkDbgSetObjectName;
        } else {
            result = NULL;
        }
    }
    pPacket = interpret_body_as_vkGetInstanceProcAddr(pHeader);
    pPacket->instance = instance;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pName), ((pName != NULL) ? strlen(pName) + 1 : 0), pName);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pName));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT PFN_vkVoidFunction VKAPI __HOOKED_vkGetDeviceProcAddr(
    VkDevice device,
    const char* pName)
{
    vktrace_trace_packet_header* pHeader;
    PFN_vkVoidFunction result;
    packet_vkGetDeviceProcAddr* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetDeviceProcAddr, ((pName != NULL) ? strlen(pName) + 1 : 0));
    if (strcmp(pName, "vkGetPhysicalDeviceSurfaceSupportKHR") == 0) {
        real_vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)real_vkGetDeviceProcAddr(device, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkGetPhysicalDeviceSurfaceSupportKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkGetPhysicalDeviceSurfaceSupportKHR;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkGetSurfacePropertiesKHR") == 0) {
        real_vkGetSurfacePropertiesKHR = (PFN_vkGetSurfacePropertiesKHR)real_vkGetDeviceProcAddr(device, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkGetSurfacePropertiesKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkGetSurfacePropertiesKHR;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkGetSurfaceFormatsKHR") == 0) {
        real_vkGetSurfaceFormatsKHR = (PFN_vkGetSurfaceFormatsKHR)real_vkGetDeviceProcAddr(device, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkGetSurfaceFormatsKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkGetSurfaceFormatsKHR;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkGetSurfacePresentModesKHR") == 0) {
        real_vkGetSurfacePresentModesKHR = (PFN_vkGetSurfacePresentModesKHR)real_vkGetDeviceProcAddr(device, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkGetSurfacePresentModesKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkGetSurfacePresentModesKHR;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkCreateSwapchainKHR") == 0) {
        real_vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)real_vkGetDeviceProcAddr(device, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkCreateSwapchainKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkCreateSwapchainKHR;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkDestroySwapchainKHR") == 0) {
        real_vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)real_vkGetDeviceProcAddr(device, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkDestroySwapchainKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkDestroySwapchainKHR;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkGetSwapchainImagesKHR") == 0) {
        real_vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)real_vkGetDeviceProcAddr(device, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkGetSwapchainImagesKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkGetSwapchainImagesKHR;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkAcquireNextImageKHR") == 0) {
        real_vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)real_vkGetDeviceProcAddr(device, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkAcquireNextImageKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkAcquireNextImageKHR;
        } else {
            result = NULL;
        }
    }
    if (strcmp(pName, "vkQueuePresentKHR") == 0) {
        real_vkQueuePresentKHR = (PFN_vkQueuePresentKHR)real_vkGetDeviceProcAddr(device, pName);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        if (real_vkQueuePresentKHR != NULL) {
            result = (PFN_vkVoidFunction)__HOOKED_vkQueuePresentKHR;
        } else {
            result = NULL;
        }
    }
    pPacket = interpret_body_as_vkGetDeviceProcAddr(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pName), ((pName != NULL) ? strlen(pName) + 1 : 0), pName);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pName));
    FINISH_TRACE_PACKET();
    return result;
}
#endif

// __HOOKED_vkCreateDevice is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDevice(
    VkDevice device)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyDevice* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyDevice, 0);
    g_devTable.DestroyDevice(device);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyDevice(pHeader);
    pPacket->device = device;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceProperties* pProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceProperties* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceProperties, sizeof(VkPhysicalDeviceProperties));
    result = g_instTable.GetPhysicalDeviceProperties(physicalDevice, pProperties);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPhysicalDeviceProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), sizeof(VkPhysicalDeviceProperties), pProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_vkGetPhysicalDeviceQueueFamilyProperties is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceMemoryProperties* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceMemoryProperties, sizeof(VkPhysicalDeviceMemoryProperties));
    result = g_instTable.GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPhysicalDeviceMemoryProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMemoryProperties), sizeof(VkPhysicalDeviceMemoryProperties), pMemoryProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemoryProperties));
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_vkGetGlobalExtensionProperties is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkGetPhysicalDeviceExtensionProperties is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkGetGlobalLayerProperties is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkGetPhysicalDeviceLayerProperties is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetDeviceQueue(
    VkDevice device,
    uint32_t queueFamilyIndex,
    uint32_t queueIndex,
    VkQueue* pQueue)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetDeviceQueue* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetDeviceQueue, sizeof(VkQueue));
    result = g_devTable.GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetDeviceQueue(pHeader);
    pPacket->device = device;
    pPacket->queueFamilyIndex = queueFamilyIndex;
    pPacket->queueIndex = queueIndex;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pQueue), sizeof(VkQueue), pQueue);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pQueue));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueSubmit(
    VkQueue queue,
    uint32_t cmdBufferCount,
    const VkCmdBuffer* pCmdBuffers,
    VkFence fence)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkQueueSubmit* pPacket = NULL;
    CREATE_TRACE_PACKET(vkQueueSubmit, cmdBufferCount*sizeof(VkCmdBuffer));
    result = g_devTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkQueueSubmit(pHeader);
    pPacket->queue = queue;
    pPacket->cmdBufferCount = cmdBufferCount;
    pPacket->fence = fence;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCmdBuffers), cmdBufferCount*sizeof(VkCmdBuffer), pCmdBuffers);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCmdBuffers));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueWaitIdle(
    VkQueue queue)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkQueueWaitIdle* pPacket = NULL;
    CREATE_TRACE_PACKET(vkQueueWaitIdle, 0);
    result = g_devTable.QueueWaitIdle(queue);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkQueueWaitIdle(pHeader);
    pPacket->queue = queue;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkDeviceWaitIdle(
    VkDevice device)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkDeviceWaitIdle* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDeviceWaitIdle, 0);
    result = g_devTable.DeviceWaitIdle(device);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDeviceWaitIdle(pHeader);
    pPacket->device = device;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_vkAllocMemory is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkFreeMemory is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkMapMemory is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkUnmapMemory is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkFlushMappedMemoryRanges is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkInvalidateMappedMemoryRanges(
    VkDevice device,
    uint32_t memRangeCount,
    const VkMappedMemoryRange* pMemRanges)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkInvalidateMappedMemoryRanges* pPacket = NULL;
    CREATE_TRACE_PACKET(vkInvalidateMappedMemoryRanges, memRangeCount*sizeof(VkMappedMemoryRange));
    result = g_devTable.InvalidateMappedMemoryRanges(device, memRangeCount, pMemRanges);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkInvalidateMappedMemoryRanges(pHeader);
    pPacket->device = device;
    pPacket->memRangeCount = memRangeCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMemRanges), memRangeCount*sizeof(VkMappedMemoryRange), pMemRanges);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemRanges));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetDeviceMemoryCommitment(
    VkDevice device,
    VkDeviceMemory memory,
    VkDeviceSize* pCommittedMemoryInBytes)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetDeviceMemoryCommitment* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetDeviceMemoryCommitment, sizeof(VkDeviceSize));
    result = g_devTable.GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetDeviceMemoryCommitment(pHeader);
    pPacket->device = device;
    pPacket->memory = memory;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCommittedMemoryInBytes), sizeof(VkDeviceSize), pCommittedMemoryInBytes);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCommittedMemoryInBytes));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkBindBufferMemory(
    VkDevice device,
    VkBuffer buffer,
    VkDeviceMemory mem,
    VkDeviceSize memOffset)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkBindBufferMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkBindBufferMemory, 0);
    result = g_devTable.BindBufferMemory(device, buffer, mem, memOffset);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkBindBufferMemory(pHeader);
    pPacket->device = device;
    pPacket->buffer = buffer;
    pPacket->mem = mem;
    pPacket->memOffset = memOffset;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkBindImageMemory(
    VkDevice device,
    VkImage image,
    VkDeviceMemory mem,
    VkDeviceSize memOffset)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkBindImageMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkBindImageMemory, 0);
    result = g_devTable.BindImageMemory(device, image, mem, memOffset);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkBindImageMemory(pHeader);
    pPacket->device = device;
    pPacket->image = image;
    pPacket->mem = mem;
    pPacket->memOffset = memOffset;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetBufferMemoryRequirements(
    VkDevice device,
    VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetBufferMemoryRequirements* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetBufferMemoryRequirements, sizeof(VkMemoryRequirements));
    result = g_devTable.GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetBufferMemoryRequirements(pHeader);
    pPacket->device = device;
    pPacket->buffer = buffer;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMemoryRequirements), sizeof(VkMemoryRequirements), pMemoryRequirements);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemoryRequirements));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetImageMemoryRequirements(
    VkDevice device,
    VkImage image,
    VkMemoryRequirements* pMemoryRequirements)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetImageMemoryRequirements* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetImageMemoryRequirements, sizeof(VkMemoryRequirements));
    result = g_devTable.GetImageMemoryRequirements(device, image, pMemoryRequirements);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetImageMemoryRequirements(pHeader);
    pPacket->device = device;
    pPacket->image = image;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMemoryRequirements), sizeof(VkMemoryRequirements), pMemoryRequirements);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemoryRequirements));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetImageSparseMemoryRequirements(
    VkDevice device,
    VkImage image,
    uint32_t* pNumRequirements,
    VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetImageSparseMemoryRequirements* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetImageSparseMemoryRequirements, sizeof(uint32_t) + sizeof(VkSparseImageMemoryRequirements));
    result = g_devTable.GetImageSparseMemoryRequirements(device, image, pNumRequirements, pSparseMemoryRequirements);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetImageSparseMemoryRequirements(pHeader);
    pPacket->device = device;
    pPacket->image = image;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pNumRequirements), sizeof(uint32_t), pNumRequirements);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSparseMemoryRequirements), (*pNumRequirements) * sizeof(VkSparseImageMemoryRequirements), pSparseMemoryRequirements);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pNumRequirements));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSparseMemoryRequirements));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice physicalDevice,
    VkFormat format,
    VkImageType type,
    uint32_t samples,
    VkImageUsageFlags usage,
    VkImageTiling tiling,
    uint32_t* pNumProperties,
    VkSparseImageFormatProperties* pProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceSparseImageFormatProperties* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceSparseImageFormatProperties, sizeof(uint32_t) + sizeof(VkSparseImageFormatProperties));
    result = g_instTable.GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pNumProperties, pProperties);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPhysicalDeviceSparseImageFormatProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    pPacket->format = format;
    pPacket->type = type;
    pPacket->samples = samples;
    pPacket->usage = usage;
    pPacket->tiling = tiling;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pNumProperties), sizeof(uint32_t), pNumProperties);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), (*pNumProperties) * sizeof(VkSparseImageFormatProperties), pProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pNumProperties));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueBindSparseBufferMemory(
    VkQueue queue,
    VkBuffer buffer,
    uint32_t numBindings,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkQueueBindSparseBufferMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkQueueBindSparseBufferMemory, sizeof(VkSparseMemoryBindInfo));
    result = g_devTable.QueueBindSparseBufferMemory(queue, buffer, numBindings, pBindInfo);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkQueueBindSparseBufferMemory(pHeader);
    pPacket->queue = queue;
    pPacket->buffer = buffer;
    pPacket->numBindings = numBindings;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBindInfo), numBindings * sizeof(VkSparseMemoryBindInfo), pBindInfo);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pBindInfo));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueBindSparseImageOpaqueMemory(
    VkQueue queue,
    VkImage image,
    uint32_t numBindings,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkQueueBindSparseImageOpaqueMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkQueueBindSparseImageOpaqueMemory, sizeof(VkSparseMemoryBindInfo));
    result = g_devTable.QueueBindSparseImageOpaqueMemory(queue, image, numBindings, pBindInfo);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkQueueBindSparseImageOpaqueMemory(pHeader);
    pPacket->queue = queue;
    pPacket->image = image;
    pPacket->numBindings = numBindings;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBindInfo), numBindings * sizeof(VkSparseMemoryBindInfo), pBindInfo);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pBindInfo));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueBindSparseImageMemory(
    VkQueue queue,
    VkImage image,
    uint32_t numBindings,
    const VkSparseImageMemoryBindInfo* pBindInfo)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkQueueBindSparseImageMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkQueueBindSparseImageMemory, sizeof(VkSparseImageMemoryBindInfo));
    result = g_devTable.QueueBindSparseImageMemory(queue, image, numBindings, pBindInfo);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkQueueBindSparseImageMemory(pHeader);
    pPacket->queue = queue;
    pPacket->image = image;
    pPacket->numBindings = numBindings;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBindInfo), numBindings * sizeof(VkSparseImageMemoryBindInfo), pBindInfo);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pBindInfo));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateFence(
    VkDevice device,
    const VkFenceCreateInfo* pCreateInfo,
    VkFence* pFence)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateFence* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateFence, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkFence));
    result = g_devTable.CreateFence(device, pCreateInfo, pFence);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateFence(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkFenceCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFence), sizeof(VkFence), pFence);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pFence));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyFence(
    VkDevice device,
    VkFence fence)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyFence* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyFence, 0);
    g_devTable.DestroyFence(device, fence);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyFence(pHeader);
    pPacket->device = device;
    pPacket->fence = fence;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetFences(
    VkDevice device,
    uint32_t fenceCount,
    const VkFence* pFences)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkResetFences* pPacket = NULL;
    CREATE_TRACE_PACKET(vkResetFences, fenceCount*sizeof(VkFence));
    result = g_devTable.ResetFences(device, fenceCount, pFences);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkResetFences(pHeader);
    pPacket->device = device;
    pPacket->fenceCount = fenceCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFences), fenceCount*sizeof(VkFence), pFences);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pFences));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetFenceStatus(
    VkDevice device,
    VkFence fence)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetFenceStatus* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetFenceStatus, 0);
    result = g_devTable.GetFenceStatus(device, fence);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetFenceStatus(pHeader);
    pPacket->device = device;
    pPacket->fence = fence;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkWaitForFences(
    VkDevice device,
    uint32_t fenceCount,
    const VkFence* pFences,
    VkBool32 waitAll,
    uint64_t timeout)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkWaitForFences* pPacket = NULL;
    CREATE_TRACE_PACKET(vkWaitForFences, fenceCount*sizeof(VkFence));
    result = g_devTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkWaitForFences(pHeader);
    pPacket->device = device;
    pPacket->fenceCount = fenceCount;
    pPacket->waitAll = waitAll;
    pPacket->timeout = timeout;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFences), fenceCount*sizeof(VkFence), pFences);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pFences));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateSemaphore(
    VkDevice device,
    const VkSemaphoreCreateInfo* pCreateInfo,
    VkSemaphore* pSemaphore)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateSemaphore* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateSemaphore, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkSemaphore));
    result = g_devTable.CreateSemaphore(device, pCreateInfo, pSemaphore);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateSemaphore(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkSemaphoreCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSemaphore), sizeof(VkSemaphore), pSemaphore);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSemaphore));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroySemaphore(
    VkDevice device,
    VkSemaphore semaphore)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroySemaphore* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroySemaphore, 0);
    g_devTable.DestroySemaphore(device, semaphore);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroySemaphore(pHeader);
    pPacket->device = device;
    pPacket->semaphore = semaphore;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueSignalSemaphore(
    VkQueue queue,
    VkSemaphore semaphore)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkQueueSignalSemaphore* pPacket = NULL;
    CREATE_TRACE_PACKET(vkQueueSignalSemaphore, 0);
    result = g_devTable.QueueSignalSemaphore(queue, semaphore);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkQueueSignalSemaphore(pHeader);
    pPacket->queue = queue;
    pPacket->semaphore = semaphore;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueWaitSemaphore(
    VkQueue queue,
    VkSemaphore semaphore)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkQueueWaitSemaphore* pPacket = NULL;
    CREATE_TRACE_PACKET(vkQueueWaitSemaphore, 0);
    result = g_devTable.QueueWaitSemaphore(queue, semaphore);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkQueueWaitSemaphore(pHeader);
    pPacket->queue = queue;
    pPacket->semaphore = semaphore;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateEvent(
    VkDevice device,
    const VkEventCreateInfo* pCreateInfo,
    VkEvent* pEvent)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateEvent, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkEvent));
    result = g_devTable.CreateEvent(device, pCreateInfo, pEvent);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateEvent(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkEventCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pEvent), sizeof(VkEvent), pEvent);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pEvent));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyEvent(
    VkDevice device,
    VkEvent event)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyEvent, 0);
    g_devTable.DestroyEvent(device, event);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyEvent(pHeader);
    pPacket->device = device;
    pPacket->event = event;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetEventStatus(
    VkDevice device,
    VkEvent event)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetEventStatus* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetEventStatus, 0);
    result = g_devTable.GetEventStatus(device, event);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetEventStatus(pHeader);
    pPacket->device = device;
    pPacket->event = event;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkSetEvent(
    VkDevice device,
    VkEvent event)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkSetEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(vkSetEvent, 0);
    result = g_devTable.SetEvent(device, event);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkSetEvent(pHeader);
    pPacket->device = device;
    pPacket->event = event;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetEvent(
    VkDevice device,
    VkEvent event)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkResetEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(vkResetEvent, 0);
    result = g_devTable.ResetEvent(device, event);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkResetEvent(pHeader);
    pPacket->device = device;
    pPacket->event = event;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateQueryPool(
    VkDevice device,
    const VkQueryPoolCreateInfo* pCreateInfo,
    VkQueryPool* pQueryPool)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateQueryPool* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateQueryPool, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkQueryPool));
    result = g_devTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateQueryPool(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkQueryPoolCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pQueryPool), sizeof(VkQueryPool), pQueryPool);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pQueryPool));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyQueryPool(
    VkDevice device,
    VkQueryPool queryPool)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyQueryPool* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyQueryPool, 0);
    g_devTable.DestroyQueryPool(device, queryPool);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyQueryPool(pHeader);
    pPacket->device = device;
    pPacket->queryPool = queryPool;
    FINISH_TRACE_PACKET();

}

// __HOOKED_vkGetQueryPoolResults is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateBuffer(
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo,
    VkBuffer* pBuffer)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateBuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkBuffer));
    result = g_devTable.CreateBuffer(device, pCreateInfo, pBuffer);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateBuffer(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkBufferCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBuffer), sizeof(VkBuffer), pBuffer);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pBuffer));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyBuffer(
    VkDevice device,
    VkBuffer buffer)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyBuffer, 0);
    g_devTable.DestroyBuffer(device, buffer);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyBuffer(pHeader);
    pPacket->device = device;
    pPacket->buffer = buffer;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateBufferView(
    VkDevice device,
    const VkBufferViewCreateInfo* pCreateInfo,
    VkBufferView* pView)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateBufferView* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateBufferView, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkBufferView));
    result = g_devTable.CreateBufferView(device, pCreateInfo, pView);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateBufferView(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkBufferViewCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pView), sizeof(VkBufferView), pView);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pView));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyBufferView(
    VkDevice device,
    VkBufferView bufferView)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyBufferView* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyBufferView, 0);
    g_devTable.DestroyBufferView(device, bufferView);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyBufferView(pHeader);
    pPacket->device = device;
    pPacket->bufferView = bufferView;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateImage(
    VkDevice device,
    const VkImageCreateInfo* pCreateInfo,
    VkImage* pImage)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateImage* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateImage, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkImage));
    result = g_devTable.CreateImage(device, pCreateInfo, pImage);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateImage(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkImageCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pImage), sizeof(VkImage), pImage);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pImage));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyImage(
    VkDevice device,
    VkImage image)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyImage* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyImage, 0);
    g_devTable.DestroyImage(device, image);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyImage(pHeader);
    pPacket->device = device;
    pPacket->image = image;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetImageSubresourceLayout(
    VkDevice device,
    VkImage image,
    const VkImageSubresource* pSubresource,
    VkSubresourceLayout* pLayout)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetImageSubresourceLayout* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetImageSubresourceLayout, sizeof(VkImageSubresource) + sizeof(VkSubresourceLayout));
    result = g_devTable.GetImageSubresourceLayout(device, image, pSubresource, pLayout);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetImageSubresourceLayout(pHeader);
    pPacket->device = device;
    pPacket->image = image;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSubresource), sizeof(VkImageSubresource), pSubresource);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pLayout), sizeof(VkSubresourceLayout), pLayout);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSubresource));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pLayout));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo,
    VkImageView* pView)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateImageView* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateImageView, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkImageView));
    result = g_devTable.CreateImageView(device, pCreateInfo, pView);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateImageView(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkImageViewCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pView), sizeof(VkImageView), pView);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pView));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyImageView(
    VkDevice device,
    VkImageView imageView)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyImageView* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyImageView, 0);
    g_devTable.DestroyImageView(device, imageView);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyImageView(pHeader);
    pPacket->device = device;
    pPacket->imageView = imageView;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateShaderModule(
    VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    VkShaderModule* pShaderModule)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateShaderModule* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateShaderModule, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkShaderModule));
    result = g_devTable.CreateShaderModule(device, pCreateInfo, pShaderModule);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateShaderModule(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkShaderModuleCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pCode), pPacket->pCreateInfo->codeSize, pCreateInfo->pCode);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pShaderModule), sizeof(VkShaderModule), pShaderModule);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pCode));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pShaderModule));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyShaderModule(
    VkDevice device,
    VkShaderModule shaderModule)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyShaderModule* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyShaderModule, 0);
    g_devTable.DestroyShaderModule(device, shaderModule);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyShaderModule(pHeader);
    pPacket->device = device;
    pPacket->shaderModule = shaderModule;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateShader(
    VkDevice device,
    const VkShaderCreateInfo* pCreateInfo,
    VkShader* pShader)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateShader* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateShader, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkShader));
    result = g_devTable.CreateShader(device, pCreateInfo, pShader);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateShader(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkShaderModuleCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pName), strlen(pPacket->pCreateInfo->pName), pCreateInfo->pName);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pShader), sizeof(VkShader), pShader);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pName));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pShader));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyShader(
    VkDevice device,
    VkShader shader)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyShader* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyShader, 0);
    g_devTable.DestroyShader(device, shader);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyShader(pHeader);
    pPacket->device = device;
    pPacket->shader = shader;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreatePipelineCache(
    VkDevice device,
    const VkPipelineCacheCreateInfo* pCreateInfo,
    VkPipelineCache* pPipelineCache)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreatePipelineCache* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreatePipelineCache, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkPipelineCache));
    result = g_devTable.CreatePipelineCache(device, pCreateInfo, pPipelineCache);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreatePipelineCache(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkPipelineCacheCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipelineCache), sizeof(VkPipelineCache), pPipelineCache);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipelineCache));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyPipelineCache(
    VkDevice device,
    VkPipelineCache pipelineCache)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyPipelineCache* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyPipelineCache, 0);
    g_devTable.DestroyPipelineCache(device, pipelineCache);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyPipelineCache(pHeader);
    pPacket->device = device;
    pPacket->pipelineCache = pipelineCache;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT size_t VKAPI __HOOKED_vkGetPipelineCacheSize(
    VkDevice device,
    VkPipelineCache pipelineCache)
{
    vktrace_trace_packet_header* pHeader;
    size_t result;
    packet_vkGetPipelineCacheSize* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPipelineCacheSize, 0);
    result = g_devTable.GetPipelineCacheSize(device, pipelineCache);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPipelineCacheSize(pHeader);
    pPacket->device = device;
    pPacket->pipelineCache = pipelineCache;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPipelineCacheData(
    VkDevice device,
    VkPipelineCache pipelineCache,
    void* pData)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPipelineCacheData* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPipelineCacheData, sizeof(void*));
    result = g_devTable.GetPipelineCacheData(device, pipelineCache, pData);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPipelineCacheData(pHeader);
    pPacket->device = device;
    pPacket->pipelineCache = pipelineCache;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), sizeof(void), pData);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkMergePipelineCaches(
    VkDevice device,
    VkPipelineCache destCache,
    uint32_t srcCacheCount,
    const VkPipelineCache* pSrcCaches)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkMergePipelineCaches* pPacket = NULL;
    CREATE_TRACE_PACKET(vkMergePipelineCaches, srcCacheCount*sizeof(VkPipelineCache));
    result = g_devTable.MergePipelineCaches(device, destCache, srcCacheCount, pSrcCaches);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkMergePipelineCaches(pHeader);
    pPacket->device = device;
    pPacket->destCache = destCache;
    pPacket->srcCacheCount = srcCacheCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSrcCaches), srcCacheCount*sizeof(VkPipelineCache), pSrcCaches);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSrcCaches));
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_vkCreateGraphicsPipelines is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkCreateComputePipelines is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyPipeline(
    VkDevice device,
    VkPipeline pipeline)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyPipeline* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyPipeline, 0);
    g_devTable.DestroyPipeline(device, pipeline);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyPipeline(pHeader);
    pPacket->device = device;
    pPacket->pipeline = pipeline;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreatePipelineLayout(
    VkDevice device,
    const VkPipelineLayoutCreateInfo* pCreateInfo,
    VkPipelineLayout* pPipelineLayout)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreatePipelineLayout* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreatePipelineLayout, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkPipelineLayout));
    result = g_devTable.CreatePipelineLayout(device, pCreateInfo, pPipelineLayout);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreatePipelineLayout(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkPipelineLayoutCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pSetLayouts), pCreateInfo->descriptorSetCount * sizeof(VkDescriptorSetLayout), pCreateInfo->pSetLayouts);;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipelineLayout), sizeof(VkPipelineLayout), pPipelineLayout);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pSetLayouts));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipelineLayout));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyPipelineLayout(
    VkDevice device,
    VkPipelineLayout pipelineLayout)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyPipelineLayout* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyPipelineLayout, 0);
    g_devTable.DestroyPipelineLayout(device, pipelineLayout);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyPipelineLayout(pHeader);
    pPacket->device = device;
    pPacket->pipelineLayout = pipelineLayout;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateSampler(
    VkDevice device,
    const VkSamplerCreateInfo* pCreateInfo,
    VkSampler* pSampler)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateSampler* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateSampler, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkSampler));
    result = g_devTable.CreateSampler(device, pCreateInfo, pSampler);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateSampler(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkSamplerCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSampler), sizeof(VkSampler), pSampler);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSampler));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroySampler(
    VkDevice device,
    VkSampler sampler)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroySampler* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroySampler, 0);
    g_devTable.DestroySampler(device, sampler);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroySampler(pHeader);
    pPacket->device = device;
    pPacket->sampler = sampler;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDescriptorSetLayout(
    VkDevice device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    VkDescriptorSetLayout* pSetLayout)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDescriptorSetLayout* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateDescriptorSetLayout, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDescriptorSetLayout));
    result = g_devTable.CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDescriptorSetLayout(pHeader);
    pPacket->device = device;
    add_create_ds_layout_to_trace_packet(pHeader, &pPacket->pCreateInfo, pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayout), sizeof(VkDescriptorSetLayout), pSetLayout);
    pPacket->result = result;
    // pCreateInfo finalized in add_create_ds_layout_to_trace_packet;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetLayout));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDescriptorSetLayout(
    VkDevice device,
    VkDescriptorSetLayout descriptorSetLayout)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyDescriptorSetLayout* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyDescriptorSetLayout, 0);
    g_devTable.DestroyDescriptorSetLayout(device, descriptorSetLayout);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyDescriptorSetLayout(pHeader);
    pPacket->device = device;
    pPacket->descriptorSetLayout = descriptorSetLayout;
    FINISH_TRACE_PACKET();
}

// __HOOKED_vkCreateDescriptorPool is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyDescriptorPool* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyDescriptorPool, 0);
    g_devTable.DestroyDescriptorPool(device, descriptorPool);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyDescriptorPool(pHeader);
    pPacket->device = device;
    pPacket->descriptorPool = descriptorPool;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetDescriptorPool(
    VkDevice device,
    VkDescriptorPool descriptorPool)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkResetDescriptorPool* pPacket = NULL;
    CREATE_TRACE_PACKET(vkResetDescriptorPool, 0);
    result = g_devTable.ResetDescriptorPool(device, descriptorPool);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkResetDescriptorPool(pHeader);
    pPacket->device = device;
    pPacket->descriptorPool = descriptorPool;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_vkAllocDescriptorSets is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkFreeDescriptorSets is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkUpdateDescriptorSets is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkCreateDynamicViewportState is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicViewportState(
    VkDevice device,
    VkDynamicViewportState dynamicViewportState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyDynamicViewportState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyDynamicViewportState, 0);
    g_devTable.DestroyDynamicViewportState(device, dynamicViewportState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyDynamicViewportState(pHeader);
    pPacket->device = device;
    pPacket->dynamicViewportState = dynamicViewportState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicLineWidthState(
    VkDevice device,
    const VkDynamicLineWidthStateCreateInfo* pCreateInfo,
    VkDynamicLineWidthState* pState)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDynamicLineWidthState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateDynamicLineWidthState, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDynamicLineWidthState));
    result = g_devTable.CreateDynamicLineWidthState(device, pCreateInfo, pState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDynamicLineWidthState(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDynamicLineWidthStateCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(VkDynamicLineWidthState), pState);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicLineWidthState(
    VkDevice device,
    VkDynamicLineWidthState dynamicLineWidthState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyDynamicLineWidthState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyDynamicLineWidthState, 0);
    g_devTable.DestroyDynamicLineWidthState(device, dynamicLineWidthState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyDynamicLineWidthState(pHeader);
    pPacket->device = device;
    pPacket->dynamicLineWidthState = dynamicLineWidthState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicDepthBiasState(
    VkDevice device,
    const VkDynamicDepthBiasStateCreateInfo* pCreateInfo,
    VkDynamicDepthBiasState* pState)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDynamicDepthBiasState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateDynamicDepthBiasState, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDynamicDepthBiasState));
    result = g_devTable.CreateDynamicDepthBiasState(device, pCreateInfo, pState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDynamicDepthBiasState(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDynamicDepthBiasStateCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(VkDynamicDepthBiasState), pState);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicDepthBiasState(
    VkDevice device,
    VkDynamicDepthBiasState dynamicDepthBiasState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyDynamicDepthBiasState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyDynamicDepthBiasState, 0);
    g_devTable.DestroyDynamicDepthBiasState(device, dynamicDepthBiasState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyDynamicDepthBiasState(pHeader);
    pPacket->device = device;
    pPacket->dynamicDepthBiasState = dynamicDepthBiasState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicBlendState(
    VkDevice device,
    const VkDynamicBlendStateCreateInfo* pCreateInfo,
    VkDynamicBlendState* pState)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDynamicBlendState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateDynamicBlendState, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDynamicBlendState));
    result = g_devTable.CreateDynamicBlendState(device, pCreateInfo, pState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDynamicBlendState(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDynamicBlendStateCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(VkDynamicBlendState), pState);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicBlendState(
    VkDevice device,
    VkDynamicBlendState DynamicBlendState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyDynamicBlendState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyDynamicBlendState, 0);
    g_devTable.DestroyDynamicBlendState(device, DynamicBlendState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyDynamicBlendState(pHeader);
    pPacket->device = device;
    pPacket->DynamicBlendState = DynamicBlendState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicDepthBoundsState(
    VkDevice device,
    const VkDynamicDepthBoundsStateCreateInfo* pCreateInfo,
    VkDynamicDepthBoundsState* pState)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDynamicDepthBoundsState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateDynamicDepthBoundsState, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDynamicDepthBoundsState));
    result = g_devTable.CreateDynamicDepthBoundsState(device, pCreateInfo, pState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDynamicDepthBoundsState(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDynamicDepthBoundsStateCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(VkDynamicDepthBoundsState), pState);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicDepthBoundsState(
    VkDevice device,
    VkDynamicDepthBoundsState dynamicDepthBoundsState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyDynamicDepthBoundsState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyDynamicDepthBoundsState, 0);
    g_devTable.DestroyDynamicDepthBoundsState(device, dynamicDepthBoundsState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyDynamicDepthBoundsState(pHeader);
    pPacket->device = device;
    pPacket->dynamicDepthBoundsState = dynamicDepthBoundsState;
    FINISH_TRACE_PACKET();
}

// __HOOKED_vkCreateDynamicStencilState is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicStencilState(
    VkDevice device,
    VkDynamicStencilState dynamicStencilState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyDynamicStencilState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyDynamicStencilState, 0);
    g_devTable.DestroyDynamicStencilState(device, dynamicStencilState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyDynamicStencilState(pHeader);
    pPacket->device = device;
    pPacket->dynamicStencilState = dynamicStencilState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateCommandPool(
    VkDevice device,
    const VkCmdPoolCreateInfo* pCreateInfo,
    VkCmdPool* pCmdPool)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateCommandPool* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateCommandPool, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkCmdPool));
    result = g_devTable.CreateCommandPool(device, pCreateInfo, pCmdPool);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateCommandPool(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkCmdPoolCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCmdPool), sizeof(VkCmdPool), pCmdPool);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCmdPool));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyCommandPool(
    VkDevice device,
    VkCmdPool cmdPool)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyCommandPool* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyCommandPool, 0);
    g_devTable.DestroyCommandPool(device, cmdPool);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyCommandPool(pHeader);
    pPacket->device = device;
    pPacket->cmdPool = cmdPool;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetCommandPool(
    VkDevice device,
    VkCmdPool cmdPool,
    VkCmdPoolResetFlags flags)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkResetCommandPool* pPacket = NULL;
    CREATE_TRACE_PACKET(vkResetCommandPool, 0);
    result = g_devTable.ResetCommandPool(device, cmdPool, flags);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkResetCommandPool(pHeader);
    pPacket->device = device;
    pPacket->cmdPool = cmdPool;
    pPacket->flags = flags;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateCommandBuffer(
    VkDevice device,
    const VkCmdBufferCreateInfo* pCreateInfo,
    VkCmdBuffer* pCmdBuffer)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateCommandBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateCommandBuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkCmdBuffer));
    result = g_devTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateCommandBuffer(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkCmdBufferCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCmdBuffer), sizeof(VkCmdBuffer), pCmdBuffer);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCmdBuffer));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyCommandBuffer(
    VkDevice device,
    VkCmdBuffer commandBuffer)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyCommandBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyCommandBuffer, 0);
    g_devTable.DestroyCommandBuffer(device, commandBuffer);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyCommandBuffer(pHeader);
    pPacket->device = device;
    pPacket->commandBuffer = commandBuffer;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkBeginCommandBuffer(
    VkCmdBuffer cmdBuffer,
    const VkCmdBufferBeginInfo* pBeginInfo)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkBeginCommandBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkBeginCommandBuffer, get_struct_chain_size((void*)pBeginInfo));
    result = g_devTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkBeginCommandBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBeginInfo), sizeof(VkCmdBufferBeginInfo), pBeginInfo);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pBeginInfo));
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkEndCommandBuffer(
    VkCmdBuffer cmdBuffer)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkEndCommandBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkEndCommandBuffer, 0);
    result = g_devTable.EndCommandBuffer(cmdBuffer);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkEndCommandBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetCommandBuffer(
    VkCmdBuffer cmdBuffer,
    VkCmdBufferResetFlags flags)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkResetCommandBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkResetCommandBuffer, 0);
    result = g_devTable.ResetCommandBuffer(cmdBuffer, flags);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkResetCommandBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->flags = flags;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindPipeline(
    VkCmdBuffer cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline pipeline)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindPipeline* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindPipeline, 0);
    g_devTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindPipeline(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->pipeline = pipeline;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicViewportState(
    VkCmdBuffer cmdBuffer,
    VkDynamicViewportState dynamicViewportState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindDynamicViewportState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindDynamicViewportState, 0);
    g_devTable.CmdBindDynamicViewportState(cmdBuffer, dynamicViewportState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindDynamicViewportState(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->dynamicViewportState = dynamicViewportState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicLineWidthState(
    VkCmdBuffer cmdBuffer,
    VkDynamicLineWidthState dynamicLineWidthState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindDynamicLineWidthState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindDynamicLineWidthState, 0);
    g_devTable.CmdBindDynamicLineWidthState(cmdBuffer, dynamicLineWidthState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindDynamicLineWidthState(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->dynamicLineWidthState = dynamicLineWidthState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicDepthBiasState(
    VkCmdBuffer cmdBuffer,
    VkDynamicDepthBiasState dynamicDepthBiasState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindDynamicDepthBiasState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindDynamicDepthBiasState, 0);
    g_devTable.CmdBindDynamicDepthBiasState(cmdBuffer, dynamicDepthBiasState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindDynamicDepthBiasState(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->dynamicDepthBiasState = dynamicDepthBiasState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicBlendState(
    VkCmdBuffer cmdBuffer,
    VkDynamicBlendState DynamicBlendState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindDynamicBlendState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindDynamicBlendState, 0);
    g_devTable.CmdBindDynamicBlendState(cmdBuffer, DynamicBlendState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindDynamicBlendState(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->DynamicBlendState = DynamicBlendState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicDepthBoundsState(
    VkCmdBuffer cmdBuffer,
    VkDynamicDepthBoundsState dynamicDepthBoundsState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindDynamicDepthBoundsState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindDynamicDepthBoundsState, 0);
    g_devTable.CmdBindDynamicDepthBoundsState(cmdBuffer, dynamicDepthBoundsState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindDynamicDepthBoundsState(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->dynamicDepthBoundsState = dynamicDepthBoundsState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicStencilState(
    VkCmdBuffer cmdBuffer,
    VkDynamicStencilState dynamicStencilState)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindDynamicStencilState* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindDynamicStencilState, 0);
    g_devTable.CmdBindDynamicStencilState(cmdBuffer, dynamicStencilState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindDynamicStencilState(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->dynamicStencilState = dynamicStencilState;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDescriptorSets(
    VkCmdBuffer cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout,
    uint32_t firstSet,
    uint32_t setCount,
    const VkDescriptorSet* pDescriptorSets,
    uint32_t dynamicOffsetCount,
    const uint32_t* pDynamicOffsets)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindDescriptorSets* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindDescriptorSets, setCount*sizeof(VkDescriptorSet) + dynamicOffsetCount*sizeof(uint32_t));
    g_devTable.CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindDescriptorSets(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->layout = layout;
    pPacket->firstSet = firstSet;
    pPacket->setCount = setCount;
    pPacket->dynamicOffsetCount = dynamicOffsetCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), setCount*sizeof(VkDescriptorSet), pDescriptorSets);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDynamicOffsets), dynamicOffsetCount*sizeof(uint32_t), pDynamicOffsets);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSets));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDynamicOffsets));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindIndexBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    VkIndexType indexType)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindIndexBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindIndexBuffer, 0);
    g_devTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindIndexBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->buffer = buffer;
    pPacket->offset = offset;
    pPacket->indexType = indexType;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindVertexBuffers(
    VkCmdBuffer cmdBuffer,
    uint32_t startBinding,
    uint32_t bindingCount,
    const VkBuffer* pBuffers,
    const VkDeviceSize* pOffsets)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBindVertexBuffers* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBindVertexBuffers, bindingCount*sizeof(VkBuffer) + bindingCount*sizeof(VkDeviceSize));
    g_devTable.CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBindVertexBuffers(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->startBinding = startBinding;
    pPacket->bindingCount = bindingCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBuffers), bindingCount*sizeof(VkBuffer), pBuffers);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOffsets), bindingCount*sizeof(VkDeviceSize), pOffsets);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pBuffers));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pOffsets));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDraw(
    VkCmdBuffer cmdBuffer,
    uint32_t firstVertex,
    uint32_t vertexCount,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdDraw* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdDraw, 0);
    g_devTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdDraw(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->firstVertex = firstVertex;
    pPacket->vertexCount = vertexCount;
    pPacket->firstInstance = firstInstance;
    pPacket->instanceCount = instanceCount;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDrawIndexed(
    VkCmdBuffer cmdBuffer,
    uint32_t firstIndex,
    uint32_t indexCount,
    int32_t vertexOffset,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdDrawIndexed* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdDrawIndexed, 0);
    g_devTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdDrawIndexed(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->firstIndex = firstIndex;
    pPacket->indexCount = indexCount;
    pPacket->vertexOffset = vertexOffset;
    pPacket->firstInstance = firstInstance;
    pPacket->instanceCount = instanceCount;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDrawIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdDrawIndirect* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdDrawIndirect, 0);
    g_devTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdDrawIndirect(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->buffer = buffer;
    pPacket->offset = offset;
    pPacket->count = count;
    pPacket->stride = stride;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDrawIndexedIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset,
    uint32_t count,
    uint32_t stride)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdDrawIndexedIndirect* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdDrawIndexedIndirect, 0);
    g_devTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdDrawIndexedIndirect(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->buffer = buffer;
    pPacket->offset = offset;
    pPacket->count = count;
    pPacket->stride = stride;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDispatch(
    VkCmdBuffer cmdBuffer,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdDispatch* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdDispatch, 0);
    g_devTable.CmdDispatch(cmdBuffer, x, y, z);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdDispatch(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->x = x;
    pPacket->y = y;
    pPacket->z = z;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDispatchIndirect(
    VkCmdBuffer cmdBuffer,
    VkBuffer buffer,
    VkDeviceSize offset)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdDispatchIndirect* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdDispatchIndirect, 0);
    g_devTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdDispatchIndirect(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->buffer = buffer;
    pPacket->offset = offset;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer srcBuffer,
    VkBuffer destBuffer,
    uint32_t regionCount,
    const VkBufferCopy* pRegions)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdCopyBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdCopyBuffer, regionCount*sizeof(VkBufferCopy));
    g_devTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdCopyBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcBuffer = srcBuffer;
    pPacket->destBuffer = destBuffer;
    pPacket->regionCount = regionCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(VkBufferCopy), pRegions);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    const VkImageCopy* pRegions)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdCopyImage* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdCopyImage, regionCount*sizeof(VkImageCopy));
    g_devTable.CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdCopyImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->srcImageLayout = srcImageLayout;
    pPacket->destImage = destImage;
    pPacket->destImageLayout = destImageLayout;
    pPacket->regionCount = regionCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(VkImageCopy), pRegions);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBlitImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    const VkImageBlit* pRegions,
    VkTexFilter filter)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBlitImage* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBlitImage, regionCount*sizeof(VkImageBlit));
    g_devTable.CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions, filter);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBlitImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->srcImageLayout = srcImageLayout;
    pPacket->destImage = destImage;
    pPacket->destImageLayout = destImageLayout;
    pPacket->regionCount = regionCount;
    pPacket->filter = filter;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(VkImageBlit), pRegions);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyBufferToImage(
    VkCmdBuffer cmdBuffer,
    VkBuffer srcBuffer,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    const VkBufferImageCopy* pRegions)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdCopyBufferToImage* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdCopyBufferToImage, regionCount*sizeof(VkBufferImageCopy));
    g_devTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdCopyBufferToImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcBuffer = srcBuffer;
    pPacket->destImage = destImage;
    pPacket->destImageLayout = destImageLayout;
    pPacket->regionCount = regionCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(VkBufferImageCopy), pRegions);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyImageToBuffer(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkBuffer destBuffer,
    uint32_t regionCount,
    const VkBufferImageCopy* pRegions)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdCopyImageToBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdCopyImageToBuffer, regionCount*sizeof(VkBufferImageCopy));
    g_devTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdCopyImageToBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->srcImageLayout = srcImageLayout;
    pPacket->destBuffer = destBuffer;
    pPacket->regionCount = regionCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(VkBufferImageCopy), pRegions);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdUpdateBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize dataSize,
    const uint32_t* pData)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdUpdateBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdUpdateBuffer, dataSize);
    g_devTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdUpdateBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->destBuffer = destBuffer;
    pPacket->destOffset = destOffset;
    pPacket->dataSize = dataSize;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), sizeof(uint32_t), pData);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdFillBuffer(
    VkCmdBuffer cmdBuffer,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize fillSize,
    uint32_t data)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdFillBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdFillBuffer, 0);
    g_devTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdFillBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->destBuffer = destBuffer;
    pPacket->destOffset = destOffset;
    pPacket->fillSize = fillSize;
    pPacket->data = data;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdClearColorImage(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    const VkClearColorValue* pColor,
    uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdClearColorImage* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdClearColorImage, sizeof(VkClearColorValue) + rangeCount*sizeof(VkImageSubresourceRange));
    g_devTable.CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdClearColorImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->image = image;
    pPacket->imageLayout = imageLayout;
    pPacket->rangeCount = rangeCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pColor), sizeof(VkClearColorValue), pColor);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRanges), rangeCount*sizeof(VkImageSubresourceRange), pRanges);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pColor));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRanges));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdClearDepthStencilImage(
    VkCmdBuffer cmdBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    float depth,
    uint32_t stencil,
    uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdClearDepthStencilImage* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdClearDepthStencilImage, rangeCount*sizeof(VkImageSubresourceRange));
    g_devTable.CmdClearDepthStencilImage(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdClearDepthStencilImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->image = image;
    pPacket->imageLayout = imageLayout;
    pPacket->depth = depth;
    pPacket->stencil = stencil;
    pPacket->rangeCount = rangeCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRanges), rangeCount*sizeof(VkImageSubresourceRange), pRanges);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRanges));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdClearColorAttachment(
    VkCmdBuffer cmdBuffer,
    uint32_t colorAttachment,
    VkImageLayout imageLayout,
    const VkClearColorValue* pColor,
    uint32_t rectCount,
    const VkRect3D* pRects)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdClearColorAttachment* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdClearColorAttachment, sizeof(VkClearColorValue) + rectCount*sizeof(VkRect3D));
    g_devTable.CmdClearColorAttachment(cmdBuffer, colorAttachment, imageLayout, pColor, rectCount, pRects);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdClearColorAttachment(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->colorAttachment = colorAttachment;
    pPacket->imageLayout = imageLayout;
    pPacket->rectCount = rectCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pColor), sizeof(VkClearColorValue), pColor);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRects), rectCount*sizeof(VkRect3D), pRects);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pColor));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRects));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdClearDepthStencilAttachment(
    VkCmdBuffer cmdBuffer,
    VkImageAspectFlags imageAspectMask,
    VkImageLayout imageLayout,
    float depth,
    uint32_t stencil,
    uint32_t rectCount,
    const VkRect3D* pRects)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdClearDepthStencilAttachment* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdClearDepthStencilAttachment, rectCount*sizeof(VkRect3D));
    g_devTable.CmdClearDepthStencilAttachment(cmdBuffer, imageAspectMask, imageLayout, depth, stencil, rectCount, pRects);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdClearDepthStencilAttachment(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->imageAspectMask = imageAspectMask;
    pPacket->imageLayout = imageLayout;
    pPacket->depth = depth;
    pPacket->stencil = stencil;
    pPacket->rectCount = rectCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRects), rectCount*sizeof(VkRect3D), pRects);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRects));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdResolveImage(
    VkCmdBuffer cmdBuffer,
    VkImage srcImage,
    VkImageLayout srcImageLayout,
    VkImage destImage,
    VkImageLayout destImageLayout,
    uint32_t regionCount,
    const VkImageResolve* pRegions)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdResolveImage* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdResolveImage, regionCount*sizeof(VkImageResolve));
    g_devTable.CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdResolveImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->srcImageLayout = srcImageLayout;
    pPacket->destImage = destImage;
    pPacket->destImageLayout = destImageLayout;
    pPacket->regionCount = regionCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(VkImageResolve), pRegions);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdSetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdSetEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdSetEvent, 0);
    g_devTable.CmdSetEvent(cmdBuffer, event, stageMask);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdSetEvent(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->event = event;
    pPacket->stageMask = stageMask;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdResetEvent(
    VkCmdBuffer cmdBuffer,
    VkEvent event,
    VkPipelineStageFlags stageMask)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdResetEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdResetEvent, 0);
    g_devTable.CmdResetEvent(cmdBuffer, event, stageMask);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdResetEvent(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->event = event;
    pPacket->stageMask = stageMask;
    FINISH_TRACE_PACKET();
}

// __HOOKED_vkCmdWaitEvents is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkCmdPipelineBarrier is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBeginQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot,
    VkQueryControlFlags flags)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBeginQuery* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdBeginQuery, 0);
    g_devTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBeginQuery(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->slot = slot;
    pPacket->flags = flags;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdEndQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t slot)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdEndQuery* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdEndQuery, 0);
    g_devTable.CmdEndQuery(cmdBuffer, queryPool, slot);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdEndQuery(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->slot = slot;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdResetQueryPool(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdResetQueryPool* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdResetQueryPool, 0);
    g_devTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdResetQueryPool(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->startQuery = startQuery;
    pPacket->queryCount = queryCount;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdWriteTimestamp(
    VkCmdBuffer cmdBuffer,
    VkTimestampType timestampType,
    VkBuffer destBuffer,
    VkDeviceSize destOffset)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdWriteTimestamp* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdWriteTimestamp, 0);
    g_devTable.CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdWriteTimestamp(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->timestampType = timestampType;
    pPacket->destBuffer = destBuffer;
    pPacket->destOffset = destOffset;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyQueryPoolResults(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    VkBuffer destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize destStride,
    VkQueryResultFlags flags)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdCopyQueryPoolResults* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdCopyQueryPoolResults, 0);
    g_devTable.CmdCopyQueryPoolResults(cmdBuffer, queryPool, startQuery, queryCount, destBuffer, destOffset, destStride, flags);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdCopyQueryPoolResults(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->startQuery = startQuery;
    pPacket->queryCount = queryCount;
    pPacket->destBuffer = destBuffer;
    pPacket->destOffset = destOffset;
    pPacket->destStride = destStride;
    pPacket->flags = flags;
    FINISH_TRACE_PACKET();
}

// __HOOKED_vkCreateFramebuffer is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyFramebuffer(
    VkDevice device,
    VkFramebuffer framebuffer)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyFramebuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyFramebuffer, 0);
    g_devTable.DestroyFramebuffer(device, framebuffer);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyFramebuffer(pHeader);
    pPacket->device = device;
    pPacket->framebuffer = framebuffer;
    FINISH_TRACE_PACKET();
}

// __HOOKED_vkCreateRenderPass is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyRenderPass(
    VkDevice device,
    VkRenderPass renderPass)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkDestroyRenderPass* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroyRenderPass, 0);
    g_devTable.DestroyRenderPass(device, renderPass);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroyRenderPass(pHeader);
    pPacket->device = device;
    pPacket->renderPass = renderPass;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetRenderAreaGranularity(
    VkDevice device,
    VkRenderPass renderPass,
    VkExtent2D* pGranularity)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetRenderAreaGranularity* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetRenderAreaGranularity, sizeof(VkExtent2D));
    result = g_devTable.GetRenderAreaGranularity(device, renderPass, pGranularity);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetRenderAreaGranularity(pHeader);
    pPacket->device = device;
    pPacket->renderPass = renderPass;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGranularity), sizeof(VkExtent2D), pGranularity);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pGranularity));
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_vkCmdBeginRenderPass is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdNextSubpass(
    VkCmdBuffer cmdBuffer,
    VkRenderPassContents contents)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdNextSubpass* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdNextSubpass, 0);
    g_devTable.CmdNextSubpass(cmdBuffer, contents);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdNextSubpass(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->contents = contents;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdPushConstants(
    VkCmdBuffer cmdBuffer,
    VkPipelineLayout layout,
    VkShaderStageFlags stageFlags,
    uint32_t start,
    uint32_t length,
    const void* values)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdPushConstants* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdPushConstants, sizeof(values));
    g_devTable.CmdPushConstants(cmdBuffer, layout, stageFlags, start, length, values);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdPushConstants(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->layout = layout;
    pPacket->stageFlags = stageFlags;
    pPacket->start = start;
    pPacket->length = length;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->values), sizeof(void), values);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->values));
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdEndRenderPass(
    VkCmdBuffer cmdBuffer)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdEndRenderPass* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdEndRenderPass, 0);
    g_devTable.CmdEndRenderPass(cmdBuffer);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdEndRenderPass(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    FINISH_TRACE_PACKET();
}

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdExecuteCommands(
    VkCmdBuffer cmdBuffer,
    uint32_t cmdBuffersCount,
    const VkCmdBuffer* pCmdBuffers)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdExecuteCommands* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCmdExecuteCommands, cmdBuffersCount*sizeof(VkCmdBuffer));
    g_devTable.CmdExecuteCommands(cmdBuffer, cmdBuffersCount, pCmdBuffers);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdExecuteCommands(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->cmdBuffersCount = cmdBuffersCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCmdBuffers), cmdBuffersCount*sizeof(VkCmdBuffer), pCmdBuffers);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCmdBuffers));
    FINISH_TRACE_PACKET();
}

