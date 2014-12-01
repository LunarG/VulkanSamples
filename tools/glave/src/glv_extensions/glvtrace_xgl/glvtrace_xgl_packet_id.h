/**************************************************************************
 *
 * Copyright 2014 Valve Software
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/
#pragma once
#include "glv_trace_packet_utils.h"
#include "glv_interconnect.h"

#include "glvtrace_xgl_xgl_structs.h"
#include "glvtrace_xgl_xgldbg_structs.h"
#include "glvtrace_xgl_xglwsix11ext_structs.h"

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
    // xgl.h
    GLV_TPI_XGL_xglGetProcAddr = GLV_TPI_BEGIN_API_HERE,
    GLV_TPI_XGL_xglInitAndEnumerateGpus,
    GLV_TPI_XGL_xglGetGpuInfo,
    GLV_TPI_XGL_xglCreateDevice,
    GLV_TPI_XGL_xglDestroyDevice,
    GLV_TPI_XGL_xglGetExtensionSupport,
    GLV_TPI_XGL_xglEnumerateLayers,
    GLV_TPI_XGL_xglGetDeviceQueue,
    GLV_TPI_XGL_xglQueueSubmit,
    GLV_TPI_XGL_xglQueueSetGlobalMemReferences,
    GLV_TPI_XGL_xglQueueWaitIdle,
    GLV_TPI_XGL_xglDeviceWaitIdle,
    GLV_TPI_XGL_xglGetMemoryHeapCount,
    GLV_TPI_XGL_xglGetMemoryHeapInfo,
    GLV_TPI_XGL_xglAllocMemory,
    GLV_TPI_XGL_xglFreeMemory,
    GLV_TPI_XGL_xglSetMemoryPriority,
    GLV_TPI_XGL_xglMapMemory,
    GLV_TPI_XGL_xglUnmapMemory,
    GLV_TPI_XGL_xglPinSystemMemory,
    GLV_TPI_XGL_xglRemapVirtualMemoryPages,
    GLV_TPI_XGL_xglGetMultiGpuCompatibility,
    GLV_TPI_XGL_xglOpenSharedMemory,
    GLV_TPI_XGL_xglOpenSharedQueueSemaphore,
    GLV_TPI_XGL_xglOpenPeerMemory,
    GLV_TPI_XGL_xglOpenPeerImage,
    GLV_TPI_XGL_xglDestroyObject,
    GLV_TPI_XGL_xglGetObjectInfo,
    GLV_TPI_XGL_xglBindObjectMemory,
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
    GLV_TPI_XGL_xglCreateImage,
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
    GLV_TPI_XGL_xglCreateDescriptorSet,
    GLV_TPI_XGL_xglBeginDescriptorSetUpdate,
    GLV_TPI_XGL_xglEndDescriptorSetUpdate,
    GLV_TPI_XGL_xglAttachSamplerDescriptors,
    GLV_TPI_XGL_xglAttachImageViewDescriptors,
    GLV_TPI_XGL_xglAttachMemoryViewDescriptors,
    GLV_TPI_XGL_xglAttachNestedDescriptors,
    GLV_TPI_XGL_xglClearDescriptorSetSlots,
    GLV_TPI_XGL_xglCreateViewportState,
    GLV_TPI_XGL_xglCreateRasterState,
    GLV_TPI_XGL_xglCreateMsaaState,
    GLV_TPI_XGL_xglCreateColorBlendState,
    GLV_TPI_XGL_xglCreateDepthStencilState,
    GLV_TPI_XGL_xglCreateCommandBuffer,
    GLV_TPI_XGL_xglBeginCommandBuffer,
    GLV_TPI_XGL_xglEndCommandBuffer,
    GLV_TPI_XGL_xglResetCommandBuffer,
    GLV_TPI_XGL_xglCmdBindPipeline,
    GLV_TPI_XGL_xglCmdBindPipelineDelta,
    GLV_TPI_XGL_xglCmdBindStateObject,
    GLV_TPI_XGL_xglCmdBindDescriptorSet,
    GLV_TPI_XGL_xglCmdBindDynamicMemoryView,
    GLV_TPI_XGL_xglCmdBindIndexData,
    GLV_TPI_XGL_xglCmdBindVertexData,
    GLV_TPI_XGL_xglCmdBindAttachments,
    GLV_TPI_XGL_xglCmdPrepareMemoryRegions,
    GLV_TPI_XGL_xglCmdPrepareImages,
    GLV_TPI_XGL_xglCmdDraw,
    GLV_TPI_XGL_xglCmdDrawIndexed,
    GLV_TPI_XGL_xglCmdDrawIndirect,
    GLV_TPI_XGL_xglCmdDrawIndexedIndirect,
    GLV_TPI_XGL_xglCmdDispatch,
    GLV_TPI_XGL_xglCmdDispatchIndirect,
    GLV_TPI_XGL_xglCmdCopyMemory,
    GLV_TPI_XGL_xglCmdCopyImage,
    GLV_TPI_XGL_xglCmdCopyMemoryToImage,
    GLV_TPI_XGL_xglCmdCopyImageToMemory,
    GLV_TPI_XGL_xglCmdCloneImageData,
    GLV_TPI_XGL_xglCmdUpdateMemory,
    GLV_TPI_XGL_xglCmdFillMemory,
    GLV_TPI_XGL_xglCmdClearColorImage,
    GLV_TPI_XGL_xglCmdClearColorImageRaw,
    GLV_TPI_XGL_xglCmdClearDepthStencil,
    GLV_TPI_XGL_xglCmdResolveImage,
    GLV_TPI_XGL_xglCmdSetEvent,
    GLV_TPI_XGL_xglCmdResetEvent,
    GLV_TPI_XGL_xglCmdMemoryAtomic,
    GLV_TPI_XGL_xglCmdBeginQuery,
    GLV_TPI_XGL_xglCmdEndQuery,
    GLV_TPI_XGL_xglCmdResetQueryPool,
    GLV_TPI_XGL_xglCmdWriteTimestamp,
    GLV_TPI_XGL_xglCmdInitAtomicCounters,
    GLV_TPI_XGL_xglCmdLoadAtomicCounters,
    GLV_TPI_XGL_xglCmdSaveAtomicCounters,

    // xglDbg.h
    GLV_TPI_XGL_xglDbgSetValidationLevel,
    GLV_TPI_XGL_xglDbgRegisterMsgCallback,
    GLV_TPI_XGL_xglDbgUnregisterMsgCallback,
    GLV_TPI_XGL_xglDbgSetMessageFilter,
    GLV_TPI_XGL_xglDbgSetObjectTag,
    GLV_TPI_XGL_xglDbgSetGlobalOption,
    GLV_TPI_XGL_xglDbgSetDeviceOption,
    GLV_TPI_XGL_xglCmdDbgMarkerBegin,
    GLV_TPI_XGL_xglCmdDbgMarkerEnd,

    //xglWsiX11Ext.h
    GLV_TPI_XGL_xglWsiX11AssociateConnection,
    GLV_TPI_XGL_xglWsiX11GetMSC,
    GLV_TPI_XGL_xglWsiX11CreatePresentableImage,
    GLV_TPI_XGL_xglWsiX11QueuePresent,
};


static glv_trace_packet_header* interpret_trace_packet_xgl(glv_trace_packet_header* pHeader)
{
    if (pHeader == NULL)
    {
        return NULL;
    }

    switch (pHeader->packet_id)
    {
        case GLV_TPI_XGL_xglInitAndEnumerateGpus:
        {
            return interpret_body_as_xglInitAndEnumerateGpus(pHeader)->header;
        }
        case GLV_TPI_XGL_xglGetGpuInfo:
        {
            return interpret_body_as_xglGetGpuInfo(pHeader)->header;
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
            struct_xglQueueWaitIdle *pPacket = interpret_body_as_xglQueueWaitIdle(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglDeviceWaitIdle:
        {
            struct_xglDeviceWaitIdle *pPacket = interpret_body_as_xglDeviceWaitIdle(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglGetMemoryHeapCount:
        {
            struct_xglGetMemoryHeapCount *pPacket = interpret_body_as_xglGetMemoryHeapCount(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglGetMemoryHeapInfo:
        {
            struct_xglGetMemoryHeapInfo *pPacket = interpret_body_as_xglGetMemoryHeapInfo(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglAllocMemory:
        {
            struct_xglAllocMemory *pPacket = interpret_body_as_xglAllocMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglFreeMemory:
        {
            struct_xglFreeMemory *pPacket = interpret_body_as_xglFreeMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglSetMemoryPriority:
        {
            struct_xglSetMemoryPriority *pPacket = interpret_body_as_xglSetMemoryPriority(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglMapMemory:
        {
            struct_xglMapMemory *pPacket = interpret_body_as_xglMapMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglUnmapMemory:
        {
            struct_xglUnmapMemory *pPacket = interpret_body_as_xglUnmapMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglPinSystemMemory:
        {
            struct_xglPinSystemMemory *pPacket = interpret_body_as_xglPinSystemMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglRemapVirtualMemoryPages:
        {
            struct_xglRemapVirtualMemoryPages *pPacket = interpret_body_as_xglRemapVirtualMemoryPages(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglGetMultiGpuCompatibility:
        {
            struct_xglGetMultiGpuCompatibility *pPacket = interpret_body_as_xglGetMultiGpuCompatibility(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglOpenSharedMemory:
        {
            struct_xglOpenSharedMemory *pPacket = interpret_body_as_xglOpenSharedMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglOpenSharedQueueSemaphore:
        {
            struct_xglOpenSharedQueueSemaphore *pPacket = interpret_body_as_xglOpenSharedQueueSemaphore(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglOpenPeerMemory:
        {
            struct_xglOpenPeerMemory *pPacket = interpret_body_as_xglOpenPeerMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglOpenPeerImage:
        {
            struct_xglOpenPeerImage *pPacket = interpret_body_as_xglOpenPeerImage(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglDestroyObject:
        {
            struct_xglDestroyObject *pPacket = interpret_body_as_xglDestroyObject(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglGetObjectInfo:
        {
            struct_xglGetObjectInfo *pPacket = interpret_body_as_xglGetObjectInfo(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglBindObjectMemory:
        {
            struct_xglBindObjectMemory *pPacket = interpret_body_as_xglBindObjectMemory(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglCreateFence:
        {
            struct_xglCreateFence* pPacket = interpret_body_as_xglCreateFence(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglGetFenceStatus:
        {
            struct_xglGetFenceStatus* pPacket = interpret_body_as_xglGetFenceStatus(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglWaitForFences:
        {
            struct_xglWaitForFences* pPacket = interpret_body_as_xglWaitForFences(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglCreateQueueSemaphore:
        {
            struct_xglCreateQueueSemaphore* pPacket = interpret_body_as_xglCreateQueueSemaphore(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglSignalQueueSemaphore:
        {
            struct_xglSignalQueueSemaphore* pPacket = interpret_body_as_xglSignalQueueSemaphore(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglWaitQueueSemaphore:
        {
            struct_xglWaitQueueSemaphore* pPacket = interpret_body_as_xglWaitQueueSemaphore(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglCreateEvent:
        {
            struct_xglCreateEvent* pPacket = interpret_body_as_xglCreateEvent(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglGetEventStatus:
        {
            struct_xglGetEventStatus* pPacket = interpret_body_as_xglGetEventStatus(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglSetEvent:
        {
            struct_xglSetEvent* pPacket = interpret_body_as_xglSetEvent(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglResetEvent:
        {
            struct_xglResetEvent* pPacket = interpret_body_as_xglResetEvent(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglCreateQueryPool:
        {
            struct_xglCreateQueryPool* pPacket = interpret_body_as_xglCreateQueryPool(pHeader);
            return pPacket->header;
        }
        case  GLV_TPI_XGL_xglGetQueryPoolResults:
        {
            struct_xglGetQueryPoolResults* pPacket = interpret_body_as_xglGetQueryPoolResults(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglGetFormatInfo:
        {
            struct_xglGetFormatInfo* pPacket = interpret_body_as_xglGetFormatInfo(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateImage:
        {
            struct_xglCreateImage* pPacket = interpret_body_as_xglCreateImage(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglGetImageSubresourceInfo:
        {
            struct_xglGetImageSubresourceInfo* pPacket = interpret_body_as_xglGetImageSubresourceInfo(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateImageView:
        {
            struct_xglCreateImageView* pPacket = interpret_body_as_xglCreateImageView(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateColorAttachmentView:
        {
            struct_xglCreateColorAttachmentView* pPacket = interpret_body_as_xglCreateColorAttachmentView(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateDepthStencilView:
        {
            struct_xglCreateDepthStencilView* pPacket = interpret_body_as_xglCreateDepthStencilView(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateShader:
        {
            struct_xglCreateShader* pPacket = interpret_body_as_xglCreateShader(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateGraphicsPipeline:
        {
            struct_xglCreateGraphicsPipeline* pPacket = interpret_body_as_xglCreateGraphicsPipeline(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateComputePipeline:
        {
            struct_xglCreateComputePipeline* pPacket = interpret_body_as_xglCreateComputePipeline(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglStorePipeline:
        {
            struct_xglStorePipeline* pPacket = interpret_body_as_xglStorePipeline(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglLoadPipeline:
        {
            struct_xglLoadPipeline* pPacket = interpret_body_as_xglLoadPipeline(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreatePipelineDelta:
        {
            struct_xglCreatePipelineDelta* pPacket = interpret_body_as_xglCreatePipelineDelta(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateSampler:
        {
            struct_xglCreateSampler* pPacket = interpret_body_as_xglCreateSampler(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateDescriptorSet:
        {
            struct_xglCreateDescriptorSet* pPacket = interpret_body_as_xglCreateDescriptorSet(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglBeginDescriptorSetUpdate:
        {
            struct_xglBeginDescriptorSetUpdate* pPacket = interpret_body_as_xglBeginDescriptorSetUpdate(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglEndDescriptorSetUpdate:
        {
            struct_xglEndDescriptorSetUpdate* pPacket = interpret_body_as_xglEndDescriptorSetUpdate(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglAttachSamplerDescriptors:
        {
            struct_xglAttachSamplerDescriptors* pPacket = interpret_body_as_xglAttachSamplerDescriptors(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglAttachImageViewDescriptors:
        {
            struct_xglAttachImageViewDescriptors* pPacket = interpret_body_as_xglAttachImageViewDescriptors(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglAttachMemoryViewDescriptors:
        {
            struct_xglAttachMemoryViewDescriptors* pPacket = interpret_body_as_xglAttachMemoryViewDescriptors(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglAttachNestedDescriptors:
        {
            struct_xglAttachNestedDescriptors* pPacket = interpret_body_as_xglAttachNestedDescriptors(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglClearDescriptorSetSlots:
        {
            struct_xglClearDescriptorSetSlots* pPacket = interpret_body_as_xglClearDescriptorSetSlots(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateViewportState:
        {
            struct_xglCreateViewportState* pPacket = interpret_body_as_xglCreateViewportState(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateRasterState:
        {
            struct_xglCreateRasterState* pPacket = interpret_body_as_xglCreateRasterState(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateMsaaState:
        {
            struct_xglCreateMsaaState* pPacket = interpret_body_as_xglCreateMsaaState(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateColorBlendState:
        {
            struct_xglCreateColorBlendState* pPacket = interpret_body_as_xglCreateColorBlendState(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateDepthStencilState:
        {
            struct_xglCreateDepthStencilState* pPacket = interpret_body_as_xglCreateDepthStencilState(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCreateCommandBuffer:
        {
            struct_xglCreateCommandBuffer* pPacket = interpret_body_as_xglCreateCommandBuffer(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglBeginCommandBuffer:
        {
            struct_xglBeginCommandBuffer* pPacket = interpret_body_as_xglBeginCommandBuffer(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglEndCommandBuffer:
        {
            struct_xglEndCommandBuffer* pPacket = interpret_body_as_xglEndCommandBuffer(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglResetCommandBuffer:
        {
            struct_xglResetCommandBuffer* pPacket = interpret_body_as_xglResetCommandBuffer(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdBindPipeline:
        {
            struct_xglCmdBindPipeline* pPacket = interpret_body_as_xglCmdBindPipeline(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdBindPipelineDelta:
        {
            struct_xglCmdBindPipelineDelta* pPacket = interpret_body_as_xglCmdBindPipelineDelta(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdBindStateObject:
        {
            struct_xglCmdBindStateObject* pPacket = interpret_body_as_xglCmdBindStateObject(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdBindDescriptorSet:
        {
            struct_xglCmdBindDescriptorSet* pPacket = interpret_body_as_xglCmdBindDescriptorSet(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdBindDynamicMemoryView:
        {
            struct_xglCmdBindDynamicMemoryView* pPacket = interpret_body_as_xglCmdBindDynamicMemoryView(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdBindIndexData:
        {
            struct_xglCmdBindIndexData* pPacket = interpret_body_as_xglCmdBindIndexData(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdBindVertexData:
        {
            struct_xglCmdBindVertexData* pPacket = interpret_body_as_xglCmdBindVertexData(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdBindAttachments:
        {
            struct_xglCmdBindAttachments* pPacket = interpret_body_as_xglCmdBindAttachments(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdPrepareMemoryRegions:
        {
            struct_xglCmdPrepareMemoryRegions* pPacket = interpret_body_as_xglCmdPrepareMemoryRegions(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdPrepareImages:
        {
            struct_xglCmdPrepareImages* pPacket = interpret_body_as_xglCmdPrepareImages(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdDraw:
        {
            struct_xglCmdDraw* pPacket = interpret_body_as_xglCmdDraw(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexed:
        {
            struct_xglCmdDrawIndexed* pPacket = interpret_body_as_xglCmdDrawIndexed(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdDrawIndirect:
        {
            struct_xglCmdDrawIndirect* pPacket = interpret_body_as_xglCmdDrawIndirect(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexedIndirect:
        {
            struct_xglCmdDrawIndexedIndirect* pPacket = interpret_body_as_xglCmdDrawIndexedIndirect(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdDispatch:
        {
            struct_xglCmdDispatch* pPacket = interpret_body_as_xglCmdDispatch(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdDispatchIndirect:
        {
            struct_xglCmdDispatchIndirect* pPacket = interpret_body_as_xglCmdDispatchIndirect(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdCopyMemory:
        {
            struct_xglCmdCopyMemory* pPacket = interpret_body_as_xglCmdCopyMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdCopyImage:
        {
            struct_xglCmdCopyImage* pPacket = interpret_body_as_xglCmdCopyImage(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdCopyMemoryToImage:
        {
            struct_xglCmdCopyMemoryToImage* pPacket = interpret_body_as_xglCmdCopyMemoryToImage(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdCopyImageToMemory:
        {
            struct_xglCmdCopyImageToMemory* pPacket = interpret_body_as_xglCmdCopyImageToMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdCloneImageData:
        {
            struct_xglCmdCloneImageData* pPacket = interpret_body_as_xglCmdCloneImageData(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdUpdateMemory:
        {
            struct_xglCmdUpdateMemory* pPacket = interpret_body_as_xglCmdUpdateMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdFillMemory:
        {
            struct_xglCmdFillMemory* pPacket = interpret_body_as_xglCmdFillMemory(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdClearColorImage:
        {
            struct_xglCmdClearColorImage* pPacket = interpret_body_as_xglCmdClearColorImage(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdClearColorImageRaw:
        {
            struct_xglCmdClearColorImageRaw* pPacket = interpret_body_as_xglCmdClearColorImageRaw(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdClearDepthStencil:
        {
            struct_xglCmdClearDepthStencil* pPacket = interpret_body_as_xglCmdClearDepthStencil(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdResolveImage:
        {
            struct_xglCmdResolveImage* pPacket = interpret_body_as_xglCmdResolveImage(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdSetEvent:
        {
            struct_xglCmdSetEvent* pPacket = interpret_body_as_xglCmdSetEvent(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdResetEvent:
        {
            struct_xglCmdResetEvent* pPacket = interpret_body_as_xglCmdResetEvent(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdMemoryAtomic:
        {
            struct_xglCmdMemoryAtomic* pPacket = interpret_body_as_xglCmdMemoryAtomic(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdBeginQuery:
        {
            struct_xglCmdBeginQuery* pPacket = interpret_body_as_xglCmdBeginQuery(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdEndQuery:
        {
            struct_xglCmdEndQuery* pPacket = interpret_body_as_xglCmdEndQuery(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdResetQueryPool:
        {
            struct_xglCmdResetQueryPool* pPacket = interpret_body_as_xglCmdResetQueryPool(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdWriteTimestamp:
        {
            struct_xglCmdWriteTimestamp* pPacket = interpret_body_as_xglCmdWriteTimestamp(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdInitAtomicCounters:
        {
            struct_xglCmdInitAtomicCounters* pPacket = interpret_body_as_xglCmdInitAtomicCounters(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdLoadAtomicCounters:
        {
            struct_xglCmdLoadAtomicCounters* pPacket = interpret_body_as_xglCmdLoadAtomicCounters(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglCmdSaveAtomicCounters:
        {
            struct_xglCmdSaveAtomicCounters* pPacket = interpret_body_as_xglCmdSaveAtomicCounters(pHeader);
            return pPacket->header;
        }
        case GLV_TPI_XGL_xglDbgSetValidationLevel:
        {
            struct_xglDbgSetValidationLevel *pPacket = interpret_body_as_xglDbgSetValidationLevel(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglDbgRegisterMsgCallback:
        {
            struct_xglDbgRegisterMsgCallback *pPacket = interpret_body_as_xglDbgRegisterMsgCallback(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglDbgUnregisterMsgCallback:
        {
            struct_xglDbgUnregisterMsgCallback *pPacket = interpret_body_as_xglDbgUnregisterMsgCallback(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglDbgSetMessageFilter:
        {
            struct_xglDbgSetMessageFilter *pPacket = interpret_body_as_xglDbgSetMessageFilter(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglDbgSetObjectTag:
        {
            struct_xglDbgSetObjectTag *pPacket = interpret_body_as_xglDbgSetObjectTag(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglDbgSetGlobalOption:
        {
            struct_xglDbgSetGlobalOption *pPacket = interpret_body_as_xglDbgSetGlobalOption(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglDbgSetDeviceOption:
        {
            struct_xglDbgSetDeviceOption *pPacket = interpret_body_as_xglDbgSetDeviceOption(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerBegin:
        {
            struct_xglCmdDbgMarkerBegin *pPacket = interpret_body_as_xglCmdDbgMarkerBegin(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerEnd:
        {
            struct_xglCmdDbgMarkerEnd *pPacket = interpret_body_as_xglCmdDbgMarkerEnd(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglWsiX11AssociateConnection:
        {
            struct_xglWsiX11AssociateConnection *pPacket = interpret_body_as_xglWsiX11AssociateConnection(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglWsiX11GetMSC:
        {
            struct_xglWsiX11GetMSC *pPacket = interpret_body_as_xglWsiX11GetMSC(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglWsiX11CreatePresentableImage:
        {
            struct_xglWsiX11CreatePresentableImage *pPacket = interpret_body_as_xglWsiX11CreatePresentableImage(pHeader);
            return pPacket->pHeader;
        }
        case GLV_TPI_XGL_xglWsiX11QueuePresent:
        {
            struct_xglWsiX11QueuePresent *pPacket = interpret_body_as_xglWsiX11QueuePresent(pHeader);
            return pPacket->pHeader;
        }
        default:
            return NULL;
    }

    return NULL;
}
