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
#include "glvtrace_vk_helpers.h"
#include "glvtrace_vk_vk.h"
#include "glvtrace_vk_vkdbg.h"
#include "glvtrace_vk_vkwsix11ext.h"
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

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateInstance(
    const VkInstanceCreateInfo* pCreateInfo,
    VkInstance* pInstance)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkCreateInstance* pPacket = NULL;
    uint64_t startTime;
    glv_platform_thread_once(&gInitOnce, InitTracer);
    SEND_ENTRYPOINT_ID(vkCreateInstance);
    if (real_vkCreateInstance == vkCreateInstance)
    {
        glv_platform_get_next_lib_sym((void **) &real_vkCreateInstance,"vkCreateInstance");
    }
    startTime = glv_get_time();
    result = real_vkCreateInstance(pCreateInfo, pInstance);
    CREATE_TRACE_PACKET(vkCreateInstance, sizeof(VkInstance) + get_struct_chain_size((void*)pCreateInfo));
    pHeader->entrypoint_begin_time = startTime;
    if (isHooked == FALSE) {
        AttachHooks();
        AttachHooks_vkdbg();
        AttachHooks_vkwsix11ext();
    }
    pPacket = interpret_body_as_vkCreateInstance(pHeader);

    add_VkInstanceCreateInfo_to_packet(pHeader, (VkInstanceCreateInfo**)&(pPacket->pCreateInfo), (VkInstanceCreateInfo*) pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pInstance), sizeof(VkInstance), pInstance);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pInstance));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkEnumerateLayers(
    VkPhysicalGpu gpu,
    size_t maxLayerCount,
    size_t maxStringSize,
    size_t* pOutLayerCount,
    char* const* pOutLayers,
    void* pReserved)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkEnumerateLayers* pPacket = NULL;
    uint64_t startTime;
    SEND_ENTRYPOINT_ID(vkEnumerateLayers);
    startTime = glv_get_time();
    result = real_vkEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
    size_t totStringSize = 0;
    uint32_t i = 0;
    for (i = 0; i < *pOutLayerCount; i++) {
        totStringSize += (pOutLayers[i] != NULL) ? strlen(pOutLayers[i]) + 1: 0;
    }
    CREATE_TRACE_PACKET(vkEnumerateLayers, totStringSize + sizeof(size_t));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_vkEnumerateLayers(pHeader);
    pPacket->gpu = gpu;
    pPacket->maxLayerCount = maxLayerCount;
    pPacket->maxStringSize = maxStringSize;
    pPacket->pReserved = pReserved;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOutLayerCount), sizeof(size_t), pOutLayerCount);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOutLayerCount));
    for (i = 0; i < *pOutLayerCount; i++) {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOutLayers[i]), ((pOutLayers[i] != NULL) ? strlen(pOutLayers[i]) + 1 : 0), pOutLayers[i]);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOutLayers[i]));
    }
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkEnumerateGpus(
    VkInstance instance,
    uint32_t maxGpus,
    uint32_t* pGpuCount,
    VkPhysicalGpu* pGpus)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkEnumerateGpus* pPacket = NULL;
    uint64_t startTime;
    SEND_ENTRYPOINT_ID(vkEnumerateGpus);
    startTime = glv_get_time();
    result = real_vkEnumerateGpus(instance, maxGpus, pGpuCount, pGpus);
    CREATE_TRACE_PACKET(vkEnumerateGpus, sizeof(uint32_t) + ((pGpus && pGpuCount) ? *pGpuCount * sizeof(VkPhysicalGpu) : 0));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_vkEnumerateGpus(pHeader);
    pPacket->instance = instance;
    pPacket->maxGpus = maxGpus;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpuCount), sizeof(uint32_t), pGpuCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpus), *pGpuCount*sizeof(VkPhysicalGpu), pGpus);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pGpuCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pGpus));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkAllocDescriptorSets(
    VkDescriptorPool descriptorPool,
    VkDescriptorSetUsage setUsage,
    uint32_t count,
    const VkDescriptorSetLayout* pSetLayouts,
    VkDescriptorSet* pDescriptorSets,
    uint32_t* pCount)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkAllocDescriptorSets* pPacket = NULL;
    uint64_t startTime;
    SEND_ENTRYPOINT_ID(vkAllocDescriptorSets);
    startTime = glv_get_time();
    result = real_vkAllocDescriptorSets(descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    size_t customSize = (*pCount <= 0) ? (sizeof(VkDescriptorSet)) : (*pCount * sizeof(VkDescriptorSet));
    CREATE_TRACE_PACKET(vkAllocDescriptorSets, sizeof(VkDescriptorSetLayout) + customSize + sizeof(uint32_t));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_vkAllocDescriptorSets(pHeader);
    pPacket->descriptorPool = descriptorPool;
    pPacket->setUsage = setUsage;
    pPacket->count = count;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayouts), count*sizeof(VkDescriptorSetLayout), pSetLayouts);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), customSize, pDescriptorSets);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetLayouts));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSets));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkMapMemory(
    VkGpuMemory mem,
    VkFlags flags,
    void** ppData)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkMapMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(vkMapMemory, sizeof(void*));
    result = real_vkMapMemory(mem, flags, ppData);
    pPacket = interpret_body_as_vkMapMemory(pHeader);
    pPacket->mem = mem;
    pPacket->flags = flags;
    if (ppData != NULL)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppData), sizeof(void*), *ppData);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData));
        add_data_to_mem_info(mem, *ppData);
    }
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT VkResult VKAPI __HOOKED_vkUnmapMemory(VkGpuMemory mem)
{
    glv_trace_packet_header* pHeader;
    VkResult result;
    struct_vkUnmapMemory* pPacket;
    VKAllocInfo *entry;
    SEND_ENTRYPOINT_PARAMS("vkUnmapMemory(mem %p)\n", mem);
    // insert into packet the data that was written by CPU between the vkMapMemory call and here
    // Note must do this prior to the real vkUnMap() or else may get a FAULT
    glv_enter_critical_section(&g_memInfoLock);
    entry = find_mem_info_entry(mem);
    CREATE_TRACE_PACKET(vkUnmapMemory, (entry) ? entry->size : 0);
    pPacket = interpret_body_as_vkUnmapMemory(pHeader);
    if (entry)
    {
        assert(entry->handle == mem);
        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pData), entry->size, entry->pData);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
        entry->pData = NULL;
    } else
    {
         glv_LogError("Failed to copy app memory into trace packet (idx = %u) on vkUnmapMemory\n", pHeader->global_packet_index);
    }
    glv_leave_critical_section(&g_memInfoLock);
