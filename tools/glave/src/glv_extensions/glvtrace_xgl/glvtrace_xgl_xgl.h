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
#include "glvtrace_xgl_xgl_structs.h"
#include "glvtrace_xgl_packet_id.h"

void AttachHooks();
void DetachHooks();

// Pointers to real functions and declarations of hooked functions
#ifdef WIN32
#define __HOOKED_xglInitAndEnumerateGpus hooked_xglInitAndEnumerateGpus
#define __HOOKED_xglGetGpuInfo hooked_xglGetGpuInfo
#define __HOOKED_xglCreateDevice hooked_xglCreateDevice
#define __HOOKED_xglDestroyDevice hooked_xglDestroyDevice
#define __HOOKED_xglGetExtensionSupport hooked_xglGetExtensionSupport
#define __HOOKED_xglGetDeviceQueue hooked_xglGetDeviceQueue
#define __HOOKED_xglQueueSubmit hooked_xglQueueSubmit
#define __HOOKED_xglQueueSetGlobalMemReferences hooked_xglQueueSetGlobalMemReferences
#define __HOOKED_xglQueueWaitIdle hooked_xglQueueWaitIdle
#define __HOOKED_xglDeviceWaitIdle hooked_xglDeviceWaitIdle
#define __HOOKED_xglGetMemoryHeapCount hooked_xglGetMemoryHeapCount
#define __HOOKED_xglGetMemoryHeapInfo hooked_xglGetMemoryHeapInfo
#define __HOOKED_xglAllocMemory hooked_xglAllocMemory
#define __HOOKED_xglFreeMemory hooked_xglFreeMemory
#define __HOOKED_xglSetMemoryPriority hooked_xglSetMemoryPriority
#define __HOOKED_xglMapMemory hooked_xglMapMemory
#define __HOOKED_xglUnmapMemory hooked_xglUnmapMemory
#define __HOOKED_xglPinSystemMemory hooked_xglPinSystemMemory
#define __HOOKED_xglRemapVirtualMemoryPages hooked_xglRemapVirtualMemoryPages
#define __HOOKED_xglGetMultiGpuCompatibility hooked_xglGetMultiGpuCompatibility
#define __HOOKED_xglOpenSharedMemory hooked_xglOpenSharedMemory
#define __HOOKED_xglOpenSharedQueueSemaphore hooked_xglOpenSharedQueueSemaphore
#define __HOOKED_xglOpenPeerMemory hooked_xglOpenPeerMemory
#define __HOOKED_xglOpenPeerImage hooked_xglOpenPeerImage
#define __HOOKED_xglDestroyObject hooked_xglDestroyObject
#define __HOOKED_xglGetObjectInfo hooked_xglGetObjectInfo
#define __HOOKED_xglBindObjectMemory hooked_xglBindObjectMemory
#define __HOOKED_xglCreateFence hooked_xglCreateFence
#define __HOOKED_xglGetFenceStatus hooked_xglGetFenceStatus
#define __HOOKED_xglWaitForFences hooked_xglWaitForFences
#define __HOOKED_xglCreateQueueSemaphore hooked_xglCreateQueueSemaphore
#define __HOOKED_xglSignalQueueSemaphore hooked_xglSignalQueueSemaphore
#define __HOOKED_xglWaitQueueSemaphore hooked_xglWaitQueueSemaphore
#define __HOOKED_xglCreateEvent hooked_xglCreateEvent
#define __HOOKED_xglGetEventStatus hooked_xglGetEventStatus
#define __HOOKED_xglSetEvent hooked_xglSetEvent
#define __HOOKED_xglResetEvent hooked_xglResetEvent
#define __HOOKED_xglCreateQueryPool hooked_xglCreateQueryPool
#define __HOOKED_xglGetQueryPoolResults hooked_xglGetQueryPoolResults
#define __HOOKED_xglGetFormatInfo hooked_xglGetFormatInfo
#define __HOOKED_xglCreateImage hooked_xglCreateImage
#define __HOOKED_xglGetImageSubresourceInfo hooked_xglGetImageSubresourceInfo
#define __HOOKED_xglCreateImageView hooked_xglCreateImageView
#define __HOOKED_xglCreateColorAttachmentView hooked_xglCreateColorAttachmentView
#define __HOOKED_xglCreateDepthStencilView hooked_xglCreateDepthStencilView
#define __HOOKED_xglCreateShader hooked_xglCreateShader
#define __HOOKED_xglCreateGraphicsPipeline hooked_xglCreateGraphicsPipeline
#define __HOOKED_xglCreateComputePipeline hooked_xglCreateComputePipeline
#define __HOOKED_xglStorePipeline hooked_xglStorePipeline
#define __HOOKED_xglLoadPipeline hooked_xglLoadPipeline
#define __HOOKED_xglCreateSampler hooked_xglCreateSampler
#define __HOOKED_xglCreateDescriptorSet hooked_xglCreateDescriptorSet
#define __HOOKED_xglBeginDescriptorSetUpdate hooked_xglBeginDescriptorSetUpdate
#define __HOOKED_xglEndDescriptorSetUpdate hooked_xglEndDescriptorSetUpdate
#define __HOOKED_xglAttachSamplerDescriptors hooked_xglAttachSamplerDescriptors
#define __HOOKED_xglAttachImageViewDescriptors hooked_xglAttachImageViewDescriptors
#define __HOOKED_xglAttachMemoryViewDescriptors hooked_xglAttachMemoryViewDescriptors
#define __HOOKED_xglAttachNestedDescriptors hooked_xglAttachNestedDescriptors
#define __HOOKED_xglClearDescriptorSetSlots hooked_xglClearDescriptorSetSlots
#define __HOOKED_xglCreateViewportState hooked_xglCreateViewportState
#define __HOOKED_xglCreateRasterState hooked_xglCreateRasterState
#define __HOOKED_xglCreateMsaaState hooked_xglCreateMsaaState
#define __HOOKED_xglCreateColorBlendState hooked_xglCreateColorBlendState
#define __HOOKED_xglCreateDepthStencilState hooked_xglCreateDepthStencilState
#define __HOOKED_xglCreateCommandBuffer hooked_xglCreateCommandBuffer
#define __HOOKED_xglBeginCommandBuffer hooked_xglBeginCommandBuffer
#define __HOOKED_xglEndCommandBuffer hooked_xglEndCommandBuffer
#define __HOOKED_xglResetCommandBuffer hooked_xglResetCommandBuffer
#define __HOOKED_xglCmdBindPipeline hooked_xglCmdBindPipeline
#define __HOOKED_xglCmdBindStateObject hooked_xglCmdBindStateObject
#define __HOOKED_xglCmdBindDescriptorSet hooked_xglCmdBindDescriptorSet
#define __HOOKED_xglCmdBindDynamicMemoryView hooked_xglCmdBindDynamicMemoryView
#define __HOOKED_xglCmdBindIndexData hooked_xglCmdBindIndexData
#define __HOOKED_xglCmdBindAttachments hooked_xglCmdBindAttachments
#define __HOOKED_xglCmdPrepareMemoryRegions hooked_xglCmdPrepareMemoryRegions
#define __HOOKED_xglCmdPrepareImages hooked_xglCmdPrepareImages
#define __HOOKED_xglCmdDraw hooked_xglCmdDraw
#define __HOOKED_xglCmdDrawIndexed hooked_xglCmdDrawIndexed
#define __HOOKED_xglCmdDrawIndirect hooked_xglCmdDrawIndirect
#define __HOOKED_xglCmdDrawIndexedIndirect hooked_xglCmdDrawIndexedIndirect
#define __HOOKED_xglCmdDispatch hooked_xglCmdDispatch
#define __HOOKED_xglCmdDispatchIndirect hooked_xglCmdDispatchIndirect
#define __HOOKED_xglCmdCopyMemory hooked_xglCmdCopyMemory
#define __HOOKED_xglCmdCopyImage hooked_xglCmdCopyImage
#define __HOOKED_xglCmdCopyMemoryToImage hooked_xglCmdCopyMemoryToImage
#define __HOOKED_xglCmdCopyImageToMemory hooked_xglCmdCopyImageToMemory
#define __HOOKED_xglCmdCloneImageData hooked_xglCmdCloneImageData
#define __HOOKED_xglCmdUpdateMemory hooked_xglCmdUpdateMemory
#define __HOOKED_xglCmdFillMemory hooked_xglCmdFillMemory
#define __HOOKED_xglCmdClearColorImage hooked_xglCmdClearColorImage
#define __HOOKED_xglCmdClearColorImageRaw hooked_xglCmdClearColorImageRaw
#define __HOOKED_xglCmdClearDepthStencil hooked_xglCmdClearDepthStencil
#define __HOOKED_xglCmdResolveImage hooked_xglCmdResolveImage
#define __HOOKED_xglCmdSetEvent hooked_xglCmdSetEvent
#define __HOOKED_xglCmdResetEvent hooked_xglCmdResetEvent
#define __HOOKED_xglCmdMemoryAtomic hooked_xglCmdMemoryAtomic
#define __HOOKED_xglCmdBeginQuery hooked_xglCmdBeginQuery
#define __HOOKED_xglCmdEndQuery hooked_xglCmdEndQuery
#define __HOOKED_xglCmdResetQueryPool hooked_xglCmdResetQueryPool
#define __HOOKED_xglCmdWriteTimestamp hooked_xglCmdWriteTimestamp
#define __HOOKED_xglCmdInitAtomicCounters hooked_xglCmdInitAtomicCounters
#define __HOOKED_xglCmdLoadAtomicCounters hooked_xglCmdLoadAtomicCounters
#define __HOOKED_xglCmdSaveAtomicCounters hooked_xglCmdSaveAtomicCounters

