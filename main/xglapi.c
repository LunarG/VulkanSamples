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

#include <xgl.h>
#include <xglDbg.h>

XGL_RESULT XGLAPI xglInitAndEnumerateGpus(
    const XGL_APPLICATION_INFO*                 pAppInfo,
    const XGL_ALLOC_CALLBACKS*                  pAllocCb,
    XGL_UINT                                    maxGpus,
    XGL_UINT*                                   pGpuCount,
    XGL_PHYSICAL_GPU*                           pGpus)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetGpuInfo(
    XGL_PHYSICAL_GPU                            gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE                  infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateDevice(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_DEVICE_CREATE_INFO*               pCreateInfo,
    XGL_DEVICE*                                 pDevice)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglDestroyDevice(
    XGL_DEVICE                                  device)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetExtensionSupport(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_CHAR*                             pExtName)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetDeviceQueue(
    XGL_DEVICE                                  device,
    XGL_QUEUE_TYPE                              queueType,
    XGL_UINT                                    queueIndex,
    XGL_QUEUE*                                  pQueue)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglQueueSubmit(
    XGL_QUEUE                                   queue,
    XGL_UINT                                    cmdBufferCount,
    const XGL_CMD_BUFFER*                       pCmdBuffers,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs,
    XGL_FENCE                                   fence)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(
    XGL_QUEUE                                   queue,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglQueueWaitIdle(
    XGL_QUEUE                                   queue)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglDeviceWaitIdle(
    XGL_DEVICE                                  device)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetMemoryHeapCount(
    XGL_DEVICE                                  device,
    XGL_UINT*                                   pCount)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetMemoryHeapInfo(
    XGL_DEVICE                                  device,
    XGL_UINT                                    heapId,
    XGL_MEMORY_HEAP_INFO_TYPE                   infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglAllocMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglFreeMemory(
    XGL_GPU_MEMORY                              mem)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglSetMemoryPriority(
    XGL_GPU_MEMORY                              mem,
    XGL_MEMORY_PRIORITY                         priority)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglMapMemory(
    XGL_GPU_MEMORY                              mem,
    XGL_FLAGS                                   flags,
    XGL_VOID**                                  ppData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglUnmapMemory(
    XGL_GPU_MEMORY                              mem)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglPinSystemMemory(
    XGL_DEVICE                                  device,
    const XGL_VOID*                             pSysMem,
    XGL_SIZE                                    memSize,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglRemapVirtualMemoryPages(
    XGL_DEVICE                                  device,
    XGL_UINT                                    rangeCount,
    const XGL_VIRTUAL_MEMORY_REMAP_RANGE*       pRanges,
    XGL_UINT                                    preWaitSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*                  pPreWaitSemaphores,
    XGL_UINT                                    postSignalSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*                  pPostSignalSemaphores)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(
    XGL_PHYSICAL_GPU                            gpu0,
    XGL_PHYSICAL_GPU                            gpu1,
    XGL_GPU_COMPATIBILITY_INFO*                 pInfo)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglOpenSharedMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_OPEN_INFO*                 pOpenInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglOpenSharedQueueSemaphore(
    XGL_DEVICE                                  device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO*        pOpenInfo,
    XGL_QUEUE_SEMAPHORE*                        pSemaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglOpenPeerMemory(
    XGL_DEVICE                                  device,
    const XGL_PEER_MEMORY_OPEN_INFO*            pOpenInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglOpenPeerImage(
    XGL_DEVICE                                  device,
    const XGL_PEER_IMAGE_OPEN_INFO*             pOpenInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglDestroyObject(
    XGL_OBJECT                                  object)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetObjectInfo(
    XGL_BASE_OBJECT                             object,
    XGL_OBJECT_INFO_TYPE                        infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglBindObjectMemory(
    XGL_OBJECT                                  object,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateFence(
    XGL_DEVICE                                  device,
    const XGL_FENCE_CREATE_INFO*                pCreateInfo,
    XGL_FENCE*                                  pFence)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetFenceStatus(
    XGL_FENCE                                   fence)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglWaitForFences(
    XGL_DEVICE                                  device,
    XGL_UINT                                    fenceCount,
    const XGL_FENCE*                            pFences,
    XGL_BOOL                                    waitAll,
    XGL_UINT64                                  timeout)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateQueueSemaphore(
    XGL_DEVICE                                  device,
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO*      pCreateInfo,
    XGL_QUEUE_SEMAPHORE*                        pSemaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglSignalQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglWaitQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateEvent(
    XGL_DEVICE                                  device,
    const XGL_EVENT_CREATE_INFO*                pCreateInfo,
    XGL_EVENT*                                  pEvent)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetEventStatus(
    XGL_EVENT                                   event)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglSetEvent(
    XGL_EVENT                                   event)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglResetEvent(
    XGL_EVENT                                   event)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateQueryPool(
    XGL_DEVICE                                  device,
    const XGL_QUERY_POOL_CREATE_INFO*           pCreateInfo,
    XGL_QUERY_POOL*                             pQueryPool)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetQueryPoolResults(
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    startQuery,
    XGL_UINT                                    queryCount,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetFormatInfo(
    XGL_DEVICE                                  device,
    XGL_FORMAT                                  format,
    XGL_FORMAT_INFO_TYPE                        infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateImage(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
    XGL_IMAGE*                                  pImage)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglGetImageSubresourceInfo(
    XGL_IMAGE                                   image,
    const XGL_IMAGE_SUBRESOURCE*                pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE                   infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateImageView(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_VIEW_CREATE_INFO*           pCreateInfo,
    XGL_IMAGE_VIEW*                             pView)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateColorAttachmentView(
    XGL_DEVICE                                  device,
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW*                  pView)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateDepthStencilView(
    XGL_DEVICE                                  device,
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*   pCreateInfo,
    XGL_DEPTH_STENCIL_VIEW*                     pView)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateShader(
    XGL_DEVICE                                  device,
    const XGL_SHADER_CREATE_INFO*               pCreateInfo,
    XGL_SHADER*                                 pShader)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateGraphicsPipeline(
    XGL_DEVICE                                  device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    XGL_PIPELINE*                               pPipeline)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateComputePipeline(
    XGL_DEVICE                                  device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO*     pCreateInfo,
    XGL_PIPELINE*                               pPipeline)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglStorePipeline(
    XGL_PIPELINE                                pipeline,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglLoadPipeline(
    XGL_DEVICE                                  device,
    XGL_SIZE                                    dataSize,
    const XGL_VOID*                             pData,
    XGL_PIPELINE*                               pPipeline)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreatePipelineDelta(
    XGL_DEVICE                                  device,
    XGL_PIPELINE                                p1,
    XGL_PIPELINE                                p2,
    XGL_PIPELINE_DELTA*                         delta)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateSampler(
    XGL_DEVICE                                  device,
    const XGL_SAMPLER_CREATE_INFO*              pCreateInfo,
    XGL_SAMPLER*                                pSampler)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateDescriptorSet(
    XGL_DEVICE                                  device,
    const XGL_DESCRIPTOR_SET_CREATE_INFO*       pCreateInfo,
    XGL_DESCRIPTOR_SET*                         pDescriptorSet)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_VOID XGLAPI xglBeginDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET                          descriptorSet)
{
}

XGL_VOID XGLAPI xglEndDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET                          descriptorSet)
{
}

XGL_VOID XGLAPI xglAttachSamplerDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_SAMPLER*                          pSamplers)
{
}

XGL_VOID XGLAPI xglAttachImageViewDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_IMAGE_VIEW_ATTACH_INFO*           pImageViews)
{
}

XGL_VOID XGLAPI xglAttachMemoryViewDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemViews)
{
}

XGL_VOID XGLAPI xglAttachNestedDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_DESCRIPTOR_SET_ATTACH_INFO*       pNestedDescriptorSets)
{
}

XGL_VOID XGLAPI xglClearDescriptorSetSlots(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount)
{
}

XGL_RESULT XGLAPI xglCreateViewportState(
    XGL_DEVICE                                  device,
    const XGL_VIEWPORT_STATE_CREATE_INFO*       pCreateInfo,
    XGL_VIEWPORT_STATE_OBJECT*                  pState)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateRasterState(
    XGL_DEVICE                                  device,
    const XGL_RASTER_STATE_CREATE_INFO*         pCreateInfo,
    XGL_RASTER_STATE_OBJECT*                    pState)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateMsaaState(
    XGL_DEVICE                                  device,
    const XGL_MSAA_STATE_CREATE_INFO*           pCreateInfo,
    XGL_MSAA_STATE_OBJECT*                      pState)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateColorBlendState(
    XGL_DEVICE                                  device,
    const XGL_COLOR_BLEND_STATE_CREATE_INFO*    pCreateInfo,
    XGL_COLOR_BLEND_STATE_OBJECT*               pState)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateDepthStencilState(
    XGL_DEVICE                                  device,
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO*  pCreateInfo,
    XGL_DEPTH_STENCIL_STATE_OBJECT*             pState)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglCreateCommandBuffer(
    XGL_DEVICE                                  device,
    const XGL_CMD_BUFFER_CREATE_INFO*           pCreateInfo,
    XGL_CMD_BUFFER*                             pCmdBuffer)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglBeginCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_FLAGS                                   flags)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglEndCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglResetCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_VOID XGLAPI xglCmdBindPipeline(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE                                pipeline)
{
}

XGL_VOID XGLAPI xglCmdBindPipelineDelta(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE_DELTA                          delta)
{
}

XGL_VOID XGLAPI xglCmdBindStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_STATE_OBJECT                            state)
{
}

XGL_VOID XGLAPI xglCmdBindDescriptorSet(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    index,
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    slotOffset)
{
}

XGL_VOID XGLAPI xglCmdBindDynamicMemoryView(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView)
{
}

XGL_VOID XGLAPI xglCmdBindIndexData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType)
{
}

XGL_VOID XGLAPI xglCmdBindAttachments(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    colorAttachmentCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*       pColorAttachments,
    const XGL_DEPTH_STENCIL_BIND_INFO*          pDepthStencilAttachment)
{
}

XGL_VOID XGLAPI xglCmdPrepareMemoryRegions(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_MEMORY_STATE_TRANSITION*          pStateTransitions)
{
}

XGL_VOID XGLAPI xglCmdPrepareImages(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_IMAGE_STATE_TRANSITION*           pStateTransitions)
{
}

XGL_VOID XGLAPI xglCmdDraw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstVertex,
    XGL_UINT                                    vertexCount,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount)
{
}

XGL_VOID XGLAPI xglCmdDrawIndexed(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstIndex,
    XGL_UINT                                    indexCount,
    XGL_INT                                     vertexOffset,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount)
{
}

XGL_VOID XGLAPI xglCmdDrawIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride)
{
}

XGL_VOID XGLAPI xglCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride)
{
}

XGL_VOID XGLAPI xglCmdDispatch(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    x,
    XGL_UINT                                    y,
    XGL_UINT                                    z)
{
}

XGL_VOID XGLAPI xglCmdDispatchIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset)
{
}

XGL_VOID XGLAPI xglCmdCopyMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_COPY*                      pRegions)
{
}

XGL_VOID XGLAPI xglCmdCopyImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_IMAGE_COPY*                       pRegions)
{
}

