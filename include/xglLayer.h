/* Need to define dispatch table
 * Core struct can then have ptr to dispatch table at the top
 * Along with object ptrs for current and next OBJ
 */
#pragma once

#include "vulkan.h"
#include "vkDbg.h"
#if defined(__linux__) || defined(XCB_NVIDIA)
#include "vkWsiX11Ext.h"
#endif
#if defined(__GNUC__) && __GNUC__ >= 4
#  define VK_LAYER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#  define VK_LAYER_EXPORT __attribute__((visibility("default")))
#else
#  define VK_LAYER_EXPORT
#endif


typedef struct _VK_BASE_LAYER_OBJECT
{
    vkGetProcAddrType pGPA;
    VK_BASE_OBJECT nextObject;
    VK_BASE_OBJECT baseObject;
} VK_BASE_LAYER_OBJECT;

typedef struct _VK_LAYER_DISPATCH_TABLE
{
    vkGetProcAddrType GetProcAddr;
    vkCreateInstanceType CreateInstance;
    vkDestroyInstanceType DestroyInstance;
    vkEnumerateGpusType EnumerateGpus;
    vkGetGpuInfoType GetGpuInfo;
    vkCreateDeviceType CreateDevice;
    vkDestroyDeviceType DestroyDevice;
    vkGetExtensionSupportType GetExtensionSupport;
    vkEnumerateLayersType EnumerateLayers;
    vkGetDeviceQueueType GetDeviceQueue;
    vkQueueSubmitType QueueSubmit;
    vkQueueAddMemReferenceType QueueAddMemReference;
    vkQueueRemoveMemReferenceType QueueRemoveMemReference;
    vkQueueWaitIdleType QueueWaitIdle;
    vkDeviceWaitIdleType DeviceWaitIdle;
    vkAllocMemoryType AllocMemory;
    vkFreeMemoryType FreeMemory;
    vkSetMemoryPriorityType SetMemoryPriority;
    vkMapMemoryType MapMemory;
    vkUnmapMemoryType UnmapMemory;
    vkPinSystemMemoryType PinSystemMemory;
    vkGetMultiGpuCompatibilityType GetMultiGpuCompatibility;
    vkOpenSharedMemoryType OpenSharedMemory;
    vkOpenSharedSemaphoreType OpenSharedSemaphore;
    vkOpenPeerMemoryType OpenPeerMemory;
    vkOpenPeerImageType OpenPeerImage;
    vkDestroyObjectType DestroyObject;
    vkGetObjectInfoType GetObjectInfo;
    vkBindObjectMemoryType BindObjectMemory;
    vkBindObjectMemoryRangeType BindObjectMemoryRange;
    vkBindImageMemoryRangeType BindImageMemoryRange;
    vkCreateFenceType CreateFence;
    vkGetFenceStatusType GetFenceStatus;
    vkResetFencesType ResetFences;
    vkWaitForFencesType WaitForFences;
    vkCreateSemaphoreType CreateSemaphore;
    vkQueueSignalSemaphoreType QueueSignalSemaphore;
    vkQueueWaitSemaphoreType QueueWaitSemaphore;
    vkCreateEventType CreateEvent;
    vkGetEventStatusType GetEventStatus;
    vkSetEventType SetEvent;
    vkResetEventType ResetEvent;
    vkCreateQueryPoolType CreateQueryPool;
    vkGetQueryPoolResultsType GetQueryPoolResults;
    vkGetFormatInfoType GetFormatInfo;
    vkCreateBufferType CreateBuffer;
    vkCreateBufferViewType CreateBufferView;
    vkCreateImageType CreateImage;
    vkGetImageSubresourceInfoType GetImageSubresourceInfo;
    vkCreateImageViewType CreateImageView;
    vkCreateColorAttachmentViewType CreateColorAttachmentView;
    vkCreateDepthStencilViewType CreateDepthStencilView;
    vkCreateShaderType CreateShader;
    vkCreateGraphicsPipelineType CreateGraphicsPipeline;
    vkCreateGraphicsPipelineDerivativeType CreateGraphicsPipelineDerivative;
    vkCreateComputePipelineType CreateComputePipeline;
    vkStorePipelineType StorePipeline;
    vkLoadPipelineType LoadPipeline;
    vkLoadPipelineDerivativeType LoadPipelineDerivative;
    vkCreateSamplerType CreateSampler;
    vkCreateDescriptorSetLayoutType CreateDescriptorSetLayout;
    vkCreateDescriptorSetLayoutChainType CreateDescriptorSetLayoutChain;
    vkBeginDescriptorPoolUpdateType BeginDescriptorPoolUpdate;
    vkEndDescriptorPoolUpdateType EndDescriptorPoolUpdate;
    vkCreateDescriptorPoolType CreateDescriptorPool;
    vkResetDescriptorPoolType ResetDescriptorPool;
    vkAllocDescriptorSetsType AllocDescriptorSets;
    vkClearDescriptorSetsType ClearDescriptorSets;
    vkUpdateDescriptorsType UpdateDescriptors;
    vkCreateDynamicViewportStateType CreateDynamicViewportState;
    vkCreateDynamicRasterStateType CreateDynamicRasterState;
    vkCreateDynamicColorBlendStateType CreateDynamicColorBlendState;
    vkCreateDynamicDepthStencilStateType CreateDynamicDepthStencilState;
    vkCreateCommandBufferType CreateCommandBuffer;
    vkBeginCommandBufferType BeginCommandBuffer;
    vkEndCommandBufferType EndCommandBuffer;
    vkResetCommandBufferType ResetCommandBuffer;
    vkCmdBindPipelineType CmdBindPipeline;
    vkCmdBindDynamicStateObjectType CmdBindDynamicStateObject;
    vkCmdBindDescriptorSetsType CmdBindDescriptorSets;
    vkCmdBindVertexBufferType CmdBindVertexBuffer;
    vkCmdBindIndexBufferType CmdBindIndexBuffer;
    vkCmdDrawType CmdDraw;
    vkCmdDrawIndexedType CmdDrawIndexed;
    vkCmdDrawIndirectType CmdDrawIndirect;
    vkCmdDrawIndexedIndirectType CmdDrawIndexedIndirect;
    vkCmdDispatchType CmdDispatch;
    vkCmdDispatchIndirectType CmdDispatchIndirect;
    vkCmdCopyBufferType CmdCopyBuffer;
    vkCmdCopyImageType CmdCopyImage;
    vkCmdBlitImageType CmdBlitImage;
    vkCmdCopyBufferToImageType CmdCopyBufferToImage;
    vkCmdCopyImageToBufferType CmdCopyImageToBuffer;
    vkCmdCloneImageDataType CmdCloneImageData;
    vkCmdUpdateBufferType CmdUpdateBuffer;
    vkCmdFillBufferType CmdFillBuffer;
    vkCmdClearColorImageType CmdClearColorImage;
    vkCmdClearDepthStencilType CmdClearDepthStencil;
    vkCmdResolveImageType CmdResolveImage;
    vkCmdSetEventType CmdSetEvent;
    vkCmdResetEventType CmdResetEvent;
    vkCmdWaitEventsType CmdWaitEvents;
    vkCmdPipelineBarrierType CmdPipelineBarrier;
    vkCmdBeginQueryType CmdBeginQuery;
    vkCmdEndQueryType CmdEndQuery;
    vkCmdResetQueryPoolType CmdResetQueryPool;
    vkCmdWriteTimestampType CmdWriteTimestamp;
    vkCmdInitAtomicCountersType CmdInitAtomicCounters;
    vkCmdLoadAtomicCountersType CmdLoadAtomicCounters;
    vkCmdSaveAtomicCountersType CmdSaveAtomicCounters;
    vkCreateFramebufferType CreateFramebuffer;
    vkCreateRenderPassType CreateRenderPass;
    vkCmdBeginRenderPassType CmdBeginRenderPass;
    vkCmdEndRenderPassType CmdEndRenderPass;
    vkDbgSetValidationLevelType DbgSetValidationLevel;
    vkDbgRegisterMsgCallbackType DbgRegisterMsgCallback;
    vkDbgUnregisterMsgCallbackType DbgUnregisterMsgCallback;
    vkDbgSetMessageFilterType DbgSetMessageFilter;
    vkDbgSetObjectTagType DbgSetObjectTag;
    vkDbgSetGlobalOptionType DbgSetGlobalOption;
    vkDbgSetDeviceOptionType DbgSetDeviceOption;
    vkCmdDbgMarkerBeginType CmdDbgMarkerBegin;
    vkCmdDbgMarkerEndType CmdDbgMarkerEnd;
#if defined(__linux__) || defined(XCB_NVIDIA)
    vkWsiX11AssociateConnectionType WsiX11AssociateConnection;
    vkWsiX11GetMSCType WsiX11GetMSC;
    vkWsiX11CreatePresentableImageType WsiX11CreatePresentableImage;
    vkWsiX11QueuePresentType WsiX11QueuePresent;
#endif // WIN32
} VK_LAYER_DISPATCH_TABLE;

// LL node for tree of dbg callback functions
typedef struct _VK_LAYER_DBG_FUNCTION_NODE
{
    VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback;
    void *pUserData;
    struct _VK_LAYER_DBG_FUNCTION_NODE *pNext;
} VK_LAYER_DBG_FUNCTION_NODE;

typedef enum _VK_LAYER_DBG_ACTION
{
    VK_DBG_LAYER_ACTION_IGNORE = 0x0,
    VK_DBG_LAYER_ACTION_CALLBACK = 0x1,
    VK_DBG_LAYER_ACTION_LOG_MSG = 0x2,
    VK_DBG_LAYER_ACTION_BREAK = 0x4
} VK_LAYER_DBG_ACTION;

typedef enum _VK_LAYER_DBG_REPORT_LEVEL
{

    VK_DBG_LAYER_LEVEL_INFO = 0,
    VK_DBG_LAYER_LEVEL_WARN,
    VK_DBG_LAYER_LEVEL_PERF_WARN,
    VK_DBG_LAYER_LEVEL_ERROR,
    VK_DBG_LAYER_LEVEL_NONE,
} VK_LAYER_DBG_REPORT_LEVEL;
// ------------------------------------------------------------------------------------------------
// API functions
