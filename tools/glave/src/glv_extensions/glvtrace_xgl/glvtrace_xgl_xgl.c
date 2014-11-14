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
#include "glv_platform.h"
#include "glv_common.h"
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


static XGL_RESULT( XGLAPI * real_xglInitAndEnumerateGpus)(
    const XGL_APPLICATION_INFO* pAppInfo,
    const XGL_ALLOC_CALLBACKS*  pAllocCb,
    XGL_UINT                    maxGpus,
    XGL_UINT*                   pGpuCount,
    XGL_PHYSICAL_GPU*           pGpus) = xglInitAndEnumerateGpus;

static XGL_RESULT( XGLAPI * real_xglGetGpuInfo)(
    XGL_PHYSICAL_GPU            gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE  infoType,
    XGL_SIZE*                   pDataSize,
    XGL_VOID*                   pData) = xglGetGpuInfo;

static XGL_RESULT( XGLAPI * real_xglCreateDevice)(
    XGL_PHYSICAL_GPU              gpu,
    const XGL_DEVICE_CREATE_INFO* pCreateInfo,
    XGL_DEVICE*                   pDevice) = xglCreateDevice;

static XGL_RESULT( XGLAPI * real_xglDestroyDevice)(
    XGL_DEVICE device) = xglDestroyDevice;

static XGL_RESULT( XGLAPI * real_xglGetExtensionSupport)(
    XGL_PHYSICAL_GPU gpu,
    const XGL_CHAR*  pExtName) = xglGetExtensionSupport;

static XGL_RESULT( XGLAPI * real_xglGetDeviceQueue)(
    XGL_DEVICE       device,
    XGL_QUEUE_TYPE   queueType,
    XGL_UINT         queueIndex,
    XGL_QUEUE*       pQueue) = xglGetDeviceQueue;

static XGL_RESULT( XGLAPI * real_xglQueueSubmit)(
    XGL_QUEUE             queue,
    XGL_UINT              cmdBufferCount,
    const XGL_CMD_BUFFER* pCmdBuffers,
    XGL_UINT              memRefCount,
    const XGL_MEMORY_REF* pMemRefs,
    XGL_FENCE             fence) = xglQueueSubmit;

static XGL_RESULT( XGLAPI * real_xglQueueSetGlobalMemReferences)(
    XGL_QUEUE             queue,
    XGL_UINT              memRefCount,
    const XGL_MEMORY_REF* pMemRefs) = xglQueueSetGlobalMemReferences;

static XGL_RESULT( XGLAPI * real_xglQueueWaitIdle)(
    XGL_QUEUE queue) = xglQueueWaitIdle;

static XGL_RESULT( XGLAPI * real_xglDeviceWaitIdle)(
    XGL_DEVICE device) = xglDeviceWaitIdle;

static XGL_RESULT( XGLAPI * real_xglGetMemoryHeapCount)(
    XGL_DEVICE  device,
    XGL_UINT*   pCount) = xglGetMemoryHeapCount;

static XGL_RESULT( XGLAPI * real_xglGetMemoryHeapInfo)(
    XGL_DEVICE                  device,
    XGL_UINT                    heapId,
    XGL_MEMORY_HEAP_INFO_TYPE   infoType,
    XGL_SIZE*                   pDataSize,
    XGL_VOID*                   pData) = xglGetMemoryHeapInfo;

static XGL_RESULT( XGLAPI * real_xglAllocMemory)(
    XGL_DEVICE                   device,
    const XGL_MEMORY_ALLOC_INFO* pAllocInfo,
    XGL_GPU_MEMORY*              pMem) = xglAllocMemory;

static XGL_RESULT( XGLAPI * real_xglFreeMemory)(
    XGL_GPU_MEMORY mem) = xglFreeMemory;

static XGL_RESULT( XGLAPI * real_xglSetMemoryPriority)(
    XGL_GPU_MEMORY            mem,
    XGL_MEMORY_PRIORITY       priority) = xglSetMemoryPriority;

static XGL_RESULT( XGLAPI * real_xglMapMemory)(
    XGL_GPU_MEMORY mem,
    XGL_FLAGS      flags,                // Reserved
    XGL_VOID**     ppData) = xglMapMemory;

static XGL_RESULT( XGLAPI * real_xglUnmapMemory)(
    XGL_GPU_MEMORY mem) = xglUnmapMemory;

static XGL_RESULT( XGLAPI * real_xglPinSystemMemory)(
    XGL_DEVICE      device,
    const XGL_VOID* pSysMem,
    XGL_SIZE        memSize,
    XGL_GPU_MEMORY* pMem) = xglPinSystemMemory;

static XGL_RESULT( XGLAPI * real_xglRemapVirtualMemoryPages)(
    XGL_DEVICE                            device,
    XGL_UINT                              rangeCount,
    const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pRanges,
    XGL_UINT                              preWaitSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*            pPreWaitSemaphores,
    XGL_UINT                              postSignalSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*            pPostSignalSemaphores) = xglRemapVirtualMemoryPages;

static XGL_RESULT( XGLAPI * real_xglGetMultiGpuCompatibility)(
    XGL_PHYSICAL_GPU            gpu0,
    XGL_PHYSICAL_GPU            gpu1,
    XGL_GPU_COMPATIBILITY_INFO* pInfo) = xglGetMultiGpuCompatibility;

static XGL_RESULT( XGLAPI * real_xglOpenSharedMemory)(
    XGL_DEVICE                  device,
    const XGL_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY*             pMem) = xglOpenSharedMemory;

static XGL_RESULT( XGLAPI * real_xglOpenSharedQueueSemaphore)(
    XGL_DEVICE                           device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo,
    XGL_QUEUE_SEMAPHORE*                 pSemaphore) = xglOpenSharedQueueSemaphore;

static XGL_RESULT( XGLAPI * real_xglOpenPeerMemory)(
    XGL_DEVICE                       device,
    const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY*                  pMem) = xglOpenPeerMemory;

static XGL_RESULT( XGLAPI * real_xglOpenPeerImage)(
    XGL_DEVICE                      device,
    const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo,
    XGL_IMAGE*                      pImage,
    XGL_GPU_MEMORY*                 pMem) = xglOpenPeerImage;

static XGL_RESULT( XGLAPI * real_xglDestroyObject)(
    XGL_OBJECT object) = xglDestroyObject;

static XGL_RESULT( XGLAPI * real_xglGetObjectInfo)(
    XGL_BASE_OBJECT             object,
    XGL_OBJECT_INFO_TYPE        infoType,
    XGL_SIZE*                   pDataSize,
    XGL_VOID*                   pData) = xglGetObjectInfo;

static XGL_RESULT( XGLAPI * real_xglBindObjectMemory)(
    XGL_OBJECT     object,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset) = xglBindObjectMemory;

static XGL_RESULT( XGLAPI * real_xglCreateFence)(
    XGL_DEVICE                   device,
    const XGL_FENCE_CREATE_INFO* pCreateInfo,
    XGL_FENCE*                   pFence) = xglCreateFence;

static XGL_RESULT( XGLAPI * real_xglGetFenceStatus)(
    XGL_FENCE fence) = xglGetFenceStatus;

static XGL_RESULT( XGLAPI * real_xglWaitForFences)(
    XGL_DEVICE       device,
    XGL_UINT         fenceCount,
    const XGL_FENCE* pFences,
    XGL_BOOL         waitAll,
    XGL_UINT64       timeout) = xglWaitForFences;

static XGL_RESULT( XGLAPI * real_xglCreateQueueSemaphore)(
    XGL_DEVICE                             device,
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo,
    XGL_QUEUE_SEMAPHORE*                   pSemaphore) = xglCreateQueueSemaphore;

static XGL_RESULT( XGLAPI * real_xglSignalQueueSemaphore)(
    XGL_QUEUE           queue,
    XGL_QUEUE_SEMAPHORE semaphore) = xglSignalQueueSemaphore;

static XGL_RESULT( XGLAPI * real_xglWaitQueueSemaphore)(
    XGL_QUEUE           queue,
    XGL_QUEUE_SEMAPHORE semaphore) = xglWaitQueueSemaphore;

static XGL_RESULT( XGLAPI * real_xglCreateEvent)(
    XGL_DEVICE                   device,
    const XGL_EVENT_CREATE_INFO* pCreateInfo,
    XGL_EVENT*                   pEvent) = xglCreateEvent;

static XGL_RESULT( XGLAPI * real_xglGetEventStatus)(
    XGL_EVENT event) = xglGetEventStatus;

static XGL_RESULT( XGLAPI * real_xglSetEvent)(
    XGL_EVENT event) = xglSetEvent;

static XGL_RESULT( XGLAPI * real_xglResetEvent)(
    XGL_EVENT event) = xglResetEvent;

static XGL_RESULT( XGLAPI * real_xglCreateQueryPool)(
    XGL_DEVICE                        device,
    const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo,
    XGL_QUERY_POOL*                   pQueryPool) = xglCreateQueryPool;

static XGL_RESULT( XGLAPI * real_xglGetQueryPoolResults)(
    XGL_QUERY_POOL queryPool,
    XGL_UINT       startQuery,
    XGL_UINT       queryCount,
    XGL_SIZE*      pDataSize,
    XGL_VOID*      pData) = xglGetQueryPoolResults;

static XGL_RESULT( XGLAPI * real_xglGetFormatInfo)(
    XGL_DEVICE             device,
    XGL_FORMAT             format,
    XGL_FORMAT_INFO_TYPE   infoType,
    XGL_SIZE*              pDataSize,
    XGL_VOID*              pData) = xglGetFormatInfo;

static XGL_RESULT( XGLAPI * real_xglCreateImage)(
    XGL_DEVICE                   device,
    const XGL_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE*                   pImage) = xglCreateImage;

static XGL_RESULT( XGLAPI * real_xglGetImageSubresourceInfo)(
    XGL_IMAGE                    image,
    const XGL_IMAGE_SUBRESOURCE* pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE    infoType,
    XGL_SIZE*                    pDataSize,
    XGL_VOID*                    pData) = xglGetImageSubresourceInfo;

static XGL_RESULT( XGLAPI * real_xglCreateImageView)(
    XGL_DEVICE                        device,
    const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
    XGL_IMAGE_VIEW*                   pView) = xglCreateImageView;

static XGL_RESULT( XGLAPI * real_xglCreateColorAttachmentView)(
    XGL_DEVICE                                   device,
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW*                   pView) = xglCreateColorAttachmentView;

static XGL_RESULT( XGLAPI * real_xglCreateDepthStencilView)(
    XGL_DEVICE                                device,
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo,
    XGL_DEPTH_STENCIL_VIEW*                   pView) = xglCreateDepthStencilView;

static XGL_RESULT( XGLAPI * real_xglCreateShader)(
    XGL_DEVICE                    device,
    const XGL_SHADER_CREATE_INFO* pCreateInfo,
    XGL_SHADER*                   pShader) = xglCreateShader;

static XGL_RESULT( XGLAPI * real_xglCreateGraphicsPipeline)(
    XGL_DEVICE                               device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE*                            pPipeline) = xglCreateGraphicsPipeline;

static XGL_RESULT( XGLAPI * real_xglCreateComputePipeline)(
    XGL_DEVICE                              device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE*                           pPipeline) = xglCreateComputePipeline;

static XGL_RESULT( XGLAPI * real_xglStorePipeline)(
    XGL_PIPELINE pipeline,
    XGL_SIZE*    pDataSize,
    XGL_VOID*    pData) = xglStorePipeline;

static XGL_RESULT( XGLAPI * real_xglLoadPipeline)(
    XGL_DEVICE      device,
    XGL_SIZE        dataSize,
    const XGL_VOID* pData,
    XGL_PIPELINE*   pPipeline) = xglLoadPipeline;

static XGL_RESULT( XGLAPI * real_xglCreatePipelineDelta)(
    XGL_DEVICE      device,
    XGL_PIPELINE    p1,
    XGL_PIPELINE    p2,
    XGL_PIPELINE_DELTA*   delta) = xglCreatePipelineDelta;

static XGL_RESULT( XGLAPI * real_xglCreateSampler)(
    XGL_DEVICE                     device,
    const XGL_SAMPLER_CREATE_INFO* pCreateInfo,
    XGL_SAMPLER*                   pSampler) = xglCreateSampler;

static XGL_RESULT( XGLAPI * real_xglCreateDescriptorSet)(
    XGL_DEVICE                            device,
    const XGL_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo,
    XGL_DESCRIPTOR_SET*                   pDescriptorSet) = xglCreateDescriptorSet;


static XGL_VOID( XGLAPI * real_xglBeginDescriptorSetUpdate)(
    XGL_DESCRIPTOR_SET descriptorSet) = xglBeginDescriptorSetUpdate;

static XGL_VOID( XGLAPI * real_xglEndDescriptorSetUpdate)(
    XGL_DESCRIPTOR_SET descriptorSet) = xglEndDescriptorSetUpdate;

static XGL_VOID( XGLAPI * real_xglAttachSamplerDescriptors)(
    XGL_DESCRIPTOR_SET descriptorSet,
    XGL_UINT           startSlot,
    XGL_UINT           slotCount,
    const XGL_SAMPLER* pSamplers) = xglAttachSamplerDescriptors;

static XGL_VOID( XGLAPI * real_xglAttachImageViewDescriptors)(
    XGL_DESCRIPTOR_SET                descriptorSet,
    XGL_UINT                          startSlot,
    XGL_UINT                          slotCount,
    const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews) = xglAttachImageViewDescriptors;

static XGL_VOID( XGLAPI * real_xglAttachMemoryViewDescriptors)(
    XGL_DESCRIPTOR_SET                 descriptorSet,
    XGL_UINT                           startSlot,
    XGL_UINT                           slotCount,
    const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews) = xglAttachMemoryViewDescriptors;

static XGL_VOID( XGLAPI * real_xglAttachNestedDescriptors)(
    XGL_DESCRIPTOR_SET                    descriptorSet,
    XGL_UINT                              startSlot,
    XGL_UINT                              slotCount,
    const XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets) = xglAttachNestedDescriptors;

static XGL_VOID( XGLAPI * real_xglClearDescriptorSetSlots)(
    XGL_DESCRIPTOR_SET descriptorSet,
    XGL_UINT           startSlot,
    XGL_UINT           slotCount) = xglClearDescriptorSetSlots;

static XGL_RESULT( XGLAPI * real_xglCreateViewportState)(
    XGL_DEVICE                            device,
    const XGL_VIEWPORT_STATE_CREATE_INFO* pCreateInfo,
    XGL_VIEWPORT_STATE_OBJECT*            pState) = xglCreateViewportState;

static XGL_RESULT( XGLAPI * real_xglCreateRasterState)(
    XGL_DEVICE                          device,
    const XGL_RASTER_STATE_CREATE_INFO* pCreateInfo,
    XGL_RASTER_STATE_OBJECT*            pState) = xglCreateRasterState;

static XGL_RESULT( XGLAPI * real_xglCreateMsaaState)(
    XGL_DEVICE                        device,
    const XGL_MSAA_STATE_CREATE_INFO* pCreateInfo,
    XGL_MSAA_STATE_OBJECT*            pState) = xglCreateMsaaState;

static XGL_RESULT( XGLAPI * real_xglCreateColorBlendState)(
    XGL_DEVICE                               device,
    const XGL_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo,
    XGL_COLOR_BLEND_STATE_OBJECT*            pState) = xglCreateColorBlendState;

static XGL_RESULT( XGLAPI * real_xglCreateDepthStencilState)(
    XGL_DEVICE                                 device,
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo,
    XGL_DEPTH_STENCIL_STATE_OBJECT*            pState) = xglCreateDepthStencilState;

static XGL_RESULT( XGLAPI * real_xglCreateCommandBuffer)(
    XGL_DEVICE                        device,
    const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo,
    XGL_CMD_BUFFER*                   pCmdBuffer) = xglCreateCommandBuffer;