XGL_VOID XGLAPI xglCmdCopyMemoryToImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions)
{
}

XGL_VOID XGLAPI xglCmdCopyImageToMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions)
{
}

XGL_VOID XGLAPI xglCmdCloneImageData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_STATE                             srcImageState,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_STATE                             destImageState)
{
}

XGL_VOID XGLAPI xglCmdUpdateMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                dataSize,
    const XGL_UINT32*                           pData)
{
}

XGL_VOID XGLAPI xglCmdFillMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                fillSize,
    XGL_UINT32                                  data)
{
}

XGL_VOID XGLAPI xglCmdClearColorImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_FLOAT                             color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
}

XGL_VOID XGLAPI xglCmdClearColorImageRaw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_UINT32                            color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
}

XGL_VOID XGLAPI xglCmdClearDepthStencil(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    XGL_FLOAT                                   depth,
    XGL_UINT32                                  stencil,
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
}

XGL_VOID XGLAPI xglCmdResolveImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    rectCount,
    const XGL_IMAGE_RESOLVE*                    pRects)
{
}

XGL_VOID XGLAPI xglCmdSetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event)
{
}

XGL_VOID XGLAPI xglCmdResetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event)
{
}

XGL_VOID XGLAPI xglCmdMemoryAtomic(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_UINT64                                  srcData,
    XGL_ATOMIC_OP                               atomicOp)
{
}