#elif defined(__linux__)
#define __HOOKED_xglInitAndEnumerateGpus xglInitAndEnumerateGpus
#define __HOOKED_xglGetGpuInfo xglGetGpuInfo
#define __HOOKED_xglCreateDevice xglCreateDevice
#define __HOOKED_xglDestroyDevice xglDestroyDevice
#define __HOOKED_xglGetExtensionSupport xglGetExtensionSupport
#define __HOOKED_xglGetDeviceQueue xglGetDeviceQueue
#define __HOOKED_xglQueueSubmit xglQueueSubmit
#define __HOOKED_xglQueueSetGlobalMemReferences xglQueueSetGlobalMemReferences
#define __HOOKED_xglQueueWaitIdle xglQueueWaitIdle
#define __HOOKED_xglDeviceWaitIdle xglDeviceWaitIdle
#define __HOOKED_xglGetMemoryHeapCount xglGetMemoryHeapCount
#define __HOOKED_xglGetMemoryHeapInfo xglGetMemoryHeapInfo
#define __HOOKED_xglAllocMemory xglAllocMemory
#define __HOOKED_xglFreeMemory xglFreeMemory
#define __HOOKED_xglSetMemoryPriority xglSetMemoryPriority
#define __HOOKED_xglMapMemory xglMapMemory
#define __HOOKED_xglUnmapMemory xglUnmapMemory
#define __HOOKED_xglPinSystemMemory xglPinSystemMemory
#define __HOOKED_xglRemapVirtualMemoryPages xglRemapVirtualMemoryPages
#define __HOOKED_xglGetMultiGpuCompatibility xglGetMultiGpuCompatibility
#define __HOOKED_xglOpenSharedMemory xglOpenSharedMemory
#define __HOOKED_xglOpenSharedQueueSemaphore xglOpenSharedQueueSemaphore
#define __HOOKED_xglOpenPeerMemory xglOpenPeerMemory
#define __HOOKED_xglOpenPeerImage xglOpenPeerImage
#define __HOOKED_xglDestroyObject xglDestroyObject
#define __HOOKED_xglGetObjectInfo xglGetObjectInfo
#define __HOOKED_xglBindObjectMemory xglBindObjectMemory
#define __HOOKED_xglCreateFence xglCreateFence
#define __HOOKED_xglGetFenceStatus xglGetFenceStatus
#define __HOOKED_xglWaitForFences xglWaitForFences
#define __HOOKED_xglCreateQueueSemaphore xglCreateQueueSemaphore
#define __HOOKED_xglSignalQueueSemaphore xglSignalQueueSemaphore
#define __HOOKED_xglWaitQueueSemaphore xglWaitQueueSemaphore
#define __HOOKED_xglCreateEvent xglCreateEvent
#define __HOOKED_xglGetEventStatus xglGetEventStatus
#define __HOOKED_xglSetEvent xglSetEvent
#define __HOOKED_xglResetEvent xglResetEvent
#define __HOOKED_xglCreateQueryPool xglCreateQueryPool
#define __HOOKED_xglGetQueryPoolResults xglGetQueryPoolResults
#define __HOOKED_xglGetFormatInfo xglGetFormatInfo
#define __HOOKED_xglCreateImage xglCreateImage
#define __HOOKED_xglGetImageSubresourceInfo xglGetImageSubresourceInfo
#define __HOOKED_xglCreateImageView xglCreateImageView
#define __HOOKED_xglCreateColorAttachmentView xglCreateColorAttachmentView
#define __HOOKED_xglCreateDepthStencilView xglCreateDepthStencilView
#define __HOOKED_xglCreateShader xglCreateShader
#define __HOOKED_xglCreateGraphicsPipeline xglCreateGraphicsPipeline
#define __HOOKED_xglCreateComputePipeline xglCreateComputePipeline
#define __HOOKED_xglStorePipeline xglStorePipeline
#define __HOOKED_xglLoadPipeline xglLoadPipeline
#define __HOOKED_xglCreateSampler xglCreateSampler
#define __HOOKED_xglCreateDescriptorSet xglCreateDescriptorSet
#define __HOOKED_xglBeginDescriptorSetUpdate xglBeginDescriptorSetUpdate
#define __HOOKED_xglEndDescriptorSetUpdate xglEndDescriptorSetUpdate
#define __HOOKED_xglAttachSamplerDescriptors xglAttachSamplerDescriptors
#define __HOOKED_xglAttachImageViewDescriptors xglAttachImageViewDescriptors
#define __HOOKED_xglAttachMemoryViewDescriptors xglAttachMemoryViewDescriptors
#define __HOOKED_xglAttachNestedDescriptors xglAttachNestedDescriptors
#define __HOOKED_xglClearDescriptorSetSlots xglClearDescriptorSetSlots
#define __HOOKED_xglCreateViewportState xglCreateViewportState
#define __HOOKED_xglCreateRasterState xglCreateRasterState
#define __HOOKED_xglCreateMsaaState xglCreateMsaaState
#define __HOOKED_xglCreateColorBlendState xglCreateColorBlendState
#define __HOOKED_xglCreateDepthStencilState xglCreateDepthStencilState
#define __HOOKED_xglCreateCommandBuffer xglCreateCommandBuffer
#define __HOOKED_xglBeginCommandBuffer xglBeginCommandBuffer
#define __HOOKED_xglEndCommandBuffer xglEndCommandBuffer
#define __HOOKED_xglResetCommandBuffer xglResetCommandBuffer
#define __HOOKED_xglCmdBindPipeline xglCmdBindPipeline
#define __HOOKED_xglCmdBindStateObject xglCmdBindStateObject
#define __HOOKED_xglCmdBindDescriptorSet xglCmdBindDescriptorSet
#define __HOOKED_xglCmdBindDynamicMemoryView xglCmdBindDynamicMemoryView
#define __HOOKED_xglCmdBindIndexData xglCmdBindIndexData
#define __HOOKED_xglCmdBindAttachments xglCmdBindAttachments
#define __HOOKED_xglCmdPrepareMemoryRegions xglCmdPrepareMemoryRegions
#define __HOOKED_xglCmdPrepareImages xglCmdPrepareImages
#define __HOOKED_xglCmdDraw xglCmdDraw
#define __HOOKED_xglCmdDrawIndexed xglCmdDrawIndexed
#define __HOOKED_xglCmdDrawIndirect xglCmdDrawIndirect
#define __HOOKED_xglCmdDrawIndexedIndirect xglCmdDrawIndexedIndirect
#define __HOOKED_xglCmdDispatch xglCmdDispatch
#define __HOOKED_xglCmdDispatchIndirect xglCmdDispatchIndirect
#define __HOOKED_xglCmdCopyMemory xglCmdCopyMemory
#define __HOOKED_xglCmdCopyImage xglCmdCopyImage
#define __HOOKED_xglCmdCopyMemoryToImage xglCmdCopyMemoryToImage
#define __HOOKED_xglCmdCopyImageToMemory xglCmdCopyImageToMemory
#define __HOOKED_xglCmdCloneImageData xglCmdCloneImageData
#define __HOOKED_xglCmdUpdateMemory xglCmdUpdateMemory
#define __HOOKED_xglCmdFillMemory xglCmdFillMemory
#define __HOOKED_xglCmdClearColorImage xglCmdClearColorImage
#define __HOOKED_xglCmdClearColorImageRaw xglCmdClearColorImageRaw
#define __HOOKED_xglCmdClearDepthStencil xglCmdClearDepthStencil
#define __HOOKED_xglCmdResolveImage xglCmdResolveImage
#define __HOOKED_xglCmdSetEvent xglCmdSetEvent
#define __HOOKED_xglCmdResetEvent xglCmdResetEvent
#define __HOOKED_xglCmdMemoryAtomic xglCmdMemoryAtomic
#define __HOOKED_xglCmdBeginQuery xglCmdBeginQuery
#define __HOOKED_xglCmdEndQuery xglCmdEndQuery
#define __HOOKED_xglCmdResetQueryPool xglCmdResetQueryPool
#define __HOOKED_xglCmdWriteTimestamp xglCmdWriteTimestamp
#define __HOOKED_xglCmdInitAtomicCounters xglCmdInitAtomicCounters
#define __HOOKED_xglCmdLoadAtomicCounters xglCmdLoadAtomicCounters
#define __HOOKED_xglCmdSaveAtomicCounters xglCmdSaveAtomicCounters
#endif

