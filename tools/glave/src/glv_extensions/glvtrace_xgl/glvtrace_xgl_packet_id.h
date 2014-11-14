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

#define SEND_ENTRYPOINT_ID(entrypoint) \
    FileLike* pFile; \
    pFile = glv_FileLike_create_msg(gMessageStream); \
    glv_trace_set_trace_file(pFile); \
    glv_TraceInfo(#entrypoint "\n");

#define SEND_ENTRYPOINT_PARAMS(entrypoint, ...) \
    FileLike* pFile; \
    pFile = glv_FileLike_create_msg(gMessageStream); \
    glv_trace_set_trace_file(pFile); \
    glv_TraceInfo(entrypoint, __VA_ARGS__);

#define CREATE_TRACE_PACKET(entrypoint, buffer_bytes_needed) \
    pHeader = glv_create_trace_packet(GLV_TID_XGL, GLV_TPI_XGL_##entrypoint, sizeof(struct_##entrypoint), buffer_bytes_needed);

#define FINISH_TRACE_PACKET() \
    glv_finalize_trace_packet(pHeader); \
    glv_write_trace_packet(pHeader, pFile); \
    glv_delete_trace_packet(&pHeader); \
    GLV_DELETE(pFile);

enum GLV_TRACE_PACKET_ID_XGL
{
    // xgl.h
    GLV_TPI_XGL_xglInitAndEnumerateGpus = GLV_TPI_BEGIN_API_HERE,
    GLV_TPI_XGL_xglGetGpuInfo,
    GLV_TPI_XGL_xglCreateDevice,
    GLV_TPI_XGL_xglDestroyDevice,
    GLV_TPI_XGL_xglGetExtensionSupport,
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
