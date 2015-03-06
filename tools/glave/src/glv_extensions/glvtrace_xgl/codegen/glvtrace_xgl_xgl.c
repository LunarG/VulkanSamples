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

#include "glv_platform.h"
#include "glv_common.h"
#include "glvtrace_xgl_helpers.h"
#include "glvtrace_xgl_xgl.h"
#include "glvtrace_xgl_xgldbg.h"
#include "glvtrace_xgl_xglwsix11ext.h"
#include "glv_interconnect.h"
#include "glv_filelike.h"
#include "xgl_struct_size_helper.h"
#ifdef WIN32
#include "mhook/mhook-lib/mhook.h"
#endif
#include "glv_trace_packet_utils.h"
#include <stdio.h>


XGL_RESULT( XGLAPI * real_xglCreateInstance)(
    const XGL_APPLICATION_INFO* pAppInfo,
    const XGL_ALLOC_CALLBACKS* pAllocCb,
    XGL_INSTANCE* pInstance) = xglCreateInstance;

XGL_RESULT( XGLAPI * real_xglDestroyInstance)(
    XGL_INSTANCE instance) = xglDestroyInstance;

XGL_RESULT( XGLAPI * real_xglEnumerateGpus)(
    XGL_INSTANCE instance,
    uint32_t maxGpus,
    uint32_t* pGpuCount,
    XGL_PHYSICAL_GPU* pGpus) = xglEnumerateGpus;

XGL_RESULT( XGLAPI * real_xglGetGpuInfo)(
    XGL_PHYSICAL_GPU gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE infoType,
    size_t* pDataSize,
    void* pData) = xglGetGpuInfo;

void*( XGLAPI * real_xglGetProcAddr)(
    XGL_PHYSICAL_GPU gpu,
    const char* pName) = xglGetProcAddr;

XGL_RESULT( XGLAPI * real_xglCreateDevice)(
    XGL_PHYSICAL_GPU gpu,
    const XGL_DEVICE_CREATE_INFO* pCreateInfo,
    XGL_DEVICE* pDevice) = xglCreateDevice;

XGL_RESULT( XGLAPI * real_xglDestroyDevice)(
    XGL_DEVICE device) = xglDestroyDevice;

XGL_RESULT( XGLAPI * real_xglGetExtensionSupport)(
    XGL_PHYSICAL_GPU gpu,
    const char* pExtName) = xglGetExtensionSupport;

XGL_RESULT( XGLAPI * real_xglEnumerateLayers)(
    XGL_PHYSICAL_GPU gpu,
    size_t maxLayerCount,
    size_t maxStringSize,
    size_t* pOutLayerCount,
    char* const* pOutLayers,
    void* pReserved) = xglEnumerateLayers;

XGL_RESULT( XGLAPI * real_xglGetDeviceQueue)(
    XGL_DEVICE device,
    uint32_t queueNodeIndex,
    uint32_t queueIndex,
    XGL_QUEUE* pQueue) = xglGetDeviceQueue;

XGL_RESULT( XGLAPI * real_xglQueueSubmit)(
    XGL_QUEUE queue,
    uint32_t cmdBufferCount,
    const XGL_CMD_BUFFER* pCmdBuffers,
    uint32_t memRefCount,
    const XGL_MEMORY_REF* pMemRefs,
    XGL_FENCE fence) = xglQueueSubmit;

XGL_RESULT( XGLAPI * real_xglQueueSetGlobalMemReferences)(
    XGL_QUEUE queue,
    uint32_t memRefCount,
    const XGL_MEMORY_REF* pMemRefs) = xglQueueSetGlobalMemReferences;

XGL_RESULT( XGLAPI * real_xglQueueWaitIdle)(
    XGL_QUEUE queue) = xglQueueWaitIdle;

XGL_RESULT( XGLAPI * real_xglDeviceWaitIdle)(
    XGL_DEVICE device) = xglDeviceWaitIdle;

XGL_RESULT( XGLAPI * real_xglAllocMemory)(
    XGL_DEVICE device,
    const XGL_MEMORY_ALLOC_INFO* pAllocInfo,
    XGL_GPU_MEMORY* pMem) = xglAllocMemory;

XGL_RESULT( XGLAPI * real_xglFreeMemory)(
    XGL_GPU_MEMORY mem) = xglFreeMemory;

XGL_RESULT( XGLAPI * real_xglSetMemoryPriority)(
    XGL_GPU_MEMORY mem,
    XGL_MEMORY_PRIORITY priority) = xglSetMemoryPriority;

XGL_RESULT( XGLAPI * real_xglMapMemory)(
    XGL_GPU_MEMORY mem,
    XGL_FLAGS flags,
    void** ppData) = xglMapMemory;

XGL_RESULT( XGLAPI * real_xglUnmapMemory)(
    XGL_GPU_MEMORY mem) = xglUnmapMemory;

XGL_RESULT( XGLAPI * real_xglPinSystemMemory)(
    XGL_DEVICE device,
    const void* pSysMem,
    size_t memSize,
    XGL_GPU_MEMORY* pMem) = xglPinSystemMemory;

XGL_RESULT( XGLAPI * real_xglGetMultiGpuCompatibility)(
    XGL_PHYSICAL_GPU gpu0,
    XGL_PHYSICAL_GPU gpu1,
    XGL_GPU_COMPATIBILITY_INFO* pInfo) = xglGetMultiGpuCompatibility;

XGL_RESULT( XGLAPI * real_xglOpenSharedMemory)(
    XGL_DEVICE device,
    const XGL_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY* pMem) = xglOpenSharedMemory;

XGL_RESULT( XGLAPI * real_xglOpenSharedQueueSemaphore)(
    XGL_DEVICE device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo,
    XGL_QUEUE_SEMAPHORE* pSemaphore) = xglOpenSharedQueueSemaphore;

XGL_RESULT( XGLAPI * real_xglOpenPeerMemory)(
    XGL_DEVICE device,
    const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY* pMem) = xglOpenPeerMemory;

XGL_RESULT( XGLAPI * real_xglOpenPeerImage)(
    XGL_DEVICE device,
    const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo,
    XGL_IMAGE* pImage,
    XGL_GPU_MEMORY* pMem) = xglOpenPeerImage;

XGL_RESULT( XGLAPI * real_xglDestroyObject)(
    XGL_OBJECT object) = xglDestroyObject;

XGL_RESULT( XGLAPI * real_xglGetObjectInfo)(
    XGL_BASE_OBJECT object,
    XGL_OBJECT_INFO_TYPE infoType,
    size_t* pDataSize,
    void* pData) = xglGetObjectInfo;

XGL_RESULT( XGLAPI * real_xglBindObjectMemory)(
    XGL_OBJECT object,
    uint32_t allocationIdx,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE offset) = xglBindObjectMemory;

XGL_RESULT( XGLAPI * real_xglBindObjectMemoryRange)(
    XGL_OBJECT object,
    uint32_t allocationIdx,
    XGL_GPU_SIZE rangeOffset,
    XGL_GPU_SIZE rangeSize,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE memOffset) = xglBindObjectMemoryRange;

XGL_RESULT( XGLAPI * real_xglBindImageMemoryRange)(
    XGL_IMAGE image,
    uint32_t allocationIdx,
    const XGL_IMAGE_MEMORY_BIND_INFO* bindInfo,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE memOffset) = xglBindImageMemoryRange;

XGL_RESULT( XGLAPI * real_xglCreateFence)(
    XGL_DEVICE device,
    const XGL_FENCE_CREATE_INFO* pCreateInfo,
    XGL_FENCE* pFence) = xglCreateFence;

XGL_RESULT( XGLAPI * real_xglGetFenceStatus)(
    XGL_FENCE fence) = xglGetFenceStatus;

XGL_RESULT( XGLAPI * real_xglWaitForFences)(
    XGL_DEVICE device,
    uint32_t fenceCount,
    const XGL_FENCE* pFences,
    bool32_t waitAll,
    uint64_t timeout) = xglWaitForFences;

XGL_RESULT( XGLAPI * real_xglCreateQueueSemaphore)(
    XGL_DEVICE device,
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo,
    XGL_QUEUE_SEMAPHORE* pSemaphore) = xglCreateQueueSemaphore;

XGL_RESULT( XGLAPI * real_xglSignalQueueSemaphore)(
    XGL_QUEUE queue,
    XGL_QUEUE_SEMAPHORE semaphore) = xglSignalQueueSemaphore;

XGL_RESULT( XGLAPI * real_xglWaitQueueSemaphore)(
    XGL_QUEUE queue,
    XGL_QUEUE_SEMAPHORE semaphore) = xglWaitQueueSemaphore;

XGL_RESULT( XGLAPI * real_xglCreateEvent)(
    XGL_DEVICE device,
    const XGL_EVENT_CREATE_INFO* pCreateInfo,
    XGL_EVENT* pEvent) = xglCreateEvent;

XGL_RESULT( XGLAPI * real_xglGetEventStatus)(
    XGL_EVENT event) = xglGetEventStatus;

XGL_RESULT( XGLAPI * real_xglSetEvent)(
    XGL_EVENT event) = xglSetEvent;

XGL_RESULT( XGLAPI * real_xglResetEvent)(
    XGL_EVENT event) = xglResetEvent;

XGL_RESULT( XGLAPI * real_xglCreateQueryPool)(
    XGL_DEVICE device,
    const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo,
    XGL_QUERY_POOL* pQueryPool) = xglCreateQueryPool;

XGL_RESULT( XGLAPI * real_xglGetQueryPoolResults)(
    XGL_QUERY_POOL queryPool,
    uint32_t startQuery,
    uint32_t queryCount,
    size_t* pDataSize,
    void* pData) = xglGetQueryPoolResults;

XGL_RESULT( XGLAPI * real_xglGetFormatInfo)(
    XGL_DEVICE device,
    XGL_FORMAT format,
    XGL_FORMAT_INFO_TYPE infoType,
    size_t* pDataSize,
    void* pData) = xglGetFormatInfo;

XGL_RESULT( XGLAPI * real_xglCreateBuffer)(
    XGL_DEVICE device,
    const XGL_BUFFER_CREATE_INFO* pCreateInfo,
    XGL_BUFFER* pBuffer) = xglCreateBuffer;

XGL_RESULT( XGLAPI * real_xglCreateBufferView)(
    XGL_DEVICE device,
    const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo,
    XGL_BUFFER_VIEW* pView) = xglCreateBufferView;

XGL_RESULT( XGLAPI * real_xglCreateImage)(
    XGL_DEVICE device,
    const XGL_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE* pImage) = xglCreateImage;

XGL_RESULT( XGLAPI * real_xglSetFastClearColor)(
    XGL_IMAGE image,
    const float color[4]) = xglSetFastClearColor;

XGL_RESULT( XGLAPI * real_xglSetFastClearDepth)(
    XGL_IMAGE image,
    float depth) = xglSetFastClearDepth;