// GPU initialization

XGL_RESULT XGLAPI __HOOKED_xglInitAndEnumerateGpus(
    const XGL_APPLICATION_INFO* pAppInfo,
    const XGL_ALLOC_CALLBACKS*  pAllocCb,
    XGL_UINT                    maxGpus,
    XGL_UINT*                   pGpuCount,
    XGL_PHYSICAL_GPU*           pGpus);

XGL_RESULT XGLAPI __HOOKED_xglGetGpuInfo(
    XGL_PHYSICAL_GPU            gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE  infoType,
    XGL_SIZE*                   pDataSize,
    XGL_VOID*                   pData);

// Device functions
XGL_RESULT XGLAPI __HOOKED_xglCreateDevice(
    XGL_PHYSICAL_GPU              gpu,
    const XGL_DEVICE_CREATE_INFO* pCreateInfo,
    XGL_DEVICE*                   pDevice);

XGL_RESULT XGLAPI __HOOKED_xglDestroyDevice(
    XGL_DEVICE device);

// Extension discovery functions
XGL_RESULT XGLAPI __HOOKED_xglGetExtensionSupport(
    XGL_PHYSICAL_GPU gpu,
    const XGL_CHAR*  pExtName);

// Queue functions

XGL_RESULT XGLAPI __HOOKED_xglGetDeviceQueue(
    XGL_DEVICE       device,
    XGL_QUEUE_TYPE   queueType,
    XGL_UINT         queueIndex,
    XGL_QUEUE*       pQueue);

