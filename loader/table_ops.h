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
#include "loader.h"
#include "loader_platform.h"

static inline void loader_init_device_dispatch_table(VkLayerDispatchTable *table,
                                                    PFN_vkGetDeviceProcAddr gpa,
                                                    VkDevice dev_next,
                                                    VkDevice dev)
{
    // If layer is next, this will trigger layers to initialize their dispatch tables
    //then use the gpa in their dispatch for subsequent layers in the chain
    table->GetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) gpa(dev_next, "vkGetDeviceProcAddr");

    table->CreateDevice = (PFN_vkCreateDevice) gpa(dev, "vkCreateDevice");
    table->DestroyDevice = (PFN_vkDestroyDevice) gpa(dev, "vkDestroyDevice");
    table->GetDeviceQueue = (PFN_vkGetDeviceQueue) gpa(dev, "vkGetDeviceQueue");
    table->QueueSubmit = (PFN_vkQueueSubmit) gpa(dev, "vkQueueSubmit");
    table->QueueWaitIdle = (PFN_vkQueueWaitIdle) gpa(dev, "vkQueueWaitIdle");
    table->DeviceWaitIdle = (PFN_vkDeviceWaitIdle) gpa(dev, "vkDeviceWaitIdle");
    table->AllocMemory = (PFN_vkAllocMemory) gpa(dev, "vkAllocMemory");
    table->FreeMemory = (PFN_vkFreeMemory) gpa(dev, "vkFreeMemory");
    table->MapMemory = (PFN_vkMapMemory) gpa(dev, "vkMapMemory");
    table->UnmapMemory = (PFN_vkUnmapMemory) gpa(dev, "vkUnmapMemory");
    table->FlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges) gpa(dev, "vkFlushMappedMemoryRanges");
    table->InvalidateMappedMemoryRanges = (PFN_vkInvalidateMappedMemoryRanges) gpa(dev, "vkInvalidateMappedMemoryRanges");
    table->DestroyObject = (PFN_vkDestroyObject) gpa(dev, "vkDestroyObject");
    table->GetObjectMemoryRequirements = (PFN_vkGetObjectMemoryRequirements) gpa(dev, "vkGetObjectMemoryRequirements");
    table->BindObjectMemory = (PFN_vkBindObjectMemory) gpa(dev, "vkBindObjectMemory");
    table->QueueBindSparseBufferMemory = (PFN_vkQueueBindSparseBufferMemory) gpa(dev, "vkQueueBindSparseBufferMemory");
    table->QueueBindSparseImageMemory = (PFN_vkQueueBindSparseImageMemory) gpa(dev, "vkQueueBindSparseImageMemory");
    table->CreateFence = (PFN_vkCreateFence) gpa(dev, "vkCreateFence");
    table->ResetFences = (PFN_vkResetFences) gpa(dev, "vkResetFences");
    table->GetFenceStatus = (PFN_vkGetFenceStatus) gpa(dev, "vkGetFenceStatus");
    table->WaitForFences = (PFN_vkWaitForFences) gpa(dev, "vkWaitForFences");
    table->CreateSemaphore = (PFN_vkCreateSemaphore) gpa(dev, "vkCreateSemaphore");
    table->QueueSignalSemaphore = (PFN_vkQueueSignalSemaphore) gpa(dev, "vkQueueSignalSemaphore");
    table->QueueWaitSemaphore = (PFN_vkQueueWaitSemaphore) gpa(dev, "vkQueueWaitSemaphore");
    table->CreateEvent = (PFN_vkCreateEvent) gpa(dev, "vkCreateEvent");
    table->GetEventStatus = (PFN_vkGetEventStatus) gpa(dev, "vkGetEventStatus");
    table->SetEvent = (PFN_vkSetEvent) gpa(dev, "vkSetEvent");
    table->ResetEvent = (PFN_vkResetEvent) gpa(dev, "vkResetEvent");
    table->CreateQueryPool = (PFN_vkCreateQueryPool) gpa(dev, "vkCreateQueryPool");
    table->GetQueryPoolResults = (PFN_vkGetQueryPoolResults) gpa(dev, "vkGetQueryPoolResults");
    table->CreateBuffer = (PFN_vkCreateBuffer) gpa(dev, "vkCreateBuffer");
    table->CreateBufferView = (PFN_vkCreateBufferView) gpa(dev, "vkCreateBufferView");
    table->CreateImage = (PFN_vkCreateImage) gpa(dev, "vkCreateImage");
    table->GetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout) gpa(dev, "vkGetImageSubresourceLayout");
    table->CreateImageView = (PFN_vkCreateImageView) gpa(dev, "vkCreateImageView");
    table->CreateColorAttachmentView = (PFN_vkCreateColorAttachmentView) gpa(dev, "vkCreateColorAttachmentView");
    table->CreateDepthStencilView = (PFN_vkCreateDepthStencilView) gpa(dev, "vkCreateDepthStencilView");
    table->CreateShader = (PFN_vkCreateShader) gpa(dev, "vkCreateShader");
    table->CreateGraphicsPipeline = (PFN_vkCreateGraphicsPipeline) gpa(dev, "vkCreateGraphicsPipeline");
    table->CreateGraphicsPipelineDerivative = (PFN_vkCreateGraphicsPipelineDerivative) gpa(dev, "vkCreateGraphicsPipelineDerivative");
    table->CreateComputePipeline = (PFN_vkCreateComputePipeline) gpa(dev, "vkCreateComputePipeline");
    table->StorePipeline = (PFN_vkStorePipeline) gpa(dev, "vkStorePipeline");
    table->LoadPipeline = (PFN_vkLoadPipeline) gpa(dev, "vkLoadPipeline");
    table->LoadPipelineDerivative = (PFN_vkLoadPipelineDerivative) gpa(dev, "vkLoadPipelineDerivative");
    table->CreatePipelineLayout = (PFN_vkCreatePipelineLayout) gpa(dev, "vkCreatePipelineLayout");
    table->CreateSampler = (PFN_vkCreateSampler) gpa(dev, "vkCreateSampler");
    table->CreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout) gpa(dev, "vkCreateDescriptorSetLayout");
    table->CreateDescriptorPool = (PFN_vkCreateDescriptorPool) gpa(dev, "vkCreateDescriptorPool");
    table->ResetDescriptorPool = (PFN_vkResetDescriptorPool) gpa(dev, "vkResetDescriptorPool");
    table->AllocDescriptorSets = (PFN_vkAllocDescriptorSets) gpa(dev, "vkAllocDescriptorSets");
    table->UpdateDescriptorSets = (PFN_vkUpdateDescriptorSets) gpa(dev, "vkUpdateDescriptorSets");
    table->CreateDynamicViewportState = (PFN_vkCreateDynamicViewportState) gpa(dev, "vkCreateDynamicViewportState");
    table->CreateDynamicRasterState = (PFN_vkCreateDynamicRasterState) gpa(dev, "vkCreateDynamicRasterState");
    table->CreateDynamicColorBlendState = (PFN_vkCreateDynamicColorBlendState) gpa(dev, "vkCreateDynamicColorBlendState");
    table->CreateDynamicDepthStencilState = (PFN_vkCreateDynamicDepthStencilState) gpa(dev, "vkCreateDynamicDepthStencilState");
    table->CreateCommandBuffer = (PFN_vkCreateCommandBuffer) gpa(dev, "vkCreateCommandBuffer");
    table->BeginCommandBuffer = (PFN_vkBeginCommandBuffer) gpa(dev, "vkBeginCommandBuffer");
    table->EndCommandBuffer = (PFN_vkEndCommandBuffer) gpa(dev, "vkEndCommandBuffer");
    table->ResetCommandBuffer = (PFN_vkResetCommandBuffer) gpa(dev, "vkResetCommandBuffer");
    table->CmdBindPipeline = (PFN_vkCmdBindPipeline) gpa(dev, "vkCmdBindPipeline");
    table->CmdBindDynamicStateObject = (PFN_vkCmdBindDynamicStateObject) gpa(dev, "vkCmdBindDynamicStateObject");
    table->CmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets) gpa(dev, "vkCmdBindDescriptorSets");
    table->CmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers) gpa(dev, "vkCmdBindVertexBuffers");
    table->CmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer) gpa(dev, "vkCmdBindIndexBuffer");
    table->CmdDraw = (PFN_vkCmdDraw) gpa(dev, "vkCmdDraw");
    table->CmdDrawIndexed = (PFN_vkCmdDrawIndexed) gpa(dev, "vkCmdDrawIndexed");
    table->CmdDrawIndirect = (PFN_vkCmdDrawIndirect) gpa(dev, "vkCmdDrawIndirect");
    table->CmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect) gpa(dev, "vkCmdDrawIndexedIndirect");
    table->CmdDispatch = (PFN_vkCmdDispatch) gpa(dev, "vkCmdDispatch");
    table->CmdDispatchIndirect = (PFN_vkCmdDispatchIndirect) gpa(dev, "vkCmdDispatchIndirect");
    table->CmdCopyBuffer = (PFN_vkCmdCopyBuffer) gpa(dev, "vkCmdCopyBuffer");
    table->CmdCopyImage = (PFN_vkCmdCopyImage) gpa(dev, "vkCmdCopyImage");
    table->CmdBlitImage = (PFN_vkCmdBlitImage) gpa(dev, "vkCmdBlitImage");
    table->CmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage) gpa(dev, "vkCmdCopyBufferToImage");
    table->CmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer) gpa(dev, "vkCmdCopyImageToBuffer");
    table->CmdUpdateBuffer = (PFN_vkCmdUpdateBuffer) gpa(dev, "vkCmdUpdateBuffer");
    table->CmdFillBuffer = (PFN_vkCmdFillBuffer) gpa(dev, "vkCmdFillBuffer");
    table->CmdClearColorImage = (PFN_vkCmdClearColorImage) gpa(dev, "vkCmdClearColorImage");
    table->CmdClearDepthStencil = (PFN_vkCmdClearDepthStencil) gpa(dev, "vkCmdClearDepthStencil");
    table->CmdResolveImage = (PFN_vkCmdResolveImage) gpa(dev, "vkCmdResolveImage");
    table->CmdSetEvent = (PFN_vkCmdSetEvent) gpa(dev, "vkCmdSetEvent");
    table->CmdResetEvent = (PFN_vkCmdResetEvent) gpa(dev, "vkCmdResetEvent");
    table->CmdWaitEvents = (PFN_vkCmdWaitEvents) gpa(dev, "vkCmdWaitEvents");
    table->CmdPipelineBarrier = (PFN_vkCmdPipelineBarrier) gpa(dev, "vkCmdPipelineBarrier");
    table->CmdBeginQuery = (PFN_vkCmdBeginQuery) gpa(dev, "vkCmdBeginQuery");
    table->CmdEndQuery = (PFN_vkCmdEndQuery) gpa(dev, "vkCmdEndQuery");
    table->CmdResetQueryPool = (PFN_vkCmdResetQueryPool) gpa(dev, "vkCmdResetQueryPool");
    table->CmdWriteTimestamp = (PFN_vkCmdWriteTimestamp) gpa(dev, "vkCmdWriteTimestamp");
    table->CmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults) gpa(dev, "vkCmdCopyQueryPoolResults");
    table->CmdInitAtomicCounters = (PFN_vkCmdInitAtomicCounters) gpa(dev, "vkCmdInitAtomicCounters");
    table->CmdLoadAtomicCounters = (PFN_vkCmdLoadAtomicCounters) gpa(dev, "vkCmdLoadAtomicCounters");
    table->CmdSaveAtomicCounters = (PFN_vkCmdSaveAtomicCounters) gpa(dev, "vkCmdSaveAtomicCounters");
    table->CreateFramebuffer = (PFN_vkCreateFramebuffer) gpa(dev, "vkCreateFramebuffer");
    table->CreateRenderPass = (PFN_vkCreateRenderPass) gpa(dev, "vkCreateRenderPass");
    table->CmdBeginRenderPass = (PFN_vkCmdBeginRenderPass) gpa(dev, "vkCmdBeginRenderPass");
    table->CmdEndRenderPass = (PFN_vkCmdEndRenderPass) gpa(dev, "vkCmdEndRenderPass");