XGL_RESULT( XGLAPI * real_xglGetImageSubresourceInfo)(
    XGL_IMAGE image,
    const XGL_IMAGE_SUBRESOURCE* pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE infoType,
    size_t* pDataSize,
    void* pData) = xglGetImageSubresourceInfo;

XGL_RESULT( XGLAPI * real_xglCreateImageView)(
    XGL_DEVICE device,
    const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
    XGL_IMAGE_VIEW* pView) = xglCreateImageView;

XGL_RESULT( XGLAPI * real_xglCreateColorAttachmentView)(
    XGL_DEVICE device,
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW* pView) = xglCreateColorAttachmentView;

XGL_RESULT( XGLAPI * real_xglCreateDepthStencilView)(
    XGL_DEVICE device,
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo,
    XGL_DEPTH_STENCIL_VIEW* pView) = xglCreateDepthStencilView;

XGL_RESULT( XGLAPI * real_xglCreateShader)(
    XGL_DEVICE device,
    const XGL_SHADER_CREATE_INFO* pCreateInfo,
    XGL_SHADER* pShader) = xglCreateShader;

XGL_RESULT( XGLAPI * real_xglCreateGraphicsPipeline)(
    XGL_DEVICE device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE* pPipeline) = xglCreateGraphicsPipeline;

XGL_RESULT( XGLAPI * real_xglCreateComputePipeline)(
    XGL_DEVICE device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE* pPipeline) = xglCreateComputePipeline;

XGL_RESULT( XGLAPI * real_xglStorePipeline)(
    XGL_PIPELINE pipeline,
    size_t* pDataSize,
    void* pData) = xglStorePipeline;

XGL_RESULT( XGLAPI * real_xglLoadPipeline)(
    XGL_DEVICE device,
    size_t dataSize,
    const void* pData,
    XGL_PIPELINE* pPipeline) = xglLoadPipeline;

XGL_RESULT( XGLAPI * real_xglCreatePipelineDelta)(
    XGL_DEVICE device,
    XGL_PIPELINE p1,
    XGL_PIPELINE p2,
    XGL_PIPELINE_DELTA* delta) = xglCreatePipelineDelta;

XGL_RESULT( XGLAPI * real_xglCreateSampler)(
    XGL_DEVICE device,
    const XGL_SAMPLER_CREATE_INFO* pCreateInfo,
    XGL_SAMPLER* pSampler) = xglCreateSampler;

XGL_RESULT( XGLAPI * real_xglCreateDescriptorSetLayout)(
    XGL_DEVICE device,
    XGL_FLAGS stageFlags,
    const uint32_t* pSetBindPoints,
    XGL_DESCRIPTOR_SET_LAYOUT priorSetLayout,
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList,
    XGL_DESCRIPTOR_SET_LAYOUT* pSetLayout) = xglCreateDescriptorSetLayout;

XGL_RESULT( XGLAPI * real_xglBeginDescriptorRegionUpdate)(
    XGL_DEVICE device,
    XGL_DESCRIPTOR_UPDATE_MODE updateMode) = xglBeginDescriptorRegionUpdate;

XGL_RESULT( XGLAPI * real_xglEndDescriptorRegionUpdate)(
    XGL_DEVICE device,
    XGL_CMD_BUFFER cmd) = xglEndDescriptorRegionUpdate;

XGL_RESULT( XGLAPI * real_xglCreateDescriptorRegion)(
    XGL_DEVICE device,
    XGL_DESCRIPTOR_REGION_USAGE regionUsage,
    uint32_t maxSets,
    const XGL_DESCRIPTOR_REGION_CREATE_INFO* pCreateInfo,
    XGL_DESCRIPTOR_REGION* pDescriptorRegion) = xglCreateDescriptorRegion;

XGL_RESULT( XGLAPI * real_xglClearDescriptorRegion)(
    XGL_DESCRIPTOR_REGION descriptorRegion) = xglClearDescriptorRegion;

XGL_RESULT( XGLAPI * real_xglAllocDescriptorSets)(
    XGL_DESCRIPTOR_REGION descriptorRegion,
    XGL_DESCRIPTOR_SET_USAGE setUsage,
    uint32_t count,
    const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayouts,
    XGL_DESCRIPTOR_SET* pDescriptorSets,
    uint32_t* pCount) = xglAllocDescriptorSets;

void( XGLAPI * real_xglClearDescriptorSets)(
    XGL_DESCRIPTOR_REGION descriptorRegion,
    uint32_t count,
    const XGL_DESCRIPTOR_SET* pDescriptorSets) = xglClearDescriptorSets;

void( XGLAPI * real_xglUpdateDescriptors)(
    XGL_DESCRIPTOR_SET descriptorSet,
    const void* pUpdateChain) = xglUpdateDescriptors;

XGL_RESULT( XGLAPI * real_xglCreateDynamicViewportState)(
    XGL_DEVICE device,
    const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_VP_STATE_OBJECT* pState) = xglCreateDynamicViewportState;

XGL_RESULT( XGLAPI * real_xglCreateDynamicRasterState)(
    XGL_DEVICE device,
    const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_RS_STATE_OBJECT* pState) = xglCreateDynamicRasterState;

XGL_RESULT( XGLAPI * real_xglCreateDynamicColorBlendState)(
    XGL_DEVICE device,
    const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_CB_STATE_OBJECT* pState) = xglCreateDynamicColorBlendState;

XGL_RESULT( XGLAPI * real_xglCreateDynamicDepthStencilState)(
    XGL_DEVICE device,
    const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_DS_STATE_OBJECT* pState) = xglCreateDynamicDepthStencilState;

XGL_RESULT( XGLAPI * real_xglCreateCommandBuffer)(
    XGL_DEVICE device,
    const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo,
    XGL_CMD_BUFFER* pCmdBuffer) = xglCreateCommandBuffer;

XGL_RESULT( XGLAPI * real_xglBeginCommandBuffer)(
    XGL_CMD_BUFFER cmdBuffer,
    const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo) = xglBeginCommandBuffer;

XGL_RESULT( XGLAPI * real_xglEndCommandBuffer)(
    XGL_CMD_BUFFER cmdBuffer) = xglEndCommandBuffer;

XGL_RESULT( XGLAPI * real_xglResetCommandBuffer)(
    XGL_CMD_BUFFER cmdBuffer) = xglResetCommandBuffer;

void( XGLAPI * real_xglCmdBindPipeline)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    XGL_PIPELINE pipeline) = xglCmdBindPipeline;

void( XGLAPI * real_xglCmdBindPipelineDelta)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    XGL_PIPELINE_DELTA delta) = xglCmdBindPipelineDelta;

void( XGLAPI * real_xglCmdBindDynamicStateObject)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_STATE_BIND_POINT stateBindPoint,
    XGL_DYNAMIC_STATE_OBJECT state) = xglCmdBindDynamicStateObject;

void( XGLAPI * real_xglCmdBindDescriptorSet)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    XGL_DESCRIPTOR_SET descriptorSet,
    const uint32_t* pUserData) = xglCmdBindDescriptorSet;

void( XGLAPI * real_xglCmdBindVertexBuffer)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset,
    uint32_t binding) = xglCmdBindVertexBuffer;

void( XGLAPI * real_xglCmdBindIndexBuffer)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset,
    XGL_INDEX_TYPE indexType) = xglCmdBindIndexBuffer;

void( XGLAPI * real_xglCmdDraw)(
    XGL_CMD_BUFFER cmdBuffer,
    uint32_t firstVertex,
    uint32_t vertexCount,
    uint32_t firstInstance,
    uint32_t instanceCount) = xglCmdDraw;

void( XGLAPI * real_xglCmdDrawIndexed)(
    XGL_CMD_BUFFER cmdBuffer,
    uint32_t firstIndex,
    uint32_t indexCount,
    int32_t vertexOffset,
    uint32_t firstInstance,
    uint32_t instanceCount) = xglCmdDrawIndexed;

void( XGLAPI * real_xglCmdDrawIndirect)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset,
    uint32_t count,
    uint32_t stride) = xglCmdDrawIndirect;

void( XGLAPI * real_xglCmdDrawIndexedIndirect)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset,
    uint32_t count,
    uint32_t stride) = xglCmdDrawIndexedIndirect;

void( XGLAPI * real_xglCmdDispatch)(
    XGL_CMD_BUFFER cmdBuffer,
    uint32_t x,
    uint32_t y,
    uint32_t z) = xglCmdDispatch;

void( XGLAPI * real_xglCmdDispatchIndirect)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset) = xglCmdDispatchIndirect;

void( XGLAPI * real_xglCmdCopyBuffer)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER srcBuffer,
    XGL_BUFFER destBuffer,
    uint32_t regionCount,
    const XGL_BUFFER_COPY* pRegions) = xglCmdCopyBuffer;

void( XGLAPI * real_xglCmdCopyImage)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE srcImage,
    XGL_IMAGE destImage,
    uint32_t regionCount,
    const XGL_IMAGE_COPY* pRegions) = xglCmdCopyImage;

void( XGLAPI * real_xglCmdCopyBufferToImage)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER srcBuffer,
    XGL_IMAGE destImage,
    uint32_t regionCount,
    const XGL_BUFFER_IMAGE_COPY* pRegions) = xglCmdCopyBufferToImage;

void( XGLAPI * real_xglCmdCopyImageToBuffer)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE srcImage,
    XGL_BUFFER destBuffer,
    uint32_t regionCount,
    const XGL_BUFFER_IMAGE_COPY* pRegions) = xglCmdCopyImageToBuffer;

void( XGLAPI * real_xglCmdCloneImageData)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE srcImage,
    XGL_IMAGE_LAYOUT srcImageLayout,
    XGL_IMAGE destImage,
    XGL_IMAGE_LAYOUT destImageLayout) = xglCmdCloneImageData;

void( XGLAPI * real_xglCmdUpdateBuffer)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER destBuffer,
    XGL_GPU_SIZE destOffset,
    XGL_GPU_SIZE dataSize,
    const uint32_t* pData) = xglCmdUpdateBuffer;

void( XGLAPI * real_xglCmdFillBuffer)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER destBuffer,
    XGL_GPU_SIZE destOffset,
    XGL_GPU_SIZE fillSize,
    uint32_t data) = xglCmdFillBuffer;

void( XGLAPI * real_xglCmdClearColorImage)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE image,
    const float color[4],
    uint32_t rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges) = xglCmdClearColorImage;

void( XGLAPI * real_xglCmdClearColorImageRaw)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE image,
    const uint32_t color[4],
    uint32_t rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges) = xglCmdClearColorImageRaw;

void( XGLAPI * real_xglCmdClearDepthStencil)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE image,
    float depth,
    uint32_t stencil,
    uint32_t rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges) = xglCmdClearDepthStencil;

void( XGLAPI * real_xglCmdResolveImage)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE srcImage,
    XGL_IMAGE destImage,
    uint32_t rectCount,
    const XGL_IMAGE_RESOLVE* pRects) = xglCmdResolveImage;

void( XGLAPI * real_xglCmdSetEvent)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT event,
    XGL_SET_EVENT pipeEvent) = xglCmdSetEvent;

