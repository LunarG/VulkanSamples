/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
 * Copyright (C) 2015 Valve Corporation
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

#include "vulkan.h"
#include "glv_platform.h"
#include "glv_common.h"
#include "glvtrace_vk_vk_wsi_swapchain.h"
#include "glvtrace_vk_vk_wsi_device_swapchain.h"
#include "glvtrace_vk_helpers.h"
#include "glvtrace_vk_vk.h"
#include "glv_interconnect.h"
#include "glv_filelike.h"
#ifdef WIN32
#include "mhook/mhook-lib/mhook.h"
#endif
#include "glv_trace_packet_utils.h"
#include <stdio.h>

// declared as extern in glvtrace_vk_helpers.h
GLV_CRITICAL_SECTION g_memInfoLock;
VKMemInfo g_memInfo = {0, NULL, NULL, 0};

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkAllocMemory(
    VkDevice device,
    const VkMemoryAllocInfo* pAllocInfo,
    VkDeviceMemory* pMem)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkAllocMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkAllocMemory, get_struct_chain_size((void*)pAllocInfo) + sizeof(VkDeviceMemory));
    result = real_vkAllocMemory(device, pAllocInfo, pMem);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkAllocMemory(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo), sizeof(VkMemoryAllocInfo), pAllocInfo);
    add_alloc_memory_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo->pNext), pAllocInfo->pNext);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(VkDeviceMemory), pMem);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAllocInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    // begin custom code
    add_new_handle_to_mem_info(*pMem, pAllocInfo->allocationSize, NULL);
    // end custom code
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkMapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkFlags flags,
    void** ppData)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkMapMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkMapMemory, sizeof(void*));
    result = real_vkMapMemory(device, mem, offset, size, flags, ppData);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkMapMemory(pHeader);
    pPacket->device = device;
    pPacket->mem = mem;
    pPacket->offset = offset;
    pPacket->size = size;
    pPacket->flags = flags;
    if (ppData != NULL)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppData), sizeof(void*), *ppData);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData));
        add_data_to_mem_info(mem, size, offset, *ppData);
    }
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkUnmapMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkUnmapMemory* pPacket;
    VKAllocInfo *entry;
    size_t siz = 0, off = 0;
    // insert into packet the data that was written by CPU between the vkMapMemory call and here
    // Note must do this prior to the real vkUnMap() or else may get a FAULT
    glv_enter_critical_section(&g_memInfoLock);
    entry = find_mem_info_entry(mem);
    if (entry && entry->pData != NULL)
    {
        if (!entry->didFlush)
        {
            // no FlushMapped Memory
            siz = entry->rangeSize;
            off = entry->rangeOffset;
        }
    }
    CREATE_TRACE_PACKET(vkUnmapMemory, siz);
    pPacket = interpret_body_as_vkUnmapMemory(pHeader);
    if (siz)
    {
        assert(entry->handle.handle == mem.handle);
        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pData), siz, entry->pData + off);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
        entry->pData = NULL;
    }
    glv_leave_critical_section(&g_memInfoLock);
    result = real_vkUnmapMemory(device, mem);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket->device = device;
    pPacket->mem = mem;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkFreeMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkFreeMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkFreeMemory, 0);
    result = real_vkFreeMemory(device, mem);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkFreeMemory(pHeader);
    pPacket->device = device;
    pPacket->mem = mem;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    // begin custom code
    rm_handle_from_mem_info(mem);
    // end custom code
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memRangeCount,
    const VkMappedMemoryRange* pMemRanges)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    size_t rangesSize = 0;
    size_t dataSize = 0;
    uint32_t iter;
    packet_vkFlushMappedMemoryRanges* pPacket = NULL;

    // find out how much memory is in the ranges
    for (iter = 0; iter < memRangeCount; iter++)
    {
        VkMappedMemoryRange* pRange = (VkMappedMemoryRange*)&pMemRanges[iter];
        rangesSize += vk_size_vkmappedmemoryrange(pRange);
        dataSize += pRange->size;
    }

    CREATE_TRACE_PACKET(vkFlushMappedMemoryRanges, rangesSize + sizeof(void*)*memRangeCount + dataSize);
    pPacket = interpret_body_as_vkFlushMappedMemoryRanges(pHeader);

    glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pMemRanges), rangesSize, pMemRanges);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemRanges));

    // insert into packet the data that was written by CPU between the vkMapMemory call and here
    // create a temporary local ppData array and add it to the packet (to reserve the space for the array)
    void** ppTmpData = malloc(memRangeCount * sizeof(void*));
    glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->ppData), sizeof(void*)*memRangeCount, ppTmpData);
    free(ppTmpData);

    // now the actual memory
    glv_enter_critical_section(&g_memInfoLock);
    for (iter = 0; iter < memRangeCount; iter++)
    {
        VkMappedMemoryRange* pRange = (VkMappedMemoryRange*)&pMemRanges[iter];
        VKAllocInfo* pEntry = find_mem_info_entry(pRange->mem);

        if (pEntry != NULL)
        {
            assert(pEntry->handle.handle == pRange->mem.handle);
            assert(pEntry->totalSize >= (pRange->size + pRange->offset));
            assert(pEntry->totalSize >= pRange->size);
            assert(pRange->offset >= pEntry->rangeOffset && (pRange->offset + pRange->size) <= (pEntry->rangeOffset + pEntry->rangeSize));
            glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->ppData[iter]), pRange->size, pEntry->pData + pRange->offset);
            glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData[iter]));
            pEntry->didFlush = TRUE;
        }
        else
        {
             glv_LogError("Failed to copy app memory into trace packet (idx = %u) on vkFlushedMappedMemoryRanges", pHeader->global_packet_index);
        }
    }
    glv_leave_critical_section(&g_memInfoLock);

    // now finalize the ppData array since it is done being updated
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData));

    result = real_vkFlushMappedMemoryRanges(device, memRangeCount, pMemRanges);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket->device = device;
    pPacket->memRangeCount = memRangeCount;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDescriptorPool(
    VkDevice device,
    VkDescriptorPoolUsage poolUsage,
    uint32_t maxSets,
    const VkDescriptorPoolCreateInfo* pCreateInfo,
    VkDescriptorPool* pDescriptorPool)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDescriptorPool* pPacket = NULL;
    // begin custom code (needs to use get_struct_chain_size)
    CREATE_TRACE_PACKET(vkCreateDescriptorPool,  get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDescriptorPool));
    // end custom code
    result = real_vkCreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDescriptorPool(pHeader);
    pPacket->device = device;
    pPacket->poolUsage = poolUsage;
    pPacket->maxSets = maxSets;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDescriptorPoolCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount), pCreateInfo->count * sizeof(VkDescriptorTypeCount), pCreateInfo->pTypeCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorPool), sizeof(VkDescriptorPool), pDescriptorPool);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorPool));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicViewportState(
    VkDevice device,
    const VkDynamicViewportStateCreateInfo* pCreateInfo,
    VkDynamicViewportState* pState)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDynamicViewportState* pPacket = NULL;
    // begin custom code (needs to call get_struct_chain_size)
    uint32_t vpsCount = (pCreateInfo != NULL && pCreateInfo->pViewports != NULL) ? pCreateInfo->viewportAndScissorCount : 0;
    CREATE_TRACE_PACKET(vkCreateDynamicViewportState,  get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDynamicViewportState));
    // end custom code
    result = real_vkCreateDynamicViewportState(device, pCreateInfo, pState);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDynamicViewportState(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDynamicViewportStateCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pViewports), vpsCount * sizeof(VkViewport), pCreateInfo->pViewports);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pScissors), vpsCount * sizeof(VkRect2D), pCreateInfo->pScissors);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(VkDynamicViewportState), pState);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pViewports));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pScissors));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo,
    VkFramebuffer* pFramebuffer)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateFramebuffer* pPacket = NULL;
    // begin custom code
    uint32_t attachmentCount = (pCreateInfo != NULL && pCreateInfo->pAttachments != NULL) ? pCreateInfo->attachmentCount : 0;
    CREATE_TRACE_PACKET(vkCreateFramebuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkFramebuffer));
    // end custom code
    result = real_vkCreateFramebuffer(device, pCreateInfo, pFramebuffer);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateFramebuffer(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkFramebufferCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pAttachments), attachmentCount * sizeof(VkAttachmentView), pCreateInfo->pAttachments);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFramebuffer), sizeof(VkFramebuffer), pFramebuffer);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pAttachments));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pFramebuffer));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    VkInstance* pInstance)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateInstance* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    glv_platform_thread_once(&gInitOnce, InitTracer);
    SEND_ENTRYPOINT_ID(vkCreateInstance);
    if (real_vkCreateInstance == vkCreateInstance)
    {
        glv_platform_get_next_lib_sym((void **) &real_vkCreateInstance,"vkCreateInstance");
    }
    startTime = glv_get_time();
    result = real_vkCreateInstance(pCreateInfo, pInstance);
    endTime = glv_get_time();
    CREATE_TRACE_PACKET(vkCreateInstance, sizeof(VkInstance) + get_struct_chain_size((void*)pCreateInfo));
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    if (isHooked == FALSE) {
        AttachHooks();
    }
    pPacket = interpret_body_as_vkCreateInstance(pHeader);

    add_VkInstanceCreateInfo_to_packet(pHeader, (VkInstanceCreateInfo**)&(pPacket->pCreateInfo), (VkInstanceCreateInfo*) pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pInstance), sizeof(VkInstance), pInstance);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pInstance));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo,
    VkRenderPass* pRenderPass)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateRenderPass* pPacket = NULL;
    // begin custom code (get_struct_chain_size)
    uint32_t attachmentCount = (pCreateInfo != NULL && (pCreateInfo->pAttachments != NULL)) ? pCreateInfo->attachmentCount : 0;
    CREATE_TRACE_PACKET(vkCreateRenderPass, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkRenderPass));
    // end custom code
    result = real_vkCreateRenderPass(device, pCreateInfo, pRenderPass);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateRenderPass(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkRenderPassCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pAttachments), attachmentCount * sizeof(VkAttachmentDescription), pCreateInfo->pAttachments);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pDependencies), sizeof(VkSubpassDependency), pCreateInfo->pDependencies);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pSubpasses), sizeof(VkSubpassDescription), pCreateInfo->pSubpasses);
    uint32_t i;
    for (i=0; i < pPacket->pCreateInfo->subpassCount; i++) {
        VkSubpassDescription *pSubpass = (VkSubpassDescription *) &pPacket->pCreateInfo->pSubpasses[i];
        const VkSubpassDescription *pSp = &pCreateInfo->pSubpasses[i];
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pSubpass->pInputAttachments), pSubpass->inputCount * sizeof(VkAttachmentReference), pSp->pInputAttachments);
        glv_finalize_buffer_address(pHeader, (void**)&(pSubpass->pInputAttachments));
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pSubpass->pColorAttachments), pSubpass->colorCount * sizeof(VkAttachmentReference), pSp->pColorAttachments);
        glv_finalize_buffer_address(pHeader, (void**)&(pSubpass->pColorAttachments));
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pSubpass->pResolveAttachments), pSubpass->colorCount * sizeof(VkAttachmentReference), pSp->pResolveAttachments);
        glv_finalize_buffer_address(pHeader, (void**)&(pSubpass->pResolveAttachments));
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pSubpass->pPreserveAttachments), pSubpass->preserveCount * sizeof(VkAttachmentReference), pSp->pPreserveAttachments);
        glv_finalize_buffer_address(pHeader, (void**)&(pSubpass->pPreserveAttachments));
    }
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRenderPass), sizeof(VkRenderPass), pRenderPass);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pAttachments));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pDependencies));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pSubpasses));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRenderPass));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetGlobalExtensionProperties(
    const char* pLayerName,
    uint32_t* pCount,
    VkExtensionProperties* pProperties)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetGlobalExtensionProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    glv_platform_thread_once(&gInitOnce, InitTracer);
    if (real_vkGetGlobalExtensionProperties == vkGetGlobalExtensionProperties) {
        glv_platform_get_next_lib_sym((void **) &real_vkGetGlobalExtensionProperties,"vkGetGlobalExtensionProperties");
    }
    startTime = glv_get_time();
    result = real_vkGetGlobalExtensionProperties(pLayerName, pCount, pProperties);
    endTime = glv_get_time();
    CREATE_TRACE_PACKET(vkGetGlobalExtensionProperties, ((pLayerName != NULL) ? strlen(pLayerName) + 1 : 0) + sizeof(uint32_t) + (*pCount * sizeof(VkExtensionProperties)));
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    if (isHooked == FALSE) {
        AttachHooks();
    }
    pPacket = interpret_body_as_vkGetGlobalExtensionProperties(pHeader);

    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pLayerName), ((pLayerName != NULL) ? strlen(pLayerName) + 1 : 0), pLayerName);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), *pCount * sizeof(VkExtensionProperties), pProperties);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pLayerName));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pCount,
    VkExtensionProperties* pProperties)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceExtensionProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    startTime = glv_get_time();
    result = real_vkGetPhysicalDeviceExtensionProperties(physicalDevice, pLayerName, pCount, pProperties);
    endTime = glv_get_time();
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceExtensionProperties, ((pLayerName != NULL) ? strlen(pLayerName) + 1 : 0) + sizeof(uint32_t) + (*pCount * sizeof(VkExtensionProperties)));
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetPhysicalDeviceExtensionProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pLayerName), ((pLayerName != NULL) ? strlen(pLayerName) + 1 : 0), pLayerName);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), *pCount * sizeof(VkExtensionProperties), pProperties);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pLayerName));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetGlobalLayerProperties(
    uint32_t* pCount,
    VkLayerProperties* pProperties)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetGlobalLayerProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    glv_platform_thread_once(&gInitOnce, InitTracer);
    if (real_vkGetGlobalLayerProperties == vkGetGlobalLayerProperties) {
        glv_platform_get_next_lib_sym((void **) &real_vkGetGlobalLayerProperties,"vkGetGlobalLayerProperties");
    }
    startTime = glv_get_time();
    result = real_vkGetGlobalLayerProperties(pCount, pProperties);
    endTime = glv_get_time();
    CREATE_TRACE_PACKET(vkGetGlobalLayerProperties, sizeof(uint32_t) + (*pCount * sizeof(VkLayerProperties)));
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    if (isHooked == FALSE) {
        AttachHooks();
    }
    pPacket = interpret_body_as_vkGetGlobalLayerProperties(pHeader);

    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), *pCount * sizeof(VkLayerProperties), pProperties);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceLayerProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pCount,
    VkLayerProperties* pProperties)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceLayerProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    startTime = glv_get_time();
    result = real_vkGetPhysicalDeviceLayerProperties(physicalDevice, pCount, pProperties);
    endTime = glv_get_time();
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceLayerProperties, sizeof(uint32_t) + (*pCount * sizeof(VkLayerProperties)));
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetPhysicalDeviceLayerProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), *pCount * sizeof(VkLayerProperties), pProperties);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}
// TODO : This should be pretty easy to fit into codegen. Don't need to make the call prior to creating packet
//  Just need to account for "count" number of queue properties
GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pCount,
    VkQueueFamilyProperties* pQueueFamilyProperties)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceQueueFamilyProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    startTime = glv_get_time();
    result = real_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pCount, pQueueFamilyProperties);
    endTime = glv_get_time();
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceQueueFamilyProperties, sizeof(uint32_t) + *pCount * sizeof(VkQueueFamilyProperties));
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetPhysicalDeviceQueueFamilyProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pQueueFamilyProperties), *pCount * sizeof(VkQueueFamilyProperties), pQueueFamilyProperties);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pQueueFamilyProperties));
    FINISH_TRACE_PACKET();
    return result;
}