XGL_RESULT XGLAPI __HOOKED_xglQueueSubmit(
    XGL_QUEUE             queue,
    XGL_UINT              cmdBufferCount,
    const XGL_CMD_BUFFER* pCmdBuffers,
    XGL_UINT              memRefCount,
    const XGL_MEMORY_REF* pMemRefs,
    XGL_FENCE             fence);

XGL_RESULT XGLAPI __HOOKED_xglQueueSetGlobalMemReferences(
    XGL_QUEUE             queue,
    XGL_UINT              memRefCount,
    const XGL_MEMORY_REF* pMemRefs);

XGL_RESULT XGLAPI __HOOKED_xglQueueWaitIdle(
    XGL_QUEUE queue);

XGL_RESULT XGLAPI __HOOKED_xglDeviceWaitIdle(
    XGL_DEVICE device);

// Memory functions
XGL_RESULT XGLAPI __HOOKED_xglGetMemoryHeapCount(
    XGL_DEVICE  device,
    XGL_UINT*   pCount);

XGL_RESULT XGLAPI __HOOKED_xglGetMemoryHeapInfo(
    XGL_DEVICE                  device,
    XGL_UINT                    heapId,
    XGL_MEMORY_HEAP_INFO_TYPE   infoType,
    XGL_SIZE*                   pDataSize,
    XGL_VOID*                   pData);