void( XGLAPI * real_xglCmdResetEvent)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT event) = xglCmdResetEvent;

void( XGLAPI * real_xglCmdWaitEvents)(
    XGL_CMD_BUFFER cmdBuffer,
    const XGL_EVENT_WAIT_INFO* pWaitInfo) = xglCmdWaitEvents;

void( XGLAPI * real_xglCmdPipelineBarrier)(
    XGL_CMD_BUFFER cmdBuffer,
    const XGL_PIPELINE_BARRIER* pBarrier) = xglCmdPipelineBarrier;

void( XGLAPI * real_xglCmdBeginQuery)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    uint32_t slot,
    XGL_FLAGS flags) = xglCmdBeginQuery;

void( XGLAPI * real_xglCmdEndQuery)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    uint32_t slot) = xglCmdEndQuery;

void( XGLAPI * real_xglCmdResetQueryPool)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    uint32_t startQuery,
    uint32_t queryCount) = xglCmdResetQueryPool;

void( XGLAPI * real_xglCmdWriteTimestamp)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_TIMESTAMP_TYPE timestampType,
    XGL_BUFFER destBuffer,
    XGL_GPU_SIZE destOffset) = xglCmdWriteTimestamp;

void( XGLAPI * real_xglCmdInitAtomicCounters)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    uint32_t startCounter,
    uint32_t counterCount,
    const uint32_t* pData) = xglCmdInitAtomicCounters;

void( XGLAPI * real_xglCmdLoadAtomicCounters)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    uint32_t startCounter,
    uint32_t counterCount,
    XGL_BUFFER srcBuffer,
    XGL_GPU_SIZE srcOffset) = xglCmdLoadAtomicCounters;

void( XGLAPI * real_xglCmdSaveAtomicCounters)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    uint32_t startCounter,
    uint32_t counterCount,
    XGL_BUFFER destBuffer,
    XGL_GPU_SIZE destOffset) = xglCmdSaveAtomicCounters;

XGL_RESULT( XGLAPI * real_xglCreateFramebuffer)(
    XGL_DEVICE device,
    const XGL_FRAMEBUFFER_CREATE_INFO* pCreateInfo,
    XGL_FRAMEBUFFER* pFramebuffer) = xglCreateFramebuffer;

XGL_RESULT( XGLAPI * real_xglCreateRenderPass)(
    XGL_DEVICE device,
    const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo,
    XGL_RENDER_PASS* pRenderPass) = xglCreateRenderPass;

void( XGLAPI * real_xglCmdBeginRenderPass)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_RENDER_PASS renderPass) = xglCmdBeginRenderPass;

void( XGLAPI * real_xglCmdEndRenderPass)(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_RENDER_PASS renderPass) = xglCmdEndRenderPass;

// declared as extern in glvtrace_xgl_helpers.h
BOOL isHooked = FALSE;

void AttachHooks()
{
   BOOL hookSuccess = TRUE;
#if defined(WIN32)
    Mhook_BeginMultiOperation(FALSE);
    if (real_xglCreateInstance != NULL)
    {
        isHooked = TRUE;
        hookSuccess = Mhook_SetHook((PVOID*)&real_xglCreateInstance, hooked_xglCreateInstance);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglDestroyInstance, hooked_xglDestroyInstance);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglEnumerateGpus, hooked_xglEnumerateGpus);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetGpuInfo, hooked_xglGetGpuInfo);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetProcAddr, hooked_xglGetProcAddr);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDevice, hooked_xglCreateDevice);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglDestroyDevice, hooked_xglDestroyDevice);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetExtensionSupport, hooked_xglGetExtensionSupport);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglEnumerateLayers, hooked_xglEnumerateLayers);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetDeviceQueue, hooked_xglGetDeviceQueue);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglQueueSubmit, hooked_xglQueueSubmit);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglQueueSetGlobalMemReferences, hooked_xglQueueSetGlobalMemReferences);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglQueueWaitIdle, hooked_xglQueueWaitIdle);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglDeviceWaitIdle, hooked_xglDeviceWaitIdle);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglAllocMemory, hooked_xglAllocMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglFreeMemory, hooked_xglFreeMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglSetMemoryPriority, hooked_xglSetMemoryPriority);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglMapMemory, hooked_xglMapMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglUnmapMemory, hooked_xglUnmapMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglPinSystemMemory, hooked_xglPinSystemMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetMultiGpuCompatibility, hooked_xglGetMultiGpuCompatibility);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglOpenSharedMemory, hooked_xglOpenSharedMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglOpenSharedQueueSemaphore, hooked_xglOpenSharedQueueSemaphore);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglOpenPeerMemory, hooked_xglOpenPeerMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglOpenPeerImage, hooked_xglOpenPeerImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglDestroyObject, hooked_xglDestroyObject);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglGetObjectInfo, hooked_xglGetObjectInfo);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglBindObjectMemory, hooked_xglBindObjectMemory);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglBindObjectMemoryRange, hooked_xglBindObjectMemoryRange);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglBindImageMemoryRange, hooked_xglBindImageMemoryRange);
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
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateBuffer, hooked_xglCreateBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateBufferView, hooked_xglCreateBufferView);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateImage, hooked_xglCreateImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglSetFastClearColor, hooked_xglSetFastClearColor);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglSetFastClearDepth, hooked_xglSetFastClearDepth);
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
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDescriptorSetLayout, hooked_xglCreateDescriptorSetLayout);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglBeginDescriptorRegionUpdate, hooked_xglBeginDescriptorRegionUpdate);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglEndDescriptorRegionUpdate, hooked_xglEndDescriptorRegionUpdate);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDescriptorRegion, hooked_xglCreateDescriptorRegion);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglClearDescriptorRegion, hooked_xglClearDescriptorRegion);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglAllocDescriptorSets, hooked_xglAllocDescriptorSets);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglClearDescriptorSets, hooked_xglClearDescriptorSets);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglUpdateDescriptors, hooked_xglUpdateDescriptors);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDynamicViewportState, hooked_xglCreateDynamicViewportState);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDynamicRasterState, hooked_xglCreateDynamicRasterState);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDynamicColorBlendState, hooked_xglCreateDynamicColorBlendState);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateDynamicDepthStencilState, hooked_xglCreateDynamicDepthStencilState);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateCommandBuffer, hooked_xglCreateCommandBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglBeginCommandBuffer, hooked_xglBeginCommandBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglEndCommandBuffer, hooked_xglEndCommandBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglResetCommandBuffer, hooked_xglResetCommandBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindPipeline, hooked_xglCmdBindPipeline);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindPipelineDelta, hooked_xglCmdBindPipelineDelta);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindDynamicStateObject, hooked_xglCmdBindDynamicStateObject);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindDescriptorSet, hooked_xglCmdBindDescriptorSet);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindVertexBuffer, hooked_xglCmdBindVertexBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBindIndexBuffer, hooked_xglCmdBindIndexBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDraw, hooked_xglCmdDraw);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDrawIndexed, hooked_xglCmdDrawIndexed);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDrawIndirect, hooked_xglCmdDrawIndirect);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDrawIndexedIndirect, hooked_xglCmdDrawIndexedIndirect);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDispatch, hooked_xglCmdDispatch);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdDispatchIndirect, hooked_xglCmdDispatchIndirect);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCopyBuffer, hooked_xglCmdCopyBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCopyImage, hooked_xglCmdCopyImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCopyBufferToImage, hooked_xglCmdCopyBufferToImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCopyImageToBuffer, hooked_xglCmdCopyImageToBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdCloneImageData, hooked_xglCmdCloneImageData);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdUpdateBuffer, hooked_xglCmdUpdateBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdFillBuffer, hooked_xglCmdFillBuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdClearColorImage, hooked_xglCmdClearColorImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdClearColorImageRaw, hooked_xglCmdClearColorImageRaw);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdClearDepthStencil, hooked_xglCmdClearDepthStencil);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdResolveImage, hooked_xglCmdResolveImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdSetEvent, hooked_xglCmdSetEvent);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdResetEvent, hooked_xglCmdResetEvent);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdWaitEvents, hooked_xglCmdWaitEvents);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdPipelineBarrier, hooked_xglCmdPipelineBarrier);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBeginQuery, hooked_xglCmdBeginQuery);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdEndQuery, hooked_xglCmdEndQuery);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdResetQueryPool, hooked_xglCmdResetQueryPool);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdWriteTimestamp, hooked_xglCmdWriteTimestamp);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdInitAtomicCounters, hooked_xglCmdInitAtomicCounters);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdLoadAtomicCounters, hooked_xglCmdLoadAtomicCounters);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdSaveAtomicCounters, hooked_xglCmdSaveAtomicCounters);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateFramebuffer, hooked_xglCreateFramebuffer);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCreateRenderPass, hooked_xglCreateRenderPass);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdBeginRenderPass, hooked_xglCmdBeginRenderPass);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglCmdEndRenderPass, hooked_xglCmdEndRenderPass);
    }

    if (!hookSuccess)
    {
        glv_LogError("Failed to hook XGL.");
    }

    Mhook_EndMultiOperation();

