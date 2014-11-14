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

static const struct icd_dispatch_table intel_normal_dispatch_table = {
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

static const struct icd_dispatch_table intel_debug_dispatch_table = {
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

static struct icd_dispatch_table *debug_dispatch = (struct icd_dispatch_table *) &intel_debug_dispatch_table;
static struct icd_dispatch_table *normal_dispatch = (struct icd_dispatch_table *) &intel_normal_dispatch_table;

const struct icd_dispatch_table *intel_dispatch_get(bool debug)
{

    return (debug) ? debug_dispatch : normal_dispatch;
}

void intelSetDispatch(struct icd_dispatch_table * dispatch, bool debug)
{
    if (debug)
        debug_dispatch = (dispatch == NULL) ? (struct icd_dispatch_table *) &intel_debug_dispatch_table : dispatch;
    else
        normal_dispatch = (dispatch == NULL) ? (struct icd_dispatch_table *) &intel_normal_dispatch_table : dispatch;
}

//TODO  since loader now has a dispatch table and it is used at first initAndEnumerate
// probably can remove the normal and debug icd dispatch table  and have GPA use hardcoded function names
XGL_VOID * intelGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR * pName)
{
    const struct icd_dispatch_table *disp_table = * (const struct icd_dispatch_table * const *) gpu;

   if (!strncmp("xglGetProcAddr", (const char *) pName, sizeof("xglGetProcAddr")))
        return disp_table->GetProcAddr;
    else if (!strncmp("xglInitAndEnumerateGpus", (const char *) pName, sizeof("xglInitAndEnumerateGpus")))
        return disp_table->InitAndEnumerateGpus;
    if (!strncmp("xglGetGpuInfo", (const char *) pName, sizeof ("xglGetGpuInfo")))
        return disp_table->GetGpuInfo;
    else if (!strncmp("xglCreateDevice", (const char *) pName, sizeof ("xglCreateDevice")))
        return disp_table->CreateDevice;
    else if (!strncmp("xglDestroyDevice", (const char *) pName, sizeof ("xglDestroyDevice")))
        return disp_table->DestroyDevice;
    else if (!strncmp("xglGetExtensionSupport", (const char *) pName, sizeof ("xglGetExtensionSupport")))
        return disp_table->GetExtensionSupport;
    else if (!strncmp("xglGetDeviceQueue", (const char *) pName, sizeof ("xglGetDeviceQueue")))
        return disp_table->GetDeviceQueue;
    else if (!strncmp("xglQueueSubmit", (const char *) pName, sizeof ("xglQueueSubmit")))
        return disp_table->QueueSubmit;
    else if (!strncmp("xglQueueSetGlobalMemReferences", (const char *) pName, sizeof ("xglQueueSetGlobalMemReferences")))
        return disp_table->QueueSetGlobalMemReferences;
    else if (!strncmp("xglQueueWaitIdle", (const char *) pName, sizeof ("xglQueueWaitIdle")))
        return disp_table->QueueWaitIdle;
    else if (!strncmp("xglDeviceWaitIdle", (const char *) pName, sizeof ("xglDeviceWaitIdle")))
        return disp_table->DeviceWaitIdle;
    else if (!strncmp("xglGetMemoryHeapCount", (const char *) pName, sizeof ("xglGetMemoryHeapCount")))
        return disp_table->GetMemoryHeapCount;
    else if (!strncmp("xglGetMemoryHeapInfo", (const char *) pName, sizeof ("xglGetMemoryHeapInfo")))
        return disp_table->GetMemoryHeapInfo;
    else if (!strncmp("xglAllocMemory", (const char *) pName, sizeof ("xglAllocMemory")))
        return disp_table->AllocMemory;
    else if (!strncmp("xglFreeMemory", (const char *) pName, sizeof ("xglFreeMemory")))
        return disp_table->FreeMemory;
    else if (!strncmp("xglSetMemoryPriority", (const char *) pName, sizeof ("xglSetMemoryPriority")))
        return disp_table->SetMemoryPriority;
    else if (!strncmp("xglMapMemory", (const char *) pName, sizeof ("xglMapMemory")))
        return disp_table->MapMemory;
    else if (!strncmp("xglUnmapMemory", (const char *) pName, sizeof ("xglUnmapMemory")))
        return disp_table->UnmapMemory;
    else if (!strncmp("xglPinSystemMemory", (const char *) pName, sizeof ("xglPinSystemMemory")))
        return disp_table->PinSystemMemory;
    else if (!strncmp("xglRemapVirtualMemoryPages", (const char *) pName, sizeof ("xglRemapVirtualMemoryPages")))
        return disp_table->RemapVirtualMemoryPages;
    else if (!strncmp("xglGetMultiGpuCompatibility", (const char *) pName, sizeof ("xglGetMultiGpuCompatibility")))
        return disp_table->GetMultiGpuCompatibility;
    else if (!strncmp("xglOpenSharedMemory", (const char *) pName, sizeof ("xglOpenSharedMemory")))
        return disp_table->OpenSharedMemory;
    else if (!strncmp("xglOpenSharedQueueSemaphore", (const char *) pName, sizeof ("xglOpenSharedQueueSemaphore")))
        return disp_table->OpenSharedQueueSemaphore;
    else if (!strncmp("xglOpenPeerMemory", (const char *) pName, sizeof ("xglOpenPeerMemory")))
        return disp_table->OpenPeerMemory;
    else if (!strncmp("xglOpenPeerImage", (const char *) pName, sizeof ("xglOpenPeerImage")))
        return disp_table->OpenPeerImage;
    else if (!strncmp("xglDestroyObject", (const char *) pName, sizeof ("xglDestroyObject")))
        return disp_table->DestroyObject;
    else if (!strncmp("xglGetObjectInfo", (const char *) pName, sizeof ("xglGetObjectInfo")))
        return disp_table->GetObjectInfo;
    else if (!strncmp("xglBindObjectMemory", (const char *) pName, sizeof ("xglBindObjectMemory")))
        return disp_table->BindObjectMemory;
    else if (!strncmp("xglCreateFence", (const char *) pName, sizeof ("xgllCreateFence")))
        return disp_table->CreateFence;
    else if (!strncmp("xglGetFenceStatus", (const char *) pName, sizeof ("xglGetFenceStatus")))
        return disp_table->GetFenceStatus;
    else if (!strncmp("xglWaitForFences", (const char *) pName, sizeof ("xglWaitForFences")))
        return disp_table->WaitForFences;
    else if (!strncmp("xglCreateQueueSemaphore", (const char *) pName, sizeof ("xgllCreateQueueSemaphore")))
        return disp_table->CreateQueueSemaphore;
    else if (!strncmp("xglSignalQueueSemaphore", (const char *) pName, sizeof ("xglSignalQueueSemaphore")))
        return disp_table->SignalQueueSemaphore;
    else if (!strncmp("xglWaitQueueSemaphore", (const char *) pName, sizeof ("xglWaitQueueSemaphore")))
        return disp_table->WaitQueueSemaphore;
    else if (!strncmp("xglCreateEvent", (const char *) pName, sizeof ("xgllCreateEvent")))
        return disp_table->CreateEvent;
    else if (!strncmp("xglGetEventStatus", (const char *) pName, sizeof ("xglGetEventStatus")))
        return disp_table->GetEventStatus;
    else if (!strncmp("xglSetEvent", (const char *) pName, sizeof ("xglSetEvent")))
        return disp_table->SetEvent;
    else if (!strncmp("xglResetEvent", (const char *) pName, sizeof ("xgllResetEvent")))
        return disp_table->ResetEvent;
    else if (!strncmp("xglCreateQueryPool", (const char *) pName, sizeof ("xglCreateQueryPool")))
        return disp_table->CreateQueryPool;
    else if (!strncmp("xglGetQueryPoolResults", (const char *) pName, sizeof ("xglGetQueryPoolResults")))
        return disp_table->GetQueryPoolResults;
    else if (!strncmp("xglGetFormatInfo", (const char *) pName, sizeof ("xgllGetFormatInfo")))
        return disp_table->GetFormatInfo;
    else if (!strncmp("xglCreateImage", (const char *) pName, sizeof ("xglCreateImage")))
        return disp_table->CreateImage;
    else if (!strncmp("xglGetImageSubresourceInfo", (const char *) pName, sizeof ("xglGetImageSubresourceInfo")))
        return disp_table->GetImageSubresourceInfo;
    else if (!strncmp("xglCreateImageView", (const char *) pName, sizeof ("xglCreateImageView")))
        return disp_table->CreateImageView;
    else if (!strncmp("xglCreateColorAttachmentView", (const char *) pName, sizeof ("xglCreateColorAttachmentView")))
        return disp_table->CreateColorAttachmentView;
    else if (!strncmp("xglCreateDepthStencilView", (const char *) pName, sizeof ("xglCreateDepthStencilView")))
        return disp_table->CreateDepthStencilView;
    else if (!strncmp("xglCreateShader", (const char *) pName, sizeof ("xglCreateShader")))
        return disp_table->CreateShader;
    else if (!strncmp("xglCreateGraphicsPipeline", (const char *) pName, sizeof ("xglCreateGraphicsPipeline")))
        return disp_table->CreateGraphicsPipeline;
    else if (!strncmp("xglCreateComputePipeline", (const char *) pName, sizeof ("xglCreateComputePipeline")))
        return disp_table->CreateComputePipeline;
    else if (!strncmp("xglStorePipeline", (const char *) pName, sizeof ("xglStorePipeline")))
        return disp_table->StorePipeline;
    else if (!strncmp("xglLoadPipeline", (const char *) pName, sizeof ("xglLoadPipeline")))
        return disp_table->LoadPipeline;
    else if (!strncmp("xglCreatePipelineDelta", (const char *) pName, sizeof ("xglCreatePipelineDelta")))
        return disp_table->CreatePipelineDelta;
    else if (!strncmp("xglCreateSampler", (const char *) pName, sizeof ("xglCreateSampler")))
        return disp_table->CreateSampler;
    else if (!strncmp("xglCreateDescriptorSet", (const char *) pName, sizeof ("xglCreateDescriptorSet")))
        return disp_table->CreateDescriptorSet;
    else if (!strncmp("xglBeginDescriptorSetUpdate", (const char *) pName, sizeof ("xglBeginDescriptorSetUpdate")))
        return disp_table->BeginDescriptorSetUpdate;
    else if (!strncmp("xglEndDescriptorSetUpdate", (const char *) pName, sizeof ("xglEndDescriptorSetUpdate")))
        return disp_table->EndDescriptorSetUpdate;
    else if (!strncmp("xglAttachSamplerDescriptors", (const char *) pName, sizeof ("xglAttachSamplerDescriptors")))
        return disp_table->AttachSamplerDescriptors;
    else if (!strncmp("xglAttachImageViewDescriptors", (const char *) pName, sizeof ("xglAttachImageViewDescriptors")))
        return disp_table->AttachImageViewDescriptors;
    else if (!strncmp("xglAttachMemoryViewDescriptors", (const char *) pName, sizeof ("xglAttachMemoryViewDescriptors")))
        return disp_table->AttachMemoryViewDescriptors;
    else if (!strncmp("xglAttachNestedDescriptors", (const char *) pName, sizeof ("xglAttachNestedDescriptors")))
        return disp_table->AttachNestedDescriptors;
    else if (!strncmp("xglClearDescriptorSetSlots", (const char *) pName, sizeof ("xglClearDescriptorSetSlots")))
        return disp_table->ClearDescriptorSetSlots;
    else if (!strncmp("xglCreateViewportState", (const char *) pName, sizeof ("xglCreateViewportState")))
        return disp_table->CreateViewportState;
    else if (!strncmp("xglCreateRasterState", (const char *) pName, sizeof ("xglCreateRasterState")))
        return disp_table->CreateRasterState;
    else if (!strncmp("xglCreateMsaaState", (const char *) pName, sizeof ("xglCreateMsaaState")))
        return disp_table->CreateMsaaState;
    else if (!strncmp("xglCreateColorBlendState", (const char *) pName, sizeof ("xglCreateColorBlendState")))
        return disp_table->CreateColorBlendState;
    else if (!strncmp("xglCreateDepthStencilState", (const char *) pName, sizeof ("xglCreateDepthStencilState")))
        return disp_table->CreateDepthStencilState;
    else if (!strncmp("xglCreateCommandBuffer", (const char *) pName, sizeof ("xglCreateCommandBuffer")))
        return disp_table->CreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", (const char *) pName, sizeof ("xglBeginCommandBuffer")))
        return disp_table->BeginCommandBuffer;
    else if (!strncmp("xglEndCommandBuffer", (const char *) pName, sizeof ("xglEndCommandBuffer")))
        return disp_table->EndCommandBuffer;
    else if (!strncmp("xglResetCommandBuffer", (const char *) pName, sizeof ("xglResetCommandBuffer")))
        return disp_table->ResetCommandBuffer;
    else if (!strncmp("xglCmdBindPipeline", (const char *) pName, sizeof ("xglCmdBindPipeline")))
        return disp_table->CmdBindPipeline;
    else if (!strncmp("xglCmdBindPipelineDelta", (const char *) pName, sizeof ("xglCmdBindPipelineDelta")))
        return disp_table->CmdBindPipelineDelta;
    else if (!strncmp("xglCmdBindStateObject", (const char *) pName, sizeof ("xglCmdBindStateObject")))
        return disp_table->CmdBindStateObject;
    else if (!strncmp("xglCmdBindDescriptorSet", (const char *) pName, sizeof ("xglCmdBindDescriptorSet")))
        return disp_table->CmdBindDescriptorSet;
    else if (!strncmp("xglCmdBindDynamicMemoryView", (const char *) pName, sizeof ("xglCmdBindDynamicMemoryView")))
        return disp_table->CmdBindDynamicMemoryView;
    else if (!strncmp("xglCmdBindVertexData", (const char *) pName, sizeof ("xglCmdBindVertexData")))
        return disp_table->CmdBindVertexData;
    else if (!strncmp("xglCmdBindIndexData", (const char *) pName, sizeof ("xglCmdBindIndexData")))
        return disp_table->CmdBindIndexData;
    else if (!strncmp("xglCmdBindAttachments", (const char *) pName, sizeof ("xglCmdBindAttachments")))
        return disp_table->CmdBindAttachments;
    else if (!strncmp("xglCmdPrepareMemoryRegions", (const char *) pName, sizeof ("xglCmdPrepareMemoryRegions")))
        return disp_table->CmdPrepareMemoryRegions;
    else if (!strncmp("xglCmdPrepareImages", (const char *) pName, sizeof ("xglCmdPrepareImages")))
        return disp_table->CmdPrepareImages;
    else if (!strncmp("xglCmdDraw", (const char *) pName, sizeof ("xglCmdDraw")))
        return disp_table->CmdDraw;
    else if (!strncmp("xglCmdDrawIndexed", (const char *) pName, sizeof ("xglCmdDrawIndexed")))
        return disp_table->CmdDrawIndexed;
    else if (!strncmp("xglCmdDrawIndirect", (const char *) pName, sizeof ("xglCmdDrawIndirect")))
        return disp_table->CmdDrawIndirect;
    else if (!strncmp("xglCmdDrawIndexedIndirect", (const char *) pName, sizeof ("xglCmdDrawIndexedIndirect")))
        return disp_table->CmdDrawIndexedIndirect;
    else if (!strncmp("xglCmdDispatch", (const char *) pName, sizeof ("xglCmdDispatch")))
        return disp_table->CmdDispatch;
    else if (!strncmp("xglCmdDispatchIndirect", (const char *) pName, sizeof ("xglCmdDispatchIndirect")))
        return disp_table->CmdDispatchIndirect;
    else if (!strncmp("xglCmdCopyMemory", (const char *) pName, sizeof ("xglCmdCopyMemory")))
        return disp_table->CmdCopyMemory;
    else if (!strncmp("xglCmdCopyImage", (const char *) pName, sizeof ("xglCmdCopyImage")))
        return disp_table->CmdCopyImage;
    else if (!strncmp("xglCmdCopyMemoryToImage", (const char *) pName, sizeof ("xglCmdCopyMemoryToImage")))
        return disp_table->CmdCopyMemoryToImage;
    else if (!strncmp("xglCmdCopyImageToMemory", (const char *) pName, sizeof ("xglCmdCopyImageToMemory")))
        return disp_table->CmdCopyImageToMemory;
    else if (!strncmp("xglCmdCloneImageData", (const char *) pName, sizeof ("xglCmdCloneImageData")))
        return disp_table->CmdCloneImageData;
    else if (!strncmp("xglCmdUpdateMemory", (const char *) pName, sizeof ("xglCmdUpdateMemory")))
        return disp_table->CmdUpdateMemory;
    else if (!strncmp("xglCmdFillMemory", (const char *) pName, sizeof ("xglCmdFillMemory")))
        return disp_table->CmdFillMemory;
    else if (!strncmp("xglCmdClearColorImage", (const char *) pName, sizeof ("xglCmdClearColorImage")))
        return disp_table->CmdClearColorImage;
    else if (!strncmp("xglCmdClearColorImageRaw", (const char *) pName, sizeof ("xglCmdClearColorImageRaw")))
        return disp_table->CmdClearColorImageRaw;
    else if (!strncmp("xglCmdClearDepthStencil", (const char *) pName, sizeof ("xglCmdClearDepthStencil")))
        return disp_table->CmdClearDepthStencil;
    else if (!strncmp("xglCmdResolveImage", (const char *) pName, sizeof ("xglCmdResolveImage")))
        return disp_table->CmdResolveImage;
    else if (!strncmp("xglCmdSetEvent", (const char *) pName, sizeof ("xglCmdSetEvent")))
        return disp_table->CmdSetEvent;
    else if (!strncmp("xglCmdResetEvent", (const char *) pName, sizeof ("xglCmdResetEvent")))
        return disp_table->CmdResetEvent;
    else if (!strncmp("xglCmdMemoryAtomic", (const char *) pName, sizeof ("xglCmdMemoryAtomic")))
        return disp_table->CmdMemoryAtomic;
    else if (!strncmp("xglCmdBeginQuery", (const char *) pName, sizeof ("xglCmdBeginQuery")))
        return disp_table->CmdBeginQuery;
    else if (!strncmp("xglCmdEndQuery", (const char *) pName, sizeof ("xglCmdEndQuery")))
        return disp_table->CmdEndQuery;
    else if (!strncmp("xglCmdResetQueryPool", (const char *) pName, sizeof ("xglCmdResetQueryPool")))
        return disp_table->CmdResetQueryPool;
    else if (!strncmp("xglCmdWriteTimestamp", (const char *) pName, sizeof ("xglCmdWriteTimestamp")))
        return disp_table->CmdWriteTimestamp;
    else if (!strncmp("xglCmdInitAtomicCounters", (const char *) pName, sizeof ("xglCmdInitAtomicCounters")))
        return disp_table->CmdInitAtomicCounters;
    else if (!strncmp("xglCmdLoadAtomicCounters", (const char *) pName, sizeof ("xglCmdLoadAtomicCounters")))
        return disp_table->CmdLoadAtomicCounters;
    else if (!strncmp("xglCmdSaveAtomicCounters", (const char *) pName, sizeof ("xglCmdSaveAtomicCounters")))
        return disp_table->CmdSaveAtomicCounters;
    else if (!strncmp("xglDbgSetValidationLevel", (const char *) pName, sizeof ("xglDbgSetValidationLevel")))
        return disp_table->DbgSetValidationLevel;
    else if (!strncmp("xglDbgRegisterMsgCallback", (const char *) pName, sizeof ("xglDbgRegisterMsgCallback")))
        return disp_table->DbgRegisterMsgCallback;
    else if (!strncmp("xglDbgUnregisterMsgCallback", (const char *) pName, sizeof ("xglDbgUnregisterMsgCallback")))
        return disp_table->DbgUnregisterMsgCallback;
    else if (!strncmp("xglDbgSetMessageFilter", (const char *) pName, sizeof ("xglDbgSetMessageFilter")))
        return disp_table->DbgSetMessageFilter;
    else if (!strncmp("xglDbgSetObjectTag", (const char *) pName, sizeof ("xglDbgSetObjectTag")))
        return disp_table->DbgSetObjectTag;
    else if (!strncmp("xglDbgSetGlobalOption", (const char *) pName, sizeof ("xglDbgSetGlobalOption")))
        return disp_table->DbgSetGlobalOption;
    else if (!strncmp("xglDbgSetDeviceOption", (const char *) pName, sizeof ("xglDbgSetDeviceOption")))
        return disp_table->DbgSetDeviceOption;
    else if (!strncmp("xglCmdDbgMarkerBegin", (const char *) pName, sizeof ("xglCmdDbgMarkerBegin")))
        return disp_table->CmdDbgMarkerBegin;
    else if (!strncmp("xglCmdDbgMarkerEnd", (const char *) pName, sizeof ("xglCmdDbgMarkerEnd")))
        return disp_table->CmdDbgMarkerEnd;
    else if (!strncmp("xglWsiX11AssociateConnection", (const char *) pName, sizeof("xglWsiX11AssociateConnection")))
        return disp_table->WsiX11AssociateConnection;
    else if (!strncmp("xglWsiX11GetMSC", (const char *) pName, sizeof("xglWsiX11GetMSC")))
        return disp_table->WsiX11GetMSC;
    else if (!strncmp("xglWsiX11CreatePresentableImage", (const char *) pName, sizeof("xglWsiX11CreatePresentableImage")))
        return disp_table->WsiX11CreatePresentableImage;
    else if (!strncmp("xglWsiX11QueuePresent", (const char *) pName, sizeof("xglWsiX11QueuePresent")))
        return disp_table->WsiX11QueuePresent;
    else {
        // no one else to call
        return NULL;
    }
}
