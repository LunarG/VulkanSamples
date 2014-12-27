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
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "icd.h"
#include "cmd.h"
#include "dev.h"
#include "dset.h"
#include "event.h"
#include "fence.h"
#include "format.h"
#include "img.h"
#include "gpu.h"
#include "mem.h"
#include "obj.h"
#include "pipeline.h"
#include "query.h"
#include "queue.h"
#include "sampler.h"
#include "shader.h"
#include "state.h"
#include "view.h"
#include "wsi_x11.h"
#include "dispatch.h"

static const XGL_LAYER_DISPATCH_TABLE intel_normal_dispatch_table = {
    .GetProcAddr = intelGetProcAddr,
    .InitAndEnumerateGpus = xglInitAndEnumerateGpus,
    .GetGpuInfo = intelGetGpuInfo,
    .CreateDevice = intelCreateDevice,
    .DestroyDevice = intelDestroyDevice,
    .GetExtensionSupport = intelGetExtensionSupport,
    .EnumerateLayers = NULL,
    .GetDeviceQueue = intelGetDeviceQueue,
    .QueueSubmit = intelQueueSubmit,
    .QueueSetGlobalMemReferences = intelQueueSetGlobalMemReferences,
    .QueueWaitIdle = intelQueueWaitIdle,
    .DeviceWaitIdle = intelDeviceWaitIdle,
    .GetMemoryHeapCount = intelGetMemoryHeapCount,
    .GetMemoryHeapInfo = intelGetMemoryHeapInfo,
    .AllocMemory = intelAllocMemory,
    .FreeMemory = intelFreeMemory,
    .SetMemoryPriority = intelSetMemoryPriority,
    .MapMemory = intelMapMemory,
    .UnmapMemory = intelUnmapMemory,
    .PinSystemMemory = intelPinSystemMemory,
    .RemapVirtualMemoryPages = intelRemapVirtualMemoryPages,
    .GetMultiGpuCompatibility = intelGetMultiGpuCompatibility,
    .OpenSharedMemory = intelOpenSharedMemory,
    .OpenSharedQueueSemaphore = intelOpenSharedQueueSemaphore,
    .OpenPeerMemory = intelOpenPeerMemory,
    .OpenPeerImage = intelOpenPeerImage,
    .DestroyObject = intelDestroyObject,
    .GetObjectInfo = intelGetObjectInfo,
    .BindObjectMemory = intelBindObjectMemory,
    .CreateFence = intelCreateFence,
    .GetFenceStatus = intelGetFenceStatus,
    .WaitForFences = intelWaitForFences,
    .CreateQueueSemaphore = intelCreateQueueSemaphore,
    .SignalQueueSemaphore = intelSignalQueueSemaphore,
    .WaitQueueSemaphore = intelWaitQueueSemaphore,
    .CreateEvent = intelCreateEvent,
    .GetEventStatus = intelGetEventStatus,
    .SetEvent = intelSetEvent,
    .ResetEvent = intelResetEvent,
    .CreateQueryPool = intelCreateQueryPool,
    .GetQueryPoolResults = intelGetQueryPoolResults,
    .GetFormatInfo = intelGetFormatInfo,
    .CreateImage = intelCreateImage,
    .GetImageSubresourceInfo = intelGetImageSubresourceInfo,
    .CreateImageView = intelCreateImageView,
    .CreateColorAttachmentView = intelCreateColorAttachmentView,
    .CreateDepthStencilView = intelCreateDepthStencilView,
    .CreateShader = intelCreateShader,
    .CreateGraphicsPipeline = intelCreateGraphicsPipeline,
    .CreateComputePipeline = intelCreateComputePipeline,
    .StorePipeline = intelStorePipeline,
    .LoadPipeline = intelLoadPipeline,
    .CreatePipelineDelta = intelCreatePipelineDelta,
    .CreateSampler = intelCreateSampler,
    .CreateDescriptorSet = intelCreateDescriptorSet,
    .BeginDescriptorSetUpdate = intelBeginDescriptorSetUpdate,
    .EndDescriptorSetUpdate = intelEndDescriptorSetUpdate,
    .AttachSamplerDescriptors = intelAttachSamplerDescriptors,
    .AttachImageViewDescriptors = intelAttachImageViewDescriptors,
    .AttachMemoryViewDescriptors = intelAttachMemoryViewDescriptors,
    .AttachNestedDescriptors = intelAttachNestedDescriptors,
    .ClearDescriptorSetSlots = intelClearDescriptorSetSlots,
    .CreateViewportState = intelCreateViewportState,
    .CreateRasterState = intelCreateRasterState,
    .CreateMsaaState = intelCreateMsaaState,
    .CreateColorBlendState = intelCreateColorBlendState,
    .CreateDepthStencilState = intelCreateDepthStencilState,
    .CreateCommandBuffer = intelCreateCommandBuffer,
    .BeginCommandBuffer = intelBeginCommandBuffer,
    .EndCommandBuffer = intelEndCommandBuffer,
    .ResetCommandBuffer = intelResetCommandBuffer,
    .CmdBindPipeline = intelCmdBindPipeline,
    .CmdBindPipelineDelta = intelCmdBindPipelineDelta,
    .CmdBindStateObject = intelCmdBindStateObject,
    .CmdBindDescriptorSet = intelCmdBindDescriptorSet,
    .CmdBindDynamicMemoryView = intelCmdBindDynamicMemoryView,
    .CmdBindVertexData = intelCmdBindVertexData,
    .CmdBindIndexData = intelCmdBindIndexData,
    .CmdBindAttachments = intelCmdBindAttachments,
    .CmdPrepareMemoryRegions = intelCmdPrepareMemoryRegions,
    .CmdPrepareImages = intelCmdPrepareImages,
    .CmdDraw = intelCmdDraw,
    .CmdDrawIndexed = intelCmdDrawIndexed,
    .CmdDrawIndirect = intelCmdDrawIndirect,
    .CmdDrawIndexedIndirect = intelCmdDrawIndexedIndirect,
    .CmdDispatch = intelCmdDispatch,
    .CmdDispatchIndirect = intelCmdDispatchIndirect,
    .CmdCopyMemory = intelCmdCopyMemory,
    .CmdCopyImage = intelCmdCopyImage,
    .CmdCopyMemoryToImage = intelCmdCopyMemoryToImage,
    .CmdCopyImageToMemory = intelCmdCopyImageToMemory,
    .CmdCloneImageData = intelCmdCloneImageData,
    .CmdUpdateMemory = intelCmdUpdateMemory,
    .CmdFillMemory = intelCmdFillMemory,
    .CmdClearColorImage = intelCmdClearColorImage,
    .CmdClearColorImageRaw = intelCmdClearColorImageRaw,
    .CmdClearDepthStencil = intelCmdClearDepthStencil,
    .CmdResolveImage = intelCmdResolveImage,
    .CmdSetEvent = intelCmdSetEvent,
    .CmdResetEvent = intelCmdResetEvent,
    .CmdMemoryAtomic = intelCmdMemoryAtomic,
    .CmdBeginQuery = intelCmdBeginQuery,
    .CmdEndQuery = intelCmdEndQuery,
    .CmdResetQueryPool = intelCmdResetQueryPool,
    .CmdWriteTimestamp = intelCmdWriteTimestamp,
    .CmdInitAtomicCounters = intelCmdInitAtomicCounters,
    .CmdLoadAtomicCounters = intelCmdLoadAtomicCounters,
    .CmdSaveAtomicCounters = intelCmdSaveAtomicCounters,
    .DbgSetValidationLevel = intelDbgSetValidationLevel,
    .DbgRegisterMsgCallback = icdDbgRegisterMsgCallback,
    .DbgUnregisterMsgCallback = icdDbgUnregisterMsgCallback,
    .DbgSetMessageFilter = intelDbgSetMessageFilter,
    .DbgSetObjectTag = intelDbgSetObjectTag,
    .DbgSetGlobalOption = icdDbgSetGlobalOption,
    .DbgSetDeviceOption = intelDbgSetDeviceOption,
    .CmdDbgMarkerBegin = intelCmdDbgMarkerBegin,
    .CmdDbgMarkerEnd = intelCmdDbgMarkerEnd,
};

