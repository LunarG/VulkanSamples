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

#include "glvreplay_xgl_xglreplay.h"

#include "glvreplay_xgl.h"

#include "glvreplay_main.h"

#include <algorithm>
#include <queue>


extern "C" {
#include "glv_vk_vk_structs.h"
#include "glv_vk_vkdbg_structs.h"
#include "glv_vk_vkwsix11ext_structs.h"
#include "glv_vk_packet_id.h"
#include "xgl_enum_string_helper.h"
}

#define APP_NAME "glvreplay_xgl"
#define IDI_ICON 101


void xglFuncs::init_funcs(void * handle)
{
    m_libHandle = handle;
    real_xglCreateInstance = (type_xglCreateInstance)(glv_platform_get_library_entrypoint(handle, "xglCreateInstance"));
    real_xglDestroyInstance = (type_xglDestroyInstance)(glv_platform_get_library_entrypoint(handle, "xglDestroyInstance"));
    real_xglEnumerateGpus = (type_xglEnumerateGpus)(glv_platform_get_library_entrypoint(handle, "xglEnumerateGpus"));
    real_xglGetGpuInfo = (type_xglGetGpuInfo)(glv_platform_get_library_entrypoint(handle, "xglGetGpuInfo"));
    real_xglGetProcAddr = (type_xglGetProcAddr)(glv_platform_get_library_entrypoint(handle, "xglGetProcAddr"));
    real_xglCreateDevice = (type_xglCreateDevice)(glv_platform_get_library_entrypoint(handle, "xglCreateDevice"));
    real_xglDestroyDevice = (type_xglDestroyDevice)(glv_platform_get_library_entrypoint(handle, "xglDestroyDevice"));
    real_xglGetExtensionSupport = (type_xglGetExtensionSupport)(glv_platform_get_library_entrypoint(handle, "xglGetExtensionSupport"));
    real_xglEnumerateLayers = (type_xglEnumerateLayers)(glv_platform_get_library_entrypoint(handle, "xglEnumerateLayers"));
    real_xglGetDeviceQueue = (type_xglGetDeviceQueue)(glv_platform_get_library_entrypoint(handle, "xglGetDeviceQueue"));
    real_xglQueueSubmit = (type_xglQueueSubmit)(glv_platform_get_library_entrypoint(handle, "xglQueueSubmit"));
    real_xglQueueSetGlobalMemReferences = (type_xglQueueSetGlobalMemReferences)(glv_platform_get_library_entrypoint(handle, "xglQueueSetGlobalMemReferences"));
    real_xglQueueWaitIdle = (type_xglQueueWaitIdle)(glv_platform_get_library_entrypoint(handle, "xglQueueWaitIdle"));
    real_xglDeviceWaitIdle = (type_xglDeviceWaitIdle)(glv_platform_get_library_entrypoint(handle, "xglDeviceWaitIdle"));
    real_xglAllocMemory = (type_xglAllocMemory)(glv_platform_get_library_entrypoint(handle, "xglAllocMemory"));
    real_xglFreeMemory = (type_xglFreeMemory)(glv_platform_get_library_entrypoint(handle, "xglFreeMemory"));
    real_xglSetMemoryPriority = (type_xglSetMemoryPriority)(glv_platform_get_library_entrypoint(handle, "xglSetMemoryPriority"));
    real_xglMapMemory = (type_xglMapMemory)(glv_platform_get_library_entrypoint(handle, "xglMapMemory"));
    real_xglUnmapMemory = (type_xglUnmapMemory)(glv_platform_get_library_entrypoint(handle, "xglUnmapMemory"));
    real_xglPinSystemMemory = (type_xglPinSystemMemory)(glv_platform_get_library_entrypoint(handle, "xglPinSystemMemory"));
    real_xglGetMultiGpuCompatibility = (type_xglGetMultiGpuCompatibility)(glv_platform_get_library_entrypoint(handle, "xglGetMultiGpuCompatibility"));
    real_xglOpenSharedMemory = (type_xglOpenSharedMemory)(glv_platform_get_library_entrypoint(handle, "xglOpenSharedMemory"));
    real_xglOpenSharedQueueSemaphore = (type_xglOpenSharedQueueSemaphore)(glv_platform_get_library_entrypoint(handle, "xglOpenSharedQueueSemaphore"));
    real_xglOpenPeerMemory = (type_xglOpenPeerMemory)(glv_platform_get_library_entrypoint(handle, "xglOpenPeerMemory"));
    real_xglOpenPeerImage = (type_xglOpenPeerImage)(glv_platform_get_library_entrypoint(handle, "xglOpenPeerImage"));
    real_xglDestroyObject = (type_xglDestroyObject)(glv_platform_get_library_entrypoint(handle, "xglDestroyObject"));
    real_xglGetObjectInfo = (type_xglGetObjectInfo)(glv_platform_get_library_entrypoint(handle, "xglGetObjectInfo"));
    real_xglBindObjectMemory = (type_xglBindObjectMemory)(glv_platform_get_library_entrypoint(handle, "xglBindObjectMemory"));
    real_xglBindObjectMemoryRange = (type_xglBindObjectMemoryRange)(glv_platform_get_library_entrypoint(handle, "xglBindObjectMemoryRange"));
    real_xglBindImageMemoryRange = (type_xglBindImageMemoryRange)(glv_platform_get_library_entrypoint(handle, "xglBindImageMemoryRange"));
    real_xglCreateFence = (type_xglCreateFence)(glv_platform_get_library_entrypoint(handle, "xglCreateFence"));
    real_xglGetFenceStatus = (type_xglGetFenceStatus)(glv_platform_get_library_entrypoint(handle, "xglGetFenceStatus"));
    real_xglWaitForFences = (type_xglWaitForFences)(glv_platform_get_library_entrypoint(handle, "xglWaitForFences"));
    real_xglCreateQueueSemaphore = (type_xglCreateQueueSemaphore)(glv_platform_get_library_entrypoint(handle, "xglCreateQueueSemaphore"));
    real_xglSignalQueueSemaphore = (type_xglSignalQueueSemaphore)(glv_platform_get_library_entrypoint(handle, "xglSignalQueueSemaphore"));
    real_xglWaitQueueSemaphore = (type_xglWaitQueueSemaphore)(glv_platform_get_library_entrypoint(handle, "xglWaitQueueSemaphore"));
    real_xglCreateEvent = (type_xglCreateEvent)(glv_platform_get_library_entrypoint(handle, "xglCreateEvent"));
    real_xglGetEventStatus = (type_xglGetEventStatus)(glv_platform_get_library_entrypoint(handle, "xglGetEventStatus"));
    real_xglSetEvent = (type_xglSetEvent)(glv_platform_get_library_entrypoint(handle, "xglSetEvent"));
    real_xglResetEvent = (type_xglResetEvent)(glv_platform_get_library_entrypoint(handle, "xglResetEvent"));
    real_xglCreateQueryPool = (type_xglCreateQueryPool)(glv_platform_get_library_entrypoint(handle, "xglCreateQueryPool"));
    real_xglGetQueryPoolResults = (type_xglGetQueryPoolResults)(glv_platform_get_library_entrypoint(handle, "xglGetQueryPoolResults"));
    real_xglGetFormatInfo = (type_xglGetFormatInfo)(glv_platform_get_library_entrypoint(handle, "xglGetFormatInfo"));
    real_xglCreateBuffer = (type_xglCreateBuffer)(glv_platform_get_library_entrypoint(handle, "xglCreateBuffer"));
    real_xglCreateBufferView = (type_xglCreateBufferView)(glv_platform_get_library_entrypoint(handle, "xglCreateBufferView"));
    real_xglCreateImage = (type_xglCreateImage)(glv_platform_get_library_entrypoint(handle, "xglCreateImage"));
    real_xglSetFastClearColor = (type_xglSetFastClearColor)(glv_platform_get_library_entrypoint(handle, "xglSetFastClearColor"));
    real_xglSetFastClearDepth = (type_xglSetFastClearDepth)(glv_platform_get_library_entrypoint(handle, "xglSetFastClearDepth"));
    real_xglGetImageSubresourceInfo = (type_xglGetImageSubresourceInfo)(glv_platform_get_library_entrypoint(handle, "xglGetImageSubresourceInfo"));
    real_xglCreateImageView = (type_xglCreateImageView)(glv_platform_get_library_entrypoint(handle, "xglCreateImageView"));
    real_xglCreateColorAttachmentView = (type_xglCreateColorAttachmentView)(glv_platform_get_library_entrypoint(handle, "xglCreateColorAttachmentView"));
    real_xglCreateDepthStencilView = (type_xglCreateDepthStencilView)(glv_platform_get_library_entrypoint(handle, "xglCreateDepthStencilView"));
    real_xglCreateShader = (type_xglCreateShader)(glv_platform_get_library_entrypoint(handle, "xglCreateShader"));
    real_xglCreateGraphicsPipeline = (type_xglCreateGraphicsPipeline)(glv_platform_get_library_entrypoint(handle, "xglCreateGraphicsPipeline"));
    real_xglCreateComputePipeline = (type_xglCreateComputePipeline)(glv_platform_get_library_entrypoint(handle, "xglCreateComputePipeline"));
    real_xglStorePipeline = (type_xglStorePipeline)(glv_platform_get_library_entrypoint(handle, "xglStorePipeline"));
    real_xglLoadPipeline = (type_xglLoadPipeline)(glv_platform_get_library_entrypoint(handle, "xglLoadPipeline"));
    real_xglCreatePipelineDelta = (type_xglCreatePipelineDelta)(glv_platform_get_library_entrypoint(handle, "xglCreatePipelineDelta"));
    real_xglCreateSampler = (type_xglCreateSampler)(glv_platform_get_library_entrypoint(handle, "xglCreateSampler"));
    real_xglCreateDescriptorSetLayout = (type_xglCreateDescriptorSetLayout)(glv_platform_get_library_entrypoint(handle, "xglCreateDescriptorSetLayout"));
    real_xglBeginDescriptorRegionUpdate = (type_xglBeginDescriptorRegionUpdate)(glv_platform_get_library_entrypoint(handle, "xglBeginDescriptorRegionUpdate"));
    real_xglEndDescriptorRegionUpdate = (type_xglEndDescriptorRegionUpdate)(glv_platform_get_library_entrypoint(handle, "xglEndDescriptorRegionUpdate"));
    real_xglCreateDescriptorRegion = (type_xglCreateDescriptorRegion)(glv_platform_get_library_entrypoint(handle, "xglCreateDescriptorRegion"));
    real_xglClearDescriptorRegion = (type_xglClearDescriptorRegion)(glv_platform_get_library_entrypoint(handle, "xglClearDescriptorRegion"));
    real_xglAllocDescriptorSets = (type_xglAllocDescriptorSets)(glv_platform_get_library_entrypoint(handle, "xglAllocDescriptorSets"));
    real_xglClearDescriptorSets = (type_xglClearDescriptorSets)(glv_platform_get_library_entrypoint(handle, "xglClearDescriptorSets"));
    real_xglUpdateDescriptors = (type_xglUpdateDescriptors)(glv_platform_get_library_entrypoint(handle, "xglUpdateDescriptors"));
    real_xglCreateDynamicViewportState = (type_xglCreateDynamicViewportState)(glv_platform_get_library_entrypoint(handle, "xglCreateDynamicViewportState"));
    real_xglCreateDynamicRasterState = (type_xglCreateDynamicRasterState)(glv_platform_get_library_entrypoint(handle, "xglCreateDynamicRasterState"));
    real_xglCreateDynamicColorBlendState = (type_xglCreateDynamicColorBlendState)(glv_platform_get_library_entrypoint(handle, "xglCreateDynamicColorBlendState"));
    real_xglCreateDynamicDepthStencilState = (type_xglCreateDynamicDepthStencilState)(glv_platform_get_library_entrypoint(handle, "xglCreateDynamicDepthStencilState"));
    real_xglCreateCommandBuffer = (type_xglCreateCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglCreateCommandBuffer"));
    real_xglBeginCommandBuffer = (type_xglBeginCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglBeginCommandBuffer"));
    real_xglEndCommandBuffer = (type_xglEndCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglEndCommandBuffer"));
    real_xglResetCommandBuffer = (type_xglResetCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglResetCommandBuffer"));
    real_xglCmdBindPipeline = (type_xglCmdBindPipeline)(glv_platform_get_library_entrypoint(handle, "xglCmdBindPipeline"));
    real_xglCmdBindPipelineDelta = (type_xglCmdBindPipelineDelta)(glv_platform_get_library_entrypoint(handle, "xglCmdBindPipelineDelta"));
    real_xglCmdBindDynamicStateObject = (type_xglCmdBindDynamicStateObject)(glv_platform_get_library_entrypoint(handle, "xglCmdBindDynamicStateObject"));
    real_xglCmdBindDescriptorSet = (type_xglCmdBindDescriptorSet)(glv_platform_get_library_entrypoint(handle, "xglCmdBindDescriptorSet"));
    real_xglCmdBindVertexBuffer = (type_xglCmdBindVertexBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdBindVertexBuffer"));
    real_xglCmdBindIndexBuffer = (type_xglCmdBindIndexBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdBindIndexBuffer"));
    real_xglCmdDraw = (type_xglCmdDraw)(glv_platform_get_library_entrypoint(handle, "xglCmdDraw"));
    real_xglCmdDrawIndexed = (type_xglCmdDrawIndexed)(glv_platform_get_library_entrypoint(handle, "xglCmdDrawIndexed"));
    real_xglCmdDrawIndirect = (type_xglCmdDrawIndirect)(glv_platform_get_library_entrypoint(handle, "xglCmdDrawIndirect"));
    real_xglCmdDrawIndexedIndirect = (type_xglCmdDrawIndexedIndirect)(glv_platform_get_library_entrypoint(handle, "xglCmdDrawIndexedIndirect"));
    real_xglCmdDispatch = (type_xglCmdDispatch)(glv_platform_get_library_entrypoint(handle, "xglCmdDispatch"));
    real_xglCmdDispatchIndirect = (type_xglCmdDispatchIndirect)(glv_platform_get_library_entrypoint(handle, "xglCmdDispatchIndirect"));
    real_xglCmdCopyBuffer = (type_xglCmdCopyBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyBuffer"));
    real_xglCmdCopyImage = (type_xglCmdCopyImage)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyImage"));
    real_xglCmdCopyBufferToImage = (type_xglCmdCopyBufferToImage)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyBufferToImage"));
    real_xglCmdCopyImageToBuffer = (type_xglCmdCopyImageToBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyImageToBuffer"));
    real_xglCmdCloneImageData = (type_xglCmdCloneImageData)(glv_platform_get_library_entrypoint(handle, "xglCmdCloneImageData"));
    real_xglCmdUpdateBuffer = (type_xglCmdUpdateBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdUpdateBuffer"));
    real_xglCmdFillBuffer = (type_xglCmdFillBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdFillBuffer"));
    real_xglCmdClearColorImage = (type_xglCmdClearColorImage)(glv_platform_get_library_entrypoint(handle, "xglCmdClearColorImage"));
    real_xglCmdClearColorImageRaw = (type_xglCmdClearColorImageRaw)(glv_platform_get_library_entrypoint(handle, "xglCmdClearColorImageRaw"));
    real_xglCmdClearDepthStencil = (type_xglCmdClearDepthStencil)(glv_platform_get_library_entrypoint(handle, "xglCmdClearDepthStencil"));
    real_xglCmdResolveImage = (type_xglCmdResolveImage)(glv_platform_get_library_entrypoint(handle, "xglCmdResolveImage"));
    real_xglCmdSetEvent = (type_xglCmdSetEvent)(glv_platform_get_library_entrypoint(handle, "xglCmdSetEvent"));
    real_xglCmdResetEvent = (type_xglCmdResetEvent)(glv_platform_get_library_entrypoint(handle, "xglCmdResetEvent"));
    real_xglCmdWaitEvents = (type_xglCmdWaitEvents)(glv_platform_get_library_entrypoint(handle, "xglCmdWaitEvents"));
    real_xglCmdPipelineBarrier = (type_xglCmdPipelineBarrier)(glv_platform_get_library_entrypoint(handle, "xglCmdPipelineBarrier"));
    real_xglCmdBeginQuery = (type_xglCmdBeginQuery)(glv_platform_get_library_entrypoint(handle, "xglCmdBeginQuery"));
    real_xglCmdEndQuery = (type_xglCmdEndQuery)(glv_platform_get_library_entrypoint(handle, "xglCmdEndQuery"));
    real_xglCmdResetQueryPool = (type_xglCmdResetQueryPool)(glv_platform_get_library_entrypoint(handle, "xglCmdResetQueryPool"));
    real_xglCmdWriteTimestamp = (type_xglCmdWriteTimestamp)(glv_platform_get_library_entrypoint(handle, "xglCmdWriteTimestamp"));
    real_xglCmdInitAtomicCounters = (type_xglCmdInitAtomicCounters)(glv_platform_get_library_entrypoint(handle, "xglCmdInitAtomicCounters"));
    real_xglCmdLoadAtomicCounters = (type_xglCmdLoadAtomicCounters)(glv_platform_get_library_entrypoint(handle, "xglCmdLoadAtomicCounters"));
    real_xglCmdSaveAtomicCounters = (type_xglCmdSaveAtomicCounters)(glv_platform_get_library_entrypoint(handle, "xglCmdSaveAtomicCounters"));
    real_xglCreateFramebuffer = (type_xglCreateFramebuffer)(glv_platform_get_library_entrypoint(handle, "xglCreateFramebuffer"));
    real_xglCreateRenderPass = (type_xglCreateRenderPass)(glv_platform_get_library_entrypoint(handle, "xglCreateRenderPass"));
    real_xglCmdBeginRenderPass = (type_xglCmdBeginRenderPass)(glv_platform_get_library_entrypoint(handle, "xglCmdBeginRenderPass"));
    real_xglCmdEndRenderPass = (type_xglCmdEndRenderPass)(glv_platform_get_library_entrypoint(handle, "xglCmdEndRenderPass"));
    real_xglDbgSetValidationLevel = (type_xglDbgSetValidationLevel)(glv_platform_get_library_entrypoint(handle, "xglDbgSetValidationLevel"));
    real_xglDbgRegisterMsgCallback = (type_xglDbgRegisterMsgCallback)(glv_platform_get_library_entrypoint(handle, "xglDbgRegisterMsgCallback"));
    real_xglDbgUnregisterMsgCallback = (type_xglDbgUnregisterMsgCallback)(glv_platform_get_library_entrypoint(handle, "xglDbgUnregisterMsgCallback"));
    real_xglDbgSetMessageFilter = (type_xglDbgSetMessageFilter)(glv_platform_get_library_entrypoint(handle, "xglDbgSetMessageFilter"));
    real_xglDbgSetObjectTag = (type_xglDbgSetObjectTag)(glv_platform_get_library_entrypoint(handle, "xglDbgSetObjectTag"));
    real_xglDbgSetGlobalOption = (type_xglDbgSetGlobalOption)(glv_platform_get_library_entrypoint(handle, "xglDbgSetGlobalOption"));
    real_xglDbgSetDeviceOption = (type_xglDbgSetDeviceOption)(glv_platform_get_library_entrypoint(handle, "xglDbgSetDeviceOption"));
    real_xglCmdDbgMarkerBegin = (type_xglCmdDbgMarkerBegin)(glv_platform_get_library_entrypoint(handle, "xglCmdDbgMarkerBegin"));
    real_xglCmdDbgMarkerEnd = (type_xglCmdDbgMarkerEnd)(glv_platform_get_library_entrypoint(handle, "xglCmdDbgMarkerEnd"));
    real_xglWsiX11AssociateConnection = (type_xglWsiX11AssociateConnection)(glv_platform_get_library_entrypoint(handle, "xglWsiX11AssociateConnection"));
    real_xglWsiX11GetMSC = (type_xglWsiX11GetMSC)(glv_platform_get_library_entrypoint(handle, "xglWsiX11GetMSC"));
    real_xglWsiX11CreatePresentableImage = (type_xglWsiX11CreatePresentableImage)(glv_platform_get_library_entrypoint(handle, "xglWsiX11CreatePresentableImage"));
    real_xglWsiX11QueuePresent = (type_xglWsiX11QueuePresent)(glv_platform_get_library_entrypoint(handle, "xglWsiX11QueuePresent"));
}
glv_replay::GLV_REPLAY_RESULT xglReplay::replay(glv_trace_packet_header *packet)
{
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    switch (packet->packet_id)
    {
        case GLV_TPI_XGL_xglApiVersion:
            break;  // nothing to replay on the version packet
        case GLV_TPI_XGL_xglCreateInstance:
        {
            struct_xglCreateInstance* pPacket = (struct_xglCreateInstance*)(packet->pBody);
            XGL_INSTANCE local_pInstance;
            replayResult = m_xglFuncs.real_xglCreateInstance(pPacket->pAppInfo, pPacket->pAllocCb, &local_pInstance);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pInstance, &local_pInstance);
            }
            CHECK_RETURN_VALUE(xglCreateInstance);
            break;
        }
        case GLV_TPI_XGL_xglDestroyInstance:
        {
            struct_xglDestroyInstance* pPacket = (struct_xglDestroyInstance*)(packet->pBody);
            xglDbgUnregisterMsgCallback(g_fpDbgMsgCallback);
            replayResult = m_xglFuncs.real_xglDestroyInstance(m_objMapper.remap(pPacket->instance));
            if (replayResult == XGL_SUCCESS)
            {
                // TODO need to handle multiple instances and only clearing maps within an instance.
                // TODO this only works with a single instance used at any given time.
                m_objMapper.clear_all_map_handles();
            }
            CHECK_RETURN_VALUE(xglDestroyInstance);
            break;
        }
        case GLV_TPI_XGL_xglEnumerateGpus:
        {
            struct_xglEnumerateGpus* pPacket = (struct_xglEnumerateGpus*)(packet->pBody);
            returnValue = manually_handle_xglEnumerateGpus(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglGetGpuInfo:
        {
            struct_xglGetGpuInfo* pPacket = (struct_xglGetGpuInfo*)(packet->pBody);
            returnValue = manually_handle_xglGetGpuInfo(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglGetProcAddr:
        {
            struct_xglGetProcAddr* pPacket = (struct_xglGetProcAddr*)(packet->pBody);
            m_xglFuncs.real_xglGetProcAddr(m_objMapper.remap(pPacket->gpu), pPacket->pName);
            break;
        }
        case GLV_TPI_XGL_xglCreateDevice:
        {
            struct_xglCreateDevice* pPacket = (struct_xglCreateDevice*)(packet->pBody);
            returnValue = manually_handle_xglCreateDevice(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglDestroyDevice:
        {
            struct_xglDestroyDevice* pPacket = (struct_xglDestroyDevice*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDestroyDevice(m_objMapper.remap(pPacket->device));
            if (replayResult == XGL_SUCCESS)
            {
                m_pCBDump = NULL;
                m_pDSDump = NULL;
                m_pGlvSnapshotPrint = NULL;
                m_objMapper.rm_from_map(pPacket->device);
                m_display->m_initedXGL = false;
            }
            CHECK_RETURN_VALUE(xglDestroyDevice);
            break;
        }
        case GLV_TPI_XGL_xglGetExtensionSupport:
        {
            struct_xglGetExtensionSupport* pPacket = (struct_xglGetExtensionSupport*)(packet->pBody);
            returnValue = manually_handle_xglGetExtensionSupport(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglEnumerateLayers:
        {
            struct_xglEnumerateLayers* pPacket = (struct_xglEnumerateLayers*)(packet->pBody);
            char **bufptr = GLV_NEW_ARRAY(char *, pPacket->maxLayerCount);
            char **ptrLayers = (pPacket->pOutLayers == NULL) ? bufptr : (char **) pPacket->pOutLayers;
            for (unsigned int i = 0; i < pPacket->maxLayerCount; i++)
                bufptr[i] = GLV_NEW_ARRAY(char, pPacket->maxStringSize);
            replayResult = m_xglFuncs.real_xglEnumerateLayers(m_objMapper.remap(pPacket->gpu), pPacket->maxLayerCount, pPacket->maxStringSize, pPacket->pOutLayerCount, ptrLayers, pPacket->pReserved);
            for (unsigned int i = 0; i < pPacket->maxLayerCount; i++)
                GLV_DELETE(bufptr[i]);
            CHECK_RETURN_VALUE(xglEnumerateLayers);
            break;
        }
        case GLV_TPI_XGL_xglGetDeviceQueue:
        {
            struct_xglGetDeviceQueue* pPacket = (struct_xglGetDeviceQueue*)(packet->pBody);
            XGL_QUEUE local_pQueue;
            replayResult = m_xglFuncs.real_xglGetDeviceQueue(m_objMapper.remap(pPacket->device), pPacket->queueType, pPacket->queueIndex, &local_pQueue);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pQueue, &local_pQueue);
            }
            CHECK_RETURN_VALUE(xglGetDeviceQueue);
            break;
        }
        case GLV_TPI_XGL_xglQueueSubmit:
        {
            struct_xglQueueSubmit* pPacket = (struct_xglQueueSubmit*)(packet->pBody);
            returnValue = manually_handle_xglQueueSubmit(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglQueueSetGlobalMemReferences:
        {
            struct_xglQueueSetGlobalMemReferences* pPacket = (struct_xglQueueSetGlobalMemReferences*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglQueueSetGlobalMemReferences(m_objMapper.remap(pPacket->queue), pPacket->memRefCount, pPacket->pMemRefs);
            CHECK_RETURN_VALUE(xglQueueSetGlobalMemReferences);
            break;
        }
        case GLV_TPI_XGL_xglQueueWaitIdle:
        {
            struct_xglQueueWaitIdle* pPacket = (struct_xglQueueWaitIdle*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglQueueWaitIdle(m_objMapper.remap(pPacket->queue));
            CHECK_RETURN_VALUE(xglQueueWaitIdle);
            break;
        }
        case GLV_TPI_XGL_xglDeviceWaitIdle:
        {
            struct_xglDeviceWaitIdle* pPacket = (struct_xglDeviceWaitIdle*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDeviceWaitIdle(m_objMapper.remap(pPacket->device));
            CHECK_RETURN_VALUE(xglDeviceWaitIdle);
            break;
        }
        case GLV_TPI_XGL_xglAllocMemory:
        {
            struct_xglAllocMemory* pPacket = (struct_xglAllocMemory*)(packet->pBody);
            gpuMemObj local_mem;
            if (!m_objMapper.m_adjustForGPU)
                replayResult = m_xglFuncs.real_xglAllocMemory(m_objMapper.remap(pPacket->device), pPacket->pAllocInfo, &local_mem.replayGpuMem);
            if (replayResult == XGL_SUCCESS || m_objMapper.m_adjustForGPU)
            {
                local_mem.pGpuMem = new (gpuMemory);
                if (local_mem.pGpuMem)
                    local_mem.pGpuMem->setAllocInfo(pPacket->pAllocInfo, m_objMapper.m_adjustForGPU);
                m_objMapper.add_to_map(pPacket->pMem, &local_mem);
            }
            CHECK_RETURN_VALUE(xglAllocMemory);
            break;
        }
        case GLV_TPI_XGL_xglFreeMemory:
        {
            struct_xglFreeMemory* pPacket = (struct_xglFreeMemory*)(packet->pBody);
            returnValue = manually_handle_xglFreeMemory(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglSetMemoryPriority:
        {
            struct_xglSetMemoryPriority* pPacket = (struct_xglSetMemoryPriority*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSetMemoryPriority(m_objMapper.remap(pPacket->mem), pPacket->priority);
            CHECK_RETURN_VALUE(xglSetMemoryPriority);
            break;
        }
        case GLV_TPI_XGL_xglMapMemory:
        {
            struct_xglMapMemory* pPacket = (struct_xglMapMemory*)(packet->pBody);
            returnValue = manually_handle_xglMapMemory(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglUnmapMemory:
        {
            struct_xglUnmapMemory* pPacket = (struct_xglUnmapMemory*)(packet->pBody);
            returnValue = manually_handle_xglUnmapMemory(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglPinSystemMemory:
        {
            struct_xglPinSystemMemory* pPacket = (struct_xglPinSystemMemory*)(packet->pBody);
            gpuMemObj local_mem;
            /* TODO do we need to skip (make pending) this call for m_adjustForGPU */
            replayResult = m_xglFuncs.real_xglPinSystemMemory(m_objMapper.remap(pPacket->device), pPacket->pSysMem, pPacket->memSize, &local_mem.replayGpuMem);
            if (replayResult == XGL_SUCCESS)
                m_objMapper.add_to_map(pPacket->pMem, &local_mem);
            CHECK_RETURN_VALUE(xglPinSystemMemory);
            break;
        }
        case GLV_TPI_XGL_xglGetMultiGpuCompatibility:
        {
            struct_xglGetMultiGpuCompatibility* pPacket = (struct_xglGetMultiGpuCompatibility*)(packet->pBody);
            returnValue = manually_handle_xglGetMultiGpuCompatibility(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglOpenSharedMemory:
        {
            struct_xglOpenSharedMemory* pPacket = (struct_xglOpenSharedMemory*)(packet->pBody);
            XGL_DEVICE handle;
            XGL_GPU_MEMORY local_pMem;
            handle = m_objMapper.remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenSharedMemory(handle, pPacket->pOpenInfo, &local_pMem);
            CHECK_RETURN_VALUE(xglOpenSharedMemory);
            break;
        }
        case GLV_TPI_XGL_xglOpenSharedQueueSemaphore:
        {
            struct_xglOpenSharedQueueSemaphore* pPacket = (struct_xglOpenSharedQueueSemaphore*)(packet->pBody);
            XGL_DEVICE handle;
            XGL_QUEUE_SEMAPHORE local_pSemaphore;
            handle = m_objMapper.remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenSharedQueueSemaphore(handle, pPacket->pOpenInfo, &local_pSemaphore);
            CHECK_RETURN_VALUE(xglOpenSharedQueueSemaphore);
            break;
        }
        case GLV_TPI_XGL_xglOpenPeerMemory:
        {
            struct_xglOpenPeerMemory* pPacket = (struct_xglOpenPeerMemory*)(packet->pBody);
            XGL_DEVICE handle;
            XGL_GPU_MEMORY local_pMem;
            handle = m_objMapper.remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenPeerMemory(handle, pPacket->pOpenInfo, &local_pMem);
            CHECK_RETURN_VALUE(xglOpenPeerMemory);
            break;
        }
        case GLV_TPI_XGL_xglOpenPeerImage:
        {
            struct_xglOpenPeerImage* pPacket = (struct_xglOpenPeerImage*)(packet->pBody);
            XGL_DEVICE handle;
            XGL_GPU_MEMORY local_pMem;
            XGL_IMAGE local_pImage;
            handle = m_objMapper.remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenPeerImage(handle, pPacket->pOpenInfo, &local_pImage, &local_pMem);
            CHECK_RETURN_VALUE(xglOpenPeerImage);
            break;
        }
        case GLV_TPI_XGL_xglDestroyObject:
        {
            struct_xglDestroyObject* pPacket = (struct_xglDestroyObject*)(packet->pBody);
            returnValue = manually_handle_xglDestroyObject(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglGetObjectInfo:
        {
            struct_xglGetObjectInfo* pPacket = (struct_xglGetObjectInfo*)(packet->pBody);
            returnValue = manually_handle_xglGetObjectInfo(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglBindObjectMemory:
        {
            struct_xglBindObjectMemory* pPacket = (struct_xglBindObjectMemory*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglBindObjectMemory(m_objMapper.remap(pPacket->object), pPacket->allocationIdx, m_objMapper.remap(pPacket->mem), pPacket->offset);
            CHECK_RETURN_VALUE(xglBindObjectMemory);
            break;
        }
        case GLV_TPI_XGL_xglBindObjectMemoryRange:
        {
            struct_xglBindObjectMemoryRange* pPacket = (struct_xglBindObjectMemoryRange*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglBindObjectMemoryRange(m_objMapper.remap(pPacket->object), pPacket->allocationIdx, pPacket->rangeOffset, pPacket->rangeSize, m_objMapper.remap(pPacket->mem), pPacket->memOffset);
            CHECK_RETURN_VALUE(xglBindObjectMemoryRange);
            break;
        }
        case GLV_TPI_XGL_xglBindImageMemoryRange:
        {
            struct_xglBindImageMemoryRange* pPacket = (struct_xglBindImageMemoryRange*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglBindImageMemoryRange(m_objMapper.remap(pPacket->image), pPacket->allocationIdx, pPacket->bindInfo, m_objMapper.remap(pPacket->mem), pPacket->memOffset);
            CHECK_RETURN_VALUE(xglBindImageMemoryRange);
            break;
        }
        case GLV_TPI_XGL_xglCreateFence:
        {
            struct_xglCreateFence* pPacket = (struct_xglCreateFence*)(packet->pBody);
            XGL_FENCE local_pFence;
            replayResult = m_xglFuncs.real_xglCreateFence(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pFence);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pFence, &local_pFence);
            }
            CHECK_RETURN_VALUE(xglCreateFence);
            break;
        }
        case GLV_TPI_XGL_xglGetFenceStatus:
        {
            struct_xglGetFenceStatus* pPacket = (struct_xglGetFenceStatus*)(packet->pBody);
            do {
                replayResult = m_xglFuncs.real_xglGetFenceStatus(m_objMapper.remap(pPacket->fence));
            } while (replayResult != pPacket->result  && pPacket->result == XGL_SUCCESS);
            if (pPacket->result != XGL_NOT_READY || replayResult != XGL_SUCCESS)
            CHECK_RETURN_VALUE(xglGetFenceStatus);
            break;
        }
        case GLV_TPI_XGL_xglWaitForFences:
        {
            struct_xglWaitForFences* pPacket = (struct_xglWaitForFences*)(packet->pBody);
            returnValue = manually_handle_xglWaitForFences(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglCreateQueueSemaphore:
        {
            struct_xglCreateQueueSemaphore* pPacket = (struct_xglCreateQueueSemaphore*)(packet->pBody);
            XGL_QUEUE_SEMAPHORE local_pSemaphore;
            replayResult = m_xglFuncs.real_xglCreateQueueSemaphore(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pSemaphore);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pSemaphore, &local_pSemaphore);
            }
            CHECK_RETURN_VALUE(xglCreateQueueSemaphore);
            break;
        }
        case GLV_TPI_XGL_xglSignalQueueSemaphore:
        {
            struct_xglSignalQueueSemaphore* pPacket = (struct_xglSignalQueueSemaphore*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSignalQueueSemaphore(m_objMapper.remap(pPacket->queue), m_objMapper.remap(pPacket->semaphore));
            CHECK_RETURN_VALUE(xglSignalQueueSemaphore);
            break;
        }
        case GLV_TPI_XGL_xglWaitQueueSemaphore:
        {
            struct_xglWaitQueueSemaphore* pPacket = (struct_xglWaitQueueSemaphore*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglWaitQueueSemaphore(m_objMapper.remap(pPacket->queue), m_objMapper.remap(pPacket->semaphore));
            CHECK_RETURN_VALUE(xglWaitQueueSemaphore);
            break;
        }
        case GLV_TPI_XGL_xglCreateEvent:
        {
            struct_xglCreateEvent* pPacket = (struct_xglCreateEvent*)(packet->pBody);
            XGL_EVENT local_pEvent;
            replayResult = m_xglFuncs.real_xglCreateEvent(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pEvent);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pEvent, &local_pEvent);
            }
            CHECK_RETURN_VALUE(xglCreateEvent);
            break;
        }
        case GLV_TPI_XGL_xglGetEventStatus:
        {
            struct_xglGetEventStatus* pPacket = (struct_xglGetEventStatus*)(packet->pBody);
            do {
                replayResult = m_xglFuncs.real_xglGetEventStatus(m_objMapper.remap(pPacket->event));
            } while ((pPacket->result == XGL_EVENT_SET || pPacket->result == XGL_EVENT_RESET) && replayResult != pPacket->result);
            if (pPacket->result != XGL_NOT_READY || replayResult != XGL_SUCCESS)
            CHECK_RETURN_VALUE(xglGetEventStatus);
            break;
        }
        case GLV_TPI_XGL_xglSetEvent:
        {
            struct_xglSetEvent* pPacket = (struct_xglSetEvent*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSetEvent(m_objMapper.remap(pPacket->event));
            CHECK_RETURN_VALUE(xglSetEvent);
            break;
        }
        case GLV_TPI_XGL_xglResetEvent:
        {
            struct_xglResetEvent* pPacket = (struct_xglResetEvent*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglResetEvent(m_objMapper.remap(pPacket->event));
            CHECK_RETURN_VALUE(xglResetEvent);
            break;
        }
        case GLV_TPI_XGL_xglCreateQueryPool:
        {
            struct_xglCreateQueryPool* pPacket = (struct_xglCreateQueryPool*)(packet->pBody);
            XGL_QUERY_POOL local_pQueryPool;
            replayResult = m_xglFuncs.real_xglCreateQueryPool(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pQueryPool);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pQueryPool, &local_pQueryPool);
            }
            CHECK_RETURN_VALUE(xglCreateQueryPool);
            break;
        }
        case GLV_TPI_XGL_xglGetQueryPoolResults:
        {
            struct_xglGetQueryPoolResults* pPacket = (struct_xglGetQueryPoolResults*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglGetQueryPoolResults(m_objMapper.remap(pPacket->queryPool), pPacket->startQuery, pPacket->queryCount, pPacket->pDataSize, pPacket->pData);
            CHECK_RETURN_VALUE(xglGetQueryPoolResults);
            break;
        }
        case GLV_TPI_XGL_xglGetFormatInfo:
        {
            struct_xglGetFormatInfo* pPacket = (struct_xglGetFormatInfo*)(packet->pBody);
            returnValue = manually_handle_xglGetFormatInfo(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglCreateBuffer:
        {
            struct_xglCreateBuffer* pPacket = (struct_xglCreateBuffer*)(packet->pBody);
            bufferObj local_bufferObj;
            replayResult = m_xglFuncs.real_xglCreateBuffer(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_bufferObj.replayBuffer);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pBuffer, &local_bufferObj);
            }
            CHECK_RETURN_VALUE(xglCreateBuffer);
            break;
        }
        case GLV_TPI_XGL_xglCreateBufferView:
        {
            struct_xglCreateBufferView* pPacket = (struct_xglCreateBufferView*)(packet->pBody);
            XGL_BUFFER_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_BUFFER_VIEW_CREATE_INFO));
            createInfo.buffer = m_objMapper.remap(pPacket->pCreateInfo->buffer);
            XGL_BUFFER_VIEW local_pView;
            replayResult = m_xglFuncs.real_xglCreateBufferView(m_objMapper.remap(pPacket->device), &createInfo, &local_pView);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pView, &local_pView);
            }
            CHECK_RETURN_VALUE(xglCreateBufferView);
            break;
        }
        case GLV_TPI_XGL_xglCreateImage:
        {
            struct_xglCreateImage* pPacket = (struct_xglCreateImage*)(packet->pBody);
            imageObj local_imageObj;
            replayResult = m_xglFuncs.real_xglCreateImage(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_imageObj.replayImage);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pImage, &local_imageObj);
            }
            CHECK_RETURN_VALUE(xglCreateImage);
            break;
        }
        case GLV_TPI_XGL_xglSetFastClearColor:
        {
            struct_xglSetFastClearColor* pPacket = (struct_xglSetFastClearColor*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSetFastClearColor(m_objMapper.remap(pPacket->image), pPacket->color);
            CHECK_RETURN_VALUE(xglSetFastClearColor);
            break;
        }
        case GLV_TPI_XGL_xglSetFastClearDepth:
        {
            struct_xglSetFastClearDepth* pPacket = (struct_xglSetFastClearDepth*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSetFastClearDepth(m_objMapper.remap(pPacket->image), pPacket->depth);
            CHECK_RETURN_VALUE(xglSetFastClearDepth);
            break;
        }
        case GLV_TPI_XGL_xglGetImageSubresourceInfo:
        {
            struct_xglGetImageSubresourceInfo* pPacket = (struct_xglGetImageSubresourceInfo*)(packet->pBody);
            returnValue = manually_handle_xglGetImageSubresourceInfo(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglCreateImageView:
        {
            struct_xglCreateImageView* pPacket = (struct_xglCreateImageView*)(packet->pBody);
            XGL_IMAGE_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_IMAGE_VIEW_CREATE_INFO));
            createInfo.image = m_objMapper.remap(pPacket->pCreateInfo->image);
            XGL_IMAGE_VIEW local_pView;
            replayResult = m_xglFuncs.real_xglCreateImageView(m_objMapper.remap(pPacket->device), &createInfo, &local_pView);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pView, &local_pView);
            }
            CHECK_RETURN_VALUE(xglCreateImageView);
            break;
        }
        case GLV_TPI_XGL_xglCreateColorAttachmentView:
        {
            struct_xglCreateColorAttachmentView* pPacket = (struct_xglCreateColorAttachmentView*)(packet->pBody);
            XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO));
            createInfo.image = m_objMapper.remap(pPacket->pCreateInfo->image);
            XGL_COLOR_ATTACHMENT_VIEW local_pView;
            replayResult = m_xglFuncs.real_xglCreateColorAttachmentView(m_objMapper.remap(pPacket->device), &createInfo, &local_pView);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pView, &local_pView);
            }
            CHECK_RETURN_VALUE(xglCreateColorAttachmentView);
            break;
        }
        case GLV_TPI_XGL_xglCreateDepthStencilView:
        {
            struct_xglCreateDepthStencilView* pPacket = (struct_xglCreateDepthStencilView*)(packet->pBody);
            XGL_DEPTH_STENCIL_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO));
            createInfo.image = m_objMapper.remap(pPacket->pCreateInfo->image);
            XGL_DEPTH_STENCIL_VIEW local_pView;
            replayResult = m_xglFuncs.real_xglCreateDepthStencilView(m_objMapper.remap(pPacket->device), &createInfo, &local_pView);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pView, &local_pView);
            }
            CHECK_RETURN_VALUE(xglCreateDepthStencilView);
            break;
        }
        case GLV_TPI_XGL_xglCreateShader:
        {
            struct_xglCreateShader* pPacket = (struct_xglCreateShader*)(packet->pBody);
            XGL_SHADER local_pShader;
            replayResult = m_xglFuncs.real_xglCreateShader(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pShader);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pShader, &local_pShader);
            }
            CHECK_RETURN_VALUE(xglCreateShader);
            break;
        }
        case GLV_TPI_XGL_xglCreateGraphicsPipeline:
        {
            struct_xglCreateGraphicsPipeline* pPacket = (struct_xglCreateGraphicsPipeline*)(packet->pBody);
            returnValue = manually_handle_xglCreateGraphicsPipeline(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglCreateComputePipeline:
        {
            struct_xglCreateComputePipeline* pPacket = (struct_xglCreateComputePipeline*)(packet->pBody);
            XGL_COMPUTE_PIPELINE_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO));
            createInfo.cs.shader = m_objMapper.remap(pPacket->pCreateInfo->cs.shader);
            XGL_PIPELINE local_pPipeline;
            replayResult = m_xglFuncs.real_xglCreateComputePipeline(m_objMapper.remap(pPacket->device), &createInfo, &local_pPipeline);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pPipeline, &local_pPipeline);
            }
            CHECK_RETURN_VALUE(xglCreateComputePipeline);
            break;
        }
        case GLV_TPI_XGL_xglStorePipeline:
        {
            struct_xglStorePipeline* pPacket = (struct_xglStorePipeline*)(packet->pBody);
            returnValue = manually_handle_xglStorePipeline(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglLoadPipeline:
        {
            struct_xglLoadPipeline* pPacket = (struct_xglLoadPipeline*)(packet->pBody);
            XGL_PIPELINE local_pPipeline;
            replayResult = m_xglFuncs.real_xglLoadPipeline(m_objMapper.remap(pPacket->device), pPacket->dataSize, pPacket->pData, &local_pPipeline);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pPipeline, &local_pPipeline);
            }
            CHECK_RETURN_VALUE(xglLoadPipeline);
            break;
        }
        case GLV_TPI_XGL_xglCreatePipelineDelta:
        {
            struct_xglCreatePipelineDelta* pPacket = (struct_xglCreatePipelineDelta*)(packet->pBody);
            XGL_PIPELINE_DELTA local_delta;
            replayResult = m_xglFuncs.real_xglCreatePipelineDelta(m_objMapper.remap(pPacket->device), pPacket->p1, pPacket->p2, &local_delta);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->delta, &local_delta);
            }
            CHECK_RETURN_VALUE(xglCreatePipelineDelta);
            break;
        }
        case GLV_TPI_XGL_xglCreateSampler:
        {
            struct_xglCreateSampler* pPacket = (struct_xglCreateSampler*)(packet->pBody);
            XGL_SAMPLER local_pSampler;
            replayResult = m_xglFuncs.real_xglCreateSampler(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pSampler);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pSampler, &local_pSampler);
            }
            CHECK_RETURN_VALUE(xglCreateSampler);
            break;
        }
        case GLV_TPI_XGL_xglCreateDescriptorSetLayout:
        {
            struct_xglCreateDescriptorSetLayout* pPacket = (struct_xglCreateDescriptorSetLayout*)(packet->pBody);
            returnValue = manually_handle_xglCreateDescriptorSetLayout(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglBeginDescriptorRegionUpdate:
        {
            struct_xglBeginDescriptorRegionUpdate* pPacket = (struct_xglBeginDescriptorRegionUpdate*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglBeginDescriptorRegionUpdate(m_objMapper.remap(pPacket->device), pPacket->updateMode);
            CHECK_RETURN_VALUE(xglBeginDescriptorRegionUpdate);
            break;
        }
        case GLV_TPI_XGL_xglEndDescriptorRegionUpdate:
        {
            struct_xglEndDescriptorRegionUpdate* pPacket = (struct_xglEndDescriptorRegionUpdate*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglEndDescriptorRegionUpdate(m_objMapper.remap(pPacket->device), m_objMapper.remap(pPacket->cmd));
            CHECK_RETURN_VALUE(xglEndDescriptorRegionUpdate);
            break;
        }
        case GLV_TPI_XGL_xglCreateDescriptorRegion:
        {
            struct_xglCreateDescriptorRegion* pPacket = (struct_xglCreateDescriptorRegion*)(packet->pBody);
            XGL_DESCRIPTOR_REGION local_pDescriptorRegion;
            replayResult = m_xglFuncs.real_xglCreateDescriptorRegion(m_objMapper.remap(pPacket->device), pPacket->regionUsage, pPacket->maxSets, pPacket->pCreateInfo, &local_pDescriptorRegion);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pDescriptorRegion, &local_pDescriptorRegion);
            }
            CHECK_RETURN_VALUE(xglCreateDescriptorRegion);
            break;
        }
        case GLV_TPI_XGL_xglClearDescriptorRegion:
        {
            struct_xglClearDescriptorRegion* pPacket = (struct_xglClearDescriptorRegion*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglClearDescriptorRegion(m_objMapper.remap(pPacket->descriptorRegion));
            CHECK_RETURN_VALUE(xglClearDescriptorRegion);
            break;
        }
        case GLV_TPI_XGL_xglAllocDescriptorSets:
        {
            struct_xglAllocDescriptorSets* pPacket = (struct_xglAllocDescriptorSets*)(packet->pBody);
            uint32_t local_pCount;
            XGL_DESCRIPTOR_SET local_pDescriptorSets[100];
            XGL_DESCRIPTOR_SET_LAYOUT localDescSets[100];
            assert(pPacket->count <= 100);
            for (uint32_t i = 0; i < pPacket->count; i++)
            {
                localDescSets[i] = m_objMapper.remap(pPacket->pSetLayouts[i]);
            }
            replayResult = m_xglFuncs.real_xglAllocDescriptorSets(m_objMapper.remap(pPacket->descriptorRegion), pPacket->setUsage, pPacket->count, localDescSets, local_pDescriptorSets, &local_pCount);
            if (replayResult == XGL_SUCCESS)
            {
                for (uint32_t i = 0; i < local_pCount; i++) {
                    m_objMapper.add_to_map(&pPacket->pDescriptorSets[i], &local_pDescriptorSets[i]);
                }
            }
            CHECK_RETURN_VALUE(xglAllocDescriptorSets);
            break;
        }
        case GLV_TPI_XGL_xglClearDescriptorSets:
        {
            struct_xglClearDescriptorSets* pPacket = (struct_xglClearDescriptorSets*)(packet->pBody);
            XGL_DESCRIPTOR_SET localDescSets[100];
            assert(pPacket->count <= 100);
            for (uint32_t i = 0; i < pPacket->count; i++)
            {
                localDescSets[i] = m_objMapper.remap(pPacket->pDescriptorSets[i]);
            }
            m_xglFuncs.real_xglClearDescriptorSets(m_objMapper.remap(pPacket->descriptorRegion), pPacket->count, localDescSets);
            break;
        }
        case GLV_TPI_XGL_xglUpdateDescriptors:
        {
            struct_xglUpdateDescriptors* pPacket = (struct_xglUpdateDescriptors*)(packet->pBody);
            returnValue = manually_handle_xglUpdateDescriptors(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglCreateDynamicViewportState:
        {
            struct_xglCreateDynamicViewportState* pPacket = (struct_xglCreateDynamicViewportState*)(packet->pBody);
            XGL_DYNAMIC_VP_STATE_OBJECT local_pState;
            replayResult = m_xglFuncs.real_xglCreateDynamicViewportState(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pState);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pState, &local_pState);
            }
            CHECK_RETURN_VALUE(xglCreateDynamicViewportState);
            break;
        }
        case GLV_TPI_XGL_xglCreateDynamicRasterState:
        {
            struct_xglCreateDynamicRasterState* pPacket = (struct_xglCreateDynamicRasterState*)(packet->pBody);
            XGL_DYNAMIC_RS_STATE_OBJECT local_pState;
            replayResult = m_xglFuncs.real_xglCreateDynamicRasterState(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pState);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pState, &local_pState);
            }
            CHECK_RETURN_VALUE(xglCreateDynamicRasterState);
            break;
        }
        case GLV_TPI_XGL_xglCreateDynamicColorBlendState:
        {
            struct_xglCreateDynamicColorBlendState* pPacket = (struct_xglCreateDynamicColorBlendState*)(packet->pBody);
            XGL_DYNAMIC_CB_STATE_OBJECT local_pState;
            replayResult = m_xglFuncs.real_xglCreateDynamicColorBlendState(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pState);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pState, &local_pState);
            }
            CHECK_RETURN_VALUE(xglCreateDynamicColorBlendState);
            break;
        }
        case GLV_TPI_XGL_xglCreateDynamicDepthStencilState:
        {
            struct_xglCreateDynamicDepthStencilState* pPacket = (struct_xglCreateDynamicDepthStencilState*)(packet->pBody);
            XGL_DYNAMIC_DS_STATE_OBJECT local_pState;
            replayResult = m_xglFuncs.real_xglCreateDynamicDepthStencilState(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pState);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pState, &local_pState);
            }
            CHECK_RETURN_VALUE(xglCreateDynamicDepthStencilState);
            break;
        }
        case GLV_TPI_XGL_xglCreateCommandBuffer:
        {
            struct_xglCreateCommandBuffer* pPacket = (struct_xglCreateCommandBuffer*)(packet->pBody);
            XGL_CMD_BUFFER local_pCmdBuffer;
            replayResult = m_xglFuncs.real_xglCreateCommandBuffer(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_pCmdBuffer);
            if (replayResult == XGL_SUCCESS)
            {
                m_objMapper.add_to_map(pPacket->pCmdBuffer, &local_pCmdBuffer);
            }
            CHECK_RETURN_VALUE(xglCreateCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglBeginCommandBuffer:
        {
            struct_xglBeginCommandBuffer* pPacket = (struct_xglBeginCommandBuffer*)(packet->pBody);
            returnValue = manually_handle_xglBeginCommandBuffer(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglEndCommandBuffer:
        {
            struct_xglEndCommandBuffer* pPacket = (struct_xglEndCommandBuffer*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglEndCommandBuffer(m_objMapper.remap(pPacket->cmdBuffer));
            CHECK_RETURN_VALUE(xglEndCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglResetCommandBuffer:
        {
            struct_xglResetCommandBuffer* pPacket = (struct_xglResetCommandBuffer*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglResetCommandBuffer(m_objMapper.remap(pPacket->cmdBuffer));
            CHECK_RETURN_VALUE(xglResetCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindPipeline:
        {
            struct_xglCmdBindPipeline* pPacket = (struct_xglCmdBindPipeline*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindPipeline(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, m_objMapper.remap(pPacket->pipeline));
            break;
        }
        case GLV_TPI_XGL_xglCmdBindPipelineDelta:
        {
            struct_xglCmdBindPipelineDelta* pPacket = (struct_xglCmdBindPipelineDelta*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindPipelineDelta(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, m_objMapper.remap(pPacket->delta));
            break;
        }
        case GLV_TPI_XGL_xglCmdBindDynamicStateObject:
        {
            struct_xglCmdBindDynamicStateObject* pPacket = (struct_xglCmdBindDynamicStateObject*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindDynamicStateObject(m_objMapper.remap(pPacket->cmdBuffer), pPacket->stateBindPoint, m_objMapper.remap(pPacket->state));
            break;
        }
        case GLV_TPI_XGL_xglCmdBindDescriptorSet:
        {
            struct_xglCmdBindDescriptorSet* pPacket = (struct_xglCmdBindDescriptorSet*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindDescriptorSet(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, m_objMapper.remap(pPacket->descriptorSet), pPacket->pUserData);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindVertexBuffer:
        {
            struct_xglCmdBindVertexBuffer* pPacket = (struct_xglCmdBindVertexBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindVertexBuffer(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->buffer), pPacket->offset, pPacket->binding);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindIndexBuffer:
        {
            struct_xglCmdBindIndexBuffer* pPacket = (struct_xglCmdBindIndexBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindIndexBuffer(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->buffer), pPacket->offset, pPacket->indexType);
            break;
        }
        case GLV_TPI_XGL_xglCmdDraw:
        {
            struct_xglCmdDraw* pPacket = (struct_xglCmdDraw*)(packet->pBody);
            m_xglFuncs.real_xglCmdDraw(m_objMapper.remap(pPacket->cmdBuffer), pPacket->firstVertex, pPacket->vertexCount, pPacket->firstInstance, pPacket->instanceCount);
            break;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexed:
        {
            struct_xglCmdDrawIndexed* pPacket = (struct_xglCmdDrawIndexed*)(packet->pBody);
            m_xglFuncs.real_xglCmdDrawIndexed(m_objMapper.remap(pPacket->cmdBuffer), pPacket->firstIndex, pPacket->indexCount, pPacket->vertexOffset, pPacket->firstInstance, pPacket->instanceCount);
            break;
        }
        case GLV_TPI_XGL_xglCmdDrawIndirect:
        {
            struct_xglCmdDrawIndirect* pPacket = (struct_xglCmdDrawIndirect*)(packet->pBody);
            m_xglFuncs.real_xglCmdDrawIndirect(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->buffer), pPacket->offset, pPacket->count, pPacket->stride);
            break;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexedIndirect:
        {
            struct_xglCmdDrawIndexedIndirect* pPacket = (struct_xglCmdDrawIndexedIndirect*)(packet->pBody);
            m_xglFuncs.real_xglCmdDrawIndexedIndirect(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->buffer), pPacket->offset, pPacket->count, pPacket->stride);
            break;
        }
        case GLV_TPI_XGL_xglCmdDispatch:
        {
            struct_xglCmdDispatch* pPacket = (struct_xglCmdDispatch*)(packet->pBody);
            m_xglFuncs.real_xglCmdDispatch(m_objMapper.remap(pPacket->cmdBuffer), pPacket->x, pPacket->y, pPacket->z);
            break;
        }
        case GLV_TPI_XGL_xglCmdDispatchIndirect:
        {
            struct_xglCmdDispatchIndirect* pPacket = (struct_xglCmdDispatchIndirect*)(packet->pBody);
            m_xglFuncs.real_xglCmdDispatchIndirect(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->buffer), pPacket->offset);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyBuffer:
        {
            struct_xglCmdCopyBuffer* pPacket = (struct_xglCmdCopyBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdCopyBuffer(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->srcBuffer), m_objMapper.remap(pPacket->destBuffer), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyImage:
        {
            struct_xglCmdCopyImage* pPacket = (struct_xglCmdCopyImage*)(packet->pBody);
            m_xglFuncs.real_xglCmdCopyImage(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->srcImage), m_objMapper.remap(pPacket->destImage), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyBufferToImage:
        {
            struct_xglCmdCopyBufferToImage* pPacket = (struct_xglCmdCopyBufferToImage*)(packet->pBody);
            m_xglFuncs.real_xglCmdCopyBufferToImage(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->srcBuffer), m_objMapper.remap(pPacket->destImage), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyImageToBuffer:
        {
            struct_xglCmdCopyImageToBuffer* pPacket = (struct_xglCmdCopyImageToBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdCopyImageToBuffer(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->srcImage), m_objMapper.remap(pPacket->destBuffer), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCloneImageData:
        {
            struct_xglCmdCloneImageData* pPacket = (struct_xglCmdCloneImageData*)(packet->pBody);
            m_xglFuncs.real_xglCmdCloneImageData(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->srcImage), pPacket->srcImageLayout, m_objMapper.remap(pPacket->destImage), pPacket->destImageLayout);
            break;
        }
        case GLV_TPI_XGL_xglCmdUpdateBuffer:
        {
            struct_xglCmdUpdateBuffer* pPacket = (struct_xglCmdUpdateBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdUpdateBuffer(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->destBuffer), pPacket->destOffset, pPacket->dataSize, pPacket->pData);
            break;
        }
        case GLV_TPI_XGL_xglCmdFillBuffer:
        {
            struct_xglCmdFillBuffer* pPacket = (struct_xglCmdFillBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdFillBuffer(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->destBuffer), pPacket->destOffset, pPacket->fillSize, pPacket->data);
            break;
        }
        case GLV_TPI_XGL_xglCmdClearColorImage:
        {
            struct_xglCmdClearColorImage* pPacket = (struct_xglCmdClearColorImage*)(packet->pBody);
            m_xglFuncs.real_xglCmdClearColorImage(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->image), pPacket->color, pPacket->rangeCount, pPacket->pRanges);
            break;
        }
        case GLV_TPI_XGL_xglCmdClearColorImageRaw:
        {
            struct_xglCmdClearColorImageRaw* pPacket = (struct_xglCmdClearColorImageRaw*)(packet->pBody);
            m_xglFuncs.real_xglCmdClearColorImageRaw(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->image), pPacket->color, pPacket->rangeCount, pPacket->pRanges);
            break;
        }
        case GLV_TPI_XGL_xglCmdClearDepthStencil:
        {
            struct_xglCmdClearDepthStencil* pPacket = (struct_xglCmdClearDepthStencil*)(packet->pBody);
            m_xglFuncs.real_xglCmdClearDepthStencil(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->image), pPacket->depth, pPacket->stencil, pPacket->rangeCount, pPacket->pRanges);
            break;
        }
        case GLV_TPI_XGL_xglCmdResolveImage:
        {
            struct_xglCmdResolveImage* pPacket = (struct_xglCmdResolveImage*)(packet->pBody);
            m_xglFuncs.real_xglCmdResolveImage(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->srcImage), m_objMapper.remap(pPacket->destImage), pPacket->rectCount, pPacket->pRects);
            break;
        }
        case GLV_TPI_XGL_xglCmdSetEvent:
        {
            struct_xglCmdSetEvent* pPacket = (struct_xglCmdSetEvent*)(packet->pBody);
            m_xglFuncs.real_xglCmdSetEvent(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->event), pPacket->pipeEvent);
            break;
        }
        case GLV_TPI_XGL_xglCmdResetEvent:
        {
            struct_xglCmdResetEvent* pPacket = (struct_xglCmdResetEvent*)(packet->pBody);
            m_xglFuncs.real_xglCmdResetEvent(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->event));
            break;
        }
        case GLV_TPI_XGL_xglCmdWaitEvents:
        {
            struct_xglCmdWaitEvents* pPacket = (struct_xglCmdWaitEvents*)(packet->pBody);
            returnValue = manually_handle_xglCmdWaitEvents(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglCmdPipelineBarrier:
        {
            struct_xglCmdPipelineBarrier* pPacket = (struct_xglCmdPipelineBarrier*)(packet->pBody);
            returnValue = manually_handle_xglCmdPipelineBarrier(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglCmdBeginQuery:
        {
            struct_xglCmdBeginQuery* pPacket = (struct_xglCmdBeginQuery*)(packet->pBody);
            m_xglFuncs.real_xglCmdBeginQuery(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->queryPool), pPacket->slot, pPacket->flags);
            break;
        }
        case GLV_TPI_XGL_xglCmdEndQuery:
        {
            struct_xglCmdEndQuery* pPacket = (struct_xglCmdEndQuery*)(packet->pBody);
            m_xglFuncs.real_xglCmdEndQuery(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->queryPool), pPacket->slot);
            break;
        }
        case GLV_TPI_XGL_xglCmdResetQueryPool:
        {
            struct_xglCmdResetQueryPool* pPacket = (struct_xglCmdResetQueryPool*)(packet->pBody);
            m_xglFuncs.real_xglCmdResetQueryPool(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->queryPool), pPacket->startQuery, pPacket->queryCount);
            break;
        }
        case GLV_TPI_XGL_xglCmdWriteTimestamp:
        {
            struct_xglCmdWriteTimestamp* pPacket = (struct_xglCmdWriteTimestamp*)(packet->pBody);
            m_xglFuncs.real_xglCmdWriteTimestamp(m_objMapper.remap(pPacket->cmdBuffer), pPacket->timestampType, m_objMapper.remap(pPacket->destBuffer), pPacket->destOffset);
            break;
        }
        case GLV_TPI_XGL_xglCmdInitAtomicCounters:
        {
            struct_xglCmdInitAtomicCounters* pPacket = (struct_xglCmdInitAtomicCounters*)(packet->pBody);
            m_xglFuncs.real_xglCmdInitAtomicCounters(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->startCounter, pPacket->counterCount, pPacket->pData);
            break;
        }
        case GLV_TPI_XGL_xglCmdLoadAtomicCounters:
        {
            struct_xglCmdLoadAtomicCounters* pPacket = (struct_xglCmdLoadAtomicCounters*)(packet->pBody);
            m_xglFuncs.real_xglCmdLoadAtomicCounters(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->startCounter, pPacket->counterCount, m_objMapper.remap(pPacket->srcBuffer), pPacket->srcOffset);
            break;
        }
        case GLV_TPI_XGL_xglCmdSaveAtomicCounters:
        {
            struct_xglCmdSaveAtomicCounters* pPacket = (struct_xglCmdSaveAtomicCounters*)(packet->pBody);
            m_xglFuncs.real_xglCmdSaveAtomicCounters(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->startCounter, pPacket->counterCount, m_objMapper.remap(pPacket->destBuffer), pPacket->destOffset);
            break;
        }
        case GLV_TPI_XGL_xglCreateFramebuffer:
        {
            struct_xglCreateFramebuffer* pPacket = (struct_xglCreateFramebuffer*)(packet->pBody);
            returnValue = manually_handle_xglCreateFramebuffer(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglCreateRenderPass:
        {
            struct_xglCreateRenderPass* pPacket = (struct_xglCreateRenderPass*)(packet->pBody);
            returnValue = manually_handle_xglCreateRenderPass(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglCmdBeginRenderPass:
        {
            struct_xglCmdBeginRenderPass* pPacket = (struct_xglCmdBeginRenderPass*)(packet->pBody);
            m_xglFuncs.real_xglCmdBeginRenderPass(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->renderPass));
            break;
        }
        case GLV_TPI_XGL_xglCmdEndRenderPass:
        {
            struct_xglCmdEndRenderPass* pPacket = (struct_xglCmdEndRenderPass*)(packet->pBody);
            m_xglFuncs.real_xglCmdEndRenderPass(m_objMapper.remap(pPacket->cmdBuffer), m_objMapper.remap(pPacket->renderPass));
            break;
        }
        case GLV_TPI_XGL_xglDbgSetValidationLevel:
        {
            struct_xglDbgSetValidationLevel* pPacket = (struct_xglDbgSetValidationLevel*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetValidationLevel(m_objMapper.remap(pPacket->device), pPacket->validationLevel);
            CHECK_RETURN_VALUE(xglDbgSetValidationLevel);
            break;
        }
        case GLV_TPI_XGL_xglDbgRegisterMsgCallback:
        {
            // Just eating these calls as no way to restore dbg func ptr.
            break;
        }
        case GLV_TPI_XGL_xglDbgUnregisterMsgCallback:
        {
            // Just eating these calls as no way to restore dbg func ptr.
            break;
        }
        case GLV_TPI_XGL_xglDbgSetMessageFilter:
        {
            struct_xglDbgSetMessageFilter* pPacket = (struct_xglDbgSetMessageFilter*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetMessageFilter(m_objMapper.remap(pPacket->device), pPacket->msgCode, pPacket->filter);
            CHECK_RETURN_VALUE(xglDbgSetMessageFilter);
            break;
        }
        case GLV_TPI_XGL_xglDbgSetObjectTag:
        {
            struct_xglDbgSetObjectTag* pPacket = (struct_xglDbgSetObjectTag*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetObjectTag(m_objMapper.remap(pPacket->object), pPacket->tagSize, pPacket->pTag);
            CHECK_RETURN_VALUE(xglDbgSetObjectTag);
            break;
        }
        case GLV_TPI_XGL_xglDbgSetGlobalOption:
        {
            struct_xglDbgSetGlobalOption* pPacket = (struct_xglDbgSetGlobalOption*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetGlobalOption(pPacket->dbgOption, pPacket->dataSize, pPacket->pData);
            CHECK_RETURN_VALUE(xglDbgSetGlobalOption);
            break;
        }
        case GLV_TPI_XGL_xglDbgSetDeviceOption:
        {
            struct_xglDbgSetDeviceOption* pPacket = (struct_xglDbgSetDeviceOption*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetDeviceOption(m_objMapper.remap(pPacket->device), pPacket->dbgOption, pPacket->dataSize, pPacket->pData);
            CHECK_RETURN_VALUE(xglDbgSetDeviceOption);
            break;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerBegin:
        {
            struct_xglCmdDbgMarkerBegin* pPacket = (struct_xglCmdDbgMarkerBegin*)(packet->pBody);
            m_xglFuncs.real_xglCmdDbgMarkerBegin(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pMarker);
            break;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerEnd:
        {
            struct_xglCmdDbgMarkerEnd* pPacket = (struct_xglCmdDbgMarkerEnd*)(packet->pBody);
            m_xglFuncs.real_xglCmdDbgMarkerEnd(m_objMapper.remap(pPacket->cmdBuffer));
            break;
        }
        case GLV_TPI_XGL_xglWsiX11AssociateConnection:
        {
            struct_xglWsiX11AssociateConnection* pPacket = (struct_xglWsiX11AssociateConnection*)(packet->pBody);
            returnValue = manually_handle_xglWsiX11AssociateConnection(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglWsiX11GetMSC:
        {
            struct_xglWsiX11GetMSC* pPacket = (struct_xglWsiX11GetMSC*)(packet->pBody);
            returnValue = manually_handle_xglWsiX11GetMSC(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglWsiX11CreatePresentableImage:
        {
            struct_xglWsiX11CreatePresentableImage* pPacket = (struct_xglWsiX11CreatePresentableImage*)(packet->pBody);
            returnValue = manually_handle_xglWsiX11CreatePresentableImage(pPacket);
            break;
        }
        case GLV_TPI_XGL_xglWsiX11QueuePresent:
        {
            struct_xglWsiX11QueuePresent* pPacket = (struct_xglWsiX11QueuePresent*)(packet->pBody);
            returnValue = manually_handle_xglWsiX11QueuePresent(pPacket);
            break;
        }
        default:
            glv_LogWarn("Unrecognized packet_id %u, skipping\n", packet->packet_id);
            returnValue = glv_replay::GLV_REPLAY_INVALID_ID;
            break;
    }
    return returnValue;
}
