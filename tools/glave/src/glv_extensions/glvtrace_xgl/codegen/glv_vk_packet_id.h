/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * XGL
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

#pragma once

#include "glv_trace_packet_utils.h"
#include "glv_trace_packet_identifiers.h"
#include "glv_interconnect.h"
#include "glv_vk_vk_structs.h"
#include "glv_vk_vkdbg_structs.h"
#include "glv_vk_vkwsix11ext_structs.h"
#include "xgl_enum_string_helper.h"
#if defined(WIN32)
#define snprintf _snprintf
#endif
#define SEND_ENTRYPOINT_ID(entrypoint) ;
//#define SEND_ENTRYPOINT_ID(entrypoint) glv_TraceInfo(#entrypoint "\n");

#define SEND_ENTRYPOINT_PARAMS(entrypoint, ...) ;
//#define SEND_ENTRYPOINT_PARAMS(entrypoint, ...) glv_TraceInfo(entrypoint, __VA_ARGS__);

#define CREATE_TRACE_PACKET(entrypoint, buffer_bytes_needed) \
    pHeader = glv_create_trace_packet(GLV_TID_XGL, GLV_TPI_XGL_##entrypoint, sizeof(struct_##entrypoint), buffer_bytes_needed);

#define FINISH_TRACE_PACKET() \
    glv_finalize_trace_packet(pHeader); \
    glv_write_trace_packet(pHeader, glv_trace_get_trace_file()); \
    glv_delete_trace_packet(&pHeader);

enum GLV_TRACE_PACKET_ID_XGL
{
    GLV_TPI_XGL_xglApiVersion = GLV_TPI_BEGIN_API_HERE,
    GLV_TPI_XGL_xglCreateInstance,
    GLV_TPI_XGL_xglDestroyInstance,
    GLV_TPI_XGL_xglEnumerateGpus,
    GLV_TPI_XGL_xglGetGpuInfo,
    GLV_TPI_XGL_xglGetProcAddr,
    GLV_TPI_XGL_xglCreateDevice,
    GLV_TPI_XGL_xglDestroyDevice,
    GLV_TPI_XGL_xglGetExtensionSupport,
    GLV_TPI_XGL_xglEnumerateLayers,
    GLV_TPI_XGL_xglGetDeviceQueue,
    GLV_TPI_XGL_xglQueueSubmit,
    GLV_TPI_XGL_xglQueueSetGlobalMemReferences,
    GLV_TPI_XGL_xglQueueWaitIdle,
    GLV_TPI_XGL_xglDeviceWaitIdle,
    GLV_TPI_XGL_xglAllocMemory,
    GLV_TPI_XGL_xglFreeMemory,
    GLV_TPI_XGL_xglSetMemoryPriority,
    GLV_TPI_XGL_xglMapMemory,
    GLV_TPI_XGL_xglUnmapMemory,
    GLV_TPI_XGL_xglPinSystemMemory,
    GLV_TPI_XGL_xglGetMultiGpuCompatibility,
    GLV_TPI_XGL_xglOpenSharedMemory,
    GLV_TPI_XGL_xglOpenSharedQueueSemaphore,
    GLV_TPI_XGL_xglOpenPeerMemory,
    GLV_TPI_XGL_xglOpenPeerImage,
    GLV_TPI_XGL_xglDestroyObject,
    GLV_TPI_XGL_xglGetObjectInfo,
    GLV_TPI_XGL_xglBindObjectMemory,
    GLV_TPI_XGL_xglBindObjectMemoryRange,
    GLV_TPI_XGL_xglBindImageMemoryRange,
    GLV_TPI_XGL_xglCreateFence,
    GLV_TPI_XGL_xglGetFenceStatus,
    GLV_TPI_XGL_xglWaitForFences,
    GLV_TPI_XGL_xglCreateQueueSemaphore,
    GLV_TPI_XGL_xglSignalQueueSemaphore,
    GLV_TPI_XGL_xglWaitQueueSemaphore,
    GLV_TPI_XGL_xglCreateEvent,
    GLV_TPI_XGL_xglGetEventStatus,
    GLV_TPI_XGL_xglSetEvent,
    GLV_TPI_XGL_xglResetEvent,
    GLV_TPI_XGL_xglCreateQueryPool,
    GLV_TPI_XGL_xglGetQueryPoolResults,
    GLV_TPI_XGL_xglGetFormatInfo,
    GLV_TPI_XGL_xglCreateBuffer,
    GLV_TPI_XGL_xglCreateBufferView,
    GLV_TPI_XGL_xglCreateImage,
    GLV_TPI_XGL_xglSetFastClearColor,
    GLV_TPI_XGL_xglSetFastClearDepth,
    GLV_TPI_XGL_xglGetImageSubresourceInfo,
    GLV_TPI_XGL_xglCreateImageView,
    GLV_TPI_XGL_xglCreateColorAttachmentView,
    GLV_TPI_XGL_xglCreateDepthStencilView,
    GLV_TPI_XGL_xglCreateShader,
    GLV_TPI_XGL_xglCreateGraphicsPipeline,
    GLV_TPI_XGL_xglCreateComputePipeline,
    GLV_TPI_XGL_xglStorePipeline,
    GLV_TPI_XGL_xglLoadPipeline,
    GLV_TPI_XGL_xglCreatePipelineDelta,
    GLV_TPI_XGL_xglCreateSampler,
    GLV_TPI_XGL_xglCreateDescriptorSetLayout,
    GLV_TPI_XGL_xglBeginDescriptorRegionUpdate,
    GLV_TPI_XGL_xglEndDescriptorRegionUpdate,
    GLV_TPI_XGL_xglCreateDescriptorRegion,
    GLV_TPI_XGL_xglClearDescriptorRegion,
    GLV_TPI_XGL_xglAllocDescriptorSets,
    GLV_TPI_XGL_xglClearDescriptorSets,
    GLV_TPI_XGL_xglUpdateDescriptors,
    GLV_TPI_XGL_xglCreateDynamicViewportState,
    GLV_TPI_XGL_xglCreateDynamicRasterState,
    GLV_TPI_XGL_xglCreateDynamicColorBlendState,
    GLV_TPI_XGL_xglCreateDynamicDepthStencilState,
    GLV_TPI_XGL_xglCreateCommandBuffer,
    GLV_TPI_XGL_xglBeginCommandBuffer,
    GLV_TPI_XGL_xglEndCommandBuffer,
    GLV_TPI_XGL_xglResetCommandBuffer,
    GLV_TPI_XGL_xglCmdBindPipeline,
    GLV_TPI_XGL_xglCmdBindPipelineDelta,
    GLV_TPI_XGL_xglCmdBindDynamicStateObject,
    GLV_TPI_XGL_xglCmdBindDescriptorSet,
    GLV_TPI_XGL_xglCmdBindVertexBuffer,
    GLV_TPI_XGL_xglCmdBindIndexBuffer,
    GLV_TPI_XGL_xglCmdDraw,
    GLV_TPI_XGL_xglCmdDrawIndexed,
    GLV_TPI_XGL_xglCmdDrawIndirect,
    GLV_TPI_XGL_xglCmdDrawIndexedIndirect,
    GLV_TPI_XGL_xglCmdDispatch,
    GLV_TPI_XGL_xglCmdDispatchIndirect,
    GLV_TPI_XGL_xglCmdCopyBuffer,
    GLV_TPI_XGL_xglCmdCopyImage,
    GLV_TPI_XGL_xglCmdCopyBufferToImage,
    GLV_TPI_XGL_xglCmdCopyImageToBuffer,
    GLV_TPI_XGL_xglCmdCloneImageData,
    GLV_TPI_XGL_xglCmdUpdateBuffer,
    GLV_TPI_XGL_xglCmdFillBuffer,
    GLV_TPI_XGL_xglCmdClearColorImage,
    GLV_TPI_XGL_xglCmdClearColorImageRaw,
    GLV_TPI_XGL_xglCmdClearDepthStencil,
    GLV_TPI_XGL_xglCmdResolveImage,
    GLV_TPI_XGL_xglCmdSetEvent,
    GLV_TPI_XGL_xglCmdResetEvent,
    GLV_TPI_XGL_xglCmdWaitEvents,
    GLV_TPI_XGL_xglCmdPipelineBarrier,
    GLV_TPI_XGL_xglCmdBeginQuery,
    GLV_TPI_XGL_xglCmdEndQuery,
    GLV_TPI_XGL_xglCmdResetQueryPool,
    GLV_TPI_XGL_xglCmdWriteTimestamp,
    GLV_TPI_XGL_xglCmdInitAtomicCounters,
    GLV_TPI_XGL_xglCmdLoadAtomicCounters,
    GLV_TPI_XGL_xglCmdSaveAtomicCounters,
    GLV_TPI_XGL_xglCreateFramebuffer,
    GLV_TPI_XGL_xglCreateRenderPass,
    GLV_TPI_XGL_xglCmdBeginRenderPass,
    GLV_TPI_XGL_xglCmdEndRenderPass,
    GLV_TPI_XGL_xglDbgSetValidationLevel,
    GLV_TPI_XGL_xglDbgRegisterMsgCallback,
    GLV_TPI_XGL_xglDbgUnregisterMsgCallback,
    GLV_TPI_XGL_xglDbgSetMessageFilter,
    GLV_TPI_XGL_xglDbgSetObjectTag,
    GLV_TPI_XGL_xglDbgSetGlobalOption,
    GLV_TPI_XGL_xglDbgSetDeviceOption,
    GLV_TPI_XGL_xglCmdDbgMarkerBegin,
    GLV_TPI_XGL_xglCmdDbgMarkerEnd,
    GLV_TPI_XGL_xglWsiX11AssociateConnection,
    GLV_TPI_XGL_xglWsiX11GetMSC,
    GLV_TPI_XGL_xglWsiX11CreatePresentableImage,
    GLV_TPI_XGL_xglWsiX11QueuePresent,
};

static const char *stringify_xgl_packet_id(const enum GLV_TRACE_PACKET_ID_XGL id, const glv_trace_packet_header* pHeader)
{
    static char str[1024];
    switch(id) {
    case GLV_TPI_XGL_xglApiVersion:
    {
        struct_xglApiVersion* pPacket = (struct_xglApiVersion*)(pHeader->pBody);
        snprintf(str, 1024, "xglApiVersion = 0x%x", pPacket->version);
        return str;
    }
    case GLV_TPI_XGL_xglCreateInstance:
    {
        struct_xglCreateInstance* pPacket = (struct_xglCreateInstance*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateInstance(pAppInfo = %p, pAllocCb = %p, pInstance = %p)", (void*)(pPacket->pAppInfo), (void*)(pPacket->pAllocCb), (void*)pPacket->pInstance);
        return str;
    }
    case GLV_TPI_XGL_xglDestroyInstance:
    {
        struct_xglDestroyInstance* pPacket = (struct_xglDestroyInstance*)(pHeader->pBody);
        snprintf(str, 1024, "xglDestroyInstance(instance = %p)", (void*)(pPacket->instance));
        return str;
    }
    case GLV_TPI_XGL_xglEnumerateGpus:
    {
        struct_xglEnumerateGpus* pPacket = (struct_xglEnumerateGpus*)(pHeader->pBody);
        snprintf(str, 1024, "xglEnumerateGpus(instance = %p, maxGpus = %i, *pGpuCount = %i, pGpus = %p)", (void*)(pPacket->instance), pPacket->maxGpus, (pPacket->pGpuCount == NULL) ? 0 : *(pPacket->pGpuCount), (void*)(pPacket->pGpus));
        return str;
    }
    case GLV_TPI_XGL_xglGetGpuInfo:
    {
        struct_xglGetGpuInfo* pPacket = (struct_xglGetGpuInfo*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetGpuInfo(gpu = %p, infoType = %s, *pDataSize = %zu, pData = %p)", (void*)(pPacket->gpu), string_XGL_PHYSICAL_GPU_INFO_TYPE(pPacket->infoType), (pPacket->pDataSize == NULL) ? 0 : *(pPacket->pDataSize), (void*)(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglGetProcAddr:
    {
        struct_xglGetProcAddr* pPacket = (struct_xglGetProcAddr*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetProcAddr(gpu = %p, pName = %p)", (void*)(pPacket->gpu), (void*)(pPacket->pName));
        return str;
    }
    case GLV_TPI_XGL_xglCreateDevice:
    {
        struct_xglCreateDevice* pPacket = (struct_xglCreateDevice*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateDevice(gpu = %p, pCreateInfo = %p, pDevice = %p)", (void*)(pPacket->gpu), (void*)(pPacket->pCreateInfo), (void*)pPacket->pDevice);
        return str;
    }
    case GLV_TPI_XGL_xglDestroyDevice:
    {
        struct_xglDestroyDevice* pPacket = (struct_xglDestroyDevice*)(pHeader->pBody);
        snprintf(str, 1024, "xglDestroyDevice(device = %p)", (void*)(pPacket->device));
        return str;
    }
    case GLV_TPI_XGL_xglGetExtensionSupport:
    {
        struct_xglGetExtensionSupport* pPacket = (struct_xglGetExtensionSupport*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetExtensionSupport(gpu = %p, pExtName = %p)", (void*)(pPacket->gpu), (void*)(pPacket->pExtName));
        return str;
    }
    case GLV_TPI_XGL_xglEnumerateLayers:
    {
        struct_xglEnumerateLayers* pPacket = (struct_xglEnumerateLayers*)(pHeader->pBody);
        snprintf(str, 1024, "xglEnumerateLayers(gpu = %p, maxLayerCount = %zu, maxStringSize = %zu, *pOutLayerCount = %zu, pOutLayers = %p, pReserved = %p)", (void*)(pPacket->gpu), pPacket->maxLayerCount, pPacket->maxStringSize, (pPacket->pOutLayerCount == NULL) ? 0 : *(pPacket->pOutLayerCount), (void*)(pPacket->pOutLayers), (void*)(pPacket->pReserved));
        return str;
    }
    case GLV_TPI_XGL_xglGetDeviceQueue:
    {
        struct_xglGetDeviceQueue* pPacket = (struct_xglGetDeviceQueue*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetDeviceQueue(device = %p, queueType = %s, queueIndex = %i, pQueue = %p)", (void*)(pPacket->device), string_XGL_QUEUE_TYPE(pPacket->queueType), pPacket->queueIndex, (void*)(pPacket->pQueue));
        return str;
    }
    case GLV_TPI_XGL_xglQueueSubmit:
    {
        struct_xglQueueSubmit* pPacket = (struct_xglQueueSubmit*)(pHeader->pBody);
        snprintf(str, 1024, "xglQueueSubmit(queue = %p, cmdBufferCount = %i, pCmdBuffers = %p, memRefCount = %i, pMemRefs = %p, fence = %p)", (void*)(pPacket->queue), pPacket->cmdBufferCount, (void*)(pPacket->pCmdBuffers), pPacket->memRefCount, (void*)(pPacket->pMemRefs), (void*)(pPacket->fence));
        return str;
    }
    case GLV_TPI_XGL_xglQueueSetGlobalMemReferences:
    {
        struct_xglQueueSetGlobalMemReferences* pPacket = (struct_xglQueueSetGlobalMemReferences*)(pHeader->pBody);
        snprintf(str, 1024, "xglQueueSetGlobalMemReferences(queue = %p, memRefCount = %i, pMemRefs = %p)", (void*)(pPacket->queue), pPacket->memRefCount, (void*)(pPacket->pMemRefs));
        return str;
    }
    case GLV_TPI_XGL_xglQueueWaitIdle:
    {
        struct_xglQueueWaitIdle* pPacket = (struct_xglQueueWaitIdle*)(pHeader->pBody);
        snprintf(str, 1024, "xglQueueWaitIdle(queue = %p)", (void*)(pPacket->queue));
        return str;
    }
    case GLV_TPI_XGL_xglDeviceWaitIdle:
    {
        struct_xglDeviceWaitIdle* pPacket = (struct_xglDeviceWaitIdle*)(pHeader->pBody);
        snprintf(str, 1024, "xglDeviceWaitIdle(device = %p)", (void*)(pPacket->device));
        return str;
    }
    case GLV_TPI_XGL_xglAllocMemory:
    {
        struct_xglAllocMemory* pPacket = (struct_xglAllocMemory*)(pHeader->pBody);
        snprintf(str, 1024, "xglAllocMemory(device = %p, pAllocInfo = %p, pMem = %p)", (void*)(pPacket->device), (void*)(pPacket->pAllocInfo), (void*)pPacket->pMem);
        return str;
    }
    case GLV_TPI_XGL_xglFreeMemory:
    {
        struct_xglFreeMemory* pPacket = (struct_xglFreeMemory*)(pHeader->pBody);
        snprintf(str, 1024, "xglFreeMemory(mem = %p)", (void*)(pPacket->mem));
        return str;
    }
    case GLV_TPI_XGL_xglSetMemoryPriority:
    {
        struct_xglSetMemoryPriority* pPacket = (struct_xglSetMemoryPriority*)(pHeader->pBody);
        snprintf(str, 1024, "xglSetMemoryPriority(mem = %p, priority = %p)", (void*)(pPacket->mem), (void*)(pPacket->priority));
        return str;
    }
    case GLV_TPI_XGL_xglMapMemory:
    {
        struct_xglMapMemory* pPacket = (struct_xglMapMemory*)(pHeader->pBody);
        snprintf(str, 1024, "xglMapMemory(mem = %p, flags = %i, ppData = %p)", (void*)(pPacket->mem), pPacket->flags, (void*)pPacket->ppData);
        return str;
    }
    case GLV_TPI_XGL_xglUnmapMemory:
    {
        struct_xglUnmapMemory* pPacket = (struct_xglUnmapMemory*)(pHeader->pBody);
        snprintf(str, 1024, "xglUnmapMemory(mem = %p)", (void*)(pPacket->mem));
        return str;
    }
    case GLV_TPI_XGL_xglPinSystemMemory:
    {
        struct_xglPinSystemMemory* pPacket = (struct_xglPinSystemMemory*)(pHeader->pBody);
        snprintf(str, 1024, "xglPinSystemMemory(device = %p, pSysMem = %p, memSize = %zu, pMem = %p)", (void*)(pPacket->device), (void*)(pPacket->pSysMem), pPacket->memSize, (void*)(pPacket->pMem));
        return str;
    }
    case GLV_TPI_XGL_xglGetMultiGpuCompatibility:
    {
        struct_xglGetMultiGpuCompatibility* pPacket = (struct_xglGetMultiGpuCompatibility*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetMultiGpuCompatibility(gpu0 = %p, gpu1 = %p, pInfo = %p)", (void*)(pPacket->gpu0), (void*)(pPacket->gpu1), (void*)(pPacket->pInfo));
        return str;
    }
    case GLV_TPI_XGL_xglOpenSharedMemory:
    {
        struct_xglOpenSharedMemory* pPacket = (struct_xglOpenSharedMemory*)(pHeader->pBody);
        snprintf(str, 1024, "xglOpenSharedMemory(device = %p, pOpenInfo = %p, pMem = %p)", (void*)(pPacket->device), (void*)(pPacket->pOpenInfo), (void*)(pPacket->pMem));
        return str;
    }
    case GLV_TPI_XGL_xglOpenSharedQueueSemaphore:
    {
        struct_xglOpenSharedQueueSemaphore* pPacket = (struct_xglOpenSharedQueueSemaphore*)(pHeader->pBody);
        snprintf(str, 1024, "xglOpenSharedQueueSemaphore(device = %p, pOpenInfo = %p, pSemaphore = %p)", (void*)(pPacket->device), (void*)(pPacket->pOpenInfo), (void*)(pPacket->pSemaphore));
        return str;
    }
    case GLV_TPI_XGL_xglOpenPeerMemory:
    {
        struct_xglOpenPeerMemory* pPacket = (struct_xglOpenPeerMemory*)(pHeader->pBody);
        snprintf(str, 1024, "xglOpenPeerMemory(device = %p, pOpenInfo = %p, pMem = %p)", (void*)(pPacket->device), (void*)(pPacket->pOpenInfo), (void*)(pPacket->pMem));
        return str;
    }
    case GLV_TPI_XGL_xglOpenPeerImage:
    {
        struct_xglOpenPeerImage* pPacket = (struct_xglOpenPeerImage*)(pHeader->pBody);
        snprintf(str, 1024, "xglOpenPeerImage(device = %p, pOpenInfo = %p, pImage = %p, pMem = %p)", (void*)(pPacket->device), (void*)(pPacket->pOpenInfo), (void*)(pPacket->pImage), (void*)(pPacket->pMem));
        return str;
    }
    case GLV_TPI_XGL_xglDestroyObject:
    {
        struct_xglDestroyObject* pPacket = (struct_xglDestroyObject*)(pHeader->pBody);
        snprintf(str, 1024, "xglDestroyObject(object = %p)", (void*)(pPacket->object));
        return str;
    }
    case GLV_TPI_XGL_xglGetObjectInfo:
    {
        struct_xglGetObjectInfo* pPacket = (struct_xglGetObjectInfo*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetObjectInfo(object = %p, infoType = %s, *pDataSize = %zu, pData = %p)", (void*)(pPacket->object), string_XGL_OBJECT_INFO_TYPE(pPacket->infoType), (pPacket->pDataSize == NULL) ? 0 : *(pPacket->pDataSize), (void*)(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglBindObjectMemory:
    {
        struct_xglBindObjectMemory* pPacket = (struct_xglBindObjectMemory*)(pHeader->pBody);
        snprintf(str, 1024, "xglBindObjectMemory(object = %p, allocationIdx = %i, mem = %p, offset = %p)", (void*)(pPacket->object), pPacket->allocationIdx, (void*)(pPacket->mem), (void*)(pPacket->offset));
        return str;
    }
    case GLV_TPI_XGL_xglBindObjectMemoryRange:
    {
        struct_xglBindObjectMemoryRange* pPacket = (struct_xglBindObjectMemoryRange*)(pHeader->pBody);
        snprintf(str, 1024, "xglBindObjectMemoryRange(object = %p, allocationIdx = %i, rangeOffset = %p, rangeSize = %p, mem = %p, memOffset = %p)", (void*)(pPacket->object), pPacket->allocationIdx, (void*)(pPacket->rangeOffset), (void*)(pPacket->rangeSize), (void*)(pPacket->mem), (void*)(pPacket->memOffset));
        return str;
    }
    case GLV_TPI_XGL_xglBindImageMemoryRange:
    {
        struct_xglBindImageMemoryRange* pPacket = (struct_xglBindImageMemoryRange*)(pHeader->pBody);
        snprintf(str, 1024, "xglBindImageMemoryRange(image = %p, allocationIdx = %i, bindInfo = %p, mem = %p, memOffset = %p)", (void*)(pPacket->image), pPacket->allocationIdx, (void*)(pPacket->bindInfo), (void*)(pPacket->mem), (void*)(pPacket->memOffset));
        return str;
    }
    case GLV_TPI_XGL_xglCreateFence:
    {
        struct_xglCreateFence* pPacket = (struct_xglCreateFence*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateFence(device = %p, pCreateInfo = %p, pFence = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pFence);
        return str;
    }
    case GLV_TPI_XGL_xglGetFenceStatus:
    {
        struct_xglGetFenceStatus* pPacket = (struct_xglGetFenceStatus*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetFenceStatus(fence = %p)", (void*)(pPacket->fence));
        return str;
    }
    case GLV_TPI_XGL_xglWaitForFences:
    {
        struct_xglWaitForFences* pPacket = (struct_xglWaitForFences*)(pHeader->pBody);
        snprintf(str, 1024, "xglWaitForFences(device = %p, fenceCount = %i, pFences = %p, waitAll = %u, timeout = %lu)", (void*)(pPacket->device), pPacket->fenceCount, (void*)(pPacket->pFences), pPacket->waitAll, pPacket->timeout);
        return str;
    }
    case GLV_TPI_XGL_xglCreateQueueSemaphore:
    {
        struct_xglCreateQueueSemaphore* pPacket = (struct_xglCreateQueueSemaphore*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateQueueSemaphore(device = %p, pCreateInfo = %p, pSemaphore = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pSemaphore);
        return str;
    }
    case GLV_TPI_XGL_xglSignalQueueSemaphore:
    {
        struct_xglSignalQueueSemaphore* pPacket = (struct_xglSignalQueueSemaphore*)(pHeader->pBody);
        snprintf(str, 1024, "xglSignalQueueSemaphore(queue = %p, semaphore = %p)", (void*)(pPacket->queue), (void*)(pPacket->semaphore));
        return str;
    }
    case GLV_TPI_XGL_xglWaitQueueSemaphore:
    {
        struct_xglWaitQueueSemaphore* pPacket = (struct_xglWaitQueueSemaphore*)(pHeader->pBody);
        snprintf(str, 1024, "xglWaitQueueSemaphore(queue = %p, semaphore = %p)", (void*)(pPacket->queue), (void*)(pPacket->semaphore));
        return str;
    }
    case GLV_TPI_XGL_xglCreateEvent:
    {
        struct_xglCreateEvent* pPacket = (struct_xglCreateEvent*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateEvent(device = %p, pCreateInfo = %p, pEvent = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pEvent);
        return str;
    }
    case GLV_TPI_XGL_xglGetEventStatus:
    {
        struct_xglGetEventStatus* pPacket = (struct_xglGetEventStatus*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetEventStatus(event = %p)", (void*)(pPacket->event));
        return str;
    }
    case GLV_TPI_XGL_xglSetEvent:
    {
        struct_xglSetEvent* pPacket = (struct_xglSetEvent*)(pHeader->pBody);
        snprintf(str, 1024, "xglSetEvent(event = %p)", (void*)(pPacket->event));
        return str;
    }
    case GLV_TPI_XGL_xglResetEvent:
    {
        struct_xglResetEvent* pPacket = (struct_xglResetEvent*)(pHeader->pBody);
        snprintf(str, 1024, "xglResetEvent(event = %p)", (void*)(pPacket->event));
        return str;
    }
    case GLV_TPI_XGL_xglCreateQueryPool:
    {
        struct_xglCreateQueryPool* pPacket = (struct_xglCreateQueryPool*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateQueryPool(device = %p, pCreateInfo = %p, pQueryPool = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pQueryPool);
        return str;
    }
    case GLV_TPI_XGL_xglGetQueryPoolResults:
    {
        struct_xglGetQueryPoolResults* pPacket = (struct_xglGetQueryPoolResults*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetQueryPoolResults(queryPool = %p, startQuery = %i, queryCount = %i, *pDataSize = %zu, pData = %p)", (void*)(pPacket->queryPool), pPacket->startQuery, pPacket->queryCount, (pPacket->pDataSize == NULL) ? 0 : *(pPacket->pDataSize), (void*)(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglGetFormatInfo:
    {
        struct_xglGetFormatInfo* pPacket = (struct_xglGetFormatInfo*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetFormatInfo(device = %p, format = %p, infoType = %s, *pDataSize = %zu, pData = %p)", (void*)(pPacket->device), (void*)(pPacket->format), string_XGL_FORMAT_INFO_TYPE(pPacket->infoType), (pPacket->pDataSize == NULL) ? 0 : *(pPacket->pDataSize), (void*)(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglCreateBuffer:
    {
        struct_xglCreateBuffer* pPacket = (struct_xglCreateBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateBuffer(device = %p, pCreateInfo = %p, pBuffer = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pBuffer);
        return str;
    }
    case GLV_TPI_XGL_xglCreateBufferView:
    {
        struct_xglCreateBufferView* pPacket = (struct_xglCreateBufferView*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateBufferView(device = %p, pCreateInfo = %p, pView = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pView);
        return str;
    }
    case GLV_TPI_XGL_xglCreateImage:
    {
        struct_xglCreateImage* pPacket = (struct_xglCreateImage*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateImage(device = %p, pCreateInfo = %p, pImage = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pImage);
        return str;
    }
    case GLV_TPI_XGL_xglSetFastClearColor:
    {
        struct_xglSetFastClearColor* pPacket = (struct_xglSetFastClearColor*)(pHeader->pBody);
        snprintf(str, 1024, "xglSetFastClearColor(image = %p, color = [%f, %f, %f, %f])", (void*)(pPacket->image), pPacket->color[0], pPacket->color[1], pPacket->color[2], pPacket->color[3]);
        return str;
    }
    case GLV_TPI_XGL_xglSetFastClearDepth:
    {
        struct_xglSetFastClearDepth* pPacket = (struct_xglSetFastClearDepth*)(pHeader->pBody);
        snprintf(str, 1024, "xglSetFastClearDepth(image = %p, depth = %f)", (void*)(pPacket->image), pPacket->depth);
        return str;
    }
    case GLV_TPI_XGL_xglGetImageSubresourceInfo:
    {
        struct_xglGetImageSubresourceInfo* pPacket = (struct_xglGetImageSubresourceInfo*)(pHeader->pBody);
        snprintf(str, 1024, "xglGetImageSubresourceInfo(image = %p, pSubresource = %p, infoType = %s, *pDataSize = %zu, pData = %p)", (void*)(pPacket->image), (void*)(pPacket->pSubresource), string_XGL_SUBRESOURCE_INFO_TYPE(pPacket->infoType), (pPacket->pDataSize == NULL) ? 0 : *(pPacket->pDataSize), (void*)(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglCreateImageView:
    {
        struct_xglCreateImageView* pPacket = (struct_xglCreateImageView*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateImageView(device = %p, pCreateInfo = %p, pView = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pView);
        return str;
    }
    case GLV_TPI_XGL_xglCreateColorAttachmentView:
    {
        struct_xglCreateColorAttachmentView* pPacket = (struct_xglCreateColorAttachmentView*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateColorAttachmentView(device = %p, pCreateInfo = %p, pView = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pView);
        return str;
    }
    case GLV_TPI_XGL_xglCreateDepthStencilView:
    {
        struct_xglCreateDepthStencilView* pPacket = (struct_xglCreateDepthStencilView*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateDepthStencilView(device = %p, pCreateInfo = %p, pView = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pView);
        return str;
    }
    case GLV_TPI_XGL_xglCreateShader:
    {
        struct_xglCreateShader* pPacket = (struct_xglCreateShader*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateShader(device = %p, pCreateInfo = %p, pShader = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pShader);
        return str;
    }
    case GLV_TPI_XGL_xglCreateGraphicsPipeline:
    {
        struct_xglCreateGraphicsPipeline* pPacket = (struct_xglCreateGraphicsPipeline*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateGraphicsPipeline(device = %p, pCreateInfo = %p, pPipeline = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pPipeline);
        return str;
    }
    case GLV_TPI_XGL_xglCreateComputePipeline:
    {
        struct_xglCreateComputePipeline* pPacket = (struct_xglCreateComputePipeline*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateComputePipeline(device = %p, pCreateInfo = %p, pPipeline = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pPipeline);
        return str;
    }
    case GLV_TPI_XGL_xglStorePipeline:
    {
        struct_xglStorePipeline* pPacket = (struct_xglStorePipeline*)(pHeader->pBody);
        snprintf(str, 1024, "xglStorePipeline(pipeline = %p, *pDataSize = %zu, pData = %p)", (void*)(pPacket->pipeline), (pPacket->pDataSize == NULL) ? 0 : *(pPacket->pDataSize), (void*)(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglLoadPipeline:
    {
        struct_xglLoadPipeline* pPacket = (struct_xglLoadPipeline*)(pHeader->pBody);
        snprintf(str, 1024, "xglLoadPipeline(device = %p, dataSize = %zu, pData = %p, pPipeline = %p)", (void*)(pPacket->device), pPacket->dataSize, (void*)(pPacket->pData), (void*)(pPacket->pPipeline));
        return str;
    }
    case GLV_TPI_XGL_xglCreatePipelineDelta:
    {
        struct_xglCreatePipelineDelta* pPacket = (struct_xglCreatePipelineDelta*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreatePipelineDelta(device = %p, p1 = %p, p2 = %p, delta = %p)", (void*)(pPacket->device), (void*)(pPacket->p1), (void*)(pPacket->p2), (void*)pPacket->delta);
        return str;
    }
    case GLV_TPI_XGL_xglCreateSampler:
    {
        struct_xglCreateSampler* pPacket = (struct_xglCreateSampler*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateSampler(device = %p, pCreateInfo = %p, pSampler = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pSampler);
        return str;
    }
    case GLV_TPI_XGL_xglCreateDescriptorSetLayout:
    {
        struct_xglCreateDescriptorSetLayout* pPacket = (struct_xglCreateDescriptorSetLayout*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateDescriptorSetLayout(device = %p, stageFlags = %i, *pSetBindPoints = %i, priorSetLayout = %p, pSetLayoutInfoList = %p, pSetLayout = %p)", (void*)(pPacket->device), pPacket->stageFlags, (pPacket->pSetBindPoints == NULL) ? 0 : *(pPacket->pSetBindPoints), (void*)(pPacket->priorSetLayout), (void*)(pPacket->pSetLayoutInfoList), (void*)pPacket->pSetLayout);
        return str;
    }
    case GLV_TPI_XGL_xglBeginDescriptorRegionUpdate:
    {
        struct_xglBeginDescriptorRegionUpdate* pPacket = (struct_xglBeginDescriptorRegionUpdate*)(pHeader->pBody);
        snprintf(str, 1024, "xglBeginDescriptorRegionUpdate(device = %p, updateMode = %p)", (void*)(pPacket->device), (void*)(pPacket->updateMode));
        return str;
    }
    case GLV_TPI_XGL_xglEndDescriptorRegionUpdate:
    {
        struct_xglEndDescriptorRegionUpdate* pPacket = (struct_xglEndDescriptorRegionUpdate*)(pHeader->pBody);
        snprintf(str, 1024, "xglEndDescriptorRegionUpdate(device = %p, cmd = %p)", (void*)(pPacket->device), (void*)(pPacket->cmd));
        return str;
    }
    case GLV_TPI_XGL_xglCreateDescriptorRegion:
    {
        struct_xglCreateDescriptorRegion* pPacket = (struct_xglCreateDescriptorRegion*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateDescriptorRegion(device = %p, regionUsage = %p, maxSets = %i, pCreateInfo = %p, pDescriptorRegion = %p)", (void*)(pPacket->device), (void*)(pPacket->regionUsage), pPacket->maxSets, (void*)(pPacket->pCreateInfo), (void*)pPacket->pDescriptorRegion);
        return str;
    }
    case GLV_TPI_XGL_xglClearDescriptorRegion:
    {
        struct_xglClearDescriptorRegion* pPacket = (struct_xglClearDescriptorRegion*)(pHeader->pBody);
        snprintf(str, 1024, "xglClearDescriptorRegion(descriptorRegion = %p)", (void*)(pPacket->descriptorRegion));
        return str;
    }
    case GLV_TPI_XGL_xglAllocDescriptorSets:
    {
        struct_xglAllocDescriptorSets* pPacket = (struct_xglAllocDescriptorSets*)(pHeader->pBody);
        snprintf(str, 1024, "xglAllocDescriptorSets(descriptorRegion = %p, setUsage = %p, count = %i, pSetLayouts = %p, pDescriptorSets = %p, *pCount = %i)", (void*)(pPacket->descriptorRegion), (void*)(pPacket->setUsage), pPacket->count, (void*)(pPacket->pSetLayouts), (void*)(pPacket->pDescriptorSets), (pPacket->pCount == NULL) ? 0 : *(pPacket->pCount));
        return str;
    }
    case GLV_TPI_XGL_xglClearDescriptorSets:
    {
        struct_xglClearDescriptorSets* pPacket = (struct_xglClearDescriptorSets*)(pHeader->pBody);
        snprintf(str, 1024, "xglClearDescriptorSets(descriptorRegion = %p, count = %i, pDescriptorSets = %p)", (void*)(pPacket->descriptorRegion), pPacket->count, (void*)(pPacket->pDescriptorSets));
        return str;
    }
    case GLV_TPI_XGL_xglUpdateDescriptors:
    {
        struct_xglUpdateDescriptors* pPacket = (struct_xglUpdateDescriptors*)(pHeader->pBody);
        snprintf(str, 1024, "xglUpdateDescriptors(descriptorSet = %p, pUpdateChain = %p)", (void*)(pPacket->descriptorSet), (void*)(pPacket->pUpdateChain));
        return str;
    }
    case GLV_TPI_XGL_xglCreateDynamicViewportState:
    {
        struct_xglCreateDynamicViewportState* pPacket = (struct_xglCreateDynamicViewportState*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateDynamicViewportState(device = %p, pCreateInfo = %p, pState = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pState);
        return str;
    }
    case GLV_TPI_XGL_xglCreateDynamicRasterState:
    {
        struct_xglCreateDynamicRasterState* pPacket = (struct_xglCreateDynamicRasterState*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateDynamicRasterState(device = %p, pCreateInfo = %p, pState = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pState);
        return str;
    }
    case GLV_TPI_XGL_xglCreateDynamicColorBlendState:
    {
        struct_xglCreateDynamicColorBlendState* pPacket = (struct_xglCreateDynamicColorBlendState*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateDynamicColorBlendState(device = %p, pCreateInfo = %p, pState = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pState);
        return str;
    }
    case GLV_TPI_XGL_xglCreateDynamicDepthStencilState:
    {
        struct_xglCreateDynamicDepthStencilState* pPacket = (struct_xglCreateDynamicDepthStencilState*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateDynamicDepthStencilState(device = %p, pCreateInfo = %p, pState = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pState);
        return str;
    }
    case GLV_TPI_XGL_xglCreateCommandBuffer:
    {
        struct_xglCreateCommandBuffer* pPacket = (struct_xglCreateCommandBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateCommandBuffer(device = %p, pCreateInfo = %p, pCmdBuffer = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pCmdBuffer);
        return str;
    }
    case GLV_TPI_XGL_xglBeginCommandBuffer:
    {
        struct_xglBeginCommandBuffer* pPacket = (struct_xglBeginCommandBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglBeginCommandBuffer(cmdBuffer = %p, pBeginInfo = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pBeginInfo));
        return str;
    }
    case GLV_TPI_XGL_xglEndCommandBuffer:
    {
        struct_xglEndCommandBuffer* pPacket = (struct_xglEndCommandBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglEndCommandBuffer(cmdBuffer = %p)", (void*)(pPacket->cmdBuffer));
        return str;
    }
    case GLV_TPI_XGL_xglResetCommandBuffer:
    {
        struct_xglResetCommandBuffer* pPacket = (struct_xglResetCommandBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglResetCommandBuffer(cmdBuffer = %p)", (void*)(pPacket->cmdBuffer));
        return str;
    }
    case GLV_TPI_XGL_xglCmdBindPipeline:
    {
        struct_xglCmdBindPipeline* pPacket = (struct_xglCmdBindPipeline*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdBindPipeline(cmdBuffer = %p, pipelineBindPoint = %p, pipeline = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pipelineBindPoint), (void*)(pPacket->pipeline));
        return str;
    }
    case GLV_TPI_XGL_xglCmdBindPipelineDelta:
    {
        struct_xglCmdBindPipelineDelta* pPacket = (struct_xglCmdBindPipelineDelta*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdBindPipelineDelta(cmdBuffer = %p, pipelineBindPoint = %p, delta = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pipelineBindPoint), (void*)(pPacket->delta));
        return str;
    }
    case GLV_TPI_XGL_xglCmdBindDynamicStateObject:
    {
        struct_xglCmdBindDynamicStateObject* pPacket = (struct_xglCmdBindDynamicStateObject*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdBindDynamicStateObject(cmdBuffer = %p, stateBindPoint = %p, state = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->stateBindPoint), (void*)(pPacket->state));
        return str;
    }
    case GLV_TPI_XGL_xglCmdBindDescriptorSet:
    {
        struct_xglCmdBindDescriptorSet* pPacket = (struct_xglCmdBindDescriptorSet*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdBindDescriptorSet(cmdBuffer = %p, pipelineBindPoint = %p, descriptorSet = %p, *pUserData = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pipelineBindPoint), (void*)(pPacket->descriptorSet), (pPacket->pUserData == NULL) ? 0 : *(pPacket->pUserData));
        return str;
    }
    case GLV_TPI_XGL_xglCmdBindVertexBuffer:
    {
        struct_xglCmdBindVertexBuffer* pPacket = (struct_xglCmdBindVertexBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdBindVertexBuffer(cmdBuffer = %p, buffer = %p, offset = %p, binding = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->buffer), (void*)(pPacket->offset), pPacket->binding);
        return str;
    }
    case GLV_TPI_XGL_xglCmdBindIndexBuffer:
    {
        struct_xglCmdBindIndexBuffer* pPacket = (struct_xglCmdBindIndexBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdBindIndexBuffer(cmdBuffer = %p, buffer = %p, offset = %p, indexType = %s)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->buffer), (void*)(pPacket->offset), string_XGL_INDEX_TYPE(pPacket->indexType));
        return str;
    }
    case GLV_TPI_XGL_xglCmdDraw:
    {
        struct_xglCmdDraw* pPacket = (struct_xglCmdDraw*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdDraw(cmdBuffer = %p, firstVertex = %i, vertexCount = %i, firstInstance = %i, instanceCount = %i)", (void*)(pPacket->cmdBuffer), pPacket->firstVertex, pPacket->vertexCount, pPacket->firstInstance, pPacket->instanceCount);
        return str;
    }
    case GLV_TPI_XGL_xglCmdDrawIndexed:
    {
        struct_xglCmdDrawIndexed* pPacket = (struct_xglCmdDrawIndexed*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdDrawIndexed(cmdBuffer = %p, firstIndex = %i, indexCount = %i, vertexOffset = %i, firstInstance = %i, instanceCount = %i)", (void*)(pPacket->cmdBuffer), pPacket->firstIndex, pPacket->indexCount, pPacket->vertexOffset, pPacket->firstInstance, pPacket->instanceCount);
        return str;
    }
    case GLV_TPI_XGL_xglCmdDrawIndirect:
    {
        struct_xglCmdDrawIndirect* pPacket = (struct_xglCmdDrawIndirect*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdDrawIndirect(cmdBuffer = %p, buffer = %p, offset = %p, count = %i, stride = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->buffer), (void*)(pPacket->offset), pPacket->count, pPacket->stride);
        return str;
    }
    case GLV_TPI_XGL_xglCmdDrawIndexedIndirect:
    {
        struct_xglCmdDrawIndexedIndirect* pPacket = (struct_xglCmdDrawIndexedIndirect*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdDrawIndexedIndirect(cmdBuffer = %p, buffer = %p, offset = %p, count = %i, stride = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->buffer), (void*)(pPacket->offset), pPacket->count, pPacket->stride);
        return str;
    }
    case GLV_TPI_XGL_xglCmdDispatch:
    {
        struct_xglCmdDispatch* pPacket = (struct_xglCmdDispatch*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdDispatch(cmdBuffer = %p, x = %i, y = %i, z = %i)", (void*)(pPacket->cmdBuffer), pPacket->x, pPacket->y, pPacket->z);
        return str;
    }
    case GLV_TPI_XGL_xglCmdDispatchIndirect:
    {
        struct_xglCmdDispatchIndirect* pPacket = (struct_xglCmdDispatchIndirect*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdDispatchIndirect(cmdBuffer = %p, buffer = %p, offset = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->buffer), (void*)(pPacket->offset));
        return str;
    }
    case GLV_TPI_XGL_xglCmdCopyBuffer:
    {
        struct_xglCmdCopyBuffer* pPacket = (struct_xglCmdCopyBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdCopyBuffer(cmdBuffer = %p, srcBuffer = %p, destBuffer = %p, regionCount = %i, pRegions = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->srcBuffer), (void*)(pPacket->destBuffer), pPacket->regionCount, (void*)(pPacket->pRegions));
        return str;
    }
    case GLV_TPI_XGL_xglCmdCopyImage:
    {
        struct_xglCmdCopyImage* pPacket = (struct_xglCmdCopyImage*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdCopyImage(cmdBuffer = %p, srcImage = %p, destImage = %p, regionCount = %i, pRegions = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->srcImage), (void*)(pPacket->destImage), pPacket->regionCount, (void*)(pPacket->pRegions));
        return str;
    }
    case GLV_TPI_XGL_xglCmdCopyBufferToImage:
    {
        struct_xglCmdCopyBufferToImage* pPacket = (struct_xglCmdCopyBufferToImage*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdCopyBufferToImage(cmdBuffer = %p, srcBuffer = %p, destImage = %p, regionCount = %i, pRegions = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->srcBuffer), (void*)(pPacket->destImage), pPacket->regionCount, (void*)(pPacket->pRegions));
        return str;
    }
    case GLV_TPI_XGL_xglCmdCopyImageToBuffer:
    {
        struct_xglCmdCopyImageToBuffer* pPacket = (struct_xglCmdCopyImageToBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdCopyImageToBuffer(cmdBuffer = %p, srcImage = %p, destBuffer = %p, regionCount = %i, pRegions = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->srcImage), (void*)(pPacket->destBuffer), pPacket->regionCount, (void*)(pPacket->pRegions));
        return str;
    }
    case GLV_TPI_XGL_xglCmdCloneImageData:
    {
        struct_xglCmdCloneImageData* pPacket = (struct_xglCmdCloneImageData*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdCloneImageData(cmdBuffer = %p, srcImage = %p, srcImageLayout = %p, destImage = %p, destImageLayout = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->srcImage), (void*)(pPacket->srcImageLayout), (void*)(pPacket->destImage), (void*)(pPacket->destImageLayout));
        return str;
    }
    case GLV_TPI_XGL_xglCmdUpdateBuffer:
    {
        struct_xglCmdUpdateBuffer* pPacket = (struct_xglCmdUpdateBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdUpdateBuffer(cmdBuffer = %p, destBuffer = %p, destOffset = %p, dataSize = %p, *pData = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->destBuffer), (void*)(pPacket->destOffset), (void*)(pPacket->dataSize), (pPacket->pData == NULL) ? 0 : *(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglCmdFillBuffer:
    {
        struct_xglCmdFillBuffer* pPacket = (struct_xglCmdFillBuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdFillBuffer(cmdBuffer = %p, destBuffer = %p, destOffset = %p, fillSize = %p, data = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->destBuffer), (void*)(pPacket->destOffset), (void*)(pPacket->fillSize), pPacket->data);
        return str;
    }
    case GLV_TPI_XGL_xglCmdClearColorImage:
    {
        struct_xglCmdClearColorImage* pPacket = (struct_xglCmdClearColorImage*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdClearColorImage(cmdBuffer = %p, image = %p, color = [%f, %f, %f, %f], rangeCount = %i, pRanges = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->image), pPacket->color[0], pPacket->color[1], pPacket->color[2], pPacket->color[3], pPacket->rangeCount, (void*)(pPacket->pRanges));
        return str;
    }
    case GLV_TPI_XGL_xglCmdClearColorImageRaw:
    {
        struct_xglCmdClearColorImageRaw* pPacket = (struct_xglCmdClearColorImageRaw*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdClearColorImageRaw(cmdBuffer = %p, image = %p, color = [%i, %i, %i, %i], rangeCount = %i, pRanges = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->image), pPacket->color[0], pPacket->color[1], pPacket->color[2], pPacket->color[3], pPacket->rangeCount, (void*)(pPacket->pRanges));
        return str;
    }
    case GLV_TPI_XGL_xglCmdClearDepthStencil:
    {
        struct_xglCmdClearDepthStencil* pPacket = (struct_xglCmdClearDepthStencil*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdClearDepthStencil(cmdBuffer = %p, image = %p, depth = %f, stencil = %i, rangeCount = %i, pRanges = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->image), pPacket->depth, pPacket->stencil, pPacket->rangeCount, (void*)(pPacket->pRanges));
        return str;
    }
    case GLV_TPI_XGL_xglCmdResolveImage:
    {
        struct_xglCmdResolveImage* pPacket = (struct_xglCmdResolveImage*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdResolveImage(cmdBuffer = %p, srcImage = %p, destImage = %p, rectCount = %i, pRects = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->srcImage), (void*)(pPacket->destImage), pPacket->rectCount, (void*)(pPacket->pRects));
        return str;
    }
    case GLV_TPI_XGL_xglCmdSetEvent:
    {
        struct_xglCmdSetEvent* pPacket = (struct_xglCmdSetEvent*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdSetEvent(cmdBuffer = %p, event = %p, pipeEvent = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->event), (void*)(pPacket->pipeEvent));
        return str;
    }
    case GLV_TPI_XGL_xglCmdResetEvent:
    {
        struct_xglCmdResetEvent* pPacket = (struct_xglCmdResetEvent*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdResetEvent(cmdBuffer = %p, event = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->event));
        return str;
    }
    case GLV_TPI_XGL_xglCmdWaitEvents:
    {
        struct_xglCmdWaitEvents* pPacket = (struct_xglCmdWaitEvents*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdWaitEvents(cmdBuffer = %p, pWaitInfo = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pWaitInfo));
        return str;
    }
    case GLV_TPI_XGL_xglCmdPipelineBarrier:
    {
        struct_xglCmdPipelineBarrier* pPacket = (struct_xglCmdPipelineBarrier*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdPipelineBarrier(cmdBuffer = %p, pBarrier = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pBarrier));
        return str;
    }
    case GLV_TPI_XGL_xglCmdBeginQuery:
    {
        struct_xglCmdBeginQuery* pPacket = (struct_xglCmdBeginQuery*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdBeginQuery(cmdBuffer = %p, queryPool = %p, slot = %i, flags = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->queryPool), pPacket->slot, pPacket->flags);
        return str;
    }
    case GLV_TPI_XGL_xglCmdEndQuery:
    {
        struct_xglCmdEndQuery* pPacket = (struct_xglCmdEndQuery*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdEndQuery(cmdBuffer = %p, queryPool = %p, slot = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->queryPool), pPacket->slot);
        return str;
    }
    case GLV_TPI_XGL_xglCmdResetQueryPool:
    {
        struct_xglCmdResetQueryPool* pPacket = (struct_xglCmdResetQueryPool*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdResetQueryPool(cmdBuffer = %p, queryPool = %p, startQuery = %i, queryCount = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->queryPool), pPacket->startQuery, pPacket->queryCount);
        return str;
    }
    case GLV_TPI_XGL_xglCmdWriteTimestamp:
    {
        struct_xglCmdWriteTimestamp* pPacket = (struct_xglCmdWriteTimestamp*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdWriteTimestamp(cmdBuffer = %p, timestampType = %s, destBuffer = %p, destOffset = %p)", (void*)(pPacket->cmdBuffer), string_XGL_TIMESTAMP_TYPE(pPacket->timestampType), (void*)(pPacket->destBuffer), (void*)(pPacket->destOffset));
        return str;
    }
    case GLV_TPI_XGL_xglCmdInitAtomicCounters:
    {
        struct_xglCmdInitAtomicCounters* pPacket = (struct_xglCmdInitAtomicCounters*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdInitAtomicCounters(cmdBuffer = %p, pipelineBindPoint = %p, startCounter = %i, counterCount = %i, *pData = %i)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pipelineBindPoint), pPacket->startCounter, pPacket->counterCount, (pPacket->pData == NULL) ? 0 : *(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglCmdLoadAtomicCounters:
    {
        struct_xglCmdLoadAtomicCounters* pPacket = (struct_xglCmdLoadAtomicCounters*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdLoadAtomicCounters(cmdBuffer = %p, pipelineBindPoint = %p, startCounter = %i, counterCount = %i, srcBuffer = %p, srcOffset = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pipelineBindPoint), pPacket->startCounter, pPacket->counterCount, (void*)(pPacket->srcBuffer), (void*)(pPacket->srcOffset));
        return str;
    }
    case GLV_TPI_XGL_xglCmdSaveAtomicCounters:
    {
        struct_xglCmdSaveAtomicCounters* pPacket = (struct_xglCmdSaveAtomicCounters*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdSaveAtomicCounters(cmdBuffer = %p, pipelineBindPoint = %p, startCounter = %i, counterCount = %i, destBuffer = %p, destOffset = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pipelineBindPoint), pPacket->startCounter, pPacket->counterCount, (void*)(pPacket->destBuffer), (void*)(pPacket->destOffset));
        return str;
    }
    case GLV_TPI_XGL_xglCreateFramebuffer:
    {
        struct_xglCreateFramebuffer* pPacket = (struct_xglCreateFramebuffer*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateFramebuffer(device = %p, pCreateInfo = %p, pFramebuffer = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pFramebuffer);
        return str;
    }
    case GLV_TPI_XGL_xglCreateRenderPass:
    {
        struct_xglCreateRenderPass* pPacket = (struct_xglCreateRenderPass*)(pHeader->pBody);
        snprintf(str, 1024, "xglCreateRenderPass(device = %p, pCreateInfo = %p, pRenderPass = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)pPacket->pRenderPass);
        return str;
    }
    case GLV_TPI_XGL_xglCmdBeginRenderPass:
    {
        struct_xglCmdBeginRenderPass* pPacket = (struct_xglCmdBeginRenderPass*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdBeginRenderPass(cmdBuffer = %p, renderPass = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->renderPass));
        return str;
    }
    case GLV_TPI_XGL_xglCmdEndRenderPass:
    {
        struct_xglCmdEndRenderPass* pPacket = (struct_xglCmdEndRenderPass*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdEndRenderPass(cmdBuffer = %p, renderPass = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->renderPass));
        return str;
    }
    case GLV_TPI_XGL_xglDbgSetValidationLevel:
    {
        struct_xglDbgSetValidationLevel* pPacket = (struct_xglDbgSetValidationLevel*)(pHeader->pBody);
        snprintf(str, 1024, "xglDbgSetValidationLevel(device = %p, validationLevel = %p)", (void*)(pPacket->device), (void*)(pPacket->validationLevel));
        return str;
    }
    case GLV_TPI_XGL_xglDbgRegisterMsgCallback:
    {
        struct_xglDbgRegisterMsgCallback* pPacket = (struct_xglDbgRegisterMsgCallback*)(pHeader->pBody);
        snprintf(str, 1024, "xglDbgRegisterMsgCallback(pfnMsgCallback = %p, pUserData = %p)", (void*)(pPacket->pfnMsgCallback), (void*)(pPacket->pUserData));
        return str;
    }
    case GLV_TPI_XGL_xglDbgUnregisterMsgCallback:
    {
        struct_xglDbgUnregisterMsgCallback* pPacket = (struct_xglDbgUnregisterMsgCallback*)(pHeader->pBody);
        snprintf(str, 1024, "xglDbgUnregisterMsgCallback(pfnMsgCallback = %p)", (void*)(pPacket->pfnMsgCallback));
        return str;
    }
    case GLV_TPI_XGL_xglDbgSetMessageFilter:
    {
        struct_xglDbgSetMessageFilter* pPacket = (struct_xglDbgSetMessageFilter*)(pHeader->pBody);
        snprintf(str, 1024, "xglDbgSetMessageFilter(device = %p, msgCode = %i, filter = %p)", (void*)(pPacket->device), pPacket->msgCode, (void*)(pPacket->filter));
        return str;
    }
    case GLV_TPI_XGL_xglDbgSetObjectTag:
    {
        struct_xglDbgSetObjectTag* pPacket = (struct_xglDbgSetObjectTag*)(pHeader->pBody);
        snprintf(str, 1024, "xglDbgSetObjectTag(object = %p, tagSize = %zu, pTag = %p)", (void*)(pPacket->object), pPacket->tagSize, (void*)(pPacket->pTag));
        return str;
    }
    case GLV_TPI_XGL_xglDbgSetGlobalOption:
    {
        struct_xglDbgSetGlobalOption* pPacket = (struct_xglDbgSetGlobalOption*)(pHeader->pBody);
        snprintf(str, 1024, "xglDbgSetGlobalOption(dbgOption = %p, dataSize = %zu, pData = %p)", (void*)(pPacket->dbgOption), pPacket->dataSize, (void*)(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglDbgSetDeviceOption:
    {
        struct_xglDbgSetDeviceOption* pPacket = (struct_xglDbgSetDeviceOption*)(pHeader->pBody);
        snprintf(str, 1024, "xglDbgSetDeviceOption(device = %p, dbgOption = %p, dataSize = %zu, pData = %p)", (void*)(pPacket->device), (void*)(pPacket->dbgOption), pPacket->dataSize, (void*)(pPacket->pData));
        return str;
    }
    case GLV_TPI_XGL_xglCmdDbgMarkerBegin:
    {
        struct_xglCmdDbgMarkerBegin* pPacket = (struct_xglCmdDbgMarkerBegin*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdDbgMarkerBegin(cmdBuffer = %p, pMarker = %p)", (void*)(pPacket->cmdBuffer), (void*)(pPacket->pMarker));
        return str;
    }
    case GLV_TPI_XGL_xglCmdDbgMarkerEnd:
    {
        struct_xglCmdDbgMarkerEnd* pPacket = (struct_xglCmdDbgMarkerEnd*)(pHeader->pBody);
        snprintf(str, 1024, "xglCmdDbgMarkerEnd(cmdBuffer = %p)", (void*)(pPacket->cmdBuffer));
        return str;
    }
    case GLV_TPI_XGL_xglWsiX11AssociateConnection:
    {
        struct_xglWsiX11AssociateConnection* pPacket = (struct_xglWsiX11AssociateConnection*)(pHeader->pBody);
        snprintf(str, 1024, "xglWsiX11AssociateConnection(gpu = %p, pConnectionInfo = %p)", (void*)(pPacket->gpu), (void*)(pPacket->pConnectionInfo));
        return str;
    }
    case GLV_TPI_XGL_xglWsiX11GetMSC:
    {
        struct_xglWsiX11GetMSC* pPacket = (struct_xglWsiX11GetMSC*)(pHeader->pBody);
        snprintf(str, 1024, "xglWsiX11GetMSC(device = %p, window = %i, crtc = %u, *pMsc = %lu)", (void*)(pPacket->device), pPacket->window, pPacket->crtc, (pPacket->pMsc == NULL) ? 0 : *(pPacket->pMsc));
        return str;
    }
    case GLV_TPI_XGL_xglWsiX11CreatePresentableImage:
    {
        struct_xglWsiX11CreatePresentableImage* pPacket = (struct_xglWsiX11CreatePresentableImage*)(pHeader->pBody);
        snprintf(str, 1024, "xglWsiX11CreatePresentableImage(device = %p, pCreateInfo = %p, pImage = %p, pMem = %p)", (void*)(pPacket->device), (void*)(pPacket->pCreateInfo), (void*)(pPacket->pImage), (void*)pPacket->pMem);
        return str;
    }
    case GLV_TPI_XGL_xglWsiX11QueuePresent:
    {
        struct_xglWsiX11QueuePresent* pPacket = (struct_xglWsiX11QueuePresent*)(pHeader->pBody);
        snprintf(str, 1024, "xglWsiX11QueuePresent(queue = %p, pPresentInfo = %p, fence = %p)", (void*)(pPacket->queue), (void*)(pPacket->pPresentInfo), (void*)(pPacket->fence));
        return str;
    }
    default:
        return NULL;
    }
};

static glv_trace_packet_header* interpret_trace_packet_xgl(glv_trace_packet_header* pHeader)
{
    if (pHeader == NULL)
    {
        return NULL;
    }
    switch (pHeader->packet_id)
    {
        case GLV_TPI_XGL_xglApiVersion:
        {
            return interpret_body_as_xglApiVersion(pHeader, TRUE)->header;
        }
        case GLV_TPI_XGL_xglCreateInstance:
        {
            return interpret_body_as_xglCreateInstance(pHeader)->header;
        }
        case GLV_TPI_XGL_xglDestroyInstance:
        {
            return interpret_body_as_xglDestroyInstance(pHeader)->header;
        }
        case GLV_TPI_XGL_xglEnumerateGpus:
        {
            return interpret_body_as_xglEnumerateGpus(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetGpuInfo:
        {
            return interpret_body_as_xglGetGpuInfo(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetProcAddr:
        {
            return interpret_body_as_xglGetProcAddr(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateDevice:
        {
            return interpret_body_as_xglCreateDevice(pHeader)->header;
        }
        case GLV_TPI_XGL_xglDestroyDevice:
        {
            return interpret_body_as_xglDestroyDevice(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetExtensionSupport:
        {
            return interpret_body_as_xglGetExtensionSupport(pHeader)->header;
        }
        case GLV_TPI_XGL_xglEnumerateLayers:
        {
            return interpret_body_as_xglEnumerateLayers(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetDeviceQueue:
        {
            return interpret_body_as_xglGetDeviceQueue(pHeader)->header;
        }
        case GLV_TPI_XGL_xglQueueSubmit:
        {
            return interpret_body_as_xglQueueSubmit(pHeader)->header;
        }
        case GLV_TPI_XGL_xglQueueSetGlobalMemReferences:
        {
            return interpret_body_as_xglQueueSetGlobalMemReferences(pHeader)->header;
        }
        case GLV_TPI_XGL_xglQueueWaitIdle:
        {
            return interpret_body_as_xglQueueWaitIdle(pHeader)->header;
        }
        case GLV_TPI_XGL_xglDeviceWaitIdle:
        {
            return interpret_body_as_xglDeviceWaitIdle(pHeader)->header;
        }
        case GLV_TPI_XGL_xglAllocMemory:
        {
            return interpret_body_as_xglAllocMemory(pHeader)->header;
        }
        case GLV_TPI_XGL_xglFreeMemory:
        {
            return interpret_body_as_xglFreeMemory(pHeader)->header;
        }
        case GLV_TPI_XGL_xglSetMemoryPriority:
        {
            return interpret_body_as_xglSetMemoryPriority(pHeader)->header;
        }
        case GLV_TPI_XGL_xglMapMemory:
        {
            return interpret_body_as_xglMapMemory(pHeader)->header;
        }
        case GLV_TPI_XGL_xglUnmapMemory:
        {
            return interpret_body_as_xglUnmapMemory(pHeader)->header;
        }
        case GLV_TPI_XGL_xglPinSystemMemory:
        {
            return interpret_body_as_xglPinSystemMemory(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetMultiGpuCompatibility:
        {
            return interpret_body_as_xglGetMultiGpuCompatibility(pHeader)->header;
        }
        case GLV_TPI_XGL_xglOpenSharedMemory:
        {
            return interpret_body_as_xglOpenSharedMemory(pHeader)->header;
        }
        case GLV_TPI_XGL_xglOpenSharedQueueSemaphore:
        {
            return interpret_body_as_xglOpenSharedQueueSemaphore(pHeader)->header;
        }
        case GLV_TPI_XGL_xglOpenPeerMemory:
        {
            return interpret_body_as_xglOpenPeerMemory(pHeader)->header;
        }
        case GLV_TPI_XGL_xglOpenPeerImage:
        {
            return interpret_body_as_xglOpenPeerImage(pHeader)->header;
        }
        case GLV_TPI_XGL_xglDestroyObject:
        {
            return interpret_body_as_xglDestroyObject(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetObjectInfo:
        {
            return interpret_body_as_xglGetObjectInfo(pHeader)->header;
        }
        case GLV_TPI_XGL_xglBindObjectMemory:
        {
            return interpret_body_as_xglBindObjectMemory(pHeader)->header;
        }
        case GLV_TPI_XGL_xglBindObjectMemoryRange:
        {
            return interpret_body_as_xglBindObjectMemoryRange(pHeader)->header;
        }
        case GLV_TPI_XGL_xglBindImageMemoryRange:
        {
            return interpret_body_as_xglBindImageMemoryRange(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateFence:
        {
            return interpret_body_as_xglCreateFence(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetFenceStatus:
        {
            return interpret_body_as_xglGetFenceStatus(pHeader)->header;
        }
        case GLV_TPI_XGL_xglWaitForFences:
        {
            return interpret_body_as_xglWaitForFences(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateQueueSemaphore:
        {
            return interpret_body_as_xglCreateQueueSemaphore(pHeader)->header;
        }
        case GLV_TPI_XGL_xglSignalQueueSemaphore:
        {
            return interpret_body_as_xglSignalQueueSemaphore(pHeader)->header;
        }
        case GLV_TPI_XGL_xglWaitQueueSemaphore:
        {
            return interpret_body_as_xglWaitQueueSemaphore(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateEvent:
        {
            return interpret_body_as_xglCreateEvent(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetEventStatus:
        {
            return interpret_body_as_xglGetEventStatus(pHeader)->header;
        }
        case GLV_TPI_XGL_xglSetEvent:
        {
            return interpret_body_as_xglSetEvent(pHeader)->header;
        }
        case GLV_TPI_XGL_xglResetEvent:
        {
            return interpret_body_as_xglResetEvent(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateQueryPool:
        {
            return interpret_body_as_xglCreateQueryPool(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetQueryPoolResults:
        {
            return interpret_body_as_xglGetQueryPoolResults(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetFormatInfo:
        {
            return interpret_body_as_xglGetFormatInfo(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateBuffer:
        {
            return interpret_body_as_xglCreateBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateBufferView:
        {
            return interpret_body_as_xglCreateBufferView(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateImage:
        {
            return interpret_body_as_xglCreateImage(pHeader)->header;
        }
        case GLV_TPI_XGL_xglSetFastClearColor:
        {
            return interpret_body_as_xglSetFastClearColor(pHeader)->header;
        }
        case GLV_TPI_XGL_xglSetFastClearDepth:
        {
            return interpret_body_as_xglSetFastClearDepth(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetImageSubresourceInfo:
        {
            return interpret_body_as_xglGetImageSubresourceInfo(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateImageView:
        {
            return interpret_body_as_xglCreateImageView(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateColorAttachmentView:
        {
            return interpret_body_as_xglCreateColorAttachmentView(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateDepthStencilView:
        {
            return interpret_body_as_xglCreateDepthStencilView(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateShader:
        {
            return interpret_body_as_xglCreateShader(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateGraphicsPipeline:
        {
            return interpret_body_as_xglCreateGraphicsPipeline(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateComputePipeline:
        {
            return interpret_body_as_xglCreateComputePipeline(pHeader)->header;
        }
        case GLV_TPI_XGL_xglStorePipeline:
        {
            return interpret_body_as_xglStorePipeline(pHeader)->header;
        }
        case GLV_TPI_XGL_xglLoadPipeline:
        {
            return interpret_body_as_xglLoadPipeline(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreatePipelineDelta:
        {
            return interpret_body_as_xglCreatePipelineDelta(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateSampler:
        {
            return interpret_body_as_xglCreateSampler(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateDescriptorSetLayout:
        {
            return interpret_body_as_xglCreateDescriptorSetLayout(pHeader)->header;
        }
        case GLV_TPI_XGL_xglBeginDescriptorRegionUpdate:
        {
            return interpret_body_as_xglBeginDescriptorRegionUpdate(pHeader)->header;
        }
        case GLV_TPI_XGL_xglEndDescriptorRegionUpdate:
        {
            return interpret_body_as_xglEndDescriptorRegionUpdate(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateDescriptorRegion:
        {
            return interpret_body_as_xglCreateDescriptorRegion(pHeader)->header;
        }
        case GLV_TPI_XGL_xglClearDescriptorRegion:
        {
            return interpret_body_as_xglClearDescriptorRegion(pHeader)->header;
        }
        case GLV_TPI_XGL_xglAllocDescriptorSets:
        {
            return interpret_body_as_xglAllocDescriptorSets(pHeader)->header;
        }
        case GLV_TPI_XGL_xglClearDescriptorSets:
        {
            return interpret_body_as_xglClearDescriptorSets(pHeader)->header;
        }
        case GLV_TPI_XGL_xglUpdateDescriptors:
        {
            return interpret_body_as_xglUpdateDescriptors(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateDynamicViewportState:
        {
            return interpret_body_as_xglCreateDynamicViewportState(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateDynamicRasterState:
        {
            return interpret_body_as_xglCreateDynamicRasterState(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateDynamicColorBlendState:
        {
            return interpret_body_as_xglCreateDynamicColorBlendState(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateDynamicDepthStencilState:
        {
            return interpret_body_as_xglCreateDynamicDepthStencilState(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateCommandBuffer:
        {
            return interpret_body_as_xglCreateCommandBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglBeginCommandBuffer:
        {
            return interpret_body_as_xglBeginCommandBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglEndCommandBuffer:
        {
            return interpret_body_as_xglEndCommandBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglResetCommandBuffer:
        {
            return interpret_body_as_xglResetCommandBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdBindPipeline:
        {
            return interpret_body_as_xglCmdBindPipeline(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdBindPipelineDelta:
        {
            return interpret_body_as_xglCmdBindPipelineDelta(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdBindDynamicStateObject:
        {
            return interpret_body_as_xglCmdBindDynamicStateObject(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdBindDescriptorSet:
        {
            return interpret_body_as_xglCmdBindDescriptorSet(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdBindVertexBuffer:
        {
            return interpret_body_as_xglCmdBindVertexBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdBindIndexBuffer:
        {
            return interpret_body_as_xglCmdBindIndexBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdDraw:
        {
            return interpret_body_as_xglCmdDraw(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexed:
        {
            return interpret_body_as_xglCmdDrawIndexed(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdDrawIndirect:
        {
            return interpret_body_as_xglCmdDrawIndirect(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexedIndirect:
        {
            return interpret_body_as_xglCmdDrawIndexedIndirect(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdDispatch:
        {
            return interpret_body_as_xglCmdDispatch(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdDispatchIndirect:
        {
            return interpret_body_as_xglCmdDispatchIndirect(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdCopyBuffer:
        {
            return interpret_body_as_xglCmdCopyBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdCopyImage:
        {
            return interpret_body_as_xglCmdCopyImage(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdCopyBufferToImage:
        {
            return interpret_body_as_xglCmdCopyBufferToImage(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdCopyImageToBuffer:
        {
            return interpret_body_as_xglCmdCopyImageToBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdCloneImageData:
        {
            return interpret_body_as_xglCmdCloneImageData(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdUpdateBuffer:
        {
            return interpret_body_as_xglCmdUpdateBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdFillBuffer:
        {
            return interpret_body_as_xglCmdFillBuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdClearColorImage:
        {
            return interpret_body_as_xglCmdClearColorImage(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdClearColorImageRaw:
        {
            return interpret_body_as_xglCmdClearColorImageRaw(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdClearDepthStencil:
        {
            return interpret_body_as_xglCmdClearDepthStencil(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdResolveImage:
        {
            return interpret_body_as_xglCmdResolveImage(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdSetEvent:
        {
            return interpret_body_as_xglCmdSetEvent(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdResetEvent:
        {
            return interpret_body_as_xglCmdResetEvent(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdWaitEvents:
        {
            return interpret_body_as_xglCmdWaitEvents(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdPipelineBarrier:
        {
            return interpret_body_as_xglCmdPipelineBarrier(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdBeginQuery:
        {
            return interpret_body_as_xglCmdBeginQuery(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdEndQuery:
        {
            return interpret_body_as_xglCmdEndQuery(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdResetQueryPool:
        {
            return interpret_body_as_xglCmdResetQueryPool(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdWriteTimestamp:
        {
            return interpret_body_as_xglCmdWriteTimestamp(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdInitAtomicCounters:
        {
            return interpret_body_as_xglCmdInitAtomicCounters(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdLoadAtomicCounters:
        {
            return interpret_body_as_xglCmdLoadAtomicCounters(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdSaveAtomicCounters:
        {
            return interpret_body_as_xglCmdSaveAtomicCounters(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateFramebuffer:
        {
            return interpret_body_as_xglCreateFramebuffer(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCreateRenderPass:
        {
            return interpret_body_as_xglCreateRenderPass(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdBeginRenderPass:
        {
            return interpret_body_as_xglCmdBeginRenderPass(pHeader)->header;
        }
        case GLV_TPI_XGL_xglCmdEndRenderPass:
        {
            return interpret_body_as_xglCmdEndRenderPass(pHeader)->header;
        }
        case GLV_TPI_XGL_xglDbgSetValidationLevel:
        {
            return interpret_body_as_xglDbgSetValidationLevel(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglDbgRegisterMsgCallback:
        {
            return interpret_body_as_xglDbgRegisterMsgCallback(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglDbgUnregisterMsgCallback:
        {
            return interpret_body_as_xglDbgUnregisterMsgCallback(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglDbgSetMessageFilter:
        {
            return interpret_body_as_xglDbgSetMessageFilter(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglDbgSetObjectTag:
        {
            return interpret_body_as_xglDbgSetObjectTag(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglDbgSetGlobalOption:
        {
            return interpret_body_as_xglDbgSetGlobalOption(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglDbgSetDeviceOption:
        {
            return interpret_body_as_xglDbgSetDeviceOption(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerBegin:
        {
            return interpret_body_as_xglCmdDbgMarkerBegin(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerEnd:
        {
            return interpret_body_as_xglCmdDbgMarkerEnd(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglWsiX11AssociateConnection:
        {
            return interpret_body_as_xglWsiX11AssociateConnection(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglWsiX11GetMSC:
        {
            return interpret_body_as_xglWsiX11GetMSC(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglWsiX11CreatePresentableImage:
        {
            return interpret_body_as_xglWsiX11CreatePresentableImage(pHeader)->pHeader;
        }
        case GLV_TPI_XGL_xglWsiX11QueuePresent:
        {
            return interpret_body_as_xglWsiX11QueuePresent(pHeader)->pHeader;
        }
        default:
            return NULL;
    }
    return NULL;
}