static XGL_RESULT( XGLAPI * real_xglBeginCommandBuffer)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_FLAGS      flags) = xglBeginCommandBuffer;               // XGL_CMD_BUFFER_BUILD_FLAGS

static XGL_RESULT( XGLAPI * real_xglEndCommandBuffer)(
    XGL_CMD_BUFFER cmdBuffer) = xglEndCommandBuffer;

static XGL_RESULT( XGLAPI * real_xglResetCommandBuffer)(
    XGL_CMD_BUFFER cmdBuffer) = xglResetCommandBuffer;

static XGL_VOID( XGLAPI * real_xglCmdBindPipeline)(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_PIPELINE                  pipeline) = xglCmdBindPipeline;

static XGL_VOID( XGLAPI * real_xglCmdBindPipelineDelta)(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_PIPELINE_DELTA            delta) = xglCmdBindPipelineDelta;

static XGL_VOID( XGLAPI * real_xglCmdBindStateObject)(
    XGL_CMD_BUFFER               cmdBuffer,
    XGL_STATE_BIND_POINT         stateBindPoint,
    XGL_STATE_OBJECT             state) = xglCmdBindStateObject;

static XGL_VOID( XGLAPI * real_xglCmdBindDescriptorSet)(
    XGL_CMD_BUFFER                    cmdBuffer,
    XGL_PIPELINE_BIND_POINT           pipelineBindPoint,
    XGL_UINT                          index,
    XGL_DESCRIPTOR_SET                descriptorSet,
    XGL_UINT                          slotOffset) = xglCmdBindDescriptorSet;

static XGL_VOID( XGLAPI * real_xglCmdBindDynamicMemoryView)(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_PIPELINE_BIND_POINT            pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView) = xglCmdBindDynamicMemoryView;

static XGL_VOID( XGLAPI * real_xglCmdBindIndexData)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset,
    XGL_INDEX_TYPE indexType) = xglCmdBindIndexData;

static XGL_VOID( XGLAPI * real_xglCmdBindAttachments)(
    XGL_CMD_BUFFER                         cmdBuffer,
    XGL_UINT                               colorTargetCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*  pColorTargets,
    const XGL_DEPTH_STENCIL_BIND_INFO*     pDepthTarget) = xglCmdBindAttachments;

static XGL_VOID( XGLAPI * real_xglCmdPrepareMemoryRegions)(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_UINT                           transitionCount,
    const XGL_MEMORY_STATE_TRANSITION* pStateTransitions) = xglCmdPrepareMemoryRegions;

static XGL_VOID( XGLAPI * real_xglCmdPrepareImages)(
    XGL_CMD_BUFFER                    cmdBuffer,
    XGL_UINT                          transitionCount,
    const XGL_IMAGE_STATE_TRANSITION* pStateTransitions) = xglCmdPrepareImages;

static XGL_VOID( XGLAPI * real_xglCmdDraw)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_UINT       firstVertex,
    XGL_UINT       vertexCount,
    XGL_UINT       firstInstance,
    XGL_UINT       instanceCount) = xglCmdDraw;

static XGL_VOID( XGLAPI * real_xglCmdDrawIndexed)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_UINT       firstIndex,
    XGL_UINT       indexCount,
    XGL_INT        vertexOffset,
    XGL_UINT       firstInstance,
    XGL_UINT       instanceCount) = xglCmdDrawIndexed;

static XGL_VOID( XGLAPI * real_xglCmdDrawIndirect)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset,
    XGL_UINT32     count,
    XGL_UINT32     stride) = xglCmdDrawIndirect;

static XGL_VOID( XGLAPI * real_xglCmdDrawIndexedIndirect)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset,
    XGL_UINT32     count,
    XGL_UINT32     stride) = xglCmdDrawIndexedIndirect;

static XGL_VOID( XGLAPI * real_xglCmdDispatch)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_UINT       x,
    XGL_UINT       y,
    XGL_UINT       z) = xglCmdDispatch;

static XGL_VOID( XGLAPI * real_xglCmdDispatchIndirect)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset) = xglCmdDispatchIndirect;

static XGL_VOID( XGLAPI * real_xglCmdCopyMemory)(
    XGL_CMD_BUFFER         cmdBuffer,
    XGL_GPU_MEMORY         srcMem,
    XGL_GPU_MEMORY         destMem,
    XGL_UINT               regionCount,
    const XGL_MEMORY_COPY* pRegions) = xglCmdCopyMemory;

static XGL_VOID( XGLAPI * real_xglCmdCopyImage)(
    XGL_CMD_BUFFER        cmdBuffer,
    XGL_IMAGE             srcImage,
    XGL_IMAGE             destImage,
    XGL_UINT              regionCount,
    const XGL_IMAGE_COPY* pRegions) = xglCmdCopyImage;

static XGL_VOID( XGLAPI * real_xglCmdCopyMemoryToImage)(
    XGL_CMD_BUFFER               cmdBuffer,
    XGL_GPU_MEMORY               srcMem,
    XGL_IMAGE                    destImage,
    XGL_UINT                     regionCount,
    const XGL_MEMORY_IMAGE_COPY* pRegions) = xglCmdCopyMemoryToImage;

static XGL_VOID( XGLAPI * real_xglCmdCopyImageToMemory)(
    XGL_CMD_BUFFER               cmdBuffer,
    XGL_IMAGE                    srcImage,
    XGL_GPU_MEMORY               destMem,
    XGL_UINT                     regionCount,
    const XGL_MEMORY_IMAGE_COPY* pRegions) = xglCmdCopyImageToMemory;

static XGL_VOID( XGLAPI * real_xglCmdCloneImageData)(
    XGL_CMD_BUFFER  cmdBuffer,
    XGL_IMAGE       srcImage,
    XGL_IMAGE_STATE srcImageState,
    XGL_IMAGE       destImage,
    XGL_IMAGE_STATE destImageState) = xglCmdCloneImageData;

static XGL_VOID( XGLAPI * real_xglCmdUpdateMemory)(
    XGL_CMD_BUFFER    cmdBuffer,
    XGL_GPU_MEMORY    destMem,
    XGL_GPU_SIZE      destOffset,
    XGL_GPU_SIZE      dataSize,
    const XGL_UINT32* pData) = xglCmdUpdateMemory;

static XGL_VOID( XGLAPI * real_xglCmdFillMemory)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY destMem,
    XGL_GPU_SIZE   destOffset,
    XGL_GPU_SIZE   fillSize,
    XGL_UINT32     data) = xglCmdFillMemory;

static XGL_VOID( XGLAPI * real_xglCmdClearColorImage)(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_IMAGE                          image,
    const XGL_FLOAT                    color[4],
    XGL_UINT                           rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges) = xglCmdClearColorImage;

static XGL_VOID( XGLAPI * real_xglCmdClearColorImageRaw)(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_IMAGE                          image,
    const XGL_UINT32                   color[4],
    XGL_UINT                           rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges) = xglCmdClearColorImageRaw;

static XGL_VOID( XGLAPI * real_xglCmdClearDepthStencil)(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_IMAGE                          image,
    XGL_FLOAT                          depth,
    XGL_UINT32                         stencil,
    XGL_UINT                           rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges) = xglCmdClearDepthStencil;

static XGL_VOID( XGLAPI * real_xglCmdResolveImage)(
    XGL_CMD_BUFFER           cmdBuffer,
    XGL_IMAGE                srcImage,
    XGL_IMAGE                destImage,
    XGL_UINT                 rectCount,
    const XGL_IMAGE_RESOLVE* pRects) = xglCmdResolveImage;

static XGL_VOID( XGLAPI * real_xglCmdSetEvent)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT      event) = xglCmdSetEvent;

static XGL_VOID( XGLAPI * real_xglCmdResetEvent)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT      event) = xglCmdResetEvent;

static XGL_VOID( XGLAPI * real_xglCmdMemoryAtomic)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY destMem,
    XGL_GPU_SIZE   destOffset,
    XGL_UINT64     srcData,
    XGL_ATOMIC_OP  atomicOp) = xglCmdMemoryAtomic;

static XGL_VOID( XGLAPI * real_xglCmdBeginQuery)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    XGL_UINT       slot,
    XGL_FLAGS      flags) = xglCmdBeginQuery;

static XGL_VOID( XGLAPI * real_xglCmdEndQuery)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    XGL_UINT       slot) = xglCmdEndQuery;

static XGL_VOID( XGLAPI * real_xglCmdResetQueryPool)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    XGL_UINT       startQuery,
    XGL_UINT       queryCount) = xglCmdResetQueryPool;

static XGL_VOID( XGLAPI * real_xglCmdWriteTimestamp)(
    XGL_CMD_BUFFER           cmdBuffer,
    XGL_TIMESTAMP_TYPE       timestampType,
    XGL_GPU_MEMORY           destMem,
    XGL_GPU_SIZE             destOffset) = xglCmdWriteTimestamp;

static XGL_VOID( XGLAPI * real_xglCmdInitAtomicCounters)(
    XGL_CMD_BUFFER                   cmdBuffer,
    XGL_PIPELINE_BIND_POINT          pipelineBindPoint,
    XGL_UINT                         startCounter,
    XGL_UINT                         counterCount,
    const XGL_UINT32*                pData) = xglCmdInitAtomicCounters;

static XGL_VOID( XGLAPI * real_xglCmdLoadAtomicCounters)(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_UINT                      startCounter,
    XGL_UINT                      counterCount,
    XGL_GPU_MEMORY                srcMem,
    XGL_GPU_SIZE                  srcOffset) = xglCmdLoadAtomicCounters;

static XGL_VOID( XGLAPI * real_xglCmdSaveAtomicCounters)(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_UINT                      startCounter,
    XGL_UINT                      counterCount,
    XGL_GPU_MEMORY                destMem,
    XGL_GPU_SIZE                  destOffset) = xglCmdSaveAtomicCounters;

static BOOL isHooked = FALSE;

void AttachHooks()
{
    BOOL hookSuccess = TRUE;
#if defined(WIN32)
    Mhook_BeginMultiOperation(FALSE);
    if (real_xglInitAndEnumerateGpus != NULL)
    {
        isHooked = TRUE;
        hookSuccess = Mhook_SetHook((PVOID*)&real_xglInitAndEnumerateGpus, hooked_xglInitAndEnumerateGpus);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetGpuInfo, hooked_xglGetGpuInfo);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDevice, hooked_xglCreateDevice);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglDestroyDevice, hooked_xglDestroyDevice);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetExtensionSupport, hooked_xglGetExtensionSupport);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetDeviceQueue, hooked_xglGetDeviceQueue);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglQueueSubmit, hooked_xglQueueSubmit);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglQueueSetGlobalMemReferences, hooked_xglQueueSetGlobalMemReferences);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglQueueWaitIdle, hooked_xglQueueWaitIdle);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglDeviceWaitIdle, hooked_xglDeviceWaitIdle);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetMemoryHeapCount, hooked_xglGetMemoryHeapCount);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetMemoryHeapInfo, hooked_xglGetMemoryHeapInfo);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglAllocMemory, hooked_xglAllocMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglFreeMemory, hooked_xglFreeMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglSetMemoryPriority, hooked_xglSetMemoryPriority);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglMapMemory, hooked_xglMapMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglUnmapMemory, hooked_xglUnmapMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglPinSystemMemory, hooked_xglPinSystemMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglRemapVirtualMemoryPages, hooked_xglRemapVirtualMemoryPages);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetMultiGpuCompatibility, hooked_xglGetMultiGpuCompatibility);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglOpenSharedMemory, hooked_xglOpenSharedMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglOpenSharedQueueSemaphore, hooked_xglOpenSharedQueueSemaphore);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglOpenPeerMemory, hooked_xglOpenPeerMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglOpenPeerImage, hooked_xglOpenPeerImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglDestroyObject, hooked_xglDestroyObject);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetObjectInfo, hooked_xglGetObjectInfo);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglBindObjectMemory, hooked_xglBindObjectMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateFence, hooked_xglCreateFence);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetFenceStatus, hooked_xglGetFenceStatus);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglWaitForFences, hooked_xglWaitForFences);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateQueueSemaphore, hooked_xglCreateQueueSemaphore);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglSignalQueueSemaphore, hooked_xglSignalQueueSemaphore);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglWaitQueueSemaphore, hooked_xglWaitQueueSemaphore);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateEvent, hooked_xglCreateEvent);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetEventStatus, hooked_xglGetEventStatus);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglSetEvent, hooked_xglSetEvent);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglResetEvent, hooked_xglResetEvent);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateQueryPool, hooked_xglCreateQueryPool);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetQueryPoolResults, hooked_xglGetQueryPoolResults);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetFormatInfo, hooked_xglGetFormatInfo);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateImage, hooked_xglCreateImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetImageSubresourceInfo, hooked_xglGetImageSubresourceInfo);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateImageView, hooked_xglCreateImageView);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateColorAttachmentView, hooked_xglCreateColorAttachmentView);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDepthStencilView, hooked_xglCreateDepthStencilView);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateShader, hooked_xglCreateShader);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateGraphicsPipeline, hooked_xglCreateGraphicsPipeline);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateComputePipeline, hooked_xglCreateComputePipeline);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglStorePipeline, hooked_xglStorePipeline);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglLoadPipeline, hooked_xglLoadPipeline);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreatePipelineDelta, hooked_xglCreatePipelineDelta);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateSampler, hooked_xglCreateSampler);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDescriptorSet, hooked_xglCreateDescriptorSet);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglBeginDescriptorSetUpdate, hooked_xglBeginDescriptorSetUpdate);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglEndDescriptorSetUpdate, hooked_xglEndDescriptorSetUpdate);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglAttachSamplerDescriptors, hooked_xglAttachSamplerDescriptors);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglAttachImageViewDescriptors, hooked_xglAttachImageViewDescriptors);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglAttachMemoryViewDescriptors, hooked_xglAttachMemoryViewDescriptors);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglAttachNestedDescriptors, hooked_xglAttachNestedDescriptors);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglClearDescriptorSetSlots, hooked_xglClearDescriptorSetSlots);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateViewportState, hooked_xglCreateViewportState);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateRasterState, hooked_xglCreateRasterState);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateMsaaState, hooked_xglCreateMsaaState);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateColorBlendState, hooked_xglCreateColorBlendState);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDepthStencilState, hooked_xglCreateDepthStencilState);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateCommandBuffer, hooked_xglCreateCommandBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglBeginCommandBuffer, hooked_xglBeginCommandBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglEndCommandBuffer, hooked_xglEndCommandBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglResetCommandBuffer, hooked_xglResetCommandBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindPipeline, hooked_xglCmdBindPipeline);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindPipelineDelta, hooked_xglCmdBindPipelineDelta);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindStateObject, hooked_xglCmdBindStateObject);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindDescriptorSet, hooked_xglCmdBindDescriptorSet);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindDynamicMemoryView, hooked_xglCmdBindDynamicMemoryView);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindIndexData, hooked_xglCmdBindIndexData);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindAttachments, hooked_xglCmdBindAttachments);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdPrepareMemoryRegions, hooked_xglCmdPrepareMemoryRegions);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdPrepareImages, hooked_xglCmdPrepareImages);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDraw, hooked_xglCmdDraw);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDrawIndexed, hooked_xglCmdDrawIndexed);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDrawIndirect, hooked_xglCmdDrawIndirect);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDrawIndexedIndirect, hooked_xglCmdDrawIndexedIndirect);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDispatch, hooked_xglCmdDispatch);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDispatchIndirect, hooked_xglCmdDispatchIndirect);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCopyMemory, hooked_xglCmdCopyMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCopyImage, hooked_xglCmdCopyImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCopyMemoryToImage, hooked_xglCmdCopyMemoryToImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCopyImageToMemory, hooked_xglCmdCopyImageToMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCloneImageData, hooked_xglCmdCloneImageData);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdUpdateMemory, hooked_xglCmdUpdateMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdFillMemory, hooked_xglCmdFillMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdClearColorImage, hooked_xglCmdClearColorImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdClearColorImageRaw, hooked_xglCmdClearColorImageRaw);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdClearDepthStencil, hooked_xglCmdClearDepthStencil);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdResolveImage, hooked_xglCmdResolveImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdSetEvent, hooked_xglCmdSetEvent);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdResetEvent, hooked_xglCmdResetEvent);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdMemoryAtomic, hooked_xglCmdMemoryAtomic);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBeginQuery, hooked_xglCmdBeginQuery);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdEndQuery, hooked_xglCmdEndQuery);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdResetQueryPool, hooked_xglCmdResetQueryPool);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdWriteTimestamp, hooked_xglCmdWriteTimestamp);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdInitAtomicCounters, hooked_xglCmdInitAtomicCounters);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdLoadAtomicCounters, hooked_xglCmdLoadAtomicCounters);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdSaveAtomicCounters, hooked_xglCmdSaveAtomicCounters);
    }

    if (!hookSuccess)
    {
        glv_LogError("Failed to hook XGL.");
    }

    Mhook_EndMultiOperation();

