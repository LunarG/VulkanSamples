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

#include <string.h>

static inline void* globalGetProcAddr(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (void*) vkCreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (void*) vkDestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (void*) vkEnumeratePhysicalDevices;
    if (!strcmp(name, "GetPhysicalDeviceInfo"))
        return (void*) vkGetPhysicalDeviceInfo;
    if (!strcmp(name, "GetInstanceProcAddr"))
        return (void*) vkGetInstanceProcAddr;
    if (!strcmp(name, "GetProcAddr"))
        return (void*) vkGetProcAddr;
    if (!strcmp(name, "CreateDevice"))
        return (void*) vkCreateDevice;
    if (!strcmp(name, "DestroyDevice"))
        return (void*) vkDestroyDevice;
    if (!strcmp(name, "GetGlobalExtensionInfo"))
        return (void*) vkGetGlobalExtensionInfo;
    if (!strcmp(name, "GetPhysicalDeviceExtensionInfo"))
        return (void*) vkGetPhysicalDeviceExtensionInfo;
    if (!strcmp(name, "EnumerateLayers"))
        return (void*) vkEnumerateLayers;
    if (!strcmp(name, "GetDeviceQueue"))
        return (void*) vkGetDeviceQueue;
    if (!strcmp(name, "QueueSubmit"))
        return (void*) vkQueueSubmit;
    if (!strcmp(name, "QueueWaitIdle"))
        return (void*) vkQueueWaitIdle;
    if (!strcmp(name, "DeviceWaitIdle"))
        return (void*) vkDeviceWaitIdle;
    if (!strcmp(name, "AllocMemory"))
        return (void*) vkAllocMemory;
    if (!strcmp(name, "FreeMemory"))
        return (void*) vkFreeMemory;
    if (!strcmp(name, "SetMemoryPriority"))
        return (void*) vkSetMemoryPriority;
    if (!strcmp(name, "MapMemory"))
        return (void*) vkMapMemory;
    if (!strcmp(name, "UnmapMemory"))
        return (void*) vkUnmapMemory;
    if (!strcmp(name, "FlushMappedMemoryRanges"))
        return (void*) vkFlushMappedMemoryRanges;
    if (!strcmp(name, "InvalidateMappedMemoryRanges"))
        return (void*) vkInvalidateMappedMemoryRanges;
    if (!strcmp(name, "PinSystemMemory"))
        return (void*) vkPinSystemMemory;
    if (!strcmp(name, "GetMultiDeviceCompatibility"))
        return (void*) vkGetMultiDeviceCompatibility;
    if (!strcmp(name, "OpenSharedMemory"))
        return (void*) vkOpenSharedMemory;
    if (!strcmp(name, "OpenSharedSemaphore"))
        return (void*) vkOpenSharedSemaphore;
    if (!strcmp(name, "OpenPeerMemory"))
        return (void*) vkOpenPeerMemory;
    if (!strcmp(name, "OpenPeerImage"))
        return (void*) vkOpenPeerImage;
    if (!strcmp(name, "DestroyObject"))
        return (void*) vkDestroyObject;
    if (!strcmp(name, "GetObjectInfo"))
        return (void*) vkGetObjectInfo;
    if (!strcmp(name, "QueueBindObjectMemory"))
        return (void*) vkQueueBindObjectMemory;
    if (!strcmp(name, "QueueBindObjectMemoryRange"))
        return (void*) vkQueueBindObjectMemoryRange;
    if (!strcmp(name, "QueueBindImageMemoryRange"))
        return (void*) vkQueueBindImageMemoryRange;
    if (!strcmp(name, "CreateFence"))
        return (void*) vkCreateFence;
    if (!strcmp(name, "ResetFences"))
        return (void*) vkResetFences;
    if (!strcmp(name, "GetFenceStatus"))
        return (void*) vkGetFenceStatus;
    if (!strcmp(name, "WaitForFences"))
        return (void*) vkWaitForFences;
    if (!strcmp(name, "CreateSemaphore"))
        return (void*) vkCreateSemaphore;
    if (!strcmp(name, "QueueSignalSemaphore"))
        return (void*) vkQueueSignalSemaphore;
    if (!strcmp(name, "QueueWaitSemaphore"))
        return (void*) vkQueueWaitSemaphore;
    if (!strcmp(name, "CreateEvent"))
        return (void*) vkCreateEvent;
    if (!strcmp(name, "GetEventStatus"))
        return (void*) vkGetEventStatus;
    if (!strcmp(name, "SetEvent"))
        return (void*) vkSetEvent;
    if (!strcmp(name, "ResetEvent"))
        return (void*) vkResetEvent;
    if (!strcmp(name, "CreateQueryPool"))
        return (void*) vkCreateQueryPool;
    if (!strcmp(name, "GetQueryPoolResults"))
        return (void*) vkGetQueryPoolResults;
    if (!strcmp(name, "GetFormatInfo"))
        return (void*) vkGetFormatInfo;
    if (!strcmp(name, "CreateBuffer"))
        return (void*) vkCreateBuffer;
    if (!strcmp(name, "CreateBufferView"))
        return (void*) vkCreateBufferView;
    if (!strcmp(name, "CreateImage"))
        return (void*) vkCreateImage;
    if (!strcmp(name, "GetImageSubresourceInfo"))
        return (void*) vkGetImageSubresourceInfo;
    if (!strcmp(name, "CreateImageView"))
        return (void*) vkCreateImageView;
    if (!strcmp(name, "CreateColorAttachmentView"))
        return (void*) vkCreateColorAttachmentView;
    if (!strcmp(name, "CreateDepthStencilView"))
        return (void*) vkCreateDepthStencilView;
    if (!strcmp(name, "CreateShader"))
        return (void*) vkCreateShader;
    if (!strcmp(name, "CreateGraphicsPipeline"))
        return (void*) vkCreateGraphicsPipeline;
    if (!strcmp(name, "CreateGraphicsPipelineDerivative"))
        return (void*) vkCreateGraphicsPipelineDerivative;
    if (!strcmp(name, "CreateComputePipeline"))
        return (void*) vkCreateComputePipeline;
    if (!strcmp(name, "StorePipeline"))
        return (void*) vkStorePipeline;
    if (!strcmp(name, "LoadPipeline"))
        return (void*) vkLoadPipeline;
    if (!strcmp(name, "LoadPipelineDerivative"))
        return (void*) vkLoadPipelineDerivative;
    if (!strcmp(name, "CreatePipelineLayout"))
        return (void*) vkCreatePipelineLayout;
    if (!strcmp(name, "CreateSampler"))
        return (void*) vkCreateSampler;
    if (!strcmp(name, "CreateDescriptorSetLayout"))
        return (void*) vkCreateDescriptorSetLayout;
    if (!strcmp(name, "BeginDescriptorPoolUpdate"))
        return (void*) vkBeginDescriptorPoolUpdate;
    if (!strcmp(name, "EndDescriptorPoolUpdate"))
        return (void*) vkEndDescriptorPoolUpdate;
    if (!strcmp(name, "CreateDescriptorPool"))
        return (void*) vkCreateDescriptorPool;
    if (!strcmp(name, "ResetDescriptorPool"))
        return (void*) vkResetDescriptorPool;
    if (!strcmp(name, "AllocDescriptorSets"))
        return (void*) vkAllocDescriptorSets;
    if (!strcmp(name, "ClearDescriptorSets"))
        return (void*) vkClearDescriptorSets;
    if (!strcmp(name, "UpdateDescriptors"))
        return (void*) vkUpdateDescriptors;
    if (!strcmp(name, "CreateDynamicViewportState"))
        return (void*) vkCreateDynamicViewportState;
    if (!strcmp(name, "CreateDynamicRasterState"))
        return (void*) vkCreateDynamicRasterState;
    if (!strcmp(name, "CreateDynamicColorBlendState"))
        return (void*) vkCreateDynamicColorBlendState;
    if (!strcmp(name, "CreateDynamicDepthStencilState"))
        return (void*) vkCreateDynamicDepthStencilState;
    if (!strcmp(name, "CreateCommandBuffer"))
        return (void*) vkCreateCommandBuffer;
    if (!strcmp(name, "BeginCommandBuffer"))
        return (void*) vkBeginCommandBuffer;
    if (!strcmp(name, "EndCommandBuffer"))
        return (void*) vkEndCommandBuffer;
    if (!strcmp(name, "ResetCommandBuffer"))
        return (void*) vkResetCommandBuffer;
    if (!strcmp(name, "CmdBindPipeline"))
        return (void*) vkCmdBindPipeline;
    if (!strcmp(name, "CmdBindDynamicStateObject"))
        return (void*) vkCmdBindDynamicStateObject;
    if (!strcmp(name, "CmdBindDescriptorSets"))
        return (void*) vkCmdBindDescriptorSets;
    if (!strcmp(name, "CmdBindVertexBuffers"))
        return (void*) vkCmdBindVertexBuffers;
    if (!strcmp(name, "CmdBindIndexBuffer"))
        return (void*) vkCmdBindIndexBuffer;
    if (!strcmp(name, "CmdDraw"))
        return (void*) vkCmdDraw;
    if (!strcmp(name, "CmdDrawIndexed"))
        return (void*) vkCmdDrawIndexed;
    if (!strcmp(name, "CmdDrawIndirect"))
        return (void*) vkCmdDrawIndirect;
    if (!strcmp(name, "CmdDrawIndexedIndirect"))
        return (void*) vkCmdDrawIndexedIndirect;
    if (!strcmp(name, "CmdDispatch"))
        return (void*) vkCmdDispatch;
    if (!strcmp(name, "CmdDispatchIndirect"))
        return (void*) vkCmdDispatchIndirect;
    if (!strcmp(name, "CmdCopyBuffer"))
        return (void*) vkCmdCopyBuffer;
    if (!strcmp(name, "CmdCopyImage"))
        return (void*) vkCmdCopyImage;
    if (!strcmp(name, "CmdBlitImage"))
        return (void*) vkCmdBlitImage;
    if (!strcmp(name, "CmdCopyBufferToImage"))
        return (void*) vkCmdCopyBufferToImage;
    if (!strcmp(name, "CmdCopyImageToBuffer"))
        return (void*) vkCmdCopyImageToBuffer;
    if (!strcmp(name, "CmdUpdateBuffer"))
        return (void*) vkCmdUpdateBuffer;
    if (!strcmp(name, "CmdFillBuffer"))
        return (void*) vkCmdFillBuffer;
    if (!strcmp(name, "CmdClearColorImage"))
        return (void*) vkCmdClearColorImage;
    if (!strcmp(name, "CmdClearDepthStencil"))
        return (void*) vkCmdClearDepthStencil;
    if (!strcmp(name, "CmdResolveImage"))
        return (void*) vkCmdResolveImage;
    if (!strcmp(name, "CmdSetEvent"))
        return (void*) vkCmdSetEvent;
    if (!strcmp(name, "CmdResetEvent"))
        return (void*) vkCmdResetEvent;
    if (!strcmp(name, "CmdWaitEvents"))
        return (void*) vkCmdWaitEvents;
    if (!strcmp(name, "CmdPipelineBarrier"))
        return (void*) vkCmdPipelineBarrier;
    if (!strcmp(name, "CmdBeginQuery"))
        return (void*) vkCmdBeginQuery;
    if (!strcmp(name, "CmdEndQuery"))
        return (void*) vkCmdEndQuery;
    if (!strcmp(name, "CmdResetQueryPool"))
        return (void*) vkCmdResetQueryPool;
    if (!strcmp(name, "CmdWriteTimestamp"))
        return (void*) vkCmdWriteTimestamp;
    if (!strcmp(name, "CmdCopyQueryPoolResults"))
        return (void*) vkCmdCopyQueryPoolResults;
    if (!strcmp(name, "CmdInitAtomicCounters"))
        return (void*) vkCmdInitAtomicCounters;
    if (!strcmp(name, "CmdLoadAtomicCounters"))
        return (void*) vkCmdLoadAtomicCounters;
    if (!strcmp(name, "CmdSaveAtomicCounters"))
        return (void*) vkCmdSaveAtomicCounters;
    if (!strcmp(name, "CreateFramebuffer"))
        return (void*) vkCreateFramebuffer;
    if (!strcmp(name, "CreateRenderPass"))
        return (void*) vkCreateRenderPass;
    if (!strcmp(name, "CmdBeginRenderPass"))
        return (void*) vkCmdBeginRenderPass;
    if (!strcmp(name, "CmdEndRenderPass"))
        return (void*) vkCmdEndRenderPass;
    if (!strcmp(name, "DbgSetValidationLevel"))
        return (void*) vkDbgSetValidationLevel;
    if (!strcmp(name, "DbgRegisterMsgCallback"))
        return (void*) vkDbgRegisterMsgCallback;
    if (!strcmp(name, "DbgUnregisterMsgCallback"))
        return (void*) vkDbgUnregisterMsgCallback;
    if (!strcmp(name, "DbgSetMessageFilter"))
        return (void*) vkDbgSetMessageFilter;
    if (!strcmp(name, "DbgSetObjectTag"))
        return (void*) vkDbgSetObjectTag;
    if (!strcmp(name, "DbgSetGlobalOption"))
        return (void*) vkDbgSetGlobalOption;
    if (!strcmp(name, "DbgSetDeviceOption"))
        return (void*) vkDbgSetDeviceOption;
    if (!strcmp(name, "CmdDbgMarkerBegin"))
        return (void*) vkCmdDbgMarkerBegin;
    if (!strcmp(name, "CmdDbgMarkerEnd"))
        return (void*) vkCmdDbgMarkerEnd;
    if (!strcmp(name, "GetDisplayInfoWSI"))
        return (void*) vkGetDisplayInfoWSI;
    if (!strcmp(name, "CreateSwapChainWSI"))
        return (void*) vkCreateSwapChainWSI;
    if (!strcmp(name, "DestroySwapChainWSI"))
        return (void*) vkDestroySwapChainWSI;
    if (!strcmp(name, "GetSwapChainInfoWSI"))
        return (void*) vkGetSwapChainInfoWSI;
    if (!strcmp(name, "QueuePresentWSI"))
        return (void*) vkQueuePresentWSI;

    return NULL;
}

