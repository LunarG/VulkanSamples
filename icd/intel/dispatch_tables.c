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

#include "icd.h"
#include "dev.h"
#include "dset.h"
#include "event.h"
#include "fence.h"
#include "format.h"
#include "img.h"
#include "gpu.h"
#include "mem.h"
#include "obj.h"
#include "query.h"
#include "queue.h"
#include "sampler.h"
#include "state.h"
#include "view.h"
#include "dispatch_tables.h"

static XGL_RESULT XGLAPI intelQueueSubmit(
    XGL_QUEUE                                   queue,
    XGL_UINT                                    cmdBufferCount,
    const XGL_CMD_BUFFER*                       pCmdBuffers,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs,
    XGL_FENCE                                   fence)
{
    /* need XGL_CMD_BUFFER first */
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelPinSystemMemory(
    XGL_DEVICE                                  device,
    const XGL_VOID*                             pSysMem,
    XGL_SIZE                                    memSize,
    XGL_GPU_MEMORY*                             pMem)
{
    /* add DRM_I915_GEM_USERPTR to wisys first */
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelRemapVirtualMemoryPages(
    XGL_DEVICE                                  device,
    XGL_UINT                                    rangeCount,
    const XGL_VIRTUAL_MEMORY_REMAP_RANGE*       pRanges,
    XGL_UINT                                    preWaitSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*                  pPreWaitSemaphores,
    XGL_UINT                                    postSignalSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*                  pPostSignalSemaphores)
{
    /* no kernel support */
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelGetMultiGpuCompatibility(
    XGL_PHYSICAL_GPU                            gpu0,
    XGL_PHYSICAL_GPU                            gpu1,
    XGL_GPU_COMPATIBILITY_INFO*                 pInfo)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelOpenSharedMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_OPEN_INFO*                 pOpenInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelOpenSharedQueueSemaphore(
    XGL_DEVICE                                  device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO*        pOpenInfo,
    XGL_QUEUE_SEMAPHORE*                        pSemaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelOpenPeerMemory(
    XGL_DEVICE                                  device,
    const XGL_PEER_MEMORY_OPEN_INFO*            pOpenInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelOpenPeerImage(
    XGL_DEVICE                                  device,
    const XGL_PEER_IMAGE_OPEN_INFO*             pOpenInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelCreateQueueSemaphore(
    XGL_DEVICE                                  device,
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO*      pCreateInfo,
    XGL_QUEUE_SEMAPHORE*                        pSemaphore)
{
    /*
     * We want to find an unused semaphore register and initialize it.  Signal
     * will increment the register.  Wait will atomically decrement it and
     * block if the value is zero, or a large constant N if we do not want to
     * go negative.
     *
     * XXX However, MI_SEMAPHORE_MBOX does not seem to have the flexibility.
     */
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelSignalQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelWaitQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelCreateShader(
    XGL_DEVICE                                  device,
    const XGL_SHADER_CREATE_INFO*               pCreateInfo,
    XGL_SHADER*                                 pShader)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelCreateGraphicsPipeline(
    XGL_DEVICE                                  device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    XGL_PIPELINE*                               pPipeline)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelCreateComputePipeline(
    XGL_DEVICE                                  device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO*     pCreateInfo,
    XGL_PIPELINE*                               pPipeline)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelStorePipeline(
    XGL_PIPELINE                                pipeline,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelLoadPipeline(
    XGL_DEVICE                                  device,
    XGL_SIZE                                    dataSize,
    const XGL_VOID*                             pData,
    XGL_PIPELINE*                               pPipeline)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelCreatePipelineDelta(
    XGL_DEVICE                                  device,
    XGL_PIPELINE                                p1,
    XGL_PIPELINE                                p2,
    XGL_PIPELINE_DELTA*                         delta)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelCreateCommandBuffer(
    XGL_DEVICE                                  device,
    const XGL_CMD_BUFFER_CREATE_INFO*           pCreateInfo,
    XGL_CMD_BUFFER*                             pCmdBuffer)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelBeginCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_FLAGS                                   flags)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelEndCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelResetCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_VOID XGLAPI intelCmdBindPipeline(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE                                pipeline)
{
}

static XGL_VOID XGLAPI intelCmdBindPipelineDelta(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE_DELTA                          delta)
{
}

static XGL_VOID XGLAPI intelCmdBindStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_STATE_OBJECT                            state)
{
}

static XGL_VOID XGLAPI intelCmdBindDescriptorSet(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    index,
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    slotOffset)
{
}

static XGL_VOID XGLAPI intelCmdBindDynamicMemoryView(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView)
{
}

static XGL_VOID XGLAPI intelCmdBindIndexData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType)
{
}

static XGL_VOID XGLAPI intelCmdBindAttachments(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    colorAttachmentCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*       pColorAttachments,
    const XGL_DEPTH_STENCIL_BIND_INFO*          pDepthStencilAttachment)
{
}

static XGL_VOID XGLAPI intelCmdPrepareMemoryRegions(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_MEMORY_STATE_TRANSITION*          pStateTransitions)
{
}

static XGL_VOID XGLAPI intelCmdPrepareImages(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_IMAGE_STATE_TRANSITION*           pStateTransitions)
{
}

static XGL_VOID XGLAPI intelCmdDraw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstVertex,
    XGL_UINT                                    vertexCount,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount)
{
}

static XGL_VOID XGLAPI intelCmdDrawIndexed(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstIndex,
    XGL_UINT                                    indexCount,
    XGL_INT                                     vertexOffset,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount)
{
}

static XGL_VOID XGLAPI intelCmdDrawIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride)
{
}

static XGL_VOID XGLAPI intelCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride)
{
}

static XGL_VOID XGLAPI intelCmdDispatch(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    x,
    XGL_UINT                                    y,
    XGL_UINT                                    z)
{
}

static XGL_VOID XGLAPI intelCmdDispatchIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset)
{
}

static XGL_VOID XGLAPI intelCmdCopyMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_COPY*                      pRegions)
{
}

