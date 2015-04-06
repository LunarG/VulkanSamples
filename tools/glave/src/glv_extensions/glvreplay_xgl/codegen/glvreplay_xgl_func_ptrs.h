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

struct xglFuncs {
    void init_funcs(void * libHandle);
    void *m_libHandle;

    typedef XGL_RESULT( XGLAPI * type_xglCreateInstance)(
        const XGL_APPLICATION_INFO* pAppInfo,
        const XGL_ALLOC_CALLBACKS* pAllocCb,
        XGL_INSTANCE* pInstance);
    type_xglCreateInstance real_xglCreateInstance;
    typedef XGL_RESULT( XGLAPI * type_xglDestroyInstance)(
        XGL_INSTANCE instance);
    type_xglDestroyInstance real_xglDestroyInstance;
    typedef XGL_RESULT( XGLAPI * type_xglEnumerateGpus)(
        XGL_INSTANCE instance,
        uint32_t maxGpus,
        uint32_t* pGpuCount,
        XGL_PHYSICAL_GPU* pGpus);
    type_xglEnumerateGpus real_xglEnumerateGpus;
    typedef XGL_RESULT( XGLAPI * type_xglGetGpuInfo)(
        XGL_PHYSICAL_GPU gpu,
        XGL_PHYSICAL_GPU_INFO_TYPE infoType,
        size_t* pDataSize,
        void* pData);
    type_xglGetGpuInfo real_xglGetGpuInfo;
    typedef void*( XGLAPI * type_xglGetProcAddr)(
        XGL_PHYSICAL_GPU gpu,
        const char* pName);
    type_xglGetProcAddr real_xglGetProcAddr;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDevice)(
        XGL_PHYSICAL_GPU gpu,
        const XGL_DEVICE_CREATE_INFO* pCreateInfo,
        XGL_DEVICE* pDevice);
    type_xglCreateDevice real_xglCreateDevice;
    typedef XGL_RESULT( XGLAPI * type_xglDestroyDevice)(
        XGL_DEVICE device);
    type_xglDestroyDevice real_xglDestroyDevice;
    typedef XGL_RESULT( XGLAPI * type_xglGetExtensionSupport)(
        XGL_PHYSICAL_GPU gpu,
        const char* pExtName);
    type_xglGetExtensionSupport real_xglGetExtensionSupport;
    typedef XGL_RESULT( XGLAPI * type_xglEnumerateLayers)(
        XGL_PHYSICAL_GPU gpu,
        size_t maxLayerCount,
        size_t maxStringSize,
        size_t* pOutLayerCount,
        char* const* pOutLayers,
        void* pReserved);
    type_xglEnumerateLayers real_xglEnumerateLayers;
    typedef XGL_RESULT( XGLAPI * type_xglGetDeviceQueue)(
        XGL_DEVICE device,
        XGL_QUEUE_TYPE queueType,
        uint32_t queueIndex,
        XGL_QUEUE* pQueue);
    type_xglGetDeviceQueue real_xglGetDeviceQueue;
    typedef XGL_RESULT( XGLAPI * type_xglQueueSubmit)(
        XGL_QUEUE queue,
        uint32_t cmdBufferCount,
        const XGL_CMD_BUFFER* pCmdBuffers,
        uint32_t memRefCount,
        const XGL_MEMORY_REF* pMemRefs,
        XGL_FENCE fence);
    type_xglQueueSubmit real_xglQueueSubmit;
    typedef XGL_RESULT( XGLAPI * type_xglQueueSetGlobalMemReferences)(
        XGL_QUEUE queue,
        uint32_t memRefCount,
        const XGL_MEMORY_REF* pMemRefs);
    type_xglQueueSetGlobalMemReferences real_xglQueueSetGlobalMemReferences;
    typedef XGL_RESULT( XGLAPI * type_xglQueueWaitIdle)(
        XGL_QUEUE queue);
    type_xglQueueWaitIdle real_xglQueueWaitIdle;
    typedef XGL_RESULT( XGLAPI * type_xglDeviceWaitIdle)(
        XGL_DEVICE device);
    type_xglDeviceWaitIdle real_xglDeviceWaitIdle;
    typedef XGL_RESULT( XGLAPI * type_xglAllocMemory)(
        XGL_DEVICE device,
        const XGL_MEMORY_ALLOC_INFO* pAllocInfo,
        XGL_GPU_MEMORY* pMem);
    type_xglAllocMemory real_xglAllocMemory;
    typedef XGL_RESULT( XGLAPI * type_xglFreeMemory)(
        XGL_GPU_MEMORY mem);
    type_xglFreeMemory real_xglFreeMemory;
    typedef XGL_RESULT( XGLAPI * type_xglSetMemoryPriority)(
        XGL_GPU_MEMORY mem,
        XGL_MEMORY_PRIORITY priority);
    type_xglSetMemoryPriority real_xglSetMemoryPriority;
    typedef XGL_RESULT( XGLAPI * type_xglMapMemory)(
        XGL_GPU_MEMORY mem,
        XGL_FLAGS flags,
        void** ppData);
    type_xglMapMemory real_xglMapMemory;
    typedef XGL_RESULT( XGLAPI * type_xglUnmapMemory)(
        XGL_GPU_MEMORY mem);
    type_xglUnmapMemory real_xglUnmapMemory;
    typedef XGL_RESULT( XGLAPI * type_xglPinSystemMemory)(
        XGL_DEVICE device,
        const void* pSysMem,
        size_t memSize,
        XGL_GPU_MEMORY* pMem);
    type_xglPinSystemMemory real_xglPinSystemMemory;
    typedef XGL_RESULT( XGLAPI * type_xglGetMultiGpuCompatibility)(
        XGL_PHYSICAL_GPU gpu0,
        XGL_PHYSICAL_GPU gpu1,
        XGL_GPU_COMPATIBILITY_INFO* pInfo);
    type_xglGetMultiGpuCompatibility real_xglGetMultiGpuCompatibility;
    typedef XGL_RESULT( XGLAPI * type_xglOpenSharedMemory)(
        XGL_DEVICE device,
        const XGL_MEMORY_OPEN_INFO* pOpenInfo,
        XGL_GPU_MEMORY* pMem);
    type_xglOpenSharedMemory real_xglOpenSharedMemory;
    typedef XGL_RESULT( XGLAPI * type_xglOpenSharedQueueSemaphore)(
        XGL_DEVICE device,
        const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo,
        XGL_QUEUE_SEMAPHORE* pSemaphore);
    type_xglOpenSharedQueueSemaphore real_xglOpenSharedQueueSemaphore;
    typedef XGL_RESULT( XGLAPI * type_xglOpenPeerMemory)(
        XGL_DEVICE device,
        const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo,
        XGL_GPU_MEMORY* pMem);
    type_xglOpenPeerMemory real_xglOpenPeerMemory;
    typedef XGL_RESULT( XGLAPI * type_xglOpenPeerImage)(
        XGL_DEVICE device,
        const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo,
        XGL_IMAGE* pImage,
        XGL_GPU_MEMORY* pMem);
    type_xglOpenPeerImage real_xglOpenPeerImage;
    typedef XGL_RESULT( XGLAPI * type_xglDestroyObject)(
        XGL_OBJECT object);
    type_xglDestroyObject real_xglDestroyObject;
    typedef XGL_RESULT( XGLAPI * type_xglGetObjectInfo)(
        XGL_BASE_OBJECT object,
        XGL_OBJECT_INFO_TYPE infoType,
        size_t* pDataSize,
        void* pData);
    type_xglGetObjectInfo real_xglGetObjectInfo;
    typedef XGL_RESULT( XGLAPI * type_xglBindObjectMemory)(
        XGL_OBJECT object,
        uint32_t allocationIdx,
        XGL_GPU_MEMORY mem,
        XGL_GPU_SIZE offset);
    type_xglBindObjectMemory real_xglBindObjectMemory;
    typedef XGL_RESULT( XGLAPI * type_xglBindObjectMemoryRange)(
        XGL_OBJECT object,
        uint32_t allocationIdx,
        XGL_GPU_SIZE rangeOffset,
        XGL_GPU_SIZE rangeSize,
        XGL_GPU_MEMORY mem,
        XGL_GPU_SIZE memOffset);
    type_xglBindObjectMemoryRange real_xglBindObjectMemoryRange;
    typedef XGL_RESULT( XGLAPI * type_xglBindImageMemoryRange)(
        XGL_IMAGE image,
        uint32_t allocationIdx,
        const XGL_IMAGE_MEMORY_BIND_INFO* bindInfo,
        XGL_GPU_MEMORY mem,
        XGL_GPU_SIZE memOffset);
    type_xglBindImageMemoryRange real_xglBindImageMemoryRange;
    typedef XGL_RESULT( XGLAPI * type_xglCreateFence)(
        XGL_DEVICE device,
        const XGL_FENCE_CREATE_INFO* pCreateInfo,
        XGL_FENCE* pFence);
    type_xglCreateFence real_xglCreateFence;
    typedef XGL_RESULT( XGLAPI * type_xglGetFenceStatus)(
        XGL_FENCE fence);
    type_xglGetFenceStatus real_xglGetFenceStatus;
    typedef XGL_RESULT( XGLAPI * type_xglWaitForFences)(
        XGL_DEVICE device,
        uint32_t fenceCount,
        const XGL_FENCE* pFences,
        bool32_t waitAll,
        uint64_t timeout);
    type_xglWaitForFences real_xglWaitForFences;
    typedef XGL_RESULT( XGLAPI * type_xglCreateQueueSemaphore)(
        XGL_DEVICE device,
        const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo,
        XGL_QUEUE_SEMAPHORE* pSemaphore);
    type_xglCreateQueueSemaphore real_xglCreateQueueSemaphore;
    typedef XGL_RESULT( XGLAPI * type_xglSignalQueueSemaphore)(
        XGL_QUEUE queue,
        XGL_QUEUE_SEMAPHORE semaphore);
    type_xglSignalQueueSemaphore real_xglSignalQueueSemaphore;
    typedef XGL_RESULT( XGLAPI * type_xglWaitQueueSemaphore)(
        XGL_QUEUE queue,
        XGL_QUEUE_SEMAPHORE semaphore);
    type_xglWaitQueueSemaphore real_xglWaitQueueSemaphore;
    typedef XGL_RESULT( XGLAPI * type_xglCreateEvent)(
        XGL_DEVICE device,
        const XGL_EVENT_CREATE_INFO* pCreateInfo,
        XGL_EVENT* pEvent);
    type_xglCreateEvent real_xglCreateEvent;
    typedef XGL_RESULT( XGLAPI * type_xglGetEventStatus)(
        XGL_EVENT event);
    type_xglGetEventStatus real_xglGetEventStatus;
    typedef XGL_RESULT( XGLAPI * type_xglSetEvent)(
        XGL_EVENT event);
    type_xglSetEvent real_xglSetEvent;
    typedef XGL_RESULT( XGLAPI * type_xglResetEvent)(
        XGL_EVENT event);
    type_xglResetEvent real_xglResetEvent;
    typedef XGL_RESULT( XGLAPI * type_xglCreateQueryPool)(
        XGL_DEVICE device,
        const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo,
        XGL_QUERY_POOL* pQueryPool);
    type_xglCreateQueryPool real_xglCreateQueryPool;
    typedef XGL_RESULT( XGLAPI * type_xglGetQueryPoolResults)(
        XGL_QUERY_POOL queryPool,
        uint32_t startQuery,
        uint32_t queryCount,
        size_t* pDataSize,
        void* pData);
    type_xglGetQueryPoolResults real_xglGetQueryPoolResults;
    typedef XGL_RESULT( XGLAPI * type_xglGetFormatInfo)(
        XGL_DEVICE device,
        XGL_FORMAT format,
        XGL_FORMAT_INFO_TYPE infoType,
        size_t* pDataSize,
        void* pData);
    type_xglGetFormatInfo real_xglGetFormatInfo;
    typedef XGL_RESULT( XGLAPI * type_xglCreateBuffer)(
        XGL_DEVICE device,
        const XGL_BUFFER_CREATE_INFO* pCreateInfo,
        XGL_BUFFER* pBuffer);
    type_xglCreateBuffer real_xglCreateBuffer;
    typedef XGL_RESULT( XGLAPI * type_xglCreateBufferView)(
        XGL_DEVICE device,
        const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo,
        XGL_BUFFER_VIEW* pView);
    type_xglCreateBufferView real_xglCreateBufferView;
    typedef XGL_RESULT( XGLAPI * type_xglCreateImage)(
        XGL_DEVICE device,
        const XGL_IMAGE_CREATE_INFO* pCreateInfo,
        XGL_IMAGE* pImage);
    type_xglCreateImage real_xglCreateImage;
    typedef XGL_RESULT( XGLAPI * type_xglSetFastClearColor)(
        XGL_IMAGE image,
        const float color[4]);
    type_xglSetFastClearColor real_xglSetFastClearColor;
    typedef XGL_RESULT( XGLAPI * type_xglSetFastClearDepth)(
        XGL_IMAGE image,
        float depth);
    type_xglSetFastClearDepth real_xglSetFastClearDepth;
    typedef XGL_RESULT( XGLAPI * type_xglGetImageSubresourceInfo)(
        XGL_IMAGE image,
        const XGL_IMAGE_SUBRESOURCE* pSubresource,
        XGL_SUBRESOURCE_INFO_TYPE infoType,
        size_t* pDataSize,
        void* pData);
    type_xglGetImageSubresourceInfo real_xglGetImageSubresourceInfo;
    typedef XGL_RESULT( XGLAPI * type_xglCreateImageView)(
        XGL_DEVICE device,
        const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
        XGL_IMAGE_VIEW* pView);
    type_xglCreateImageView real_xglCreateImageView;
    typedef XGL_RESULT( XGLAPI * type_xglCreateColorAttachmentView)(
        XGL_DEVICE device,
        const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
        XGL_COLOR_ATTACHMENT_VIEW* pView);
    type_xglCreateColorAttachmentView real_xglCreateColorAttachmentView;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDepthStencilView)(
        XGL_DEVICE device,
        const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo,
        XGL_DEPTH_STENCIL_VIEW* pView);
    type_xglCreateDepthStencilView real_xglCreateDepthStencilView;
    typedef XGL_RESULT( XGLAPI * type_xglCreateShader)(
        XGL_DEVICE device,
        const XGL_SHADER_CREATE_INFO* pCreateInfo,
        XGL_SHADER* pShader);
    type_xglCreateShader real_xglCreateShader;
    typedef XGL_RESULT( XGLAPI * type_xglCreateGraphicsPipeline)(
        XGL_DEVICE device,
        const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
        XGL_PIPELINE* pPipeline);
    type_xglCreateGraphicsPipeline real_xglCreateGraphicsPipeline;
    typedef XGL_RESULT( XGLAPI * type_xglCreateComputePipeline)(
        XGL_DEVICE device,
        const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo,
        XGL_PIPELINE* pPipeline);
    type_xglCreateComputePipeline real_xglCreateComputePipeline;
    typedef XGL_RESULT( XGLAPI * type_xglStorePipeline)(
        XGL_PIPELINE pipeline,
        size_t* pDataSize,
        void* pData);
    type_xglStorePipeline real_xglStorePipeline;
    typedef XGL_RESULT( XGLAPI * type_xglLoadPipeline)(
        XGL_DEVICE device,
        size_t dataSize,
        const void* pData,
        XGL_PIPELINE* pPipeline);
    type_xglLoadPipeline real_xglLoadPipeline;
    typedef XGL_RESULT( XGLAPI * type_xglCreatePipelineDelta)(
        XGL_DEVICE device,
        XGL_PIPELINE p1,
        XGL_PIPELINE p2,
        XGL_PIPELINE_DELTA* delta);
    type_xglCreatePipelineDelta real_xglCreatePipelineDelta;
    typedef XGL_RESULT( XGLAPI * type_xglCreateSampler)(
        XGL_DEVICE device,
        const XGL_SAMPLER_CREATE_INFO* pCreateInfo,
        XGL_SAMPLER* pSampler);
    type_xglCreateSampler real_xglCreateSampler;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDescriptorSetLayout)(
        XGL_DEVICE device,
        XGL_FLAGS stageFlags,
        const uint32_t* pSetBindPoints,
        XGL_DESCRIPTOR_SET_LAYOUT priorSetLayout,
        const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList,
        XGL_DESCRIPTOR_SET_LAYOUT* pSetLayout);
    type_xglCreateDescriptorSetLayout real_xglCreateDescriptorSetLayout;
    typedef XGL_RESULT( XGLAPI * type_xglBeginDescriptorRegionUpdate)(
        XGL_DEVICE device,
        XGL_DESCRIPTOR_UPDATE_MODE updateMode);
    type_xglBeginDescriptorRegionUpdate real_xglBeginDescriptorRegionUpdate;
    typedef XGL_RESULT( XGLAPI * type_xglEndDescriptorRegionUpdate)(
        XGL_DEVICE device,
        XGL_CMD_BUFFER cmd);
    type_xglEndDescriptorRegionUpdate real_xglEndDescriptorRegionUpdate;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDescriptorRegion)(
        XGL_DEVICE device,
        XGL_DESCRIPTOR_REGION_USAGE regionUsage,
        uint32_t maxSets,
        const XGL_DESCRIPTOR_REGION_CREATE_INFO* pCreateInfo,
        XGL_DESCRIPTOR_REGION* pDescriptorRegion);
    type_xglCreateDescriptorRegion real_xglCreateDescriptorRegion;
    typedef XGL_RESULT( XGLAPI * type_xglClearDescriptorRegion)(
        XGL_DESCRIPTOR_REGION descriptorRegion);
    type_xglClearDescriptorRegion real_xglClearDescriptorRegion;
    typedef XGL_RESULT( XGLAPI * type_xglAllocDescriptorSets)(
        XGL_DESCRIPTOR_REGION descriptorRegion,
        XGL_DESCRIPTOR_SET_USAGE setUsage,
        uint32_t count,
        const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayouts,
        XGL_DESCRIPTOR_SET* pDescriptorSets,
        uint32_t* pCount);
    type_xglAllocDescriptorSets real_xglAllocDescriptorSets;
    typedef void( XGLAPI * type_xglClearDescriptorSets)(
        XGL_DESCRIPTOR_REGION descriptorRegion,
        uint32_t count,
        const XGL_DESCRIPTOR_SET* pDescriptorSets);
    type_xglClearDescriptorSets real_xglClearDescriptorSets;
    typedef void( XGLAPI * type_xglUpdateDescriptors)(
        XGL_DESCRIPTOR_SET descriptorSet,
        const void* pUpdateChain);
    type_xglUpdateDescriptors real_xglUpdateDescriptors;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDynamicViewportState)(
        XGL_DEVICE device,
        const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo,
        XGL_DYNAMIC_VP_STATE_OBJECT* pState);
    type_xglCreateDynamicViewportState real_xglCreateDynamicViewportState;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDynamicRasterState)(
        XGL_DEVICE device,
        const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo,
        XGL_DYNAMIC_RS_STATE_OBJECT* pState);
    type_xglCreateDynamicRasterState real_xglCreateDynamicRasterState;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDynamicColorBlendState)(
        XGL_DEVICE device,
        const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo,
        XGL_DYNAMIC_CB_STATE_OBJECT* pState);
    type_xglCreateDynamicColorBlendState real_xglCreateDynamicColorBlendState;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDynamicDepthStencilState)(
        XGL_DEVICE device,
        const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo,
        XGL_DYNAMIC_DS_STATE_OBJECT* pState);
    type_xglCreateDynamicDepthStencilState real_xglCreateDynamicDepthStencilState;
    typedef XGL_RESULT( XGLAPI * type_xglCreateCommandBuffer)(
        XGL_DEVICE device,
        const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo,
        XGL_CMD_BUFFER* pCmdBuffer);
    type_xglCreateCommandBuffer real_xglCreateCommandBuffer;
    typedef XGL_RESULT( XGLAPI * type_xglBeginCommandBuffer)(
        XGL_CMD_BUFFER cmdBuffer,
        const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo);
    type_xglBeginCommandBuffer real_xglBeginCommandBuffer;
    typedef XGL_RESULT( XGLAPI * type_xglEndCommandBuffer)(
        XGL_CMD_BUFFER cmdBuffer);
    type_xglEndCommandBuffer real_xglEndCommandBuffer;
    typedef XGL_RESULT( XGLAPI * type_xglResetCommandBuffer)(
        XGL_CMD_BUFFER cmdBuffer);
    type_xglResetCommandBuffer real_xglResetCommandBuffer;
    typedef void( XGLAPI * type_xglCmdBindPipeline)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_PIPELINE_BIND_POINT pipelineBindPoint,
        XGL_PIPELINE pipeline);
    type_xglCmdBindPipeline real_xglCmdBindPipeline;
    typedef void( XGLAPI * type_xglCmdBindPipelineDelta)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_PIPELINE_BIND_POINT pipelineBindPoint,
        XGL_PIPELINE_DELTA delta);
    type_xglCmdBindPipelineDelta real_xglCmdBindPipelineDelta;
    typedef void( XGLAPI * type_xglCmdBindDynamicStateObject)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_STATE_BIND_POINT stateBindPoint,
        XGL_DYNAMIC_STATE_OBJECT state);
    type_xglCmdBindDynamicStateObject real_xglCmdBindDynamicStateObject;
    typedef void( XGLAPI * type_xglCmdBindDescriptorSet)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_PIPELINE_BIND_POINT pipelineBindPoint,
        XGL_DESCRIPTOR_SET descriptorSet,
        const uint32_t* pUserData);
    type_xglCmdBindDescriptorSet real_xglCmdBindDescriptorSet;
    typedef void( XGLAPI * type_xglCmdBindVertexBuffer)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_BUFFER buffer,
        XGL_GPU_SIZE offset,
        uint32_t binding);
    type_xglCmdBindVertexBuffer real_xglCmdBindVertexBuffer;
    typedef void( XGLAPI * type_xglCmdBindIndexBuffer)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_BUFFER buffer,
        XGL_GPU_SIZE offset,
        XGL_INDEX_TYPE indexType);
    type_xglCmdBindIndexBuffer real_xglCmdBindIndexBuffer;
    typedef void( XGLAPI * type_xglCmdDraw)(
        XGL_CMD_BUFFER cmdBuffer,
        uint32_t firstVertex,
        uint32_t vertexCount,
        uint32_t firstInstance,
        uint32_t instanceCount);
    type_xglCmdDraw real_xglCmdDraw;
    typedef void( XGLAPI * type_xglCmdDrawIndexed)(
        XGL_CMD_BUFFER cmdBuffer,
        uint32_t firstIndex,
        uint32_t indexCount,
        int32_t vertexOffset,
        uint32_t firstInstance,
        uint32_t instanceCount);
    type_xglCmdDrawIndexed real_xglCmdDrawIndexed;
    typedef void( XGLAPI * type_xglCmdDrawIndirect)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_BUFFER buffer,
        XGL_GPU_SIZE offset,
        uint32_t count,
        uint32_t stride);
    type_xglCmdDrawIndirect real_xglCmdDrawIndirect;
    typedef void( XGLAPI * type_xglCmdDrawIndexedIndirect)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_BUFFER buffer,
        XGL_GPU_SIZE offset,
        uint32_t count,
        uint32_t stride);
    type_xglCmdDrawIndexedIndirect real_xglCmdDrawIndexedIndirect;
    typedef void( XGLAPI * type_xglCmdDispatch)(
        XGL_CMD_BUFFER cmdBuffer,
        uint32_t x,
        uint32_t y,
        uint32_t z);
    type_xglCmdDispatch real_xglCmdDispatch;
    typedef void( XGLAPI * type_xglCmdDispatchIndirect)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_BUFFER buffer,
        XGL_GPU_SIZE offset);
    type_xglCmdDispatchIndirect real_xglCmdDispatchIndirect;
    typedef void( XGLAPI * type_xglCmdCopyBuffer)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_BUFFER srcBuffer,
        XGL_BUFFER destBuffer,
        uint32_t regionCount,
        const XGL_BUFFER_COPY* pRegions);
    type_xglCmdCopyBuffer real_xglCmdCopyBuffer;
    typedef void( XGLAPI * type_xglCmdCopyImage)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_IMAGE srcImage,
        XGL_IMAGE destImage,
        uint32_t regionCount,
        const XGL_IMAGE_COPY* pRegions);
    type_xglCmdCopyImage real_xglCmdCopyImage;
    typedef void( XGLAPI * type_xglCmdCopyBufferToImage)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_BUFFER srcBuffer,
        XGL_IMAGE destImage,
        uint32_t regionCount,
        const XGL_BUFFER_IMAGE_COPY* pRegions);
    type_xglCmdCopyBufferToImage real_xglCmdCopyBufferToImage;
    typedef void( XGLAPI * type_xglCmdCopyImageToBuffer)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_IMAGE srcImage,
        XGL_BUFFER destBuffer,
        uint32_t regionCount,
        const XGL_BUFFER_IMAGE_COPY* pRegions);
    type_xglCmdCopyImageToBuffer real_xglCmdCopyImageToBuffer;
    typedef void( XGLAPI * type_xglCmdCloneImageData)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_IMAGE srcImage,
        XGL_IMAGE_LAYOUT srcImageLayout,
        XGL_IMAGE destImage,
        XGL_IMAGE_LAYOUT destImageLayout);
    type_xglCmdCloneImageData real_xglCmdCloneImageData;
    typedef void( XGLAPI * type_xglCmdUpdateBuffer)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_BUFFER destBuffer,
        XGL_GPU_SIZE destOffset,
        XGL_GPU_SIZE dataSize,
        const uint32_t* pData);
    type_xglCmdUpdateBuffer real_xglCmdUpdateBuffer;
    typedef void( XGLAPI * type_xglCmdFillBuffer)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_BUFFER destBuffer,
        XGL_GPU_SIZE destOffset,
        XGL_GPU_SIZE fillSize,
        uint32_t data);
    type_xglCmdFillBuffer real_xglCmdFillBuffer;
    typedef void( XGLAPI * type_xglCmdClearColorImage)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_IMAGE image,
        const float color[4],
        uint32_t rangeCount,
        const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
    type_xglCmdClearColorImage real_xglCmdClearColorImage;
    typedef void( XGLAPI * type_xglCmdClearColorImageRaw)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_IMAGE image,
        const uint32_t color[4],
        uint32_t rangeCount,
        const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
    type_xglCmdClearColorImageRaw real_xglCmdClearColorImageRaw;
    typedef void( XGLAPI * type_xglCmdClearDepthStencil)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_IMAGE image,
        float depth,
        uint32_t stencil,
        uint32_t rangeCount,
        const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
    type_xglCmdClearDepthStencil real_xglCmdClearDepthStencil;
    typedef void( XGLAPI * type_xglCmdResolveImage)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_IMAGE srcImage,
        XGL_IMAGE destImage,
        uint32_t rectCount,
        const XGL_IMAGE_RESOLVE* pRects);
    type_xglCmdResolveImage real_xglCmdResolveImage;
    typedef void( XGLAPI * type_xglCmdSetEvent)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_EVENT event,
        XGL_SET_EVENT pipeEvent);
    type_xglCmdSetEvent real_xglCmdSetEvent;
    typedef void( XGLAPI * type_xglCmdResetEvent)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_EVENT event);
    type_xglCmdResetEvent real_xglCmdResetEvent;
    typedef void( XGLAPI * type_xglCmdWaitEvents)(
        XGL_CMD_BUFFER cmdBuffer,
        const XGL_EVENT_WAIT_INFO* pWaitInfo);
    type_xglCmdWaitEvents real_xglCmdWaitEvents;
    typedef void( XGLAPI * type_xglCmdPipelineBarrier)(
        XGL_CMD_BUFFER cmdBuffer,
        const XGL_PIPELINE_BARRIER* pBarrier);
    type_xglCmdPipelineBarrier real_xglCmdPipelineBarrier;
    typedef void( XGLAPI * type_xglCmdBeginQuery)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_QUERY_POOL queryPool,
        uint32_t slot,
        XGL_FLAGS flags);
    type_xglCmdBeginQuery real_xglCmdBeginQuery;
    typedef void( XGLAPI * type_xglCmdEndQuery)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_QUERY_POOL queryPool,
        uint32_t slot);
    type_xglCmdEndQuery real_xglCmdEndQuery;
    typedef void( XGLAPI * type_xglCmdResetQueryPool)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_QUERY_POOL queryPool,
        uint32_t startQuery,
        uint32_t queryCount);
    type_xglCmdResetQueryPool real_xglCmdResetQueryPool;
    typedef void( XGLAPI * type_xglCmdWriteTimestamp)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_TIMESTAMP_TYPE timestampType,
        XGL_BUFFER destBuffer,
        XGL_GPU_SIZE destOffset);
    type_xglCmdWriteTimestamp real_xglCmdWriteTimestamp;
    typedef void( XGLAPI * type_xglCmdInitAtomicCounters)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_PIPELINE_BIND_POINT pipelineBindPoint,
        uint32_t startCounter,
        uint32_t counterCount,
        const uint32_t* pData);
    type_xglCmdInitAtomicCounters real_xglCmdInitAtomicCounters;
    typedef void( XGLAPI * type_xglCmdLoadAtomicCounters)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_PIPELINE_BIND_POINT pipelineBindPoint,
        uint32_t startCounter,
        uint32_t counterCount,
        XGL_BUFFER srcBuffer,
        XGL_GPU_SIZE srcOffset);
    type_xglCmdLoadAtomicCounters real_xglCmdLoadAtomicCounters;
    typedef void( XGLAPI * type_xglCmdSaveAtomicCounters)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_PIPELINE_BIND_POINT pipelineBindPoint,
        uint32_t startCounter,
        uint32_t counterCount,
        XGL_BUFFER destBuffer,
        XGL_GPU_SIZE destOffset);
    type_xglCmdSaveAtomicCounters real_xglCmdSaveAtomicCounters;
    typedef XGL_RESULT( XGLAPI * type_xglCreateFramebuffer)(
        XGL_DEVICE device,
        const XGL_FRAMEBUFFER_CREATE_INFO* pCreateInfo,
        XGL_FRAMEBUFFER* pFramebuffer);
    type_xglCreateFramebuffer real_xglCreateFramebuffer;
    typedef XGL_RESULT( XGLAPI * type_xglCreateRenderPass)(
        XGL_DEVICE device,
        const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo,
        XGL_RENDER_PASS* pRenderPass);
    type_xglCreateRenderPass real_xglCreateRenderPass;
    typedef void( XGLAPI * type_xglCmdBeginRenderPass)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_RENDER_PASS renderPass);
    type_xglCmdBeginRenderPass real_xglCmdBeginRenderPass;
    typedef void( XGLAPI * type_xglCmdEndRenderPass)(
        XGL_CMD_BUFFER cmdBuffer,
        XGL_RENDER_PASS renderPass);
    type_xglCmdEndRenderPass real_xglCmdEndRenderPass;
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetValidationLevel)(
        XGL_DEVICE device,
        XGL_VALIDATION_LEVEL validationLevel);
    type_xglDbgSetValidationLevel real_xglDbgSetValidationLevel;
    typedef XGL_RESULT( XGLAPI * type_xglDbgRegisterMsgCallback)(
        XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback,
        void* pUserData);
    type_xglDbgRegisterMsgCallback real_xglDbgRegisterMsgCallback;
    typedef XGL_RESULT( XGLAPI * type_xglDbgUnregisterMsgCallback)(
        XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);
    type_xglDbgUnregisterMsgCallback real_xglDbgUnregisterMsgCallback;
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetMessageFilter)(
        XGL_DEVICE device,
        int32_t msgCode,
        XGL_DBG_MSG_FILTER filter);
    type_xglDbgSetMessageFilter real_xglDbgSetMessageFilter;
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetObjectTag)(
        XGL_BASE_OBJECT object,
        size_t tagSize,
        const void* pTag);
    type_xglDbgSetObjectTag real_xglDbgSetObjectTag;
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetGlobalOption)(
        XGL_DBG_GLOBAL_OPTION dbgOption,
        size_t dataSize,
        const void* pData);
    type_xglDbgSetGlobalOption real_xglDbgSetGlobalOption;
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetDeviceOption)(
        XGL_DEVICE device,
        XGL_DBG_DEVICE_OPTION dbgOption,
        size_t dataSize,
        const void* pData);
    type_xglDbgSetDeviceOption real_xglDbgSetDeviceOption;
    typedef void( XGLAPI * type_xglCmdDbgMarkerBegin)(
        XGL_CMD_BUFFER cmdBuffer,
        const char* pMarker);
    type_xglCmdDbgMarkerBegin real_xglCmdDbgMarkerBegin;
    typedef void( XGLAPI * type_xglCmdDbgMarkerEnd)(
        XGL_CMD_BUFFER cmdBuffer);
    type_xglCmdDbgMarkerEnd real_xglCmdDbgMarkerEnd;
    typedef XGL_RESULT( XGLAPI * type_xglWsiX11AssociateConnection)(
        XGL_PHYSICAL_GPU gpu,
        const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo);
    type_xglWsiX11AssociateConnection real_xglWsiX11AssociateConnection;
    typedef XGL_RESULT( XGLAPI * type_xglWsiX11GetMSC)(
        XGL_DEVICE device,
        xcb_window_t window,
        xcb_randr_crtc_t crtc,
        uint64_t* pMsc);
    type_xglWsiX11GetMSC real_xglWsiX11GetMSC;
    typedef XGL_RESULT( XGLAPI * type_xglWsiX11CreatePresentableImage)(
        XGL_DEVICE device,
        const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
        XGL_IMAGE* pImage,
        XGL_GPU_MEMORY* pMem);
    type_xglWsiX11CreatePresentableImage real_xglWsiX11CreatePresentableImage;
    typedef XGL_RESULT( XGLAPI * type_xglWsiX11QueuePresent)(
        XGL_QUEUE queue,
        const XGL_WSI_X11_PRESENT_INFO* pPresentInfo,
        XGL_FENCE fence);
    type_xglWsiX11QueuePresent real_xglWsiX11QueuePresent;
};