static const XGL_LAYER_DISPATCH_TABLE intel_debug_dispatch_table = {
    .GetProcAddr = intelGetProcAddr,
    .InitAndEnumerateGpus = xglInitAndEnumerateGpus,
    .GetGpuInfo = intelGetGpuInfo,
    .CreateDevice = intelCreateDevice,
    .DestroyDevice = intelDestroyDevice,
    .GetExtensionSupport = intelGetExtensionSupport,
    .EnumerateLayers = NULL,
    .GetDeviceQueue = intelGetDeviceQueue,
    .QueueSubmit = intelQueueSubmit,
    .QueueSetGlobalMemReferences = intelQueueSetGlobalMemReferences,
    .QueueWaitIdle = intelQueueWaitIdle,
    .DeviceWaitIdle = intelDeviceWaitIdle,
    .GetMemoryHeapCount = intelGetMemoryHeapCount,
    .GetMemoryHeapInfo = intelGetMemoryHeapInfo,
    .AllocMemory = intelAllocMemory,
    .FreeMemory = intelFreeMemory,
    .SetMemoryPriority = intelSetMemoryPriority,
    .MapMemory = intelMapMemory,
    .UnmapMemory = intelUnmapMemory,
    .PinSystemMemory = intelPinSystemMemory,
    .RemapVirtualMemoryPages = intelRemapVirtualMemoryPages,
    .GetMultiGpuCompatibility = intelGetMultiGpuCompatibility,
    .OpenSharedMemory = intelOpenSharedMemory,
    .OpenSharedQueueSemaphore = intelOpenSharedQueueSemaphore,
    .OpenPeerMemory = intelOpenPeerMemory,
    .OpenPeerImage = intelOpenPeerImage,
    .DestroyObject = intelDestroyObject,
    .GetObjectInfo = intelGetObjectInfo,
    .BindObjectMemory = intelBindObjectMemory,
    .CreateFence = intelCreateFence,
    .GetFenceStatus = intelGetFenceStatus,
    .WaitForFences = intelWaitForFences,
    .CreateQueueSemaphore = intelCreateQueueSemaphore,
    .SignalQueueSemaphore = intelSignalQueueSemaphore,
    .WaitQueueSemaphore = intelWaitQueueSemaphore,
    .CreateEvent = intelCreateEvent,
    .GetEventStatus = intelGetEventStatus,
    .SetEvent = intelSetEvent,
    .ResetEvent = intelResetEvent,
    .CreateQueryPool = intelCreateQueryPool,
    .GetQueryPoolResults = intelGetQueryPoolResults,
    .GetFormatInfo = intelGetFormatInfo,
    .CreateImage = intelCreateImage,
    .GetImageSubresourceInfo = intelGetImageSubresourceInfo,
    .CreateImageView = intelCreateImageView,
    .CreateColorAttachmentView = intelCreateColorAttachmentView,
    .CreateDepthStencilView = intelCreateDepthStencilView,
    .CreateShader = intelCreateShader,
    .CreateGraphicsPipeline = intelCreateGraphicsPipeline,
    .CreateComputePipeline = intelCreateComputePipeline,
    .StorePipeline = intelStorePipeline,
    .LoadPipeline = intelLoadPipeline,
    .CreatePipelineDelta = intelCreatePipelineDelta,
    .CreateSampler = intelCreateSampler,
    .CreateDescriptorSet = intelCreateDescriptorSet,
    .BeginDescriptorSetUpdate = intelBeginDescriptorSetUpdate,
    .EndDescriptorSetUpdate = intelEndDescriptorSetUpdate,
    .AttachSamplerDescriptors = intelAttachSamplerDescriptors,
    .AttachImageViewDescriptors = intelAttachImageViewDescriptors,
    .AttachMemoryViewDescriptors = intelAttachMemoryViewDescriptors,
    .AttachNestedDescriptors = intelAttachNestedDescriptors,
    .ClearDescriptorSetSlots = intelClearDescriptorSetSlots,
    .CreateViewportState = intelCreateViewportState,
    .CreateRasterState = intelCreateRasterState,
    .CreateMsaaState = intelCreateMsaaState,
    .CreateColorBlendState = intelCreateColorBlendState,
    .CreateDepthStencilState = intelCreateDepthStencilState,
    .CreateCommandBuffer = intelCreateCommandBuffer,
    .BeginCommandBuffer = intelBeginCommandBuffer,
    .EndCommandBuffer = intelEndCommandBuffer,
    .ResetCommandBuffer = intelResetCommandBuffer,
    .CmdBindPipeline = intelCmdBindPipeline,
    .CmdBindPipelineDelta = intelCmdBindPipelineDelta,
    .CmdBindStateObject = intelCmdBindStateObject,
    .CmdBindDescriptorSet = intelCmdBindDescriptorSet,
    .CmdBindDynamicMemoryView = intelCmdBindDynamicMemoryView,
    .CmdBindVertexData = intelCmdBindVertexData,
    .CmdBindIndexData = intelCmdBindIndexData,
    .CmdBindAttachments = intelCmdBindAttachments,
    .CmdPrepareMemoryRegions = intelCmdPrepareMemoryRegions,
    .CmdPrepareImages = intelCmdPrepareImages,
    .CmdDraw = intelCmdDraw,
    .CmdDrawIndexed = intelCmdDrawIndexed,
    .CmdDrawIndirect = intelCmdDrawIndirect,
    .CmdDrawIndexedIndirect = intelCmdDrawIndexedIndirect,
    .CmdDispatch = intelCmdDispatch,
    .CmdDispatchIndirect = intelCmdDispatchIndirect,
    .CmdCopyMemory = intelCmdCopyMemory,
    .CmdCopyImage = intelCmdCopyImage,
    .CmdCopyMemoryToImage = intelCmdCopyMemoryToImage,
    .CmdCopyImageToMemory = intelCmdCopyImageToMemory,
    .CmdCloneImageData = intelCmdCloneImageData,
    .CmdUpdateMemory = intelCmdUpdateMemory,
    .CmdFillMemory = intelCmdFillMemory,
    .CmdClearColorImage = intelCmdClearColorImage,
    .CmdClearColorImageRaw = intelCmdClearColorImageRaw,
    .CmdClearDepthStencil = intelCmdClearDepthStencil,
    .CmdResolveImage = intelCmdResolveImage,
    .CmdSetEvent = intelCmdSetEvent,
    .CmdResetEvent = intelCmdResetEvent,
    .CmdMemoryAtomic = intelCmdMemoryAtomic,
    .CmdBeginQuery = intelCmdBeginQuery,
    .CmdEndQuery = intelCmdEndQuery,
    .CmdResetQueryPool = intelCmdResetQueryPool,
    .CmdWriteTimestamp = intelCmdWriteTimestamp,
    .CmdInitAtomicCounters = intelCmdInitAtomicCounters,
    .CmdLoadAtomicCounters = intelCmdLoadAtomicCounters,
    .CmdSaveAtomicCounters = intelCmdSaveAtomicCounters,
    .DbgSetValidationLevel = intelDbgSetValidationLevel,
    .DbgRegisterMsgCallback = icdDbgRegisterMsgCallback,
    .DbgUnregisterMsgCallback = icdDbgUnregisterMsgCallback,
    .DbgSetMessageFilter = intelDbgSetMessageFilter,
    .DbgSetObjectTag = intelDbgSetObjectTag,
    .DbgSetGlobalOption = icdDbgSetGlobalOption,
    .DbgSetDeviceOption = intelDbgSetDeviceOption,
    .CmdDbgMarkerBegin = intelCmdDbgMarkerBegin,
    .CmdDbgMarkerEnd = intelCmdDbgMarkerEnd,
    .WsiX11AssociateConnection = intelWsiX11AssociateConnection,
    .WsiX11GetMSC = intelWsiX11GetMSC,
    .WsiX11CreatePresentableImage = intelWsiX11CreatePresentableImage,
    .WsiX11QueuePresent = intelWsiX11QueuePresent,
};