//TODO move into it's own table
//TODO also consider dropping trampoline code for these device level extensions entirely
// then don't need loader to know about these at all but then not queryable via GIPA
    table->CreateSwapChainWSI = (PFN_vkCreateSwapChainWSI) gpa(dev, "vkCreateSwapChainWSI");
    table->DestroySwapChainWSI = (PFN_vkDestroySwapChainWSI) gpa(dev, "vkDestroySwapChainWSI");
    table->GetSwapChainInfoWSI = (PFN_vkGetSwapChainInfoWSI) gpa(dev, "vkGetSwapChainInfoWSI");
    table->QueuePresentWSI = (PFN_vkQueuePresentWSI) gpa(dev, "vkQueuePresentWSI");
}

static inline void *loader_lookup_device_dispatch_table(
                                            const VkLayerDispatchTable *table,
                                            const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "GetDeviceProcAddr"))
        return (void *) table->GetDeviceProcAddr;
    if (!strcmp(name, "CreateDevice"))
        return (void *) table->CreateDevice;
    if (!strcmp(name, "DestroyDevice"))
        return (void *) table->DestroyDevice;
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
    if (!strcmp(name, "MapMemory"))
        return (void *) table->MapMemory;
    if (!strcmp(name, "UnmapMemory"))
        return (void *) table->UnmapMemory;
    if (!strcmp(name, "FlushMappedMemoryRanges"))
        return (void *) table->FlushMappedMemoryRanges;
    if (!strcmp(name, "InvalidateMappedMemoryRanges"))
        return (void *) table->InvalidateMappedMemoryRanges;
    if (!strcmp(name, "DestroyObject"))
        return (void *) table->DestroyObject;
    if (!strcmp(name, "GetObjectMemoryRequirements"))
        return (void *) table->GetObjectMemoryRequirements;
    if (!strcmp(name, "BindObjectMemory"))
        return (void *) table->BindObjectMemory;
    if (!strcmp(name, "QueueBindSparseBufferMemory"))
        return (void *) table->QueueBindSparseBufferMemory;
    if (!strcmp(name, "QueueBindSparseImageMemory"))
        return (void *) table->QueueBindSparseImageMemory;
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
    if (!strcmp(name, "CreateBuffer"))
        return (void *) table->CreateBuffer;
    if (!strcmp(name, "CreateBufferView"))
        return (void *) table->CreateBufferView;
    if (!strcmp(name, "CreateImage"))
        return (void *) table->CreateImage;
    if (!strcmp(name, "GetImageSubresourceLayout"))
        return (void *) table->GetImageSubresourceLayout;
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
    if (!strcmp(name, "CreateDescriptorPool"))
        return (void *) table->CreateDescriptorPool;
    if (!strcmp(name, "ResetDescriptorPool"))
        return (void *) table->ResetDescriptorPool;
    if (!strcmp(name, "AllocDescriptorSets"))
        return (void *) table->AllocDescriptorSets;
    if (!strcmp(name, "UpdateDescriptorSets"))
        return (void *) table->UpdateDescriptorSets;
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

    return NULL;
}