static XGL_VOID XGLAPI intelCmdCopyImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_IMAGE_COPY*                       pRegions)
{
}

static XGL_VOID XGLAPI intelCmdCopyMemoryToImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions)
{
}

static XGL_VOID XGLAPI intelCmdCopyImageToMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions)
{
}

static XGL_VOID XGLAPI intelCmdCloneImageData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_STATE                             srcImageState,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_STATE                             destImageState)
{
}

static XGL_VOID XGLAPI intelCmdUpdateMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                dataSize,
    const XGL_UINT32*                           pData)
{
}

static XGL_VOID XGLAPI intelCmdFillMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                fillSize,
    XGL_UINT32                                  data)
{
}

static XGL_VOID XGLAPI intelCmdClearColorImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_FLOAT                             color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
}

static XGL_VOID XGLAPI intelCmdClearColorImageRaw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_UINT32                            color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
}

static XGL_VOID XGLAPI intelCmdClearDepthStencil(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    XGL_FLOAT                                   depth,
    XGL_UINT32                                  stencil,
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
}

static XGL_VOID XGLAPI intelCmdResolveImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    rectCount,
    const XGL_IMAGE_RESOLVE*                    pRects)
{
}

static XGL_VOID XGLAPI intelCmdSetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event)
{
}

static XGL_VOID XGLAPI intelCmdResetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event)
{
}

static XGL_VOID XGLAPI intelCmdMemoryAtomic(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_UINT64                                  srcData,
    XGL_ATOMIC_OP                               atomicOp)
{
}

static XGL_VOID XGLAPI intelCmdBeginQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot,
    XGL_FLAGS                                   flags)
{
}

static XGL_VOID XGLAPI intelCmdEndQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot)
{
}

static XGL_VOID XGLAPI intelCmdResetQueryPool(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    startQuery,
    XGL_UINT                                    queryCount)
{
}

static XGL_VOID XGLAPI intelCmdWriteTimestamp(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_TIMESTAMP_TYPE                          timestampType,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset)
{
}

static XGL_VOID XGLAPI intelCmdInitAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    const XGL_UINT32*                           pData)
{
}

