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

#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include <xcb/xcb.h>

#endif
#include "glvreplay_window.h"
#include "glvreplay_factory.h"
#include "glv_trace_packet_identifiers.h"

#include "xgl.h"
#include "xglDbg.h"
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include "xglWsiX11Ext.h"
#else
#include "xglWsiWinExt.h"
#endif
#include "draw_state.h"

class ApiReplay {
public:
    virtual ~ApiReplay() { }
    virtual enum glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header * packet) = 0;
    virtual int init(glv_replay::Display & disp) = 0;
    virtual void push_validation_msg(XGL_VALIDATION_LEVEL validationLevel, XGL_BASE_OBJECT srcObject, size_t location, int32_t msgCode, const char* pMsg) = 0;
    virtual glv_replay::GLV_REPLAY_RESULT pop_validation_msgs() = 0;
    virtual int dump_validation_data() = 0;
};

class xglDisplay: public glv_replay::DisplayImp {
friend class xglReplay;
public:
    xglDisplay();
    ~xglDisplay();
    int init(const unsigned int gpu_idx);
    int set_window(glv_window_handle hWindow, unsigned int width, unsigned int height);
    int create_window(const unsigned int width, const unsigned int height);
    void resize_window(const unsigned int width, const unsigned int height);
    void process_event();
    // XGL_DEVICE get_device() { return m_dev[m_gpuIdx];}
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    xcb_window_t get_window_handle() { return m_XcbWindow; }
#elif defined(WIN32)
    HWND get_window_handle() { return m_windowHandle; }
#endif
private:
    XGL_RESULT init_xgl(const unsigned int gpu_idx);
    bool m_initedXGL;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    XGL_WSI_X11_CONNECTION_INFO m_WsiConnection;
    xcb_screen_t *m_pXcbScreen;
    xcb_window_t m_XcbWindow;
#elif defined(WIN32)
    HWND m_windowHandle;
#endif
    unsigned int m_windowWidth;
    unsigned int m_windowHeight;
    unsigned int m_frameNumber;
    std::vector<uint32_t> imageWidth;
    std::vector<uint32_t> imageHeight;
    std::vector<XGL_IMAGE> imageHandles;
    std::vector<XGL_GPU_MEMORY> imageMemory;
#if 0
    XGL_DEVICE m_dev[XGL_MAX_PHYSICAL_GPUS];
    uint32_t m_gpuCount;
    unsigned int m_gpuIdx;
    XGL_PHYSICAL_GPU m_gpus[XGL_MAX_PHYSICAL_GPUS];
    XGL_PHYSICAL_GPU_PROPERTIES m_gpuProps[XGL_MAX_PHYSICAL_GPUS];
#endif
    std::vector<char *>m_extensions;
};

class objMemory {
public:
    objMemory() : m_numAllocations(0), m_pMemReqs(NULL) {}
    ~objMemory() { free(m_pMemReqs);}
    void setCount(const uint32_t num);
    void setReqs(const XGL_MEMORY_REQUIREMENTS *pReqs, const uint32_t num);
private:
    uint32_t m_numAllocations;
    XGL_MEMORY_REQUIREMENTS *m_pMemReqs;
};

class gpuMemory {
public:
    gpuMemory() : m_pendingAlloc(false) {m_allocInfo.allocationSize = 0;}
    ~gpuMemory() {}
    bool isPendingAlloc();
    void setAllocInfo(const XGL_MEMORY_ALLOC_INFO *info, const bool pending);
    void setMemoryDataAddr(void *pBuf);
    void setMemoryMapRange(void *pBuf, const size_t size, const size_t offset, const bool pending);
    void copyMappingData(const void *pSrcData);
    size_t getMemoryMapSize();
private:
    bool m_pendingAlloc;
    struct mapRange {
        bool pending;
        size_t size;
        size_t offset;
        void *pData;
    };
    std::vector<struct mapRange> m_mapRange;
    XGL_MEMORY_ALLOC_INFO m_allocInfo;
};

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
class xglReplay : public ApiReplay {
public:
    ~xglReplay();
    xglReplay(glvreplay_settings *pReplaySettings);