XGL_RESULT XGLAPI __HOOKED_xglAllocMemory(
    XGL_DEVICE                   device,
    const XGL_MEMORY_ALLOC_INFO* pAllocInfo,
    XGL_GPU_MEMORY*              pMem);

XGL_RESULT XGLAPI __HOOKED_xglFreeMemory(
    XGL_GPU_MEMORY mem);

XGL_RESULT XGLAPI __HOOKED_xglSetMemoryPriority(
    XGL_GPU_MEMORY            mem,
    XGL_MEMORY_PRIORITY       priority);

XGL_RESULT XGLAPI __HOOKED_xglMapMemory(
    XGL_GPU_MEMORY mem,
    XGL_FLAGS      flags,                // Reserved
    XGL_VOID**     ppData);

XGL_RESULT XGLAPI __HOOKED_xglUnmapMemory(
    XGL_GPU_MEMORY mem);

XGL_RESULT XGLAPI __HOOKED_xglPinSystemMemory(
    XGL_DEVICE      device,
    const XGL_VOID* pSysMem,
    XGL_SIZE        memSize,
    XGL_GPU_MEMORY* pMem);

XGL_RESULT XGLAPI __HOOKED_xglRemapVirtualMemoryPages(
    XGL_DEVICE                            device,
    XGL_UINT                              rangeCount,
    const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pRanges,
    XGL_UINT                              preWaitSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*            pPreWaitSemaphores,
    XGL_UINT                              postSignalSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*            pPostSignalSemaphores);

// Multi-device functions
XGL_RESULT XGLAPI __HOOKED_xglGetMultiGpuCompatibility(
    XGL_PHYSICAL_GPU            gpu0,
    XGL_PHYSICAL_GPU            gpu1,
    XGL_GPU_COMPATIBILITY_INFO* pInfo);

XGL_RESULT XGLAPI __HOOKED_xglOpenSharedMemory(
    XGL_DEVICE                  device,
    const XGL_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY*             pMem);

XGL_RESULT XGLAPI __HOOKED_xglOpenSharedQueueSemaphore(
    XGL_DEVICE                           device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo,
    XGL_QUEUE_SEMAPHORE*                 pSemaphore);

XGL_RESULT XGLAPI __HOOKED_xglOpenPeerMemory(
    XGL_DEVICE                       device,
    const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY*                  pMem);

XGL_RESULT XGLAPI __HOOKED_xglOpenPeerImage(
    XGL_DEVICE                      device,
    const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo,
    XGL_IMAGE*                      pImage,
    XGL_GPU_MEMORY*                 pMem);

// Generic API object functions

XGL_RESULT XGLAPI __HOOKED_xglDestroyObject(
    XGL_OBJECT object);

XGL_RESULT XGLAPI __HOOKED_xglGetObjectInfo(
    XGL_BASE_OBJECT             object,
    XGL_OBJECT_INFO_TYPE        infoType,
    XGL_SIZE*                   pDataSize,
    XGL_VOID*                   pData);

XGL_RESULT XGLAPI __HOOKED_xglBindObjectMemory(
    XGL_OBJECT     object,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset);

// Fence functions
XGL_RESULT XGLAPI __HOOKED_xglCreateFence(
    XGL_DEVICE                   device,
    const XGL_FENCE_CREATE_INFO* pCreateInfo,
    XGL_FENCE*                   pFence);

XGL_RESULT XGLAPI __HOOKED_xglGetFenceStatus(
    XGL_FENCE fence);

XGL_RESULT XGLAPI __HOOKED_xglWaitForFences(
    XGL_DEVICE       device,
    XGL_UINT         fenceCount,
    const XGL_FENCE* pFences,
    XGL_BOOL         waitAll,
    XGL_UINT64       timeout);

// Queue semaphore functions
XGL_RESULT XGLAPI __HOOKED_xglCreateQueueSemaphore(
    XGL_DEVICE                             device,
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo,
    XGL_QUEUE_SEMAPHORE*                   pSemaphore);

XGL_RESULT XGLAPI __HOOKED_xglSignalQueueSemaphore(
    XGL_QUEUE           queue,
    XGL_QUEUE_SEMAPHORE semaphore);

XGL_RESULT XGLAPI __HOOKED_xglWaitQueueSemaphore(
    XGL_QUEUE           queue,
    XGL_QUEUE_SEMAPHORE semaphore);

// Event functions
XGL_RESULT XGLAPI __HOOKED_xglCreateEvent(
    XGL_DEVICE                   device,
    const XGL_EVENT_CREATE_INFO* pCreateInfo,
    XGL_EVENT*                   pEvent);

