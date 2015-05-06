/*
 * Vulkan
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

#include <vulkan.h>
#include <vkLayer.h>
#include <string.h>
#include "loader_platform.h"

static inline void loader_initialize_dispatch_table(VkLayerDispatchTable *table,
                                                    PFN_vkGetProcAddr gpa,
                                                    VkPhysicalDevice gpu)
{
    table->CreateInstance = (PFN_vkCreateInstance) gpa(gpu, "vkCreateInstance");
    table->DestroyInstance = (PFN_vkDestroyInstance) gpa(gpu, "vkDestroyInstance");
    table->EnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) gpa(gpu, "vkEnumeratePhysicalDevices");
    table->GetPhysicalDeviceInfo = (PFN_vkGetPhysicalDeviceInfo) gpa(gpu, "vkGetPhysicalDeviceInfo");
    table->GetProcAddr = (PFN_vkGetProcAddr) gpa(gpu, "vkGetProcAddr");
    table->CreateDevice = (PFN_vkCreateDevice) gpa(gpu, "vkCreateDevice");
    table->DestroyDevice = (PFN_vkDestroyDevice) gpa(gpu, "vkDestroyDevice");
    table->GetGlobalExtensionInfo = vkGetGlobalExtensionInfo; /* non-dispatchable */
    table->GetPhysicalDeviceExtensionInfo = (PFN_vkGetPhysicalDeviceExtensionInfo) gpa(gpu, "vkGetPhysicalDeviceExtensionInfo");
    table->EnumerateLayers = (PFN_vkEnumerateLayers) gpa(gpu, "vkEnumerateLayers");
    table->GetDeviceQueue = (PFN_vkGetDeviceQueue) gpa(gpu, "vkGetDeviceQueue");
    table->QueueSubmit = (PFN_vkQueueSubmit) gpa(gpu, "vkQueueSubmit");
    table->QueueWaitIdle = (PFN_vkQueueWaitIdle) gpa(gpu, "vkQueueWaitIdle");
    table->DeviceWaitIdle = (PFN_vkDeviceWaitIdle) gpa(gpu, "vkDeviceWaitIdle");
    table->AllocMemory = (PFN_vkAllocMemory) gpa(gpu, "vkAllocMemory");
    table->FreeMemory = (PFN_vkFreeMemory) gpa(gpu, "vkFreeMemory");
    table->SetMemoryPriority = (PFN_vkSetMemoryPriority) gpa(gpu, "vkSetMemoryPriority");
    table->MapMemory = (PFN_vkMapMemory) gpa(gpu, "vkMapMemory");
    table->UnmapMemory = (PFN_vkUnmapMemory) gpa(gpu, "vkUnmapMemory");
    table->FlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges) gpa(gpu, "vkFlushMappedMemoryRanges");
    table->InvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges) gpa(gpu, "vkInvalidateMappedMemoryRanges");
    table->PinSystemMemory = (PFN_vkPinSystemMemory) gpa(gpu, "vkPinSystemMemory");
    table->GetMultiDeviceCompatibility = (PFN_vkGetMultiDeviceCompatibility) gpa(gpu, "vkGetMultiDeviceCompatibility");
    table->OpenSharedMemory = (PFN_vkOpenSharedMemory) gpa(gpu, "vkOpenSharedMemory");
    table->OpenSharedSemaphore = (PFN_vkOpenSharedSemaphore) gpa(gpu, "vkOpenSharedSemaphore");
    table->OpenPeerMemory = (PFN_vkOpenPeerMemory) gpa(gpu, "vkOpenPeerMemory");
    table->OpenPeerImage = (PFN_vkOpenPeerImage) gpa(gpu, "vkOpenPeerImage");
    table->DestroyObject = (PFN_vkDestroyObject) gpa(gpu, "vkDestroyObject");
    table->GetObjectInfo = (PFN_vkGetObjectInfo) gpa(gpu, "vkGetObjectInfo");
    table->QueueBindObjectMemory = (PFN_vkQueueBindObjectMemory) gpa(gpu, "vkQueueBindObjectMemory");
    table->QueueBindObjectMemoryRange = (PFN_vkQueueBindObjectMemoryRange) gpa(gpu, "vkQueueBindObjectMemoryRange");
    table->QueueBindImageMemoryRange = (PFN_vkQueueBindImageMemoryRange) gpa(gpu, "vkQueueBindImageMemoryRange");
    table->CreateFence = (PFN_vkCreateFence) gpa(gpu, "vkCreateFence");
    table->ResetFences = (PFN_vkResetFences) gpa(gpu, "vkResetFences");
    table->GetFenceStatus = (PFN_vkGetFenceStatus) gpa(gpu, "vkGetFenceStatus");
    table->WaitForFences = (PFN_vkWaitForFences) gpa(gpu, "vkWaitForFences");
    table->CreateSemaphore = (PFN_vkCreateSemaphore) gpa(gpu, "vkCreateSemaphore");
    table->QueueSignalSemaphore = (PFN_vkQueueSignalSemaphore) gpa(gpu, "vkQueueSignalSemaphore");
    table->QueueWaitSemaphore = (PFN_vkQueueWaitSemaphore) gpa(gpu, "vkQueueWaitSemaphore");
    table->CreateEvent = (PFN_vkCreateEvent) gpa(gpu, "vkCreateEvent");
    table->GetEventStatus = (PFN_vkGetEventStatus) gpa(gpu, "vkGetEventStatus");
    table->SetEvent = (PFN_vkSetEvent) gpa(gpu, "vkSetEvent");
    table->ResetEvent = (PFN_vkResetEvent) gpa(gpu, "vkResetEvent");
    table->CreateQueryPool = (PFN_vkCreateQueryPool) gpa(gpu, "vkCreateQueryPool");
    table->GetQueryPoolResults = (PFN_vkGetQueryPoolResults) gpa(gpu, "vkGetQueryPoolResults");
    table->GetFormatInfo = (PFN_vkGetFormatInfo) gpa(gpu, "vkGetFormatInfo");
    table->CreateBuffer = (PFN_vkCreateBuffer) gpa(gpu, "vkCreateBuffer");
    table->CreateBufferView = (PFN_vkCreateBufferView) gpa(gpu, "vkCreateBufferView");
    table->CreateImage = (PFN_vkCreateImage) gpa(gpu, "vkCreateImage");
    table->GetImageSubresourceInfo = (PFN_vkGetImageSubresourceInfo) gpa(gpu, "vkGetImageSubresourceInfo");
    table->CreateImageView = (PFN_vkCreateImageView) gpa(gpu, "vkCreateImageView");
    table->CreateColorAttachmentView = (PFN_vkCreateColorAttachmentView) gpa(gpu, "vkCreateColorAttachmentView");
    table->CreateDepthStencilView = (PFN_vkCreateDepthStencilView) gpa(gpu, "vkCreateDepthStencilView");
    table->CreateShader = (PFN_vkCreateShader) gpa(gpu, "vkCreateShader");
    table->CreateGraphicsPipeline = (PFN_vkCreateGraphicsPipeline) gpa(gpu, "vkCreateGraphicsPipeline");
    table->CreateGraphicsPipelineDerivative = (PFN_vkCreateGraphicsPipelineDerivative) gpa(gpu, "vkCreateGraphicsPipelineDerivative");
    table->CreateComputePipeline = (PFN_vkCreateComputePipeline) gpa(gpu, "vkCreateComputePipeline");
    table->StorePipeline = (PFN_vkStorePipeline) gpa(gpu, "vkStorePipeline");
    table->LoadPipeline = (PFN_vkLoadPipeline) gpa(gpu, "vkLoadPipeline");
    table->LoadPipelineDerivative = (PFN_vkLoadPipelineDerivative) gpa(gpu, "vkLoadPipelineDerivative");
    table->CreatePipelineLayout = (PFN_vkCreatePipelineLayout) gpa(gpu, "vkCreatePipelineLayout");
    table->CreateSampler = (PFN_vkCreateSampler) gpa(gpu, "vkCreateSampler");
    table->CreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout) gpa(gpu, "vkCreateDescriptorSetLayout");
    table->BeginDescriptorPoolUpdate = (PFN_vkBeginDescriptorPoolUpdate) gpa(gpu, "vkBeginDescriptorPoolUpdate");
    table->EndDescriptorPoolUpdate = (PFN_vkEndDescriptorPoolUpdate) gpa(gpu, "vkEndDescriptorPoolUpdate");
    table->CreateDescriptorPool = (PFN_vkCreateDescriptorPool) gpa(gpu, "vkCreateDescriptorPool");
    table->ResetDescriptorPool = (PFN_vkResetDescriptorPool) gpa(gpu, "vkResetDescriptorPool");
    table->AllocDescriptorSets = (PFN_vkAllocDescriptorSets) gpa(gpu, "vkAllocDescriptorSets");
    table->ClearDescriptorSets = (PFN_vkClearDescriptorSets) gpa(gpu, "vkClearDescriptorSets");
    table->UpdateDescriptors = (PFN_vkUpdateDescriptors) gpa(gpu, "vkUpdateDescriptors");
    table->CreateDynamicViewportState = (PFN_vkCreateDynamicViewportState) gpa(gpu, "vkCreateDynamicViewportState");
    table->CreateDynamicRasterState = (PFN_vkCreateDynamicRasterState) gpa(gpu, "vkCreateDynamicRasterState");
    table->CreateDynamicColorBlendState = (PFN_vkCreateDynamicColorBlendState) gpa(gpu, "vkCreateDynamicColorBlendState");
    table->CreateDynamicDepthStencilState = (PFN_vkCreateDynamicDepthStencilState) gpa(gpu, "vkCreateDynamicDepthStencilState");
    table->CreateCommandBuffer = (PFN_vkCreateCommandBuffer) gpa(gpu, "vkCreateCommandBuffer");
    table->BeginCommandBuffer = (PFN_vkBeginCommandBuffer) gpa(gpu, "vkBeginCommandBuffer");
    table->EndCommandBuffer = (PFN_vkEndCommandBuffer) gpa(gpu, "vkEndCommandBuffer");
    table->ResetCommandBuffer = (PFN_vkResetCommandBuffer) gpa(gpu, "vkResetCommandBuffer");
    table->CmdBindPipeline = (PFN_vkCmdBindPipeline) gpa(gpu, "vkCmdBindPipeline");
    table->CmdBindDynamicStateObject = (PFN_vkCmdBindDynamicStateObject) gpa(gpu, "vkCmdBindDynamicStateObject");
    table->CmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets) gpa(gpu, "vkCmdBindDescriptorSets");
    table->CmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers) gpa(gpu, "vkCmdBindVertexBuffers");
    table->CmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer) gpa(gpu, "vkCmdBindIndexBuffer");
    table->CmdDraw = (PFN_vkCmdDraw) gpa(gpu, "vkCmdDraw");
    table->CmdDrawIndexed = (PFN_vkCmdDrawIndexed) gpa(gpu, "vkCmdDrawIndexed");
    table->CmdDrawIndirect = (PFN_vkCmdDrawIndirect) gpa(gpu, "vkCmdDrawIndirect");
    table->CmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect) gpa(gpu, "vkCmdDrawIndexedIndirect");
    table->CmdDispatch = (PFN_vkCmdDispatch) gpa(gpu, "vkCmdDispatch");
    table->CmdDispatchIndirect = (PFN_vkCmdDispatchIndirect) gpa(gpu, "vkCmdDispatchIndirect");
    table->CmdCopyBuffer = (PFN_vkCmdCopyBuffer) gpa(gpu, "vkCmdCopyBuffer");
    table->CmdCopyImage = (PFN_vkCmdCopyImage) gpa(gpu, "vkCmdCopyImage");
    table->CmdBlitImage = (PFN_vkCmdBlitImage) gpa(gpu, "vkCmdBlitImage");
    table->CmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage) gpa(gpu, "vkCmdCopyBufferToImage");
    table->CmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer) gpa(gpu, "vkCmdCopyImageToBuffer");
    table->CmdCloneImageData = (PFN_vkCmdCloneImageData) gpa(gpu, "vkCmdCloneImageData");
    table->CmdUpdateBuffer = (PFN_vkCmdUpdateBuffer) gpa(gpu, "vkCmdUpdateBuffer");
    table->CmdFillBuffer = (PFN_vkCmdFillBuffer) gpa(gpu, "vkCmdFillBuffer");
    table->CmdClearColorImage = (PFN_vkCmdClearColorImage) gpa(gpu, "vkCmdClearColorImage");
    table->CmdClearDepthStencil = (PFN_vkCmdClearDepthStencil) gpa(gpu, "vkCmdClearDepthStencil");
    table->CmdResolveImage = (PFN_vkCmdResolveImage) gpa(gpu, "vkCmdResolveImage");
    table->CmdSetEvent = (PFN_vkCmdSetEvent) gpa(gpu, "vkCmdSetEvent");
    table->CmdResetEvent = (PFN_vkCmdResetEvent) gpa(gpu, "vkCmdResetEvent");
    table->CmdWaitEvents = (PFN_vkCmdWaitEvents) gpa(gpu, "vkCmdWaitEvents");
    table->CmdPipelineBarrier = (PFN_vkCmdPipelineBarrier) gpa(gpu, "vkCmdPipelineBarrier");
    table->CmdBeginQuery = (PFN_vkCmdBeginQuery) gpa(gpu, "vkCmdBeginQuery");
    table->CmdEndQuery = (PFN_vkCmdEndQuery) gpa(gpu, "vkCmdEndQuery");
    table->CmdResetQueryPool = (PFN_vkCmdResetQueryPool) gpa(gpu, "vkCmdResetQueryPool");
    table->CmdWriteTimestamp = (PFN_vkCmdWriteTimestamp) gpa(gpu, "vkCmdWriteTimestamp");
    table->CmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults) gpa(gpu, "vkCmdCopyQueryPoolResults");
    table->CmdInitAtomicCounters = (PFN_vkCmdInitAtomicCounters) gpa(gpu, "vkCmdInitAtomicCounters");
    table->CmdLoadAtomicCounters = (PFN_vkCmdLoadAtomicCounters) gpa(gpu, "vkCmdLoadAtomicCounters");
    table->CmdSaveAtomicCounters = (PFN_vkCmdSaveAtomicCounters) gpa(gpu, "vkCmdSaveAtomicCounters");
    table->CreateFramebuffer = (PFN_vkCreateFramebuffer) gpa(gpu, "vkCreateFramebuffer");
    table->CreateRenderPass = (PFN_vkCreateRenderPass) gpa(gpu, "vkCreateRenderPass");
    table->CmdBeginRenderPass = (PFN_vkCmdBeginRenderPass) gpa(gpu, "vkCmdBeginRenderPass");
    table->CmdEndRenderPass = (PFN_vkCmdEndRenderPass) gpa(gpu, "vkCmdEndRenderPass");
    table->DbgSetValidationLevel = (PFN_vkDbgSetValidationLevel) gpa(gpu, "vkDbgSetValidationLevel");
    table->DbgRegisterMsgCallback = (PFN_vkDbgRegisterMsgCallback) gpa(gpu, "vkDbgRegisterMsgCallback");
    table->DbgUnregisterMsgCallback = (PFN_vkDbgUnregisterMsgCallback) gpa(gpu, "vkDbgUnregisterMsgCallback");
    table->DbgSetMessageFilter = (PFN_vkDbgSetMessageFilter) gpa(gpu, "vkDbgSetMessageFilter");
    table->DbgSetObjectTag = (PFN_vkDbgSetObjectTag) gpa(gpu, "vkDbgSetObjectTag");
    table->DbgSetGlobalOption = (PFN_vkDbgSetGlobalOption) gpa(gpu, "vkDbgSetGlobalOption");
    table->DbgSetDeviceOption = (PFN_vkDbgSetDeviceOption) gpa(gpu, "vkDbgSetDeviceOption");
    table->CmdDbgMarkerBegin = (PFN_vkCmdDbgMarkerBegin) gpa(gpu, "vkCmdDbgMarkerBegin");
    table->CmdDbgMarkerEnd = (PFN_vkCmdDbgMarkerEnd) gpa(gpu, "vkCmdDbgMarkerEnd");
    table->GetDisplayInfoWSI = (PFN_vkGetDisplayInfoWSI) gpa(gpu, "vkGetDisplayInfoWSI");
    table->CreateSwapChainWSI = (PFN_vkCreateSwapChainWSI) gpa(gpu, "vkCreateSwapChainWSI");
    table->DestroySwapChainWSI = (PFN_vkDestroySwapChainWSI) gpa(gpu, "vkDestroySwapChainWSI");
    table->GetSwapChainInfoWSI = (PFN_vkGetSwapChainInfoWSI) gpa(gpu, "vkGetSwapChainInfoWSI");
    table->QueuePresentWSI = (PFN_vkQueuePresentWSI) gpa(gpu, "vkQueuePresentWSI");
}

