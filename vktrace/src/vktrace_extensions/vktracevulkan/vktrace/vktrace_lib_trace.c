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
#include <stdbool.h>

#include "vulkan.h"
#include "vktrace_platform.h"
#include "vktrace_common.h"
#include "vktrace_vk_vk_ext_khr_swapchain.h"
#include "vktrace_vk_vk_ext_khr_device_swapchain.h"
#include "vk_ext_khr_device_swapchain_struct_size_helper.h"
#include "vktrace_lib_helpers.h"
#include "vktrace_vk_vk.h"
#include "vktrace_interconnect.h"
#include "vktrace_filelike.h"
#ifdef WIN32
#include "mhook/mhook-lib/mhook.h"
#endif
#include "vktrace_trace_packet_utils.h"
#include <stdio.h>

// declared as extern in vktrace_lib_helpers.h
VKTRACE_CRITICAL_SECTION g_memInfoLock;
VKMemInfo g_memInfo = {0, NULL, NULL, 0};

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkAllocMemory(
    VkDevice device,
    const VkMemoryAllocInfo* pAllocInfo,
    VkDeviceMemory* pMem)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkAllocMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkAllocMemory, get_struct_chain_size((void*)pAllocInfo) + sizeof(VkDeviceMemory));
    result = real_vkAllocMemory(device, pAllocInfo, pMem);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkAllocMemory(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo), sizeof(VkMemoryAllocInfo), pAllocInfo);
    add_alloc_memory_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo->pNext), pAllocInfo->pNext);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(VkDeviceMemory), pMem);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pAllocInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    // begin custom code
    add_new_handle_to_mem_info(*pMem, pAllocInfo->allocationSize, NULL);
    // end custom code
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkMapMemory(
    VkDevice device,
    VkDeviceMemory mem,
    VkDeviceSize offset,
    VkDeviceSize size,
    VkFlags flags,
    void** ppData)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkMapMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkMapMemory, sizeof(void*));
    result = real_vkMapMemory(device, mem, offset, size, flags, ppData);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkMapMemory(pHeader);
    pPacket->device = device;
    pPacket->mem = mem;
    pPacket->offset = offset;
    pPacket->size = size;
    pPacket->flags = flags;
    if (ppData != NULL)
    {
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppData), sizeof(void*), *ppData);
        vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData));
        add_data_to_mem_info(mem, size, offset, *ppData);
    }
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkUnmapMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkUnmapMemory* pPacket;
    VKAllocInfo *entry;
    size_t siz = 0, off = 0;
    // insert into packet the data that was written by CPU between the vkMapMemory call and here
    // Note must do this prior to the real vkUnMap() or else may get a FAULT
    vktrace_enter_critical_section(&g_memInfoLock);
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
        vktrace_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pData), siz, entry->pData + off);
        vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
        entry->pData = NULL;
    }
    vktrace_leave_critical_section(&g_memInfoLock);
    result = real_vkUnmapMemory(device, mem);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket->device = device;
    pPacket->mem = mem;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkFreeMemory(
    VkDevice device,
    VkDeviceMemory mem)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkFreeMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkFreeMemory, 0);
    result = real_vkFreeMemory(device, mem);
    vktrace_set_packet_entrypoint_end_time(pHeader);
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

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkFlushMappedMemoryRanges(
    VkDevice device,
    uint32_t memRangeCount,
    const VkMappedMemoryRange* pMemRanges)
{
    vktrace_trace_packet_header* pHeader;
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

    vktrace_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pMemRanges), rangesSize, pMemRanges);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemRanges));

    // insert into packet the data that was written by CPU between the vkMapMemory call and here
    // create a temporary local ppData array and add it to the packet (to reserve the space for the array)
    void** ppTmpData = malloc(memRangeCount * sizeof(void*));
    vktrace_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->ppData), sizeof(void*)*memRangeCount, ppTmpData);
    free(ppTmpData);

    // now the actual memory
    vktrace_enter_critical_section(&g_memInfoLock);
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
            vktrace_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->ppData[iter]), pRange->size, pEntry->pData + pRange->offset);
            vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData[iter]));
            pEntry->didFlush = TRUE;
        }
        else
        {
             vktrace_LogError("Failed to copy app memory into trace packet (idx = %u) on vkFlushedMappedMemoryRanges", pHeader->global_packet_index);
        }
    }
    vktrace_leave_critical_section(&g_memInfoLock);

    // now finalize the ppData array since it is done being updated
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData));

    result = real_vkFlushMappedMemoryRanges(device, memRangeCount, pMemRanges);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket->device = device;
    pPacket->memRangeCount = memRangeCount;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDescriptorPool(
    VkDevice device,
    VkDescriptorPoolUsage poolUsage,
    uint32_t maxSets,
    const VkDescriptorPoolCreateInfo* pCreateInfo,
    VkDescriptorPool* pDescriptorPool)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDescriptorPool* pPacket = NULL;
    // begin custom code (needs to use get_struct_chain_size)
    CREATE_TRACE_PACKET(vkCreateDescriptorPool,  get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDescriptorPool));
    // end custom code
    result = real_vkCreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDescriptorPool(pHeader);
    pPacket->device = device;
    pPacket->poolUsage = poolUsage;
    pPacket->maxSets = maxSets;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDescriptorPoolCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount), pCreateInfo->count * sizeof(VkDescriptorTypeCount), pCreateInfo->pTypeCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorPool), sizeof(VkDescriptorPool), pDescriptorPool);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorPool));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* pCreateInfo,
    VkDevice* pDevice)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    uint32_t i, count;
    const char strScreenShot[] = "ScreenShot";
    char *strScreenShotEnv = vktrace_get_global_var("_VK_SCREENSHOT");
    packet_vkCreateDevice* pPacket = NULL;
    char **ppEnabledLayerNames = NULL, **saved_ppELN;
    VkDeviceCreateInfo **ppCI;

    if (strScreenShotEnv && strlen(strScreenShotEnv) != 0)
    {
        // enable screenshot layer if it is available and not already in list
        bool found_ss = false;
        for (i = 0; i < pCreateInfo->layerCount; i++)
        {
            if (!strcmp(pCreateInfo->ppEnabledLayerNames[i], "ScreenShot"))
            {
                found_ss = true;
                break;
            }
        }
        if (!found_ss)
        {
            // query to find if ScreenShot layer is available
            real_vkGetPhysicalDeviceLayerProperties(physicalDevice, &count, NULL);
            VkLayerProperties *props = (VkLayerProperties *) vktrace_malloc(count * sizeof (VkLayerProperties));
            if (props && count > 0)
                real_vkGetPhysicalDeviceLayerProperties(physicalDevice, &count, props);
            for (i = 0; i < count; i++) {
                if (!strcmp(props[i].layerName, "ScreenShot"))
                {
                    found_ss = true;
                    break;
                }
            }
            if (found_ss)
            {
                // screenshot layer is available so enable it
                ppEnabledLayerNames = (char **) vktrace_malloc((pCreateInfo->layerCount + 1) * sizeof (char *));
                for (i = 0; i < pCreateInfo->layerCount && ppEnabledLayerNames; i++) {
                    ppEnabledLayerNames[i] = (char *) pCreateInfo->ppEnabledLayerNames[i];
                }
                ppEnabledLayerNames[pCreateInfo->layerCount] = (char *) vktrace_malloc(strlen(strScreenShot) + 1);
                ppCI = (VkDeviceCreateInfo **) &pCreateInfo;
                strcpy(ppEnabledLayerNames[pCreateInfo->layerCount], strScreenShot);
                (*ppCI)->layerCount = pCreateInfo->layerCount + 1;
                saved_ppELN = (char **) pCreateInfo->ppEnabledLayerNames;
                (*ppCI)->ppEnabledLayerNames = (const char*const*) ppEnabledLayerNames;
            }
            vktrace_free(props);
        }
    }
    CREATE_TRACE_PACKET(vkCreateDevice, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDevice));
    result = real_vkCreateDevice(physicalDevice, pCreateInfo, pDevice);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDevice(pHeader);
    pPacket->physicalDevice = physicalDevice;
    add_VkDeviceCreateInfo_to_packet(pHeader, (VkDeviceCreateInfo**) &(pPacket->pCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDevice), sizeof(VkDevice), pDevice);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDevice));
    if (ppEnabledLayerNames)
    {
        vktrace_free(ppEnabledLayerNames[pCreateInfo->layerCount-1]);
        vktrace_free(ppEnabledLayerNames);
        (*ppCI)->ppEnabledLayerNames = (const char*const*) saved_ppELN;
    }
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicViewportState(
    VkDevice device,
    const VkDynamicViewportStateCreateInfo* pCreateInfo,
    VkDynamicViewportState* pState)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDynamicViewportState* pPacket = NULL;
    // begin custom code (needs to call get_struct_chain_size)
    uint32_t vpsCount = (pCreateInfo != NULL && pCreateInfo->pViewports != NULL) ? pCreateInfo->viewportAndScissorCount : 0;
    CREATE_TRACE_PACKET(vkCreateDynamicViewportState,  get_struct_chain_size((void*)pCreateInfo) + sizeof(VkDynamicViewportState));
    // end custom code
    result = real_vkCreateDynamicViewportState(device, pCreateInfo, pState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDynamicViewportState(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkDynamicViewportStateCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pViewports), vpsCount * sizeof(VkViewport), pCreateInfo->pViewports);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pScissors), vpsCount * sizeof(VkRect2D), pCreateInfo->pScissors);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(VkDynamicViewportState), pState);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pViewports));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pScissors));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateFramebuffer(
    VkDevice device,
    const VkFramebufferCreateInfo* pCreateInfo,
    VkFramebuffer* pFramebuffer)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateFramebuffer* pPacket = NULL;
    // begin custom code
    uint32_t attachmentCount = (pCreateInfo != NULL && pCreateInfo->pAttachments != NULL) ? pCreateInfo->attachmentCount : 0;
    CREATE_TRACE_PACKET(vkCreateFramebuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkFramebuffer));
    // end custom code
    result = real_vkCreateFramebuffer(device, pCreateInfo, pFramebuffer);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateFramebuffer(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkFramebufferCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pAttachments), attachmentCount * sizeof(VkImageView), pCreateInfo->pAttachments);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFramebuffer), sizeof(VkFramebuffer), pFramebuffer);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pAttachments));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pFramebuffer));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    VkInstance* pInstance)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateInstance* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    vktrace_platform_thread_once(&gInitOnce, InitTracer);
    SEND_ENTRYPOINT_ID(vkCreateInstance);
    if (real_vkCreateInstance == vkCreateInstance)
    {
        vktrace_platform_get_next_lib_sym((void **) &real_vkCreateInstance,"vkCreateInstance");
    }
    startTime = vktrace_get_time();
    result = real_vkCreateInstance(pCreateInfo, pInstance);
    endTime = vktrace_get_time();
    CREATE_TRACE_PACKET(vkCreateInstance, sizeof(VkInstance) + get_struct_chain_size((void*)pCreateInfo));
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    if (isHooked == FALSE) {
        AttachHooks();
    }
    pPacket = interpret_body_as_vkCreateInstance(pHeader);

    add_VkInstanceCreateInfo_to_packet(pHeader, (VkInstanceCreateInfo**)&(pPacket->pCreateInfo), (VkInstanceCreateInfo*) pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pInstance), sizeof(VkInstance), pInstance);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pInstance));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateRenderPass(
    VkDevice device,
    const VkRenderPassCreateInfo* pCreateInfo,
    VkRenderPass* pRenderPass)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateRenderPass* pPacket = NULL;
    // begin custom code (get_struct_chain_size)
    uint32_t attachmentCount = (pCreateInfo != NULL && (pCreateInfo->pAttachments != NULL)) ? pCreateInfo->attachmentCount : 0;
    uint32_t dependencyCount = (pCreateInfo != NULL && (pCreateInfo->pDependencies != NULL)) ? pCreateInfo->dependencyCount : 0;
    uint32_t subpassCount = (pCreateInfo != NULL && (pCreateInfo->pSubpasses != NULL)) ? pCreateInfo->subpassCount : 0;
    CREATE_TRACE_PACKET(vkCreateRenderPass, get_struct_chain_size((void*)pCreateInfo) + sizeof(VkRenderPass));
    // end custom code
    result = real_vkCreateRenderPass(device, pCreateInfo, pRenderPass);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateRenderPass(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkRenderPassCreateInfo), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pAttachments), attachmentCount * sizeof(VkAttachmentDescription), pCreateInfo->pAttachments);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pDependencies), dependencyCount * sizeof(VkSubpassDependency), pCreateInfo->pDependencies);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pSubpasses), subpassCount * sizeof(VkSubpassDescription), pCreateInfo->pSubpasses);
    uint32_t i;
    for (i=0; i < pPacket->pCreateInfo->subpassCount; i++) {
        VkSubpassDescription *pSubpass = (VkSubpassDescription *) &pPacket->pCreateInfo->pSubpasses[i];
        const VkSubpassDescription *pSp = &pCreateInfo->pSubpasses[i];
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pSubpass->pInputAttachments), pSubpass->inputCount * sizeof(VkAttachmentReference), pSp->pInputAttachments);
        vktrace_finalize_buffer_address(pHeader, (void**)&(pSubpass->pInputAttachments));
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pSubpass->pColorAttachments), pSubpass->colorCount * sizeof(VkAttachmentReference), pSp->pColorAttachments);
        vktrace_finalize_buffer_address(pHeader, (void**)&(pSubpass->pColorAttachments));
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pSubpass->pResolveAttachments), pSubpass->colorCount * sizeof(VkAttachmentReference), pSp->pResolveAttachments);
        vktrace_finalize_buffer_address(pHeader, (void**)&(pSubpass->pResolveAttachments));
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pSubpass->pPreserveAttachments), pSubpass->preserveCount * sizeof(VkAttachmentReference), pSp->pPreserveAttachments);
        vktrace_finalize_buffer_address(pHeader, (void**)&(pSubpass->pPreserveAttachments));
    }
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRenderPass), sizeof(VkRenderPass), pRenderPass);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pAttachments));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pDependencies));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pSubpasses));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRenderPass));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetGlobalExtensionProperties(
    const char* pLayerName,
    uint32_t* pCount,
    VkExtensionProperties* pProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetGlobalExtensionProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    vktrace_platform_thread_once(&gInitOnce, InitTracer);
    if (real_vkGetGlobalExtensionProperties == vkGetGlobalExtensionProperties) {
        vktrace_platform_get_next_lib_sym((void **) &real_vkGetGlobalExtensionProperties,"vkGetGlobalExtensionProperties");
    }
    startTime = vktrace_get_time();
    result = real_vkGetGlobalExtensionProperties(pLayerName, pCount, pProperties);
    endTime = vktrace_get_time();
    CREATE_TRACE_PACKET(vkGetGlobalExtensionProperties, ((pLayerName != NULL) ? strlen(pLayerName) + 1 : 0) + sizeof(uint32_t) + (*pCount * sizeof(VkExtensionProperties)));
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    if (isHooked == FALSE) {
        AttachHooks();
    }
    pPacket = interpret_body_as_vkGetGlobalExtensionProperties(pHeader);

    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pLayerName), ((pLayerName != NULL) ? strlen(pLayerName) + 1 : 0), pLayerName);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), *pCount * sizeof(VkExtensionProperties), pProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pLayerName));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice,
    const char* pLayerName,
    uint32_t* pCount,
    VkExtensionProperties* pProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceExtensionProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    startTime = vktrace_get_time();
    result = real_vkGetPhysicalDeviceExtensionProperties(physicalDevice, pLayerName, pCount, pProperties);
    endTime = vktrace_get_time();
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceExtensionProperties, ((pLayerName != NULL) ? strlen(pLayerName) + 1 : 0) + sizeof(uint32_t) + (*pCount * sizeof(VkExtensionProperties)));
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetPhysicalDeviceExtensionProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pLayerName), ((pLayerName != NULL) ? strlen(pLayerName) + 1 : 0), pLayerName);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), *pCount * sizeof(VkExtensionProperties), pProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pLayerName));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetGlobalLayerProperties(
    uint32_t* pCount,
    VkLayerProperties* pProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetGlobalLayerProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    vktrace_platform_thread_once(&gInitOnce, InitTracer);
    if (real_vkGetGlobalLayerProperties == vkGetGlobalLayerProperties) {
        vktrace_platform_get_next_lib_sym((void **) &real_vkGetGlobalLayerProperties,"vkGetGlobalLayerProperties");
    }
    startTime = vktrace_get_time();
    result = real_vkGetGlobalLayerProperties(pCount, pProperties);
    endTime = vktrace_get_time();
    CREATE_TRACE_PACKET(vkGetGlobalLayerProperties, sizeof(uint32_t) + (*pCount * sizeof(VkLayerProperties)));
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    if (isHooked == FALSE) {
        AttachHooks();
    }
    pPacket = interpret_body_as_vkGetGlobalLayerProperties(pHeader);

    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), *pCount * sizeof(VkLayerProperties), pProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceLayerProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pCount,
    VkLayerProperties* pProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceLayerProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    startTime = vktrace_get_time();
    result = real_vkGetPhysicalDeviceLayerProperties(physicalDevice, pCount, pProperties);
    endTime = vktrace_get_time();
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceLayerProperties, sizeof(uint32_t) + (*pCount * sizeof(VkLayerProperties)));
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetPhysicalDeviceLayerProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pProperties), *pCount * sizeof(VkLayerProperties), pProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pProperties));
    FINISH_TRACE_PACKET();
    return result;
}
// TODO : This should be pretty easy to fit into codegen. Don't need to make the call prior to creating packet
//  Just need to account for "count" number of queue properties
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice,
    uint32_t* pCount,
    VkQueueFamilyProperties* pQueueFamilyProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceQueueFamilyProperties* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    startTime = vktrace_get_time();
    result = real_vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pCount, pQueueFamilyProperties);
    endTime = vktrace_get_time();
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceQueueFamilyProperties, sizeof(uint32_t) + *pCount * sizeof(VkQueueFamilyProperties));
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetPhysicalDeviceQueueFamilyProperties(pHeader);
    pPacket->physicalDevice = physicalDevice;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pQueueFamilyProperties), *pCount * sizeof(VkQueueFamilyProperties), pQueueFamilyProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pQueueFamilyProperties));
    FINISH_TRACE_PACKET();
    return result;
}