XGL_RESULT XGLAPI __HOOKED_xglGetEventStatus(
    XGL_EVENT event);

XGL_RESULT XGLAPI __HOOKED_xglSetEvent(
    XGL_EVENT event);

XGL_RESULT XGLAPI __HOOKED_xglResetEvent(
    XGL_EVENT event);

// Query functions
XGL_RESULT XGLAPI __HOOKED_xglCreateQueryPool(
    XGL_DEVICE                        device,
    const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo,
    XGL_QUERY_POOL*                   pQueryPool);

XGL_RESULT XGLAPI __HOOKED_xglGetQueryPoolResults(
    XGL_QUERY_POOL queryPool,
    XGL_UINT       startQuery,
    XGL_UINT       queryCount,
    XGL_SIZE*      pDataSize,
    XGL_VOID*      pData);

// Format capabilities

XGL_RESULT XGLAPI __HOOKED_xglGetFormatInfo(
    XGL_DEVICE             device,
    XGL_FORMAT             format,
    XGL_FORMAT_INFO_TYPE   infoType,
    XGL_SIZE*              pDataSize,
    XGL_VOID*              pData);

// Image functions
XGL_RESULT XGLAPI __HOOKED_xglCreateImage(
    XGL_DEVICE                   device,
    const XGL_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE*                   pImage);

XGL_RESULT XGLAPI __HOOKED_xglGetImageSubresourceInfo(
    XGL_IMAGE                    image,
    const XGL_IMAGE_SUBRESOURCE* pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE    infoType,
    XGL_SIZE*                    pDataSize,
    XGL_VOID*                    pData);

// Image view functions
XGL_RESULT XGLAPI __HOOKED_xglCreateImageView(
    XGL_DEVICE                        device,
    const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
    XGL_IMAGE_VIEW*                   pView);

XGL_RESULT XGLAPI __HOOKED_xglCreateColorAttachmentView(
    XGL_DEVICE                                   device,
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW*                   pView);

XGL_RESULT XGLAPI __HOOKED_xglCreateDepthStencilView(
    XGL_DEVICE                                device,
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo,
    XGL_DEPTH_STENCIL_VIEW*                   pView);

// Shader functions
XGL_RESULT XGLAPI __HOOKED_xglCreateShader(
    XGL_DEVICE                    device,
    const XGL_SHADER_CREATE_INFO* pCreateInfo,
    XGL_SHADER*                   pShader);

// Pipeline functions
XGL_RESULT XGLAPI __HOOKED_xglCreateGraphicsPipeline(
    XGL_DEVICE                               device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE*                            pPipeline);

XGL_RESULT XGLAPI __HOOKED_xglCreateComputePipeline(
    XGL_DEVICE                              device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE*                           pPipeline);

XGL_RESULT XGLAPI __HOOKED_xglStorePipeline(
    XGL_PIPELINE pipeline,
    XGL_SIZE*    pDataSize,
    XGL_VOID*    pData);

XGL_RESULT XGLAPI __HOOKED_xglLoadPipeline(
    XGL_DEVICE      device,
    XGL_SIZE        dataSize,
    const XGL_VOID* pData,
    XGL_PIPELINE*   pPipeline);

// Sampler functions

XGL_RESULT XGLAPI __HOOKED_xglCreateSampler(
    XGL_DEVICE                     device,
    const XGL_SAMPLER_CREATE_INFO* pCreateInfo,
    XGL_SAMPLER*                   pSampler);

// Descriptor set functions
XGL_RESULT XGLAPI __HOOKED_xglCreateDescriptorSet(
    XGL_DEVICE                            device,
    const XGL_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo,
    XGL_DESCRIPTOR_SET*                   pDescriptorSet);

XGL_VOID XGLAPI __HOOKED_xglBeginDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET descriptorSet);

XGL_VOID XGLAPI __HOOKED_xglEndDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET descriptorSet);

XGL_VOID XGLAPI __HOOKED_xglAttachSamplerDescriptors(
    XGL_DESCRIPTOR_SET descriptorSet,
    XGL_UINT           startSlot,
    XGL_UINT           slotCount,
    const XGL_SAMPLER* pSamplers);

XGL_VOID XGLAPI __HOOKED_xglAttachImageViewDescriptors(
    XGL_DESCRIPTOR_SET                descriptorSet,
    XGL_UINT                          startSlot,
    XGL_UINT                          slotCount,
    const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews);

XGL_VOID XGLAPI __HOOKED_xglAttachMemoryViewDescriptors(
    XGL_DESCRIPTOR_SET                 descriptorSet,
    XGL_UINT                           startSlot,
    XGL_UINT                           slotCount,
    const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews);

XGL_VOID XGLAPI __HOOKED_xglAttachNestedDescriptors(
    XGL_DESCRIPTOR_SET                    descriptorSet,
    XGL_UINT                              startSlot,
    XGL_UINT                              slotCount,
    const XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets);

XGL_VOID XGLAPI __HOOKED_xglClearDescriptorSetSlots(
    XGL_DESCRIPTOR_SET descriptorSet,
    XGL_UINT           startSlot,
    XGL_UINT           slotCount);

