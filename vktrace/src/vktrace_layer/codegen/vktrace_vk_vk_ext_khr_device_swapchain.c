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
#include "vk_ext_khr_swapchain.h"
#include "vktrace_vk_vk_ext_khr_device_swapchain.h"
#include "vktrace_vk_vk_ext_khr_device_swapchain_packets.h"
#include "vktrace_vk_packet_id.h"
#include "vk_struct_size_helper.h"
#include "vk_ext_khr_device_swapchain_struct_size_helper.h"
#include "vktrace_lib_helpers.h"

// __HOOKED_vkGetSurfacePropertiesKHR is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkGetSurfaceFormatsKHR is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkGetSurfacePresentModesKHR is manually written. Look in vktrace_vk_trace.c

// __HOOKED_vkCreateSwapchainKHR is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkDestroySwapchainKHR(
    VkDevice device,
    VkSwapchainKHR swapchain)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkDestroySwapchainKHR* pPacket = NULL;
    CREATE_TRACE_PACKET(vkDestroySwapchainKHR, 0);
    result = g_devTable.DestroySwapchainKHR(device, swapchain);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkDestroySwapchainKHR(pHeader);
    pPacket->device = device;
    pPacket->swapchain = swapchain;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_vkGetSwapchainImagesKHR is manually written. Look in vktrace_vk_trace.c

// CODEGEN : file /home/jon/vk/vk/vktrace/vktrace_generate.py line #511
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkAcquireNextImageKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint64_t timeout,
    VkSemaphore semaphore,
    uint32_t* pImageIndex)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkAcquireNextImageKHR* pPacket = NULL;
    CREATE_TRACE_PACKET(vkAcquireNextImageKHR, sizeof(uint32_t));
    result = g_devTable.AcquireNextImageKHR(device, swapchain, timeout, semaphore, pImageIndex);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkAcquireNextImageKHR(pHeader);
    pPacket->device = device;
    pPacket->swapchain = swapchain;
    pPacket->timeout = timeout;
    pPacket->semaphore = semaphore;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pImageIndex), sizeof(uint32_t), pImageIndex);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pImageIndex));
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_vkQueuePresentKHR is manually written. Look in vktrace_vk_trace.c

