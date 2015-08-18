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
#include <vk_layer.h>
#include <string.h>
#include "loader.h"
#include "vk_loader_platform.h"

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
    table->GetDeviceMemoryCommitment = (PFN_vkGetDeviceMemoryCommitment) gpa(dev, "vkGetDeviceMemoryCommitment");
    table->GetImageSparseMemoryRequirements = (PFN_vkGetImageSparseMemoryRequirements) gpa(dev, "vkGetImageSparseMemoryRequirements");
    table->GetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements) gpa(dev, "vkGetBufferMemoryRequirements");
    table->GetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements) gpa(dev, "vkGetImageMemoryRequirements");
    table->BindBufferMemory = (PFN_vkBindBufferMemory) gpa(dev, "vkBindBufferMemory");
    table->BindImageMemory = (PFN_vkBindImageMemory) gpa(dev, "vkBindImageMemory");
    table->QueueBindSparseBufferMemory = (PFN_vkQueueBindSparseBufferMemory) gpa(dev, "vkQueueBindSparseBufferMemory");
    table->QueueBindSparseImageMemory = (PFN_vkQueueBindSparseImageMemory) gpa(dev, "vkQueueBindSparseImageMemory");
    table->QueueBindSparseImageOpaqueMemory = (PFN_vkQueueBindSparseImageOpaqueMemory) gpa(dev, "vkQueueBindSparseImageOpaqueMemory");
    table->CreateFence = (PFN_vkCreateFence) gpa(dev, "vkCreateFence");
    table->DestroyFence = (PFN_vkDestroyFence) gpa(dev, "vkDestroyFence");
    table->ResetFences = (PFN_vkResetFences) gpa(dev, "vkResetFences");
    table->GetFenceStatus = (PFN_vkGetFenceStatus) gpa(dev, "vkGetFenceStatus");
    table->WaitForFences = (PFN_vkWaitForFences) gpa(dev, "vkWaitForFences");
    table->CreateSemaphore = (PFN_vkCreateSemaphore) gpa(dev, "vkCreateSemaphore");
    table->DestroySemaphore = (PFN_vkDestroySemaphore) gpa(dev, "vkDestroySemaphore");
    table->QueueSignalSemaphore = (PFN_vkQueueSignalSemaphore) gpa(dev, "vkQueueSignalSemaphore");
    table->QueueWaitSemaphore = (PFN_vkQueueWaitSemaphore) gpa(dev, "vkQueueWaitSemaphore");
    table->CreateEvent = (PFN_vkCreateEvent) gpa(dev, "vkCreateEvent");
    table->DestroyEvent = (PFN_vkDestroyEvent) gpa(dev, "vkDestroyEvent");
    table->GetEventStatus = (PFN_vkGetEventStatus) gpa(dev, "vkGetEventStatus");
    table->SetEvent = (PFN_vkSetEvent) gpa(dev, "vkSetEvent");
    table->ResetEvent = (PFN_vkResetEvent) gpa(dev, "vkResetEvent");
    table->CreateQueryPool = (PFN_vkCreateQueryPool) gpa(dev, "vkCreateQueryPool");
    table->DestroyQueryPool = (PFN_vkDestroyQueryPool) gpa(dev, "vkDestroyQueryPool");
    table->GetQueryPoolResults = (PFN_vkGetQueryPoolResults) gpa(dev, "vkGetQueryPoolResults");
    table->CreateBuffer = (PFN_vkCreateBuffer) gpa(dev, "vkCreateBuffer");
    table->DestroyBuffer = (PFN_vkDestroyBuffer) gpa(dev, "vkDestroyBuffer");
    table->CreateBufferView = (PFN_vkCreateBufferView) gpa(dev, "vkCreateBufferView");
    table->DestroyBufferView = (PFN_vkDestroyBufferView) gpa(dev, "vkDestroyBufferView");
    table->CreateImage = (PFN_vkCreateImage) gpa(dev, "vkCreateImage");
    table->DestroyImage = (PFN_vkDestroyImage) gpa(dev, "vkDestroyImage");
    table->GetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout) gpa(dev, "vkGetImageSubresourceLayout");
    table->CreateImageView = (PFN_vkCreateImageView) gpa(dev, "vkCreateImageView");
    table->DestroyImageView = (PFN_vkDestroyImageView) gpa(dev, "vkDestroyImageView");
    table->CreateAttachmentView = (PFN_vkCreateAttachmentView) gpa(dev, "vkCreateAttachmentView");
    table->DestroyAttachmentView = (PFN_vkDestroyAttachmentView) gpa(dev, "vkDestroyAttachmentView");
    table->CreateShaderModule = (PFN_vkCreateShaderModule) gpa(dev, "vkCreateShaderModule");
    table->DestroyShaderModule = (PFN_vkDestroyShaderModule) gpa(dev, "vkDestroyShaderModule");
    table->CreateShader = (PFN_vkCreateShader) gpa(dev, "vkCreateShader");
    table->DestroyShader = (PFN_vkDestroyShader) gpa(dev, "vkDestroyShader");
    table->CreatePipelineCache = (PFN_vkCreatePipelineCache) gpa(dev, "vkCreatePipelineCache");
    table->DestroyPipelineCache = (PFN_vkDestroyPipelineCache) gpa(dev, "vkDestroyPipelineCache");
    table->GetPipelineCacheSize = (PFN_vkGetPipelineCacheSize) gpa(dev, "vkGetPipelineCacheSize");
    table->GetPipelineCacheData = (PFN_vkGetPipelineCacheData) gpa(dev, "vkGetPipelineCacheData");
    table->MergePipelineCaches = (PFN_vkMergePipelineCaches) gpa(dev, "vkMergePipelineCaches");
    table->CreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines) gpa(dev, "vkCreateGraphicsPipelines");
    table->CreateComputePipelines = (PFN_vkCreateComputePipelines) gpa(dev, "vkCreateComputePipelines");
    table->DestroyPipeline = (PFN_vkDestroyPipeline) gpa(dev, "vkDestroyPipeline");
    table->CreatePipelineLayout = (PFN_vkCreatePipelineLayout) gpa(dev, "vkCreatePipelineLayout");
    table->DestroyPipelineLayout = (PFN_vkDestroyPipelineLayout) gpa(dev, "vkDestroyPipelineLayout");
    table->CreateSampler = (PFN_vkCreateSampler) gpa(dev, "vkCreateSampler");
    table->DestroySampler = (PFN_vkDestroySampler) gpa(dev, "vkDestroySampler");
    table->CreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout) gpa(dev, "vkCreateDescriptorSetLayout");
    table->DestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout) gpa(dev, "vkDestroyDescriptorSetLayout");
    table->CreateDescriptorPool = (PFN_vkCreateDescriptorPool) gpa(dev, "vkCreateDescriptorPool");
    table->DestroyDescriptorPool = (PFN_vkDestroyDescriptorPool) gpa(dev, "vkDestroyDescriptorPool");
    table->ResetDescriptorPool = (PFN_vkResetDescriptorPool) gpa(dev, "vkResetDescriptorPool");
    table->AllocDescriptorSets = (PFN_vkAllocDescriptorSets) gpa(dev, "vkAllocDescriptorSets");
    table->FreeDescriptorSets = (PFN_vkFreeDescriptorSets) gpa(dev, "vkFreeDescriptorSets");
    table->UpdateDescriptorSets = (PFN_vkUpdateDescriptorSets) gpa(dev, "vkUpdateDescriptorSets");
    table->CreateDynamicViewportState = (PFN_vkCreateDynamicViewportState) gpa(dev, "vkCreateDynamicViewportState");
    table->DestroyDynamicViewportState = (PFN_vkDestroyDynamicViewportState) gpa(dev, "vkDestroyDynamicViewportState");
    table->CreateDynamicRasterLineState = (PFN_vkCreateDynamicRasterLineState) gpa(dev, "vkCreateDynamicRasterLineState");
    table->DestroyDynamicRasterLineState = (PFN_vkDestroyDynamicRasterLineState) gpa(dev, "vkDestroyDynamicRasterLineState");
    table->CreateDynamicRasterDepthBiasState = (PFN_vkCreateDynamicRasterDepthBiasState) gpa(dev, "vkCreateDynamicRasterDepthBiasState");
    table->DestroyDynamicRasterDepthBiasState = (PFN_vkDestroyDynamicRasterDepthBiasState) gpa(dev, "vkDestroyDynamicRasterDepthBiasState");
    table->CreateDynamicColorBlendState = (PFN_vkCreateDynamicColorBlendState) gpa(dev, "vkCreateDynamicColorBlendState");
    table->DestroyDynamicColorBlendState = (PFN_vkDestroyDynamicColorBlendState) gpa(dev, "vkDestroyDynamicColorBlendState");
    table->CreateDynamicDepthState = (PFN_vkCreateDynamicDepthState) gpa(dev, "vkCreateDynamicDepthState");
    table->DestroyDynamicDepthState = (PFN_vkDestroyDynamicDepthState) gpa(dev, "vkDestroyDynamicDepthState");
    table->CreateDynamicStencilState = (PFN_vkCreateDynamicStencilState) gpa(dev, "vkCreateDynamicStencilState");
    table->DestroyDynamicStencilState = (PFN_vkDestroyDynamicStencilState) gpa(dev, "vkDestroyDynamicStencilState");
    table->CreateFramebuffer = (PFN_vkCreateFramebuffer) gpa(dev, "vkCreateFramebuffer");
    table->DestroyFramebuffer = (PFN_vkDestroyFramebuffer) gpa(dev, "vkDestroyFramebuffer");
    table->CreateRenderPass = (PFN_vkCreateRenderPass) gpa(dev, "vkCreateRenderPass");
    table->DestroyRenderPass = (PFN_vkDestroyRenderPass) gpa(dev, "vkDestroyRenderPass");
    table->GetRenderAreaGranularity = (PFN_vkGetRenderAreaGranularity) gpa(dev, "vkGetRenderAreaGranularity");
    table->CreateCommandPool = (PFN_vkCreateCommandPool) gpa(dev, "vkCreateCommandPool");
    table->DestroyCommandPool = (PFN_vkDestroyCommandPool) gpa(dev, "vkDestroyCommandPool");
    table->ResetCommandPool = (PFN_vkResetCommandPool) gpa(dev, "vkResetCommandPool");
    table->CreateCommandBuffer = (PFN_vkCreateCommandBuffer) gpa(dev, "vkCreateCommandBuffer");
    table->DestroyCommandBuffer = (PFN_vkDestroyCommandBuffer) gpa(dev, "vkDestroyCommandBuffer");
    table->BeginCommandBuffer = (PFN_vkBeginCommandBuffer) gpa(dev, "vkBeginCommandBuffer");
    table->EndCommandBuffer = (PFN_vkEndCommandBuffer) gpa(dev, "vkEndCommandBuffer");
    table->ResetCommandBuffer = (PFN_vkResetCommandBuffer) gpa(dev, "vkResetCommandBuffer");
    table->CmdBindPipeline = (PFN_vkCmdBindPipeline) gpa(dev, "vkCmdBindPipeline");
    table->CmdBindDynamicViewportState = (PFN_vkCmdBindDynamicViewportState) gpa(dev, "vkCmdBindDynamicViewportState");
    table->CmdBindDynamicRasterLineState = (PFN_vkCmdBindDynamicRasterLineState) gpa(dev, "vkCmdBindDynamicRasterLineState");
    table->CmdBindDynamicRasterDepthBiasState = (PFN_vkCmdBindDynamicRasterDepthBiasState) gpa(dev, "vkCmdBindDynamicRasterDepthBiasState");
    table->CmdBindDynamicColorBlendState = (PFN_vkCmdBindDynamicColorBlendState) gpa(dev, "vkCmdBindDynamicColorBlendState");
    table->CmdBindDynamicDepthState = (PFN_vkCmdBindDynamicDepthState) gpa(dev, "vkCmdBindDynamicDepthState");
    table->CmdBindDynamicStencilState = (PFN_vkCmdBindDynamicStencilState) gpa(dev, "vkCmdBindDynamicStencilState");
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
    table->CmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage) gpa(dev, "vkCmdClearDepthStencilImage");
    table->CmdClearColorAttachment = (PFN_vkCmdClearColorAttachment) gpa(dev, "vkCmdClearColorAttachment");
    table->CmdClearDepthStencilAttachment = (PFN_vkCmdClearDepthStencilAttachment) gpa(dev, "vkCmdClearDepthStencilAttachment");
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
    table->CmdPushConstants = (PFN_vkCmdPushConstants) gpa(dev, "vkCmdPushConstants");
    table->CmdBeginRenderPass = (PFN_vkCmdBeginRenderPass) gpa(dev, "vkCmdBeginRenderPass");
    table->CmdNextSubpass = (PFN_vkCmdNextSubpass) gpa(dev, "vkCmdNextSubpass");
    table->CmdEndRenderPass = (PFN_vkCmdEndRenderPass) gpa(dev, "vkCmdEndRenderPass");
    table->CmdExecuteCommands = (PFN_vkCmdExecuteCommands) gpa(dev, "vkCmdExecuteCommands");