#elif defined(__linux__)
    if (real_xglCreateInstance == xglCreateInstance)
        hookSuccess = glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateInstance,"xglCreateInstance");
    isHooked = TRUE;
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglDestroyInstance, "xglDestroyInstance");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglEnumerateGpus, "xglEnumerateGpus");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetGpuInfo, "xglGetGpuInfo");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetProcAddr, "xglGetProcAddr");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDevice, "xglCreateDevice");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglDestroyDevice, "xglDestroyDevice");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetExtensionSupport, "xglGetExtensionSupport");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglEnumerateLayers, "xglEnumerateLayers");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetDeviceQueue, "xglGetDeviceQueue");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglQueueSubmit, "xglQueueSubmit");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglQueueSetGlobalMemReferences, "xglQueueSetGlobalMemReferences");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglQueueWaitIdle, "xglQueueWaitIdle");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglDeviceWaitIdle, "xglDeviceWaitIdle");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglAllocMemory, "xglAllocMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglFreeMemory, "xglFreeMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglSetMemoryPriority, "xglSetMemoryPriority");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglMapMemory, "xglMapMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglUnmapMemory, "xglUnmapMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglPinSystemMemory, "xglPinSystemMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetMultiGpuCompatibility, "xglGetMultiGpuCompatibility");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglOpenSharedMemory, "xglOpenSharedMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglOpenSharedQueueSemaphore, "xglOpenSharedQueueSemaphore");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglOpenPeerMemory, "xglOpenPeerMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglOpenPeerImage, "xglOpenPeerImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglDestroyObject, "xglDestroyObject");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglGetObjectInfo, "xglGetObjectInfo");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglBindObjectMemory, "xglBindObjectMemory");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglBindObjectMemoryRange, "xglBindObjectMemoryRange");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglBindImageMemoryRange, "xglBindImageMemoryRange");
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
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateBuffer, "xglCreateBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateBufferView, "xglCreateBufferView");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateImage, "xglCreateImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglSetFastClearColor, "xglSetFastClearColor");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglSetFastClearDepth, "xglSetFastClearDepth");
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
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDescriptorSetLayout, "xglCreateDescriptorSetLayout");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglBeginDescriptorRegionUpdate, "xglBeginDescriptorRegionUpdate");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglEndDescriptorRegionUpdate, "xglEndDescriptorRegionUpdate");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDescriptorRegion, "xglCreateDescriptorRegion");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglClearDescriptorRegion, "xglClearDescriptorRegion");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglAllocDescriptorSets, "xglAllocDescriptorSets");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglClearDescriptorSets, "xglClearDescriptorSets");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglUpdateDescriptors, "xglUpdateDescriptors");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDynamicViewportState, "xglCreateDynamicViewportState");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDynamicRasterState, "xglCreateDynamicRasterState");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDynamicColorBlendState, "xglCreateDynamicColorBlendState");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateDynamicDepthStencilState, "xglCreateDynamicDepthStencilState");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateCommandBuffer, "xglCreateCommandBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglBeginCommandBuffer, "xglBeginCommandBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglEndCommandBuffer, "xglEndCommandBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglResetCommandBuffer, "xglResetCommandBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindPipeline, "xglCmdBindPipeline");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindPipelineDelta, "xglCmdBindPipelineDelta");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindDynamicStateObject, "xglCmdBindDynamicStateObject");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindDescriptorSet, "xglCmdBindDescriptorSet");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindVertexBuffer, "xglCmdBindVertexBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBindIndexBuffer, "xglCmdBindIndexBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDraw, "xglCmdDraw");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDrawIndexed, "xglCmdDrawIndexed");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDrawIndirect, "xglCmdDrawIndirect");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDrawIndexedIndirect, "xglCmdDrawIndexedIndirect");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDispatch, "xglCmdDispatch");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdDispatchIndirect, "xglCmdDispatchIndirect");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCopyBuffer, "xglCmdCopyBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCopyImage, "xglCmdCopyImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCopyBufferToImage, "xglCmdCopyBufferToImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCopyImageToBuffer, "xglCmdCopyImageToBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdCloneImageData, "xglCmdCloneImageData");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdUpdateBuffer, "xglCmdUpdateBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdFillBuffer, "xglCmdFillBuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdClearColorImage, "xglCmdClearColorImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdClearColorImageRaw, "xglCmdClearColorImageRaw");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdClearDepthStencil, "xglCmdClearDepthStencil");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdResolveImage, "xglCmdResolveImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdSetEvent, "xglCmdSetEvent");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdResetEvent, "xglCmdResetEvent");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdWaitEvents, "xglCmdWaitEvents");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdPipelineBarrier, "xglCmdPipelineBarrier");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBeginQuery, "xglCmdBeginQuery");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdEndQuery, "xglCmdEndQuery");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdResetQueryPool, "xglCmdResetQueryPool");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdWriteTimestamp, "xglCmdWriteTimestamp");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdInitAtomicCounters, "xglCmdInitAtomicCounters");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdLoadAtomicCounters, "xglCmdLoadAtomicCounters");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdSaveAtomicCounters, "xglCmdSaveAtomicCounters");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateFramebuffer, "xglCreateFramebuffer");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCreateRenderPass, "xglCreateRenderPass");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdBeginRenderPass, "xglCmdBeginRenderPass");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglCmdEndRenderPass, "xglCmdEndRenderPass");
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
        unhookSuccess = Mhook_Unhook((PVOID*)&real_xglCreateInstance);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglDestroyInstance);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglEnumerateGpus);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetGpuInfo);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetProcAddr);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDevice);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglDestroyDevice);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetExtensionSupport);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglEnumerateLayers);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetDeviceQueue);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglQueueSubmit);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglQueueSetGlobalMemReferences);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglQueueWaitIdle);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglDeviceWaitIdle);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglAllocMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglFreeMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglSetMemoryPriority);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglMapMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglUnmapMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglPinSystemMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetMultiGpuCompatibility);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglOpenSharedMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglOpenSharedQueueSemaphore);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglOpenPeerMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglOpenPeerImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglDestroyObject);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglGetObjectInfo);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglBindObjectMemory);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglBindObjectMemoryRange);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglBindImageMemoryRange);
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
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateBufferView);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglSetFastClearColor);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglSetFastClearDepth);
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
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDescriptorSetLayout);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglBeginDescriptorRegionUpdate);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglEndDescriptorRegionUpdate);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDescriptorRegion);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglClearDescriptorRegion);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglAllocDescriptorSets);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglClearDescriptorSets);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglUpdateDescriptors);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDynamicViewportState);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDynamicRasterState);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDynamicColorBlendState);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateDynamicDepthStencilState);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateCommandBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglBeginCommandBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglEndCommandBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglResetCommandBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindPipeline);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindPipelineDelta);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindDynamicStateObject);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindDescriptorSet);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindVertexBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBindIndexBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDraw);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDrawIndexed);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDrawIndirect);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDrawIndexedIndirect);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDispatch);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdDispatchIndirect);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCopyBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCopyImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCopyBufferToImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCopyImageToBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdCloneImageData);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdUpdateBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdFillBuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdClearColorImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdClearColorImageRaw);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdClearDepthStencil);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdResolveImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdSetEvent);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdResetEvent);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdWaitEvents);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdPipelineBarrier);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBeginQuery);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdEndQuery);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdResetQueryPool);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdWriteTimestamp);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdInitAtomicCounters);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdLoadAtomicCounters);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdSaveAtomicCounters);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateFramebuffer);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCreateRenderPass);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdBeginRenderPass);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglCmdEndRenderPass);
    }
    isHooked = FALSE;
    if (!unhookSuccess)
    {
        glv_LogError("Failed to unhook XGL.");
    }
#endif
}
#ifdef WIN32
INIT_ONCE gInitOnce = INIT_ONCE_STATIC_INIT;
#elif defined(PLATFORM_LINUX)
pthread_once_t gInitOnce = PTHREAD_ONCE_INIT;
#endif

void send_xgl_api_version_packet()
{
    struct_xglApiVersion* pPacket;
    glv_trace_packet_header* pHeader;
    pHeader = glv_create_trace_packet(GLV_TID_XGL, GLV_TPI_XGL_xglApiVersion, sizeof(struct_xglApiVersion), 0);
    pPacket = interpret_body_as_xglApiVersion(pHeader, FALSE);
    pPacket->version = XGL_API_VERSION;
    FINISH_TRACE_PACKET();
}

extern GLV_CRITICAL_SECTION g_memInfoLock;
void InitTracer(void)
{
    char *ipAddr = glv_get_global_var("GLVLIB_TRACE_IPADDR");
    if (ipAddr == NULL)
        ipAddr = "127.0.0.1";
    gMessageStream = glv_MessageStream_create(FALSE, ipAddr, GLV_BASE_PORT + GLV_TID_XGL);
    glv_trace_set_trace_file(glv_FileLike_create_msg(gMessageStream));
//    glv_tracelog_set_log_file(glv_FileLike_create_file(fopen("glv_log_traceside.txt","w")));
    glv_tracelog_set_tracer_id(GLV_TID_XGL);
    glv_create_critical_section(&g_memInfoLock);
    send_xgl_api_version_packet();
}

// __HOOKED_xglCreateInstance is manually written. Look in glvtrace_xgl_trace.c

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglDestroyInstance(
    XGL_INSTANCE instance)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglDestroyInstance* pPacket = NULL;
    CREATE_TRACE_PACKET(xglDestroyInstance, 0);
    result = real_xglDestroyInstance(instance);
    pPacket = interpret_body_as_xglDestroyInstance(pHeader);
    pPacket->instance = instance;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_xglEnumerateGpus is manually written. Look in glvtrace_xgl_trace.c

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetGpuInfo(
    XGL_PHYSICAL_GPU gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE infoType,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    size_t _dataSize;
    struct_xglGetGpuInfo* pPacket = NULL;
    CREATE_TRACE_PACKET(xglGetGpuInfo, ((pDataSize != NULL) ? sizeof(size_t) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglGetGpuInfo(gpu, infoType, pDataSize, pData);
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_xglGetGpuInfo(pHeader);
    pPacket->gpu = gpu;
    pPacket->infoType = infoType;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT void* XGLAPI __HOOKED_xglGetProcAddr(
    XGL_PHYSICAL_GPU gpu,
    const char* pName)
{
    glv_trace_packet_header* pHeader;
    void* result;
    struct_xglGetProcAddr* pPacket = NULL;
    CREATE_TRACE_PACKET(xglGetProcAddr, ((pName != NULL) ? strlen(pName) + 1 : 0));
    result = real_xglGetProcAddr(gpu, pName);
    pPacket = interpret_body_as_xglGetProcAddr(pHeader);
    pPacket->gpu = gpu;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pName), ((pName != NULL) ? strlen(pName) + 1 : 0), pName);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pName));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDevice(
    XGL_PHYSICAL_GPU gpu,
    const XGL_DEVICE_CREATE_INFO* pCreateInfo,
    XGL_DEVICE* pDevice)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDevice* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateDevice, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_DEVICE));
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
    struct_xglDestroyDevice* pPacket = NULL;
    CREATE_TRACE_PACKET(xglDestroyDevice, 0);
    result = real_xglDestroyDevice(device);
    pPacket = interpret_body_as_xglDestroyDevice(pHeader);
    pPacket->device = device;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetExtensionSupport(
    XGL_PHYSICAL_GPU gpu,
    const char* pExtName)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetExtensionSupport* pPacket = NULL;
    CREATE_TRACE_PACKET(xglGetExtensionSupport, ((pExtName != NULL) ? strlen(pExtName) + 1 : 0));
    result = real_xglGetExtensionSupport(gpu, pExtName);
    pPacket = interpret_body_as_xglGetExtensionSupport(pHeader);
    pPacket->gpu = gpu;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pExtName), ((pExtName != NULL) ? strlen(pExtName) + 1 : 0), pExtName);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pExtName));
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_xglEnumerateLayers is manually written. Look in glvtrace_xgl_trace.c

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetDeviceQueue(
    XGL_DEVICE device,
    uint32_t queueNodeIndex,
    uint32_t queueIndex,
    XGL_QUEUE* pQueue)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetDeviceQueue* pPacket = NULL;
    CREATE_TRACE_PACKET(xglGetDeviceQueue, sizeof(XGL_QUEUE));
    result = real_xglGetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    pPacket = interpret_body_as_xglGetDeviceQueue(pHeader);
    pPacket->device = device;
    pPacket->queueNodeIndex = queueNodeIndex;
    pPacket->queueIndex = queueIndex;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pQueue), sizeof(XGL_QUEUE), pQueue);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pQueue));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglQueueSubmit(
    XGL_QUEUE queue,
    uint32_t cmdBufferCount,
    const XGL_CMD_BUFFER* pCmdBuffers,
    uint32_t memRefCount,
    const XGL_MEMORY_REF* pMemRefs,
    XGL_FENCE fence)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglQueueSubmit* pPacket = NULL;
    CREATE_TRACE_PACKET(xglQueueSubmit, cmdBufferCount*sizeof(XGL_CMD_BUFFER) + memRefCount*sizeof(XGL_MEMORY_REF));
    result = real_xglQueueSubmit(queue, cmdBufferCount, pCmdBuffers, memRefCount, pMemRefs, fence);
    pPacket = interpret_body_as_xglQueueSubmit(pHeader);
    pPacket->queue = queue;
    pPacket->cmdBufferCount = cmdBufferCount;
    pPacket->memRefCount = memRefCount;
    pPacket->fence = fence;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCmdBuffers), cmdBufferCount*sizeof(XGL_CMD_BUFFER), pCmdBuffers);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMemRefs), memRefCount*sizeof(XGL_MEMORY_REF), pMemRefs);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCmdBuffers));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMemRefs));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglQueueSetGlobalMemReferences(
    XGL_QUEUE queue,
    uint32_t memRefCount,
    const XGL_MEMORY_REF* pMemRefs)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglQueueSetGlobalMemReferences* pPacket = NULL;
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
    struct_xglQueueWaitIdle* pPacket = NULL;
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
    struct_xglDeviceWaitIdle* pPacket = NULL;
    CREATE_TRACE_PACKET(xglDeviceWaitIdle, 0);
    result = real_xglDeviceWaitIdle(device);
    pPacket = interpret_body_as_xglDeviceWaitIdle(pHeader);
    pPacket->device = device;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglAllocMemory(
    XGL_DEVICE device,
    const XGL_MEMORY_ALLOC_INFO* pAllocInfo,
    XGL_GPU_MEMORY* pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglAllocMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(xglAllocMemory, get_struct_chain_size((void*)pAllocInfo) + sizeof(XGL_GPU_MEMORY));
    result = real_xglAllocMemory(device, pAllocInfo, pMem);
    pPacket = interpret_body_as_xglAllocMemory(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo), sizeof(XGL_MEMORY_ALLOC_INFO), pAllocInfo);
    add_alloc_memory_to_trace_packet(pHeader, (void**)&(pPacket->pAllocInfo->pNext), pAllocInfo->pNext);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(XGL_GPU_MEMORY), pMem);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pAllocInfo));
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
    struct_xglFreeMemory* pPacket = NULL;
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
    XGL_MEMORY_PRIORITY priority)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglSetMemoryPriority* pPacket = NULL;
    CREATE_TRACE_PACKET(xglSetMemoryPriority, 0);
    result = real_xglSetMemoryPriority(mem, priority);
    pPacket = interpret_body_as_xglSetMemoryPriority(pHeader);
    pPacket->mem = mem;
    pPacket->priority = priority;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_xglMapMemory is manually written. Look in glvtrace_xgl_trace.c