XGL_VOID XGLAPI xglCmdBeginQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot,
    XGL_FLAGS                                   flags)
{
}

XGL_VOID XGLAPI xglCmdEndQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot)
{
}

XGL_VOID XGLAPI xglCmdResetQueryPool(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    startQuery,
    XGL_UINT                                    queryCount)
{
}

XGL_VOID XGLAPI xglCmdWriteTimestamp(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_TIMESTAMP_TYPE                          timestampType,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset)
{
}

XGL_VOID XGLAPI xglCmdInitAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    const XGL_UINT32*                           pData)
{
}

XGL_VOID XGLAPI xglCmdLoadAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_SIZE                                srcOffset)
{
}

XGL_VOID XGLAPI xglCmdSaveAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset)
{
}

XGL_RESULT XGLAPI xglDbgSetValidationLevel(
    XGL_DEVICE                                  device,
    XGL_VALIDATION_LEVEL                        validationLevel)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(
    XGL_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback,
    XGL_VOID*                                   pUserData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(
    XGL_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglDbgSetMessageFilter(
    XGL_DEVICE                                  device,
    XGL_INT                                     msgCode,
    XGL_DBG_MSG_FILTER                          filter)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglDbgSetObjectTag(
    XGL_BASE_OBJECT                             object,
    XGL_SIZE                                    tagSize,
    const XGL_VOID*                             pTag)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglDbgSetGlobalOption(
    XGL_DBG_GLOBAL_OPTION                       dbgOption,
    XGL_SIZE                                    dataSize,
    const XGL_VOID*                             pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI xglDbgSetDeviceOption(
    XGL_DEVICE                                  device,
    XGL_DBG_DEVICE_OPTION                       dbgOption,
    XGL_SIZE                                    dataSize,
    const XGL_VOID*                             pData)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_VOID XGLAPI xglCmdDbgMarkerBegin(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_CHAR*                             pMarker)
{
}

XGL_VOID XGLAPI xglCmdDbgMarkerEnd(
    XGL_CMD_BUFFER                              cmdBuffer)
{
}