//TODO move into it's own table
//TODO also consider dropping trampoline code for these device level extensions entirely
// then don't need loader to know about these at all but then not queryable via GIPA
    table->AcquireNextImageWSI = (PFN_vkAcquireNextImageWSI) gpa(dev, "vkAcquireNextImageWSI");
    table->CreateSwapChainWSI = (PFN_vkCreateSwapChainWSI) gpa(dev, "vkCreateSwapChainWSI");
    table->DestroySwapChainWSI = (PFN_vkDestroySwapChainWSI) gpa(dev, "vkDestroySwapChainWSI");
    table->GetSurfacePropertiesWSI = (PFN_vkGetSurfacePropertiesWSI) gpa(dev, "vkGetSurfacePropertiesWSI");
    table->GetSurfaceFormatsWSI = (PFN_vkGetSurfaceFormatsWSI) gpa(dev, "vkGetSurfaceFormatsWSI");
    table->GetSurfacePresentModesWSI = (PFN_vkGetSurfacePresentModesWSI) gpa(dev, "vkGetSurfacePresentModesWSI");
    table->GetSwapChainImagesWSI = (PFN_vkGetSwapChainImagesWSI) gpa(dev, "vkGetSwapChainImagesWSI");
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
    if (!strcmp(name, "GetDeviceMemoryCommitment"))
        return (void *) table->GetDeviceMemoryCommitment;
    if (!strcmp(name, "GetImageSparseMemoryRequirements"))
        return (void *) table->GetImageSparseMemoryRequirements;
    if (!strcmp(name, "GetBufferMemoryRequirements"))
        return (void *) table->GetBufferMemoryRequirements;
    if (!strcmp(name, "GetImageMemoryRequirements"))
        return (void *) table->GetImageMemoryRequirements;
    if (!strcmp(name, "BindBufferMemory"))
        return (void *) table->BindBufferMemory;
    if (!strcmp(name, "BindImageMemory"))
        return (void *) table->BindImageMemory;
    if (!strcmp(name, "QueueBindSparseBufferMemory"))
        return (void *) table->QueueBindSparseBufferMemory;
    if (!strcmp(name, "QueueBindSparseImageMemory"))
        return (void *) table->QueueBindSparseImageMemory;
    if (!strcmp(name, "QueueBindSparseImageOpaqueMemory"))
        return (void *) table->QueueBindSparseImageOpaqueMemory;
    if (!strcmp(name, "CreateFence"))
        return (void *) table->CreateFence;
    if (!strcmp(name, "DestroyFence"))
        return (void *) table->DestroyFence;
    if (!strcmp(name, "ResetFences"))
        return (void *) table->ResetFences;
    if (!strcmp(name, "GetFenceStatus"))
        return (void *) table->GetFenceStatus;
    if (!strcmp(name, "WaitForFences"))
        return (void *) table->WaitForFences;
    if (!strcmp(name, "CreateSemaphore"))
        return (void *) table->CreateSemaphore;
    if (!strcmp(name, "DestroySemaphore"))
        return (void *) table->DestroySemaphore;
    if (!strcmp(name, "QueueSignalSemaphore"))
        return (void *) table->QueueSignalSemaphore;
    if (!strcmp(name, "QueueWaitSemaphore"))
        return (void *) table->QueueWaitSemaphore;
    if (!strcmp(name, "CreateEvent"))
        return (void *) table->CreateEvent;
    if (!strcmp(name, "DestroyEvent"))
        return (void *) table->DestroyEvent;
    if (!strcmp(name, "GetEventStatus"))
        return (void *) table->GetEventStatus;
    if (!strcmp(name, "SetEvent"))
        return (void *) table->SetEvent;
    if (!strcmp(name, "ResetEvent"))
        return (void *) table->ResetEvent;
    if (!strcmp(name, "CreateQueryPool"))
        return (void *) table->CreateQueryPool;
    if (!strcmp(name, "DestroyQueryPool"))
        return (void *) table->DestroyQueryPool;
    if (!strcmp(name, "GetQueryPoolResults"))
        return (void *) table->GetQueryPoolResults;
    if (!strcmp(name, "CreateBuffer"))
        return (void *) table->CreateBuffer;
    if (!strcmp(name, "DestroyBuffer"))
        return (void *) table->DestroyBuffer;
    if (!strcmp(name, "CreateBufferView"))
        return (void *) table->CreateBufferView;
    if (!strcmp(name, "DestroyBufferView"))
        return (void *) table->DestroyBufferView;
    if (!strcmp(name, "CreateImage"))
        return (void *) table->CreateImage;
    if (!strcmp(name, "DestroyImage"))
        return (void *) table->DestroyImage;
    if (!strcmp(name, "GetImageSubresourceLayout"))
        return (void *) table->GetImageSubresourceLayout;
    if (!strcmp(name, "CreateImageView"))
        return (void *) table->CreateImageView;
    if (!strcmp(name, "DestroyImageView"))
        return (void *) table->DestroyImageView;
    if (!strcmp(name, "CreateAttachmentView"))
        return (void *) table->CreateAttachmentView;
    if (!strcmp(name, "DestroyAttachmentView"))
        return (void *) table->DestroyAttachmentView;
    if (!strcmp(name, "CreateShaderModule"))
        return (void *) table->CreateShaderModule;
    if (!strcmp(name, "DestroyShaderModule"))
        return (void *) table->DestroyShaderModule;
    if (!strcmp(name, "CreateShader"))
        return (void *) table->CreateShader;
    if (!strcmp(name, "DestroyShader"))
        return (void *) table->DestroyShader;
    if (!strcmp(name, "CreatePipelineCache"))
        return (void*) vkCreatePipelineCache;
    if (!strcmp(name, "DestroyPipelineCache"))
        return (void*) vkDestroyPipelineCache;
    if (!strcmp(name, "GetPipelineCacheSize"))
        return (void*) vkGetPipelineCacheSize;
    if (!strcmp(name, "GetPipelineCacheData"))
        return (void*) vkGetPipelineCacheData;
    if (!strcmp(name, "MergePipelineCaches"))
        return (void*) vkMergePipelineCaches;
    if (!strcmp(name, "CreateGraphicsPipelines"))
        return (void*) vkCreateGraphicsPipelines;
    if (!strcmp(name, "CreateComputePipelines"))
        return (void*) vkCreateComputePipelines;
    if (!strcmp(name, "DestroyPipeline"))
        return (void *) table->DestroyPipeline;
    if (!strcmp(name, "CreatePipelineLayout"))
        return (void *) table->CreatePipelineLayout;
    if (!strcmp(name, "DestroyPipelineLayout"))
        return (void *) table->DestroyPipelineLayout;
    if (!strcmp(name, "CreateSampler"))
        return (void *) table->CreateSampler;
    if (!strcmp(name, "DestroySampler"))
        return (void *) table->DestroySampler;
    if (!strcmp(name, "CreateDescriptorSetLayout"))
        return (void *) table->CreateDescriptorSetLayout;
    if (!strcmp(name, "DestroyDescriptorSetLayout"))
        return (void *) table->DestroyDescriptorSetLayout;
    if (!strcmp(name, "CreateDescriptorPool"))
        return (void *) table->CreateDescriptorPool;
    if (!strcmp(name, "DestroyDescriptorPool"))
        return (void *) table->DestroyDescriptorPool;
    if (!strcmp(name, "ResetDescriptorPool"))
        return (void *) table->ResetDescriptorPool;
    if (!strcmp(name, "AllocDescriptorSets"))
        return (void *) table->AllocDescriptorSets;
    if (!strcmp(name, "FreeDescriptorSets"))
        return (void *) table->FreeDescriptorSets;
    if (!strcmp(name, "UpdateDescriptorSets"))
        return (void *) table->UpdateDescriptorSets;
    if (!strcmp(name, "CreateDynamicViewportState"))
        return (void *) table->CreateDynamicViewportState;
    if (!strcmp(name, "DestroyDynamicViewportState"))
        return (void *) table->DestroyDynamicViewportState;
    if (!strcmp(name, "CreateDynamicRasterLineState"))
        return (void *) table->CreateDynamicRasterLineState;
    if (!strcmp(name, "DestroyDynamicRasterLineState"))
        return (void *) table->DestroyDynamicRasterLineState;
    if (!strcmp(name, "CreateDynamicRasterDepthBiasState"))
        return (void *) table->CreateDynamicRasterDepthBiasState;
    if (!strcmp(name, "DestroyDynamicRasterDepthBiasState"))
        return (void *) table->DestroyDynamicRasterDepthBiasState;
    if (!strcmp(name, "CreateDynamicColorBlendState"))
        return (void *) table->CreateDynamicColorBlendState;
    if (!strcmp(name, "DestroyDynamicColorBlendState"))
        return (void *) table->DestroyDynamicColorBlendState;
    if (!strcmp(name, "CreateDynamicDepthState"))
        return (void *) table->CreateDynamicDepthState;
    if (!strcmp(name, "DestroyDynamicDepthState"))
        return (void *) table->DestroyDynamicDepthState;
    if (!strcmp(name, "CreateDynamicStencilState"))
        return (void *) table->CreateDynamicStencilState;
    if (!strcmp(name, "DestroyDynamicStencilState"))
        return (void *) table->DestroyDynamicStencilState;
    if (!strcmp(name, "CreateFramebuffer"))
        return (void *) table->CreateFramebuffer;
    if (!strcmp(name, "DestroyFramebuffer"))
        return (void *) table->DestroyFramebuffer;
    if (!strcmp(name, "CreateRenderPass"))
        return (void *) table->CreateRenderPass;
    if (!strcmp(name, "DestroyRenderPass"))
        return (void *) table->DestroyRenderPass;
    if (!strcmp(name, "GetRenderAreaGranularity"))
        return (void *) table->GetRenderAreaGranularity;
    if (!strcmp(name, "CreateCommandPool"))
        return (void *) table->CreateCommandPool;
    if (!strcmp(name, "DestroyCommandPool"))
        return (void *) table->DestroyCommandPool;
    if (!strcmp(name, "ResetCommandPool"))
        return (void *) table->ResetCommandPool;
    if (!strcmp(name, "CreateCommandBuffer"))
        return (void *) table->CreateCommandBuffer;
    if (!strcmp(name, "DestroyCommandBuffer"))
        return (void *) table->DestroyCommandBuffer;
    if (!strcmp(name, "BeginCommandBuffer"))
        return (void *) table->BeginCommandBuffer;
    if (!strcmp(name, "EndCommandBuffer"))
        return (void *) table->EndCommandBuffer;
    if (!strcmp(name, "ResetCommandBuffer"))
        return (void *) table->ResetCommandBuffer;
    if (!strcmp(name, "CmdBindPipeline"))
        return (void *) table->CmdBindPipeline;
    if (!strcmp(name, "CmdBindDynamicViewportState"))
        return (void *) table->CmdBindDynamicViewportState;
    if (!strcmp(name, "CmdBindDynamicRasterLineState"))
        return (void *) table->CmdBindDynamicRasterLineState;
    if (!strcmp(name, "CmdBindDynamicRasterDepthBiasState"))
        return (void *) table->CmdBindDynamicRasterDepthBiasState;
    if (!strcmp(name, "CmdBindDynamicColorBlendState"))
        return (void *) table->CmdBindDynamicColorBlendState;
    if (!strcmp(name, "CmdBindDynamicDepthState"))
        return (void *) table->CmdBindDynamicDepthState;
    if (!strcmp(name, "CmdBindDynamicStencilState"))
        return (void *) table->CmdBindDynamicStencilState;
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
    if (!strcmp(name, "CmdClearDepthStencilImage"))
        return (void *) table->CmdClearDepthStencilImage;
    if (!strcmp(name, "CmdClearColorAttachment"))
        return (void *) table->CmdClearColorAttachment;
    if (!strcmp(name, "CmdClearDepthStencilAttachment"))
        return (void *) table->CmdClearDepthStencilAttachment;
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
    if (!strcmp(name, "CmdPushConstants"))
        return (void *) table->CmdPushConstants;
    if (!strcmp(name, "CmdBeginRenderPass"))
        return (void *) table->CmdBeginRenderPass;
    if (!strcmp(name, "CmdNextSubpass"))
        return (void *) table->CmdNextSubpass;
    if (!strcmp(name, "CmdEndRenderPass"))
        return (void *) table->CmdEndRenderPass;
    if (!strcmp(name, "CmdExecuteCommands"))
        return (void *) table->CmdExecuteCommands;

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
    table->GetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties) gpa(inst, "vkGetPhysicalDeviceImageFormatProperties");
    table->GetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties) gpa(inst, "vkGetPhysicalDeviceFormatProperties");
    table->GetPhysicalDeviceLimits = (PFN_vkGetPhysicalDeviceLimits) gpa(inst, "vkGetPhysicalDeviceLimits");
    table->GetPhysicalDeviceSparseImageFormatProperties = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties) gpa(inst, "vkGetPhysicalDeviceSparseImageFormatProperties");
    table->GetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) gpa(inst, "vkGetPhysicalDeviceProperties");
    table->GetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) gpa(inst, "vkGetPhysicalDeviceQueueFamilyProperties");
    table->GetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties) gpa(inst, "vkGetPhysicalDeviceMemoryProperties");
    table->GetPhysicalDeviceExtensionProperties = (PFN_vkGetPhysicalDeviceExtensionProperties) gpa(inst, "vkGetPhysicalDeviceExtensionProperties");
    table->GetPhysicalDeviceLayerProperties = (PFN_vkGetPhysicalDeviceLayerProperties) gpa(inst, "vkGetPhysicalDeviceLayerProperties");
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
    if (!strcmp(name, "GetPhysicalDeviceImageFormatProperties"))
        return (void *) table->GetPhysicalDeviceImageFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceFormatProperties"))
        return (void *) table->GetPhysicalDeviceFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceLimits"))
        return (void *) table->GetPhysicalDeviceLimits;
    if (!strcmp(name, "GetPhysicalDeviceSparseImageFormatProperties"))
        return (void *) table->GetPhysicalDeviceSparseImageFormatProperties;
    if (!strcmp(name, "GetPhysicalDeviceProperties"))
        return (void *) table->GetPhysicalDeviceProperties;
    if (!strcmp(name, "GetPhysicalDeviceQueueFamilyProperties"))
        return (void *) table->GetPhysicalDeviceQueueFamilyProperties;
    if (!strcmp(name, "GetPhysicalDeviceMemoryProperties"))
        return (void *) table->GetPhysicalDeviceMemoryProperties;
    if (!strcmp(name, "GetInstanceProcAddr"))
        return (void *) table->GetInstanceProcAddr;
    if (!strcmp(name, "GetPhysicalDeviceExtensionProperties"))
        return (void *) table->GetPhysicalDeviceExtensionProperties;
    if (!strcmp(name, "GetPhysicalDeviceLayerProperties"))
        return (void *) table->GetPhysicalDeviceLayerProperties;
    if (!strcmp(name, "GetPhysicalDeviceSurfaceSupportWSI"))
        return (void *) table->GetPhysicalDeviceSurfaceSupportWSI;
    if (!strcmp(name, "DbgCreateMsgCallback"))
        return (void *) table->DbgCreateMsgCallback;
    if (!strcmp(name, "DbgDestroyMsgCallback"))
        return (void *) table->DbgDestroyMsgCallback;

    return NULL;
}