/* These functions require special handling by the loader.
*  They are not just generic trampoline code entrypoints.
*  Thus GPA must return loader entrypoint for these instead of first function
*  in the chain. */
static inline void *loader_non_passthrough_gpa(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (void*) vkCreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (void*) vkDestroyInstance;
    if (!strcmp(name, "EnumeratePhysicalDevices"))
        return (void*) vkEnumeratePhysicalDevices;
    if (!strcmp(name, "GetPhysicalDeviceInfo"))
        return (void*) vkGetPhysicalDeviceInfo;
    if (!strcmp(name, "GetInstanceProcAddr"))
        return (void*) vkGetInstanceProcAddr;
    if (!strcmp(name, "GetProcAddr"))
        return (void*) vkGetProcAddr;
    if (!strcmp(name, "CreateDevice"))
        return (void*) vkCreateDevice;
    if (!strcmp(name, "GetGlobalExtensionInfo"))
        return (void*) vkGetGlobalExtensionInfo;
    if (!strcmp(name, "EnumerateLayers"))
        return (void*) vkEnumerateLayers;
    if (!strcmp(name, "GetDeviceQueue"))
        return (void*) vkGetDeviceQueue;
    if (!strcmp(name, "CreateCommandBuffer"))
        return (void*) vkCreateCommandBuffer;
    if (!strcmp(name, "DbgRegisterMsgCallback"))
        return (void*) vkDbgRegisterMsgCallback;
    if (!strcmp(name, "DbgUnregisterMsgCallback"))
        return (void*) vkDbgUnregisterMsgCallback;
    if (!strcmp(name, "DbgSetGlobalOption"))
        return (void*) vkDbgSetGlobalOption;
    if (!strcmp(name, "CreateSwapChainWSI"))
        return (void*) vkCreateSwapChainWSI;

    return NULL;
}
