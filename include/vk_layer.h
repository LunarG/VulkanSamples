/* Need to define dispatch table
 * Core struct can then have ptr to dispatch table at the top
 * Along with object ptrs for current and next OBJ
 */
#pragma once

#include "vulkan.h"
#include "vk_debug_report_lunarg.h"
#include "vk_debug_marker_lunarg.h"
#include "vk_wsi_lunarg.h"
#include "vk_wsi_lunarg.h"
#if defined(__GNUC__) && __GNUC__ >= 4
#  define VK_LAYER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#  define VK_LAYER_EXPORT __attribute__((visibility("default")))
#else
#  define VK_LAYER_EXPORT
#endif

typedef void * (*PFN_vkGPA)(VkObject obj, const char * pName);
typedef struct VkBaseLayerObject_
{
    PFN_vkGPA pGPA;
    VkObject nextObject;
    VkObject baseObject;
} VkBaseLayerObject;

typedef struct VkLayerDispatchTable_
{
    PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
    PFN_vkCreateDevice CreateDevice;
    PFN_vkDestroyDevice DestroyDevice;
    PFN_vkGetDeviceQueue GetDeviceQueue;
    PFN_vkQueueSubmit QueueSubmit;
    PFN_vkQueueWaitIdle QueueWaitIdle;
    PFN_vkDeviceWaitIdle DeviceWaitIdle;
    PFN_vkAllocMemory AllocMemory;
    PFN_vkFreeMemory FreeMemory;
    PFN_vkMapMemory MapMemory;
    PFN_vkUnmapMemory UnmapMemory;
    PFN_vkFlushMappedMemoryRanges FlushMappedMemoryRanges;
    PFN_vkInvalidateMappedMemoryRanges InvalidateMappedMemoryRanges;
    PFN_vkDestroyObject DestroyObject;
    PFN_vkGetObjectMemoryRequirements GetObjectMemoryRequirements;
    PFN_vkBindObjectMemory BindObjectMemory;
    PFN_vkGetImageSparseMemoryRequirements GetImageSparseMemoryRequirements;
    PFN_vkQueueBindSparseBufferMemory QueueBindSparseBufferMemory;
    PFN_vkQueueBindSparseImageOpaqueMemory QueueBindSparseImageOpaqueMemory;
    PFN_vkQueueBindSparseImageMemory QueueBindSparseImageMemory;
    PFN_vkCreateFence CreateFence;
    PFN_vkGetFenceStatus GetFenceStatus;
    PFN_vkResetFences ResetFences;
    PFN_vkWaitForFences WaitForFences;
    PFN_vkCreateSemaphore CreateSemaphore;
    PFN_vkQueueSignalSemaphore QueueSignalSemaphore;
    PFN_vkQueueWaitSemaphore QueueWaitSemaphore;
    PFN_vkCreateEvent CreateEvent;
    PFN_vkGetEventStatus GetEventStatus;
    PFN_vkSetEvent SetEvent;
    PFN_vkResetEvent ResetEvent;
    PFN_vkCreateQueryPool CreateQueryPool;
    PFN_vkGetQueryPoolResults GetQueryPoolResults;
    PFN_vkCreateBuffer CreateBuffer;
    PFN_vkCreateBufferView CreateBufferView;
    PFN_vkCreateImage CreateImage;
    PFN_vkGetImageSubresourceLayout GetImageSubresourceLayout;
    PFN_vkCreateImageView CreateImageView;
    PFN_vkCreateAttachmentView CreateAttachmentView;
    PFN_vkCreateShaderModule CreateShaderModule;
    PFN_vkCreateShader CreateShader;
    PFN_vkCreatePipelineCache CreatePipelineCache;
    PFN_vkDestroyPipelineCache DestroyPipelineCache;
    PFN_vkGetPipelineCacheSize GetPipelineCacheSize;
    PFN_vkGetPipelineCacheData GetPipelineCacheData;
    PFN_vkMergePipelineCaches MergePipelineCaches;
    PFN_vkCreateGraphicsPipelines CreateGraphicsPipelines;
    PFN_vkCreateComputePipelines CreateComputePipelines;
    PFN_vkCreatePipelineLayout CreatePipelineLayout;
    PFN_vkCreateSampler CreateSampler;
    PFN_vkCreateDescriptorSetLayout CreateDescriptorSetLayout;
    PFN_vkCreateDescriptorPool CreateDescriptorPool;
    PFN_vkResetDescriptorPool ResetDescriptorPool;
    PFN_vkAllocDescriptorSets AllocDescriptorSets;
    PFN_vkUpdateDescriptorSets UpdateDescriptorSets;
    PFN_vkCreateDynamicViewportState CreateDynamicViewportState;
    PFN_vkCreateDynamicRasterState CreateDynamicRasterState;
    PFN_vkCreateDynamicColorBlendState CreateDynamicColorBlendState;
    PFN_vkCreateDynamicDepthStencilState CreateDynamicDepthStencilState;
    PFN_vkCreateCommandBuffer CreateCommandBuffer;
    PFN_vkBeginCommandBuffer BeginCommandBuffer;
    PFN_vkEndCommandBuffer EndCommandBuffer;
    PFN_vkResetCommandBuffer ResetCommandBuffer;
    PFN_vkCmdBindPipeline CmdBindPipeline;
    PFN_vkCmdBindDynamicStateObject CmdBindDynamicStateObject;
    PFN_vkCmdBindDescriptorSets CmdBindDescriptorSets;
    PFN_vkCmdBindVertexBuffers CmdBindVertexBuffers;
    PFN_vkCmdBindIndexBuffer CmdBindIndexBuffer;
    PFN_vkCmdDraw CmdDraw;
    PFN_vkCmdDrawIndexed CmdDrawIndexed;
    PFN_vkCmdDrawIndirect CmdDrawIndirect;
    PFN_vkCmdDrawIndexedIndirect CmdDrawIndexedIndirect;
    PFN_vkCmdDispatch CmdDispatch;
    PFN_vkCmdDispatchIndirect CmdDispatchIndirect;
    PFN_vkCmdCopyBuffer CmdCopyBuffer;
    PFN_vkCmdCopyImage CmdCopyImage;
    PFN_vkCmdBlitImage CmdBlitImage;
    PFN_vkCmdCopyBufferToImage CmdCopyBufferToImage;
    PFN_vkCmdCopyImageToBuffer CmdCopyImageToBuffer;
    PFN_vkCmdUpdateBuffer CmdUpdateBuffer;
    PFN_vkCmdFillBuffer CmdFillBuffer;
    PFN_vkCmdClearColorImage CmdClearColorImage;
    PFN_vkCmdClearDepthStencilImage CmdClearDepthStencilImage;
    PFN_vkCmdClearColorAttachment CmdClearColorAttachment;
    PFN_vkCmdClearDepthStencilAttachment CmdClearDepthStencilAttachment;
    PFN_vkCmdResolveImage CmdResolveImage;
    PFN_vkCmdSetEvent CmdSetEvent;
    PFN_vkCmdResetEvent CmdResetEvent;
    PFN_vkCmdWaitEvents CmdWaitEvents;
    PFN_vkCmdPipelineBarrier CmdPipelineBarrier;
    PFN_vkCmdBeginQuery CmdBeginQuery;
    PFN_vkCmdEndQuery CmdEndQuery;
    PFN_vkCmdResetQueryPool CmdResetQueryPool;
    PFN_vkCmdWriteTimestamp CmdWriteTimestamp;
    PFN_vkCmdCopyQueryPoolResults CmdCopyQueryPoolResults;
    PFN_vkCreateFramebuffer CreateFramebuffer;
    PFN_vkCreateRenderPass CreateRenderPass;
    PFN_vkCmdBeginRenderPass CmdBeginRenderPass;
    PFN_vkCmdNextSubpass CmdNextSubpass;
    PFN_vkCmdEndRenderPass CmdEndRenderPass;
    PFN_vkCmdExecuteCommands CmdExecuteCommands;
    PFN_vkCreateSwapChainWSI CreateSwapChainWSI;
    PFN_vkDestroySwapChainWSI DestroySwapChainWSI;
    PFN_vkGetSwapChainInfoWSI GetSwapChainInfoWSI;
    PFN_vkQueuePresentWSI QueuePresentWSI;
    PFN_vkDbgCreateMsgCallback DbgCreateMsgCallback;
    PFN_vkDbgDestroyMsgCallback DbgDestroyMsgCallback;
    PFN_vkDbgStringCallback DbgStringCallback;
    PFN_vkDbgStdioCallback DbgStdioCallback;
    PFN_vkDbgBreakCallback DbgBreakCallback;
} VkLayerDispatchTable;

