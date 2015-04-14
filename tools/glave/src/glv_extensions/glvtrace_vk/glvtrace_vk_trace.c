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

#include "xgl.h"
#include "glv_platform.h"
#include "glv_common.h"
#include "glvtrace_xgl_helpers.h"
#include "glvtrace_xgl_xgl.h"
#include "glvtrace_xgl_xgldbg.h"
#include "glvtrace_xgl_xglwsix11ext.h"
#include "glv_interconnect.h"
#include "glv_filelike.h"
#ifdef WIN32
#include "mhook/mhook-lib/mhook.h"
#endif
#include "glv_trace_packet_utils.h"
#include <stdio.h>

// declared as extern in glvtrace_xgl_helpers.h
GLV_CRITICAL_SECTION g_memInfoLock;
XGLMemInfo g_memInfo = {0, NULL, NULL, 0};

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateInstance(
    const XGL_INSTANCE_CREATE_INFO* pCreateInfo,
    XGL_INSTANCE* pInstance)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateInstance* pPacket = NULL;
    uint64_t startTime;
    glv_platform_thread_once(&gInitOnce, InitTracer);
    SEND_ENTRYPOINT_ID(xglCreateInstance);
    if (real_xglCreateInstance == xglCreateInstance)
    {
        glv_platform_get_next_lib_sym((void **) &real_xglCreateInstance,"xglCreateInstance");
    }
    startTime = glv_get_time();
    result = real_xglCreateInstance(pCreateInfo, pInstance);
    CREATE_TRACE_PACKET(xglCreateInstance, sizeof(XGL_INSTANCE) + get_struct_chain_size((void*)pCreateInfo));
    pHeader->entrypoint_begin_time = startTime;
    if (isHooked == FALSE) {
        AttachHooks();
        AttachHooks_xgldbg();
        AttachHooks_xglwsix11ext();
    }
    pPacket = interpret_body_as_xglCreateInstance(pHeader);

    add_XGL_INSTANCE_CREATE_INFO_to_packet(pHeader, (XGL_INSTANCE_CREATE_INFO**)&(pPacket->pCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pInstance), sizeof(XGL_INSTANCE), pInstance);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pInstance));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglEnumerateLayers(
    XGL_PHYSICAL_GPU gpu,
    size_t maxLayerCount,
    size_t maxStringSize,
    size_t* pOutLayerCount,
    char* const* pOutLayers,
    void* pReserved)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglEnumerateLayers* pPacket = NULL;
    uint64_t startTime;
    SEND_ENTRYPOINT_ID(xglEnumerateLayers);
    startTime = glv_get_time();
    result = real_xglEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
    size_t totStringSize = 0;
    uint32_t i = 0;
    for (i = 0; i < *pOutLayerCount; i++) {
        totStringSize += (pOutLayers[i] != NULL) ? strlen(pOutLayers[i]) + 1: 0;
    }
    CREATE_TRACE_PACKET(xglEnumerateLayers, totStringSize + sizeof(size_t));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_xglEnumerateLayers(pHeader);
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

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglEnumerateGpus(
    XGL_INSTANCE instance,
    uint32_t maxGpus,
    uint32_t* pGpuCount,
    XGL_PHYSICAL_GPU* pGpus)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglEnumerateGpus* pPacket = NULL;
    uint64_t startTime;
    SEND_ENTRYPOINT_ID(xglEnumerateGpus);
    startTime = glv_get_time();
    result = real_xglEnumerateGpus(instance, maxGpus, pGpuCount, pGpus);
    CREATE_TRACE_PACKET(xglEnumerateGpus, sizeof(uint32_t) + ((pGpus && pGpuCount) ? *pGpuCount * sizeof(XGL_PHYSICAL_GPU) : 0));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_xglEnumerateGpus(pHeader);
    pPacket->instance = instance;
    pPacket->maxGpus = maxGpus;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpuCount), sizeof(uint32_t), pGpuCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpus), *pGpuCount*sizeof(XGL_PHYSICAL_GPU), pGpus);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pGpuCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pGpus));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglAllocDescriptorSets(
    XGL_DESCRIPTOR_POOL descriptorPool,
    XGL_DESCRIPTOR_SET_USAGE setUsage,
    uint32_t count,
    const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayouts,
    XGL_DESCRIPTOR_SET* pDescriptorSets,
    uint32_t* pCount)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglAllocDescriptorSets* pPacket = NULL;
    uint64_t startTime;
    SEND_ENTRYPOINT_ID(xglAllocDescriptorSets);
    startTime = glv_get_time();
    result = real_xglAllocDescriptorSets(descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    size_t customSize = (*pCount <= 0) ? (sizeof(XGL_DESCRIPTOR_SET)) : (*pCount * sizeof(XGL_DESCRIPTOR_SET));
    CREATE_TRACE_PACKET(xglAllocDescriptorSets, sizeof(XGL_DESCRIPTOR_SET_LAYOUT) + customSize + sizeof(uint32_t));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_xglAllocDescriptorSets(pHeader);
    pPacket->descriptorPool = descriptorPool;
    pPacket->setUsage = setUsage;
    pPacket->count = count;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayouts), count*sizeof(XGL_DESCRIPTOR_SET_LAYOUT), pSetLayouts);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), customSize, pDescriptorSets);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(uint32_t), pCount);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetLayouts));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSets));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglMapMemory(
    XGL_GPU_MEMORY mem,
    XGL_FLAGS flags,
    void** ppData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglMapMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(xglMapMemory, sizeof(void*));
    result = real_xglMapMemory(mem, flags, ppData);
    pPacket = interpret_body_as_xglMapMemory(pHeader);
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

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglUnmapMemory(XGL_GPU_MEMORY mem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglUnmapMemory* pPacket;
    XGLAllocInfo *entry;
    SEND_ENTRYPOINT_PARAMS("xglUnmapMemory(mem %p)\n", mem);
    // insert into packet the data that was written by CPU between the xglMapMemory call and here
    // Note must do this prior to the real xglUnMap() or else may get a FAULT
    glv_enter_critical_section(&g_memInfoLock);
    entry = find_mem_info_entry(mem);
    CREATE_TRACE_PACKET(xglUnmapMemory, (entry) ? entry->size : 0);
    pPacket = interpret_body_as_xglUnmapMemory(pHeader);
    if (entry)
    {
        assert(entry->handle == mem);
        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pData), entry->size, entry->pData);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
        entry->pData = NULL;
    } else
    {
         glv_LogError("Failed to copy app memory into trace packet (idx = %u) on xglUnmapMemory\n", pHeader->global_packet_index);
    }
    glv_leave_critical_section(&g_memInfoLock);