/*
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetGlobalExtensionInfo(
    VkExtensionInfoType infoType,
    uint32_t extensionIndex,
    size_t* pDataSize,
    void* pData)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    packet_vkGetGlobalExtensionInfo* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    vktrace_platform_thread_once(&gInitOnce, InitTracer);
    if (real_vkGetGlobalExtensionInfo == vkGetGlobalExtensionInfo)
    {
        vktrace_platform_get_next_lib_sym((void **) &real_vkGetGlobalExtensionInfo,"vkGetGlobalExtensionInfo");
    }
    startTime = vktrace_get_time();
    result = real_vkGetGlobalExtensionInfo(infoType, extensionIndex, pDataSize, pData);
    endTime = vktrace_get_time();
    CREATE_TRACE_PACKET(vkGetGlobalExtensionInfo, (((pDataSize != NULL) ? sizeof(size_t) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0)));
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    if (isHooked == FALSE) {
        AttachHooks();
    }
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_vkGetGlobalExtensionInfo(pHeader);
    pPacket->infoType = infoType;
    pPacket->extensionIndex = extensionIndex;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}
*/

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkEnumeratePhysicalDevices(
    VkInstance instance,
    uint32_t* pPhysicalDeviceCount,
    VkPhysicalDevice* pPhysicalDevices)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkEnumeratePhysicalDevices* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    //TODO make sure can handle being called twice with pPD == 0
    SEND_ENTRYPOINT_ID(vkEnumeratePhysicalDevices);
    startTime = vktrace_get_time();
    result = real_vkEnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    endTime = vktrace_get_time();
    CREATE_TRACE_PACKET(vkEnumeratePhysicalDevices, sizeof(uint32_t) + ((pPhysicalDevices && pPhysicalDeviceCount) ? *pPhysicalDeviceCount * sizeof(VkPhysicalDevice) : 0));
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkEnumeratePhysicalDevices(pHeader);
    pPacket->instance = instance;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPhysicalDeviceCount), sizeof(uint32_t), pPhysicalDeviceCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPhysicalDevices), *pPhysicalDeviceCount*sizeof(VkPhysicalDevice), pPhysicalDevices);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPhysicalDeviceCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPhysicalDevices));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetQueryPoolResults(
    VkDevice device,
    VkQueryPool queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    size_t* pDataSize,
    void* pData,
    VkQueryResultFlags flags)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    packet_vkGetQueryPoolResults* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    startTime = vktrace_get_time();
    result = real_vkGetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags);
    endTime = vktrace_get_time();
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    CREATE_TRACE_PACKET(vkGetQueryPoolResults, ((pDataSize != NULL) ? sizeof(size_t) : 0) + _dataSize);
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetQueryPoolResults(pHeader);
    pPacket->device = device;
    pPacket->queryPool = queryPool;
    pPacket->startQuery = startQuery;
    pPacket->queryCount = queryCount;
    pPacket->flags = flags;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkAllocDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetUsage setUsage,
    uint32_t count,
    const VkDescriptorSetLayout* pSetLayouts,
    VkDescriptorSet* pDescriptorSets)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkAllocDescriptorSets* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    SEND_ENTRYPOINT_ID(vkAllocDescriptorSets);
    startTime = vktrace_get_time();
    result = real_vkAllocDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets);
    endTime = vktrace_get_time();
    CREATE_TRACE_PACKET(vkAllocDescriptorSets, (count * sizeof(VkDescriptorSetLayout)) + (count * sizeof(VkDescriptorSet)));
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkAllocDescriptorSets(pHeader);
    pPacket->device = device;
    pPacket->descriptorPool = descriptorPool;
    pPacket->setUsage = setUsage;
    pPacket->count = count;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayouts), count * sizeof(VkDescriptorSetLayout), pSetLayouts);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), count * sizeof(VkDescriptorSet), pDescriptorSets);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetLayouts));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSets));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkUpdateDescriptorSets(
VkDevice device,
        uint32_t writeCount, 
        const VkWriteDescriptorSet* pDescriptorWrites, 
        uint32_t copyCount, 
        const VkCopyDescriptorSet* pDescriptorCopies);