#elif defined(__linux__)
    if (real_xglInitAndEnumerateGpus == xglInitAndEnumerateGpus)
        hookSuccess = glv_platform_get_next_lib_sym((PVOID*)&real_xglInitAndEnumerateGpus,"xglInitAndEnumerateGpus");
    isHooked = TRUE;
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetGpuInfo, "xglGetGpuInfo");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDevice, "xglCreateDevice");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglDestroyDevice, "xglDestroyDevice");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetExtensionSupport, "xglGetExtensionSupport");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetDeviceQueue, "xglGetDeviceQueue");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglQueueSubmit, "xglQueueSubmit");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglQueueSetGlobalMemReferences, "xglQueueSetGlobalMemReferences");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglQueueWaitIdle, "xglQueueWaitIdle");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglDeviceWaitIdle, "xglDeviceWaitIdle");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetMemoryHeapCount, "xglGetMemoryHeapCount");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetMemoryHeapInfo, "xglGetMemoryHeapInfo");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglAllocMemory, "xglAllocMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglFreeMemory, "xglFreeMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglSetMemoryPriority, "xglSetMemoryPriority");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglMapMemory, "xglMapMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglUnmapMemory, "xglUnmapMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglPinSystemMemory, "xglPinSystemMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglRemapVirtualMemoryPages, "xglRemapVirtualMemoryPages");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetMultiGpuCompatibility, "xglGetMultiGpuCompatibility");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglOpenSharedMemory, "xglOpenSharedMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglOpenSharedQueueSemaphore, "xglOpenSharedQueueSemaphore");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglOpenPeerMemory, "xglOpenPeerMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglOpenPeerImage, "xglOpenPeerImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglDestroyObject, "xglDestroyObject");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetObjectInfo, "xglGetObjectInfo");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglBindObjectMemory, "xglBindObjectMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateFence, "xglCreateFence");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetFenceStatus, "xglGetFenceStatus");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglWaitForFences, "xglWaitForFences");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateQueueSemaphore, "xglCreateQueueSemaphore");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglSignalQueueSemaphore, "xglSignalQueueSemaphore");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglWaitQueueSemaphore, "xglWaitQueueSemaphore");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateEvent, "xglCreateEvent");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetEventStatus, "xglGetEventStatus");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglSetEvent, "xglSetEvent");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglResetEvent, "xglResetEvent");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateQueryPool, "xglCreateQueryPool");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetQueryPoolResults, "xglGetQueryPoolResults");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetFormatInfo, "xglGetFormatInfo");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateImage, "xglCreateImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetImageSubresourceInfo, "xglGetImageSubresourceInfo");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateImageView, "xglCreateImageView");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateColorAttachmentView, "xglCreateColorAttachmentView");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDepthStencilView, "xglCreateDepthStencilView");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateShader, "xglCreateShader");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateGraphicsPipeline, "xglCreateGraphicsPipeline");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateComputePipeline, "xglCreateComputePipeline");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglStorePipeline, "xglStorePipeline");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglLoadPipeline, "xglLoadPipeline");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreatePipelineDelta, "xglCreatePipelineDelta");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateSampler, "xglCreateSampler");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDescriptorSet, "xglCreateDescriptorSet");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglBeginDescriptorSetUpdate, "xglBeginDescriptorSetUpdate");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglEndDescriptorSetUpdate, "xglEndDescriptorSetUpdate");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglAttachSamplerDescriptors, "xglAttachSamplerDescriptors");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglAttachImageViewDescriptors, "xglAttachImageViewDescriptors");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglAttachMemoryViewDescriptors, "xglAttachMemoryViewDescriptors");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglAttachNestedDescriptors, "xglAttachNestedDescriptors");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglClearDescriptorSetSlots, "xglClearDescriptorSetSlots");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateViewportState, "xglCreateViewportState");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateRasterState, "xglCreateRasterState");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateMsaaState, "xglCreateMsaaState");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateColorBlendState, "xglCreateColorBlendState");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDepthStencilState, "xglCreateDepthStencilState");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateCommandBuffer, "xglCreateCommandBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglBeginCommandBuffer, "xglBeginCommandBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglEndCommandBuffer, "xglEndCommandBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglResetCommandBuffer, "xglResetCommandBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindPipeline, "xglCmdBindPipeline");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindPipelineDelta, "xglCmdBindPipelineDelta");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindStateObject, "xglCmdBindStateObject");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindDescriptorSet, "xglCmdBindDescriptorSet");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindDynamicMemoryView, "xglCmdBindDynamicMemoryView");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindIndexData, "xglCmdBindIndexData");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindAttachments, "xglCmdBindAttachments");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdPrepareMemoryRegions, "xglCmdPrepareMemoryRegions");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdPrepareImages, "xglCmdPrepareImages");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDraw, "xglCmdDraw");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDrawIndexed, "xglCmdDrawIndexed");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDrawIndirect, "xglCmdDrawIndirect");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDrawIndexedIndirect, "xglCmdDrawIndexedIndirect");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDispatch, "xglCmdDispatch");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDispatchIndirect, "xglCmdDispatchIndirect");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCopyMemory, "xglCmdCopyMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCopyImage, "xglCmdCopyImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCopyMemoryToImage, "xglCmdCopyMemoryToImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCopyImageToMemory, "xglCmdCopyImageToMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCloneImageData, "xglCmdCloneImageData");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdUpdateMemory, "xglCmdUpdateMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdFillMemory, "xglCmdFillMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdClearColorImage, "xglCmdClearColorImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdClearColorImageRaw, "xglCmdClearColorImageRaw");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdClearDepthStencil, "xglCmdClearDepthStencil");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdResolveImage, "xglCmdResolveImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdSetEvent, "xglCmdSetEvent");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdResetEvent, "xglCmdResetEvent");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdMemoryAtomic, "xglCmdMemoryAtomic");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBeginQuery, "xglCmdBeginQuery");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdEndQuery, "xglCmdEndQuery");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdResetQueryPool, "xglCmdResetQueryPool");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdWriteTimestamp, "xglCmdWriteTimestamp");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdInitAtomicCounters, "xglCmdInitAtomicCounters");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdLoadAtomicCounters, "xglCmdLoadAtomicCounters");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdSaveAtomicCounters, "xglCmdSaveAtomicCounters");
    if (!hookSuccess)
    {
        glv_LogError("Failed to hook XGL.");
    }

#endif
}

void DetachHooks()
{
#ifdef __linux__
    return;
#elif defined(WIN32)
    BOOL unhookSuccess = TRUE;
    if (real_xglGetGpuInfo != NULL)
    {
        unhookSuccess = Mhook_Unhook((PVOID*)&real_xglInitAndEnumerateGpus);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetGpuInfo);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDevice);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglDestroyDevice);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetExtensionSupport);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetDeviceQueue);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglQueueSubmit);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglQueueSetGlobalMemReferences);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglQueueWaitIdle);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglDeviceWaitIdle);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetMemoryHeapCount);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetMemoryHeapInfo);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglAllocMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglFreeMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglSetMemoryPriority);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglMapMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglUnmapMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglPinSystemMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglRemapVirtualMemoryPages);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetMultiGpuCompatibility);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglOpenSharedMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglOpenSharedQueueSemaphore);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglOpenPeerMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglOpenPeerImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglDestroyObject);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetObjectInfo);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglBindObjectMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateFence);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetFenceStatus);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglWaitForFences);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateQueueSemaphore);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglSignalQueueSemaphore);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglWaitQueueSemaphore);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateEvent);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetEventStatus);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglSetEvent);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglResetEvent);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateQueryPool);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetQueryPoolResults);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetFormatInfo);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetImageSubresourceInfo);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateImageView);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateColorAttachmentView);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDepthStencilView);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateShader);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateGraphicsPipeline);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateComputePipeline);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglStorePipeline);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglLoadPipeline);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreatePipelineDelta);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateSampler);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDescriptorSet);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglBeginDescriptorSetUpdate);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglEndDescriptorSetUpdate);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglAttachSamplerDescriptors);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglAttachImageViewDescriptors);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglAttachMemoryViewDescriptors);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglAttachNestedDescriptors);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglClearDescriptorSetSlots);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateViewportState);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateRasterState);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateMsaaState);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateColorBlendState);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDepthStencilState);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateCommandBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglBeginCommandBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglEndCommandBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglResetCommandBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindPipeline);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindPipelineDelta);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindStateObject);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindDescriptorSet);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindDynamicMemoryView);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindIndexData);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindAttachments);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdPrepareMemoryRegions);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdPrepareImages);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDraw);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDrawIndexed);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDrawIndirect);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDrawIndexedIndirect);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDispatch);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDispatchIndirect);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCopyMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCopyImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCopyMemoryToImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCopyImageToMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCloneImageData);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdUpdateMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdFillMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdClearColorImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdClearColorImageRaw);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdClearDepthStencil);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdResolveImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdSetEvent);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdResetEvent);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdMemoryAtomic);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBeginQuery);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdEndQuery);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdResetQueryPool);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdWriteTimestamp);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdInitAtomicCounters);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdLoadAtomicCounters);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdSaveAtomicCounters);
    }
    isHooked = FALSE;

    if (!unhookSuccess)
    {
        glv_LogError("Failed to unhook XGL.");
    }
#endif
}

// GPU initialization

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglInitAndEnumerateGpus(
    const XGL_APPLICATION_INFO* pAppInfo,
    const XGL_ALLOC_CALLBACKS*  pAllocCb,
    XGL_UINT                    maxGpus,
    XGL_UINT*                   pGpuCount,
    XGL_PHYSICAL_GPU*           pGpus)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    uint64_t startTime;
    struct_xglInitAndEnumerateGpus* pPacket;
    SEND_ENTRYPOINT_ID(xglInitAndEnumerateGpus);

    if (real_xglInitAndEnumerateGpus == xglInitAndEnumerateGpus)
    {
        glv_platform_get_next_lib_sym((void **) &real_xglInitAndEnumerateGpus,"xglInitAndEnumerateGpus");
    }
    startTime = glv_get_time();
    result = real_xglInitAndEnumerateGpus(pAppInfo, pAllocCb, maxGpus, pGpuCount, pGpus);

    // since we don't know how many gpus will be found must  create trace packet after calling xglInit
    CREATE_TRACE_PACKET(xglInitAndEnumerateGpus, calc_size_XGL_APPLICATION_INFO(pAppInfo) + ((pAllocCb == NULL) ? 0 :sizeof(XGL_ALLOC_CALLBACKS))
        + sizeof(XGL_UINT) + ((pGpus && pGpuCount) ? *pGpuCount * sizeof(XGL_PHYSICAL_GPU) : 0));
    pHeader->entrypoint_begin_time = startTime;
    if (isHooked == FALSE) {
        AttachHooks();
        AttachHooks_xgldbg();
        AttachHooks_xglwsix11ext();
    }
    pPacket = interpret_body_as_xglInitAndEnumerateGpus(pHeader);
    add_XGL_APPLICATION_INFO_to_packet(pHeader, (XGL_APPLICATION_INFO**)&(pPacket->pAppInfo), pAppInfo);
    if (pAllocCb) {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocCb), sizeof(XGL_ALLOC_CALLBACKS), pAllocCb);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAllocCb));
    }
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpuCount), sizeof(XGL_UINT), pGpuCount);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pGpuCount));
    if (pGpuCount && pGpus)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pGpus), sizeof(XGL_PHYSICAL_GPU) * *pGpuCount, pGpus);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pGpus));
    }
    pPacket->maxGpus = maxGpus;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetGpuInfo(
    XGL_PHYSICAL_GPU                   gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE         infoType,
    XGL_SIZE*                          pDataSize,
    XGL_VOID*                          pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetGpuInfo* pPacket;
    SEND_ENTRYPOINT_ID(xglGetGpuInfo);
    CREATE_TRACE_PACKET(xglGetGpuInfo, ((pDataSize != NULL)? sizeof(XGL_SIZE) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglGetGpuInfo(gpu, infoType, pDataSize, pData);
    pPacket = interpret_body_as_xglGetGpuInfo(pHeader);
    pPacket->gpu = gpu;
    pPacket->infoType = infoType;
    pPacket->result = result;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(XGL_SIZE), pDataSize);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    if (pData && pDataSize)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), *pDataSize, pData);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    }
    FINISH_TRACE_PACKET();
    return result;
}

// Device functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDevice(
    XGL_PHYSICAL_GPU              gpu,
    const XGL_DEVICE_CREATE_INFO* pCreateInfo,
    XGL_DEVICE*                   pDevice)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDevice* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateDevice);
    CREATE_TRACE_PACKET(xglCreateDevice, calc_size_XGL_DEVICE_CREATE_INFO(pCreateInfo) + sizeof(XGL_DEVICE));
    result = real_xglCreateDevice(gpu, pCreateInfo, pDevice);
    pPacket = interpret_body_as_xglCreateDevice(pHeader);
    pPacket->gpu = gpu;
    add_XGL_DEVICE_CREATE_INFO_to_packet(pHeader, (XGL_DEVICE_CREATE_INFO**) &(pPacket->pCreateInfo), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDevice), sizeof(XGL_DEVICE), pDevice);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDevice));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglDestroyDevice(
    XGL_DEVICE device)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglDestroyDevice* pPacket;
    SEND_ENTRYPOINT_ID(xglDestroyDevice);
    CREATE_TRACE_PACKET(xglDestroyDevice, 0);
    result = real_xglDestroyDevice(device);
    pPacket = interpret_body_as_xglDestroyDevice(pHeader);
    pPacket->device = device;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// Extension discovery functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetExtensionSupport(
    XGL_PHYSICAL_GPU gpu,
    const XGL_CHAR*  pExtName)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetExtensionSupport* pPacket;
    SEND_ENTRYPOINT_ID(xglGetExtensionSupport);
    CREATE_TRACE_PACKET(xglGetExtensionSupport, strlen((const char *)pExtName) + 1);
    result = real_xglGetExtensionSupport(gpu, pExtName);
    pPacket = interpret_body_as_xglGetExtensionSupport(pHeader);
    pPacket->gpu = gpu;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pExtName), strlen((const char *)pExtName) + 1, pExtName);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pExtName));
    FINISH_TRACE_PACKET();
    return result;
}