static XGL_VOID XGLAPI intelCmdLoadAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_SIZE                                srcOffset)
{
}

static XGL_VOID XGLAPI intelCmdSaveAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset)
{
}

static XGL_RESULT XGLAPI intelDbgSetValidationLevel(
    XGL_DEVICE                                  device,
    XGL_VALIDATION_LEVEL                        validationLevel)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelDbgSetMessageFilter(
    XGL_DEVICE                                  device,
    XGL_INT                                     msgCode,
    XGL_DBG_MSG_FILTER                          filter)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelDbgSetObjectTag(
    XGL_BASE_OBJECT                             object,
    XGL_SIZE                                    tagSize,
    const XGL_VOID*                             pTag)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_RESULT XGLAPI intelDbgSetDeviceOption(
    XGL_DEVICE                                  device,
    XGL_DBG_DEVICE_OPTION                       dbgOption,
    XGL_SIZE                                    dataSize,
    const XGL_VOID*                             pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

static XGL_VOID XGLAPI intelCmdDbgMarkerBegin(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_CHAR*                             pMarker)
{
}

static XGL_VOID XGLAPI intelCmdDbgMarkerEnd(
    XGL_CMD_BUFFER                              cmdBuffer)
{
}

const struct icd_dispatch_table intel_normal_dispatch_table = {
    .InitAndEnumerateGpus = xglInitAndEnumerateGpus,
    .GetGpuInfo = intelGetGpuInfo,
    .CreateDevice = intelCreateDevice,
    .DestroyDevice = intelDestroyDevice,
    .GetExtensionSupport = intelGetExtensionSupport,
    .GetDeviceQueue = intelGetDeviceQueue,
    .QueueSubmit = intelQueueSubmit,
    .QueueSetGlobalMemReferences = intelQueueSetGlobalMemReferences,
    .QueueWaitIdle = intelQueueWaitIdle,
    .DeviceWaitIdle = intelDeviceWaitIdle,
    .GetMemoryHeapCount = intelGetMemoryHeapCount,
    .GetMemoryHeapInfo = intelGetMemoryHeapInfo,
    .AllocMemory = intelAllocMemory,
    .FreeMemory = intelFreeMemory,
    .SetMemoryPriority = intelSetMemoryPriority,
    .MapMemory = intelMapMemory,
    .UnmapMemory = intelUnmapMemory,
    .PinSystemMemory = intelPinSystemMemory,
    .RemapVirtualMemoryPages = intelRemapVirtualMemoryPages,
    .GetMultiGpuCompatibility = intelGetMultiGpuCompatibility,
    .OpenSharedMemory = intelOpenSharedMemory,
    .OpenSharedQueueSemaphore = intelOpenSharedQueueSemaphore,
    .OpenPeerMemory = intelOpenPeerMemory,
    .OpenPeerImage = intelOpenPeerImage,
    .DestroyObject = intelDestroyObject,
    .GetObjectInfo = intelGetObjectInfo,
    .BindObjectMemory = intelBindObjectMemory,
    .CreateFence = intelCreateFence,
    .GetFenceStatus = intelGetFenceStatus,
    .WaitForFences = intelWaitForFences,
    .CreateQueueSemaphore = intelCreateQueueSemaphore,
    .SignalQueueSemaphore = intelSignalQueueSemaphore,
    .WaitQueueSemaphore = intelWaitQueueSemaphore,
    .CreateEvent = intelCreateEvent,
    .GetEventStatus = intelGetEventStatus,
    .SetEvent = intelSetEvent,
    .ResetEvent = intelResetEvent,
    .CreateQueryPool = intelCreateQueryPool,
    .GetQueryPoolResults = intelGetQueryPoolResults,
    .GetFormatInfo = intelGetFormatInfo,
    .CreateImage = intelCreateImage,
    .GetImageSubresourceInfo = intelGetImageSubresourceInfo,
    .CreateImageView = intelCreateImageView,
    .CreateColorAttachmentView = intelCreateColorAttachmentView,
    .CreateDepthStencilView = intelCreateDepthStencilView,
    .CreateShader = intelCreateShader,
    .CreateGraphicsPipeline = intelCreateGraphicsPipeline,
    .CreateComputePipeline = intelCreateComputePipeline,
    .StorePipeline = intelStorePipeline,
    .LoadPipeline = intelLoadPipeline,
    .CreatePipelineDelta = intelCreatePipelineDelta,
    .CreateSampler = intelCreateSampler,
    .CreateDescriptorSet = intelCreateDescriptorSet,
    .BeginDescriptorSetUpdate = intelBeginDescriptorSetUpdate,
    .EndDescriptorSetUpdate = intelEndDescriptorSetUpdate,
    .AttachSamplerDescriptors = intelAttachSamplerDescriptors,
    .AttachImageViewDescriptors = intelAttachImageViewDescriptors,
    .AttachMemoryViewDescriptors = intelAttachMemoryViewDescriptors,
    .AttachNestedDescriptors = intelAttachNestedDescriptors,
    .ClearDescriptorSetSlots = intelClearDescriptorSetSlots,
    .CreateViewportState = intelCreateViewportState,
    .CreateRasterState = intelCreateRasterState,
    .CreateMsaaState = intelCreateMsaaState,
    .CreateColorBlendState = intelCreateColorBlendState,
    .CreateDepthStencilState = intelCreateDepthStencilState,
    .CreateCommandBuffer = intelCreateCommandBuffer,
    .BeginCommandBuffer = intelBeginCommandBuffer,
    .EndCommandBuffer = intelEndCommandBuffer,
    .ResetCommandBuffer = intelResetCommandBuffer,
    .CmdBindPipeline = intelCmdBindPipeline,
    .CmdBindPipelineDelta = intelCmdBindPipelineDelta,
    .CmdBindStateObject = intelCmdBindStateObject,
    .CmdBindDescriptorSet = intelCmdBindDescriptorSet,
    .CmdBindDynamicMemoryView = intelCmdBindDynamicMemoryView,
    .CmdBindIndexData = intelCmdBindIndexData,
    .CmdBindAttachments = intelCmdBindAttachments,
    .CmdPrepareMemoryRegions = intelCmdPrepareMemoryRegions,
    .CmdPrepareImages = intelCmdPrepareImages,
    .CmdDraw = intelCmdDraw,
    .CmdDrawIndexed = intelCmdDrawIndexed,
    .CmdDrawIndirect = intelCmdDrawIndirect,
    .CmdDrawIndexedIndirect = intelCmdDrawIndexedIndirect,
    .CmdDispatch = intelCmdDispatch,
    .CmdDispatchIndirect = intelCmdDispatchIndirect,
    .CmdCopyMemory = intelCmdCopyMemory,
    .CmdCopyImage = intelCmdCopyImage,
    .CmdCopyMemoryToImage = intelCmdCopyMemoryToImage,
    .CmdCopyImageToMemory = intelCmdCopyImageToMemory,
    .CmdCloneImageData = intelCmdCloneImageData,
    .CmdUpdateMemory = intelCmdUpdateMemory,
    .CmdFillMemory = intelCmdFillMemory,
    .CmdClearColorImage = intelCmdClearColorImage,
    .CmdClearColorImageRaw = intelCmdClearColorImageRaw,
    .CmdClearDepthStencil = intelCmdClearDepthStencil,
    .CmdResolveImage = intelCmdResolveImage,
    .CmdSetEvent = intelCmdSetEvent,
    .CmdResetEvent = intelCmdResetEvent,
    .CmdMemoryAtomic = intelCmdMemoryAtomic,
    .CmdBeginQuery = intelCmdBeginQuery,
    .CmdEndQuery = intelCmdEndQuery,
    .CmdResetQueryPool = intelCmdResetQueryPool,
    .CmdWriteTimestamp = intelCmdWriteTimestamp,
    .CmdInitAtomicCounters = intelCmdInitAtomicCounters,
    .CmdLoadAtomicCounters = intelCmdLoadAtomicCounters,
    .CmdSaveAtomicCounters = intelCmdSaveAtomicCounters,
    .DbgSetValidationLevel = intelDbgSetValidationLevel,
    .DbgRegisterMsgCallback = xglDbgRegisterMsgCallback,
    .DbgUnregisterMsgCallback = xglDbgUnregisterMsgCallback,
    .DbgSetMessageFilter = intelDbgSetMessageFilter,
    .DbgSetObjectTag = intelDbgSetObjectTag,
    .DbgSetGlobalOption = xglDbgSetGlobalOption,
    .DbgSetDeviceOption = intelDbgSetDeviceOption,
    .CmdDbgMarkerBegin = intelCmdDbgMarkerBegin,
    .CmdDbgMarkerEnd = intelCmdDbgMarkerEnd,
};

const struct icd_dispatch_table intel_debug_dispatch_table = {
    .InitAndEnumerateGpus = xglInitAndEnumerateGpus,
    .GetGpuInfo = intelGetGpuInfo,
    .CreateDevice = intelCreateDevice,
    .DestroyDevice = intelDestroyDevice,
    .GetExtensionSupport = intelGetExtensionSupport,
    .GetDeviceQueue = intelGetDeviceQueue,
    .QueueSubmit = intelQueueSubmit,
    .QueueSetGlobalMemReferences = intelQueueSetGlobalMemReferences,
    .QueueWaitIdle = intelQueueWaitIdle,
    .DeviceWaitIdle = intelDeviceWaitIdle,
    .GetMemoryHeapCount = intelGetMemoryHeapCount,
    .GetMemoryHeapInfo = intelGetMemoryHeapInfo,
    .AllocMemory = intelAllocMemory,
    .FreeMemory = intelFreeMemory,
    .SetMemoryPriority = intelSetMemoryPriority,
    .MapMemory = intelMapMemory,
    .UnmapMemory = intelUnmapMemory,
    .PinSystemMemory = intelPinSystemMemory,
    .RemapVirtualMemoryPages = intelRemapVirtualMemoryPages,
    .GetMultiGpuCompatibility = intelGetMultiGpuCompatibility,
    .OpenSharedMemory = intelOpenSharedMemory,
    .OpenSharedQueueSemaphore = intelOpenSharedQueueSemaphore,
    .OpenPeerMemory = intelOpenPeerMemory,
    .OpenPeerImage = intelOpenPeerImage,
    .DestroyObject = intelDestroyObject,
    .GetObjectInfo = intelGetObjectInfo,
    .BindObjectMemory = intelBindObjectMemory,
    .CreateFence = intelCreateFence,
    .GetFenceStatus = intelGetFenceStatus,
    .WaitForFences = intelWaitForFences,
    .CreateQueueSemaphore = intelCreateQueueSemaphore,
    .SignalQueueSemaphore = intelSignalQueueSemaphore,
    .WaitQueueSemaphore = intelWaitQueueSemaphore,
    .CreateEvent = intelCreateEvent,
    .GetEventStatus = intelGetEventStatus,
    .SetEvent = intelSetEvent,
    .ResetEvent = intelResetEvent,
    .CreateQueryPool = intelCreateQueryPool,
    .GetQueryPoolResults = intelGetQueryPoolResults,
    .GetFormatInfo = intelGetFormatInfo,
    .CreateImage = intelCreateImage,
    .GetImageSubresourceInfo = intelGetImageSubresourceInfo,
    .CreateImageView = intelCreateImageView,
    .CreateColorAttachmentView = intelCreateColorAttachmentView,
    .CreateDepthStencilView = intelCreateDepthStencilView,
    .CreateShader = intelCreateShader,
    .CreateGraphicsPipeline = intelCreateGraphicsPipeline,
    .CreateComputePipeline = intelCreateComputePipeline,
    .StorePipeline = intelStorePipeline,
    .LoadPipeline = intelLoadPipeline,
    .CreatePipelineDelta = intelCreatePipelineDelta,
    .CreateSampler = intelCreateSampler,
    .CreateDescriptorSet = intelCreateDescriptorSet,
    .BeginDescriptorSetUpdate = intelBeginDescriptorSetUpdate,
    .EndDescriptorSetUpdate = intelEndDescriptorSetUpdate,
    .AttachSamplerDescriptors = intelAttachSamplerDescriptors,
    .AttachImageViewDescriptors = intelAttachImageViewDescriptors,
    .AttachMemoryViewDescriptors = intelAttachMemoryViewDescriptors,
    .AttachNestedDescriptors = intelAttachNestedDescriptors,
    .ClearDescriptorSetSlots = intelClearDescriptorSetSlots,
    .CreateViewportState = intelCreateViewportState,
    .CreateRasterState = intelCreateRasterState,
    .CreateMsaaState = intelCreateMsaaState,
    .CreateColorBlendState = intelCreateColorBlendState,
    .CreateDepthStencilState = intelCreateDepthStencilState,
    .CreateCommandBuffer = intelCreateCommandBuffer,
    .BeginCommandBuffer = intelBeginCommandBuffer,
    .EndCommandBuffer = intelEndCommandBuffer,
    .ResetCommandBuffer = intelResetCommandBuffer,
    .CmdBindPipeline = intelCmdBindPipeline,
    .CmdBindPipelineDelta = intelCmdBindPipelineDelta,
    .CmdBindStateObject = intelCmdBindStateObject,
    .CmdBindDescriptorSet = intelCmdBindDescriptorSet,
    .CmdBindDynamicMemoryView = intelCmdBindDynamicMemoryView,
    .CmdBindIndexData = intelCmdBindIndexData,
    .CmdBindAttachments = intelCmdBindAttachments,
    .CmdPrepareMemoryRegions = intelCmdPrepareMemoryRegions,
    .CmdPrepareImages = intelCmdPrepareImages,
    .CmdDraw = intelCmdDraw,
    .CmdDrawIndexed = intelCmdDrawIndexed,
    .CmdDrawIndirect = intelCmdDrawIndirect,
    .CmdDrawIndexedIndirect = intelCmdDrawIndexedIndirect,
    .CmdDispatch = intelCmdDispatch,
    .CmdDispatchIndirect = intelCmdDispatchIndirect,
    .CmdCopyMemory = intelCmdCopyMemory,
    .CmdCopyImage = intelCmdCopyImage,
    .CmdCopyMemoryToImage = intelCmdCopyMemoryToImage,
    .CmdCopyImageToMemory = intelCmdCopyImageToMemory,
    .CmdCloneImageData = intelCmdCloneImageData,
    .CmdUpdateMemory = intelCmdUpdateMemory,
    .CmdFillMemory = intelCmdFillMemory,
    .CmdClearColorImage = intelCmdClearColorImage,
    .CmdClearColorImageRaw = intelCmdClearColorImageRaw,
    .CmdClearDepthStencil = intelCmdClearDepthStencil,
    .CmdResolveImage = intelCmdResolveImage,
    .CmdSetEvent = intelCmdSetEvent,
    .CmdResetEvent = intelCmdResetEvent,
    .CmdMemoryAtomic = intelCmdMemoryAtomic,
    .CmdBeginQuery = intelCmdBeginQuery,
    .CmdEndQuery = intelCmdEndQuery,
    .CmdResetQueryPool = intelCmdResetQueryPool,
    .CmdWriteTimestamp = intelCmdWriteTimestamp,
    .CmdInitAtomicCounters = intelCmdInitAtomicCounters,
    .CmdLoadAtomicCounters = intelCmdLoadAtomicCounters,
    .CmdSaveAtomicCounters = intelCmdSaveAtomicCounters,
    .DbgSetValidationLevel = intelDbgSetValidationLevel,
    .DbgRegisterMsgCallback = xglDbgRegisterMsgCallback,
    .DbgUnregisterMsgCallback = xglDbgUnregisterMsgCallback,
    .DbgSetMessageFilter = intelDbgSetMessageFilter,
    .DbgSetObjectTag = intelDbgSetObjectTag,
    .DbgSetGlobalOption = xglDbgSetGlobalOption,
    .DbgSetDeviceOption = intelDbgSetDeviceOption,
    .CmdDbgMarkerBegin = intelCmdDbgMarkerBegin,
    .CmdDbgMarkerEnd = intelCmdDbgMarkerEnd,
};
