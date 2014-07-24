/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
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
 *
 * Authors:
 *    Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#ifndef XGLAPI_INCLUDED
#define XGLAPI_INCLUDED
#include "xgl.h"

/**
 * A generic function ptr type
 */
typedef void (*_XGLProc)(void);

/**
 * Typedefs for all XGL API entrypoint functions.
 */

#ifdef __cplusplus
extern "C"
{
#endif

// GPU initialization

typedef XGL_RESULT XGLAPI (*InitAndEnumerateGpus_t)(
      const XGL_APPLICATION_INFO*                 pAppInfo,
      const XGL_ALLOC_CALLBACKS*                  pAllocCb,
      XGL_UINT                                    maxGpus,
      XGL_UINT*                                   pGpuCount,
      XGL_PHYSICAL_GPU*                           pGpus);

typedef XGL_RESULT XGLAPI (*GetGpuInfo_t)(
      XGL_PHYSICAL_GPU                            gpu,
      XGL_PHYSICAL_GPU_INFO_TYPE                  infoType,
      XGL_SIZE*                                   pDataSize,
      XGL_VOID*                                   pData);

// Device functions

typedef XGL_RESULT XGLAPI (*CreateDevice_t)(
      XGL_PHYSICAL_GPU                            gpu,
      const XGL_DEVICE_CREATE_INFO*               pCreateInfo,
      XGL_DEVICE*                                 pDevice);

typedef XGL_RESULT XGLAPI (*DestroyDevice_t)(
      XGL_DEVICE                                  device);

// Extension discovery functions

typedef XGL_RESULT XGLAPI (*GetExtensionSupport_t)(
      XGL_PHYSICAL_GPU                            gpu,
      const XGL_CHAR*                             pExtName);

// Queue functions

typedef XGL_RESULT XGLAPI (*GetDeviceQueue_t)(
      XGL_DEVICE                                  device,
      XGL_QUEUE_TYPE                              queueType,
      XGL_UINT                                    queueIndex,
      XGL_QUEUE*                                  pQueue);

typedef XGL_RESULT XGLAPI (*QueueSubmit_t)(
      XGL_QUEUE                                   queue,
      XGL_UINT                                    cmdBufferCount,
      const XGL_CMD_BUFFER*                       pCmdBuffers,
      XGL_UINT                                    memRefCount,
      const XGL_MEMORY_REF*                       pMemRefs,
      XGL_FENCE                                   fence);

typedef XGL_RESULT XGLAPI (*QueueSetGlobalMemReferences_t)(
      XGL_QUEUE                                   queue,
      XGL_UINT                                    memRefCount,
      const XGL_MEMORY_REF*                       pMemRefs);

typedef XGL_RESULT XGLAPI (*QueueWaitIdle_t)(
      XGL_QUEUE                                   queue);

typedef XGL_RESULT XGLAPI (*DeviceWaitIdle_t)(
      XGL_DEVICE                                  device);

// Memory functions

typedef XGL_RESULT XGLAPI (*GetMemoryHeapCount_t)(
      XGL_DEVICE                                  device,
      XGL_UINT*                                   pCount);

typedef XGL_RESULT XGLAPI (*GetMemoryHeapInfo_t)(
      XGL_DEVICE                                  device,
      XGL_UINT                                    heapId,
      XGL_MEMORY_HEAP_INFO_TYPE                   infoType,
      XGL_SIZE*                                   pDataSize,
      XGL_VOID*                                   pData);

typedef XGL_RESULT XGLAPI (*AllocMemory_t)(
      XGL_DEVICE                                  device,
      const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
      XGL_GPU_MEMORY*                             pMem);

typedef XGL_RESULT XGLAPI (*FreeMemory_t)(
      XGL_GPU_MEMORY                              mem);

typedef XGL_RESULT XGLAPI (*SetMemoryPriority_t)(
      XGL_GPU_MEMORY                              mem,
      XGL_MEMORY_PRIORITY                         priority);

typedef XGL_RESULT XGLAPI (*MapMemory_t)(
      XGL_GPU_MEMORY                              mem,
      XGL_FLAGS                                   flags,                // Reserved
      XGL_VOID**                                  ppData);

typedef XGL_RESULT XGLAPI (*UnmapMemory_t)(
      XGL_GPU_MEMORY                              mem);

typedef XGL_RESULT XGLAPI (*PinSystemMemory_t)(
      XGL_DEVICE                                  device,
      const XGL_VOID*                             pSysMem,
      XGL_SIZE                                    memSize,
      XGL_GPU_MEMORY*                             pMem);

typedef XGL_RESULT XGLAPI (*RemapVirtualMemoryPages_t)(
      XGL_DEVICE                                  device,
      XGL_UINT                                    rangeCount,
      const XGL_VIRTUAL_MEMORY_REMAP_RANGE*       pRanges,
      XGL_UINT                                    preWaitSemaphoreCount,
      const XGL_QUEUE_SEMAPHORE*                  pPreWaitSemaphores,
      XGL_UINT                                    postSignalSemaphoreCount,
      const XGL_QUEUE_SEMAPHORE*                  pPostSignalSemaphores);

// Multi-device functions

typedef XGL_RESULT XGLAPI (*GetMultiGpuCompatibility_t)(
      XGL_PHYSICAL_GPU                            gpu0,
      XGL_PHYSICAL_GPU                            gpu1,
      XGL_GPU_COMPATIBILITY_INFO*                 pInfo);

typedef XGL_RESULT XGLAPI (*OpenSharedMemory_t)(
      XGL_DEVICE                                  device,
      const XGL_MEMORY_OPEN_INFO*                 pOpenInfo,
      XGL_GPU_MEMORY*                             pMem);

typedef XGL_RESULT XGLAPI (*OpenSharedQueueSemaphore_t)(
      XGL_DEVICE                                  device,
      const XGL_QUEUE_SEMAPHORE_OPEN_INFO*        pOpenInfo,
      XGL_QUEUE_SEMAPHORE*                        pSemaphore);

typedef XGL_RESULT XGLAPI (*OpenPeerMemory_t)(
      XGL_DEVICE                                  device,
      const XGL_PEER_MEMORY_OPEN_INFO*            pOpenInfo,
      XGL_GPU_MEMORY*                             pMem);

typedef XGL_RESULT XGLAPI (*OpenPeerImage_t)(
      XGL_DEVICE                                  device,
      const XGL_PEER_IMAGE_OPEN_INFO*             pOpenInfo,
      XGL_IMAGE*                                  pImage,
      XGL_GPU_MEMORY*                             pMem);

// Generic API object functions

typedef XGL_RESULT XGLAPI (*DestroyObject_t)(
      XGL_OBJECT                                  object);

typedef XGL_RESULT XGLAPI (*GetObjectInfo_t)(
      XGL_BASE_OBJECT                             object,
      XGL_OBJECT_INFO_TYPE                        infoType,
      XGL_SIZE*                                   pDataSize,
      XGL_VOID*                                   pData);

typedef XGL_RESULT XGLAPI (*BindObjectMemory_t)(
      XGL_OBJECT                                  object,
      XGL_GPU_MEMORY                              mem,
      XGL_GPU_SIZE                                offset);

// Fence functions

typedef XGL_RESULT XGLAPI (*CreateFence_t)(
      XGL_DEVICE                                  device,
      const XGL_FENCE_CREATE_INFO*                pCreateInfo,
      XGL_FENCE*                                  pFence);

typedef XGL_RESULT XGLAPI (*GetFenceStatus_t)(
      XGL_FENCE fence);

typedef XGL_RESULT XGLAPI (*WaitForFences_t)(
      XGL_DEVICE                                  device,
      XGL_UINT                                    fenceCount,
      const XGL_FENCE*                            pFences,
      XGL_BOOL                                    waitAll,
      XGL_UINT64                                  timeout);

// Queue semaphore functions

typedef XGL_RESULT XGLAPI (*CreateQueueSemaphore_t)(
      XGL_DEVICE                                  device,
      const XGL_QUEUE_SEMAPHORE_CREATE_INFO*      pCreateInfo,
      XGL_QUEUE_SEMAPHORE*                        pSemaphore);

typedef XGL_RESULT XGLAPI (*SignalQueueSemaphore_t)(
      XGL_QUEUE                                   queue,
      XGL_QUEUE_SEMAPHORE                         semaphore);

typedef XGL_RESULT XGLAPI (*WaitQueueSemaphore_t)(
      XGL_QUEUE                                   queue,
      XGL_QUEUE_SEMAPHORE                         semaphore);

// Event functions

typedef XGL_RESULT XGLAPI (*CreateEvent_t)(
      XGL_DEVICE                                  device,
      const XGL_EVENT_CREATE_INFO*                pCreateInfo,
      XGL_EVENT*                                  pEvent);

typedef XGL_RESULT XGLAPI (*GetEventStatus_t)(
      XGL_EVENT                                   event);

typedef XGL_RESULT XGLAPI (*SetEvent_t)(
      XGL_EVENT                                   event);

typedef XGL_RESULT XGLAPI (*ResetEvent_t)(
      XGL_EVENT                                   event);

// Query functions

typedef XGL_RESULT XGLAPI (*CreateQueryPool_t)(
      XGL_DEVICE                                  device,
      const XGL_QUERY_POOL_CREATE_INFO*           pCreateInfo,
      XGL_QUERY_POOL*                             pQueryPool);

typedef XGL_RESULT XGLAPI (*GetQueryPoolResults_t)(
      XGL_QUERY_POOL                              queryPool,
      XGL_UINT                                    startQuery,
      XGL_UINT                                    queryCount,
      XGL_SIZE*                                   pDataSize,
      XGL_VOID*                                   pData);

// Format capabilities

typedef XGL_RESULT XGLAPI (*GetFormatInfo_t)(
      XGL_DEVICE                                  device,
      XGL_FORMAT                                  format,
      XGL_FORMAT_INFO_TYPE                        infoType,
      XGL_SIZE*                                   pDataSize,
      XGL_VOID*                                   pData);

// Image functions

typedef XGL_RESULT XGLAPI (*CreateImage_t)(
      XGL_DEVICE                                  device,
      const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
      XGL_IMAGE*                                  pImage);

typedef XGL_RESULT XGLAPI (*GetImageSubresourceInfo_t)(
      XGL_IMAGE                                   image,
      const XGL_IMAGE_SUBRESOURCE*                pSubresource,
      XGL_SUBRESOURCE_INFO_TYPE                   infoType,
      XGL_SIZE*                                   pDataSize,
      XGL_VOID*                                   pData);

// Image view functions

typedef XGL_RESULT XGLAPI (*CreateImageView_t)(
      XGL_DEVICE                                  device,
      const XGL_IMAGE_VIEW_CREATE_INFO*           pCreateInfo,
      XGL_IMAGE_VIEW*                             pView);

typedef XGL_RESULT XGLAPI (*CreateColorAttachmentView_t)(
      XGL_DEVICE                                  device,
      const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
      XGL_COLOR_ATTACHMENT_VIEW*                  pView);

typedef XGL_RESULT XGLAPI (*CreateDepthStencilView_t)(
      XGL_DEVICE                                  device,
      const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*   pCreateInfo,
      XGL_DEPTH_STENCIL_VIEW*                     pView);

// Shader functions

typedef XGL_RESULT XGLAPI (*CreateShader_t)(
      XGL_DEVICE                                  device,
      const XGL_SHADER_CREATE_INFO*               pCreateInfo,
      XGL_SHADER*                                 pShader);

// Pipeline functions

typedef XGL_RESULT XGLAPI (*CreateGraphicsPipeline_t)(
      XGL_DEVICE                                  device,
      const XGL_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
      XGL_PIPELINE*                               pPipeline);

typedef XGL_RESULT XGLAPI (*CreateComputePipeline_t)(
      XGL_DEVICE                                  device,
      const XGL_COMPUTE_PIPELINE_CREATE_INFO*     pCreateInfo,
      XGL_PIPELINE*                               pPipeline);

typedef XGL_RESULT XGLAPI (*StorePipeline_t)(
      XGL_PIPELINE                                pipeline,
      XGL_SIZE*                                   pDataSize,
      XGL_VOID*                                   pData);

typedef XGL_RESULT XGLAPI (*LoadPipeline_t)(
      XGL_DEVICE                                  device,
      XGL_SIZE                                    dataSize,
      const XGL_VOID*                             pData,
      XGL_PIPELINE*                               pPipeline);

typedef XGL_RESULT XGLAPI (*CreatePipelineDelta_t)(
      XGL_DEVICE                                  device,
      XGL_PIPELINE                                p1,
      XGL_PIPELINE                                p2,
      XGL_PIPELINE_DELTA*                         delta);

// Sampler functions

typedef XGL_RESULT XGLAPI (*CreateSampler_t)(
      XGL_DEVICE                                  device,
      const XGL_SAMPLER_CREATE_INFO*              pCreateInfo,
      XGL_SAMPLER*                                pSampler);

// Descriptor set functions

typedef XGL_RESULT XGLAPI (*CreateDescriptorSet_t)(
      XGL_DEVICE                                  device,
      const XGL_DESCRIPTOR_SET_CREATE_INFO*       pCreateInfo,
      XGL_DESCRIPTOR_SET*                         pDescriptorSet);

typedef XGL_VOID XGLAPI (*BeginDescriptorSetUpdate_t)(
      XGL_DESCRIPTOR_SET                          descriptorSet);

typedef XGL_VOID XGLAPI (*EndDescriptorSetUpdate_t)(
      XGL_DESCRIPTOR_SET                          descriptorSet);

typedef XGL_VOID XGLAPI (*AttachSamplerDescriptors_t)(
      XGL_DESCRIPTOR_SET                          descriptorSet,
      XGL_UINT                                    startSlot,
      XGL_UINT                                    slotCount,
      const XGL_SAMPLER*                          pSamplers);

typedef XGL_VOID XGLAPI (*AttachImageViewDescriptors_t)(
      XGL_DESCRIPTOR_SET                          descriptorSet,
      XGL_UINT                                    startSlot,
      XGL_UINT                                    slotCount,
      const XGL_IMAGE_VIEW_ATTACH_INFO*           pImageViews);

typedef XGL_VOID XGLAPI (*AttachMemoryViewDescriptors_t)(
      XGL_DESCRIPTOR_SET                          descriptorSet,
      XGL_UINT                                    startSlot,
      XGL_UINT                                    slotCount,
      const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemViews);

typedef XGL_VOID XGLAPI (*AttachNestedDescriptors_t)(
      XGL_DESCRIPTOR_SET                          descriptorSet,
      XGL_UINT                                    startSlot,
      XGL_UINT                                    slotCount,
      const XGL_DESCRIPTOR_SET_ATTACH_INFO*       pNestedDescriptorSets);

typedef XGL_VOID XGLAPI (*ClearDescriptorSetSlots_t)(
      XGL_DESCRIPTOR_SET                          descriptorSet,
      XGL_UINT                                    startSlot,
      XGL_UINT                                    slotCount);

// State object functions

typedef XGL_RESULT XGLAPI (*CreateViewportState_t)(
      XGL_DEVICE                                  device,
      const XGL_VIEWPORT_STATE_CREATE_INFO*       pCreateInfo,
      XGL_VIEWPORT_STATE_OBJECT*                  pState);

typedef XGL_RESULT XGLAPI (*CreateRasterState_t)(
      XGL_DEVICE                                  device,
      const XGL_RASTER_STATE_CREATE_INFO*         pCreateInfo,
      XGL_RASTER_STATE_OBJECT*                    pState);

typedef XGL_RESULT XGLAPI (*CreateMsaaState_t)(
      XGL_DEVICE                                  device,
      const XGL_MSAA_STATE_CREATE_INFO*           pCreateInfo,
      XGL_MSAA_STATE_OBJECT*                      pState);

typedef XGL_RESULT XGLAPI (*CreateColorBlendState_t)(
      XGL_DEVICE                                  device,
      const XGL_COLOR_BLEND_STATE_CREATE_INFO*    pCreateInfo,
      XGL_COLOR_BLEND_STATE_OBJECT*               pState);

typedef XGL_RESULT XGLAPI (*CreateDepthStencilState_t)(
      XGL_DEVICE                                  device,
      const XGL_DEPTH_STENCIL_STATE_CREATE_INFO*  pCreateInfo,
      XGL_DEPTH_STENCIL_STATE_OBJECT*             pState);

// Command buffer functions

typedef XGL_RESULT XGLAPI (*CreateCommandBuffer_t)(
      XGL_DEVICE                                  device,
      const XGL_CMD_BUFFER_CREATE_INFO*           pCreateInfo,
      XGL_CMD_BUFFER*                             pCmdBuffer);

typedef XGL_RESULT XGLAPI (*BeginCommandBuffer_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_FLAGS                                   flags);               // XGL_CMD_BUFFER_BUILD_FLAGS

typedef XGL_RESULT XGLAPI (*EndCommandBuffer_t)(
      XGL_CMD_BUFFER                              cmdBuffer);

typedef XGL_RESULT XGLAPI (*ResetCommandBuffer_t)(
      XGL_CMD_BUFFER                              cmdBuffer);

// Command buffer building functions

typedef XGL_VOID XGLAPI (*CmdBindPipeline_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
      XGL_PIPELINE                                pipeline);

typedef XGL_VOID XGLAPI (*CmdBindPipelineDelta_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
      XGL_PIPELINE_DELTA                          delta);

typedef XGL_VOID XGLAPI (*CmdBindStateObject_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_STATE_BIND_POINT                        stateBindPoint,
      XGL_STATE_OBJECT                            state);

typedef XGL_VOID XGLAPI (*CmdBindDescriptorSet_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
      XGL_UINT                                    index,
      XGL_DESCRIPTOR_SET                          descriptorSet,
      XGL_UINT                                    slotOffset);

typedef XGL_VOID XGLAPI (*CmdBindDynamicMemoryView_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
      const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView);

typedef XGL_VOID XGLAPI (*CmdBindIndexData_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_GPU_MEMORY                              mem,
      XGL_GPU_SIZE                                offset,
      XGL_INDEX_TYPE                              indexType);

typedef XGL_VOID XGLAPI (*CmdBindAttachments_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_UINT                                    colorAttachmentCount,
      const XGL_COLOR_ATTACHMENT_BIND_INFO*       pColorAttachments,
      const XGL_DEPTH_STENCIL_BIND_INFO*          pDepthStencilAttachment);

typedef XGL_VOID XGLAPI (*CmdPrepareMemoryRegions_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_UINT                                    transitionCount,
      const XGL_MEMORY_STATE_TRANSITION*          pStateTransitions);

typedef XGL_VOID XGLAPI (*CmdPrepareImages_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_UINT                                    transitionCount,
      const XGL_IMAGE_STATE_TRANSITION*           pStateTransitions);

typedef XGL_VOID XGLAPI (*CmdDraw_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_UINT                                    firstVertex,
      XGL_UINT                                    vertexCount,
      XGL_UINT                                    firstInstance,
      XGL_UINT                                    instanceCount);

typedef XGL_VOID XGLAPI (*CmdDrawIndexed_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_UINT                                    firstIndex,
      XGL_UINT                                    indexCount,
      XGL_INT                                     vertexOffset,
      XGL_UINT                                    firstInstance,
      XGL_UINT                                    instanceCount);

typedef XGL_VOID XGLAPI (*CmdDrawIndirect_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_GPU_MEMORY                              mem,
      XGL_GPU_SIZE                                offset,
      XGL_UINT32                                  count,
      XGL_UINT32                                  stride);

typedef XGL_VOID XGLAPI (*CmdDrawIndexedIndirect_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_GPU_MEMORY                              mem,
      XGL_GPU_SIZE                                offset,
      XGL_UINT32                                  count,
      XGL_UINT32                                  stride);

typedef XGL_VOID XGLAPI (*CmdDispatch_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_UINT                                    x,
      XGL_UINT                                    y,
      XGL_UINT                                    z);

typedef XGL_VOID XGLAPI (*CmdDispatchIndirect_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_GPU_MEMORY                              mem,
      XGL_GPU_SIZE                                offset);

typedef XGL_VOID XGLAPI (*CmdCopyMemory_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_GPU_MEMORY                              srcMem,
      XGL_GPU_MEMORY                              destMem,
      XGL_UINT                                    regionCount,
      const XGL_MEMORY_COPY*                      pRegions);

typedef XGL_VOID XGLAPI (*CmdCopyImage_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_IMAGE                                   srcImage,
      XGL_IMAGE                                   destImage,
      XGL_UINT                                    regionCount,
      const XGL_IMAGE_COPY*                       pRegions);

typedef XGL_VOID XGLAPI (*CmdCopyMemoryToImage_t)(        
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_GPU_MEMORY                              srcMem,
      XGL_IMAGE                                   destImage,
      XGL_UINT                                    regionCount,
      const XGL_MEMORY_IMAGE_COPY*                pRegions);

typedef XGL_VOID XGLAPI (*CmdCopyImageToMemory_t)(        
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_IMAGE                                   srcImage,
      XGL_GPU_MEMORY                              destMem,
      XGL_UINT                                    regionCount,
      const XGL_MEMORY_IMAGE_COPY*                pRegions);

typedef XGL_VOID XGLAPI (*CmdCloneImageData_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_IMAGE                                   srcImage,
      XGL_IMAGE_STATE                             srcImageState,
      XGL_IMAGE                                   destImage,
      XGL_IMAGE_STATE                             destImageState);

typedef XGL_VOID XGLAPI (*CmdUpdateMemory_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_GPU_MEMORY                              destMem,
      XGL_GPU_SIZE                                destOffset,
      XGL_GPU_SIZE                                dataSize,
      const XGL_UINT32*                           pData);

typedef XGL_VOID XGLAPI (*CmdFillMemory_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_GPU_MEMORY                              destMem,
      XGL_GPU_SIZE                                destOffset,
      XGL_GPU_SIZE                                fillSize,
      XGL_UINT32                                  data);

typedef XGL_VOID XGLAPI (*CmdClearColorImage_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_IMAGE                                   image,
      const XGL_FLOAT                             color[4],
XGL_UINT                                    rangeCount,
const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

typedef XGL_VOID XGLAPI (*CmdClearColorImageRaw_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_IMAGE                                   image,
      const XGL_UINT32                            color[4],
XGL_UINT                                    rangeCount,
const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

typedef XGL_VOID XGLAPI (*CmdClearDepthStencil_t)(        
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_IMAGE                                   image,
      XGL_FLOAT                                   depth,
      XGL_UINT32                                  stencil,
      XGL_UINT                                    rangeCount,
      const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

typedef XGL_VOID XGLAPI (*CmdResolveImage_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_IMAGE                                   srcImage,
      XGL_IMAGE                                   destImage,
      XGL_UINT                                    rectCount,
      const XGL_IMAGE_RESOLVE*                    pRects);

typedef XGL_VOID XGLAPI (*CmdSetEvent_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_EVENT                                   event);

typedef XGL_VOID XGLAPI (*CmdResetEvent_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_EVENT                                   event);

typedef XGL_VOID XGLAPI (*CmdMemoryAtomic_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_GPU_MEMORY                              destMem,
      XGL_GPU_SIZE                                destOffset,
      XGL_UINT64                                  srcData,
      XGL_ATOMIC_OP                               atomicOp);

typedef XGL_VOID XGLAPI (*CmdBeginQuery_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_QUERY_POOL                              queryPool,
      XGL_UINT                                    slot,
      XGL_FLAGS                                   flags);

typedef XGL_VOID XGLAPI (*CmdEndQuery_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_QUERY_POOL                              queryPool,
      XGL_UINT                                    slot);

typedef XGL_VOID XGLAPI (*CmdResetQueryPool_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_QUERY_POOL                              queryPool,
      XGL_UINT                                    startQuery,
      XGL_UINT                                    queryCount);

typedef XGL_VOID XGLAPI (*CmdWriteTimestamp_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_TIMESTAMP_TYPE                          timestampType,
      XGL_GPU_MEMORY                              destMem,
      XGL_GPU_SIZE                                destOffset);

typedef XGL_VOID XGLAPI (*CmdInitAtomicCounters_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
      XGL_UINT                                    startCounter,
      XGL_UINT                                    counterCount,
      const XGL_UINT32*                           pData);

typedef XGL_VOID XGLAPI (*CmdLoadAtomicCounters_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
      XGL_UINT                                    startCounter,
      XGL_UINT                                    counterCount,
      XGL_GPU_MEMORY                              srcMem,
      XGL_GPU_SIZE                                srcOffset);

typedef XGL_VOID XGLAPI (*CmdSaveAtomicCounters_t)(
      XGL_CMD_BUFFER                              cmdBuffer,
      XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
      XGL_UINT                                    startCounter,
      XGL_UINT                                    counterCount,
      XGL_GPU_MEMORY                              destMem,
      XGL_GPU_SIZE                                destOffset);

/**
 * The API dispatcher jumps through these functions
 */
struct _xgl_api
{
   // GPU initialization

   InitAndEnumerateGpus_t InitAndEnumerateGpus;
   GetGpuInfo_t GetGpuInfo;

   // Device functions
   CreateDevice_t CreateDevice;
   DestroyDevice_t DestroyDevice;

   // Extension discovery functions

   GetExtensionSupport_t GetExtensionSupport;

   // Queue functions
   GetDeviceQueue_t GetDeviceQueue;
   QueueSubmit_t QueueSubmit;
   QueueSetGlobalMemReferences_t QueueSetGlobalMemReferences;
   QueueWaitIdle_t QueueWaitIdle;
   DeviceWaitIdle_t DeviceWaitIdle;

   // Memory functions

   GetMemoryHeapCount_t GetMemoryHeapCount;
   GetMemoryHeapInfo_t GetMemoryHeapInfo;
   AllocMemory_t AllocMemory;
   FreeMemory_t FreeMemory;
   SetMemoryPriority_t SetMemoryPriority;
   MapMemory_t MapMemory;
   UnmapMemory_t UnmapMemory;
   PinSystemMemory_t PinSystemMemory;
   RemapVirtualMemoryPages_t RemapVirtualMemoryPages;

   // Multi-device functions

   GetMultiGpuCompatibility_t GetMultiGpuCompatibility;
   OpenSharedMemory_t OpenSharedMemory;
   OpenSharedQueueSemaphore_t OpenSharedQueueSemaphore;
   OpenPeerMemory_t OpenPeerMemory;
   OpenPeerImage_t OpenPeerImage;

   // Generic API object functions

   DestroyObject_t DestroyObject;
   GetObjectInfo_t GetObjectInfo;
   BindObjectMemory_t BindObjectMemory;

   // Fence functions

   CreateFence_t CreateFence;
   GetFenceStatus_t GetFenceStatus;
   WaitForFences_t WaitForFences;

   // Queue semaphore functions

   CreateQueueSemaphore_t CreateQueueSemaphore;
   SignalQueueSemaphore_t SignalQueueSemaphore;
   WaitQueueSemaphore_t WaitQueueSemaphore;

   // Event functions

   CreateEvent_t CreateEvent;
   GetEventStatus_t GetEventStatus;
   SetEvent_t SetEvent;
   ResetEvent_t ResetEvent;

   // Query functions

   CreateQueryPool_t CreateQueryPool;
   GetQueryPoolResults_t GetQueryPoolResults;

   // Format capabilities

   GetFormatInfo_t GetFormatInfo;

   // Image functions

   CreateImage_t CreateImage;
   GetImageSubresourceInfo_t GetImageSubresourceInfo;

   // Image view functions

   CreateImageView_t CreateImageView;
   CreateColorAttachmentView_t CreateColorAttachmentView;
   CreateDepthStencilView_t CreateDepthStencilView;

   // Shader functions

   CreateShader_t CreateShader;

   // Pipeline functions

   CreateGraphicsPipeline_t CreateGraphicsPipeline;
   CreateComputePipeline_t CreateComputePipeline;
   StorePipeline_t StorePipeline;
   LoadPipeline_t LoadPipeline;
   CreatePipelineDelta_t CreatePipelineDelta;

   // Sampler functions

   CreateSampler_t CreateSampler;

   // Descriptor set functions

   CreateDescriptorSet_t CreateDescriptorSet;
   BeginDescriptorSetUpdate_t BeginDescriptorSetUpdate;
   EndDescriptorSetUpdate_t EndDescriptorSetUpdate;
   AttachSamplerDescriptors_t AttachSamplerDescriptors;
   AttachImageViewDescriptors_t AttachImageViewDescriptors;
   AttachMemoryViewDescriptors_t AttachMemoryViewDescriptors;
   AttachNestedDescriptors_t AttachNestedDescriptors;
   ClearDescriptorSetSlots_t ClearDescriptorSetSlots;

   // State object functions

   CreateViewportState_t CreateViewportState;
   CreateRasterState_t CreateRasterState;
   CreateMsaaState_t CreateMsaaState;
   CreateColorBlendState_t CreateColorBlendState;
   CreateDepthStencilState_t CreateDepthStencilState;

   // Command buffer functions

   CreateCommandBuffer_t CreateCommandBuffer;
   BeginCommandBuffer_t BeginCommandBuffer;
   EndCommandBuffer_t EndCommandBuffer;
   ResetCommandBuffer_t ResetCommandBuffer;

   // Command buffer building functions

   CmdBindPipeline_t CmdBindPipeline;
   CmdBindPipelineDelta_t CmdBindPipelineDelta;
   CmdBindStateObject_t CmdBindStateObject;
   CmdBindDescriptorSet_t CmdBindDescriptorSet;
   CmdBindDynamicMemoryView_t CmdBindDynamicMemoryView;
   CmdBindIndexData_t CmdBindIndexData;
   CmdBindAttachments_t CmdBindAttachments;
   CmdPrepareMemoryRegions_t CmdPrepareMemoryRegions;
   CmdPrepareImages_t CmdPrepareImages;
   CmdDraw_t CmdDraw;
   CmdDrawIndexed_t CmdDrawIndexed;
   CmdDrawIndirect_t CmdDrawIndirect;
   CmdDrawIndexedIndirect_t CmdDrawIndexedIndirect;
   CmdDispatch_t CmdDispatch;
   CmdDispatchIndirect_t CmdDispatchIndirect;
   CmdCopyMemory_t CmdCopyMemory;
   CmdCopyImage_t CmdCopyImage;
   CmdCopyMemoryToImage_t CmdCopyMemoryToImage;
   CmdCopyImageToMemory_t CmdCopyImageToMemory;
   CmdCloneImageData_t CmdCloneImageData;
   CmdUpdateMemory_t CmdUpdateMemory;
   CmdFillMemory_t CmdFillMemory;
   CmdClearColorImage_t CmdClearColorImage;
   CmdClearColorImageRaw_t CmdClearColorImageRaw;
   CmdClearDepthStencil_t CmdClearDepthStencil;
   CmdResolveImage_t CmdResolveImage;
   CmdSetEvent_t CmdSetEvent;
   CmdResetEvent_t CmdResetEvent;
   CmdMemoryAtomic_t CmdMemoryAtomic;
   CmdBeginQuery_t CmdBeginQuery;
   CmdEndQuery_t CmdEndQuery;
   CmdResetQueryPool_t CmdResetQueryPool;
   CmdWriteTimestamp_t CmdWriteTimestamp;
   CmdInitAtomicCounters_t CmdInitAtomicCounters;
   CmdLoadAtomicCounters_t CmdLoadAtomicCounters;
   CmdSaveAtomicCounters_t CmdSaveAtomicCounters;
};

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif /* EGLAPI_INCLUDED */