// Queue functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetDeviceQueue(
    XGL_DEVICE       device,
    XGL_QUEUE_TYPE   queueType,
    XGL_UINT         queueIndex,
    XGL_QUEUE*       pQueue)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetDeviceQueue* pPacket;
    SEND_ENTRYPOINT_ID(xglGetDeviceQueue);
    CREATE_TRACE_PACKET(xglGetDeviceQueue, sizeof(XGL_QUEUE));
    result = real_xglGetDeviceQueue(device, queueType, queueIndex, pQueue);
    pPacket = interpret_body_as_xglGetDeviceQueue(pHeader);
    pPacket->device = device;
    pPacket->queueType = queueType;
    pPacket->queueIndex = queueIndex;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pQueue), sizeof(XGL_QUEUE), pQueue);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pQueue));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglQueueSubmit(
    XGL_QUEUE             queue,
    XGL_UINT              cmdBufferCount,
    const XGL_CMD_BUFFER* pCmdBuffers,
    XGL_UINT              memRefCount,
    const XGL_MEMORY_REF* pMemRefs,
    XGL_FENCE             fence)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglQueueSubmit* pPacket;
    SEND_ENTRYPOINT_ID(xglQueueSubmit);
    CREATE_TRACE_PACKET(xglQueueSubmit, cmdBufferCount*sizeof(XGL_CMD_BUFFER) + memRefCount*sizeof(XGL_MEMORY_REF));
    result = real_xglQueueSubmit(queue, cmdBufferCount, pCmdBuffers, memRefCount, pMemRefs, fence);
    pPacket = interpret_body_as_xglQueueSubmit(pHeader);
    pPacket->queue = queue;
    pPacket->cmdBufferCount = cmdBufferCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCmdBuffers), cmdBufferCount*sizeof(XGL_CMD_BUFFER), pCmdBuffers);
    pPacket->memRefCount = memRefCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMemRefs), memRefCount*sizeof(XGL_MEMORY_REF), pMemRefs);
    pPacket->fence = fence;
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCmdBuffers));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemRefs));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglQueueSetGlobalMemReferences(
    XGL_QUEUE             queue,
    XGL_UINT              memRefCount,
    const XGL_MEMORY_REF* pMemRefs)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglQueueSetGlobalMemReferences* pPacket;
    SEND_ENTRYPOINT_ID(xglQueueSetGlobalMemReferences);
    CREATE_TRACE_PACKET(xglQueueSetGlobalMemReferences, memRefCount*sizeof(XGL_MEMORY_REF));
    result = real_xglQueueSetGlobalMemReferences(queue, memRefCount, pMemRefs);
    pPacket = interpret_body_as_xglQueueSetGlobalMemReferences(pHeader);
    pPacket->queue = queue;
    pPacket->memRefCount = memRefCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMemRefs), memRefCount*sizeof(XGL_MEMORY_REF), pMemRefs);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemRefs));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglQueueWaitIdle(
    XGL_QUEUE queue)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglQueueWaitIdle* pPacket;
    SEND_ENTRYPOINT_ID(xglQueueWaitIdle);
    CREATE_TRACE_PACKET(xglQueueWaitIdle, 0);
    result = real_xglQueueWaitIdle(queue);
    pPacket = interpret_body_as_xglQueueWaitIdle(pHeader);
    pPacket->queue = queue;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglDeviceWaitIdle(
    XGL_DEVICE device)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglDeviceWaitIdle* pPacket;
    SEND_ENTRYPOINT_ID(xglDeviceWaitIdle);
    CREATE_TRACE_PACKET(xglDeviceWaitIdle, 0);
    result = real_xglDeviceWaitIdle(device);
    pPacket = interpret_body_as_xglDeviceWaitIdle(pHeader);
    pPacket->device = device;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// Support for shadowing CPU mapped memory
typedef struct _XGLAllocInfo {
    XGL_GPU_SIZE   size;
    XGL_GPU_MEMORY handle;
    XGL_VOID       *pData;
    BOOL           valid;
} XGLAllocInfo;
typedef struct _XGLMemInfo {
    unsigned int numEntrys;
    XGLAllocInfo *pEntrys;
    XGLAllocInfo *pLastMapped;
    unsigned int capacity;
} XGLMemInfo;

static XGLMemInfo mInfo = {0, NULL, NULL, 0};

static void init_mem_info_entrys(XGLAllocInfo *ptr, const unsigned int num)
{
    unsigned int i;
    for (i = 0; i < num; i++)
    {
        XGLAllocInfo *entry = ptr + i;
        entry->pData = NULL;
        entry->size  = 0;
        entry->handle = NULL;
        entry->valid = FALSE;
    }
}

static void init_mem_info()
{
    mInfo.numEntrys = 0;
    mInfo.capacity = 1024;
    mInfo.pLastMapped = NULL;

    mInfo.pEntrys = GLV_NEW_ARRAY(XGLAllocInfo, mInfo.capacity);

    if (mInfo.pEntrys == NULL)
        glv_LogError("init_mem_info()  malloc failed\n");
    else
        init_mem_info_entrys(mInfo.pEntrys, mInfo.capacity);
}

static void delete_mem_info()
{
    GLV_DELETE(mInfo.pEntrys);
    mInfo.pEntrys = NULL;
    mInfo.numEntrys = 0;
    mInfo.capacity = 0;
    mInfo.pLastMapped = NULL;
}

static XGLAllocInfo * get_mem_info_entry()
{
    unsigned int i;
    XGLAllocInfo *entry;
    if (mInfo.numEntrys > mInfo.capacity)
    {
        glv_LogError("get_mem_info_entry() bad internal state numEntrys\n");
        return NULL;
    }

    if (mInfo.numEntrys == mInfo.capacity)
    {  // grow the array 2x
        mInfo.capacity *= 2;
        mInfo.pEntrys = (XGLAllocInfo *) GLV_REALLOC(mInfo.pEntrys, mInfo.capacity * sizeof(XGLAllocInfo));
        //init the newly added entrys
        init_mem_info_entrys(mInfo.pEntrys + mInfo.capacity / 2, mInfo.capacity / 2);
    }

    assert(mInfo.numEntrys < mInfo.capacity);
    entry = mInfo.pEntrys;
    for (i = 0; i < mInfo.capacity; i++)
    {
        if ((entry + i)->valid == FALSE)
            return entry + i;
    }

    glv_LogError("get_mem_info_entry() didn't find an entry\n");
    return NULL;
}

static XGLAllocInfo * find_mem_info_entry(const XGL_GPU_MEMORY handle)
{
    XGLAllocInfo *entry = mInfo.pEntrys;
    unsigned int i;
    if (mInfo.pLastMapped && mInfo.pLastMapped->handle == handle && mInfo.pLastMapped->valid)
        return mInfo.pLastMapped;
    for (i = 0; i < mInfo.numEntrys; i++)
    {
        if ((entry + i)->valid && (handle == (entry + i)->handle))
            return entry + i;
    }

    return NULL;
}

static void add_new_handle_to_mem_info(const XGL_GPU_MEMORY handle, XGL_GPU_SIZE size, XGL_VOID *pData)
{
    XGLAllocInfo *entry;

    if (mInfo.capacity == 0)
        init_mem_info();

    entry = get_mem_info_entry();
    if (entry)
    {
        entry->valid = TRUE;
        entry->handle = handle;
        entry->size = size;
        entry->pData = pData;   // NOTE: xglFreeMemory will free this mem, so no malloc()
        mInfo.numEntrys++;
    }
}

static void add_data_to_mem_info(const XGL_GPU_MEMORY handle, XGL_VOID *pData)
{
    XGLAllocInfo *entry = find_mem_info_entry(handle);

    if (entry)
    {
        entry->pData = pData;
    }
    mInfo.pLastMapped = entry;
}

static void rm_handle_from_mem_info(const XGL_GPU_MEMORY handle)
{
    XGLAllocInfo *entry = find_mem_info_entry(handle);

    if (entry)
    {
        entry->valid = FALSE;
        entry->pData = NULL;
        entry->size = 0;
        entry->handle = NULL;

        mInfo.numEntrys--;
        if (entry == mInfo.pLastMapped)
            mInfo.pLastMapped = NULL;
        if (mInfo.numEntrys == 0)
            delete_mem_info();
    }
}

// Memory functions
GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetMemoryHeapCount(
    XGL_DEVICE  device,
    XGL_UINT*   pCount)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetMemoryHeapCount* pPacket;
    SEND_ENTRYPOINT_ID(xglGetMemoryHeapCount);
    CREATE_TRACE_PACKET(xglGetMemoryHeapCount, sizeof(XGL_UINT));
    result = real_xglGetMemoryHeapCount(device, pCount);
    pPacket = interpret_body_as_xglGetMemoryHeapCount(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCount), sizeof(XGL_UINT), pCount);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCount));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetMemoryHeapInfo(
    XGL_DEVICE                  device,
    XGL_UINT                    heapId,
    XGL_MEMORY_HEAP_INFO_TYPE   infoType,
    XGL_SIZE*                   pDataSize,
    XGL_VOID*                   pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetMemoryHeapInfo* pPacket;
    SEND_ENTRYPOINT_ID(xglGetMemoryHeapInfo);
    CREATE_TRACE_PACKET(xglGetMemoryHeapInfo, ((pDataSize != NULL) ? sizeof(XGL_SIZE) : 0) + ((pDataSize != NULL && pData != NULL)? *pDataSize : 0));
    result = real_xglGetMemoryHeapInfo(device, heapId, infoType, pDataSize, pData);
    pPacket = interpret_body_as_xglGetMemoryHeapInfo(pHeader);
    pPacket->device = device;
    pPacket->heapId = heapId;
    pPacket->infoType = infoType;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(XGL_SIZE), pDataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), (pDataSize != NULL && pData != NULL)? *pDataSize : 0, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglAllocMemory(
    XGL_DEVICE                   device,
    const XGL_MEMORY_ALLOC_INFO* pAllocInfo,
    XGL_GPU_MEMORY*              pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglAllocMemory* pPacket;
    SEND_ENTRYPOINT_PARAMS("xglAllocMemory(device %p, AllocInfo (size %d, align %d, flags %d), pMem %p)\n",
        device, pAllocInfo->allocationSize, pAllocInfo->alignment, pAllocInfo->flags, pMem);
    CREATE_TRACE_PACKET(xglAllocMemory, sizeof(XGL_MEMORY_ALLOC_INFO) + sizeof(XGL_GPU_MEMORY));
    result = real_xglAllocMemory(device, pAllocInfo, pMem);
    pPacket = interpret_body_as_xglAllocMemory(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo), sizeof(XGL_MEMORY_ALLOC_INFO), pAllocInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(XGL_GPU_MEMORY), pMem);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader,(void**)&(pPacket->pAllocInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    add_new_handle_to_mem_info(*pMem, pAllocInfo->allocationSize, NULL);
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglFreeMemory(
    XGL_GPU_MEMORY mem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglFreeMemory* pPacket;
    SEND_ENTRYPOINT_PARAMS("xglFreeMemory( mem %p)\n", mem);
    CREATE_TRACE_PACKET(xglFreeMemory, 0);
    result = real_xglFreeMemory(mem);
    pPacket = interpret_body_as_xglFreeMemory(pHeader);
    pPacket->mem = mem;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    rm_handle_from_mem_info(mem);
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglSetMemoryPriority(
    XGL_GPU_MEMORY mem,
    XGL_MEMORY_PRIORITY       priority)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglSetMemoryPriority* pPacket;
    SEND_ENTRYPOINT_ID(xglSetMemoryPriority);
    CREATE_TRACE_PACKET(xglSetMemoryPriority, 0);
    result = real_xglSetMemoryPriority(mem, priority);
    pPacket = interpret_body_as_xglSetMemoryPriority(pHeader);
    pPacket->mem = mem;
    pPacket->priority = priority;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglMapMemory(
    XGL_GPU_MEMORY mem,
    XGL_FLAGS      flags,                // Reserved
    XGL_VOID**     ppData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglMapMemory* pPacket;
    SEND_ENTRYPOINT_PARAMS("xglMapMemory(mem %p, flags %u, ppData %p)\n", mem, flags, ppData);
    CREATE_TRACE_PACKET(xglMapMemory, sizeof(XGL_VOID*));
    result = real_xglMapMemory(mem, flags, ppData);
    pPacket = interpret_body_as_xglMapMemory(pHeader);
    pPacket->mem = mem;
    pPacket->flags = flags;
    if (ppData != NULL)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->ppData), sizeof(XGL_VOID*), *ppData);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->ppData));
        add_data_to_mem_info(mem, *ppData);
    }
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglUnmapMemory(
    XGL_GPU_MEMORY mem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglUnmapMemory* pPacket;
    XGLAllocInfo *entry;
    SEND_ENTRYPOINT_PARAMS("xglUnmapMemory(mem %p)\n", mem);
    // insert into packet the data that was written by CPU between the xglMapMemory call and here
    // Note must do this prior to the real xglUnMap() or else may get a FAULT
    entry = find_mem_info_entry(mem);
    CREATE_TRACE_PACKET(xglUnmapMemory, (entry) ? entry->size : 0);
    pPacket = interpret_body_as_xglUnmapMemory(pHeader);

    if (entry)
    {
        assert(entry->handle == mem);
        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pData), entry->size, entry->pData);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
        entry->pData = NULL;
    }
    result = real_xglUnmapMemory(mem);
    pPacket->mem = mem;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglPinSystemMemory(
    XGL_DEVICE      device,
    const XGL_VOID* pSysMem,
    XGL_SIZE        memSize,
    XGL_GPU_MEMORY* pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglPinSystemMemory* pPacket;
    SEND_ENTRYPOINT_ID(xglPinSystemMemory);
    CREATE_TRACE_PACKET(xglPinSystemMemory, sizeof(XGL_GPU_MEMORY));
    result = real_xglPinSystemMemory(device, pSysMem, memSize, pMem);
    pPacket = interpret_body_as_xglPinSystemMemory(pHeader);
    pPacket->device = device;
    pPacket->pSysMem = pSysMem;
    pPacket->memSize = memSize;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(XGL_GPU_MEMORY), pMem);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglRemapVirtualMemoryPages(
    XGL_DEVICE                            device,
    XGL_UINT                              rangeCount,
    const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pRanges,
    XGL_UINT                              preWaitSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*            pPreWaitSemaphores,
    XGL_UINT                              postSignalSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*            pPostSignalSemaphores)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglRemapVirtualMemoryPages* pPacket;
    SEND_ENTRYPOINT_ID(xglRemapVirtualMemoryPages);
    CREATE_TRACE_PACKET(xglRemapVirtualMemoryPages, rangeCount*sizeof(XGL_VIRTUAL_MEMORY_REMAP_RANGE) + preWaitSemaphoreCount*sizeof(XGL_QUEUE_SEMAPHORE) + postSignalSemaphoreCount*sizeof(XGL_QUEUE_SEMAPHORE));
    result = real_xglRemapVirtualMemoryPages(device, rangeCount, pRanges, preWaitSemaphoreCount, pPreWaitSemaphores, postSignalSemaphoreCount, pPostSignalSemaphores);
    pPacket = interpret_body_as_xglRemapVirtualMemoryPages(pHeader);
    pPacket->device = device;
    pPacket->rangeCount = rangeCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRanges), rangeCount*sizeof(XGL_VIRTUAL_MEMORY_REMAP_RANGE), pRanges);
    pPacket->preWaitSemaphoreCount = preWaitSemaphoreCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPreWaitSemaphores), preWaitSemaphoreCount*sizeof(XGL_QUEUE_SEMAPHORE), pPreWaitSemaphores);
    pPacket->postSignalSemaphoreCount = postSignalSemaphoreCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPostSignalSemaphores), postSignalSemaphoreCount*sizeof(XGL_QUEUE_SEMAPHORE), pPostSignalSemaphores);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRanges));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPreWaitSemaphores));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPostSignalSemaphores));
    FINISH_TRACE_PACKET();
    return result;
}