// State object functions
XGL_RESULT XGLAPI __HOOKED_xglCreateViewportState(
    XGL_DEVICE                            device,
    const XGL_VIEWPORT_STATE_CREATE_INFO* pCreateInfo,
    XGL_VIEWPORT_STATE_OBJECT*            pState);

XGL_RESULT XGLAPI __HOOKED_xglCreateRasterState(
    XGL_DEVICE                          device,
    const XGL_RASTER_STATE_CREATE_INFO* pCreateInfo,
    XGL_RASTER_STATE_OBJECT*            pState);

XGL_RESULT XGLAPI __HOOKED_xglCreateMsaaState(
    XGL_DEVICE                        device,
    const XGL_MSAA_STATE_CREATE_INFO* pCreateInfo,
    XGL_MSAA_STATE_OBJECT*            pState);

XGL_RESULT XGLAPI __HOOKED_xglCreateColorBlendState(
    XGL_DEVICE                               device,
    const XGL_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo,
    XGL_COLOR_BLEND_STATE_OBJECT*            pState);

XGL_RESULT XGLAPI __HOOKED_xglCreateDepthStencilState(
    XGL_DEVICE                                 device,
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo,
    XGL_DEPTH_STENCIL_STATE_OBJECT*            pState);

// Command buffer functions

XGL_RESULT XGLAPI __HOOKED_xglCreateCommandBuffer(
    XGL_DEVICE                        device,
    const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo,
    XGL_CMD_BUFFER*                   pCmdBuffer);

XGL_RESULT XGLAPI __HOOKED_xglBeginCommandBuffer(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_FLAGS      flags);               // XGL_CMD_BUFFER_BUILD_FLAGS

XGL_RESULT XGLAPI __HOOKED_xglEndCommandBuffer(
    XGL_CMD_BUFFER cmdBuffer);

XGL_RESULT XGLAPI __HOOKED_xglResetCommandBuffer(
    XGL_CMD_BUFFER cmdBuffer);

// Command buffer building functions
XGL_VOID XGLAPI __HOOKED_xglCmdBindPipeline(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_PIPELINE                  pipeline);

XGL_VOID XGLAPI __HOOKED_xglCmdBindStateObject(
    XGL_CMD_BUFFER               cmdBuffer,
    XGL_STATE_BIND_POINT         stateBindPoint,
    XGL_STATE_OBJECT             state);

XGL_VOID XGLAPI __HOOKED_xglCmdBindDescriptorSet(
    XGL_CMD_BUFFER                    cmdBuffer,
    XGL_PIPELINE_BIND_POINT           pipelineBindPoint,
    XGL_UINT                          index,
    XGL_DESCRIPTOR_SET                descriptorSet,
    XGL_UINT                          slotOffset);

XGL_VOID XGLAPI __HOOKED_xglCmdBindDynamicMemoryView(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_PIPELINE_BIND_POINT            pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView);

XGL_VOID XGLAPI __HOOKED_xglCmdBindIndexData(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset,
    XGL_INDEX_TYPE indexType);

XGL_VOID XGLAPI __HOOKED_xglCmdBindAttachments(
    XGL_CMD_BUFFER                         cmdBuffer,
    XGL_UINT                               colorTargetCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*  pColorTargets,
    const XGL_DEPTH_STENCIL_BIND_INFO*     pDepthTarget);

XGL_VOID XGLAPI __HOOKED_xglCmdPrepareMemoryRegions(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_UINT                           transitionCount,
    const XGL_MEMORY_STATE_TRANSITION* pStateTransitions);

XGL_VOID XGLAPI __HOOKED_xglCmdPrepareImages(
    XGL_CMD_BUFFER                    cmdBuffer,
    XGL_UINT                          transitionCount,
    const XGL_IMAGE_STATE_TRANSITION* pStateTransitions);

XGL_VOID XGLAPI __HOOKED_xglCmdDraw(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_UINT       firstVertex,
    XGL_UINT       vertexCount,
    XGL_UINT       firstInstance,
    XGL_UINT       instanceCount);

XGL_VOID XGLAPI __HOOKED_xglCmdDrawIndexed(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_UINT       firstIndex,
    XGL_UINT       indexCount,
    XGL_INT        vertexOffset,
    XGL_UINT       firstInstance,
    XGL_UINT       instanceCount);

XGL_VOID XGLAPI __HOOKED_xglCmdDrawIndirect(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset,
    XGL_UINT32     count,
    XGL_UINT32     stride);

XGL_VOID XGLAPI __HOOKED_xglCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset,
    XGL_UINT32     count,
    XGL_UINT32     stride);

XGL_VOID XGLAPI __HOOKED_xglCmdDispatch(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_UINT       x,
    XGL_UINT       y,
    XGL_UINT       z);

XGL_VOID XGLAPI __HOOKED_xglCmdDispatchIndirect(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE   offset);

XGL_VOID XGLAPI __HOOKED_xglCmdCopyMemory(
    XGL_CMD_BUFFER         cmdBuffer,
    XGL_GPU_MEMORY         srcMem,
    XGL_GPU_MEMORY         destMem,
    XGL_UINT               regionCount,
    const XGL_MEMORY_COPY* pRegions);