//    glv_LogError("manual address of xglUnmapMemory: %p\n", real_xglUnmapMemory);
    result = real_xglUnmapMemory(mem);
    pPacket->mem = mem;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdWaitEvents(
    XGL_CMD_BUFFER cmdBuffer,
    const XGL_EVENT_WAIT_INFO* pWaitInfo)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdWaitEvents* pPacket = NULL;
    size_t customSize;
    uint32_t eventCount = (pWaitInfo != NULL && pWaitInfo->pEvents != NULL) ? pWaitInfo->eventCount : 0;
    uint32_t mbCount = (pWaitInfo != NULL && pWaitInfo->ppMemBarriers != NULL) ? pWaitInfo->memBarrierCount : 0;
    customSize = (eventCount * sizeof(XGL_EVENT)) + mbCount * sizeof(void*) + calculate_memory_barrier_size(mbCount, pWaitInfo->ppMemBarriers);
    CREATE_TRACE_PACKET(xglCmdWaitEvents, sizeof(XGL_EVENT_WAIT_INFO) + customSize);
    real_xglCmdWaitEvents(cmdBuffer, pWaitInfo);
    pPacket = interpret_body_as_xglCmdWaitEvents(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pWaitInfo), sizeof(XGL_EVENT_WAIT_INFO), pWaitInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pWaitInfo->pEvents), eventCount * sizeof(XGL_EVENT), pWaitInfo->pEvents);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pWaitInfo->pEvents));
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pWaitInfo->ppMemBarriers), mbCount * sizeof(void*), pWaitInfo->ppMemBarriers);
    uint32_t i, siz;
    for (i = 0; i < mbCount; i++) {
        XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pWaitInfo->ppMemBarriers[i];
        switch (pNext->sType) {
            case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:
                siz = sizeof(XGL_MEMORY_BARRIER);
                break;
            case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                siz = sizeof(XGL_BUFFER_MEMORY_BARRIER);
                break;
            case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                siz = sizeof(XGL_IMAGE_MEMORY_BARRIER);
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

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdPipelineBarrier(
    XGL_CMD_BUFFER cmdBuffer,
    const XGL_PIPELINE_BARRIER* pBarrier)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdPipelineBarrier* pPacket = NULL;
    size_t customSize;
    uint32_t eventCount = (pBarrier != NULL && pBarrier->pEvents != NULL) ? pBarrier->eventCount : 0;
    uint32_t mbCount = (pBarrier != NULL && pBarrier->ppMemBarriers != NULL) ? pBarrier->memBarrierCount : 0;
    customSize = (eventCount * sizeof(XGL_PIPE_EVENT)) + mbCount * sizeof(void*) + calculate_memory_barrier_size(mbCount, pBarrier->ppMemBarriers);
    CREATE_TRACE_PACKET(xglCmdPipelineBarrier, sizeof(XGL_PIPELINE_BARRIER) + customSize);
    real_xglCmdPipelineBarrier(cmdBuffer, pBarrier);
    pPacket = interpret_body_as_xglCmdPipelineBarrier(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBarrier), sizeof(XGL_PIPELINE_BARRIER), pBarrier);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBarrier->pEvents), eventCount * sizeof(XGL_PIPE_EVENT), pBarrier->pEvents);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBarrier->pEvents));
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBarrier->ppMemBarriers), mbCount * sizeof(void*), pBarrier->ppMemBarriers);
    uint32_t i, siz;
    for (i = 0; i < mbCount; i++) {
        XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pBarrier->ppMemBarriers[i];
        switch (pNext->sType) {
            case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:
                siz = sizeof(XGL_MEMORY_BARRIER);
                break;
            case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                siz = sizeof(XGL_BUFFER_MEMORY_BARRIER);
                break;
            case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                siz = sizeof(XGL_IMAGE_MEMORY_BARRIER);
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