// Manually written because it needs to use get_struct_chain_size and allocate some extra pointers (why?)
// Also since it needs to app the array of pointers and sub-buffers (see comments in function)
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkUpdateDescriptorSets(
    VkDevice device,
    uint32_t writeCount,
    const VkWriteDescriptorSet* pDescriptorWrites,
    uint32_t copyCount,
    const VkCopyDescriptorSet* pDescriptorCopies )
{
    vktrace_trace_packet_header* pHeader;
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
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkUpdateDescriptorSets(pHeader);
    pPacket->device = device;
    pPacket->writeCount = writeCount;
    // begin custom code
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorWrites), writeCount * sizeof(VkWriteDescriptorSet), pDescriptorWrites);
    for (i = 0; i < writeCount; i++)
    {
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorWrites[i].pDescriptors), pDescriptorWrites[i].count * sizeof(VkDescriptorInfo), pDescriptorWrites[i].pDescriptors);
        vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorWrites[i].pDescriptors));
    }
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorWrites));

    pPacket->copyCount = copyCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorCopies), copyCount * sizeof(VkCopyDescriptorSet), pDescriptorCopies);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorCopies));
    // end custom code
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdWaitEvents(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        destStageMask,
    uint32_t                                    memBarrierCount,
    const void* const*                          ppMemBarriers)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdWaitEvents* pPacket = NULL;
    size_t customSize;
    customSize = (eventCount * sizeof(VkEvent)) + memBarrierCount * sizeof(void*) + calculate_memory_barrier_size(memBarrierCount, ppMemBarriers);
    CREATE_TRACE_PACKET(vkCmdWaitEvents, customSize);
    real_vkCmdWaitEvents(cmdBuffer, eventCount, pEvents, srcStageMask, destStageMask, memBarrierCount, ppMemBarriers);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdWaitEvents(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->eventCount = eventCount;
    pPacket->srcStageMask = srcStageMask;
    pPacket->destStageMask = destStageMask;
    pPacket->memBarrierCount = memBarrierCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pEvents), eventCount * sizeof(VkEvent), pEvents);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pEvents));
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers), memBarrierCount * sizeof(void*), ppMemBarriers);
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
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers[i]), siz, ppMemBarriers[i]);
        vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers[i]));
    }
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers));
    FINISH_TRACE_PACKET();
}

VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdPipelineBarrier(
    VkCmdBuffer                                 cmdBuffer,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        destStageMask,
    VkBool32                                    byRegion,
    uint32_t                                    memBarrierCount,
    const void* const*                          ppMemBarriers)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdPipelineBarrier* pPacket = NULL;
    size_t customSize;
    customSize = (memBarrierCount * sizeof(void*)) + calculate_memory_barrier_size(memBarrierCount, ppMemBarriers);
    CREATE_TRACE_PACKET(vkCmdPipelineBarrier, customSize);
    real_vkCmdPipelineBarrier(cmdBuffer, srcStageMask, destStageMask, byRegion, memBarrierCount, ppMemBarriers);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdPipelineBarrier(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcStageMask = srcStageMask;
    pPacket->destStageMask = destStageMask;
    pPacket->byRegion = byRegion;
    pPacket->memBarrierCount = memBarrierCount;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers), memBarrierCount * sizeof(void*), ppMemBarriers);
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
        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppMemBarriers[i]), siz, ppMemBarriers[i]);
        vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers[i]));
    }
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->ppMemBarriers));
    FINISH_TRACE_PACKET();
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateGraphicsPipelines* pPacket = NULL;
    size_t total_size = 0;
    uint32_t i;
    for (i = 0; i < count; i++) {
        total_size += get_struct_chain_size((void*)&pCreateInfos[i]);
    }
    CREATE_TRACE_PACKET(vkCreateGraphicsPipelines, total_size + count*sizeof(VkPipeline));
    result = real_vkCreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pPipelines);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateGraphicsPipelines(pHeader);
    pPacket->device = device;
    pPacket->pipelineCache = pipelineCache;
    pPacket->count = count;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfos), count*sizeof(VkGraphicsPipelineCreateInfo), pCreateInfos);
    add_VkGraphicsPipelineCreateInfos_to_trace_packet(pHeader, (VkGraphicsPipelineCreateInfo*)pPacket->pCreateInfos, pCreateInfos, count);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipelines), count*sizeof(VkPipeline), pPipelines);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfos));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipelines));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateComputePipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t count,
    const VkComputePipelineCreateInfo* pCreateInfos,
    VkPipeline* pPipelines)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateComputePipelines* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateComputePipelines, count*sizeof(VkComputePipelineCreateInfo) + sizeof(VkPipeline));
    result = real_vkCreateComputePipelines(device, pipelineCache, count, pCreateInfos, pPipelines);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateComputePipelines(pHeader);
    pPacket->device = device;
    pPacket->pipelineCache = pipelineCache;
    pPacket->count = count;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfos), count*sizeof(VkComputePipelineCreateInfo), pCreateInfos);
    add_VkComputePipelineCreateInfos_to_trace_packet(pHeader, (VkComputePipelineCreateInfo*)pPacket->pCreateInfos, pCreateInfos, count);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipelines), count*sizeof(VkPipeline), pPipelines);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfos));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipelines));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBeginRenderPass(
    VkCmdBuffer cmdBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin,
    VkRenderPassContents contents)
{
    vktrace_trace_packet_header* pHeader;
    packet_vkCmdBeginRenderPass* pPacket = NULL;
    size_t clearValueSize = sizeof(VkClearValue) * pRenderPassBegin->clearValueCount;
    CREATE_TRACE_PACKET(vkCmdBeginRenderPass, sizeof(VkRenderPassBeginInfo) + clearValueSize);
    real_vkCmdBeginRenderPass(cmdBuffer, pRenderPassBegin, contents);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCmdBeginRenderPass(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->contents = contents;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRenderPassBegin), sizeof(VkRenderPassBeginInfo), pRenderPassBegin);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRenderPassBegin->pClearValues), clearValueSize, pRenderPassBegin->pClearValues);
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRenderPassBegin->pClearValues));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pRenderPassBegin));
    FINISH_TRACE_PACKET();
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkFreeDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    uint32_t count,
    const VkDescriptorSet* pDescriptorSets)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkFreeDescriptorSets* pPacket = NULL;
    CREATE_TRACE_PACKET(vkFreeDescriptorSets, count*sizeof(VkDescriptorSet));
    result = real_vkFreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkFreeDescriptorSets(pHeader);
    pPacket->device = device;
    pPacket->descriptorPool = descriptorPool;
    pPacket->count = count;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), count*sizeof(VkDescriptorSet), pDescriptorSets);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSets));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSurfacePropertiesKHR(
    VkDevice device,
    const VkSurfaceDescriptionKHR* pSurfaceDescription,
    VkSurfacePropertiesKHR* pSurfaceProperties)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetSurfacePropertiesKHR* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetSurfacePropertiesKHR, sizeof(VkSurfaceDescriptionKHR) + sizeof(VkSurfacePropertiesKHR));
    result = real_vkGetSurfacePropertiesKHR(device, pSurfaceDescription, pSurfaceProperties);
    pPacket = interpret_body_as_vkGetSurfacePropertiesKHR(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSurfaceDescription), sizeof(VkSurfaceDescriptionKHR), pSurfaceDescription);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSurfaceProperties), sizeof(VkSurfacePropertiesKHR), pSurfaceProperties);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSurfaceDescription));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSurfaceProperties));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSurfaceFormatsKHR(
    VkDevice device,
    const VkSurfaceDescriptionKHR* pSurfaceDescription,
    uint32_t* pCount,
    VkSurfaceFormatKHR* pSurfaceFormats)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    packet_vkGetSurfaceFormatsKHR* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    startTime = vktrace_get_time();
    result = real_vkGetSurfaceFormatsKHR(device, pSurfaceDescription, pCount, pSurfaceFormats);
    endTime = vktrace_get_time();
    _dataSize = (pCount == NULL || pSurfaceFormats == NULL) ? 0 : (*pCount *sizeof(VkSurfaceFormatKHR));
    CREATE_TRACE_PACKET(vkGetSurfaceFormatsKHR, sizeof(VkSurfaceDescriptionKHR) + sizeof(uint32_t) + _dataSize);
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetSurfaceFormatsKHR(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSurfaceDescription), sizeof(VkSurfaceDescriptionKHR), pSurfaceDescription);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSurfaceFormats), _dataSize, pSurfaceFormats);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSurfaceDescription));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSurfaceFormats));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSurfacePresentModesKHR(
    VkDevice device,
    const VkSurfaceDescriptionKHR* pSurfaceDescription,
    uint32_t* pCount,
    VkPresentModeKHR* pPresentModes)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    packet_vkGetSurfacePresentModesKHR* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    startTime = vktrace_get_time();
    result = real_vkGetSurfacePresentModesKHR(device, pSurfaceDescription, pCount, pPresentModes);
    endTime = vktrace_get_time();
    _dataSize = (pCount == NULL || pPresentModes == NULL) ? 0 : (*pCount *sizeof(VkPresentModeKHR));
    CREATE_TRACE_PACKET(vkGetSurfacePresentModesKHR, sizeof(VkSurfaceDescriptionKHR) + sizeof(uint32_t) + _dataSize);
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetSurfacePresentModesKHR(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSurfaceDescription), sizeof(VkSurfaceDescriptionKHR), pSurfaceDescription);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPresentModes), _dataSize, pPresentModes);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSurfaceDescription));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPresentModes));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateSwapchainKHR(
    VkDevice device,
    const VkSwapchainCreateInfoKHR* pCreateInfo,
    VkSwapchainKHR* pSwapchain)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateSwapchainKHR* pPacket = NULL;
    CREATE_TRACE_PACKET(vkCreateSwapchainKHR, vk_ext_khr_device_swapchain_size_vkswapchaincreateinfokhr(pCreateInfo) + sizeof(VkSwapchainKHR));
    result = real_vkCreateSwapchainKHR(device, pCreateInfo, pSwapchain);
    pPacket = interpret_body_as_vkCreateSwapchainKHR(pHeader);
    pPacket->device = device;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(VkSwapchainCreateInfoKHR), pCreateInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pSurfaceDescription), sizeof(VkSurfaceDescriptionKHR), pCreateInfo->pSurfaceDescription);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSwapchain), sizeof(VkSwapchainKHR), pSwapchain);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pQueueFamilyIndices), pCreateInfo->queueFamilyCount * sizeof(uint32_t), pCreateInfo->pQueueFamilyIndices);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pSurfaceDescription));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pQueueFamilyIndices));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSwapchain));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSwapchainImagesKHR(
    VkDevice device,
    VkSwapchainKHR swapchain,
    uint32_t* pCount,
    VkImage* pSwapchainImages)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    size_t _dataSize;
    packet_vkGetSwapchainImagesKHR* pPacket = NULL;
    uint64_t startTime;
    uint64_t endTime;
    uint64_t vktraceStartTime = vktrace_get_time();
    startTime = vktrace_get_time();
    result = real_vkGetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);
    endTime = vktrace_get_time();
    _dataSize = (pCount == NULL || pSwapchainImages == NULL) ? 0 : (*pCount *sizeof(VkImage));
    CREATE_TRACE_PACKET(vkGetSwapchainImagesKHR, sizeof(uint32_t) + _dataSize);
    pHeader->vktrace_begin_time = vktraceStartTime;
    pHeader->entrypoint_begin_time = startTime;
    pHeader->entrypoint_end_time = endTime;
    pPacket = interpret_body_as_vkGetSwapchainImagesKHR(pHeader);
    pPacket->device = device;
    pPacket->swapchain = swapchain;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSwapchainImages), _dataSize, pSwapchainImages);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSwapchainImages));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueuePresentKHR(
    VkQueue queue,
    VkPresentInfoKHR* pPresentInfo)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkQueuePresentKHR* pPacket = NULL;
    size_t swapchainSize = pPresentInfo->swapchainCount*sizeof(VkSwapchainKHR);
    size_t indexSize = pPresentInfo->swapchainCount*sizeof(uint32_t);
    CREATE_TRACE_PACKET(vkQueuePresentKHR, sizeof(VkPresentInfoKHR)+swapchainSize+indexSize);
    result = real_vkQueuePresentKHR(queue, pPresentInfo);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkQueuePresentKHR(pHeader);
    pPacket->queue = queue;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPresentInfo), sizeof(VkPresentInfoKHR), pPresentInfo);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPresentInfo->swapchains), swapchainSize, pPresentInfo->swapchains);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPresentInfo->imageIndices), indexSize, pPresentInfo->imageIndices);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPresentInfo->imageIndices));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPresentInfo->swapchains));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pPresentInfo));
    FINISH_TRACE_PACKET();
    return result;
}

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicStencilState(
    VkDevice device,
    const VkDynamicStencilStateCreateInfo* pCreateInfoFront,
    const VkDynamicStencilStateCreateInfo* pCreateInfoBack,
    VkDynamicStencilState* pState)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkCreateDynamicStencilState* pPacket = NULL;

    /* If front and back pointers are the same, only track front */
    const VkDynamicStencilStateCreateInfo* pLocalCreateInfoBack = (pCreateInfoFront == pCreateInfoBack) ? NULL : pCreateInfoBack;
    uint32_t createInfoMultiplier = (pLocalCreateInfoBack != NULL) ? 2 : 1;

    CREATE_TRACE_PACKET(vkCreateDynamicStencilState, createInfoMultiplier * sizeof(VkDynamicStencilStateCreateInfo) + sizeof(VkDynamicStencilState));
    result = real_vkCreateDynamicStencilState(device, pCreateInfoFront, pCreateInfoBack, pState);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkCreateDynamicStencilState(pHeader);
    pPacket->device = device;


    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfoFront), sizeof(VkDynamicStencilStateCreateInfo), pCreateInfoFront);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfoBack), sizeof(VkDynamicStencilStateCreateInfo), pLocalCreateInfoBack);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(VkDynamicStencilState), pState);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfoFront));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfoBack));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