// Multi-device functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetMultiGpuCompatibility(
    XGL_PHYSICAL_GPU            gpu0,
    XGL_PHYSICAL_GPU            gpu1,
    XGL_GPU_COMPATIBILITY_INFO* pInfo)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetMultiGpuCompatibility* pPacket;
    SEND_ENTRYPOINT_ID(xglGetMultiGpuCompatibility);
    CREATE_TRACE_PACKET(xglGetMultiGpuCompatibility, sizeof(XGL_GPU_COMPATIBILITY_INFO));
    result = real_xglGetMultiGpuCompatibility(gpu0, gpu1, pInfo);
    pPacket = interpret_body_as_xglGetMultiGpuCompatibility(pHeader);
    pPacket->gpu0 = gpu0;
    pPacket->gpu1 = gpu1;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pInfo), sizeof(XGL_GPU_COMPATIBILITY_INFO), pInfo);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pInfo));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglOpenSharedMemory(
    XGL_DEVICE                  device,
    const XGL_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY*             pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglOpenSharedMemory* pPacket;
    SEND_ENTRYPOINT_ID(xglOpenSharedMemory);
    CREATE_TRACE_PACKET(xglOpenSharedMemory, sizeof(XGL_MEMORY_OPEN_INFO) + sizeof(XGL_GPU_MEMORY));
    result = real_xglOpenSharedMemory(device, pOpenInfo, pMem);
    pPacket = interpret_body_as_xglOpenSharedMemory(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOpenInfo), sizeof(XGL_MEMORY_OPEN_INFO), pOpenInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(XGL_GPU_MEMORY), pMem);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOpenInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglOpenSharedQueueSemaphore(
    XGL_DEVICE                           device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo,
    XGL_QUEUE_SEMAPHORE*                 pSemaphore)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglOpenSharedQueueSemaphore* pPacket;
    SEND_ENTRYPOINT_ID(xglOpenSharedQueueSemaphore);
    CREATE_TRACE_PACKET(xglOpenSharedQueueSemaphore, sizeof(XGL_QUEUE_SEMAPHORE_OPEN_INFO) + sizeof(XGL_QUEUE_SEMAPHORE));
    result = real_xglOpenSharedQueueSemaphore(device, pOpenInfo, pSemaphore);
    pPacket = interpret_body_as_xglOpenSharedQueueSemaphore(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOpenInfo), sizeof(XGL_QUEUE_SEMAPHORE_OPEN_INFO), pOpenInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSemaphore), sizeof(XGL_QUEUE_SEMAPHORE), pSemaphore);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOpenInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSemaphore));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglOpenPeerMemory(
    XGL_DEVICE                       device,
    const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY*                  pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglOpenPeerMemory* pPacket;
    SEND_ENTRYPOINT_ID(xglOpenPeerMemory);
    CREATE_TRACE_PACKET(xglOpenPeerMemory, sizeof(XGL_PEER_MEMORY_OPEN_INFO) + sizeof(XGL_GPU_MEMORY));
    result = real_xglOpenPeerMemory(device, pOpenInfo, pMem);
    pPacket = interpret_body_as_xglOpenPeerMemory(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOpenInfo), sizeof(XGL_PEER_MEMORY_OPEN_INFO), pOpenInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(XGL_GPU_MEMORY), pMem);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOpenInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglOpenPeerImage(
    XGL_DEVICE                      device,
    const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo,
    XGL_IMAGE*                      pImage,
    XGL_GPU_MEMORY*                 pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglOpenPeerImage* pPacket;
    SEND_ENTRYPOINT_ID(xglOpenPeerImage);
    CREATE_TRACE_PACKET(xglOpenPeerImage, sizeof(XGL_PEER_IMAGE_OPEN_INFO) + sizeof(XGL_IMAGE) + sizeof(XGL_GPU_MEMORY));
    result = real_xglOpenPeerImage(device, pOpenInfo, pImage, pMem);
    pPacket = interpret_body_as_xglOpenPeerImage(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pOpenInfo), sizeof(XGL_PEER_IMAGE_OPEN_INFO), pOpenInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pImage), sizeof(XGL_IMAGE), pImage);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(XGL_GPU_MEMORY), pMem);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pOpenInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImage));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    return result;
}

// Generic API object functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglDestroyObject(
    XGL_OBJECT object)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglDestroyObject* pPacket;
    SEND_ENTRYPOINT_ID(xglDestroyObject);
    CREATE_TRACE_PACKET(xglDestroyObject, 0);
    result = real_xglDestroyObject(object);
    pPacket = interpret_body_as_xglDestroyObject(pHeader);
    pPacket->object = object;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetObjectInfo(
    XGL_BASE_OBJECT             object,
    XGL_OBJECT_INFO_TYPE        infoType,
    XGL_SIZE*                   pDataSize,
    XGL_VOID*                   pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetObjectInfo* pPacket;
    uint64_t startTime;
    SEND_ENTRYPOINT_ID(xglGetObjectInfo);
    startTime = glv_get_time();
    result = real_xglGetObjectInfo(object, infoType, pDataSize, pData);
    // since don't know the size of *pDataSize potentially until after the call create trace packet post call
    CREATE_TRACE_PACKET(xglGetObjectInfo, ((pDataSize != NULL) ? sizeof(XGL_SIZE) : 0) + ((pDataSize != NULL)? *pDataSize : 0));
    pHeader->entrypoint_begin_time = startTime;
    pPacket = interpret_body_as_xglGetObjectInfo(pHeader);
    pPacket->object = object;
	pPacket->infoType = infoType;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(XGL_SIZE), pDataSize);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    if (pDataSize && pData)
    {
	    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), *pDataSize, pData);
        glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    }
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglBindObjectMemory(
    XGL_OBJECT     object,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglBindObjectMemory* pPacket;
    SEND_ENTRYPOINT_ID(xglBindObjectMemory);
    CREATE_TRACE_PACKET(xglBindObjectMemory, 0);
    result = real_xglBindObjectMemory(object, mem, offset);
    pPacket = interpret_body_as_xglBindObjectMemory(pHeader);
    pPacket->object = object;
    pPacket->mem = mem;
    pPacket->offset = offset;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// Fence functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateFence(
    XGL_DEVICE                   device,
    const XGL_FENCE_CREATE_INFO* pCreateInfo,
    XGL_FENCE*                   pFence)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateFence* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateFence);
    CREATE_TRACE_PACKET(xglCreateFence, sizeof(XGL_FENCE_CREATE_INFO) + sizeof(XGL_FENCE));
    result = real_xglCreateFence(device, pCreateInfo, pFence);
    pPacket = interpret_body_as_xglCreateFence(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_FENCE_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFence), sizeof(XGL_FENCE), pFence);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pFence));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetFenceStatus(
    XGL_FENCE fence)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetFenceStatus* pPacket;
    SEND_ENTRYPOINT_ID(xglGetFenceStatus);
    CREATE_TRACE_PACKET(xglGetFenceStatus, 0);
    result = real_xglGetFenceStatus(fence);
    pPacket = interpret_body_as_xglGetFenceStatus(pHeader);
    pPacket->fence = fence;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWaitForFences(
    XGL_DEVICE       device,
    XGL_UINT         fenceCount,
    const XGL_FENCE* pFences,
    XGL_BOOL         waitAll,
    XGL_UINT64       timeout)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWaitForFences* pPacket;
    SEND_ENTRYPOINT_ID(xglWaitForFences);
    CREATE_TRACE_PACKET(xglWaitForFences, fenceCount*sizeof(XGL_FENCE));
    result = real_xglWaitForFences(device, fenceCount, pFences, waitAll, timeout);
    pPacket = interpret_body_as_xglWaitForFences(pHeader);
    pPacket->device = device;
    pPacket->fenceCount = fenceCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFences), fenceCount*sizeof(XGL_FENCE), pFences);
    pPacket->waitAll = waitAll;
    pPacket->timeout = timeout;
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pFences));
    FINISH_TRACE_PACKET();
    return result;
}

// Queue semaphore functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateQueueSemaphore(
    XGL_DEVICE                             device,
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo,
    XGL_QUEUE_SEMAPHORE*                   pSemaphore)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateQueueSemaphore* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateQueueSemaphore);
    CREATE_TRACE_PACKET(xglCreateQueueSemaphore, sizeof(XGL_QUEUE_SEMAPHORE_CREATE_INFO) + sizeof(XGL_QUEUE_SEMAPHORE));
    result = real_xglCreateQueueSemaphore(device, pCreateInfo, pSemaphore);
    pPacket = interpret_body_as_xglCreateQueueSemaphore(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_QUEUE_SEMAPHORE_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSemaphore), sizeof(XGL_QUEUE_SEMAPHORE), pSemaphore);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSemaphore));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglSignalQueueSemaphore(
    XGL_QUEUE           queue,
    XGL_QUEUE_SEMAPHORE semaphore)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglSignalQueueSemaphore* pPacket;
    SEND_ENTRYPOINT_ID(xglSignalQueueSemaphore);
    CREATE_TRACE_PACKET(xglSignalQueueSemaphore, 0);
    result = real_xglSignalQueueSemaphore(queue, semaphore);
    pPacket = interpret_body_as_xglSignalQueueSemaphore(pHeader);
    pPacket->queue = queue;
    pPacket->semaphore = semaphore;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWaitQueueSemaphore(
    XGL_QUEUE           queue,
    XGL_QUEUE_SEMAPHORE semaphore)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWaitQueueSemaphore* pPacket;
    SEND_ENTRYPOINT_ID(xglWaitQueueSemaphore);
    CREATE_TRACE_PACKET(xglWaitQueueSemaphore, 0);
    result = real_xglWaitQueueSemaphore(queue, semaphore);
    pPacket = interpret_body_as_xglWaitQueueSemaphore(pHeader);
    pPacket->queue = queue;
    pPacket->semaphore = semaphore;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// Event functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateEvent(
    XGL_DEVICE                   device,
    const XGL_EVENT_CREATE_INFO* pCreateInfo,
    XGL_EVENT*                   pEvent)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateEvent* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateEvent);
    CREATE_TRACE_PACKET(xglCreateEvent, sizeof(XGL_EVENT_CREATE_INFO) + sizeof(XGL_EVENT));
    result = real_xglCreateEvent(device, pCreateInfo, pEvent);
    pPacket = interpret_body_as_xglCreateEvent(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_EVENT_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pEvent), sizeof(XGL_EVENT), pEvent);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pEvent));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetEventStatus(
    XGL_EVENT event)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetEventStatus* pPacket;
    SEND_ENTRYPOINT_ID(xglGetEventStatus);
    CREATE_TRACE_PACKET(xglGetEventStatus, 0);
    result = real_xglGetEventStatus(event);
    pPacket = interpret_body_as_xglGetEventStatus(pHeader);
    pPacket->event = event;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglSetEvent(
    XGL_EVENT event)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglSetEvent* pPacket;
    SEND_ENTRYPOINT_ID(xglSetEvent);
    CREATE_TRACE_PACKET(xglSetEvent, 0);
    result = real_xglSetEvent(event);
    pPacket = interpret_body_as_xglSetEvent(pHeader);
    pPacket->event = event;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglResetEvent(
    XGL_EVENT event)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglResetEvent* pPacket;
    SEND_ENTRYPOINT_ID(xglResetEvent);
    CREATE_TRACE_PACKET(xglResetEvent, 0);
    result = real_xglResetEvent(event);
    pPacket = interpret_body_as_xglResetEvent(pHeader);
    pPacket->event = event;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// Query functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateQueryPool(
    XGL_DEVICE                        device,
    const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo,
    XGL_QUERY_POOL*                   pQueryPool)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateQueryPool* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateQueryPool);
    CREATE_TRACE_PACKET(xglCreateQueryPool, sizeof(XGL_QUERY_POOL) + sizeof(XGL_QUERY_POOL_CREATE_INFO));
    result = real_xglCreateQueryPool(device, pCreateInfo, pQueryPool);
    pPacket = interpret_body_as_xglCreateQueryPool(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_QUERY_POOL_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pQueryPool), sizeof(XGL_QUERY_POOL), pQueryPool);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pQueryPool));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetQueryPoolResults(
    XGL_QUERY_POOL queryPool,
    XGL_UINT       startQuery,
    XGL_UINT       queryCount,
    XGL_SIZE*      pDataSize,
    XGL_VOID*      pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetQueryPoolResults* pPacket;
    SEND_ENTRYPOINT_ID(xglGetQueryPoolResults);
    CREATE_TRACE_PACKET(xglGetQueryPoolResults, ((pDataSize != NULL ) ? sizeof(XGL_SIZE) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglGetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    pPacket = interpret_body_as_xglGetQueryPoolResults(pHeader);
    pPacket->queryPool = queryPool;
    pPacket->startQuery = startQuery;
    pPacket->queryCount = queryCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(XGL_SIZE), pDataSize);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), (pDataSize != NULL && pData != NULL) ? *pDataSize : 0, pData);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

// Format capabilities

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetFormatInfo(
    XGL_DEVICE             device,
    XGL_FORMAT             format,
    XGL_FORMAT_INFO_TYPE   infoType,
    XGL_SIZE*              pDataSize,
    XGL_VOID*              pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetFormatInfo* pPacket;
    SEND_ENTRYPOINT_ID(xglGetFormatInfo);
    CREATE_TRACE_PACKET(xglGetFormatInfo, ((pDataSize != NULL ) ? sizeof(XGL_SIZE) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglGetFormatInfo(device, format, infoType, pDataSize, pData);
    pPacket = interpret_body_as_xglGetFormatInfo(pHeader);
    pPacket->device = device;
    pPacket->format = format;
    pPacket->infoType = infoType;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(XGL_SIZE), pDataSize);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), (pDataSize != NULL && pData != NULL) ? *pDataSize : 0, pData);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

// Image functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateImage(
    XGL_DEVICE                   device,
    const XGL_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE*                   pImage)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateImage* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateImage);
    CREATE_TRACE_PACKET(xglCreateImage, sizeof(XGL_IMAGE_CREATE_INFO) + sizeof(XGL_IMAGE));
    result = real_xglCreateImage(device, pCreateInfo, pImage);
    pPacket = interpret_body_as_xglCreateImage(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_IMAGE_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pImage), sizeof(XGL_IMAGE), pImage);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImage));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetImageSubresourceInfo(
    XGL_IMAGE                    image,
    const XGL_IMAGE_SUBRESOURCE* pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE    infoType,
    XGL_SIZE*                    pDataSize,
    XGL_VOID*                    pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetImageSubresourceInfo* pPacket;
    SEND_ENTRYPOINT_ID(xglGetImageSubresourceInfo);
    CREATE_TRACE_PACKET(xglGetImageSubresourceInfo, ((pSubresource != NULL) ? sizeof(XGL_IMAGE_SUBRESOURCE) : 0) + ((pDataSize != NULL ) ? sizeof(XGL_SIZE) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglGetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    pPacket = interpret_body_as_xglGetImageSubresourceInfo(pHeader);
    pPacket->image = image;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSubresource), sizeof(XGL_IMAGE_SUBRESOURCE), pSubresource);
    pPacket->infoType = infoType;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(XGL_SIZE), pDataSize);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), (pDataSize != NULL && pData != NULL) ? *pDataSize : 0, pData);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSubresource));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