static inline void loader_init_instance_core_dispatch_table(VkLayerInstanceDispatchTable *table,
                                                PFN_vkGetInstanceProcAddr gpa,
                                                VkInstance inst_next,
                                                VkInstance inst)
{
    // If layer is next, this will trigger layers to initialize their dispatch tables
    //then use the gpa in their dispatch for subsequent layers in the chain
    table->GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) gpa(inst_next, "vkGetInstanceProcAddr");

    table->CreateInstance = (PFN_vkCreateInstance) gpa(inst, "vkCreateInstance");
    table->DestroyInstance = (PFN_vkDestroyInstance) gpa(inst, "vkDestroyInstance");
    table->EnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) gpa(inst, "vkEnumeratePhysicalDevices");
    table->GetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) gpa(inst, "vkGetPhysicalDeviceFeatures");
    table->GetPhysicalDeviceFormatInfo = (PFN_vkGetPhysicalDeviceFormatInfo) gpa(inst, "vkGetPhysicalDeviceFormatInfo");
    table->GetPhysicalDeviceLimits = (PFN_vkGetPhysicalDeviceLimits) gpa(inst, "vkGetPhysicalDeviceLimits");
    table->GetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) gpa(inst, "vkGetPhysicalDeviceProperties");
    table->GetPhysicalDevicePerformance = (PFN_vkGetPhysicalDevicePerformance) gpa(inst, "vkGetPhysicalDevicePerformance");
    table->GetPhysicalDeviceQueueCount = (PFN_vkGetPhysicalDeviceQueueCount) gpa(inst, "vkGetPhysicalDeviceQueueCount");
    table->GetPhysicalDeviceQueueProperties = (PFN_vkGetPhysicalDeviceQueueProperties) gpa(inst, "vkGetPhysicalDeviceQueueProperties");
    table->GetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties) gpa(inst, "vkGetPhysicalDeviceMemoryProperties");
    table->GetPhysicalDeviceExtensionProperties = (PFN_vkGetPhysicalDeviceExtensionProperties) gpa(inst, "vkGetPhysicalDeviceExtensionProperties");
    table->GetPhysicalDeviceExtensionCount = (PFN_vkGetPhysicalDeviceExtensionCount) gpa(inst, "vkGetPhysicalDeviceExtensionCount");
}