XGL_VOID XGLAPI __HOOKED_xglCmdCopyImage(
    XGL_CMD_BUFFER        cmdBuffer,
    XGL_IMAGE             srcImage,
    XGL_IMAGE             destImage,
    XGL_UINT              regionCount,
    const XGL_IMAGE_COPY* pRegions);

XGL_VOID XGLAPI __HOOKED_xglCmdCopyMemoryToImage(
    XGL_CMD_BUFFER               cmdBuffer,
    XGL_GPU_MEMORY               srcMem,
    XGL_IMAGE                    destImage,
    XGL_UINT                     regionCount,
    const XGL_MEMORY_IMAGE_COPY* pRegions);

XGL_VOID XGLAPI __HOOKED_xglCmdCopyImageToMemory(
    XGL_CMD_BUFFER               cmdBuffer,
    XGL_IMAGE                    srcImage,
    XGL_GPU_MEMORY               destMem,
    XGL_UINT                     regionCount,
    const XGL_MEMORY_IMAGE_COPY* pRegions);

XGL_VOID XGLAPI __HOOKED_xglCmdCloneImageData(
    XGL_CMD_BUFFER  cmdBuffer,
    XGL_IMAGE       srcImage,
    XGL_IMAGE_STATE srcImageState,
    XGL_IMAGE       destImage,
    XGL_IMAGE_STATE destImageState);

XGL_VOID XGLAPI __HOOKED_xglCmdUpdateMemory(
    XGL_CMD_BUFFER    cmdBuffer,
    XGL_GPU_MEMORY    destMem,
    XGL_GPU_SIZE      destOffset,
    XGL_GPU_SIZE      dataSize,
    const XGL_UINT32* pData);

XGL_VOID XGLAPI __HOOKED_xglCmdFillMemory(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY destMem,
    XGL_GPU_SIZE   destOffset,
    XGL_GPU_SIZE   fillSize,
    XGL_UINT32     data);

XGL_VOID XGLAPI __HOOKED_xglCmdClearColorImage(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_IMAGE                          image,
    const XGL_FLOAT                    color[4],
    XGL_UINT                           rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);

XGL_VOID XGLAPI __HOOKED_xglCmdClearColorImageRaw(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_IMAGE                          image,
    const XGL_UINT32                   color[4],
    XGL_UINT                           rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);

XGL_VOID XGLAPI __HOOKED_xglCmdClearDepthStencil(
    XGL_CMD_BUFFER                     cmdBuffer,
    XGL_IMAGE                          image,
    XGL_FLOAT                          depth,
    XGL_UINT32                          stencil,
    XGL_UINT                           rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);

XGL_VOID XGLAPI __HOOKED_xglCmdResolveImage(
    XGL_CMD_BUFFER           cmdBuffer,
    XGL_IMAGE                srcImage,
    XGL_IMAGE                destImage,
    XGL_UINT                 rectCount,
    const XGL_IMAGE_RESOLVE* pRects);

XGL_VOID XGLAPI __HOOKED_xglCmdSetEvent(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT      event);

XGL_VOID XGLAPI __HOOKED_xglCmdResetEvent(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT      event);

XGL_VOID XGLAPI __HOOKED_xglCmdMemoryAtomic(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_GPU_MEMORY destMem,
    XGL_GPU_SIZE   destOffset,
    XGL_UINT64     srcData,
    XGL_ATOMIC_OP  atomicOp);

XGL_VOID XGLAPI __HOOKED_xglCmdBeginQuery(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    XGL_UINT       slot,
    XGL_FLAGS      flags);

XGL_VOID XGLAPI __HOOKED_xglCmdEndQuery(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    XGL_UINT       slot);

XGL_VOID XGLAPI __HOOKED_xglCmdResetQueryPool(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    XGL_UINT       startQuery,
    XGL_UINT       queryCount);

XGL_VOID XGLAPI __HOOKED_xglCmdWriteTimestamp(
    XGL_CMD_BUFFER           cmdBuffer,
    XGL_TIMESTAMP_TYPE       timestampType,
    XGL_GPU_MEMORY           destMem,
    XGL_GPU_SIZE             destOffset);

XGL_VOID XGLAPI __HOOKED_xglCmdInitAtomicCounters(
    XGL_CMD_BUFFER                   cmdBuffer,
    XGL_PIPELINE_BIND_POINT          pipelineBindPoint,
    XGL_UINT                         startCounter,
    XGL_UINT                         counterCount,
    const XGL_UINT32*                pData);

XGL_VOID XGLAPI __HOOKED_xglCmdLoadAtomicCounters(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_UINT                      startCounter,
    XGL_UINT                      counterCount,
    XGL_GPU_MEMORY                srcMem,
    XGL_GPU_SIZE                  srcOffset);

XGL_VOID XGLAPI __HOOKED_xglCmdSaveAtomicCounters(
    XGL_CMD_BUFFER                cmdBuffer,
    XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
    XGL_UINT                      startCounter,
    XGL_UINT                      counterCount,
    XGL_GPU_MEMORY                destMem,
    XGL_GPU_SIZE                  destOffset);