// __HOOKED_xglUnmapMemory is manually written. Look in glvtrace_xgl_trace.c

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglPinSystemMemory(
    XGL_DEVICE device,
    const void* pSysMem,
    size_t memSize,
    XGL_GPU_MEMORY* pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglPinSystemMemory* pPacket = NULL;
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

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetMultiGpuCompatibility(
    XGL_PHYSICAL_GPU gpu0,
    XGL_PHYSICAL_GPU gpu1,
    XGL_GPU_COMPATIBILITY_INFO* pInfo)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglGetMultiGpuCompatibility* pPacket = NULL;
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
    XGL_DEVICE device,
    const XGL_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY* pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglOpenSharedMemory* pPacket = NULL;
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
    XGL_DEVICE device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo,
    XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglOpenSharedQueueSemaphore* pPacket = NULL;
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
    XGL_DEVICE device,
    const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo,
    XGL_GPU_MEMORY* pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglOpenPeerMemory* pPacket = NULL;
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
    XGL_DEVICE device,
    const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo,
    XGL_IMAGE* pImage,
    XGL_GPU_MEMORY* pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglOpenPeerImage* pPacket = NULL;
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

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglDestroyObject(
    XGL_OBJECT object)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglDestroyObject* pPacket = NULL;
    CREATE_TRACE_PACKET(xglDestroyObject, 0);
    result = real_xglDestroyObject(object);
    pPacket = interpret_body_as_xglDestroyObject(pHeader);
    pPacket->object = object;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetObjectInfo(
    XGL_BASE_OBJECT object,
    XGL_OBJECT_INFO_TYPE infoType,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    size_t _dataSize;
    struct_xglGetObjectInfo* pPacket = NULL;
    CREATE_TRACE_PACKET(xglGetObjectInfo, ((pDataSize != NULL) ? sizeof(size_t) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglGetObjectInfo(object, infoType, pDataSize, pData);
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_xglGetObjectInfo(pHeader);
    pPacket->object = object;
    pPacket->infoType = infoType;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglBindObjectMemory(
    XGL_OBJECT object,
    uint32_t allocationIdx,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE offset)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglBindObjectMemory* pPacket = NULL;
    CREATE_TRACE_PACKET(xglBindObjectMemory, 0);
    result = real_xglBindObjectMemory(object, allocationIdx, mem, offset);
    pPacket = interpret_body_as_xglBindObjectMemory(pHeader);
    pPacket->object = object;
    pPacket->allocationIdx = allocationIdx;
    pPacket->mem = mem;
    pPacket->offset = offset;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglBindObjectMemoryRange(
    XGL_OBJECT object,
    uint32_t allocationIdx,
    XGL_GPU_SIZE rangeOffset,
    XGL_GPU_SIZE rangeSize,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE memOffset)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglBindObjectMemoryRange* pPacket = NULL;
    CREATE_TRACE_PACKET(xglBindObjectMemoryRange, 0);
    result = real_xglBindObjectMemoryRange(object, allocationIdx, rangeOffset, rangeSize, mem, memOffset);
    pPacket = interpret_body_as_xglBindObjectMemoryRange(pHeader);
    pPacket->object = object;
    pPacket->allocationIdx = allocationIdx;
    pPacket->rangeOffset = rangeOffset;
    pPacket->rangeSize = rangeSize;
    pPacket->mem = mem;
    pPacket->memOffset = memOffset;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglBindImageMemoryRange(
    XGL_IMAGE image,
    uint32_t allocationIdx,
    const XGL_IMAGE_MEMORY_BIND_INFO* bindInfo,
    XGL_GPU_MEMORY mem,
    XGL_GPU_SIZE memOffset)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglBindImageMemoryRange* pPacket = NULL;
    CREATE_TRACE_PACKET(xglBindImageMemoryRange, sizeof(XGL_IMAGE_MEMORY_BIND_INFO));
    result = real_xglBindImageMemoryRange(image, allocationIdx, bindInfo, mem, memOffset);
    pPacket = interpret_body_as_xglBindImageMemoryRange(pHeader);
    pPacket->image = image;
    pPacket->allocationIdx = allocationIdx;
    pPacket->mem = mem;
    pPacket->memOffset = memOffset;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->bindInfo), sizeof(XGL_IMAGE_MEMORY_BIND_INFO), bindInfo);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->bindInfo));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateFence(
    XGL_DEVICE device,
    const XGL_FENCE_CREATE_INFO* pCreateInfo,
    XGL_FENCE* pFence)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateFence* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateFence, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_FENCE));
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
    struct_xglGetFenceStatus* pPacket = NULL;
    CREATE_TRACE_PACKET(xglGetFenceStatus, 0);
    result = real_xglGetFenceStatus(fence);
    pPacket = interpret_body_as_xglGetFenceStatus(pHeader);
    pPacket->fence = fence;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWaitForFences(
    XGL_DEVICE device,
    uint32_t fenceCount,
    const XGL_FENCE* pFences,
    bool32_t waitAll,
    uint64_t timeout)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWaitForFences* pPacket = NULL;
    CREATE_TRACE_PACKET(xglWaitForFences, fenceCount*sizeof(XGL_FENCE));
    result = real_xglWaitForFences(device, fenceCount, pFences, waitAll, timeout);
    pPacket = interpret_body_as_xglWaitForFences(pHeader);
    pPacket->device = device;
    pPacket->fenceCount = fenceCount;
    pPacket->waitAll = waitAll;
    pPacket->timeout = timeout;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFences), fenceCount*sizeof(XGL_FENCE), pFences);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pFences));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateQueueSemaphore(
    XGL_DEVICE device,
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo,
    XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateQueueSemaphore* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateQueueSemaphore, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_QUEUE_SEMAPHORE));
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
    XGL_QUEUE queue,
    XGL_QUEUE_SEMAPHORE semaphore)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglSignalQueueSemaphore* pPacket = NULL;
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
    XGL_QUEUE queue,
    XGL_QUEUE_SEMAPHORE semaphore)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWaitQueueSemaphore* pPacket = NULL;
    CREATE_TRACE_PACKET(xglWaitQueueSemaphore, 0);
    result = real_xglWaitQueueSemaphore(queue, semaphore);
    pPacket = interpret_body_as_xglWaitQueueSemaphore(pHeader);
    pPacket->queue = queue;
    pPacket->semaphore = semaphore;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateEvent(
    XGL_DEVICE device,
    const XGL_EVENT_CREATE_INFO* pCreateInfo,
    XGL_EVENT* pEvent)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateEvent, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_EVENT));
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
    struct_xglGetEventStatus* pPacket = NULL;
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
    struct_xglSetEvent* pPacket = NULL;
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
    struct_xglResetEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(xglResetEvent, 0);
    result = real_xglResetEvent(event);
    pPacket = interpret_body_as_xglResetEvent(pHeader);
    pPacket->event = event;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateQueryPool(
    XGL_DEVICE device,
    const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo,
    XGL_QUERY_POOL* pQueryPool)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateQueryPool* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateQueryPool, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_QUERY_POOL));
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
    uint32_t startQuery,
    uint32_t queryCount,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    size_t _dataSize;
    struct_xglGetQueryPoolResults* pPacket = NULL;
    CREATE_TRACE_PACKET(xglGetQueryPoolResults, ((pDataSize != NULL) ? sizeof(size_t) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglGetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_xglGetQueryPoolResults(pHeader);
    pPacket->queryPool = queryPool;
    pPacket->startQuery = startQuery;
    pPacket->queryCount = queryCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetFormatInfo(
    XGL_DEVICE device,
    XGL_FORMAT format,
    XGL_FORMAT_INFO_TYPE infoType,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    size_t _dataSize;
    struct_xglGetFormatInfo* pPacket = NULL;
    CREATE_TRACE_PACKET(xglGetFormatInfo, ((pDataSize != NULL) ? sizeof(size_t) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglGetFormatInfo(device, format, infoType, pDataSize, pData);
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_xglGetFormatInfo(pHeader);
    pPacket->device = device;
    pPacket->format = format;
    pPacket->infoType = infoType;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateBuffer(
    XGL_DEVICE device,
    const XGL_BUFFER_CREATE_INFO* pCreateInfo,
    XGL_BUFFER* pBuffer)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateBuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_BUFFER));
    result = real_xglCreateBuffer(device, pCreateInfo, pBuffer);
    pPacket = interpret_body_as_xglCreateBuffer(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_BUFFER_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBuffer), sizeof(XGL_BUFFER), pBuffer);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBuffer));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateBufferView(
    XGL_DEVICE device,
    const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo,
    XGL_BUFFER_VIEW* pView)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateBufferView* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateBufferView, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_BUFFER_VIEW));
    result = real_xglCreateBufferView(device, pCreateInfo, pView);
    pPacket = interpret_body_as_xglCreateBufferView(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_BUFFER_VIEW_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pView), sizeof(XGL_BUFFER_VIEW), pView);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pView));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateImage(
    XGL_DEVICE device,
    const XGL_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE* pImage)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateImage* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateImage, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_IMAGE));
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

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglSetFastClearColor(
    XGL_IMAGE image,
    const float color[4])
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglSetFastClearColor* pPacket = NULL;
    CREATE_TRACE_PACKET(xglSetFastClearColor, 0);
    result = real_xglSetFastClearColor(image, color);
    pPacket = interpret_body_as_xglSetFastClearColor(pHeader);
    pPacket->image = image;
    memcpy((void*)pPacket->color, color, 4 * sizeof(float));
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglSetFastClearDepth(
    XGL_IMAGE image,
    float depth)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglSetFastClearDepth* pPacket = NULL;
    CREATE_TRACE_PACKET(xglSetFastClearDepth, 0);
    result = real_xglSetFastClearDepth(image, depth);
    pPacket = interpret_body_as_xglSetFastClearDepth(pHeader);
    pPacket->image = image;
    pPacket->depth = depth;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglGetImageSubresourceInfo(
    XGL_IMAGE image,
    const XGL_IMAGE_SUBRESOURCE* pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE infoType,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    size_t _dataSize;
    struct_xglGetImageSubresourceInfo* pPacket = NULL;
    CREATE_TRACE_PACKET(xglGetImageSubresourceInfo, ((pSubresource != NULL) ? sizeof(XGL_IMAGE_SUBRESOURCE) : 0) + ((pDataSize != NULL) ? sizeof(size_t) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglGetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_xglGetImageSubresourceInfo(pHeader);
    pPacket->image = image;
    pPacket->infoType = infoType;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSubresource), sizeof(XGL_IMAGE_SUBRESOURCE), pSubresource);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSubresource));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateImageView(
    XGL_DEVICE device,
    const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
    XGL_IMAGE_VIEW* pView)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateImageView* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateImageView, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_IMAGE_VIEW));
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
    XGL_DEVICE device,
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW* pView)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateColorAttachmentView* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateColorAttachmentView, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_COLOR_ATTACHMENT_VIEW));
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
    XGL_DEVICE device,
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo,
    XGL_DEPTH_STENCIL_VIEW* pView)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDepthStencilView* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateDepthStencilView, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_DEPTH_STENCIL_VIEW));
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

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateShader(
    XGL_DEVICE device,
    const XGL_SHADER_CREATE_INFO* pCreateInfo,
    XGL_SHADER* pShader)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateShader* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateShader, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_SHADER));
    result = real_xglCreateShader(device, pCreateInfo, pShader);
    pPacket = interpret_body_as_xglCreateShader(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_SHADER_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pCode), ((pCreateInfo != NULL) ? pCreateInfo->codeSize : 0), pCreateInfo->pCode);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pShader), sizeof(XGL_SHADER), pShader);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pCode));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pShader));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateGraphicsPipeline(
    XGL_DEVICE device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE* pPipeline)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateGraphicsPipeline* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateGraphicsPipeline, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_PIPELINE));
    result = real_xglCreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    pPacket = interpret_body_as_xglCreateGraphicsPipeline(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO), pCreateInfo);
    add_pipeline_state_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipeline), sizeof(XGL_PIPELINE), pPipeline);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipeline));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateComputePipeline(
    XGL_DEVICE device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo,
    XGL_PIPELINE* pPipeline)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateComputePipeline* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateComputePipeline, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_PIPELINE));
    result = real_xglCreateComputePipeline(device, pCreateInfo, pPipeline);
    pPacket = interpret_body_as_xglCreateComputePipeline(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO), pCreateInfo);
    add_pipeline_state_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pNext), pCreateInfo->pNext);
    add_pipeline_shader_to_trace_packet(pHeader, (XGL_PIPELINE_SHADER*)&pPacket->pCreateInfo->cs, &pCreateInfo->cs);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipeline), sizeof(XGL_PIPELINE), pPipeline);
    pPacket->result = result;
    finalize_pipeline_shader_address(pHeader, &pPacket->pCreateInfo->cs);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipeline));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglStorePipeline(
    XGL_PIPELINE pipeline,
    size_t* pDataSize,
    void* pData)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    size_t _dataSize;
    struct_xglStorePipeline* pPacket = NULL;
    CREATE_TRACE_PACKET(xglStorePipeline, ((pDataSize != NULL) ? sizeof(size_t) : 0) + ((pDataSize != NULL && pData != NULL) ? *pDataSize : 0));
    result = real_xglStorePipeline(pipeline, pDataSize, pData);
    _dataSize = (pDataSize == NULL || pData == NULL) ? 0 : *pDataSize;
    pPacket = interpret_body_as_xglStorePipeline(pHeader);
    pPacket->pipeline = pipeline;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDataSize), sizeof(size_t), &_dataSize);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), _dataSize, pData);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDataSize));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglLoadPipeline(
    XGL_DEVICE device,
    size_t dataSize,
    const void* pData,
    XGL_PIPELINE* pPipeline)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglLoadPipeline* pPacket = NULL;
    CREATE_TRACE_PACKET(xglLoadPipeline, dataSize + sizeof(XGL_PIPELINE));
    result = real_xglLoadPipeline(device, dataSize, pData, pPipeline);
    pPacket = interpret_body_as_xglLoadPipeline(pHeader);
    pPacket->device = device;
    pPacket->dataSize = dataSize;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), dataSize, pData);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPipeline), sizeof(XGL_PIPELINE), pPipeline);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPipeline));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreatePipelineDelta(
    XGL_DEVICE device,
    XGL_PIPELINE p1,
    XGL_PIPELINE p2,
    XGL_PIPELINE_DELTA* delta)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreatePipelineDelta* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreatePipelineDelta, sizeof(XGL_PIPELINE_DELTA));
    result = real_xglCreatePipelineDelta(device, p1, p2, delta);
    pPacket = interpret_body_as_xglCreatePipelineDelta(pHeader);
    pPacket->device = device;
    pPacket->p1 = p1;
    pPacket->p2 = p2;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->delta), sizeof(XGL_PIPELINE_DELTA), delta);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->delta));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateSampler(
    XGL_DEVICE device,
    const XGL_SAMPLER_CREATE_INFO* pCreateInfo,
    XGL_SAMPLER* pSampler)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateSampler* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateSampler, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_SAMPLER));
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

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDescriptorSetLayout(
    XGL_DEVICE device,
    XGL_FLAGS stageFlags,
    const uint32_t* pSetBindPoints,
    XGL_DESCRIPTOR_SET_LAYOUT priorSetLayout,
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList,
    XGL_DESCRIPTOR_SET_LAYOUT* pSetLayout)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDescriptorSetLayout* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateDescriptorSetLayout, (XGL_SHADER_STAGE_COMPUTE * sizeof(uint32_t)) + get_struct_chain_size((void*)pSetLayoutInfoList) + sizeof(XGL_DESCRIPTOR_SET_LAYOUT));
    result = real_xglCreateDescriptorSetLayout(device, stageFlags, pSetBindPoints, priorSetLayout, pSetLayoutInfoList, pSetLayout);
    pPacket = interpret_body_as_xglCreateDescriptorSetLayout(pHeader);
    pPacket->device = device;
    pPacket->stageFlags = stageFlags;
    pPacket->priorSetLayout = priorSetLayout;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetBindPoints), sizeof(uint32_t), pSetBindPoints);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayoutInfoList), sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO), pSetLayoutInfoList);
    if (pSetLayoutInfoList)
        add_create_ds_layout_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayoutInfoList->pNext), pSetLayoutInfoList->pNext);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pSetLayout), sizeof(XGL_DESCRIPTOR_SET_LAYOUT), pSetLayout);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetBindPoints));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetLayoutInfoList));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pSetLayout));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglBeginDescriptorRegionUpdate(
    XGL_DEVICE device,
    XGL_DESCRIPTOR_UPDATE_MODE updateMode)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglBeginDescriptorRegionUpdate* pPacket = NULL;
    CREATE_TRACE_PACKET(xglBeginDescriptorRegionUpdate, 0);
    result = real_xglBeginDescriptorRegionUpdate(device, updateMode);
    pPacket = interpret_body_as_xglBeginDescriptorRegionUpdate(pHeader);
    pPacket->device = device;
    pPacket->updateMode = updateMode;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglEndDescriptorRegionUpdate(
    XGL_DEVICE device,
    XGL_CMD_BUFFER cmd)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglEndDescriptorRegionUpdate* pPacket = NULL;
    CREATE_TRACE_PACKET(xglEndDescriptorRegionUpdate, 0);
    result = real_xglEndDescriptorRegionUpdate(device, cmd);
    pPacket = interpret_body_as_xglEndDescriptorRegionUpdate(pHeader);
    pPacket->device = device;
    pPacket->cmd = cmd;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDescriptorRegion(
    XGL_DEVICE device,
    XGL_DESCRIPTOR_REGION_USAGE regionUsage,
    uint32_t maxSets,
    const XGL_DESCRIPTOR_REGION_CREATE_INFO* pCreateInfo,
    XGL_DESCRIPTOR_REGION* pDescriptorRegion)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDescriptorRegion* pPacket = NULL;
    uint32_t rgCount = (pCreateInfo != NULL && pCreateInfo->pTypeCount != NULL) ? pCreateInfo->count : 0;
    CREATE_TRACE_PACKET(xglCreateDescriptorRegion,  get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_DESCRIPTOR_REGION));
    result = real_xglCreateDescriptorRegion(device, regionUsage, maxSets, pCreateInfo, pDescriptorRegion);
    pPacket = interpret_body_as_xglCreateDescriptorRegion(pHeader);
    pPacket->device = device;
    pPacket->regionUsage = regionUsage;
    pPacket->maxSets = maxSets;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DESCRIPTOR_REGION_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount), rgCount * sizeof(XGL_DESCRIPTOR_TYPE_COUNT), pCreateInfo->pTypeCount);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorRegion), sizeof(XGL_DESCRIPTOR_REGION), pDescriptorRegion);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pTypeCount));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorRegion));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglClearDescriptorRegion(
    XGL_DESCRIPTOR_REGION descriptorRegion)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglClearDescriptorRegion* pPacket = NULL;
    CREATE_TRACE_PACKET(xglClearDescriptorRegion, 0);
    result = real_xglClearDescriptorRegion(descriptorRegion);
    pPacket = interpret_body_as_xglClearDescriptorRegion(pHeader);
    pPacket->descriptorRegion = descriptorRegion;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