/* TODO: Probably want to make this manual to get the result of the boolean and then check it on replay
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice physicalDevice,
    uint32_t queueFamilyIndex,
    const VkSurfaceDescriptionKHR* pSurfaceDescription,
    VkBool32* pSupported)
{
    vktrace_trace_packet_header* pHeader;
    VkResult result;
    packet_vkGetPhysicalDeviceSurfaceSupportKHR* pPacket = NULL;
    CREATE_TRACE_PACKET(vkGetPhysicalDeviceSurfaceSupportKHR, sizeof(VkSurfaceDescriptionKHR) + sizeof(VkBool32));
    result = real_vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, pSurfaceDescription, pSupported);
    vktrace_set_packet_entrypoint_end_time(pHeader);
    pPacket = interpret_body_as_vkGetPhysicalDeviceSurfaceSupportKHR(pHeader);
    pPacket->physicalDevice = physicalDevice;
    pPacket->queueFamilyIndex = queueFamilyIndex;
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSurfaceDescription), sizeof(VkSurfaceDescriptionKHR), pSurfaceDescription);
    vktrace_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSupported), sizeof(VkBool32), pSupported);
    pPacket->result = result;
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSurfaceDescription));
    vktrace_finalize_buffer_address(pHeader, (void**)&(pPacket->pSupported));
    FINISH_TRACE_PACKET();
    return result;
}
*/