/*
GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetGlobalExtensionInfo(
    VkExtensionInfoType infoType,
    uint32_t extensionIndex,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    packet_vkGetGlobalExtensionInfo* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    glv_platform_thread_once(&gInitOnce, InitTracer);
    if (real_vkGetGlobalExtensionInfo == vkGetGlobalExtensionInfo)
    {
        glv_platform_get_next_lib_sym((void **) &real_vkGetGlobalExtensionInfo,"vkGetGlobalExtensionInfo");
    }
    startTime = glv_get_time();
    result = real_vkGetGlobalExtensionInfo(infoType, extensionIndex, pDataSize, pData);
    endTime = glv_get_time();
    CREATE_TRACE_PACKET(vkGetGlobalExtensionInfo, (((pDataSize != NULL) ? sizeof(size_t) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0)));
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    if (isHooked == FALSE) {
        AttachHooks();
    }
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_vkGetGlobalExtensionInfo(pHeader);
    pPacket->infoType = infoType;
    pPacket->extensionIndex = extensionIndex;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}
*/

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkEnumeratePhysicalDevices* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    //TODO make sure can handle being called twice with pPD == 0
    SEND_ENTRYPOINT_ID(vkEnumeratePhysicalDevices);
    startTime = glv_get_time();
    result = real_vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    endTime = glv_get_time();
    CREATE_TRACE_PACKET(vkEnumeratePhysicalDevices, sizeof(uint32_t) + ((pPhysicalDevices && pPhysicalDeviceCount) ? *pPhysicalDeviceCount * sizeof(VkPhysicalDevice) : 0));
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkEnumeratePhysicalDevices(pHeader);
    pPacket->instance = instance;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPhysicalDeviceCount), sizeof(uint32_t), pPhysicalDeviceCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPhysicalDevices), *pPhysicalDeviceCount*sizeof(VkPhysicalDevice), pPhysicalDevices);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPhysicalDeviceCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPhysicalDevices));
    FINISH_TRACE_PACKET();
    return result;
}