static inline void *loader_lookup_dispatch_table(const VkLayerDispatchTable *table,
                                                 const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "DestroyInstance"))
        return (void *) table->DestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (void *) table->EnumeratePhysicalDevices;
    if (!strcmp(name, "GetPhysicalDeviceInfo"))
        return (void *) table->GetPhysicalDeviceInfo;
    if (!strcmp(name, "GetProcAddr"))
        return (void *) table->GetProcAddr;
    if (!strcmp(name, "CreateDevice"))
        return (void *) table->CreateDevice;
    if (!strcmp(name, "DestroyDevice"))
        return (void *) table->DestroyDevice;
    if (!strcmp(name, "GetPhysicalDeviceExtensionInfo"))
        return (void *) table->GetPhysicalDeviceExtensionInfo;
    if (!strcmp(name, "EnumerateLayers"))
        return (void *) table->EnumerateLayers;
    if (!strcmp(name, "GetDeviceQueue"))
        return (void *) table->GetDeviceQueue;
    if (!strcmp(name, "QueueSubmit"))
        return (void *) table->QueueSubmit;
    if (!strcmp(name, "QueueWaitIdle"))
        return (void *) table->QueueWaitIdle;
    if (!strcmp(name, "DeviceWaitIdle"))
        return (void *) table->DeviceWaitIdle;
    if (!strcmp(name, "AllocMemory"))
        return (void *) table->AllocMemory;
    if (!strcmp(name, "FreeMemory"))
        return (void *) table->FreeMemory;
    if (!strcmp(name, "SetMemoryPriority"))
        return (void *) table->SetMemoryPriority;
    if (!strcmp(name, "MapMemory"))
        return (void *) table->MapMemory;
    if (!strcmp(name, "UnmapMemory"))
        return (void *) table->UnmapMemory;
    if (!strcmp(name, "FlushMappedMemoryRanges"))
        return (void *) table->FlushMappedMemoryRanges;
    if (!strcmp(name, "InvalidateMappedMemoryRanges"))
        return (void *) table->InvalidateMappedMemoryRanges;
    if (!strcmp(name, "PinSystemMemory"))
        return (void *) table->PinSystemMemory;
    if (!strcmp(name, "GetMultiDeviceCompatibility"))
        return (void *) table->GetMultiDeviceCompatibility;
    if (!strcmp(name, "OpenSharedMemory"))
        return (void *) table->OpenSharedMemory;
    if (!strcmp(name, "OpenSharedSemaphore"))
        return (void *) table->OpenSharedSemaphore;
    if (!strcmp(name, "OpenPeerMemory"))
        return (void *) table->OpenPeerMemory;
    if (!strcmp(name, "OpenPeerImage"))
        return (void *) table->OpenPeerImage;
    if (!strcmp(name, "DestroyObject"))
        return (void *) table->DestroyObject;
    if (!strcmp(name, "GetObjectInfo"))
        return (void *) table->GetObjectInfo;
    if (!strcmp(name, "QueueBindObjectMemory"))
        return (void *) table->QueueBindObjectMemory;
    if (!strcmp(name, "QueueBindObjectMemoryRange"))
        return (void *) table->QueueBindObjectMemoryRange;
    if (!strcmp(name, "QueueBindImageMemoryRange"))
        return (void *) table->QueueBindImageMemoryRange;
    if (!strcmp(name, "CreateFence"))
        return (void *) table->CreateFence;
    if (!strcmp(name, "ResetFences"))
        return (void *) table->ResetFences;
    if (!strcmp(name, "GetFenceStatus"))
        return (void *) table->GetFenceStatus;
    if (!strcmp(name, "WaitForFences"))
        return (void *) table->WaitForFences;
    if (!strcmp(name, "CreateSemaphore"))
        return (void *) table->CreateSemaphore;
    if (!strcmp(name, "QueueSignalSemaphore"))
        return (void *) table->QueueSignalSemaphore;
    if (!strcmp(name, "QueueWaitSemaphore"))
        return (void *) table->QueueWaitSemaphore;
    if (!strcmp(name, "CreateEvent"))
        return (void *) table->CreateEvent;
    if (!strcmp(name, "GetEventStatus"))
        return (void *) table->GetEventStatus;
    if (!strcmp(name, "SetEvent"))
        return (void *) table->SetEvent;
    if (!strcmp(name, "ResetEvent"))
        return (void *) table->ResetEvent;
    if (!strcmp(name, "CreateQueryPool"))
        return (void *) table->CreateQueryPool;
    if (!strcmp(name, "GetQueryPoolResults"))
        return (void *) table->GetQueryPoolResults;
    if (!strcmp(name, "GetFormatInfo"))
        return (void *) table->GetFormatInfo;
    if (!strcmp(name, "CreateBuffer"))
        return (void *) table->CreateBuffer;
    if (!strcmp(name, "CreateBufferView"))
        return (void *) table->CreateBufferView;
    if (!strcmp(name, "CreateImage"))
        return (void *) table->CreateImage;
    if (!strcmp(name, "GetImageSubresourceInfo"))
        return (void *) table->GetImageSubresourceInfo;
    if (!strcmp(name, "CreateImageView"))
        return (void *) table->CreateImageView;
    if (!strcmp(name, "CreateColorAttachmentView"))
        return (void *) table->CreateColorAttachmentView;
    if (!strcmp(name, "CreateDepthStencilView"))
        return (void *) table->CreateDepthStencilView;
    if (!strcmp(name, "CreateShader"))
        return (void *) table->CreateShader;
    if (!strcmp(name, "CreateGraphicsPipeline"))
        return (void *) table->CreateGraphicsPipeline;
    if (!strcmp(name, "CreateGraphicsPipelineDerivative"))
        return (void *) table->CreateGraphicsPipelineDerivative;
    if (!strcmp(name, "CreateComputePipeline"))
        return (void *) table->CreateComputePipeline;
    if (!strcmp(name, "StorePipeline"))
        return (void *) table->StorePipeline;
    if (!strcmp(name, "LoadPipeline"))
        return (void *) table->LoadPipeline;
    if (!strcmp(name, "LoadPipelineDerivative"))
        return (void *) table->LoadPipelineDerivative;
    if (!strcmp(name, "CreatePipelineLayout"))
        return (void *) table->CreatePipelineLayout;
    if (!strcmp(name, "CreateSampler"))
        return (void *) table->CreateSampler;
    if (!strcmp(name, "CreateDescriptorSetLayout"))
        return (void *) table->CreateDescriptorSetLayout;
    if (!strcmp(name, "BeginDescriptorPoolUpdate"))
        return (void *) table->BeginDescriptorPoolUpdate;
    if (!strcmp(name, "EndDescriptorPoolUpdate"))
        return (void *) table->EndDescriptorPoolUpdate;
    if (!strcmp(name, "CreateDescriptorPool"))
        return (void *) table->CreateDescriptorPool;
    if (!strcmp(name, "ResetDescriptorPool"))
        return (void *) table->ResetDescriptorPool;
    if (!strcmp(name, "AllocDescriptorSets"))
        return (void *) table->AllocDescriptorSets;
    if (!strcmp(name, "ClearDescriptorSets"))
        return (void *) table->ClearDescriptorSets;
    if (!strcmp(name, "UpdateDescriptors"))
        return (void *) table->UpdateDescriptors;
    if (!strcmp(name, "CreateDynamicViewportState"))
        return (void *) table->CreateDynamicViewportState;
    if (!strcmp(name, "CreateDynamicRasterState"))
        return (void *) table->CreateDynamicRasterState;
    if (!strcmp(name, "CreateDynamicColorBlendState"))
        return (void *) table->CreateDynamicColorBlendState;
    if (!strcmp(name, "CreateDynamicDepthStencilState"))
        return (void *) table->CreateDynamicDepthStencilState;
    if (!strcmp(name, "CreateCommandBuffer"))
        return (void *) table->CreateCommandBuffer;
    if (!strcmp(name, "BeginCommandBuffer"))
        return (void *) table->BeginCommandBuffer;
    if (!strcmp(name, "EndCommandBuffer"))
        return (void *) table->EndCommandBuffer;
    if (!strcmp(name, "ResetCommandBuffer"))
        return (void *) table->ResetCommandBuffer;
    if (!strcmp(name, "CmdBindPipeline"))
        return (void *) table->CmdBindPipeline;
    if (!strcmp(name, "CmdBindDynamicStateObject"))
        return (void *) table->CmdBindDynamicStateObject;
    if (!strcmp(name, "CmdBindDescriptorSets"))
        return (void *) table->CmdBindDescriptorSets;
    if (!strcmp(name, "CmdBindVertexBuffers"))
        return (void *) table->CmdBindVertexBuffers;
    if (!strcmp(name, "CmdBindIndexBuffer"))
        return (void *) table->CmdBindIndexBuffer;
    if (!strcmp(name, "CmdDraw"))
        return (void *) table->CmdDraw;
    if (!strcmp(name, "CmdDrawIndexed"))
        return (void *) table->CmdDrawIndexed;
    if (!strcmp(name, "CmdDrawIndirect"))
        return (void *) table->CmdDrawIndirect;
    if (!strcmp(name, "CmdDrawIndexedIndirect"))
        return (void *) table->CmdDrawIndexedIndirect;
    if (!strcmp(name, "CmdDispatch"))
        return (void *) table->CmdDispatch;
    if (!strcmp(name, "CmdDispatchIndirect"))
        return (void *) table->CmdDispatchIndirect;
    if (!strcmp(name, "CmdCopyBuffer"))
        return (void *) table->CmdCopyBuffer;
    if (!strcmp(name, "CmdCopyImage"))
        return (void *) table->CmdCopyImage;
    if (!strcmp(name, "CmdBlitImage"))
        return (void *) table->CmdBlitImage;
    if (!strcmp(name, "CmdCopyBufferToImage"))
        return (void *) table->CmdCopyBufferToImage;
    if (!strcmp(name, "CmdCopyImageToBuffer"))
        return (void *) table->CmdCopyImageToBuffer;
    if (!strcmp(name, "CmdCloneImageData"))
        return (void *) table->CmdCloneImageData;
    if (!strcmp(name, "CmdUpdateBuffer"))
        return (void *) table->CmdUpdateBuffer;
    if (!strcmp(name, "CmdFillBuffer"))
        return (void *) table->CmdFillBuffer;
    if (!strcmp(name, "CmdClearColorImage"))
        return (void *) table->CmdClearColorImage;
    if (!strcmp(name, "CmdClearDepthStencil"))
        return (void *) table->CmdClearDepthStencil;
    if (!strcmp(name, "CmdResolveImage"))
        return (void *) table->CmdResolveImage;
    if (!strcmp(name, "CmdSetEvent"))
        return (void *) table->CmdSetEvent;
    if (!strcmp(name, "CmdResetEvent"))
        return (void *) table->CmdResetEvent;
    if (!strcmp(name, "CmdWaitEvents"))
        return (void *) table->CmdWaitEvents;
    if (!strcmp(name, "CmdPipelineBarrier"))
        return (void *) table->CmdPipelineBarrier;
    if (!strcmp(name, "CmdBeginQuery"))
        return (void *) table->CmdBeginQuery;
    if (!strcmp(name, "CmdEndQuery"))
        return (void *) table->CmdEndQuery;
    if (!strcmp(name, "CmdResetQueryPool"))
        return (void *) table->CmdResetQueryPool;
    if (!strcmp(name, "CmdWriteTimestamp"))
        return (void *) table->CmdWriteTimestamp;
    if (!strcmp(name, "CmdCopyQueryPoolResults"))
        return (void *) table->CmdCopyQueryPoolResults;
    if (!strcmp(name, "CmdInitAtomicCounters"))
        return (void *) table->CmdInitAtomicCounters;
    if (!strcmp(name, "CmdLoadAtomicCounters"))
        return (void *) table->CmdLoadAtomicCounters;
    if (!strcmp(name, "CmdSaveAtomicCounters"))
        return (void *) table->CmdSaveAtomicCounters;
    if (!strcmp(name, "CreateFramebuffer"))
        return (void *) table->CreateFramebuffer;
    if (!strcmp(name, "CreateRenderPass"))
        return (void *) table->CreateRenderPass;
    if (!strcmp(name, "CmdBeginRenderPass"))
        return (void *) table->CmdBeginRenderPass;
    if (!strcmp(name, "CmdEndRenderPass"))
        return (void *) table->CmdEndRenderPass;
    if (!strcmp(name, "DbgSetValidationLevel"))
        return (void *) table->DbgSetValidationLevel;
    if (!strcmp(name, "DbgRegisterMsgCallback"))
        return (void *) table->DbgRegisterMsgCallback;
    if (!strcmp(name, "DbgUnregisterMsgCallback"))
        return (void *) table->DbgUnregisterMsgCallback;
    if (!strcmp(name, "DbgSetMessageFilter"))
        return (void *) table->DbgSetMessageFilter;
    if (!strcmp(name, "DbgSetObjectTag"))
        return (void *) table->DbgSetObjectTag;
    if (!strcmp(name, "DbgSetGlobalOption"))
        return (void *) table->DbgSetGlobalOption;
    if (!strcmp(name, "DbgSetDeviceOption"))
        return (void *) table->DbgSetDeviceOption;
    if (!strcmp(name, "CmdDbgMarkerBegin"))
        return (void *) table->CmdDbgMarkerBegin;
    if (!strcmp(name, "CmdDbgMarkerEnd"))
        return (void *) table->CmdDbgMarkerEnd;
    if (!strcmp(name, "GetDisplayInfoWSI"))
        return (void *) table->GetDisplayInfoWSI;
    if (!strcmp(name, "CreateSwapChainWSI"))
        return (void *) table->CreateSwapChainWSI;
    if (!strcmp(name, "DestroySwapChainWSI"))
        return (void *) table->DestroySwapChainWSI;
    if (!strcmp(name, "GetSwapChainInfoWSI"))
        return (void *) table->GetSwapChainInfoWSI;
    if (!strcmp(name, "QueuePresentWSI"))
        return (void *) table->QueuePresentWSI;

    return NULL;
}