    int init(glv_replay::Display & disp);
    xglDisplay * get_display() {return m_display;}
    glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header *packet);
    glv_replay::GLV_REPLAY_RESULT handle_replay_errors(const char* entrypointName, const XGL_RESULT resCall, const XGL_RESULT resTrace, const glv_replay::GLV_REPLAY_RESULT resIn);

    void push_validation_msg(XGL_VALIDATION_LEVEL validationLevel, XGL_BASE_OBJECT srcObject, size_t location, int32_t msgCode, const char* pMsg);
    glv_replay::GLV_REPLAY_RESULT pop_validation_msgs();
    int dump_validation_data();
private:
    struct xglFuncs m_xglFuncs;
    DRAW_STATE_DUMP_DOT_FILE m_pDSDump;
    DRAW_STATE_DUMP_COMMAND_BUFFER_DOT_FILE m_pCBDump;
    xglDisplay *m_display;
    struct shaderPair {
        XGL_SHADER *addr;
        XGL_SHADER val;
    };
    struct validationMsg {
        XGL_VALIDATION_LEVEL validationLevel;
        XGL_BASE_OBJECT srcObject;
        size_t location;
        int32_t msgCode;
        char msg[256];
    };
    std::vector<struct validationMsg> m_validationMsgs;
    std::vector<int> m_screenshotFrames;
    bool m_adjustForGPU; // true if replay adjusts behavior based on GPU
    struct imageObj {
       objMemory imageMem;
       XGL_IMAGE replayImage;
    };

    struct bufferObj {
       objMemory bufferMem;
       XGL_BUFFER replayBuffer;
    };

    struct gpuMemObj {
       gpuMemory *pGpuMem;
       XGL_GPU_MEMORY replayGpuMem;
    };

    void init_objMemCount(const XGL_BASE_OBJECT& object, const uint32_t &num)
    {
        XGL_IMAGE img = static_cast <XGL_IMAGE> (object);
        std::map<XGL_IMAGE, struct imageObj>::const_iterator it = m_images.find(img);
        if (it != m_images.end())
        {
            objMemory obj = it->second.imageMem;
            obj.setCount(num);
            return;
        }
        XGL_BUFFER buf = static_cast <XGL_BUFFER> (object);
        std::map<XGL_BUFFER, struct bufferObj>::const_iterator itb = m_buffers.find(buf);
        if (itb != m_buffers.end())
        {
            objMemory obj = itb->second.bufferMem;
            obj.setCount(num);
            return;
        }
        return;
    }

    void init_objMemReqs(const XGL_BASE_OBJECT& object, const XGL_MEMORY_REQUIREMENTS *pMemReqs, const unsigned int num)
    {
        XGL_IMAGE img = static_cast <XGL_IMAGE> (object);
        std::map<XGL_IMAGE, struct imageObj>::const_iterator it = m_images.find(img);
        if (it != m_images.end())
        {
            objMemory obj = it->second.imageMem;
            obj.setReqs(pMemReqs, num);
            return;
        }
        XGL_BUFFER buf = static_cast <XGL_BUFFER> (object);
        std::map<XGL_BUFFER, struct bufferObj>::const_iterator itb = m_buffers.find(buf);
        if (itb != m_buffers.end())
        {
            objMemory obj = itb->second.bufferMem;
            obj.setReqs(pMemReqs, num);
            return;
        }
        return;
    }

    void clear_all_map_handles()
    {
        m_bufferViews.clear();
        m_buffers.clear();
        m_cmdBuffers.clear();
        m_colorAttachmentViews.clear();
        m_depthStencilViews.clear();
        m_descriptorRegions.clear();
        m_descriptorSetLayouts.clear();
        m_descriptorSets.clear();
        m_devices.clear();
        m_dynamicCbStateObjects.clear();
        m_dynamicDsStateObjects.clear();
        m_dynamicRsStateObjects.clear();
        m_dynamicVpStateObjects.clear();
        m_events.clear();
        m_fences.clear();
        m_framebuffers.clear();
        m_gpuMemorys.clear();
        m_imageViews.clear();
        m_images.clear();
        m_instances.clear();
        m_physicalGpus.clear();
        m_pipelineDeltas.clear();
        m_pipelines.clear();
        m_queryPools.clear();
        m_queueSemaphores.clear();
        m_queues.clear();
        m_renderPasss.clear();
        m_samplers.clear();
        m_shaders.clear();
    }

    std::map<XGL_BUFFER_VIEW, XGL_BUFFER_VIEW> m_bufferViews;
    void add_to_map(XGL_BUFFER_VIEW* pTraceVal, XGL_BUFFER_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_bufferViews[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_BUFFER_VIEW& key)
    {
        m_bufferViews.erase(key);
    }

    XGL_BUFFER_VIEW remap(const XGL_BUFFER_VIEW& value)
    {
        std::map<XGL_BUFFER_VIEW, XGL_BUFFER_VIEW>::const_iterator q = m_bufferViews.find(value);
        return (q == m_bufferViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_BUFFER, struct bufferObj> m_buffers;
    void add_to_map(XGL_BUFFER* pTraceVal, struct bufferObj* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_buffers[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_BUFFER& key)
    {
        m_buffers.erase(key);
    }

    XGL_BUFFER remap(const XGL_BUFFER& value)
    {
        std::map<XGL_BUFFER, struct bufferObj>::const_iterator q = m_buffers.find(value);
        return (q == m_buffers.end()) ? XGL_NULL_HANDLE : q->second.replayBuffer;
    }

    std::map<XGL_CMD_BUFFER, XGL_CMD_BUFFER> m_cmdBuffers;
    void add_to_map(XGL_CMD_BUFFER* pTraceVal, XGL_CMD_BUFFER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_cmdBuffers[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_CMD_BUFFER& key)
    {
        m_cmdBuffers.erase(key);
    }

    XGL_CMD_BUFFER remap(const XGL_CMD_BUFFER& value)
    {
        std::map<XGL_CMD_BUFFER, XGL_CMD_BUFFER>::const_iterator q = m_cmdBuffers.find(value);
        return (q == m_cmdBuffers.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_COLOR_ATTACHMENT_VIEW, XGL_COLOR_ATTACHMENT_VIEW> m_colorAttachmentViews;
    void add_to_map(XGL_COLOR_ATTACHMENT_VIEW* pTraceVal, XGL_COLOR_ATTACHMENT_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_colorAttachmentViews[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_COLOR_ATTACHMENT_VIEW& key)
    {
        m_colorAttachmentViews.erase(key);
    }

    XGL_COLOR_ATTACHMENT_VIEW remap(const XGL_COLOR_ATTACHMENT_VIEW& value)
    {
        std::map<XGL_COLOR_ATTACHMENT_VIEW, XGL_COLOR_ATTACHMENT_VIEW>::const_iterator q = m_colorAttachmentViews.find(value);
        return (q == m_colorAttachmentViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DEPTH_STENCIL_VIEW, XGL_DEPTH_STENCIL_VIEW> m_depthStencilViews;
    void add_to_map(XGL_DEPTH_STENCIL_VIEW* pTraceVal, XGL_DEPTH_STENCIL_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_depthStencilViews[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DEPTH_STENCIL_VIEW& key)
    {
        m_depthStencilViews.erase(key);
    }

    XGL_DEPTH_STENCIL_VIEW remap(const XGL_DEPTH_STENCIL_VIEW& value)
    {
        std::map<XGL_DEPTH_STENCIL_VIEW, XGL_DEPTH_STENCIL_VIEW>::const_iterator q = m_depthStencilViews.find(value);
        return (q == m_depthStencilViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DESCRIPTOR_REGION, XGL_DESCRIPTOR_REGION> m_descriptorRegions;
    void add_to_map(XGL_DESCRIPTOR_REGION* pTraceVal, XGL_DESCRIPTOR_REGION* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_descriptorRegions[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DESCRIPTOR_REGION& key)
    {
        m_descriptorRegions.erase(key);
    }

    XGL_DESCRIPTOR_REGION remap(const XGL_DESCRIPTOR_REGION& value)
    {
        std::map<XGL_DESCRIPTOR_REGION, XGL_DESCRIPTOR_REGION>::const_iterator q = m_descriptorRegions.find(value);
        return (q == m_descriptorRegions.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DESCRIPTOR_SET_LAYOUT, XGL_DESCRIPTOR_SET_LAYOUT> m_descriptorSetLayouts;
    void add_to_map(XGL_DESCRIPTOR_SET_LAYOUT* pTraceVal, XGL_DESCRIPTOR_SET_LAYOUT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_descriptorSetLayouts[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DESCRIPTOR_SET_LAYOUT& key)
    {
        m_descriptorSetLayouts.erase(key);
    }

    XGL_DESCRIPTOR_SET_LAYOUT remap(const XGL_DESCRIPTOR_SET_LAYOUT& value)
    {
        std::map<XGL_DESCRIPTOR_SET_LAYOUT, XGL_DESCRIPTOR_SET_LAYOUT>::const_iterator q = m_descriptorSetLayouts.find(value);
        return (q == m_descriptorSetLayouts.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DESCRIPTOR_SET, XGL_DESCRIPTOR_SET> m_descriptorSets;
    void add_to_map(XGL_DESCRIPTOR_SET* pTraceVal, XGL_DESCRIPTOR_SET* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_descriptorSets[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DESCRIPTOR_SET& key)
    {
        m_descriptorSets.erase(key);
    }

    XGL_DESCRIPTOR_SET remap(const XGL_DESCRIPTOR_SET& value)
    {
        std::map<XGL_DESCRIPTOR_SET, XGL_DESCRIPTOR_SET>::const_iterator q = m_descriptorSets.find(value);
        return (q == m_descriptorSets.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DEVICE, XGL_DEVICE> m_devices;
    void add_to_map(XGL_DEVICE* pTraceVal, XGL_DEVICE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_devices[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DEVICE& key)
    {
        m_devices.erase(key);
    }

    XGL_DEVICE remap(const XGL_DEVICE& value)
    {
        std::map<XGL_DEVICE, XGL_DEVICE>::const_iterator q = m_devices.find(value);
        return (q == m_devices.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DYNAMIC_CB_STATE_OBJECT, XGL_DYNAMIC_CB_STATE_OBJECT> m_dynamicCbStateObjects;
    void add_to_map(XGL_DYNAMIC_CB_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_CB_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicCbStateObjects[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DYNAMIC_CB_STATE_OBJECT& key)
    {
        m_dynamicCbStateObjects.erase(key);
    }

    XGL_DYNAMIC_CB_STATE_OBJECT remap(const XGL_DYNAMIC_CB_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_CB_STATE_OBJECT, XGL_DYNAMIC_CB_STATE_OBJECT>::const_iterator q = m_dynamicCbStateObjects.find(value);
        return (q == m_dynamicCbStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DYNAMIC_DS_STATE_OBJECT, XGL_DYNAMIC_DS_STATE_OBJECT> m_dynamicDsStateObjects;
    void add_to_map(XGL_DYNAMIC_DS_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_DS_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicDsStateObjects[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DYNAMIC_DS_STATE_OBJECT& key)
    {
        m_dynamicDsStateObjects.erase(key);
    }

    XGL_DYNAMIC_DS_STATE_OBJECT remap(const XGL_DYNAMIC_DS_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_DS_STATE_OBJECT, XGL_DYNAMIC_DS_STATE_OBJECT>::const_iterator q = m_dynamicDsStateObjects.find(value);
        return (q == m_dynamicDsStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DYNAMIC_RS_STATE_OBJECT, XGL_DYNAMIC_RS_STATE_OBJECT> m_dynamicRsStateObjects;
    void add_to_map(XGL_DYNAMIC_RS_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_RS_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicRsStateObjects[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DYNAMIC_RS_STATE_OBJECT& key)
    {
        m_dynamicRsStateObjects.erase(key);
    }

    XGL_DYNAMIC_RS_STATE_OBJECT remap(const XGL_DYNAMIC_RS_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_RS_STATE_OBJECT, XGL_DYNAMIC_RS_STATE_OBJECT>::const_iterator q = m_dynamicRsStateObjects.find(value);
        return (q == m_dynamicRsStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DYNAMIC_VP_STATE_OBJECT, XGL_DYNAMIC_VP_STATE_OBJECT> m_dynamicVpStateObjects;
    void add_to_map(XGL_DYNAMIC_VP_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_VP_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicVpStateObjects[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DYNAMIC_VP_STATE_OBJECT& key)
    {
        m_dynamicVpStateObjects.erase(key);
    }

    XGL_DYNAMIC_VP_STATE_OBJECT remap(const XGL_DYNAMIC_VP_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_VP_STATE_OBJECT, XGL_DYNAMIC_VP_STATE_OBJECT>::const_iterator q = m_dynamicVpStateObjects.find(value);
        return (q == m_dynamicVpStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_EVENT, XGL_EVENT> m_events;
    void add_to_map(XGL_EVENT* pTraceVal, XGL_EVENT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_events[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_EVENT& key)
    {
        m_events.erase(key);
    }

    XGL_EVENT remap(const XGL_EVENT& value)
    {
        std::map<XGL_EVENT, XGL_EVENT>::const_iterator q = m_events.find(value);
        return (q == m_events.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_FENCE, XGL_FENCE> m_fences;
    void add_to_map(XGL_FENCE* pTraceVal, XGL_FENCE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_fences[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_FENCE& key)
    {
        m_fences.erase(key);
    }

    XGL_FENCE remap(const XGL_FENCE& value)
    {
        std::map<XGL_FENCE, XGL_FENCE>::const_iterator q = m_fences.find(value);
        return (q == m_fences.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_FRAMEBUFFER, XGL_FRAMEBUFFER> m_framebuffers;
    void add_to_map(XGL_FRAMEBUFFER* pTraceVal, XGL_FRAMEBUFFER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_framebuffers[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_FRAMEBUFFER& key)
    {
        m_framebuffers.erase(key);
    }

    XGL_FRAMEBUFFER remap(const XGL_FRAMEBUFFER& value)
    {
        std::map<XGL_FRAMEBUFFER, XGL_FRAMEBUFFER>::const_iterator q = m_framebuffers.find(value);
        return (q == m_framebuffers.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_GPU_MEMORY, struct gpuMemObj> m_gpuMemorys;
    void add_to_map(XGL_GPU_MEMORY* pTraceVal, struct gpuMemObj* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_gpuMemorys[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_GPU_MEMORY& key)
    {
        m_gpuMemorys.erase(key);
    }

    XGL_GPU_MEMORY remap(const XGL_GPU_MEMORY& value)
    {
        std::map<XGL_GPU_MEMORY, struct gpuMemObj>::const_iterator q = m_gpuMemorys.find(value);
        return (q == m_gpuMemorys.end()) ? XGL_NULL_HANDLE : q->second.replayGpuMem;
    }

    std::map<XGL_IMAGE_VIEW, XGL_IMAGE_VIEW> m_imageViews;
    void add_to_map(XGL_IMAGE_VIEW* pTraceVal, XGL_IMAGE_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_imageViews[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_IMAGE_VIEW& key)
    {
        m_imageViews.erase(key);
    }

    XGL_IMAGE_VIEW remap(const XGL_IMAGE_VIEW& value)
    {
        std::map<XGL_IMAGE_VIEW, XGL_IMAGE_VIEW>::const_iterator q = m_imageViews.find(value);
        return (q == m_imageViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_IMAGE, struct imageObj> m_images;
    void add_to_map(XGL_IMAGE* pTraceVal, struct imageObj* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_images[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_IMAGE& key)
    {
        m_images.erase(key);
    }

    XGL_IMAGE remap(const XGL_IMAGE& value)
    {
        std::map<XGL_IMAGE, struct imageObj>::const_iterator q = m_images.find(value);
        return (q == m_images.end()) ? XGL_NULL_HANDLE : q->second.replayImage;
    }

    std::map<XGL_INSTANCE, XGL_INSTANCE> m_instances;
    void add_to_map(XGL_INSTANCE* pTraceVal, XGL_INSTANCE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_instances[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_INSTANCE& key)
    {
        m_instances.erase(key);
    }

    XGL_INSTANCE remap(const XGL_INSTANCE& value)
    {
        std::map<XGL_INSTANCE, XGL_INSTANCE>::const_iterator q = m_instances.find(value);
        return (q == m_instances.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU> m_physicalGpus;
    void add_to_map(XGL_PHYSICAL_GPU* pTraceVal, XGL_PHYSICAL_GPU* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_physicalGpus[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_PHYSICAL_GPU& key)
    {
        m_physicalGpus.erase(key);
    }

    XGL_PHYSICAL_GPU remap(const XGL_PHYSICAL_GPU& value)
    {
        std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU>::const_iterator q = m_physicalGpus.find(value);
        return (q == m_physicalGpus.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_PIPELINE_DELTA, XGL_PIPELINE_DELTA> m_pipelineDeltas;
    void add_to_map(XGL_PIPELINE_DELTA* pTraceVal, XGL_PIPELINE_DELTA* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_pipelineDeltas[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_PIPELINE_DELTA& key)
    {
        m_pipelineDeltas.erase(key);
    }

    XGL_PIPELINE_DELTA remap(const XGL_PIPELINE_DELTA& value)
    {
        std::map<XGL_PIPELINE_DELTA, XGL_PIPELINE_DELTA>::const_iterator q = m_pipelineDeltas.find(value);
        return (q == m_pipelineDeltas.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_PIPELINE, XGL_PIPELINE> m_pipelines;
    void add_to_map(XGL_PIPELINE* pTraceVal, XGL_PIPELINE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_pipelines[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_PIPELINE& key)
    {
        m_pipelines.erase(key);
    }

    XGL_PIPELINE remap(const XGL_PIPELINE& value)
    {
        std::map<XGL_PIPELINE, XGL_PIPELINE>::const_iterator q = m_pipelines.find(value);
        return (q == m_pipelines.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_QUERY_POOL, XGL_QUERY_POOL> m_queryPools;
    void add_to_map(XGL_QUERY_POOL* pTraceVal, XGL_QUERY_POOL* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_queryPools[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_QUERY_POOL& key)
    {
        m_queryPools.erase(key);
    }

    XGL_QUERY_POOL remap(const XGL_QUERY_POOL& value)
    {
        std::map<XGL_QUERY_POOL, XGL_QUERY_POOL>::const_iterator q = m_queryPools.find(value);
        return (q == m_queryPools.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_QUEUE_SEMAPHORE, XGL_QUEUE_SEMAPHORE> m_queueSemaphores;
    void add_to_map(XGL_QUEUE_SEMAPHORE* pTraceVal, XGL_QUEUE_SEMAPHORE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_queueSemaphores[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_QUEUE_SEMAPHORE& key)
    {
        m_queueSemaphores.erase(key);
    }

    XGL_QUEUE_SEMAPHORE remap(const XGL_QUEUE_SEMAPHORE& value)
    {
        std::map<XGL_QUEUE_SEMAPHORE, XGL_QUEUE_SEMAPHORE>::const_iterator q = m_queueSemaphores.find(value);
        return (q == m_queueSemaphores.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_QUEUE, XGL_QUEUE> m_queues;
    void add_to_map(XGL_QUEUE* pTraceVal, XGL_QUEUE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_queues[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_QUEUE& key)
    {
        m_queues.erase(key);
    }

    XGL_QUEUE remap(const XGL_QUEUE& value)
    {
        std::map<XGL_QUEUE, XGL_QUEUE>::const_iterator q = m_queues.find(value);
        return (q == m_queues.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_RENDER_PASS, XGL_RENDER_PASS> m_renderPasss;
    void add_to_map(XGL_RENDER_PASS* pTraceVal, XGL_RENDER_PASS* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_renderPasss[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_RENDER_PASS& key)
    {
        m_renderPasss.erase(key);
    }

    XGL_RENDER_PASS remap(const XGL_RENDER_PASS& value)
    {
        std::map<XGL_RENDER_PASS, XGL_RENDER_PASS>::const_iterator q = m_renderPasss.find(value);
        return (q == m_renderPasss.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_SAMPLER, XGL_SAMPLER> m_samplers;
    void add_to_map(XGL_SAMPLER* pTraceVal, XGL_SAMPLER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_samplers[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_SAMPLER& key)
    {
        m_samplers.erase(key);
    }

    XGL_SAMPLER remap(const XGL_SAMPLER& value)
    {
        std::map<XGL_SAMPLER, XGL_SAMPLER>::const_iterator q = m_samplers.find(value);
        return (q == m_samplers.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_SHADER, XGL_SHADER> m_shaders;
    void add_to_map(XGL_SHADER* pTraceVal, XGL_SHADER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_shaders[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_SHADER& key)
    {
        m_shaders.erase(key);
    }

    XGL_SHADER remap(const XGL_SHADER& value)
    {
        std::map<XGL_SHADER, XGL_SHADER>::const_iterator q = m_shaders.find(value);
        return (q == m_shaders.end()) ? XGL_NULL_HANDLE : q->second;
    }

    XGL_DYNAMIC_STATE_OBJECT remap(const XGL_DYNAMIC_STATE_OBJECT& state)
    {
        XGL_DYNAMIC_STATE_OBJECT obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_VP_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_RS_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_CB_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_DS_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }
    void rm_from_map(const XGL_DYNAMIC_STATE_OBJECT& state)
    {
        rm_from_map(static_cast <XGL_DYNAMIC_VP_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DYNAMIC_RS_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DYNAMIC_CB_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DYNAMIC_DS_STATE_OBJECT> (state));
    }

    XGL_OBJECT remap(const XGL_OBJECT& object)
    {
        XGL_OBJECT obj;
        if ((obj = remap(static_cast <XGL_BUFFER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_BUFFER_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_IMAGE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_IMAGE_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_COLOR_ATTACHMENT_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DEPTH_STENCIL_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_SHADER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_PIPELINE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_PIPELINE_DELTA> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_SAMPLER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DESCRIPTOR_SET> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DESCRIPTOR_SET_LAYOUT> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DESCRIPTOR_REGION> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_STATE_OBJECT> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_CMD_BUFFER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_FENCE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_QUEUE_SEMAPHORE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_EVENT> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_QUERY_POOL> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_FRAMEBUFFER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_RENDER_PASS> (object))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }
    void rm_from_map(const XGL_OBJECT & objKey)
    {
        rm_from_map(static_cast <XGL_BUFFER> (objKey));
        rm_from_map(static_cast <XGL_BUFFER_VIEW> (objKey));
        rm_from_map(static_cast <XGL_IMAGE> (objKey));
        rm_from_map(static_cast <XGL_IMAGE_VIEW> (objKey));
        rm_from_map(static_cast <XGL_COLOR_ATTACHMENT_VIEW> (objKey));
        rm_from_map(static_cast <XGL_DEPTH_STENCIL_VIEW> (objKey));
        rm_from_map(static_cast <XGL_SHADER> (objKey));
        rm_from_map(static_cast <XGL_PIPELINE> (objKey));
        rm_from_map(static_cast <XGL_PIPELINE_DELTA> (objKey));
        rm_from_map(static_cast <XGL_SAMPLER> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_SET> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_SET_LAYOUT> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_REGION> (objKey));
        rm_from_map(static_cast <XGL_DYNAMIC_STATE_OBJECT> (objKey));
        rm_from_map(static_cast <XGL_CMD_BUFFER> (objKey));
        rm_from_map(static_cast <XGL_FENCE> (objKey));
        rm_from_map(static_cast <XGL_QUEUE_SEMAPHORE> (objKey));
        rm_from_map(static_cast <XGL_EVENT> (objKey));
        rm_from_map(static_cast <XGL_QUERY_POOL> (objKey));
        rm_from_map(static_cast <XGL_FRAMEBUFFER> (objKey));
        rm_from_map(static_cast <XGL_RENDER_PASS> (objKey));
    }
    XGL_BASE_OBJECT remap(const XGL_BASE_OBJECT& object)
    {
        XGL_BASE_OBJECT obj;
        if ((obj = remap(static_cast <XGL_DEVICE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_QUEUE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_GPU_MEMORY> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_OBJECT> (object))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }
void process_screenshot_list(const char *list)
{
    std::string spec(list), word;
    size_t start = 0, comma = 0;

    while (start < spec.size()) {
        comma = spec.find(',', start);

        if (comma == std::string::npos)
            word = std::string(spec, start);
        else
            word = std::string(spec, start, comma - start);

        m_screenshotFrames.push_back(atoi(word.c_str()));
        if (comma == std::string::npos)
            break;

        start = comma + 1;

    }
}
};