// Image view functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateImageView(
    XGL_DEVICE                        device,
    const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
    XGL_IMAGE_VIEW*                   pView)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateImageView* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateImageView);
    CREATE_TRACE_PACKET(xglCreateImageView, sizeof(XGL_IMAGE_VIEW_CREATE_INFO) + sizeof(XGL_IMAGE_VIEW));
    result = real_xglCreateImageView(device, pCreateInfo, pView);
    pPacket = interpret_body_as_xglCreateImageView(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_IMAGE_VIEW_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pView), sizeof(XGL_IMAGE_VIEW), pView);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pView));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateColorAttachmentView(
    XGL_DEVICE                               device,
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW*                   pView)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateColorAttachmentView* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateColorAttachmentView);
    CREATE_TRACE_PACKET(xglCreateColorAttachmentView, sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO)+sizeof(XGL_COLOR_ATTACHMENT_VIEW));
    result = real_xglCreateColorAttachmentView(device, pCreateInfo, pView);
    pPacket = interpret_body_as_xglCreateColorAttachmentView(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pView), sizeof(XGL_COLOR_ATTACHMENT_VIEW), pView);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pView));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDepthStencilView(
    XGL_DEVICE                                device,
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo,
    XGL_DEPTH_STENCIL_VIEW*                   pView)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDepthStencilView* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateDepthStencilView);
    CREATE_TRACE_PACKET(xglCreateDepthStencilView, sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO) + sizeof(XGL_DEPTH_STENCIL_VIEW));
    result = real_xglCreateDepthStencilView(device, pCreateInfo, pView);
    pPacket = interpret_body_as_xglCreateDepthStencilView(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pView), sizeof(XGL_DEPTH_STENCIL_VIEW), pView);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pView));
    FINISH_TRACE_PACKET();
    return result;
}

// Shader functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateShader(
    XGL_DEVICE                    device,
    const XGL_SHADER_CREATE_INFO* pCreateInfo,
    XGL_SHADER*                   pShader)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateShader* pPacket;
    size_t shaderCodeSize;
    SEND_ENTRYPOINT_ID(xglCreateShader);
    shaderCodeSize = (pCreateInfo != NULL) ? pCreateInfo->codeSize : 0;
    CREATE_TRACE_PACKET(xglCreateShader, sizeof(XGL_SHADER_CREATE_INFO) + sizeof(XGL_SHADER) + shaderCodeSize);
    result = real_xglCreateShader(device, pCreateInfo, pShader);
    pPacket = interpret_body_as_xglCreateShader(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_SHADER_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pCode), shaderCodeSize, pCreateInfo->pCode);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pShader), sizeof(XGL_SHADER), pShader);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pCode));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pShader));
    FINISH_TRACE_PACKET();
    return result;
}

// Pipeline functions

static size_t calculate_pipeline_shader_size(const XGL_PIPELINE_SHADER* shader)
{
    size_t size = 0;
    XGL_UINT i, j;
    
    size += sizeof(XGL_PIPELINE_SHADER);
    // descriptor sets
    for (i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++)
    {
        for (j = 0; j < shader->descriptorSetMapping[i].descriptorCount; j++)
        {
            size += sizeof(XGL_DESCRIPTOR_SLOT_INFO);
            if (shader->descriptorSetMapping[i].pDescriptorInfo[j].slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET)
            {
                size += sizeof(XGL_DESCRIPTOR_SET_MAPPING);
            }
        }
    }

    // constant buffers
    if (shader->linkConstBufferCount > 0 && shader->pLinkConstBufferInfo != NULL)
    {
        XGL_UINT i;
        for (i = 0; i < shader->linkConstBufferCount; i++)
        {
            size += sizeof(XGL_LINK_CONST_BUFFER);
            size += shader->pLinkConstBufferInfo[i].bufferSize;
        }
    }
    return size;
}

static void add_pipeline_shader_to_trace_packet(glv_trace_packet_header* pHeader, XGL_PIPELINE_SHADER* packetShader, const XGL_PIPELINE_SHADER* paramShader)
{
    XGL_UINT i, j;
    // descriptor sets
    for (i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->descriptorSetMapping[i].pDescriptorInfo), sizeof(XGL_DESCRIPTOR_SLOT_INFO)* paramShader->descriptorSetMapping[i].descriptorCount, paramShader->descriptorSetMapping[i].pDescriptorInfo);
        for (j = 0; j < paramShader->descriptorSetMapping[i].descriptorCount; j++)
        {
            if (paramShader->descriptorSetMapping[i].pDescriptorInfo[j].slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET)
            {
                glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->descriptorSetMapping[i].pDescriptorInfo[j].pNextLevelSet), sizeof(XGL_DESCRIPTOR_SET_MAPPING), paramShader->descriptorSetMapping[i].pDescriptorInfo[j].pNextLevelSet);
            }
        }
        packetShader->descriptorSetMapping[i].descriptorCount = paramShader->descriptorSetMapping[i].descriptorCount;
    }

    // constant buffers
    if (paramShader->linkConstBufferCount > 0 && paramShader->pLinkConstBufferInfo != NULL)
    {
        glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->pLinkConstBufferInfo), sizeof(XGL_LINK_CONST_BUFFER) * paramShader->linkConstBufferCount, paramShader->pLinkConstBufferInfo);
        for (i = 0; i < paramShader->linkConstBufferCount; i++)
        {
            glv_add_buffer_to_trace_packet(pHeader, (void**)&(packetShader->pLinkConstBufferInfo[i].pBufferData), packetShader->pLinkConstBufferInfo[i].bufferSize, paramShader->pLinkConstBufferInfo[i].pBufferData);
        }
    }
}

static void finalize_pipeline_shader_address(glv_trace_packet_header* pHeader, const XGL_PIPELINE_SHADER* packetShader)
{
    XGL_UINT i, j;
    // descriptor sets
    for (i = 0; i < XGL_MAX_DESCRIPTOR_SETS; i++)
    {
        for (j = 0; j < packetShader->descriptorSetMapping[i].descriptorCount; j++)
        {
            if (packetShader->descriptorSetMapping[i].pDescriptorInfo[j].slotObjectType == XGL_SLOT_NEXT_DESCRIPTOR_SET)
            {
                glv_finalize_buffer_address(pHeader, (void**)&(packetShader->descriptorSetMapping[i].pDescriptorInfo[j].pNextLevelSet));
            }
        }
        glv_finalize_buffer_address(pHeader, (void**)&(packetShader->descriptorSetMapping[i].pDescriptorInfo));
    }

    // constant buffers
    if (packetShader->linkConstBufferCount > 0 && packetShader->pLinkConstBufferInfo != NULL)
    {
        for (i = 0; i < packetShader->linkConstBufferCount; i++)
        {
            glv_finalize_buffer_address(pHeader, (void**)&(packetShader->pLinkConstBufferInfo[i].pBufferData));
        }
        glv_finalize_buffer_address(pHeader, (void**)&(packetShader->pLinkConstBufferInfo));
    }
}

static size_t calculate_pipeline_state_size(const XGL_VOID* pState)
{
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pNext = pState;
    size_t totalStateSize = 0;
    while (pNext)
    {
        switch (pNext->sType)
        {
            //case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
            //    totalStateSize += sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO);
            //    break;
            case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
                totalStateSize += sizeof(XGL_PIPELINE_IA_STATE_CREATE_INFO);
                break;
            case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
                totalStateSize += sizeof(XGL_PIPELINE_TESS_STATE_CREATE_INFO);
                break;
            case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
                totalStateSize += sizeof(XGL_PIPELINE_RS_STATE_CREATE_INFO);
                break;
            case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:
                totalStateSize += sizeof(XGL_PIPELINE_DB_STATE_CREATE_INFO);
                break;
            case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
                totalStateSize += sizeof(XGL_PIPELINE_CB_STATE);
                break;
            case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            {
                const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pShaderStage = (const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext;
                totalStateSize += (sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO) + calculate_pipeline_shader_size(&pShaderStage->shader));
                break;
            }
            //case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
            //    totalStateSize += sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO);
            //   break;
            default:
                assert(0);
        }
        pNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pNext->pNext;
    }
    return totalStateSize;
}

static void add_pipeline_state_to_trace_packet(glv_trace_packet_header* pHeader, XGL_VOID** ppOut, const XGL_VOID* pIn)
{
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pInNow = pIn;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO** ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)ppOut;
    while (pInNow != NULL)
    {
        XGL_GRAPHICS_PIPELINE_CREATE_INFO** ppOutNow = ppOutNext;
        ppOutNext = NULL;

        switch (pInNow->sType)
        {
            case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
            {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_IA_STATE_CREATE_INFO), pInNow);
                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
            {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_TESS_STATE_CREATE_INFO), pInNow);
                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
            {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_RS_STATE_CREATE_INFO), pInNow);
                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:
            {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_DB_STATE_CREATE_INFO), pInNow);
                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            {
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_CB_STATE), pInNow);
                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            {
                XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pPacket = NULL;
                XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pInPacket = NULL;
                glv_add_buffer_to_trace_packet(pHeader, (void**)(ppOutNow), sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO), pInNow);
                pPacket = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) *ppOutNow;
                pInPacket = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*) pInNow;
                add_pipeline_shader_to_trace_packet(pHeader, &pPacket->shader, &pInPacket->shader);
                finalize_pipeline_shader_address(pHeader, &pPacket->shader);
                ppOutNext = (XGL_GRAPHICS_PIPELINE_CREATE_INFO**)&(*ppOutNow)->pNext;
                glv_finalize_buffer_address(pHeader, (void**)(ppOutNow));
                break;
            }
            default:
                assert(!"Encountered an unexpected type in pipeline state list");
        }
        pInNow = (XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pInNow->pNext;
    }
    return;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateGraphicsPipeline(
    XGL_DEVICE                               device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE*                            pPipeline)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    size_t stateSize;
    struct_xglCreateGraphicsPipeline* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateGraphicsPipeline);
    stateSize = calculate_pipeline_state_size(pCreateInfo->pNext);
    CREATE_TRACE_PACKET(xglCreateGraphicsPipeline, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO) + sizeof(XGL_PIPELINE) + stateSize);
    result = real_xglCreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    pPacket = interpret_body_as_xglCreateGraphicsPipeline(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO), pCreateInfo);
    add_pipeline_state_to_trace_packet(pHeader, (XGL_VOID**)&pPacket->pCreateInfo->pNext, pCreateInfo->pNext);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipeline), sizeof(XGL_PIPELINE), pPipeline);
    pPacket->result = result;
    // finalize the addresses in the pipeline state list is done in add_pipeline_state_to_trace_packet()
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipeline));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateComputePipeline(
    XGL_DEVICE                              device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE*                           pPipeline)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    size_t stateSize;
    struct_xglCreateComputePipeline* pPacket = NULL;
    SEND_ENTRYPOINT_ID(xglCreateComputePipeline);
    stateSize = calculate_pipeline_state_size(pCreateInfo->pNext);
    CREATE_TRACE_PACKET(xglCreateComputePipeline, sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO) + sizeof(XGL_PIPELINE) + stateSize + calculate_pipeline_shader_size(&pCreateInfo->cs));
    result = real_xglCreateComputePipeline(device, pCreateInfo, pPipeline);
    pPacket = interpret_body_as_xglCreateComputePipeline(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO), pCreateInfo);
    add_pipeline_state_to_trace_packet(pHeader, (XGL_VOID**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext);
    add_pipeline_shader_to_trace_packet(pHeader, (XGL_PIPELINE_SHADER*)&pPacket->pCreateInfo->cs, &pCreateInfo->cs);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipeline), sizeof(XGL_PIPELINE), pPipeline);
    pPacket->result = result;
    // finalize the addresses in the pipeline state list is done in add_pipeline_state_to_trace_packet()
    finalize_pipeline_shader_address(pHeader, &pPacket->pCreateInfo->cs);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipeline));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglStorePipeline(
    XGL_PIPELINE pipeline,
    XGL_SIZE*    pDataSize,
    XGL_VOID*    pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglStorePipeline* pPacket;
    SEND_ENTRYPOINT_ID(xglStorePipeline);
    CREATE_TRACE_PACKET(xglStorePipeline, sizeof(XGL_SIZE) + ((pDataSize != NULL) ? *pDataSize : 0));
    result = real_xglStorePipeline(pipeline, pDataSize, pData);
    pPacket = interpret_body_as_xglStorePipeline(pHeader);
    pPacket->pipeline = pipeline;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(XGL_SIZE), pDataSize);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), (pDataSize != NULL) ? *pDataSize : 0, pData);
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglLoadPipeline(
    XGL_DEVICE      device,
    XGL_SIZE        dataSize,
    const XGL_VOID* pData,
    XGL_PIPELINE*   pPipeline)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglLoadPipeline* pPacket;
    SEND_ENTRYPOINT_ID(xglLoadPipeline);
    CREATE_TRACE_PACKET(xglLoadPipeline, dataSize + sizeof(XGL_PIPELINE));
    result = real_xglLoadPipeline(device, dataSize, pData, pPipeline);
    pPacket = interpret_body_as_xglLoadPipeline(pHeader);
    pPacket->device = device;
    pPacket->dataSize = dataSize;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), dataSize, pData);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipeline), sizeof(XGL_PIPELINE), pPipeline);
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipeline));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreatePipelineDelta(
    XGL_DEVICE      device,
    XGL_PIPELINE    p1,
    XGL_PIPELINE    p2,
    XGL_PIPELINE_DELTA* delta)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreatePipelineDelta* pPacket;
    SEND_ENTRYPOINT_ID(xglCreatePipelineDelta);
    CREATE_TRACE_PACKET(xglCreatePipelineDelta, sizeof(XGL_PIPELINE_DELTA));
    result = real_xglCreatePipelineDelta(device, p1, p2, delta);
    pPacket = interpret_body_as_xglCreatePipelineDelta(pHeader);
    pPacket->device = device;
    pPacket->p1 = p1;
    pPacket->p2 = p2;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->delta), sizeof(XGL_PIPELINE_DELTA), delta);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->delta));
    FINISH_TRACE_PACKET();
    return result;
}

// Sampler functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateSampler(
    XGL_DEVICE                     device,
    const XGL_SAMPLER_CREATE_INFO* pCreateInfo,
    XGL_SAMPLER*                   pSampler)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateSampler* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateSampler);
    CREATE_TRACE_PACKET(xglCreateSampler, sizeof(XGL_SAMPLER_CREATE_INFO) + sizeof(XGL_SAMPLER));
    result = real_xglCreateSampler(device, pCreateInfo, pSampler);
    pPacket = interpret_body_as_xglCreateSampler(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_SAMPLER_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSampler), sizeof(XGL_SAMPLER), pSampler);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSampler));
    FINISH_TRACE_PACKET();
    return result;
}