// TODO138 : Update this
GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetQueryPoolResults(
    VkDevice device,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    size_t* pDataSize,
    void* pData,
    VkQueryResultFlags flags)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    packet_vkGetQueryPoolResults* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetQueryPoolResults, ((pDataSize != NULL) ? sizeof(size_t) : 0) + sizeof(void*));
    result = real_vkGetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags);
    glv_set_packet_entrypoint_end_time(pHeader);
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_vkGetQueryPoolResults(pHeader);
    pPacket->device = device;
    pPacket->queryPool = queryPool;
    pPacket->startQuery = startQuery;
    pPacket->queryCount = queryCount;
    pPacket->flags = flags;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), sizeof(void), pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkAllocDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetUsage setUsage,
    uint32_t count,
    const VkDescriptorSetLayout* pSetLayouts,
    VkDescriptorSet* pDescriptorSets)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkAllocDescriptorSets* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    SEND_ENTRYPOINT_ID(vkAllocDescriptorSets);
    startTime = glv_get_time();
    result = real_vkAllocDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets);
    endTime = glv_get_time();
    CREATE_TRACE_PACKET(vkAllocDescriptorSets, (count * sizeof(VkDescriptorSetLayout)) + (count * sizeof(VkDescriptorSet)));
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkAllocDescriptorSets(pHeader);
    pPacket->device = device;
    pPacket->descriptorPool = descriptorPool;
    pPacket->setUsage = setUsage;
    pPacket->count = count;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayouts), count * sizeof(VkDescriptorSetLayout), pSetLayouts);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), count * sizeof(VkDescriptorSet), pDescriptorSets);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetLayouts));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSets));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkUpdateDescriptorSets(
VkDevice device,
        uint32_t writeCount, 
        const VkWriteDescriptorSet* pDescriptorWrites, 
        uint32_t copyCount, 
        const VkCopyDescriptorSet* pDescriptorCopies);
