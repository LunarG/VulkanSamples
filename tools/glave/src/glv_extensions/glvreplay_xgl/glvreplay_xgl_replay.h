/**************************************************************************
 *
 * Copyright 2014 Lunarg, Inc.
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

#include <set>
#include <map>
#include <vector>
#include <xcb/xcb.h>

#include "glvreplay_window.h"
#include "glvreplay_factory.h"
#include "glv_trace_packet_identifiers.h"

#include "xgl.h"
#include "xglDbg.h"
#include "xglWsiX11Ext.h"

class ApiReplay {
public:
    virtual ~ApiReplay() { }

    virtual enum glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header * packet) = 0;
    virtual int init(glv_replay::Display & disp) = 0;
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
#if defined(WIN32)
    HWND get_window_handle() { return m_windowHandle; }
#elif defined(PLATFORM_LINUX)
    xcb_window_t get_window_handle() { return m_XcbWindow; }
#endif
private:
    XGL_RESULT init_xgl(const unsigned int gpu_idx);
    bool m_initedXGL;
#if defined(WIN32)
    HWND m_windowHandle;
#elif defined(PLATFORM_LINUX)
    XGL_WSI_X11_CONNECTION_INFO m_WsiConnection;
    xcb_screen_t *m_pXcbScreen;
    xcb_window_t m_XcbWindow;
#endif
    unsigned int m_windowWidth;
    unsigned int m_windowHeight;
#if 0
    XGL_DEVICE m_dev[XGL_MAX_PHYSICAL_GPUS];
    XGL_UINT32 m_gpuCount;
    unsigned int m_gpuIdx;
    XGL_PHYSICAL_GPU m_gpus[XGL_MAX_PHYSICAL_GPUS];
    XGL_PHYSICAL_GPU_PROPERTIES m_gpuProps[XGL_MAX_PHYSICAL_GPUS];
#endif
    std::vector<XGL_CHAR *>m_extensions;
};

typedef struct _XGLAllocInfo {
    XGL_GPU_SIZE size;
    XGL_VOID *pData;
} XGLAllocInfo;

struct xglFuncs {
    void init_funcs(void * libHandle);
    void *m_libHandle;

    typedef XGL_RESULT( XGLAPI * type_xglInitAndEnumerateGpus)(
            const XGL_APPLICATION_INFO* pAppInfo,
            const XGL_ALLOC_CALLBACKS*  pAllocCb,
            XGL_UINT                    maxGpus,
            XGL_UINT*                   pGpuCount,
            XGL_PHYSICAL_GPU*           pGpus);
    type_xglInitAndEnumerateGpus real_xglInitAndEnumerateGpus;
    typedef XGL_RESULT( XGLAPI * type_xglGetGpuInfo)(
            XGL_PHYSICAL_GPU            gpu,
            XGL_PHYSICAL_GPU_INFO_TYPE  infoType,
            XGL_SIZE*                   pDataSize,
            XGL_VOID*                   pData);
    type_xglGetGpuInfo real_xglGetGpuInfo;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDevice)(
            XGL_PHYSICAL_GPU              gpu,
            const XGL_DEVICE_CREATE_INFO* pCreateInfo,
            XGL_DEVICE*                   pDevice);
    type_xglCreateDevice real_xglCreateDevice;
    typedef XGL_RESULT( XGLAPI * type_xglDestroyDevice)(XGL_DEVICE device);
    type_xglDestroyDevice real_xglDestroyDevice;
    typedef XGL_RESULT( XGLAPI * type_xglGetExtensionSupport)(
            XGL_PHYSICAL_GPU gpu,
            const XGL_CHAR*  pExtName);
    type_xglGetExtensionSupport real_xglGetExtensionSupport;
    typedef XGL_RESULT( XGLAPI * type_xglGetDeviceQueue)(
            XGL_DEVICE       device,
            XGL_QUEUE_TYPE   queueType,
            XGL_UINT         queueIndex,
            XGL_QUEUE*       pQueue);
    type_xglGetDeviceQueue real_xglGetDeviceQueue;
    typedef XGL_RESULT( XGLAPI * type_xglQueueSubmit)(
            XGL_QUEUE             queue,
            XGL_UINT              cmdBufferCount,
            const XGL_CMD_BUFFER* pCmdBuffers,
            XGL_UINT              memRefCount,
            const XGL_MEMORY_REF* pMemRefs,
            XGL_FENCE             fence);
    type_xglQueueSubmit real_xglQueueSubmit;
    typedef XGL_RESULT( XGLAPI * type_xglQueueSetGlobalMemReferences)(
            XGL_QUEUE             queue,
            XGL_UINT              memRefCount,
            const XGL_MEMORY_REF* pMemRefs);
    type_xglQueueSetGlobalMemReferences real_xglQueueSetGlobalMemReferences;
    typedef XGL_RESULT( XGLAPI * type_xglQueueWaitIdle)(XGL_QUEUE queue);
    type_xglQueueWaitIdle real_xglQueueWaitIdle;
    typedef XGL_RESULT( XGLAPI * type_xglDeviceWaitIdle)(XGL_DEVICE device);
    type_xglDeviceWaitIdle real_xglDeviceWaitIdle;
    typedef XGL_RESULT( XGLAPI * type_xglGetMemoryHeapCount)(
            XGL_DEVICE  device,
            XGL_UINT*   pCount);
    type_xglGetMemoryHeapCount real_xglGetMemoryHeapCount;
    typedef XGL_RESULT( XGLAPI * type_xglGetMemoryHeapInfo)(
            XGL_DEVICE                  device,
            XGL_UINT                    heapId,
            XGL_MEMORY_HEAP_INFO_TYPE   infoType,
            XGL_SIZE*                   pDataSize,
            XGL_VOID*                   pData);
    type_xglGetMemoryHeapInfo real_xglGetMemoryHeapInfo;
    typedef XGL_RESULT( XGLAPI * type_xglAllocMemory)(
            XGL_DEVICE                   device,
            const XGL_MEMORY_ALLOC_INFO* pAllocInfo,
            XGL_GPU_MEMORY*              pMem);
    type_xglAllocMemory real_xglAllocMemory;
    typedef XGL_RESULT( XGLAPI * type_xglFreeMemory)(XGL_GPU_MEMORY mem);
    type_xglFreeMemory real_xglFreeMemory;
    typedef XGL_RESULT( XGLAPI * type_xglSetMemoryPriority)(
            XGL_GPU_MEMORY            mem,
            XGL_MEMORY_PRIORITY       priority);
    type_xglSetMemoryPriority real_xglSetMemoryPriority;
    typedef XGL_RESULT( XGLAPI * type_xglMapMemory)(
            XGL_GPU_MEMORY mem,
            XGL_FLAGS      flags,                // Reserved
            XGL_VOID**     ppData);
    type_xglMapMemory real_xglMapMemory;
    typedef XGL_RESULT( XGLAPI * type_xglUnmapMemory)(XGL_GPU_MEMORY mem);
    type_xglUnmapMemory real_xglUnmapMemory;
    typedef XGL_RESULT( XGLAPI * type_xglPinSystemMemory)(
            XGL_DEVICE      device,
            const XGL_VOID* pSysMem,
            XGL_SIZE        memSize,
            XGL_GPU_MEMORY* pMem);
    type_xglPinSystemMemory real_xglPinSystemMemory;
    typedef XGL_RESULT( XGLAPI * type_xglRemapVirtualMemoryPages)(
            XGL_DEVICE                            device,
            XGL_UINT                              rangeCount,
            const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pRanges,
            XGL_UINT                              preWaitSemaphoreCount,
            const XGL_QUEUE_SEMAPHORE*            pPreWaitSemaphores,
            XGL_UINT                              postSignalSemaphoreCount,
            const XGL_QUEUE_SEMAPHORE*            pPostSignalSemaphores);
    type_xglRemapVirtualMemoryPages real_xglRemapVirtualMemoryPages;
    typedef XGL_RESULT( XGLAPI * type_xglGetMultiGpuCompatibility)(
            XGL_PHYSICAL_GPU            gpu0,
            XGL_PHYSICAL_GPU            gpu1,
            XGL_GPU_COMPATIBILITY_INFO* pInfo);
    type_xglGetMultiGpuCompatibility real_xglGetMultiGpuCompatibility;
    typedef XGL_RESULT( XGLAPI * type_xglOpenSharedMemory)(
            XGL_DEVICE                  device,
            const XGL_MEMORY_OPEN_INFO* pOpenInfo,
            XGL_GPU_MEMORY*             pMem);
    type_xglOpenSharedMemory real_xglOpenSharedMemory;
    typedef XGL_RESULT( XGLAPI * type_xglOpenSharedQueueSemaphore)(
            XGL_DEVICE                           device,
            const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo,
            XGL_QUEUE_SEMAPHORE*                 pSemaphore);
    type_xglOpenSharedQueueSemaphore real_xglOpenSharedQueueSemaphore;
    typedef XGL_RESULT( XGLAPI * type_xglOpenPeerMemory)(
            XGL_DEVICE                       device,
            const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo,
            XGL_GPU_MEMORY*                  pMem);
    type_xglOpenPeerMemory real_xglOpenPeerMemory;
    typedef XGL_RESULT( XGLAPI * type_xglOpenPeerImage)(
            XGL_DEVICE                      device,
            const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo,
            XGL_IMAGE*                      pImage,
            XGL_GPU_MEMORY*                 pMem);
    type_xglOpenPeerImage real_xglOpenPeerImage;
    typedef XGL_RESULT( XGLAPI * type_xglDestroyObject)(XGL_OBJECT object);
    type_xglDestroyObject real_xglDestroyObject;
    typedef XGL_RESULT( XGLAPI * type_xglGetObjectInfo)(
            XGL_BASE_OBJECT             object,
            XGL_OBJECT_INFO_TYPE        infoType,
            XGL_SIZE*                   pDataSize,
            XGL_VOID*                   pData);
    type_xglGetObjectInfo real_xglGetObjectInfo;
    typedef XGL_RESULT( XGLAPI * type_xglBindObjectMemory)(
            XGL_OBJECT     object,
            XGL_GPU_MEMORY mem,
            XGL_GPU_SIZE   offset);
    type_xglBindObjectMemory real_xglBindObjectMemory;
    typedef XGL_RESULT( XGLAPI * type_xglCreateFence)(
            XGL_DEVICE                   device,
            const XGL_FENCE_CREATE_INFO* pCreateInfo,
            XGL_FENCE*                   pFence);
    type_xglCreateFence real_xglCreateFence;
    typedef XGL_RESULT( XGLAPI * type_xglGetFenceStatus)(XGL_FENCE fence);
    type_xglGetFenceStatus real_xglGetFenceStatus;
    typedef XGL_RESULT( XGLAPI * type_xglWaitForFences)(
            XGL_DEVICE       device,
            XGL_UINT         fenceCount,
            const XGL_FENCE* pFences,
            XGL_BOOL         waitAll,
            XGL_UINT64       timeout);
    type_xglWaitForFences real_xglWaitForFences;
    typedef XGL_RESULT( XGLAPI * type_xglCreateQueueSemaphore)(
            XGL_DEVICE                             device,
            const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo,
            XGL_QUEUE_SEMAPHORE*                   pSemaphore);
    type_xglCreateQueueSemaphore real_xglCreateQueueSemaphore;
    typedef XGL_RESULT( XGLAPI * type_xglSignalQueueSemaphore)(
            XGL_QUEUE           queue,
            XGL_QUEUE_SEMAPHORE semaphore);
    type_xglSignalQueueSemaphore real_xglSignalQueueSemaphore;
    typedef XGL_RESULT( XGLAPI * type_xglWaitQueueSemaphore)(
            XGL_QUEUE           queue,
            XGL_QUEUE_SEMAPHORE semaphore);
    type_xglWaitQueueSemaphore real_xglWaitQueueSemaphore;
    typedef XGL_RESULT( XGLAPI * type_xglCreateEvent)(
            XGL_DEVICE                   device,
            const XGL_EVENT_CREATE_INFO* pCreateInfo,
            XGL_EVENT*                   pEvent);
    type_xglCreateEvent real_xglCreateEvent;
    typedef XGL_RESULT( XGLAPI * type_xglGetEventStatus)(XGL_EVENT event);
    type_xglGetEventStatus real_xglGetEventStatus;
    typedef XGL_RESULT( XGLAPI * type_xglSetEvent)(XGL_EVENT event);
    type_xglSetEvent real_xglSetEvent;
    typedef XGL_RESULT( XGLAPI * type_xglResetEvent)(XGL_EVENT event);
    type_xglResetEvent real_xglResetEvent;
    typedef XGL_RESULT( XGLAPI * type_xglCreateQueryPool)(
            XGL_DEVICE                        device,
            const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo,
            XGL_QUERY_POOL*                   pQueryPool);
    type_xglCreateQueryPool real_xglCreateQueryPool;
    typedef XGL_RESULT( XGLAPI * type_xglGetQueryPoolResults)(
            XGL_QUERY_POOL queryPool,
            XGL_UINT       startQuery,
            XGL_UINT       queryCount,
            XGL_SIZE*      pDataSize,
            XGL_VOID*      pData);
    type_xglGetQueryPoolResults real_xglGetQueryPoolResults;
    typedef XGL_RESULT( XGLAPI * type_xglGetFormatInfo)(
            XGL_DEVICE             device,
            XGL_FORMAT             format,
            XGL_FORMAT_INFO_TYPE   infoType,
            XGL_SIZE*              pDataSize,
            XGL_VOID*              pData);
    type_xglGetFormatInfo real_xglGetFormatInfo;
    typedef XGL_RESULT( XGLAPI * type_xglCreateImage)(
            XGL_DEVICE                   device,
            const XGL_IMAGE_CREATE_INFO* pCreateInfo,
            XGL_IMAGE*                   pImage);
    type_xglCreateImage real_xglCreateImage;
    typedef XGL_RESULT( XGLAPI * type_xglGetImageSubresourceInfo)(
            XGL_IMAGE                    image,
            const XGL_IMAGE_SUBRESOURCE* pSubresource,
            XGL_SUBRESOURCE_INFO_TYPE    infoType,
            XGL_SIZE*                    pDataSize,
            XGL_VOID*                    pData);
    type_xglGetImageSubresourceInfo real_xglGetImageSubresourceInfo;
    typedef XGL_RESULT( XGLAPI * type_xglCreateImageView)(
            XGL_DEVICE                        device,
            const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
            XGL_IMAGE_VIEW*                   pView);
    type_xglCreateImageView real_xglCreateImageView;
    typedef XGL_RESULT( XGLAPI * type_xglCreateColorAttachmentView)(
            XGL_DEVICE                                   device,
            const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
            XGL_COLOR_ATTACHMENT_VIEW*                   pView);
    type_xglCreateColorAttachmentView real_xglCreateColorAttachmentView;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDepthStencilView)(
            XGL_DEVICE                                device,
            const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo,
            XGL_DEPTH_STENCIL_VIEW*                   pView);
    type_xglCreateDepthStencilView real_xglCreateDepthStencilView;
    typedef XGL_RESULT( XGLAPI * type_xglCreateShader)(
            XGL_DEVICE                    device,
            const XGL_SHADER_CREATE_INFO* pCreateInfo,
            XGL_SHADER*                   pShader);
    type_xglCreateShader real_xglCreateShader;
    typedef XGL_RESULT( XGLAPI * type_xglCreateGraphicsPipeline)(
            XGL_DEVICE                               device,
            const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
            XGL_PIPELINE*                            pPipeline);
    type_xglCreateGraphicsPipeline real_xglCreateGraphicsPipeline;
    typedef XGL_RESULT( XGLAPI * type_xglCreateComputePipeline)(
            XGL_DEVICE                              device,
            const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo,
            XGL_PIPELINE*                           pPipeline);
    type_xglCreateComputePipeline real_xglCreateComputePipeline;
    typedef XGL_RESULT( XGLAPI * type_xglStorePipeline)(
            XGL_PIPELINE pipeline,
            XGL_SIZE*    pDataSize,
            XGL_VOID*    pData);
    type_xglStorePipeline real_xglStorePipeline;
    typedef XGL_RESULT( XGLAPI * type_xglLoadPipeline)(
            XGL_DEVICE      device,
            XGL_SIZE        dataSize,
            const XGL_VOID* pData,
            XGL_PIPELINE*   pPipeline);
    type_xglLoadPipeline real_xglLoadPipeline;
    typedef XGL_RESULT( XGLAPI * type_xglCreatePipelineDelta)(
            XGL_DEVICE          device,
	    XGL_PIPELINE        p1,
	    XGL_PIPELINE        p2,
	    XGL_PIPELINE_DELTA* delta);
    type_xglCreatePipelineDelta real_xglCreatePipelineDelta;
    typedef XGL_RESULT( XGLAPI * type_xglCreateSampler)(
            XGL_DEVICE                     device,
            const XGL_SAMPLER_CREATE_INFO* pCreateInfo,
            XGL_SAMPLER*                   pSampler);
    type_xglCreateSampler real_xglCreateSampler;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDescriptorSet)(
            XGL_DEVICE                            device,
            const XGL_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo,
            XGL_DESCRIPTOR_SET*                   pDescriptorSet);
    type_xglCreateDescriptorSet real_xglCreateDescriptorSet;
    typedef XGL_VOID( XGLAPI * type_xglBeginDescriptorSetUpdate)(XGL_DESCRIPTOR_SET descriptorSet);
    type_xglBeginDescriptorSetUpdate real_xglBeginDescriptorSetUpdate;
    typedef XGL_VOID( XGLAPI * type_xglEndDescriptorSetUpdate)(XGL_DESCRIPTOR_SET descriptorSet);
    type_xglEndDescriptorSetUpdate real_xglEndDescriptorSetUpdate;
    typedef XGL_VOID( XGLAPI * type_xglAttachSamplerDescriptors)(
            XGL_DESCRIPTOR_SET descriptorSet,
            XGL_UINT           startSlot,
            XGL_UINT           slotCount,
            const XGL_SAMPLER* pSamplers);
    type_xglAttachSamplerDescriptors real_xglAttachSamplerDescriptors;
    typedef XGL_VOID( XGLAPI * type_xglAttachImageViewDescriptors)(
            XGL_DESCRIPTOR_SET                descriptorSet,
            XGL_UINT                          startSlot,
            XGL_UINT                          slotCount,
            const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews);
    type_xglAttachImageViewDescriptors real_xglAttachImageViewDescriptors;
    typedef XGL_VOID( XGLAPI * type_xglAttachMemoryViewDescriptors)(
            XGL_DESCRIPTOR_SET                 descriptorSet,
            XGL_UINT                           startSlot,
            XGL_UINT                           slotCount,
            const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews);
    type_xglAttachMemoryViewDescriptors real_xglAttachMemoryViewDescriptors;
    typedef XGL_VOID( XGLAPI * type_xglAttachNestedDescriptors)(
            XGL_DESCRIPTOR_SET                    descriptorSet,
            XGL_UINT                              startSlot,
            XGL_UINT                              slotCount,
            const XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets);
    type_xglAttachNestedDescriptors real_xglAttachNestedDescriptors;
    typedef XGL_VOID( XGLAPI * type_xglClearDescriptorSetSlots)(
            XGL_DESCRIPTOR_SET descriptorSet,
            XGL_UINT           startSlot,
            XGL_UINT           slotCount);
    type_xglClearDescriptorSetSlots real_xglClearDescriptorSetSlots;
    typedef XGL_RESULT( XGLAPI * type_xglCreateViewportState)(
            XGL_DEVICE                            device,
            const XGL_VIEWPORT_STATE_CREATE_INFO* pCreateInfo,
            XGL_VIEWPORT_STATE_OBJECT*            pState);
    type_xglCreateViewportState real_xglCreateViewportState;
    typedef XGL_RESULT( XGLAPI * type_xglCreateRasterState)(
            XGL_DEVICE                          device,
            const XGL_RASTER_STATE_CREATE_INFO* pCreateInfo,
            XGL_RASTER_STATE_OBJECT*            pState);
    type_xglCreateRasterState real_xglCreateRasterState;
    typedef XGL_RESULT( XGLAPI * type_xglCreateMsaaState)(
            XGL_DEVICE                        device,
            const XGL_MSAA_STATE_CREATE_INFO* pCreateInfo,
            XGL_MSAA_STATE_OBJECT*            pState);
    type_xglCreateMsaaState real_xglCreateMsaaState;
    typedef XGL_RESULT( XGLAPI * type_xglCreateColorBlendState)(
            XGL_DEVICE                               device,
            const XGL_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo,
            XGL_COLOR_BLEND_STATE_OBJECT*            pState);
    type_xglCreateColorBlendState real_xglCreateColorBlendState;
    typedef XGL_RESULT( XGLAPI * type_xglCreateDepthStencilState)(
            XGL_DEVICE                                 device,
            const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo,
            XGL_DEPTH_STENCIL_STATE_OBJECT*            pState);
    type_xglCreateDepthStencilState real_xglCreateDepthStencilState;
    typedef XGL_RESULT( XGLAPI * type_xglCreateCommandBuffer)(
            XGL_DEVICE                        device,
            const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo,
            XGL_CMD_BUFFER*                   pCmdBuffer);
    type_xglCreateCommandBuffer real_xglCreateCommandBuffer;
    typedef XGL_RESULT( XGLAPI * type_xglBeginCommandBuffer)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_FLAGS      flags);
    type_xglBeginCommandBuffer real_xglBeginCommandBuffer;
    typedef XGL_RESULT( XGLAPI * type_xglEndCommandBuffer)(XGL_CMD_BUFFER cmdBuffer);
    type_xglEndCommandBuffer real_xglEndCommandBuffer;
    typedef XGL_RESULT( XGLAPI * type_xglResetCommandBuffer)(XGL_CMD_BUFFER cmdBuffer);
    type_xglResetCommandBuffer real_xglResetCommandBuffer;
    typedef XGL_VOID( XGLAPI * type_xglCmdBindPipeline)(
            XGL_CMD_BUFFER                cmdBuffer,
            XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
            XGL_PIPELINE                  pipeline);
    type_xglCmdBindPipeline real_xglCmdBindPipeline;
    typedef XGL_VOID( XGLAPI * type_xglCmdBindPipelineDelta)(
            XGL_CMD_BUFFER                cmdBuffer,
            XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
            XGL_PIPELINE_DELTA            delta);
    type_xglCmdBindPipelineDelta real_xglCmdBindPipelineDelta;
    typedef XGL_VOID( XGLAPI * type_xglCmdBindStateObject)(
            XGL_CMD_BUFFER               cmdBuffer,
            XGL_STATE_BIND_POINT         stateBindPoint,
            XGL_STATE_OBJECT             state);
    type_xglCmdBindStateObject real_xglCmdBindStateObject;
    typedef XGL_VOID( XGLAPI * type_xglCmdBindDescriptorSet)(
            XGL_CMD_BUFFER                    cmdBuffer,
            XGL_PIPELINE_BIND_POINT           pipelineBindPoint,
            XGL_UINT                          index,
            XGL_DESCRIPTOR_SET                descriptorSet,
            XGL_UINT                          slotOffset);
    type_xglCmdBindDescriptorSet real_xglCmdBindDescriptorSet;
    typedef XGL_VOID( XGLAPI * type_xglCmdBindDynamicMemoryView)(
            XGL_CMD_BUFFER                     cmdBuffer,
            XGL_PIPELINE_BIND_POINT            pipelineBindPoint,
            const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView);
    type_xglCmdBindDynamicMemoryView real_xglCmdBindDynamicMemoryView;
    typedef XGL_VOID( XGLAPI * type_xglCmdBindIndexData)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_GPU_MEMORY mem,
            XGL_GPU_SIZE   offset,
            XGL_INDEX_TYPE indexType);
    type_xglCmdBindIndexData real_xglCmdBindIndexData;
    typedef XGL_VOID( XGLAPI * type_xglCmdBindVertexData)(
            XGL_CMD_BUFFER  cmdBuffer,
            XGL_GPU_MEMORY  mem,
            XGL_GPU_SIZE    offset,
            XGL_UINT        binding);
    type_xglCmdBindVertexData real_xglCmdBindVertexData;
    typedef XGL_VOID( XGLAPI * type_xglCmdBindAttachments)(
            XGL_CMD_BUFFER                         cmdBuffer,
            XGL_UINT                               colorTargetCount,
            const XGL_COLOR_ATTACHMENT_BIND_INFO*  pColorTargets,
            const XGL_DEPTH_STENCIL_BIND_INFO*     pDepthTarget);
    type_xglCmdBindAttachments real_xglCmdBindAttachments;
    typedef XGL_VOID( XGLAPI * type_xglCmdPrepareMemoryRegions)(
            XGL_CMD_BUFFER                     cmdBuffer,
            XGL_UINT                           transitionCount,
            const XGL_MEMORY_STATE_TRANSITION* pStateTransitions);
    type_xglCmdPrepareMemoryRegions real_xglCmdPrepareMemoryRegions;
    typedef XGL_VOID( XGLAPI * type_xglCmdPrepareImages)(
            XGL_CMD_BUFFER                    cmdBuffer,
            XGL_UINT                          transitionCount,
            const XGL_IMAGE_STATE_TRANSITION* pStateTransitions);
    type_xglCmdPrepareImages real_xglCmdPrepareImages;
    typedef XGL_VOID( XGLAPI * type_xglCmdDraw)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_UINT       firstVertex,
            XGL_UINT       vertexCount,
            XGL_UINT       firstInstance,
            XGL_UINT       instanceCount);
    type_xglCmdDraw real_xglCmdDraw;
    typedef XGL_VOID( XGLAPI * type_xglCmdDrawIndexed)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_UINT       firstIndex,
            XGL_UINT       indexCount,
            XGL_INT        vertexOffset,
            XGL_UINT       firstInstance,
            XGL_UINT       instanceCount);
    type_xglCmdDrawIndexed real_xglCmdDrawIndexed;
    typedef XGL_VOID( XGLAPI * type_xglCmdDrawIndirect)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_GPU_MEMORY mem,
            XGL_GPU_SIZE   offset,
            XGL_UINT32     count,
            XGL_UINT32     stride);
    type_xglCmdDrawIndirect real_xglCmdDrawIndirect;
    typedef XGL_VOID( XGLAPI * type_xglCmdDrawIndexedIndirect)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_GPU_MEMORY mem,
            XGL_GPU_SIZE   offset,
            XGL_UINT32     count,
            XGL_UINT32     stride);
    type_xglCmdDrawIndexedIndirect real_xglCmdDrawIndexedIndirect;
    typedef XGL_VOID( XGLAPI * type_xglCmdDispatch)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_UINT       x,
            XGL_UINT       y,
            XGL_UINT       z);
    type_xglCmdDispatch real_xglCmdDispatch;
    typedef XGL_VOID( XGLAPI * type_xglCmdDispatchIndirect)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_GPU_MEMORY mem,
            XGL_GPU_SIZE   offset);
    type_xglCmdDispatchIndirect real_xglCmdDispatchIndirect;
    typedef XGL_VOID( XGLAPI * type_xglCmdCopyMemory)(
            XGL_CMD_BUFFER         cmdBuffer,
            XGL_GPU_MEMORY         srcMem,
            XGL_GPU_MEMORY         destMem,
            XGL_UINT               regionCount,
            const XGL_MEMORY_COPY* pRegions);
    type_xglCmdCopyMemory real_xglCmdCopyMemory;
    typedef XGL_VOID( XGLAPI * type_xglCmdCopyImage)(
            XGL_CMD_BUFFER        cmdBuffer,
            XGL_IMAGE             srcImage,
            XGL_IMAGE             destImage,
            XGL_UINT              regionCount,
            const XGL_IMAGE_COPY* pRegions);
    type_xglCmdCopyImage real_xglCmdCopyImage;
    typedef XGL_VOID( XGLAPI * type_xglCmdCopyMemoryToImage)(
            XGL_CMD_BUFFER               cmdBuffer,
            XGL_GPU_MEMORY               srcMem,
            XGL_IMAGE                    destImage,
            XGL_UINT                     regionCount,
            const XGL_MEMORY_IMAGE_COPY* pRegions);
    type_xglCmdCopyMemoryToImage real_xglCmdCopyMemoryToImage;
    typedef XGL_VOID( XGLAPI * type_xglCmdCopyImageToMemory)(
            XGL_CMD_BUFFER               cmdBuffer,
            XGL_IMAGE                    srcImage,
            XGL_GPU_MEMORY               destMem,
            XGL_UINT                     regionCount,
            const XGL_MEMORY_IMAGE_COPY* pRegions);
    type_xglCmdCopyImageToMemory real_xglCmdCopyImageToMemory;
    typedef XGL_VOID( XGLAPI * type_xglCmdCloneImageData)(
            XGL_CMD_BUFFER  cmdBuffer,
            XGL_IMAGE       srcImage,
            XGL_IMAGE_STATE srcImageState,
            XGL_IMAGE       destImage,
            XGL_IMAGE_STATE destImageState);
    type_xglCmdCloneImageData real_xglCmdCloneImageData;
    typedef XGL_VOID( XGLAPI * type_xglCmdUpdateMemory)(
            XGL_CMD_BUFFER    cmdBuffer,
            XGL_GPU_MEMORY    destMem,
            XGL_GPU_SIZE      destOffset,
            XGL_GPU_SIZE      dataSize,
            const XGL_UINT32* pData);
    type_xglCmdUpdateMemory real_xglCmdUpdateMemory;
    typedef XGL_VOID( XGLAPI * type_xglCmdFillMemory)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_GPU_MEMORY destMem,
            XGL_GPU_SIZE   destOffset,
            XGL_GPU_SIZE   fillSize,
            XGL_UINT32     data);
    type_xglCmdFillMemory real_xglCmdFillMemory;
    typedef XGL_VOID( XGLAPI * type_xglCmdClearColorImage)(
            XGL_CMD_BUFFER                     cmdBuffer,
            XGL_IMAGE                          image,
            const XGL_FLOAT                    color[4],
            XGL_UINT                           rangeCount,
            const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
    type_xglCmdClearColorImage real_xglCmdClearColorImage;
    typedef XGL_VOID( XGLAPI * type_xglCmdClearColorImageRaw)(
            XGL_CMD_BUFFER                     cmdBuffer,
            XGL_IMAGE                          image,
            const XGL_UINT32                   color[4],
            XGL_UINT                           rangeCount,
            const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
    type_xglCmdClearColorImageRaw real_xglCmdClearColorImageRaw;
    typedef XGL_VOID( XGLAPI * type_xglCmdClearDepthStencil)(
            XGL_CMD_BUFFER                     cmdBuffer,
            XGL_IMAGE                          image,
            XGL_FLOAT                          depth,
            XGL_UINT32                         stencil,
            XGL_UINT                           rangeCount,
            const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
    type_xglCmdClearDepthStencil real_xglCmdClearDepthStencil;
    typedef XGL_VOID( XGLAPI * type_xglCmdResolveImage)(
            XGL_CMD_BUFFER           cmdBuffer,
            XGL_IMAGE                srcImage,
            XGL_IMAGE                destImage,
            XGL_UINT                 rectCount,
            const XGL_IMAGE_RESOLVE* pRects);
    type_xglCmdResolveImage real_xglCmdResolveImage;
    typedef XGL_VOID( XGLAPI * type_xglCmdSetEvent)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_EVENT      event);
    type_xglCmdSetEvent real_xglCmdSetEvent;
    typedef XGL_VOID( XGLAPI * type_xglCmdResetEvent)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_EVENT      event);
    type_xglCmdResetEvent real_xglCmdResetEvent;
    typedef XGL_VOID( XGLAPI * type_xglCmdMemoryAtomic)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_GPU_MEMORY destMem,
            XGL_GPU_SIZE   destOffset,
            XGL_UINT64     srcData,
            XGL_ATOMIC_OP  atomicOp);
    type_xglCmdMemoryAtomic real_xglCmdMemoryAtomic;
    typedef XGL_VOID( XGLAPI * type_xglCmdBeginQuery)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_QUERY_POOL queryPool,
            XGL_UINT       slot,
            XGL_FLAGS      flags);
    type_xglCmdBeginQuery real_xglCmdBeginQuery;
    typedef XGL_VOID( XGLAPI * type_xglCmdEndQuery)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_QUERY_POOL queryPool,
            XGL_UINT       slot);
    type_xglCmdEndQuery real_xglCmdEndQuery;
    typedef XGL_VOID( XGLAPI * type_xglCmdResetQueryPool)(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_QUERY_POOL queryPool,
            XGL_UINT       startQuery,
            XGL_UINT       queryCount);
    type_xglCmdResetQueryPool real_xglCmdResetQueryPool;
    typedef XGL_VOID( XGLAPI * type_xglCmdWriteTimestamp)(
            XGL_CMD_BUFFER           cmdBuffer,
            XGL_TIMESTAMP_TYPE       timestampType,
            XGL_GPU_MEMORY           destMem,
            XGL_GPU_SIZE             destOffset);
    type_xglCmdWriteTimestamp real_xglCmdWriteTimestamp;
    typedef XGL_VOID( XGLAPI * type_xglCmdInitAtomicCounters)(
            XGL_CMD_BUFFER                   cmdBuffer,
            XGL_PIPELINE_BIND_POINT          pipelineBindPoint,
            XGL_UINT                         startCounter,
            XGL_UINT                         counterCount,
            const XGL_UINT32*                pData);
    type_xglCmdInitAtomicCounters real_xglCmdInitAtomicCounters;
    typedef XGL_VOID( XGLAPI * type_xglCmdLoadAtomicCounters)(
            XGL_CMD_BUFFER                cmdBuffer,
            XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
            XGL_UINT                      startCounter,
            XGL_UINT                      counterCount,
            XGL_GPU_MEMORY                srcMem,
            XGL_GPU_SIZE                  srcOffset);
    type_xglCmdLoadAtomicCounters real_xglCmdLoadAtomicCounters;
    typedef XGL_VOID( XGLAPI * type_xglCmdSaveAtomicCounters)(
            XGL_CMD_BUFFER                cmdBuffer,
            XGL_PIPELINE_BIND_POINT       pipelineBindPoint,
            XGL_UINT                      startCounter,
            XGL_UINT                      counterCount,
            XGL_GPU_MEMORY                destMem,
            XGL_GPU_SIZE                  destOffset);
    type_xglCmdSaveAtomicCounters real_xglCmdSaveAtomicCounters;

    // Debug entrypoints
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetValidationLevel)(
            XGL_DEVICE             device,
            XGL_VALIDATION_LEVEL   validationLevel);
    type_xglDbgSetValidationLevel real_xglDbgSetValidationLevel;
    typedef XGL_RESULT( XGLAPI * type_xglDbgRegisterMsgCallback)(
            XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback,
            XGL_VOID*                     pUserData);
    type_xglDbgRegisterMsgCallback real_xglDbgRegisterMsgCallback;
    typedef XGL_RESULT( XGLAPI * type_xglDbgUnregisterMsgCallback)(
            XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);
    type_xglDbgUnregisterMsgCallback real_xglDbgUnregisterMsgCallback;
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetMessageFilter)(
            XGL_DEVICE           device,
            XGL_INT              msgCode,
            XGL_DBG_MSG_FILTER   filter);
    type_xglDbgSetMessageFilter real_xglDbgSetMessageFilter;
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetObjectTag)(
            XGL_BASE_OBJECT object,
            XGL_SIZE        tagSize,
            const XGL_VOID* pTag);
    type_xglDbgSetObjectTag real_xglDbgSetObjectTag;
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetGlobalOption)(
            XGL_DBG_GLOBAL_OPTION        dbgOption,
            XGL_SIZE                     dataSize,
            const XGL_VOID*              pData);
    type_xglDbgSetGlobalOption real_xglDbgSetGlobalOption;
    typedef XGL_RESULT( XGLAPI * type_xglDbgSetDeviceOption)(
            XGL_DEVICE                   device,
            XGL_DBG_DEVICE_OPTION        dbgOption,
            XGL_SIZE                     dataSize,
            const XGL_VOID*              pData);
    type_xglDbgSetDeviceOption real_xglDbgSetDeviceOption;
    typedef XGL_VOID( XGLAPI * type_xglCmdDbgMarkerBegin)(
            XGL_CMD_BUFFER  cmdBuffer,
            const XGL_CHAR* pMarker);
    type_xglCmdDbgMarkerBegin real_xglCmdDbgMarkerBegin;
    typedef XGL_VOID( XGLAPI * type_xglCmdDbgMarkerEnd)(
            XGL_CMD_BUFFER  cmdBuffer);
    type_xglCmdDbgMarkerEnd real_xglCmdDbgMarkerEnd;

    //WsiX11Ext entrypoints
    typedef XGL_RESULT (XGLAPI * type_xglWsiX11AssociateConnection)(
            XGL_PHYSICAL_GPU                            gpu,
            const XGL_WSI_X11_CONNECTION_INFO*          pConnectionInfo);
    type_xglWsiX11AssociateConnection real_xglWsiX11AssociateConnection;
    typedef XGL_RESULT (XGLAPI * type_xglWsiX11GetMSC)(
            XGL_DEVICE                                  device,
            xcb_window_t                                window,
            xcb_randr_crtc_t                            crtc,
            XGL_UINT64*                                 pMsc);
    type_xglWsiX11GetMSC real_xglWsiX11GetMSC;
    typedef XGL_RESULT (XGLAPI * type_xglWsiX11CreatePresentableImage)(
            XGL_DEVICE                                  device,
            const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
            XGL_IMAGE*                                  pImage,
            XGL_GPU_MEMORY*                             pMem);
    type_xglWsiX11CreatePresentableImage real_xglWsiX11CreatePresentableImage;
    typedef XGL_RESULT (XGLAPI * type_xglWsiX11QueuePresent)(
            XGL_QUEUE                                   queue,
            const XGL_WSI_X11_PRESENT_INFO*             pPresentInfo,
            XGL_FENCE                                   fence);
    type_xglWsiX11QueuePresent real_xglWsiX11QueuePresent;
};

class xglReplay : public ApiReplay {
public:
    ~xglReplay();
    xglReplay(unsigned int debugLevel);

    int init(glv_replay::Display & disp);
    xglDisplay * get_display() {return m_display;}
    glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header *packet);
    glv_replay::GLV_REPLAY_RESULT handle_replay_errors(const char* entrypointName, const XGL_RESULT resCall, const XGL_RESULT resTrace, const glv_replay::GLV_REPLAY_RESULT resIn);

private:
    struct xglFuncs m_xglFuncs;
    void copy_mem_remap_range_struct(XGL_VIRTUAL_MEMORY_REMAP_RANGE *outRange, const XGL_VIRTUAL_MEMORY_REMAP_RANGE *inRange);
    unsigned int m_debugLevel;
    xglDisplay *m_display;
    XGL_MEMORY_HEAP_PROPERTIES m_heapProps[XGL_MAX_MEMORY_HEAPS];

    std::map<XGL_GPU_MEMORY, XGLAllocInfo> m_mapData;
    void add_entry_to_mapData(XGL_GPU_MEMORY handle, XGL_GPU_SIZE size)
    {
        XGLAllocInfo info;
        info.pData = NULL;
        info.size = size;
        m_mapData.insert(std::pair<XGL_GPU_MEMORY, XGLAllocInfo>(handle, info));
    }
    void add_mapping_to_mapData(XGL_GPU_MEMORY handle, XGL_VOID *pData)
    {
        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);
        if (it == m_mapData.end())
        {
            glv_LogWarn("add_mapping_to_mapData() couldn't find entry\n");
            return;
        }

        XGLAllocInfo &info = it->second;
        if (info.pData != NULL)
        {
            glv_LogWarn("add_mapping_to_mapData() data already mapped overwrite old mapping\n");
        }
        info.pData = pData;
    }
    void rm_entry_from_mapData(XGL_GPU_MEMORY handle)
    {
        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);
        if (it == m_mapData.end())
            return;
        m_mapData.erase(it);
    }
    void rm_mapping_from_mapData(XGL_GPU_MEMORY handle, XGL_VOID* pData)
    {
        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);

        if (it == m_mapData.end())
            return;

        XGLAllocInfo &info = it->second;

        if (!pData || !info.pData)
        {
            glv_LogWarn("rm_mapping_from_mapData() null src or dest pointers\n");
            info.pData = NULL;
            return;
        }
        memcpy(info.pData, pData, info.size);
        info.pData = NULL;
    }

    std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU> m_gpus;
    void add_to_map(XGL_PHYSICAL_GPU* pTraceGpu, XGL_PHYSICAL_GPU* pReplayGpu)
    {
        assert(pTraceGpu != NULL);
        assert(pReplayGpu != NULL);
        m_gpus[*pTraceGpu] = *pReplayGpu;
    }

    XGL_PHYSICAL_GPU remap(const XGL_PHYSICAL_GPU& gpu)
    {
        std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU>::const_iterator q = m_gpus.find(gpu);
        return (q == m_gpus.end()) ? XGL_NULL_HANDLE : q->second;
    }

    void clear_all_map_handles()
    {
        m_gpus.clear();
        m_devices.clear();
        m_queues.clear();
        m_memories.clear();
        m_images.clear();
        m_imageViews.clear();
        m_colorTargetViews.clear();
        m_depthStencilViews.clear();
        m_shader.clear();
        m_pipeline.clear();
        m_pipelineDelta.clear();
        m_sampler.clear();
        m_descriptorSets.clear();
        m_viewportStates.clear();
        m_rasterStates.clear();
        m_msaaStates.clear();
        m_colorBlendStates.clear();
        m_depthStencilStates.clear();
        m_cmdBuffers.clear();
        m_fences.clear();
        m_queue_semaphores.clear();
        m_events.clear();
        m_queryPools.clear();
        m_presentableImageSizes.clear();
    }

    std::map<XGL_DEVICE, XGL_DEVICE> m_devices;
    void add_to_map(XGL_DEVICE* pTraceDevice, XGL_DEVICE* pReplayDevice)
    {
        assert(pTraceDevice != NULL);
        assert(pReplayDevice != NULL);
        m_devices[*pTraceDevice] = *pReplayDevice;
    }
    void rm_from_map(const XGL_DEVICE& deviceKey)
    {
        m_devices.erase(deviceKey);
    }
    XGL_DEVICE remap(const XGL_DEVICE& device)
    {
        std::map<XGL_DEVICE, XGL_DEVICE>::const_iterator q = m_devices.find(device);
        return (q == m_devices.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_QUEUE, XGL_QUEUE> m_queues;
    void add_to_map(XGL_QUEUE* pTraceQueue, XGL_QUEUE* pReplayQueue)
    {
        assert(pTraceQueue != NULL);
        assert(pReplayQueue != NULL);
        m_queues[*pTraceQueue] = *pReplayQueue;
    }
    // TODO how are queues individually removed from map???
    XGL_QUEUE remap(const XGL_QUEUE& queue)
    {
        std::map<XGL_QUEUE, XGL_QUEUE>::const_iterator q = m_queues.find(queue);
        return (q == m_queues.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_GPU_MEMORY, XGL_GPU_MEMORY> m_memories;
    void add_to_map(XGL_GPU_MEMORY* pTraceMemory, XGL_GPU_MEMORY* pReplayMemory)
    {
        assert(pTraceMemory != NULL);
        assert(pReplayMemory != NULL);
        m_memories[*pTraceMemory] = *pReplayMemory;
    }
    void rm_from_map(const XGL_GPU_MEMORY &gpuMemKey)
    {
        m_memories.erase(gpuMemKey);
    }
    XGL_GPU_MEMORY remap(const XGL_GPU_MEMORY& memory)
    {
        std::map<XGL_GPU_MEMORY, XGL_GPU_MEMORY>::const_iterator m = m_memories.find(memory);
        return (m == m_memories.end()) ? XGL_NULL_HANDLE : m->second;
    }

    std::map<XGL_IMAGE, XGL_IMAGE> m_images;
    void add_to_map(XGL_IMAGE* pTraceImage, XGL_IMAGE* pReplayImage)
    {
        assert(pTraceImage != NULL);
        assert(pReplayImage != NULL);
        m_images[*pTraceImage] = *pReplayImage;
    }
    void rm_from_map(const XGL_IMAGE& image)
    {
        m_images.erase(image);
    }
    XGL_IMAGE remap(const XGL_IMAGE& image)
    {
        std::map<XGL_IMAGE, XGL_IMAGE>::const_iterator q = m_images.find(image);
        return (q == m_images.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_IMAGE, XGL_EXTENT2D> m_presentableImageSizes;
    void add_presentable_image_extents(const XGL_IMAGE* image, const XGL_EXTENT2D* extent)
    {
        assert(image != NULL);
        assert(extent != NULL);
        m_presentableImageSizes[*image] = *extent;
    }

    const XGL_EXTENT2D* get_presentable_image_extents(const XGL_IMAGE& image)
    {
        std::map<XGL_IMAGE, XGL_EXTENT2D>::const_iterator q = m_presentableImageSizes.find(image);
        return (q == m_presentableImageSizes.end()) ? NULL : &(q->second);
    }

    std::map<XGL_IMAGE_VIEW, XGL_IMAGE_VIEW> m_imageViews;
    void add_to_map(XGL_IMAGE_VIEW* pTraceImageView, XGL_IMAGE_VIEW* pReplayImageView)
    {
        assert(pTraceImageView != NULL);
        assert(pReplayImageView != NULL);
        m_imageViews[*pTraceImageView] = *pReplayImageView;
    }
    void rm_from_map(const XGL_IMAGE_VIEW& imageView)
    {
        m_imageViews.erase(imageView);
    }
    XGL_IMAGE_VIEW remap(const XGL_IMAGE_VIEW& imageView)
    {
        std::map<XGL_IMAGE_VIEW, XGL_IMAGE_VIEW>::const_iterator q = m_imageViews.find(imageView);
        return (q == m_imageViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_COLOR_ATTACHMENT_VIEW, XGL_COLOR_ATTACHMENT_VIEW> m_colorTargetViews;
    void add_to_map(XGL_COLOR_ATTACHMENT_VIEW* pTraceColorTargetView, XGL_COLOR_ATTACHMENT_VIEW* pReplayColorTargetView)
    {
        assert(pTraceColorTargetView != NULL);
        assert(pReplayColorTargetView != NULL);
        m_colorTargetViews[*pTraceColorTargetView] = *pReplayColorTargetView;
    }
    void rm_from_map(const XGL_COLOR_ATTACHMENT_VIEW& colorTargetView)
    {
        m_colorTargetViews.erase(colorTargetView);
    }
    XGL_COLOR_ATTACHMENT_VIEW remap(const XGL_COLOR_ATTACHMENT_VIEW& colorTargetView)
    {
        std::map<XGL_COLOR_ATTACHMENT_VIEW, XGL_COLOR_ATTACHMENT_VIEW>::const_iterator q = m_colorTargetViews.find(colorTargetView);
        return (q == m_colorTargetViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DEPTH_STENCIL_VIEW, XGL_DEPTH_STENCIL_VIEW> m_depthStencilViews;
    void add_to_map(XGL_DEPTH_STENCIL_VIEW* pTraceDepthStencilView, XGL_DEPTH_STENCIL_VIEW* pReplayDepthStencilView)
    {
        assert(pTraceDepthStencilView != NULL);
        assert(pReplayDepthStencilView != NULL);
        m_depthStencilViews[*pTraceDepthStencilView] = *pReplayDepthStencilView;
    }
    void rm_from_map(const XGL_DEPTH_STENCIL_VIEW& depthStencilView)
    {
        m_depthStencilViews.erase(depthStencilView);
    }
    XGL_DEPTH_STENCIL_VIEW remap(const XGL_DEPTH_STENCIL_VIEW& depthStencilView)
    {
        std::map<XGL_DEPTH_STENCIL_VIEW, XGL_DEPTH_STENCIL_VIEW>::const_iterator q = m_depthStencilViews.find(depthStencilView);
        return (q == m_depthStencilViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_SHADER, XGL_SHADER> m_shader;
    void add_to_map(XGL_SHADER* pTraceShader, XGL_SHADER* pReplayShader)
    {
        assert(pTraceShader != NULL);
        assert(pReplayShader != NULL);
        m_shader[*pTraceShader] = *pReplayShader;
    }
    void rm_from_map(const XGL_SHADER& shader)
    {
        m_shader.erase(shader);
    }
    XGL_SHADER remap(const XGL_SHADER& shader)
    {
        std::map<XGL_SHADER, XGL_SHADER>::const_iterator q = m_shader.find(shader);
        return (q == m_shader.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_PIPELINE, XGL_PIPELINE> m_pipeline;
    void add_to_map(XGL_PIPELINE* pTracepipeline, XGL_PIPELINE* pReplaypipeline)
    {
        assert(pTracepipeline != NULL);
        assert(pReplaypipeline != NULL);
        m_pipeline[*pTracepipeline] = *pReplaypipeline;
    }
    void rm_from_map(const XGL_PIPELINE& pipeline)
    {
        m_pipeline.erase(pipeline);
    }
    XGL_PIPELINE remap(const XGL_PIPELINE& pipeline)
    {
        std::map<XGL_PIPELINE, XGL_PIPELINE>::const_iterator q = m_pipeline.find(pipeline);
        return (q == m_pipeline.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_PIPELINE_DELTA, XGL_PIPELINE_DELTA> m_pipelineDelta;
    void add_to_map(XGL_PIPELINE_DELTA* pTracepipedelta, XGL_PIPELINE_DELTA* pReplaypipedelta)
    {
        assert(pTracepipedelta != NULL);
        assert(pReplaypipedelta != NULL);
        m_pipelineDelta[*pTracepipedelta] = *pReplaypipedelta;
    }
    void rm_from_map(const XGL_PIPELINE_DELTA& pipedelta)
    {
        m_pipelineDelta.erase(pipedelta);
    }
    XGL_PIPELINE_DELTA remap(const XGL_PIPELINE_DELTA& pipedelta)
    {
        std::map<XGL_PIPELINE_DELTA, XGL_PIPELINE_DELTA>::const_iterator q = m_pipelineDelta.find(pipedelta);
        return (q == m_pipelineDelta.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_SAMPLER, XGL_SAMPLER> m_sampler;
    void add_to_map(XGL_SAMPLER* pTracesampler, XGL_SAMPLER* pReplaysampler)
    {
        assert(pTracesampler != NULL);
        assert(pReplaysampler != NULL);
        m_sampler[*pTracesampler] = *pReplaysampler;
    }
    void rm_from_map(const XGL_SAMPLER& sampler)
    {
        m_sampler.erase(sampler);
    }
    XGL_SAMPLER remap(const XGL_SAMPLER& sampler)
    {
        std::map<XGL_SAMPLER, XGL_SAMPLER>::const_iterator q = m_sampler.find(sampler);
        return (q == m_sampler.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DESCRIPTOR_SET, XGL_DESCRIPTOR_SET> m_descriptorSets;
    void add_to_map(XGL_DESCRIPTOR_SET* pTraceSet, XGL_DESCRIPTOR_SET* pReplaySet)
    {
        assert(pTraceSet != NULL);
        assert(pReplaySet != NULL);
        m_descriptorSets[*pTraceSet] = *pReplaySet;
    }
    void rm_from_map(const XGL_DESCRIPTOR_SET& descriptorSet)
    {
        m_descriptorSets.erase(descriptorSet);
    }
    XGL_DESCRIPTOR_SET remap(const XGL_DESCRIPTOR_SET& descriptorSet)
    {
        std::map<XGL_DESCRIPTOR_SET, XGL_DESCRIPTOR_SET>::const_iterator q = m_descriptorSets.find(descriptorSet);
        return (q == m_descriptorSets.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_VIEWPORT_STATE_OBJECT, XGL_VIEWPORT_STATE_OBJECT> m_viewportStates;
    void add_to_map(XGL_VIEWPORT_STATE_OBJECT* pTraceState, XGL_VIEWPORT_STATE_OBJECT* pReplayState)
    {
        assert(pTraceState != NULL);
        assert(pReplayState != NULL);
        m_viewportStates[*pTraceState] = *pReplayState;
    }
    void rm_from_map(const XGL_VIEWPORT_STATE_OBJECT& state)
    {
        m_viewportStates.erase(state);
    }
    XGL_VIEWPORT_STATE_OBJECT remap(const XGL_VIEWPORT_STATE_OBJECT& state)
    {
        std::map<XGL_VIEWPORT_STATE_OBJECT, XGL_VIEWPORT_STATE_OBJECT>::const_iterator q = m_viewportStates.find(state);
        return (q == m_viewportStates.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_RASTER_STATE_OBJECT, XGL_RASTER_STATE_OBJECT> m_rasterStates;
    void add_to_map(XGL_RASTER_STATE_OBJECT* pTraceState, XGL_RASTER_STATE_OBJECT* pReplayState)
    {
        assert(pTraceState != NULL);
        assert(pReplayState != NULL);
        m_rasterStates[*pTraceState] = *pReplayState;
    }
    void rm_from_map(const XGL_RASTER_STATE_OBJECT& state)
    {
        m_rasterStates.erase(state);
    }
    XGL_RASTER_STATE_OBJECT remap(const XGL_RASTER_STATE_OBJECT& state)
    {
        std::map<XGL_RASTER_STATE_OBJECT, XGL_RASTER_STATE_OBJECT>::const_iterator q = m_rasterStates.find(state);
        return (q == m_rasterStates.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_MSAA_STATE_OBJECT, XGL_MSAA_STATE_OBJECT> m_msaaStates;
    void add_to_map(XGL_MSAA_STATE_OBJECT* pTraceState, XGL_MSAA_STATE_OBJECT* pReplayState)
    {
        assert(pTraceState != NULL);
        assert(pReplayState != NULL);
        m_msaaStates[*pTraceState] = *pReplayState;
    }
    void rm_from_map(const XGL_MSAA_STATE_OBJECT& state)
    {
        m_msaaStates.erase(state);
    }
    XGL_MSAA_STATE_OBJECT remap(const XGL_MSAA_STATE_OBJECT& state)
    {
        std::map<XGL_MSAA_STATE_OBJECT, XGL_MSAA_STATE_OBJECT>::const_iterator q = m_msaaStates.find(state);
        return (q == m_msaaStates.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_COLOR_BLEND_STATE_OBJECT, XGL_COLOR_BLEND_STATE_OBJECT> m_colorBlendStates;
    void add_to_map(XGL_COLOR_BLEND_STATE_OBJECT* pTraceState, XGL_COLOR_BLEND_STATE_OBJECT* pReplayState)
    {
        assert(pTraceState != NULL);
        assert(pReplayState != NULL);
        m_colorBlendStates[*pTraceState] = *pReplayState;
    }
    void rm_from_map(const XGL_COLOR_BLEND_STATE_OBJECT& state)
    {
        m_colorBlendStates.erase(state);
    }
    XGL_COLOR_BLEND_STATE_OBJECT remap(const XGL_COLOR_BLEND_STATE_OBJECT& state)
    {
        std::map<XGL_COLOR_BLEND_STATE_OBJECT, XGL_COLOR_BLEND_STATE_OBJECT>::const_iterator q = m_colorBlendStates.find(state);
        return (q == m_colorBlendStates.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DEPTH_STENCIL_STATE_OBJECT, XGL_DEPTH_STENCIL_STATE_OBJECT> m_depthStencilStates;
    void add_to_map(XGL_DEPTH_STENCIL_STATE_OBJECT* pTraceState, XGL_DEPTH_STENCIL_STATE_OBJECT* pReplayState)
    {
        assert(pTraceState != NULL);
        assert(pReplayState != NULL);
        m_depthStencilStates[*pTraceState] = *pReplayState;
    }
    void rm_from_map(const XGL_DEPTH_STENCIL_STATE_OBJECT& state)
    {
        m_depthStencilStates.erase(state);
    }
    XGL_DEPTH_STENCIL_STATE_OBJECT remap(const XGL_DEPTH_STENCIL_STATE_OBJECT& state)
    {
        std::map<XGL_DEPTH_STENCIL_STATE_OBJECT, XGL_DEPTH_STENCIL_STATE_OBJECT>::const_iterator q = m_depthStencilStates.find(state);
        return (q == m_depthStencilStates.end()) ? XGL_NULL_HANDLE : q->second;
    }

    XGL_STATE_OBJECT remap(const XGL_STATE_OBJECT& state)
    {
        /* includes: XGL_VIEWPORT_STATE_OBJECT, XGL_RASTER_STATE_OBJECT, XGL_MSAA_STATE_OBJECT
         * XGL_COLOR_BLEND_STATE_OBJECT, XGL_DEPTH_STENCIL_STATE_OBJECT
         */
        XGL_STATE_OBJECT obj;
        if ((obj = remap(static_cast <XGL_VIEWPORT_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_RASTER_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_MSAA_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_COLOR_BLEND_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DEPTH_STENCIL_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }
    void rm_from_map(const XGL_STATE_OBJECT& state)
    {
        rm_from_map(static_cast <XGL_VIEWPORT_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_RASTER_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_MSAA_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_COLOR_BLEND_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DEPTH_STENCIL_STATE_OBJECT> (state));
    }
    std::map<XGL_CMD_BUFFER, XGL_CMD_BUFFER> m_cmdBuffers;
    void add_to_map(XGL_CMD_BUFFER* pTraceCmdBuffer, XGL_CMD_BUFFER* pReplayCmdBuffer)
    {
        assert(pTraceCmdBuffer != NULL);
        assert(pReplayCmdBuffer != NULL);
        m_cmdBuffers[*pTraceCmdBuffer] = *pReplayCmdBuffer;
    }
    void rm_from_map(const XGL_CMD_BUFFER& cmdBuffer)
    {
        m_cmdBuffers.erase(cmdBuffer);
    }
    XGL_CMD_BUFFER remap(const XGL_CMD_BUFFER& cmdBuffer)
    {
        std::map<XGL_CMD_BUFFER, XGL_CMD_BUFFER>::const_iterator q = m_cmdBuffers.find(cmdBuffer);
        return (q == m_cmdBuffers.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_FENCE, XGL_FENCE> m_fences;
    void add_to_map(XGL_FENCE* pTraceFence, XGL_FENCE* pReplayFence)
    {
        assert(pTraceFence != NULL);
        assert(pReplayFence != NULL);
        m_fences[*pTraceFence] = *pReplayFence;
    }
    void rm_from_map(const XGL_FENCE& fence)
    {
        m_fences.erase(fence);
    }
    XGL_FENCE remap(const XGL_FENCE& fence)
    {
        std::map<XGL_FENCE, XGL_FENCE>::const_iterator q = m_fences.find(fence);
        return (q == m_fences.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_QUEUE_SEMAPHORE, XGL_QUEUE_SEMAPHORE> m_queue_semaphores;
    void add_to_map(XGL_QUEUE_SEMAPHORE* pTraceSema, XGL_QUEUE_SEMAPHORE* pReplaySema)
    {
        assert(pTraceSema != NULL);
        assert(pReplaySema != NULL);
        m_queue_semaphores[*pTraceSema] = *pReplaySema;
    }
    void rm_from_map(const XGL_QUEUE_SEMAPHORE& sema)
    {
        m_queue_semaphores.erase(sema);
    }
    XGL_QUEUE_SEMAPHORE remap(const XGL_QUEUE_SEMAPHORE& sema)
    {
        std::map<XGL_QUEUE_SEMAPHORE, XGL_QUEUE_SEMAPHORE>::const_iterator q = m_queue_semaphores.find(sema);
        return (q == m_queue_semaphores.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_EVENT, XGL_EVENT> m_events;
    void add_to_map(XGL_EVENT* pTraceEvent, XGL_EVENT* pReplayEvent)
    {
        assert(pTraceEvent != NULL);
        assert(pReplayEvent != NULL);
        m_events[*pTraceEvent] = *pReplayEvent;
    }
    void rm_from_map(const XGL_EVENT& event)
    {
        m_events.erase(event);
    }
    XGL_EVENT remap(const XGL_EVENT& event)
    {
        std::map<XGL_EVENT, XGL_EVENT>::const_iterator q = m_events.find(event);
        return (q == m_events.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_QUERY_POOL, XGL_QUERY_POOL> m_queryPools;
    void add_to_map(XGL_QUERY_POOL* pTracePool, XGL_QUERY_POOL* pReplayPool)
    {
        assert(pTracePool != NULL);
        assert(pReplayPool != NULL);
        m_queryPools[*pTracePool] = *pReplayPool;
    }
    void rm_from_map(const XGL_QUERY_POOL& queryPool)
    {
        m_queryPools.erase(queryPool);
    }
    XGL_QUERY_POOL remap(const XGL_QUERY_POOL& queryPool)
    {
        std::map<XGL_QUERY_POOL, XGL_QUERY_POOL>::const_iterator q = m_queryPools.find(queryPool);
        return (q == m_queryPools.end()) ? XGL_NULL_HANDLE : q->second;
    }

    XGL_OBJECT remap(const XGL_OBJECT& object)
    {
        /*
         * Includes: XGL_IMAGE, XGL_IMAGE_VIEW, XGL_COLOR_ATTACHMENT_VIEW, XGL_DEPTH_STENCIL_VIEW,
         * XGL_SHADER, XGL_PIPELINE, XGL_PIPELINE_DELTA, XGL_SAMPLER, XGL_DESCRIPTOR_SET, XGL_STATE_OBJECT (plus subclasses),
         * XGL_CMD_BUFFER, XGL_FENCE, XGL_QUEUE_SEMAPHORE, XGL_EVENT, XGL_QUERY_POOL
         * XGL_WSI_WIN_DISPLAY
         * XGL_BORDER_COLOR_PALETTE
         */
        XGL_OBJECT obj;
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
        if ((obj = remap(static_cast <XGL_STATE_OBJECT> (object))) != XGL_NULL_HANDLE)
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

        return XGL_NULL_HANDLE;
    }

    void rm_from_map(const XGL_OBJECT & objKey)
    {
        rm_from_map(static_cast <XGL_IMAGE> (objKey));
        rm_from_map(static_cast <XGL_IMAGE_VIEW> (objKey));
        rm_from_map(static_cast <XGL_COLOR_ATTACHMENT_VIEW> (objKey));
        rm_from_map(static_cast <XGL_DEPTH_STENCIL_VIEW> (objKey));
        rm_from_map(static_cast <XGL_SHADER> (objKey));
        rm_from_map(static_cast <XGL_PIPELINE> (objKey));
        rm_from_map(static_cast <XGL_PIPELINE_DELTA> (objKey));
        rm_from_map(static_cast <XGL_SAMPLER> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_SET> (objKey));
        rm_from_map(static_cast <XGL_STATE_OBJECT> (objKey));
        rm_from_map(static_cast <XGL_CMD_BUFFER> (objKey));
        rm_from_map(static_cast <XGL_FENCE> (objKey));
        rm_from_map(static_cast <XGL_QUEUE_SEMAPHORE> (objKey));
        rm_from_map(static_cast <XGL_EVENT> (objKey));
        rm_from_map(static_cast <XGL_QUERY_POOL> (objKey));
    }

    XGL_BASE_OBJECT remap(const XGL_BASE_OBJECT& object)
    {
        XGL_BASE_OBJECT obj;

        /*
         * Includes: XGL_DEVICE, XGL_QUEUE, XGL_GPU_MEMORY, XGL_OBJECT
         */
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

};