//    glv_LogError("manual address of vkUnmapMemory: %p\n", real_vkUnmapMemory);
    result = real_vkUnmapMemory(mem);
    pPacket->mem = mem;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT void VKAPI __HOOKED_vkCmdWaitEvents(
    VkCmdBuffer cmdBuffer,
    const VkEventWaitInfo* pWaitInfo)
{
    glv_trace_packet_header* pHeader;
    struct_vkCmdWaitEvents* pPacket = NULL;
    size_t customSize;
    uint32_t eventCount = (pWaitInfo != NULL && pWaitInfo->pEvents != NULL) ? pWaitInfo->eventCount : 0;
    uint32_t mbCount = (pWaitInfo != NULL && pWaitInfo->ppMemBarriers != NULL) ? pWaitInfo->memBarrierCount : 0;
    customSize = (eventCount * sizeof(VkEvent)) + mbCount * sizeof(void*) + calculate_memory_barrier_size(mbCount, pWaitInfo->ppMemBarriers);
    CREATE_TRACE_PACKET(vkCmdWaitEvents, sizeof(VkEventWaitInfo) + customSize);
    real_vkCmdWaitEvents(cmdBuffer, pWaitInfo);
    pPacket = interpret_body_as_vkCmdWaitEvents(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pWaitInfo), sizeof(VkEventWaitInfo), pWaitInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pWaitInfo->pEvents), eventCount * sizeof(VkEvent), pWaitInfo->pEvents);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pWaitInfo->pEvents));
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pWaitInfo->ppMemBarriers), mbCount * sizeof(void*), pWaitInfo->ppMemBarriers);
    uint32_t i, siz;
    for (i = 0; i < mbCount; i++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pWaitInfo->ppMemBarriers[i];
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
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pWaitInfo->ppMemBarriers[i]), siz, pWaitInfo->ppMemBarriers[i]);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pWaitInfo->ppMemBarriers[i]));
    }
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pWaitInfo->ppMemBarriers));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pWaitInfo));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void VKAPI __HOOKED_vkCmdPipelineBarrier(
    VkCmdBuffer cmdBuffer,
    const VkPipelineBarrier* pBarrier)
{
    glv_trace_packet_header* pHeader;
    struct_vkCmdPipelineBarrier* pPacket = NULL;
    size_t customSize;
    uint32_t eventCount = (pBarrier != NULL && pBarrier->pEvents != NULL) ? pBarrier->eventCount : 0;
    uint32_t mbCount = (pBarrier != NULL && pBarrier->ppMemBarriers != NULL) ? pBarrier->memBarrierCount : 0;
    customSize = (eventCount * sizeof(VkPipeEvent)) + mbCount * sizeof(void*) + calculate_memory_barrier_size(mbCount, pBarrier->ppMemBarriers);
    CREATE_TRACE_PACKET(vkCmdPipelineBarrier, sizeof(VkPipelineBarrier) + customSize);
    real_vkCmdPipelineBarrier(cmdBuffer, pBarrier);
    pPacket = interpret_body_as_vkCmdPipelineBarrier(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBarrier), sizeof(VkPipelineBarrier), pBarrier);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBarrier->pEvents), eventCount * sizeof(VkPipeEvent), pBarrier->pEvents);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBarrier->pEvents));
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBarrier->ppMemBarriers), mbCount * sizeof(void*), pBarrier->ppMemBarriers);
    uint32_t i, siz;
    for (i = 0; i < mbCount; i++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pBarrier->ppMemBarriers[i];
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
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBarrier->ppMemBarriers[i]), siz, pBarrier->ppMemBarriers[i]);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBarrier->ppMemBarriers[i]));
    }
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBarrier->ppMemBarriers));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBarrier));
    FINISH_TRACE_PACKET();
}