static inline void loader_init_instance_extension_dispatch_table(
        VkLayerInstanceDispatchTable *table,
        PFN_vkGetInstanceProcAddr gpa,
        VkInstance inst)
{
    table->DbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback) gpa(inst, "vkDbgCreateMsgCallback");
    table->DbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback) gpa(inst, "vkDbgDestroyMsgCallback");
}

static inline void *loader_lookup_instance_dispatch_table(
                                        const VkLayerInstanceDispatchTable *table,
                                        const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (void *) table->CreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (void *) table->DestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (void *) table->EnumeratePhysicalDevices;
    if (!strcmp(name, "GetPhysicalDeviceFeatures"))
        return (void *) table->GetPhysicalDeviceFeatures;
    if (!strcmp(name, "GetPhysicalDeviceFormatInfo"))
        return (void *) table->GetPhysicalDeviceFormatInfo;
    if (!strcmp(name, "GetPhysicalDeviceLimits"))
        return (void *) table->GetPhysicalDeviceLimits;
    if (!strcmp(name, "GetPhysicalDeviceProperties"))
        return (void *) table->GetPhysicalDeviceProperties;
    if (!strcmp(name, "GetPhysicalDevicePerformance"))
        return (void *) table->GetPhysicalDevicePerformance;
    if (!strcmp(name, "GetPhysicalDeviceQueueCount"))
        return (void *) table->GetPhysicalDeviceQueueCount;
    if (!strcmp(name, "GetPhysicalDeviceQueueProperties"))
        return (void *) table->GetPhysicalDeviceQueueProperties;
    if (!strcmp(name, "GetPhysicalDeviceMemoryProperties"))
        return (void *) table->GetPhysicalDeviceMemoryProperties;
    if (!strcmp(name, "GetInstanceProcAddr"))
        return (void *) table->GetInstanceProcAddr;
    if (!strcmp(name, "GetPhysicalDeviceExtensionCount"))
        return (void *) table->GetPhysicalDeviceExtensionCount;
    if (!strcmp(name, "GetPhysicalDeviceExtensionProperties"))
        return (void *) table->GetPhysicalDeviceExtensionProperties;
    if (!strcmp(name, "DbgCreateMsgCallback"))
        return (void *) table->DbgCreateMsgCallback;
    if (!strcmp(name, "DbgDestroyMsgCallback"))
        return (void *) table->DbgDestroyMsgCallback;

    return NULL;
}