typedef struct VkLayerInstanceDispatchTable_
{
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
    PFN_vkCreateInstance CreateInstance;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceFeatures GetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceFormatInfo GetPhysicalDeviceFormatInfo;
    PFN_vkGetPhysicalDeviceLimits GetPhysicalDeviceLimits;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties GetPhysicalDeviceSparseImageFormatProperties;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDevicePerformance GetPhysicalDevicePerformance;
    PFN_vkGetPhysicalDeviceQueueCount GetPhysicalDeviceQueueCount;
    PFN_vkGetPhysicalDeviceQueueProperties GetPhysicalDeviceQueueProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceExtensionProperties GetPhysicalDeviceExtensionProperties;
    PFN_vkGetPhysicalDeviceLayerProperties GetPhysicalDeviceLayerProperties;
    PFN_vkDbgCreateMsgCallback DbgCreateMsgCallback;
    PFN_vkDbgDestroyMsgCallback DbgDestroyMsgCallback;
    PFN_vkDbgStringCallback DbgStringCallback;
    PFN_vkDbgStdioCallback DbgStdioCallback;
    PFN_vkDbgBreakCallback DbgBreakCallback;
} VkLayerInstanceDispatchTable;

// LL node for tree of dbg callback functions
typedef struct VkLayerDbgFunctionNode_
{
    VkDbgMsgCallback msgCallback;
    PFN_vkDbgMsgCallback pfnMsgCallback;
    VkFlags msgFlags;
    const void *pUserData;
    struct VkLayerDbgFunctionNode_ *pNext;
} VkLayerDbgFunctionNode;

typedef enum VkLayerDbgAction_
{
    VK_DBG_LAYER_ACTION_IGNORE = 0x0,
    VK_DBG_LAYER_ACTION_CALLBACK = 0x1,
    VK_DBG_LAYER_ACTION_LOG_MSG = 0x2,
    VK_DBG_LAYER_ACTION_BREAK = 0x4
} VkLayerDbgAction;

// ------------------------------------------------------------------------------------------------
// API functions