// __HOOKED_xglAllocDescriptorSets is manually written. Look in glvtrace_xgl_trace.c

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglClearDescriptorSets(
    XGL_DESCRIPTOR_REGION descriptorRegion,
    uint32_t count,
    const XGL_DESCRIPTOR_SET* pDescriptorSets)
{
    glv_trace_packet_header* pHeader;
    struct_xglClearDescriptorSets* pPacket = NULL;
    CREATE_TRACE_PACKET(xglClearDescriptorSets, count*sizeof(XGL_DESCRIPTOR_SET));
    real_xglClearDescriptorSets(descriptorRegion, count, pDescriptorSets);
    pPacket = interpret_body_as_xglClearDescriptorSets(pHeader);
    pPacket->descriptorRegion = descriptorRegion;
    pPacket->count = count;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pDescriptorSets), count*sizeof(XGL_DESCRIPTOR_SET), pDescriptorSets);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pDescriptorSets));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglUpdateDescriptors(
    XGL_DESCRIPTOR_SET descriptorSet,
    const void* pUpdateChain)
{
    glv_trace_packet_header* pHeader;
    struct_xglUpdateDescriptors* pPacket = NULL;
    CREATE_TRACE_PACKET(xglUpdateDescriptors, get_struct_chain_size((void*)pUpdateChain));
    real_xglUpdateDescriptors(descriptorSet, pUpdateChain);
    pPacket = interpret_body_as_xglUpdateDescriptors(pHeader);
    pPacket->descriptorSet = descriptorSet;
    add_update_descriptors_to_trace_packet(pHeader, (void**)&(pPacket->pUpdateChain), pUpdateChain);
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDynamicViewportState(
    XGL_DEVICE device,
    const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_VP_STATE_OBJECT* pState)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDynamicViewportState* pPacket = NULL;
    uint32_t vpsCount = (pCreateInfo != NULL && pCreateInfo->pViewports != NULL) ? pCreateInfo->viewportAndScissorCount : 0;
    CREATE_TRACE_PACKET(xglCreateDynamicViewportState,  get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_DYNAMIC_VP_STATE_OBJECT));
    result = real_xglCreateDynamicViewportState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_xglCreateDynamicViewportState(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DYNAMIC_VP_STATE_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pViewports), vpsCount * sizeof(XGL_VIEWPORT), pCreateInfo->pViewports);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pScissors), vpsCount * sizeof(XGL_RECT), pCreateInfo->pScissors);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(XGL_DYNAMIC_VP_STATE_OBJECT), pState);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pViewports));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pScissors));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDynamicRasterState(
    XGL_DEVICE device,
    const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_RS_STATE_OBJECT* pState)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDynamicRasterState* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateDynamicRasterState, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_DYNAMIC_RS_STATE_OBJECT));
    result = real_xglCreateDynamicRasterState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_xglCreateDynamicRasterState(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DYNAMIC_RS_STATE_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(XGL_DYNAMIC_RS_STATE_OBJECT), pState);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDynamicColorBlendState(
    XGL_DEVICE device,
    const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_CB_STATE_OBJECT* pState)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDynamicColorBlendState* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateDynamicColorBlendState, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_DYNAMIC_CB_STATE_OBJECT));
    result = real_xglCreateDynamicColorBlendState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_xglCreateDynamicColorBlendState(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DYNAMIC_CB_STATE_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(XGL_DYNAMIC_CB_STATE_OBJECT), pState);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateDynamicDepthStencilState(
    XGL_DEVICE device,
    const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo,
    XGL_DYNAMIC_DS_STATE_OBJECT* pState)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateDynamicDepthStencilState* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateDynamicDepthStencilState, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_DYNAMIC_DS_STATE_OBJECT));
    result = real_xglCreateDynamicDepthStencilState(device, pCreateInfo, pState);
    pPacket = interpret_body_as_xglCreateDynamicDepthStencilState(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_DYNAMIC_DS_STATE_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pState), sizeof(XGL_DYNAMIC_DS_STATE_OBJECT), pState);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pState));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateCommandBuffer(
    XGL_DEVICE device,
    const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo,
    XGL_CMD_BUFFER* pCmdBuffer)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateCommandBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCreateCommandBuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_CMD_BUFFER));
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
    const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglBeginCommandBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglBeginCommandBuffer, get_struct_chain_size((void*)pBeginInfo));
    result = real_xglBeginCommandBuffer(cmdBuffer, pBeginInfo);
    pPacket = interpret_body_as_xglBeginCommandBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pBeginInfo), sizeof(XGL_CMD_BUFFER_BEGIN_INFO), pBeginInfo);
    add_begin_cmdbuf_to_trace_packet(pHeader, (void**)&(pPacket->pBeginInfo->pNext), pBeginInfo->pNext);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pBeginInfo));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglEndCommandBuffer(
    XGL_CMD_BUFFER cmdBuffer)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglEndCommandBuffer* pPacket = NULL;
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
    struct_xglResetCommandBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglResetCommandBuffer, 0);
    result = real_xglResetCommandBuffer(cmdBuffer);
    pPacket = interpret_body_as_xglResetCommandBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->result = result;
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdBindPipeline(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    XGL_PIPELINE pipeline)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindPipeline* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdBindPipeline, 0);
    real_xglCmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
    pPacket = interpret_body_as_xglCmdBindPipeline(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->pipeline = pipeline;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdBindPipelineDelta(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    XGL_PIPELINE_DELTA delta)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindPipelineDelta* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdBindPipelineDelta, 0);
    real_xglCmdBindPipelineDelta(cmdBuffer, pipelineBindPoint, delta);
    pPacket = interpret_body_as_xglCmdBindPipelineDelta(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->delta = delta;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdBindDynamicStateObject(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_STATE_BIND_POINT stateBindPoint,
    XGL_DYNAMIC_STATE_OBJECT state)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindDynamicStateObject* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdBindDynamicStateObject, 0);
    real_xglCmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
    pPacket = interpret_body_as_xglCmdBindDynamicStateObject(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->stateBindPoint = stateBindPoint;
    pPacket->state = state;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdBindDescriptorSet(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    XGL_DESCRIPTOR_SET descriptorSet,
    const uint32_t* pUserData)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindDescriptorSet* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdBindDescriptorSet, sizeof(uint32_t));
    real_xglCmdBindDescriptorSet(cmdBuffer, pipelineBindPoint, descriptorSet, pUserData);
    pPacket = interpret_body_as_xglCmdBindDescriptorSet(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->descriptorSet = descriptorSet;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pUserData), sizeof(uint32_t), pUserData);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pUserData));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdBindVertexBuffer(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset,
    uint32_t binding)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindVertexBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdBindVertexBuffer, 0);
    real_xglCmdBindVertexBuffer(cmdBuffer, buffer, offset, binding);
    pPacket = interpret_body_as_xglCmdBindVertexBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->buffer = buffer;
    pPacket->offset = offset;
    pPacket->binding = binding;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdBindIndexBuffer(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset,
    XGL_INDEX_TYPE indexType)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBindIndexBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdBindIndexBuffer, 0);
    real_xglCmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
    pPacket = interpret_body_as_xglCmdBindIndexBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->buffer = buffer;
    pPacket->offset = offset;
    pPacket->indexType = indexType;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdDraw(
    XGL_CMD_BUFFER cmdBuffer,
    uint32_t firstVertex,
    uint32_t vertexCount,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDraw* pPacket = NULL;
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

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdDrawIndexed(
    XGL_CMD_BUFFER cmdBuffer,
    uint32_t firstIndex,
    uint32_t indexCount,
    int32_t vertexOffset,
    uint32_t firstInstance,
    uint32_t instanceCount)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDrawIndexed* pPacket = NULL;
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

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdDrawIndirect(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset,
    uint32_t count,
    uint32_t stride)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDrawIndirect* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdDrawIndirect, 0);
    real_xglCmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
    pPacket = interpret_body_as_xglCmdDrawIndirect(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->buffer = buffer;
    pPacket->offset = offset;
    pPacket->count = count;
    pPacket->stride = stride;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset,
    uint32_t count,
    uint32_t stride)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDrawIndexedIndirect* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdDrawIndexedIndirect, 0);
    real_xglCmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
    pPacket = interpret_body_as_xglCmdDrawIndexedIndirect(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->buffer = buffer;
    pPacket->offset = offset;
    pPacket->count = count;
    pPacket->stride = stride;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdDispatch(
    XGL_CMD_BUFFER cmdBuffer,
    uint32_t x,
    uint32_t y,
    uint32_t z)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDispatch* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdDispatch, 0);
    real_xglCmdDispatch(cmdBuffer, x, y, z);
    pPacket = interpret_body_as_xglCmdDispatch(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->x = x;
    pPacket->y = y;
    pPacket->z = z;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdDispatchIndirect(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER buffer,
    XGL_GPU_SIZE offset)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdDispatchIndirect* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdDispatchIndirect, 0);
    real_xglCmdDispatchIndirect(cmdBuffer, buffer, offset);
    pPacket = interpret_body_as_xglCmdDispatchIndirect(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->buffer = buffer;
    pPacket->offset = offset;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdCopyBuffer(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER srcBuffer,
    XGL_BUFFER destBuffer,
    uint32_t regionCount,
    const XGL_BUFFER_COPY* pRegions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCopyBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdCopyBuffer, regionCount*sizeof(XGL_BUFFER_COPY));
    real_xglCmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
    pPacket = interpret_body_as_xglCmdCopyBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcBuffer = srcBuffer;
    pPacket->destBuffer = destBuffer;
    pPacket->regionCount = regionCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(XGL_BUFFER_COPY), pRegions);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdCopyImage(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE srcImage,
    XGL_IMAGE destImage,
    uint32_t regionCount,
    const XGL_IMAGE_COPY* pRegions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCopyImage* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdCopyImage, regionCount*sizeof(XGL_IMAGE_COPY));
    real_xglCmdCopyImage(cmdBuffer, srcImage, destImage, regionCount, pRegions);
    pPacket = interpret_body_as_xglCmdCopyImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->destImage = destImage;
    pPacket->regionCount = regionCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(XGL_IMAGE_COPY), pRegions);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdCopyBufferToImage(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER srcBuffer,
    XGL_IMAGE destImage,
    uint32_t regionCount,
    const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCopyBufferToImage* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdCopyBufferToImage, regionCount*sizeof(XGL_BUFFER_IMAGE_COPY));
    real_xglCmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, regionCount, pRegions);
    pPacket = interpret_body_as_xglCmdCopyBufferToImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcBuffer = srcBuffer;
    pPacket->destImage = destImage;
    pPacket->regionCount = regionCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(XGL_BUFFER_IMAGE_COPY), pRegions);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdCopyImageToBuffer(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE srcImage,
    XGL_BUFFER destBuffer,
    uint32_t regionCount,
    const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCopyImageToBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdCopyImageToBuffer, regionCount*sizeof(XGL_BUFFER_IMAGE_COPY));
    real_xglCmdCopyImageToBuffer(cmdBuffer, srcImage, destBuffer, regionCount, pRegions);
    pPacket = interpret_body_as_xglCmdCopyImageToBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->destBuffer = destBuffer;
    pPacket->regionCount = regionCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRegions), regionCount*sizeof(XGL_BUFFER_IMAGE_COPY), pRegions);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRegions));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdCloneImageData(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE srcImage,
    XGL_IMAGE_LAYOUT srcImageLayout,
    XGL_IMAGE destImage,
    XGL_IMAGE_LAYOUT destImageLayout)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdCloneImageData* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdCloneImageData, 0);
    real_xglCmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
    pPacket = interpret_body_as_xglCmdCloneImageData(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->srcImage = srcImage;
    pPacket->srcImageLayout = srcImageLayout;
    pPacket->destImage = destImage;
    pPacket->destImageLayout = destImageLayout;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdUpdateBuffer(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER destBuffer,
    XGL_GPU_SIZE destOffset,
    XGL_GPU_SIZE dataSize,
    const uint32_t* pData)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdUpdateBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdUpdateBuffer, dataSize);
    real_xglCmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
    pPacket = interpret_body_as_xglCmdUpdateBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->destBuffer = destBuffer;
    pPacket->destOffset = destOffset;
    pPacket->dataSize = dataSize;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), dataSize, pData);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdFillBuffer(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_BUFFER destBuffer,
    XGL_GPU_SIZE destOffset,
    XGL_GPU_SIZE fillSize,
    uint32_t data)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdFillBuffer* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdFillBuffer, 0);
    real_xglCmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
    pPacket = interpret_body_as_xglCmdFillBuffer(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->destBuffer = destBuffer;
    pPacket->destOffset = destOffset;
    pPacket->fillSize = fillSize;
    pPacket->data = data;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdClearColorImage(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE image,
    const float color[4],
    uint32_t rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdClearColorImage* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdClearColorImage, rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE));
    real_xglCmdClearColorImage(cmdBuffer, image, color, rangeCount, pRanges);
    pPacket = interpret_body_as_xglCmdClearColorImage(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->image = image;
    memcpy((void*)pPacket->color, color, 4 * sizeof(float));
    pPacket->rangeCount = rangeCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRanges), rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE), pRanges);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRanges));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdClearColorImageRaw(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE image,
    const uint32_t color[4],
    uint32_t rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdClearColorImageRaw* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdClearColorImageRaw, rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE));
    real_xglCmdClearColorImageRaw(cmdBuffer, image, color, rangeCount, pRanges);
    pPacket = interpret_body_as_xglCmdClearColorImageRaw(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->image = image;
    memcpy((void*)pPacket->color, color, 4 * sizeof(uint32_t));
    pPacket->rangeCount = rangeCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRanges), rangeCount*sizeof(XGL_IMAGE_SUBRESOURCE_RANGE), pRanges);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRanges));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdClearDepthStencil(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE image,
    float depth,
    uint32_t stencil,
    uint32_t rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdClearDepthStencil* pPacket = NULL;
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

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdResolveImage(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_IMAGE srcImage,
    XGL_IMAGE destImage,
    uint32_t rectCount,
    const XGL_IMAGE_RESOLVE* pRects)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdResolveImage* pPacket = NULL;
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

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdSetEvent(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT event,
    XGL_SET_EVENT pipeEvent)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdSetEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdSetEvent, 0);
    real_xglCmdSetEvent(cmdBuffer, event, pipeEvent);
    pPacket = interpret_body_as_xglCmdSetEvent(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->event = event;
    pPacket->pipeEvent = pipeEvent;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdResetEvent(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_EVENT event)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdResetEvent* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdResetEvent, 0);
    real_xglCmdResetEvent(cmdBuffer, event);
    pPacket = interpret_body_as_xglCmdResetEvent(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->event = event;
    FINISH_TRACE_PACKET();
}

