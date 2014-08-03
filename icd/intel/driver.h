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

#ifndef __DRIVER_H_INCLUDED__
#define __DRIVER_H_INCLUDED__

#include <stdio.h>

#include "xgl.h"
#include "xglDbg.h"

struct icd_dispatch_table {
    XGL_RESULT (XGLAPI *InitAndEnumerateGpus)(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, XGL_UINT maxGpus, XGL_UINT* pGpuCount, XGL_PHYSICAL_GPU* pGpus);
    XGL_RESULT (XGLAPI *GetGpuInfo)(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData);
    XGL_RESULT (XGLAPI *CreateDevice)(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice);
    XGL_RESULT (XGLAPI *DestroyDevice)(XGL_DEVICE device);
    XGL_RESULT (XGLAPI *GetExtensionSupport)(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pExtName);
    XGL_RESULT (XGLAPI *GetDeviceQueue)(XGL_DEVICE device, XGL_QUEUE_TYPE queueType, XGL_UINT queueIndex, XGL_QUEUE* pQueue);
    XGL_RESULT (XGLAPI *QueueSubmit)(XGL_QUEUE queue, XGL_UINT cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers, XGL_UINT memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence);
    XGL_RESULT (XGLAPI *QueueSetGlobalMemReferences)(XGL_QUEUE queue, XGL_UINT memRefCount, const XGL_MEMORY_REF* pMemRefs);
    XGL_RESULT (XGLAPI *QueueWaitIdle)(XGL_QUEUE queue);
    XGL_RESULT (XGLAPI *DeviceWaitIdle)(XGL_DEVICE device);
    XGL_RESULT (XGLAPI *GetMemoryHeapCount)(XGL_DEVICE device, XGL_UINT* pCount);
    XGL_RESULT (XGLAPI *GetMemoryHeapInfo)(XGL_DEVICE device, XGL_UINT heapId, XGL_MEMORY_HEAP_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData);
    XGL_RESULT (XGLAPI *AllocMemory)(XGL_DEVICE device, const XGL_MEMORY_ALLOC_INFO* pAllocInfo, XGL_GPU_MEMORY* pMem);
    XGL_RESULT (XGLAPI *FreeMemory)(XGL_GPU_MEMORY mem);
    XGL_RESULT (XGLAPI *SetMemoryPriority)(XGL_GPU_MEMORY mem, XGL_MEMORY_PRIORITY priority);
    XGL_RESULT (XGLAPI *MapMemory)(XGL_GPU_MEMORY mem, XGL_FLAGS flags, XGL_VOID** ppData);
    XGL_RESULT (XGLAPI *UnmapMemory)(XGL_GPU_MEMORY mem);
    XGL_RESULT (XGLAPI *PinSystemMemory)(XGL_DEVICE device, const XGL_VOID* pSysMem, XGL_SIZE memSize, XGL_GPU_MEMORY* pMem);
    XGL_RESULT (XGLAPI *RemapVirtualMemoryPages)(XGL_DEVICE device, XGL_UINT rangeCount, const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pRanges, XGL_UINT preWaitSemaphoreCount, const XGL_QUEUE_SEMAPHORE* pPreWaitSemaphores, XGL_UINT postSignalSemaphoreCount, const XGL_QUEUE_SEMAPHORE* pPostSignalSemaphores);
    XGL_RESULT (XGLAPI *GetMultiGpuCompatibility)(XGL_PHYSICAL_GPU gpu0, XGL_PHYSICAL_GPU gpu1, XGL_GPU_COMPATIBILITY_INFO* pInfo);
    XGL_RESULT (XGLAPI *OpenSharedMemory)(XGL_DEVICE device, const XGL_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem);
    XGL_RESULT (XGLAPI *OpenSharedQueueSemaphore)(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo, XGL_QUEUE_SEMAPHORE* pSemaphore);
    XGL_RESULT (XGLAPI *OpenPeerMemory)(XGL_DEVICE device, const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem);
    XGL_RESULT (XGLAPI *OpenPeerImage)(XGL_DEVICE device, const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem);
    XGL_RESULT (XGLAPI *DestroyObject)(XGL_OBJECT object);
    XGL_RESULT (XGLAPI *GetObjectInfo)(XGL_BASE_OBJECT object, XGL_OBJECT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData);
    XGL_RESULT (XGLAPI *BindObjectMemory)(XGL_OBJECT object, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset);
    XGL_RESULT (XGLAPI *CreateFence)(XGL_DEVICE device, const XGL_FENCE_CREATE_INFO* pCreateInfo, XGL_FENCE* pFence);
    XGL_RESULT (XGLAPI *GetFenceStatus)(XGL_FENCE fence);
    XGL_RESULT (XGLAPI *WaitForFences)(XGL_DEVICE device, XGL_UINT fenceCount, const XGL_FENCE* pFences, XGL_BOOL waitAll, XGL_UINT64 timeout);
    XGL_RESULT (XGLAPI *CreateQueueSemaphore)(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo, XGL_QUEUE_SEMAPHORE* pSemaphore);
    XGL_RESULT (XGLAPI *SignalQueueSemaphore)(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore);
    XGL_RESULT (XGLAPI *WaitQueueSemaphore)(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore);
    XGL_RESULT (XGLAPI *CreateEvent)(XGL_DEVICE device, const XGL_EVENT_CREATE_INFO* pCreateInfo, XGL_EVENT* pEvent);
    XGL_RESULT (XGLAPI *GetEventStatus)(XGL_EVENT event);
    XGL_RESULT (XGLAPI *SetEvent)(XGL_EVENT event);
    XGL_RESULT (XGLAPI *ResetEvent)(XGL_EVENT event);
    XGL_RESULT (XGLAPI *CreateQueryPool)(XGL_DEVICE device, const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo, XGL_QUERY_POOL* pQueryPool);
    XGL_RESULT (XGLAPI *GetQueryPoolResults)(XGL_QUERY_POOL queryPool, XGL_UINT startQuery, XGL_UINT queryCount, XGL_SIZE* pDataSize, XGL_VOID* pData);
    XGL_RESULT (XGLAPI *GetFormatInfo)(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData);
    XGL_RESULT (XGLAPI *CreateImage)(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage);
    XGL_RESULT (XGLAPI *GetImageSubresourceInfo)(XGL_IMAGE image, const XGL_IMAGE_SUBRESOURCE* pSubresource, XGL_SUBRESOURCE_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData);
    XGL_RESULT (XGLAPI *CreateImageView)(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView);
    XGL_RESULT (XGLAPI *CreateColorAttachmentView)(XGL_DEVICE device, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, XGL_COLOR_ATTACHMENT_VIEW* pView);
    XGL_RESULT (XGLAPI *CreateDepthStencilView)(XGL_DEVICE device, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_VIEW* pView);
    XGL_RESULT (XGLAPI *CreateShader)(XGL_DEVICE device, const XGL_SHADER_CREATE_INFO* pCreateInfo, XGL_SHADER* pShader);
    XGL_RESULT (XGLAPI *CreateGraphicsPipeline)(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline);
    XGL_RESULT (XGLAPI *CreateComputePipeline)(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline);
    XGL_RESULT (XGLAPI *StorePipeline)(XGL_PIPELINE pipeline, XGL_SIZE* pDataSize, XGL_VOID* pData);
    XGL_RESULT (XGLAPI *LoadPipeline)(XGL_DEVICE device, XGL_SIZE dataSize, const XGL_VOID* pData, XGL_PIPELINE* pPipeline);
    XGL_RESULT (XGLAPI *CreatePipelineDelta)(XGL_DEVICE device, XGL_PIPELINE p1, XGL_PIPELINE p2, XGL_PIPELINE_DELTA* delta);
    XGL_RESULT (XGLAPI *CreateSampler)(XGL_DEVICE device, const XGL_SAMPLER_CREATE_INFO* pCreateInfo, XGL_SAMPLER* pSampler);
    XGL_RESULT (XGLAPI *CreateDescriptorSet)(XGL_DEVICE device, const XGL_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_SET* pDescriptorSet);
    XGL_VOID (XGLAPI *BeginDescriptorSetUpdate)(XGL_DESCRIPTOR_SET descriptorSet);
    XGL_VOID (XGLAPI *EndDescriptorSetUpdate)(XGL_DESCRIPTOR_SET descriptorSet);
    XGL_VOID (XGLAPI *AttachSamplerDescriptors)(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_SAMPLER* pSamplers);
    XGL_VOID (XGLAPI *AttachImageViewDescriptors)(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews);
    XGL_VOID (XGLAPI *AttachMemoryViewDescriptors)(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews);
    XGL_VOID (XGLAPI *AttachNestedDescriptors)(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets);
    XGL_VOID (XGLAPI *ClearDescriptorSetSlots)(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount);
    XGL_RESULT (XGLAPI *CreateViewportState)(XGL_DEVICE device, const XGL_VIEWPORT_STATE_CREATE_INFO* pCreateInfo, XGL_VIEWPORT_STATE_OBJECT* pState);
    XGL_RESULT (XGLAPI *CreateRasterState)(XGL_DEVICE device, const XGL_RASTER_STATE_CREATE_INFO* pCreateInfo, XGL_RASTER_STATE_OBJECT* pState);
    XGL_RESULT (XGLAPI *CreateMsaaState)(XGL_DEVICE device, const XGL_MSAA_STATE_CREATE_INFO* pCreateInfo, XGL_MSAA_STATE_OBJECT* pState);
    XGL_RESULT (XGLAPI *CreateColorBlendState)(XGL_DEVICE device, const XGL_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo, XGL_COLOR_BLEND_STATE_OBJECT* pState);
    XGL_RESULT (XGLAPI *CreateDepthStencilState)(XGL_DEVICE device, const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_STATE_OBJECT* pState);
    XGL_RESULT (XGLAPI *CreateCommandBuffer)(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, XGL_CMD_BUFFER* pCmdBuffer);
    XGL_RESULT (XGLAPI *BeginCommandBuffer)(XGL_CMD_BUFFER cmdBuffer, XGL_FLAGS flags);
    XGL_RESULT (XGLAPI *EndCommandBuffer)(XGL_CMD_BUFFER cmdBuffer);
    XGL_RESULT (XGLAPI *ResetCommandBuffer)(XGL_CMD_BUFFER cmdBuffer);
    XGL_VOID (XGLAPI *CmdBindPipeline)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE pipeline);
    XGL_VOID (XGLAPI *CmdBindPipelineDelta)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE_DELTA delta);
    XGL_VOID (XGLAPI *CmdBindStateObject)(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_STATE_OBJECT state);
    XGL_VOID (XGLAPI *CmdBindDescriptorSet)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT index, XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT slotOffset);
    XGL_VOID (XGLAPI *CmdBindDynamicMemoryView)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView);
    XGL_VOID (XGLAPI *CmdBindIndexData)(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType);
    XGL_VOID (XGLAPI *CmdBindAttachments)(XGL_CMD_BUFFER cmdBuffer, XGL_UINT colorAttachmentCount, const XGL_COLOR_ATTACHMENT_BIND_INFO* pColorAttachments, const XGL_DEPTH_STENCIL_BIND_INFO* pDepthStencilAttachment);
    XGL_VOID (XGLAPI *CmdPrepareMemoryRegions)(XGL_CMD_BUFFER cmdBuffer, XGL_UINT transitionCount, const XGL_MEMORY_STATE_TRANSITION* pStateTransitions);
    XGL_VOID (XGLAPI *CmdPrepareImages)(XGL_CMD_BUFFER cmdBuffer, XGL_UINT transitionCount, const XGL_IMAGE_STATE_TRANSITION* pStateTransitions);
    XGL_VOID (XGLAPI *CmdDraw)(XGL_CMD_BUFFER cmdBuffer, XGL_UINT firstVertex, XGL_UINT vertexCount, XGL_UINT firstInstance, XGL_UINT instanceCount);
    XGL_VOID (XGLAPI *CmdDrawIndexed)(XGL_CMD_BUFFER cmdBuffer, XGL_UINT firstIndex, XGL_UINT indexCount, XGL_INT vertexOffset, XGL_UINT firstInstance, XGL_UINT instanceCount);
    XGL_VOID (XGLAPI *CmdDrawIndirect)(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_UINT32 count, XGL_UINT32 stride);
    XGL_VOID (XGLAPI *CmdDrawIndexedIndirect)(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_UINT32 count, XGL_UINT32 stride);
    XGL_VOID (XGLAPI *CmdDispatch)(XGL_CMD_BUFFER cmdBuffer, XGL_UINT x, XGL_UINT y, XGL_UINT z);
    XGL_VOID (XGLAPI *CmdDispatchIndirect)(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset);
    XGL_VOID (XGLAPI *CmdCopyMemory)(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY srcMem, XGL_GPU_MEMORY destMem, XGL_UINT regionCount, const XGL_MEMORY_COPY* pRegions);
    XGL_VOID (XGLAPI *CmdCopyImage)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, XGL_UINT regionCount, const XGL_IMAGE_COPY* pRegions);
    XGL_VOID (XGLAPI *CmdCopyMemoryToImage)(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY srcMem, XGL_IMAGE destImage, XGL_UINT regionCount, const XGL_MEMORY_IMAGE_COPY* pRegions);
    XGL_VOID (XGLAPI *CmdCopyImageToMemory)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_GPU_MEMORY destMem, XGL_UINT regionCount, const XGL_MEMORY_IMAGE_COPY* pRegions);
    XGL_VOID (XGLAPI *CmdCloneImageData)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_STATE srcImageState, XGL_IMAGE destImage, XGL_IMAGE_STATE destImageState);
    XGL_VOID (XGLAPI *CmdUpdateMemory)(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const XGL_UINT32* pData);
    XGL_VOID (XGLAPI *CmdFillMemory)(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, XGL_UINT32 data);
    XGL_VOID (XGLAPI *CmdClearColorImage)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const XGL_FLOAT color[4], XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
    XGL_VOID (XGLAPI *CmdClearColorImageRaw)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const XGL_UINT32 color[4], XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
    XGL_VOID (XGLAPI *CmdClearDepthStencil)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, XGL_FLOAT depth, XGL_UINT32 stencil, XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
    XGL_VOID (XGLAPI *CmdResolveImage)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, XGL_UINT rectCount, const XGL_IMAGE_RESOLVE* pRects);
    XGL_VOID (XGLAPI *CmdSetEvent)(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event);
    XGL_VOID (XGLAPI *CmdResetEvent)(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event);
    XGL_VOID (XGLAPI *CmdMemoryAtomic)(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_UINT64 srcData, XGL_ATOMIC_OP atomicOp);
    XGL_VOID (XGLAPI *CmdBeginQuery)(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT slot, XGL_FLAGS flags);
    XGL_VOID (XGLAPI *CmdEndQuery)(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT slot);
    XGL_VOID (XGLAPI *CmdResetQueryPool)(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT startQuery, XGL_UINT queryCount);
    XGL_VOID (XGLAPI *CmdWriteTimestamp)(XGL_CMD_BUFFER cmdBuffer, XGL_TIMESTAMP_TYPE timestampType, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset);
    XGL_VOID (XGLAPI *CmdInitAtomicCounters)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, const XGL_UINT32* pData);
    XGL_VOID (XGLAPI *CmdLoadAtomicCounters)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, XGL_GPU_MEMORY srcMem, XGL_GPU_SIZE srcOffset);
    XGL_VOID (XGLAPI *CmdSaveAtomicCounters)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset);
    XGL_RESULT (XGLAPI *DbgSetValidationLevel)(XGL_DEVICE device, XGL_VALIDATION_LEVEL validationLevel);
    XGL_RESULT (XGLAPI *DbgRegisterMsgCallback)(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData);
    XGL_RESULT (XGLAPI *DbgUnregisterMsgCallback)(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);
    XGL_RESULT (XGLAPI *DbgSetMessageFilter)(XGL_DEVICE device, XGL_INT msgCode, XGL_DBG_MSG_FILTER filter);
    XGL_RESULT (XGLAPI *DbgSetObjectTag)(XGL_BASE_OBJECT object, XGL_SIZE tagSize, const XGL_VOID* pTag);
    XGL_RESULT (XGLAPI *DbgSetGlobalOption)(XGL_DBG_GLOBAL_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData);
    XGL_RESULT (XGLAPI *DbgSetDeviceOption)(XGL_DEVICE device, XGL_DBG_DEVICE_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData);
    XGL_VOID (XGLAPI *CmdDbgMarkerBegin)(XGL_CMD_BUFFER cmdBuffer, const XGL_CHAR* pMarker);
    XGL_VOID (XGLAPI *CmdDbgMarkerEnd)(XGL_CMD_BUFFER cmdBuffer);
};

/**
 * Typedefs for all XGL API entrypoint functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Base class for device drivers.
 */
#define MAX_DRIVER_NAME 64

struct _xgl_device {
    /* MUST be first element of structure */
    struct icd_dispatch_table *exec;

    struct icd_dispatch_table xgl;
    struct icd_dispatch_table validation;

    int fd;                 /* file descriptor of render-node */
    uint32_t dev_id;
    uint32_t ven_id;
    char device_path[XGL_MAX_PHYSICAL_GPU_NAME];
    char driver_name[MAX_DRIVER_NAME];
    void *driver_handle;
};


struct _xgl_config {
    XGL_BOOL UserDefinedAlloc;
    XGL_ALLOC_CALLBACKS xgl_alloc;

    XGL_UINT num_gpus;
    struct _xgl_device gpu_info[XGL_MAX_PHYSICAL_GPUS];
    XGL_PHYSICAL_GPU render_node_list[XGL_MAX_PHYSICAL_GPUS];
};

extern struct _xgl_config icd_data;

#ifdef __cplusplus
}                               // extern "C"
#endif                          // __cplusplus
#endif                          /* __DRIVER_H_INCLUDED__ */