// Descriptor set functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDescriptorSet(
    XGL_DEVICE                            device,
    const XGL_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo,
    XGL_DESCRIPTOR_SET*                   pDescriptorSet)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDescriptorSet* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateDescriptorSet);
    CREATE_TRACE_PACKET(xglCreateDescriptorSet, sizeof(XGL_DESCRIPTOR_SET_CREATE_INFO) + sizeof(XGL_DESCRIPTOR_SET));
    result = real_xglCreateDescriptorSet(device, pCreateInfo, pDescriptorSet);
    pPacket = interpret_body_as_xglCreateDescriptorSet(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DESCRIPTOR_SET_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSet), sizeof(XGL_DESCRIPTOR_SET), pDescriptorSet);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSet));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglBeginDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET descriptorSet)
{
    glv_trace_packet_header* pHeader;
    struct_xglBeginDescriptorSetUpdate* pPacket;
    SEND_ENTRYPOINT_ID(xglBeginDescriptorSetUpdate);
    CREATE_TRACE_PACKET(xglBeginDescriptorSetUpdate, 0);
    real_xglBeginDescriptorSetUpdate(descriptorSet);
    pPacket = interpret_body_as_xglBeginDescriptorSetUpdate(pHeader);
    pPacket->descriptorSet = descriptorSet;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglEndDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET descriptorSet)
{
    glv_trace_packet_header* pHeader;
    struct_xglEndDescriptorSetUpdate* pPacket;
    SEND_ENTRYPOINT_ID(xglEndDescriptorSetUpdate);
    CREATE_TRACE_PACKET(xglEndDescriptorSetUpdate, 0);
    real_xglEndDescriptorSetUpdate(descriptorSet);
    pPacket = interpret_body_as_xglEndDescriptorSetUpdate(pHeader);
    pPacket->descriptorSet = descriptorSet;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglAttachSamplerDescriptors(
    XGL_DESCRIPTOR_SET descriptorSet,
    XGL_UINT           startSlot,
    XGL_UINT           slotCount,
    const XGL_SAMPLER* pSamplers)
{
    glv_trace_packet_header* pHeader;
    struct_xglAttachSamplerDescriptors* pPacket;
    SEND_ENTRYPOINT_ID(xglAttachSamplerDescriptors);
    CREATE_TRACE_PACKET(xglAttachSamplerDescriptors, slotCount*sizeof(XGL_SAMPLER));
    real_xglAttachSamplerDescriptors(descriptorSet, startSlot, slotCount, pSamplers);
    pPacket = interpret_body_as_xglAttachSamplerDescriptors(pHeader);
    pPacket->descriptorSet = descriptorSet;
    pPacket->startSlot = startSlot;
	pPacket->slotCount = slotCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSamplers), slotCount*sizeof(XGL_SAMPLER), pSamplers);
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSamplers));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglAttachImageViewDescriptors(
    XGL_DESCRIPTOR_SET                descriptorSet,
    XGL_UINT                          startSlot,
    XGL_UINT                          slotCount,
    const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews)
{
    glv_trace_packet_header* pHeader;
    struct_xglAttachImageViewDescriptors* pPacket;
    SEND_ENTRYPOINT_ID(xglAttachImageViewDescriptors);
    CREATE_TRACE_PACKET(xglAttachImageViewDescriptors, slotCount*sizeof(XGL_IMAGE_VIEW_ATTACH_INFO));
    real_xglAttachImageViewDescriptors(descriptorSet, startSlot, slotCount, pImageViews);
    pPacket = interpret_body_as_xglAttachImageViewDescriptors(pHeader);
    pPacket->descriptorSet = descriptorSet;
    pPacket->startSlot = startSlot;
	pPacket->slotCount = slotCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pImageViews), slotCount*sizeof(XGL_IMAGE_VIEW_ATTACH_INFO), pImageViews);
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImageViews));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglAttachMemoryViewDescriptors(
    XGL_DESCRIPTOR_SET                 descriptorSet,
    XGL_UINT                           startSlot,
    XGL_UINT                           slotCount,
    const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews)
{
    glv_trace_packet_header* pHeader;
    struct_xglAttachMemoryViewDescriptors* pPacket;
    SEND_ENTRYPOINT_ID(xglAttachMemoryViewDescriptors);
    CREATE_TRACE_PACKET(xglAttachMemoryViewDescriptors, slotCount*sizeof(XGL_MEMORY_VIEW_ATTACH_INFO));
    real_xglAttachMemoryViewDescriptors(descriptorSet, startSlot, slotCount, pMemViews);
    pPacket = interpret_body_as_xglAttachMemoryViewDescriptors(pHeader);
    pPacket->descriptorSet = descriptorSet;
    pPacket->startSlot = startSlot;
	pPacket->slotCount = slotCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMemViews), slotCount*sizeof(XGL_MEMORY_VIEW_ATTACH_INFO), pMemViews);
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemViews));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglAttachNestedDescriptors(
    XGL_DESCRIPTOR_SET                    descriptorSet,
    XGL_UINT                              startSlot,
    XGL_UINT                              slotCount,
    const XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets)
{
    glv_trace_packet_header* pHeader;
    struct_xglAttachNestedDescriptors* pPacket;
    SEND_ENTRYPOINT_ID(xglAttachNestedDescriptors);
    CREATE_TRACE_PACKET(xglAttachNestedDescriptors, slotCount*sizeof(XGL_DESCRIPTOR_SET_ATTACH_INFO));
    real_xglAttachNestedDescriptors(descriptorSet, startSlot, slotCount, pNestedDescriptorSets);
    pPacket = interpret_body_as_xglAttachNestedDescriptors(pHeader);
    pPacket->descriptorSet = descriptorSet;
    pPacket->startSlot = startSlot;
	pPacket->slotCount = slotCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pNestedDescriptorSets), slotCount*sizeof(XGL_DESCRIPTOR_SET_ATTACH_INFO), pNestedDescriptorSets);
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pNestedDescriptorSets));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglClearDescriptorSetSlots(
    XGL_DESCRIPTOR_SET descriptorSet,
    XGL_UINT           startSlot,
    XGL_UINT           slotCount)
{
    glv_trace_packet_header* pHeader;
    struct_xglClearDescriptorSetSlots* pPacket;
    SEND_ENTRYPOINT_ID(xglClearDescriptorSetSlots);
    CREATE_TRACE_PACKET(xglClearDescriptorSetSlots, 0);
    real_xglClearDescriptorSetSlots(descriptorSet, startSlot, slotCount);
    pPacket = interpret_body_as_xglClearDescriptorSetSlots(pHeader);
    pPacket->descriptorSet = descriptorSet;
    pPacket->startSlot = startSlot;
    pPacket->slotCount = slotCount;
    FINISH_TRACE_PACKET();
}

// State object functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateViewportState(
    XGL_DEVICE                            device,
    const XGL_VIEWPORT_STATE_CREATE_INFO* pCreateInfo,
    XGL_VIEWPORT_STATE_OBJECT*            pState)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateViewportState* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateViewportState);
    CREATE_TRACE_PACKET(xglCreateViewportState, sizeof(XGL_VIEWPORT_STATE_CREATE_INFO) + sizeof(XGL_VIEWPORT_STATE_OBJECT));
    result = real_xglCreateViewportState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_xglCreateViewportState(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_VIEWPORT_STATE_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(XGL_VIEWPORT_STATE_OBJECT), pState);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateRasterState(
    XGL_DEVICE                          device,
    const XGL_RASTER_STATE_CREATE_INFO* pCreateInfo,
    XGL_RASTER_STATE_OBJECT*            pState)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateRasterState* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateRasterState);
    CREATE_TRACE_PACKET(xglCreateRasterState, sizeof(XGL_RASTER_STATE_CREATE_INFO) + sizeof(XGL_RASTER_STATE_OBJECT));
    result = real_xglCreateRasterState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_xglCreateRasterState(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_RASTER_STATE_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(XGL_RASTER_STATE_OBJECT), pState);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateMsaaState(
    XGL_DEVICE                        device,
    const XGL_MSAA_STATE_CREATE_INFO* pCreateInfo,
    XGL_MSAA_STATE_OBJECT*            pState)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateMsaaState* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateMsaaState);
    CREATE_TRACE_PACKET(xglCreateMsaaState, sizeof(XGL_MSAA_STATE_CREATE_INFO) + sizeof(XGL_MSAA_STATE_OBJECT));
    result = real_xglCreateMsaaState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_xglCreateMsaaState(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_MSAA_STATE_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(XGL_MSAA_STATE_OBJECT), pState);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
	FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateColorBlendState(
    XGL_DEVICE                               device,
    const XGL_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo,
    XGL_COLOR_BLEND_STATE_OBJECT*            pState)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateColorBlendState* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateColorBlendState);
    CREATE_TRACE_PACKET(xglCreateColorBlendState, sizeof(XGL_COLOR_BLEND_STATE_CREATE_INFO) + sizeof(XGL_COLOR_BLEND_STATE_OBJECT));
    result = real_xglCreateColorBlendState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_xglCreateColorBlendState(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_COLOR_BLEND_STATE_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(XGL_COLOR_BLEND_STATE_OBJECT), pState);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDepthStencilState(
    XGL_DEVICE                                 device,
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo,
    XGL_DEPTH_STENCIL_STATE_OBJECT*            pState)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDepthStencilState* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateDepthStencilState);
    CREATE_TRACE_PACKET(xglCreateDepthStencilState, sizeof(XGL_DEPTH_STENCIL_STATE_CREATE_INFO) + sizeof(XGL_DEPTH_STENCIL_STATE_OBJECT));
    result = real_xglCreateDepthStencilState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_xglCreateDepthStencilState(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DEPTH_STENCIL_STATE_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(XGL_DEPTH_STENCIL_STATE_OBJECT), pState);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

// Command buffer functions

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateCommandBuffer(
    XGL_DEVICE                        device,
    const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo,
    XGL_CMD_BUFFER*                   pCmdBuffer)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateCommandBuffer* pPacket;
    SEND_ENTRYPOINT_ID(xglCreateCommandBuffer);
    CREATE_TRACE_PACKET(xglCreateCommandBuffer, sizeof(XGL_CMD_BUFFER_CREATE_INFO) + sizeof(XGL_CMD_BUFFER));
    result = real_xglCreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    pPacket = interpret_body_as_xglCreateCommandBuffer(pHeader);
    pPacket->device = device;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_CMD_BUFFER_CREATE_INFO), pCreateInfo);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCmdBuffer), sizeof(XGL_CMD_BUFFER), pCmdBuffer);
    pPacket->result = result;
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCmdBuffer));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglBeginCommandBuffer(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_FLAGS      flags)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglBeginCommandBuffer* pPacket;
    SEND_ENTRYPOINT_ID(xglBeginCommandBuffer);
    CREATE_TRACE_PACKET(xglBeginCommandBuffer, 0);
    result = real_xglBeginCommandBuffer(cmdBuffer, flags);
    pPacket = interpret_body_as_xglBeginCommandBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->flags = flags;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglEndCommandBuffer(
    XGL_CMD_BUFFER cmdBuffer)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglEndCommandBuffer* pPacket;
    SEND_ENTRYPOINT_ID(xglEndCommandBuffer);
    CREATE_TRACE_PACKET(xglEndCommandBuffer, 0);
    result = real_xglEndCommandBuffer(cmdBuffer);
    pPacket = interpret_body_as_xglEndCommandBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglResetCommandBuffer(
    XGL_CMD_BUFFER cmdBuffer)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglResetCommandBuffer* pPacket;
    SEND_ENTRYPOINT_ID(xglResetCommandBuffer);
    CREATE_TRACE_PACKET(xglResetCommandBuffer, 0);
    result = real_xglResetCommandBuffer(cmdBuffer);
    pPacket = interpret_body_as_xglResetCommandBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->result = result;
    return result;
}