// Manually written because it needs to use get_struct_chain_size and allocate some extra pointers (why?)
// Also since it needs to app the array of pointers and sub-buffers (see comments in function)
GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkUpdateDescriptorSets(
    VkDevice device,
    uint32_t writeCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t copyCount,
    const VkCopyDescriptorSet* pDescriptorCopies )
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkUpdateDescriptorSets* pPacket = NULL;
    // begin custom code
    size_t arrayByteCount = 0;
    size_t i;

    for (i = 0; i < writeCount; i++)
    {
        arrayByteCount += get_struct_chain_size(&pDescriptorWrites[i]);
    }

    for (i = 0; i < copyCount; i++)
    {
        arrayByteCount += get_struct_chain_size(&pDescriptorCopies[i]);
    }

    CREATE_TRACE_PACKET(vkUpdateDescriptorSets, arrayByteCount);
    // end custom code
    result = real_vkUpdateDescriptorSets(device, writeCount, pDescriptorWrites, copyCount, pDescriptorCopies);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkUpdateDescriptorSets(pHeader);
    pPacket->device = device;
    pPacket->writeCount = writeCount;
    // begin custom code
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorWrites), writeCount * sizeof(VkWriteDescriptorSet), pDescriptorWrites);
    for (i = 0; i < writeCount; i++)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorWrites[i].pDescriptors), pDescriptorWrites[i].count * sizeof(VkDescriptorInfo), pDescriptorWrites[i].pDescriptors);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorWrites[i].pDescriptors));
    }
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorWrites));

    pPacket->copyCount = copyCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorCopies), copyCount * sizeof(VkCopyDescriptorSet), pDescriptorCopies);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorCopies));
    // end custom code
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT void VKAPI __HOOKED_vkCmdWaitEvents(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        destStageMask,
    uint32_t                                    memBarrierCount,
    const void* const*                          ppMemBarriers)
{
    glv_trace_packet_header* pHeader;
    packet_vkCmdWaitEvents* pPacket = NULL;
    size_t customSize;
    customSize = (eventCount * sizeof(VkEvent)) + memBarrierCount * sizeof(void*) + calculate_memory_barrier_size(memBarrierCount, ppMemBarriers);
    CREATE_TRACE_PACKET(vkCmdWaitEvents, customSize);
    real_vkCmdWaitEvents(cmdBuffer, eventCount, pEvents, srcStageMask, destStageMask, memBarrierCount, ppMemBarriers);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdWaitEvents(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->eventCount = eventCount;
    pPacket->srcStageMask = srcStageMask;
    pPacket->destStageMask = destStageMask;
    pPacket->memBarrierCount = memBarrierCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pEvents), eventCount * sizeof(VkEvent), pEvents);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pEvents));
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers), memBarrierCount * sizeof(void*), ppMemBarriers);
    uint32_t i, siz;
    for (i = 0; i < memBarrierCount; i++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) ppMemBarriers[i];
        switch (pNext->sType) {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                siz = sizeof(VkMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                siz = sizeof(VkBufferMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                siz = sizeof(VkImageMemoryBarrier);
                break;
            default:
                assert(0);
                siz = 0;
                break;
        }
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers[i]), siz, ppMemBarriers[i]);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers[i]));
    }
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void VKAPI __HOOKED_vkCmdPipelineBarrier(
    VkCmdBuffer                                 cmdBuffer,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        destStageMask,
    VkBool32                                    byRegion,
    uint32_t                                    memBarrierCount,
    const void* const*                          ppMemBarriers)
{
    glv_trace_packet_header* pHeader;
    packet_vkCmdPipelineBarrier* pPacket = NULL;
    size_t customSize;
    customSize = (memBarrierCount * sizeof(void*)) + calculate_memory_barrier_size(memBarrierCount, ppMemBarriers);
    CREATE_TRACE_PACKET(vkCmdPipelineBarrier, customSize);
    real_vkCmdPipelineBarrier(cmdBuffer, srcStageMask, destStageMask, byRegion, memBarrierCount, ppMemBarriers);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdPipelineBarrier(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcStageMask = srcStageMask;
    pPacket->destStageMask = destStageMask;
    pPacket->byRegion = byRegion;
    pPacket->memBarrierCount = memBarrierCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers), memBarrierCount * sizeof(void*), ppMemBarriers);
    uint32_t i, siz;
    for (i = 0; i < memBarrierCount; i++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) ppMemBarriers[i];
        switch (pNext->sType) {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                siz = sizeof(VkMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                siz = sizeof(VkBufferMemoryBarrier);
                break;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                siz = sizeof(VkImageMemoryBarrier);
                break;
            default:
                assert(0);
                siz = 0;
                break;
        }
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers[i]), siz, ppMemBarriers[i]);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers[i]));
    }
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateGraphicsPipelines* pPacket = NULL;
    size_t total_size = 0;
    uint32_t i;
    for (i = 0; i < count; i++) {
        total_size += get_struct_chain_size((void*)&pCreateInfos[i]);
    }
    CREATE_TRACE_PACKET(vkCreateGraphicsPipelines, total_size + count*sizeof(VkPipeline));
    result = real_vkCreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pPipelines);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateGraphicsPipelines(pHeader);
    pPacket->device = device;
    pPacket->pipelineCache = pipelineCache;
    pPacket->count = count;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfos), count*sizeof(VkGraphicsPipelineCreateInfo), pCreateInfos);
    add_VkGraphicsPipelineCreateInfos_to_trace_packet(pHeader, (VkGraphicsPipelineCreateInfo*)pPacket->pCreateInfos, pCreateInfos, count);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipelines), count*sizeof(VkPipeline), pPipelines);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfos));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipelines));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateComputePipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    const VkComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateComputePipelines* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateComputePipelines, count*sizeof(VkComputePipelineCreateInfo) + sizeof(VkPipeline));
    result = real_vkCreateComputePipelines(device, pipelineCache, count, pCreateInfos, pPipelines);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateComputePipelines(pHeader);
    pPacket->device = device;
    pPacket->pipelineCache = pipelineCache;
    pPacket->count = count;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfos), count*sizeof(VkComputePipelineCreateInfo), pCreateInfos);
    add_VkComputePipelineCreateInfos_to_trace_packet(pHeader, (VkComputePipelineCreateInfo*)pPacket->pCreateInfos, pCreateInfos, count);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipelines), count*sizeof(VkPipeline), pPipelines);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfos));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipelines));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT void VKAPI __HOOKED_vkCmdBeginRenderPass(
    VkCmdBuffer cmdBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkRenderPassContents contents)
{
    glv_trace_packet_header* pHeader;
    packet_vkCmdBeginRenderPass* pPacket = NULL;
    size_t clearValueSize = sizeof(VkClearValue) * pRenderPassBegin->clearValueCount;
    CREATE_TRACE_PACKET(vkCmdBeginRenderPass, sizeof(VkRenderPassBeginInfo) + clearValueSize);
    real_vkCmdBeginRenderPass(cmdBuffer, pRenderPassBegin, contents);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBeginRenderPass(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->contents = contents;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRenderPassBegin), sizeof(VkRenderPassBeginInfo), pRenderPassBegin);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRenderPassBegin->pClearValues), clearValueSize, pRenderPassBegin->pClearValues);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRenderPassBegin->pClearValues));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRenderPassBegin));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkFreeDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t count,
    const VkDescriptorSet* pDescriptorSets)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkFreeDescriptorSets* pPacket = NULL;
    CREATE_TRACE_PACKET(vkFreeDescriptorSets, count*sizeof(VkDescriptorSet));
    result = real_vkFreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkFreeDescriptorSets(pHeader);
    pPacket->device = device;
    pPacket->descriptorPool = descriptorPool;
    pPacket->count = count;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), count*sizeof(VkDescriptorSet), pDescriptorSets);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSets));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSurfaceInfoWSI(
    VkDevice device,
    const VkSurfaceDescriptionWSI* pSurfaceDescription,
    VkSurfaceInfoTypeWSI infoType,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    packet_vkGetSurfaceInfoWSI* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    startTime = glv_get_time();
    result = real_vkGetSurfaceInfoWSI(device, pSurfaceDescription, infoType, pDataSize, pData);
    endTime = glv_get_time();
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    CREATE_TRACE_PACKET(vkGetSurfaceInfoWSI, sizeof(VkSurfaceDescriptionWSI) + sizeof(size_t) + _dataSize);
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetSurfaceInfoWSI(pHeader);
    pPacket->device = device;
    pPacket->infoType = infoType;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSurfaceDescription), sizeof(VkSurfaceDescriptionWSI), pSurfaceDescription);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSurfaceDescription));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}


GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSwapChainInfoWSI(
    VkDevice device,
    VkSwapChainWSI swapChain,
    VkSwapChainInfoTypeWSI infoType,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    packet_vkGetSwapChainInfoWSI* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t glvStartTime = glv_get_time();
    startTime = glv_get_time();
    result = real_vkGetSwapChainInfoWSI(device, swapChain, infoType, pDataSize, pData);
    endTime = glv_get_time();
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    CREATE_TRACE_PACKET(vkGetSwapChainInfoWSI, sizeof(size_t) + _dataSize);
    pHeader->glave_begin_time = glvStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetSwapChainInfoWSI(pHeader);
    pPacket->device = device;
    pPacket->swapChain = swapChain;
    pPacket->infoType = infoType;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueuePresentWSI(
    VkQueue queue,
    VkPresentInfoWSI* pPresentInfo)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkQueuePresentWSI* pPacket = NULL;
    size_t swapChainSize = pPresentInfo->swapChainCount*sizeof(VkSwapChainWSI);
    size_t indexSize = pPresentInfo->swapChainCount*sizeof(uint32_t);
    CREATE_TRACE_PACKET(vkQueuePresentWSI, sizeof(VkPresentInfoWSI)+swapChainSize+indexSize);
    result = real_vkQueuePresentWSI(queue, pPresentInfo);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkQueuePresentWSI(pHeader);
    pPacket->queue = queue;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPresentInfo), sizeof(VkPresentInfoWSI), pPresentInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPresentInfo->swapChains), swapChainSize, pPresentInfo->swapChains);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPresentInfo->imageIndices), indexSize, pPresentInfo->imageIndices);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPresentInfo->imageIndices));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPresentInfo->swapChains));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPresentInfo));
    FINISH_TRACE_PACKET();
    return result;
}


/* TODO: Probably want to make this manual to get the result of the boolean and then check it on replay
GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceSurfaceSupportWSI(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    const VkSurfaceDescriptionWSI* pSurfaceDescription,
    VkBool32* pSupported)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceSurfaceSupportWSI* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceSurfaceSupportWSI, sizeof(VkSurfaceDescriptionWSI) + sizeof(VkBool32));
    result = real_vkGetPhysicalDeviceSurfaceSupportWSI(physicalDevice, queueFamilyIndex, pSurfaceDescription, pSupported);
    glv_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPhysicalDeviceSurfaceSupportWSI(pHeader);
    pPacket->physicalDevice = physicalDevice;
    pPacket->queueFamilyIndex = queueFamilyIndex;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSurfaceDescription), sizeof(VkSurfaceDescriptionWSI), pSurfaceDescription);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSupported), sizeof(VkBool32), pSupported);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSurfaceDescription));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSupported));
    FINISH_TRACE_PACKET();
    return result;
}
*/