// __HOOKED_xglCmdWaitEvents is manually written. Look in glvtrace_xgl_trace.c

// __HOOKED_xglCmdPipelineBarrier is manually written. Look in glvtrace_xgl_trace.c

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdBeginQuery(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    uint32_t slot,
    XGL_FLAGS flags)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBeginQuery* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdBeginQuery, 0);
    real_xglCmdBeginQuery(cmdBuffer, queryPool, slot, flags);
    pPacket = interpret_body_as_xglCmdBeginQuery(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->slot = slot;
    pPacket->flags = flags;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdEndQuery(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    uint32_t slot)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdEndQuery* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdEndQuery, 0);
    real_xglCmdEndQuery(cmdBuffer, queryPool, slot);
    pPacket = interpret_body_as_xglCmdEndQuery(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->slot = slot;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdResetQueryPool(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_QUERY_POOL queryPool,
    uint32_t startQuery,
    uint32_t queryCount)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdResetQueryPool* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdResetQueryPool, 0);
    real_xglCmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
    pPacket = interpret_body_as_xglCmdResetQueryPool(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->queryPool = queryPool;
    pPacket->startQuery = startQuery;
    pPacket->queryCount = queryCount;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdWriteTimestamp(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_TIMESTAMP_TYPE timestampType,
    XGL_BUFFER destBuffer,
    XGL_GPU_SIZE destOffset)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdWriteTimestamp* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdWriteTimestamp, 0);
    real_xglCmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
    pPacket = interpret_body_as_xglCmdWriteTimestamp(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->timestampType = timestampType;
    pPacket->destBuffer = destBuffer;
    pPacket->destOffset = destOffset;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdInitAtomicCounters(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    uint32_t startCounter,
    uint32_t counterCount,
    const uint32_t* pData)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdInitAtomicCounters* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdInitAtomicCounters, counterCount*sizeof(uint32_t));
    real_xglCmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
    pPacket = interpret_body_as_xglCmdInitAtomicCounters(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->startCounter = startCounter;
    pPacket->counterCount = counterCount;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pData), counterCount*sizeof(uint32_t), pData);
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pData));
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdLoadAtomicCounters(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    uint32_t startCounter,
    uint32_t counterCount,
    XGL_BUFFER srcBuffer,
    XGL_GPU_SIZE srcOffset)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdLoadAtomicCounters* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdLoadAtomicCounters, 0);
    real_xglCmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
    pPacket = interpret_body_as_xglCmdLoadAtomicCounters(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->startCounter = startCounter;
    pPacket->counterCount = counterCount;
    pPacket->srcBuffer = srcBuffer;
    pPacket->srcOffset = srcOffset;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdSaveAtomicCounters(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_PIPELINE_BIND_POINT pipelineBindPoint,
    uint32_t startCounter,
    uint32_t counterCount,
    XGL_BUFFER destBuffer,
    XGL_GPU_SIZE destOffset)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdSaveAtomicCounters* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdSaveAtomicCounters, 0);
    real_xglCmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
    pPacket = interpret_body_as_xglCmdSaveAtomicCounters(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->pipelineBindPoint = pipelineBindPoint;
    pPacket->startCounter = startCounter;
    pPacket->counterCount = counterCount;
    pPacket->destBuffer = destBuffer;
    pPacket->destOffset = destOffset;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateFramebuffer(
    XGL_DEVICE device,
    const XGL_FRAMEBUFFER_CREATE_INFO* pCreateInfo,
    XGL_FRAMEBUFFER* pFramebuffer)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateFramebuffer* pPacket = NULL;
    int dsSize = (pCreateInfo != NULL && pCreateInfo->pDepthStencilAttachment != NULL) ? sizeof(XGL_DEPTH_STENCIL_BIND_INFO) : 0;
    uint32_t colorCount = (pCreateInfo != NULL && pCreateInfo->pColorAttachments != NULL) ? pCreateInfo->colorAttachmentCount : 0;
    CREATE_TRACE_PACKET(xglCreateFramebuffer, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_FRAMEBUFFER));
    result = real_xglCreateFramebuffer(device, pCreateInfo, pFramebuffer);
    pPacket = interpret_body_as_xglCreateFramebuffer(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_FRAMEBUFFER_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments), colorCount * sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO), pCreateInfo->pColorAttachments);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment), dsSize, pCreateInfo->pDepthStencilAttachment);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pFramebuffer), sizeof(XGL_FRAMEBUFFER), pFramebuffer);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorAttachments));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pDepthStencilAttachment));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pFramebuffer));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglCreateRenderPass(
    XGL_DEVICE device,
    const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo,
    XGL_RENDER_PASS* pRenderPass)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglCreateRenderPass* pPacket = NULL;
    uint32_t colorCount = (pCreateInfo != NULL && (pCreateInfo->pColorLoadOps != NULL || pCreateInfo->pColorStoreOps != NULL || pCreateInfo->pColorLoadClearValues != NULL)) ? pCreateInfo->colorAttachmentCount : 0;
    CREATE_TRACE_PACKET(xglCreateRenderPass, get_struct_chain_size((void*)pCreateInfo) + sizeof(XGL_RENDER_PASS));
    result = real_xglCreateRenderPass(device, pCreateInfo, pRenderPass);
    pPacket = interpret_body_as_xglCreateRenderPass(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_RENDER_PASS_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps), colorCount * sizeof(XGL_ATTACHMENT_LOAD_OP), pCreateInfo->pColorLoadOps);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps), colorCount * sizeof(XGL_ATTACHMENT_STORE_OP), pCreateInfo->pColorStoreOps);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues), colorCount * sizeof(XGL_CLEAR_COLOR), pCreateInfo->pColorLoadClearValues);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pRenderPass), sizeof(XGL_RENDER_PASS), pRenderPass);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadOps));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorStoreOps));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo->pColorLoadClearValues));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pRenderPass));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdBeginRenderPass(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_RENDER_PASS renderPass)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdBeginRenderPass* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdBeginRenderPass, 0);
    real_xglCmdBeginRenderPass(cmdBuffer, renderPass);
    pPacket = interpret_body_as_xglCmdBeginRenderPass(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->renderPass = renderPass;
    FINISH_TRACE_PACKET();
}

GLVTRACER_EXPORT void XGLAPI __HOOKED_xglCmdEndRenderPass(
    XGL_CMD_BUFFER cmdBuffer,
    XGL_RENDER_PASS renderPass)
{
    glv_trace_packet_header* pHeader;
    struct_xglCmdEndRenderPass* pPacket = NULL;
    CREATE_TRACE_PACKET(xglCmdEndRenderPass, 0);
    real_xglCmdEndRenderPass(cmdBuffer, renderPass);
    pPacket = interpret_body_as_xglCmdEndRenderPass(pHeader);
    pPacket->cmdBuffer = cmdBuffer;
    pPacket->renderPass = renderPass;
    FINISH_TRACE_PACKET();
}