// Command buffer building functions

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdBindPipeline(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_PIPELINE                  pipeline)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindPipeline* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdBindPipeline);
    CREATE_TRACE_PACKET(xglCmdBindPipeline, 0);
    real_xglCmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
    pPacket = interpret_body_as_xglCmdBindPipeline(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->pipeline = pipeline;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdBindPipelineDelta(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_PIPELINE_DELTA            delta)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindPipelineDelta* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdBindPipelineDelta);
    CREATE_TRACE_PACKET(xglCmdBindPipelineDelta, 0);
    real_xglCmdBindPipelineDelta(cmdBuffer, pipelineBindPoint, delta);
    pPacket = interpret_body_as_xglCmdBindPipelineDelta(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->delta = delta;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdBindStateObject(
    XGL_CMD_BUFFER                   cmdBuffer,
    XGL_STATE_BIND_POINT             stateBindPoint,
    XGL_STATE_OBJECT                 state)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindStateObject* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdBindStateObject);
    CREATE_TRACE_PACKET(xglCmdBindStateObject, 0);
    real_xglCmdBindStateObject(cmdBuffer, stateBindPoint, state);
    pPacket = interpret_body_as_xglCmdBindStateObject(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->stateBindPoint = stateBindPoint;
    pPacket->state = state;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdBindDescriptorSet(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_PIPELINE_BIND_POINT            pipelineBindPoint,
    XGL_UINT                           index,
    XGL_DESCRIPTOR_SET                 descriptorSet,
    XGL_UINT                           slotOffset)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindDescriptorSet* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdBindDescriptorSet);
    CREATE_TRACE_PACKET(xglCmdBindDescriptorSet, 0);
    real_xglCmdBindDescriptorSet(cmdBuffer, pipelineBindPoint, index, descriptorSet, slotOffset);
    pPacket = interpret_body_as_xglCmdBindDescriptorSet(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->index = index;
    pPacket->descriptorSet = descriptorSet;
    pPacket->slotOffset = slotOffset;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdBindDynamicMemoryView(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_PIPELINE_BIND_POINT            pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindDynamicMemoryView* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdBindDynamicMemoryView);
    CREATE_TRACE_PACKET(xglCmdBindDynamicMemoryView, sizeof(XGL_MEMORY_VIEW_ATTACH_INFO));
    real_xglCmdBindDynamicMemoryView(cmdBuffer, pipelineBindPoint, pMemView);
    pPacket = interpret_body_as_xglCmdBindDynamicMemoryView(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMemView), sizeof(XGL_MEMORY_VIEW_ATTACH_INFO), pMemView);
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemView));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdBindIndexData(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset,
    XGL_INDEX_TYPE indexType)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindIndexData* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdBindIndexData);
    CREATE_TRACE_PACKET(xglCmdBindIndexData, 0);
    real_xglCmdBindIndexData(cmdBuffer, mem, offset, indexType);
    pPacket = interpret_body_as_xglCmdBindIndexData(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->mem = mem;
    pPacket->offset = offset;
    pPacket->indexType = indexType;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdBindAttachments(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_UINT                           colorAttachmentCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*  pColorAttachments,
    const XGL_DEPTH_STENCIL_BIND_INFO* pDepthAttachment)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindAttachments* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdBindAttachments);
    CREATE_TRACE_PACKET(xglCmdBindAttachments, colorAttachmentCount*sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO)+((pDepthAttachment != NULL) ? sizeof(XGL_DEPTH_STENCIL_BIND_INFO) : 0));
    real_xglCmdBindAttachments(cmdBuffer, colorAttachmentCount, pColorAttachments, pDepthAttachment);
    pPacket = interpret_body_as_xglCmdBindAttachments(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->colorAttachmentCount = colorAttachmentCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pColorAttachments), colorAttachmentCount*sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO), pColorAttachments);
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDepthAttachment), sizeof(XGL_DEPTH_STENCIL_BIND_INFO), pDepthAttachment);
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pColorAttachments));
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDepthAttachment));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdPrepareMemoryRegions(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_UINT                           transitionCount,
    const XGL_MEMORY_STATE_TRANSITION* pStateTransitions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdPrepareMemoryRegions* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdPrepareMemoryRegions);
    CREATE_TRACE_PACKET(xglCmdPrepareMemoryRegions, transitionCount*sizeof(XGL_MEMORY_STATE_TRANSITION));
    real_xglCmdPrepareMemoryRegions(cmdBuffer, transitionCount, pStateTransitions);
    pPacket = interpret_body_as_xglCmdPrepareMemoryRegions(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->transitionCount = transitionCount;
	glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pStateTransitions), transitionCount*sizeof(XGL_MEMORY_STATE_TRANSITION), pStateTransitions);
	glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pStateTransitions));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdPrepareImages(
    XGL_CMD_BUFFER                    cmdBuffer,
    XGL_UINT                          transitionCount,
    const XGL_IMAGE_STATE_TRANSITION* pStateTransitions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdPrepareImages* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdPrepareImages);
    CREATE_TRACE_PACKET(xglCmdPrepareImages, transitionCount*sizeof(XGL_IMAGE_STATE_TRANSITION));
    real_xglCmdPrepareImages(cmdBuffer, transitionCount, pStateTransitions);
    pPacket = interpret_body_as_xglCmdPrepareImages(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->transitionCount = transitionCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pStateTransitions), transitionCount*sizeof(XGL_IMAGE_STATE_TRANSITION), pStateTransitions);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pStateTransitions));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdDraw(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_UINT       firstVertex,
    XGL_UINT       vertexCount,
    XGL_UINT       firstInstance,
    XGL_UINT       instanceCount)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDraw* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdDraw);
    CREATE_TRACE_PACKET(xglCmdDraw, 0);
    real_xglCmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
    pPacket = interpret_body_as_xglCmdDraw(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->firstVertex = firstVertex;
    pPacket->vertexCount = vertexCount;
    pPacket->firstInstance = firstInstance;
    pPacket->instanceCount = instanceCount;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdDrawIndexed(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_UINT       firstIndex,
    XGL_UINT       indexCount,
    XGL_INT        vertexOffset,
    XGL_UINT       firstInstance,
    XGL_UINT       instanceCount)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDrawIndexed* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdDrawIndexed);
    CREATE_TRACE_PACKET(xglCmdDrawIndexed, 0);
    real_xglCmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
    pPacket = interpret_body_as_xglCmdDrawIndexed(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->firstIndex = firstIndex;
    pPacket->indexCount = indexCount;
    pPacket->vertexOffset = vertexOffset;
    pPacket->firstInstance = firstInstance;
    pPacket->instanceCount = instanceCount;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdDrawIndirect(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset,
    XGL_UINT32     count,
    XGL_UINT32     stride)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDrawIndirect* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdDrawIndirect);
    CREATE_TRACE_PACKET(xglCmdDrawIndirect, 0);
    real_xglCmdDrawIndirect(cmdBuffer, mem, offset, count, stride);
    pPacket = interpret_body_as_xglCmdDrawIndirect(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->mem = mem;
    pPacket->offset = offset;
    pPacket->count = count;
    pPacket->stride = stride;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset,
    XGL_UINT32     count,
    XGL_UINT32     stride)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDrawIndexedIndirect* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdDrawIndexedIndirect);
    CREATE_TRACE_PACKET(xglCmdDrawIndexedIndirect, 0);
    real_xglCmdDrawIndexedIndirect(cmdBuffer, mem, offset, count, stride);
    pPacket = interpret_body_as_xglCmdDrawIndexedIndirect(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->mem = mem;
    pPacket->offset = offset;
    pPacket->count = count;
    pPacket->stride = stride;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdDispatch(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_UINT       x,
    XGL_UINT       y,
    XGL_UINT       z)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDispatch* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdDispatch);
    CREATE_TRACE_PACKET(xglCmdDispatch, 0);
    real_xglCmdDispatch(cmdBuffer, x, y, z);
    pPacket = interpret_body_as_xglCmdDispatch(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->x = x;
    pPacket->y = y;
    pPacket->z = z;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdDispatchIndirect(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDispatchIndirect* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdDispatchIndirect);
    CREATE_TRACE_PACKET(xglCmdDispatchIndirect, 0);
    real_xglCmdDispatchIndirect(cmdBuffer, mem, offset);
    pPacket = interpret_body_as_xglCmdDispatchIndirect(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->mem = mem;
    pPacket->offset = offset;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdCopyMemory(
    XGL_CMD_BUFFER         cmdBuffer,
    XGL_GPU_MEMORY         srcMem,
    XGL_GPU_MEMORY         destMem,
    XGL_UINT               regionCount,
    const XGL_MEMORY_COPY* pRegions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCopyMemory* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdCopyMemory);
    CREATE_TRACE_PACKET(xglCmdCopyMemory, regionCount*sizeof(XGL_MEMORY_COPY));
    real_xglCmdCopyMemory(cmdBuffer, srcMem, destMem, regionCount, pRegions);
    pPacket = interpret_body_as_xglCmdCopyMemory(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcMem = srcMem;
    pPacket->destMem = destMem;
    pPacket->regionCount = regionCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(XGL_MEMORY_COPY), pRegions);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdCopyImage(
    XGL_CMD_BUFFER        cmdBuffer,
    XGL_IMAGE             srcImage,
    XGL_IMAGE             destImage,
    XGL_UINT              regionCount,
    const XGL_IMAGE_COPY* pRegions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCopyImage* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdCopyImage);
    CREATE_TRACE_PACKET(xglCmdCopyImage, regionCount*sizeof(XGL_MEMORY_IMAGE_COPY));
    real_xglCmdCopyImage(cmdBuffer, srcImage, destImage, regionCount, pRegions);
    pPacket = interpret_body_as_xglCmdCopyImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->destImage = destImage;
    pPacket->regionCount = regionCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(XGL_MEMORY_IMAGE_COPY), pRegions);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));    FINISH_TRACE_PACKET();
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdCopyMemoryToImage(
    XGL_CMD_BUFFER               cmdBuffer,
    XGL_GPU_MEMORY               srcMem,
    XGL_IMAGE                    destImage,
    XGL_UINT                     regionCount,
    const XGL_MEMORY_IMAGE_COPY* pRegions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCopyMemoryToImage* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdCopyMemoryToImage);
    CREATE_TRACE_PACKET(xglCmdCopyMemoryToImage, regionCount*sizeof(XGL_MEMORY_IMAGE_COPY));
    real_xglCmdCopyMemoryToImage(cmdBuffer, srcMem, destImage, regionCount, pRegions);
    pPacket = interpret_body_as_xglCmdCopyMemoryToImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcMem = srcMem;
    pPacket->destImage = destImage;
    pPacket->regionCount = regionCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(XGL_MEMORY_IMAGE_COPY), pRegions);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdCopyImageToMemory(
    XGL_CMD_BUFFER               cmdBuffer,
    XGL_IMAGE                    srcImage,
    XGL_GPU_MEMORY               destMem,
    XGL_UINT                     regionCount,
    const XGL_MEMORY_IMAGE_COPY* pRegions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCopyImageToMemory* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdCopyImageToMemory);
    CREATE_TRACE_PACKET(xglCmdCopyImageToMemory, regionCount*sizeof(XGL_MEMORY_IMAGE_COPY));
    real_xglCmdCopyImageToMemory(cmdBuffer, srcImage, destMem, regionCount, pRegions);
    pPacket = interpret_body_as_xglCmdCopyImageToMemory(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->destMem = destMem;
    pPacket->regionCount = regionCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(XGL_MEMORY_IMAGE_COPY), pRegions);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdCloneImageData(
    XGL_CMD_BUFFER  cmdBuffer,
    XGL_IMAGE       srcImage,
    XGL_IMAGE_STATE srcImageState,
    XGL_IMAGE       destImage,
    XGL_IMAGE_STATE destImageState)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCloneImageData* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdCloneImageData);
    CREATE_TRACE_PACKET(xglCmdCloneImageData, 0);
    real_xglCmdCloneImageData(cmdBuffer, srcImage, srcImageState, destImage, destImageState);
    pPacket = interpret_body_as_xglCmdCloneImageData(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->srcImageState = srcImageState;
    pPacket->destImage = destImage;
    pPacket->destImageState = destImageState;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdUpdateMemory(
    XGL_CMD_BUFFER    cmdBuffer,
    XGL_GPU_MEMORY    destMem,
    XGL_GPU_SIZE      destOffset,
    XGL_GPU_SIZE      dataSize,
    const XGL_UINT32* pData)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdUpdateMemory* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdUpdateMemory);
    CREATE_TRACE_PACKET(xglCmdUpdateMemory, dataSize);
    real_xglCmdUpdateMemory(cmdBuffer, destMem, destOffset, dataSize, pData);
    pPacket = interpret_body_as_xglCmdUpdateMemory(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->destMem = destMem;
    pPacket->destOffset = destOffset;
    pPacket->dataSize = dataSize;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), dataSize, pData);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdFillMemory(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY destMem,
    XGL_GPU_SIZE   destOffset,
    XGL_GPU_SIZE   fillSize,
    XGL_UINT32     data)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdFillMemory* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdFillMemory);
    CREATE_TRACE_PACKET(xglCmdFillMemory, 0);
    real_xglCmdFillMemory(cmdBuffer, destMem, destOffset, fillSize, data);
    pPacket = interpret_body_as_xglCmdFillMemory(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->destMem = destMem;
    pPacket->destOffset = destOffset;
    pPacket->fillSize = fillSize;
    pPacket->data = data;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdClearColorImage(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_IMAGE                          image,
    const XGL_FLOAT                    color[4],
    XGL_UINT                           rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdClearColorImage* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdClearColorImage);
    CREATE_TRACE_PACKET(xglCmdClearColorImage, rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE));
    real_xglCmdClearColorImage(cmdBuffer, image, color, rangeCount, pRanges);
    pPacket = interpret_body_as_xglCmdClearColorImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->image = image;
    memcpy((void*)pPacket->color, color, 4 * sizeof(XGL_UINT32));
    pPacket->rangeCount = rangeCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRanges), rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE), pRanges);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRanges));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdClearColorImageRaw(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_IMAGE                          image,
    const XGL_UINT32                   color[4],
    XGL_UINT                           rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdClearColorImageRaw* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdClearColorImageRaw);
    CREATE_TRACE_PACKET(xglCmdClearColorImageRaw, rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE));
    real_xglCmdClearColorImageRaw(cmdBuffer, image, color, rangeCount, pRanges);
    pPacket = interpret_body_as_xglCmdClearColorImageRaw(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->image = image;
    memcpy((void*)pPacket->color, color, 4 * sizeof(XGL_UINT32));
    pPacket->rangeCount = rangeCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRanges), rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE), pRanges);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRanges));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdClearDepthStencil(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_IMAGE                          image,
    XGL_FLOAT                          depth,
    XGL_UINT32                          stencil,
    XGL_UINT                           rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdClearDepthStencil* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdClearDepthStencil);
    CREATE_TRACE_PACKET(xglCmdClearDepthStencil, rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE));
    real_xglCmdClearDepthStencil(cmdBuffer, image, depth, stencil, rangeCount, pRanges);
    pPacket = interpret_body_as_xglCmdClearDepthStencil(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->image = image;
    pPacket->depth = depth;
    pPacket->stencil = stencil;
    pPacket->rangeCount = rangeCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRanges), rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE), pRanges);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRanges));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdResolveImage(
    XGL_CMD_BUFFER           cmdBuffer,
    XGL_IMAGE                srcImage,
    XGL_IMAGE                destImage,
    XGL_UINT                 rectCount,
    const XGL_IMAGE_RESOLVE* pRects)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdResolveImage* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdResolveImage);
    CREATE_TRACE_PACKET(xglCmdResolveImage, rectCount*sizeof(XGL_IMAGE_RESOLVE));
    real_xglCmdResolveImage(cmdBuffer, srcImage, destImage, rectCount, pRects);
    pPacket = interpret_body_as_xglCmdResolveImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->destImage = destImage;
    pPacket->rectCount = rectCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRects), rectCount*sizeof(XGL_IMAGE_RESOLVE), pRects);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRects));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdSetEvent(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT      event)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdSetEvent* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdSetEvent);
    CREATE_TRACE_PACKET(xglCmdSetEvent, 0);
    real_xglCmdSetEvent(cmdBuffer, event);
    pPacket = interpret_body_as_xglCmdSetEvent(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->event = event;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdResetEvent(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT      event)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdResetEvent* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdResetEvent);
    CREATE_TRACE_PACKET(xglCmdResetEvent, 0);
    real_xglCmdResetEvent(cmdBuffer, event);
    pPacket = interpret_body_as_xglCmdResetEvent(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->event = event;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdMemoryAtomic(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY destMem,
    XGL_GPU_SIZE   destOffset,
    XGL_UINT64     srcData,
    XGL_ATOMIC_OP  atomicOp)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdMemoryAtomic* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdMemoryAtomic);
    CREATE_TRACE_PACKET(xglCmdMemoryAtomic, 0);
    real_xglCmdMemoryAtomic(cmdBuffer, destMem, destOffset, srcData, atomicOp);
    pPacket = interpret_body_as_xglCmdMemoryAtomic(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->destMem = destMem;
    pPacket->destOffset = destOffset;
    pPacket->srcData = srcData;
    pPacket->atomicOp = atomicOp;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdBeginQuery(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    XGL_UINT       slot,
    XGL_FLAGS      flags)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBeginQuery* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdBeginQuery);
    CREATE_TRACE_PACKET(xglCmdBeginQuery, 0);
    real_xglCmdBeginQuery(cmdBuffer, queryPool, slot, flags);
    pPacket = interpret_body_as_xglCmdBeginQuery(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->slot = slot;
    pPacket->flags = flags;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdEndQuery(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    XGL_UINT       slot)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdEndQuery* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdEndQuery);
    CREATE_TRACE_PACKET(xglCmdEndQuery, 0);
    real_xglCmdEndQuery(cmdBuffer, queryPool, slot);
    pPacket = interpret_body_as_xglCmdEndQuery(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->slot = slot;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdResetQueryPool(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    XGL_UINT       startQuery,
    XGL_UINT       queryCount)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdResetQueryPool* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdResetQueryPool);
    CREATE_TRACE_PACKET(xglCmdResetQueryPool, 0);
    real_xglCmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
    pPacket = interpret_body_as_xglCmdResetQueryPool(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->startQuery = startQuery;
    pPacket->queryCount = queryCount;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdWriteTimestamp(
    XGL_CMD_BUFFER           cmdBuffer,
    XGL_TIMESTAMP_TYPE       timestampType,
    XGL_GPU_MEMORY           destMem,
    XGL_GPU_SIZE             destOffset)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdWriteTimestamp* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdWriteTimestamp);
    CREATE_TRACE_PACKET(xglCmdWriteTimestamp, 0);
    real_xglCmdWriteTimestamp(cmdBuffer, timestampType, destMem, destOffset);
    pPacket = interpret_body_as_xglCmdWriteTimestamp(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->timestampType = timestampType;
    pPacket->destMem = destMem;
    pPacket->destOffset = destOffset;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdInitAtomicCounters(
    XGL_CMD_BUFFER                   cmdBuffer,
    XGL_PIPELINE_BIND_POINT          pipelineBindPoint,
    XGL_UINT                         startCounter,
    XGL_UINT                         counterCount,
    const XGL_UINT32*                pData)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdInitAtomicCounters* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdInitAtomicCounters);
    CREATE_TRACE_PACKET(xglCmdInitAtomicCounters, counterCount*sizeof(XGL_UINT32));
    real_xglCmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
    pPacket = interpret_body_as_xglCmdInitAtomicCounters(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->startCounter = startCounter;
    pPacket->counterCount = counterCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), counterCount*sizeof(XGL_UINT32), pData);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdLoadAtomicCounters(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_UINT                      startCounter,
    XGL_UINT                      counterCount,
    XGL_GPU_MEMORY                srcMem,
    XGL_GPU_SIZE                  srcOffset)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdLoadAtomicCounters* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdLoadAtomicCounters);
    CREATE_TRACE_PACKET(xglCmdLoadAtomicCounters, 0);
    real_xglCmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcMem, srcOffset);
    pPacket = interpret_body_as_xglCmdLoadAtomicCounters(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->startCounter = startCounter;
    pPacket->counterCount = counterCount;
    pPacket->srcMem = srcMem;
    pPacket->srcOffset = srcOffset;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_VOID XGLAPI __HOOKED_xglCmdSaveAtomicCounters(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_UINT                      startCounter,
    XGL_UINT                      counterCount,
    XGL_GPU_MEMORY                destMem,
    XGL_GPU_SIZE                  destOffset)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdSaveAtomicCounters* pPacket;
    SEND_ENTRYPOINT_ID(xglCmdSaveAtomicCounters);
    CREATE_TRACE_PACKET(xglCmdSaveAtomicCounters, 0);
    real_xglCmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destMem, destOffset);
    pPacket = interpret_body_as_xglCmdSaveAtomicCounters(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->startCounter = startCounter;
    pPacket->counterCount = counterCount;
    pPacket->destMem = destMem;
    pPacket->destOffset = destOffset;
    FINISH_TRACE_PACKET();
}