static XGL_LAYER_DISPATCH_TABLE *debug_dispatch = (XGL_LAYER_DISPATCH_TABLE *) &intel_debug_dispatch_table;
static XGL_LAYER_DISPATCH_TABLE *normal_dispatch = (XGL_LAYER_DISPATCH_TABLE *) &intel_normal_dispatch_table;

const XGL_LAYER_DISPATCH_TABLE *intel_dispatch_get(bool debug)
{

    return (debug) ? debug_dispatch : normal_dispatch;
}

XGL_VOID * intelGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR * pName)
{
    const XGL_LAYER_DISPATCH_TABLE *disp_table = * (const XGL_LAYER_DISPATCH_TABLE * const *) gpu;

   if (!strncmp("xglGetProcAddr", pName, sizeof("xglGetProcAddr")))
        return disp_table->GetProcAddr;
    else if (!strncmp("xglInitAndEnumerateGpus", pName, sizeof("xglInitAndEnumerateGpus")))
        return disp_table->InitAndEnumerateGpus;
    if (!strncmp("xglGetGpuInfo", pName, sizeof ("xglGetGpuInfo")))
        return disp_table->GetGpuInfo;
    else if (!strncmp("xglCreateDevice", pName, sizeof ("xglCreateDevice")))
        return disp_table->CreateDevice;
    else if (!strncmp("xglDestroyDevice", pName, sizeof ("xglDestroyDevice")))
        return disp_table->DestroyDevice;
    else if (!strncmp("xglGetExtensionSupport", pName, sizeof ("xglGetExtensionSupport")))
        return disp_table->GetExtensionSupport;
    else if (!strncmp("xglGetDeviceQueue", pName, sizeof ("xglGetDeviceQueue")))
        return disp_table->GetDeviceQueue;
    else if (!strncmp("xglQueueSubmit", pName, sizeof ("xglQueueSubmit")))
        return disp_table->QueueSubmit;
    else if (!strncmp("xglQueueSetGlobalMemReferences", pName, sizeof ("xglQueueSetGlobalMemReferences")))
        return disp_table->QueueSetGlobalMemReferences;
    else if (!strncmp("xglQueueWaitIdle", pName, sizeof ("xglQueueWaitIdle")))
        return disp_table->QueueWaitIdle;
    else if (!strncmp("xglDeviceWaitIdle", pName, sizeof ("xglDeviceWaitIdle")))
        return disp_table->DeviceWaitIdle;
    else if (!strncmp("xglGetMemoryHeapCount", pName, sizeof ("xglGetMemoryHeapCount")))
        return disp_table->GetMemoryHeapCount;
    else if (!strncmp("xglGetMemoryHeapInfo", pName, sizeof ("xglGetMemoryHeapInfo")))
        return disp_table->GetMemoryHeapInfo;
    else if (!strncmp("xglAllocMemory", pName, sizeof ("xglAllocMemory")))
        return disp_table->AllocMemory;
    else if (!strncmp("xglFreeMemory", pName, sizeof ("xglFreeMemory")))
        return disp_table->FreeMemory;
    else if (!strncmp("xglSetMemoryPriority", pName, sizeof ("xglSetMemoryPriority")))
        return disp_table->SetMemoryPriority;
    else if (!strncmp("xglMapMemory", pName, sizeof ("xglMapMemory")))
        return disp_table->MapMemory;
    else if (!strncmp("xglUnmapMemory", pName, sizeof ("xglUnmapMemory")))
        return disp_table->UnmapMemory;
    else if (!strncmp("xglPinSystemMemory", pName, sizeof ("xglPinSystemMemory")))
        return disp_table->PinSystemMemory;
    else if (!strncmp("xglRemapVirtualMemoryPages", pName, sizeof ("xglRemapVirtualMemoryPages")))
        return disp_table->RemapVirtualMemoryPages;
    else if (!strncmp("xglGetMultiGpuCompatibility", pName, sizeof ("xglGetMultiGpuCompatibility")))
        return disp_table->GetMultiGpuCompatibility;
    else if (!strncmp("xglOpenSharedMemory", pName, sizeof ("xglOpenSharedMemory")))
        return disp_table->OpenSharedMemory;
    else if (!strncmp("xglOpenSharedQueueSemaphore", pName, sizeof ("xglOpenSharedQueueSemaphore")))
        return disp_table->OpenSharedQueueSemaphore;
    else if (!strncmp("xglOpenPeerMemory", pName, sizeof ("xglOpenPeerMemory")))
        return disp_table->OpenPeerMemory;
    else if (!strncmp("xglOpenPeerImage", pName, sizeof ("xglOpenPeerImage")))
        return disp_table->OpenPeerImage;
    else if (!strncmp("xglDestroyObject", pName, sizeof ("xglDestroyObject")))
        return disp_table->DestroyObject;
    else if (!strncmp("xglGetObjectInfo", pName, sizeof ("xglGetObjectInfo")))
        return disp_table->GetObjectInfo;
    else if (!strncmp("xglBindObjectMemory", pName, sizeof ("xglBindObjectMemory")))
        return disp_table->BindObjectMemory;
    else if (!strncmp("xglCreateFence", pName, sizeof ("xgllCreateFence")))
        return disp_table->CreateFence;
    else if (!strncmp("xglGetFenceStatus", pName, sizeof ("xglGetFenceStatus")))
        return disp_table->GetFenceStatus;
    else if (!strncmp("xglWaitForFences", pName, sizeof ("xglWaitForFences")))
        return disp_table->WaitForFences;
    else if (!strncmp("xglCreateQueueSemaphore", pName, sizeof ("xgllCreateQueueSemaphore")))
        return disp_table->CreateQueueSemaphore;
    else if (!strncmp("xglSignalQueueSemaphore", pName, sizeof ("xglSignalQueueSemaphore")))
        return disp_table->SignalQueueSemaphore;
    else if (!strncmp("xglWaitQueueSemaphore", pName, sizeof ("xglWaitQueueSemaphore")))
        return disp_table->WaitQueueSemaphore;
    else if (!strncmp("xglCreateEvent", pName, sizeof ("xgllCreateEvent")))
        return disp_table->CreateEvent;
    else if (!strncmp("xglGetEventStatus", pName, sizeof ("xglGetEventStatus")))
        return disp_table->GetEventStatus;
    else if (!strncmp("xglSetEvent", pName, sizeof ("xglSetEvent")))
        return disp_table->SetEvent;
    else if (!strncmp("xglResetEvent", pName, sizeof ("xgllResetEvent")))
        return disp_table->ResetEvent;
    else if (!strncmp("xglCreateQueryPool", pName, sizeof ("xglCreateQueryPool")))
        return disp_table->CreateQueryPool;
    else if (!strncmp("xglGetQueryPoolResults", pName, sizeof ("xglGetQueryPoolResults")))
        return disp_table->GetQueryPoolResults;
    else if (!strncmp("xglGetFormatInfo", pName, sizeof ("xgllGetFormatInfo")))
        return disp_table->GetFormatInfo;
    else if (!strncmp("xglCreateImage", pName, sizeof ("xglCreateImage")))
        return disp_table->CreateImage;
    else if (!strncmp("xglGetImageSubresourceInfo", pName, sizeof ("xglGetImageSubresourceInfo")))
        return disp_table->GetImageSubresourceInfo;
    else if (!strncmp("xglCreateImageView", pName, sizeof ("xglCreateImageView")))
        return disp_table->CreateImageView;
    else if (!strncmp("xglCreateColorAttachmentView", pName, sizeof ("xglCreateColorAttachmentView")))
        return disp_table->CreateColorAttachmentView;
    else if (!strncmp("xglCreateDepthStencilView", pName, sizeof ("xglCreateDepthStencilView")))
        return disp_table->CreateDepthStencilView;
    else if (!strncmp("xglCreateShader", pName, sizeof ("xglCreateShader")))
        return disp_table->CreateShader;
    else if (!strncmp("xglCreateGraphicsPipeline", pName, sizeof ("xglCreateGraphicsPipeline")))
        return disp_table->CreateGraphicsPipeline;
    else if (!strncmp("xglCreateComputePipeline", pName, sizeof ("xglCreateComputePipeline")))
        return disp_table->CreateComputePipeline;
    else if (!strncmp("xglStorePipeline", pName, sizeof ("xglStorePipeline")))
        return disp_table->StorePipeline;
    else if (!strncmp("xglLoadPipeline", pName, sizeof ("xglLoadPipeline")))
        return disp_table->LoadPipeline;
    else if (!strncmp("xglCreatePipelineDelta", pName, sizeof ("xglCreatePipelineDelta")))
        return disp_table->CreatePipelineDelta;
    else if (!strncmp("xglCreateSampler", pName, sizeof ("xglCreateSampler")))
        return disp_table->CreateSampler;
    else if (!strncmp("xglCreateDescriptorSet", pName, sizeof ("xglCreateDescriptorSet")))
        return disp_table->CreateDescriptorSet;
    else if (!strncmp("xglBeginDescriptorSetUpdate", pName, sizeof ("xglBeginDescriptorSetUpdate")))
        return disp_table->BeginDescriptorSetUpdate;
    else if (!strncmp("xglEndDescriptorSetUpdate", pName, sizeof ("xglEndDescriptorSetUpdate")))
        return disp_table->EndDescriptorSetUpdate;
    else if (!strncmp("xglAttachSamplerDescriptors", pName, sizeof ("xglAttachSamplerDescriptors")))
        return disp_table->AttachSamplerDescriptors;
    else if (!strncmp("xglAttachImageViewDescriptors", pName, sizeof ("xglAttachImageViewDescriptors")))
        return disp_table->AttachImageViewDescriptors;
    else if (!strncmp("xglAttachMemoryViewDescriptors", pName, sizeof ("xglAttachMemoryViewDescriptors")))
        return disp_table->AttachMemoryViewDescriptors;
    else if (!strncmp("xglAttachNestedDescriptors", pName, sizeof ("xglAttachNestedDescriptors")))
        return disp_table->AttachNestedDescriptors;
    else if (!strncmp("xglClearDescriptorSetSlots", pName, sizeof ("xglClearDescriptorSetSlots")))
        return disp_table->ClearDescriptorSetSlots;
    else if (!strncmp("xglCreateViewportState", pName, sizeof ("xglCreateViewportState")))
        return disp_table->CreateViewportState;
    else if (!strncmp("xglCreateRasterState", pName, sizeof ("xglCreateRasterState")))
        return disp_table->CreateRasterState;
    else if (!strncmp("xglCreateMsaaState", pName, sizeof ("xglCreateMsaaState")))
        return disp_table->CreateMsaaState;
    else if (!strncmp("xglCreateColorBlendState", pName, sizeof ("xglCreateColorBlendState")))
        return disp_table->CreateColorBlendState;
    else if (!strncmp("xglCreateDepthStencilState", pName, sizeof ("xglCreateDepthStencilState")))
        return disp_table->CreateDepthStencilState;
    else if (!strncmp("xglCreateCommandBuffer", pName, sizeof ("xglCreateCommandBuffer")))
        return disp_table->CreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", pName, sizeof ("xglBeginCommandBuffer")))
        return disp_table->BeginCommandBuffer;
    else if (!strncmp("xglEndCommandBuffer", pName, sizeof ("xglEndCommandBuffer")))
        return disp_table->EndCommandBuffer;
    else if (!strncmp("xglResetCommandBuffer", pName, sizeof ("xglResetCommandBuffer")))
        return disp_table->ResetCommandBuffer;
    else if (!strncmp("xglCmdBindPipeline", pName, sizeof ("xglCmdBindPipeline")))
        return disp_table->CmdBindPipeline;
    else if (!strncmp("xglCmdBindPipelineDelta", pName, sizeof ("xglCmdBindPipelineDelta")))
        return disp_table->CmdBindPipelineDelta;
    else if (!strncmp("xglCmdBindStateObject", pName, sizeof ("xglCmdBindStateObject")))
        return disp_table->CmdBindStateObject;
    else if (!strncmp("xglCmdBindDescriptorSet", pName, sizeof ("xglCmdBindDescriptorSet")))
        return disp_table->CmdBindDescriptorSet;
    else if (!strncmp("xglCmdBindDynamicMemoryView", pName, sizeof ("xglCmdBindDynamicMemoryView")))
        return disp_table->CmdBindDynamicMemoryView;
    else if (!strncmp("xglCmdBindVertexData", pName, sizeof ("xglCmdBindVertexData")))
        return disp_table->CmdBindVertexData;
    else if (!strncmp("xglCmdBindIndexData", pName, sizeof ("xglCmdBindIndexData")))
        return disp_table->CmdBindIndexData;
    else if (!strncmp("xglCmdBindAttachments", pName, sizeof ("xglCmdBindAttachments")))
        return disp_table->CmdBindAttachments;
    else if (!strncmp("xglCmdPrepareMemoryRegions", pName, sizeof ("xglCmdPrepareMemoryRegions")))
        return disp_table->CmdPrepareMemoryRegions;
    else if (!strncmp("xglCmdPrepareImages", pName, sizeof ("xglCmdPrepareImages")))
        return disp_table->CmdPrepareImages;
    else if (!strncmp("xglCmdDraw", pName, sizeof ("xglCmdDraw")))
        return disp_table->CmdDraw;
    else if (!strncmp("xglCmdDrawIndexed", pName, sizeof ("xglCmdDrawIndexed")))
        return disp_table->CmdDrawIndexed;
    else if (!strncmp("xglCmdDrawIndirect", pName, sizeof ("xglCmdDrawIndirect")))
        return disp_table->CmdDrawIndirect;
    else if (!strncmp("xglCmdDrawIndexedIndirect", pName, sizeof ("xglCmdDrawIndexedIndirect")))
        return disp_table->CmdDrawIndexedIndirect;
    else if (!strncmp("xglCmdDispatch", pName, sizeof ("xglCmdDispatch")))
        return disp_table->CmdDispatch;
    else if (!strncmp("xglCmdDispatchIndirect", pName, sizeof ("xglCmdDispatchIndirect")))
        return disp_table->CmdDispatchIndirect;
    else if (!strncmp("xglCmdCopyMemory", pName, sizeof ("xglCmdCopyMemory")))
        return disp_table->CmdCopyMemory;
    else if (!strncmp("xglCmdCopyImage", pName, sizeof ("xglCmdCopyImage")))
        return disp_table->CmdCopyImage;
    else if (!strncmp("xglCmdCopyMemoryToImage", pName, sizeof ("xglCmdCopyMemoryToImage")))
        return disp_table->CmdCopyMemoryToImage;
    else if (!strncmp("xglCmdCopyImageToMemory", pName, sizeof ("xglCmdCopyImageToMemory")))
        return disp_table->CmdCopyImageToMemory;
    else if (!strncmp("xglCmdCloneImageData", pName, sizeof ("xglCmdCloneImageData")))
        return disp_table->CmdCloneImageData;
    else if (!strncmp("xglCmdUpdateMemory", pName, sizeof ("xglCmdUpdateMemory")))
        return disp_table->CmdUpdateMemory;
    else if (!strncmp("xglCmdFillMemory", pName, sizeof ("xglCmdFillMemory")))
        return disp_table->CmdFillMemory;
    else if (!strncmp("xglCmdClearColorImage", pName, sizeof ("xglCmdClearColorImage")))
        return disp_table->CmdClearColorImage;
    else if (!strncmp("xglCmdClearColorImageRaw", pName, sizeof ("xglCmdClearColorImageRaw")))
        return disp_table->CmdClearColorImageRaw;
    else if (!strncmp("xglCmdClearDepthStencil", pName, sizeof ("xglCmdClearDepthStencil")))
        return disp_table->CmdClearDepthStencil;
    else if (!strncmp("xglCmdResolveImage", pName, sizeof ("xglCmdResolveImage")))
        return disp_table->CmdResolveImage;
    else if (!strncmp("xglCmdSetEvent", pName, sizeof ("xglCmdSetEvent")))
        return disp_table->CmdSetEvent;
    else if (!strncmp("xglCmdResetEvent", pName, sizeof ("xglCmdResetEvent")))
        return disp_table->CmdResetEvent;
    else if (!strncmp("xglCmdMemoryAtomic", pName, sizeof ("xglCmdMemoryAtomic")))
        return disp_table->CmdMemoryAtomic;
    else if (!strncmp("xglCmdBeginQuery", pName, sizeof ("xglCmdBeginQuery")))
        return disp_table->CmdBeginQuery;
    else if (!strncmp("xglCmdEndQuery", pName, sizeof ("xglCmdEndQuery")))
        return disp_table->CmdEndQuery;
    else if (!strncmp("xglCmdResetQueryPool", pName, sizeof ("xglCmdResetQueryPool")))
        return disp_table->CmdResetQueryPool;
    else if (!strncmp("xglCmdWriteTimestamp", pName, sizeof ("xglCmdWriteTimestamp")))
        return disp_table->CmdWriteTimestamp;
    else if (!strncmp("xglCmdInitAtomicCounters", pName, sizeof ("xglCmdInitAtomicCounters")))
        return disp_table->CmdInitAtomicCounters;
    else if (!strncmp("xglCmdLoadAtomicCounters", pName, sizeof ("xglCmdLoadAtomicCounters")))
        return disp_table->CmdLoadAtomicCounters;
    else if (!strncmp("xglCmdSaveAtomicCounters", pName, sizeof ("xglCmdSaveAtomicCounters")))
        return disp_table->CmdSaveAtomicCounters;
    else if (!strncmp("xglDbgSetValidationLevel", pName, sizeof ("xglDbgSetValidationLevel")))
        return disp_table->DbgSetValidationLevel;
    else if (!strncmp("xglDbgRegisterMsgCallback", pName, sizeof ("xglDbgRegisterMsgCallback")))
        return disp_table->DbgRegisterMsgCallback;
    else if (!strncmp("xglDbgUnregisterMsgCallback", pName, sizeof ("xglDbgUnregisterMsgCallback")))
        return disp_table->DbgUnregisterMsgCallback;
    else if (!strncmp("xglDbgSetMessageFilter", pName, sizeof ("xglDbgSetMessageFilter")))
        return disp_table->DbgSetMessageFilter;
    else if (!strncmp("xglDbgSetObjectTag", pName, sizeof ("xglDbgSetObjectTag")))
        return disp_table->DbgSetObjectTag;
    else if (!strncmp("xglDbgSetGlobalOption", pName, sizeof ("xglDbgSetGlobalOption")))
        return disp_table->DbgSetGlobalOption;
    else if (!strncmp("xglDbgSetDeviceOption", pName, sizeof ("xglDbgSetDeviceOption")))
        return disp_table->DbgSetDeviceOption;
    else if (!strncmp("xglCmdDbgMarkerBegin", pName, sizeof ("xglCmdDbgMarkerBegin")))
        return disp_table->CmdDbgMarkerBegin;
    else if (!strncmp("xglCmdDbgMarkerEnd", pName, sizeof ("xglCmdDbgMarkerEnd")))
        return disp_table->CmdDbgMarkerEnd;
    else if (!strncmp("xglWsiX11AssociateConnection", pName, sizeof("xglWsiX11AssociateConnection")))
        return disp_table->WsiX11AssociateConnection;
    else if (!strncmp("xglWsiX11GetMSC", pName, sizeof("xglWsiX11GetMSC")))
        return disp_table->WsiX11GetMSC;
    else if (!strncmp("xglWsiX11CreatePresentableImage", pName, sizeof("xglWsiX11CreatePresentableImage")))
        return disp_table->WsiX11CreatePresentableImage;
    else if (!strncmp("xglWsiX11QueuePresent", pName, sizeof("xglWsiX11QueuePresent")))
        return disp_table->WsiX11QueuePresent;
    else {
        // no one else to call
        return NULL;
    }
}
