/* Need to define dispatch table
 * Core struct can then have ptr to dispatch table at the top
 * Along with object ptrs for current and next OBJ
 */
#pragma once

#include "xgl.h"
#include "xglDbg.h"
#if defined(__linux__) || defined(XCB_NVIDIA)
#include "xglWsiX11Ext.h"
#endif
#if defined(__GNUC__) && __GNUC__ >= 4
#  define XGL_LAYER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#  define XGL_LAYER_EXPORT __attribute__((visibility("default")))
#else
#  define XGL_LAYER_EXPORT
#endif


typedef struct _XGL_BASE_LAYER_OBJECT
{
    xglGetProcAddrType pGPA;
    XGL_BASE_OBJECT nextObject;
    XGL_BASE_OBJECT baseObject;
} XGL_BASE_LAYER_OBJECT;

typedef struct _XGL_LAYER_DISPATCH_TABLE
{
    xglGetProcAddrType GetProcAddr;
    xglCreateInstanceType CreateInstance;
    xglDestroyInstanceType DestroyInstance;
    xglEnumerateGpusType EnumerateGpus;
    xglGetGpuInfoType GetGpuInfo;
    xglCreateDeviceType CreateDevice;
    xglDestroyDeviceType DestroyDevice;
    xglGetExtensionSupportType GetExtensionSupport;
    xglEnumerateLayersType EnumerateLayers;
    xglGetDeviceQueueType GetDeviceQueue;
    xglQueueSubmitType QueueSubmit;
    xglQueueAddMemReferenceType QueueAddMemReference;
    xglQueueRemoveMemReferenceType QueueRemoveMemReference;
    xglQueueWaitIdleType QueueWaitIdle;
    xglDeviceWaitIdleType DeviceWaitIdle;
    xglAllocMemoryType AllocMemory;
    xglFreeMemoryType FreeMemory;
    xglSetMemoryPriorityType SetMemoryPriority;
    xglMapMemoryType MapMemory;
    xglUnmapMemoryType UnmapMemory;
    xglPinSystemMemoryType PinSystemMemory;
    xglGetMultiGpuCompatibilityType GetMultiGpuCompatibility;
    xglOpenSharedMemoryType OpenSharedMemory;
    xglOpenSharedSemaphoreType OpenSharedSemaphore;
    xglOpenPeerMemoryType OpenPeerMemory;
    xglOpenPeerImageType OpenPeerImage;
    xglDestroyObjectType DestroyObject;
    xglGetObjectInfoType GetObjectInfo;
    xglBindObjectMemoryType BindObjectMemory;
    xglBindObjectMemoryRangeType BindObjectMemoryRange;
    xglBindImageMemoryRangeType BindImageMemoryRange;
    xglCreateFenceType CreateFence;
    xglGetFenceStatusType GetFenceStatus;
    xglResetFencesType ResetFences;
    xglWaitForFencesType WaitForFences;
    xglCreateSemaphoreType CreateSemaphore;
    xglQueueSignalSemaphoreType QueueSignalSemaphore;
    xglQueueWaitSemaphoreType QueueWaitSemaphore;
    xglCreateEventType CreateEvent;
    xglGetEventStatusType GetEventStatus;
    xglSetEventType SetEvent;
    xglResetEventType ResetEvent;
    xglCreateQueryPoolType CreateQueryPool;
    xglGetQueryPoolResultsType GetQueryPoolResults;
    xglGetFormatInfoType GetFormatInfo;
    xglCreateBufferType CreateBuffer;
    xglCreateBufferViewType CreateBufferView;
    xglCreateImageType CreateImage;
    xglGetImageSubresourceInfoType GetImageSubresourceInfo;
    xglCreateImageViewType CreateImageView;
    xglCreateColorAttachmentViewType CreateColorAttachmentView;
    xglCreateDepthStencilViewType CreateDepthStencilView;
    xglCreateShaderType CreateShader;
    xglCreateGraphicsPipelineType CreateGraphicsPipeline;
    xglCreateGraphicsPipelineDerivativeType CreateGraphicsPipelineDerivative;
    xglCreateComputePipelineType CreateComputePipeline;
    xglStorePipelineType StorePipeline;
    xglLoadPipelineType LoadPipeline;
    xglLoadPipelineDerivativeType LoadPipelineDerivative;
    xglCreateSamplerType CreateSampler;
    xglCreateDescriptorSetLayoutType CreateDescriptorSetLayout;
    xglCreateDescriptorSetLayoutChainType CreateDescriptorSetLayoutChain;
    xglBeginDescriptorPoolUpdateType BeginDescriptorPoolUpdate;
    xglEndDescriptorPoolUpdateType EndDescriptorPoolUpdate;
    xglCreateDescriptorPoolType CreateDescriptorPool;
    xglResetDescriptorPoolType ResetDescriptorPool;
    xglAllocDescriptorSetsType AllocDescriptorSets;
    xglClearDescriptorSetsType ClearDescriptorSets;
    xglUpdateDescriptorsType UpdateDescriptors;
    xglCreateDynamicViewportStateType CreateDynamicViewportState;
    xglCreateDynamicRasterStateType CreateDynamicRasterState;
    xglCreateDynamicColorBlendStateType CreateDynamicColorBlendState;
    xglCreateDynamicDepthStencilStateType CreateDynamicDepthStencilState;
    xglCreateCommandBufferType CreateCommandBuffer;
    xglBeginCommandBufferType BeginCommandBuffer;
    xglEndCommandBufferType EndCommandBuffer;
    xglResetCommandBufferType ResetCommandBuffer;
    xglCmdBindPipelineType CmdBindPipeline;
    xglCmdBindDynamicStateObjectType CmdBindDynamicStateObject;
    xglCmdBindDescriptorSetsType CmdBindDescriptorSets;
    xglCmdBindVertexBufferType CmdBindVertexBuffer;
    xglCmdBindIndexBufferType CmdBindIndexBuffer;
    xglCmdDrawType CmdDraw;
    xglCmdDrawIndexedType CmdDrawIndexed;
    xglCmdDrawIndirectType CmdDrawIndirect;
    xglCmdDrawIndexedIndirectType CmdDrawIndexedIndirect;
    xglCmdDispatchType CmdDispatch;
    xglCmdDispatchIndirectType CmdDispatchIndirect;
    xglCmdCopyBufferType CmdCopyBuffer;
    xglCmdCopyImageType CmdCopyImage;
    xglCmdBlitImageType CmdBlitImage;
    xglCmdCopyBufferToImageType CmdCopyBufferToImage;
    xglCmdCopyImageToBufferType CmdCopyImageToBuffer;
    xglCmdCloneImageDataType CmdCloneImageData;
    xglCmdUpdateBufferType CmdUpdateBuffer;
    xglCmdFillBufferType CmdFillBuffer;
    xglCmdClearColorImageType CmdClearColorImage;
    xglCmdClearDepthStencilType CmdClearDepthStencil;
    xglCmdResolveImageType CmdResolveImage;
    xglCmdSetEventType CmdSetEvent;
    xglCmdResetEventType CmdResetEvent;
    xglCmdWaitEventsType CmdWaitEvents;
    xglCmdPipelineBarrierType CmdPipelineBarrier;
    xglCmdBeginQueryType CmdBeginQuery;
    xglCmdEndQueryType CmdEndQuery;
    xglCmdResetQueryPoolType CmdResetQueryPool;
    xglCmdWriteTimestampType CmdWriteTimestamp;
    xglCmdInitAtomicCountersType CmdInitAtomicCounters;
    xglCmdLoadAtomicCountersType CmdLoadAtomicCounters;
    xglCmdSaveAtomicCountersType CmdSaveAtomicCounters;
    xglCreateFramebufferType CreateFramebuffer;
    xglCreateRenderPassType CreateRenderPass;
    xglCmdBeginRenderPassType CmdBeginRenderPass;
    xglCmdEndRenderPassType CmdEndRenderPass;
    xglDbgSetValidationLevelType DbgSetValidationLevel;
    xglDbgRegisterMsgCallbackType DbgRegisterMsgCallback;
    xglDbgUnregisterMsgCallbackType DbgUnregisterMsgCallback;
    xglDbgSetMessageFilterType DbgSetMessageFilter;
    xglDbgSetObjectTagType DbgSetObjectTag;
    xglDbgSetGlobalOptionType DbgSetGlobalOption;
    xglDbgSetDeviceOptionType DbgSetDeviceOption;
    xglCmdDbgMarkerBeginType CmdDbgMarkerBegin;
    xglCmdDbgMarkerEndType CmdDbgMarkerEnd;
#if defined(__linux__) || defined(XCB_NVIDIA)
    xglWsiX11AssociateConnectionType WsiX11AssociateConnection;
    xglWsiX11GetMSCType WsiX11GetMSC;
    xglWsiX11CreatePresentableImageType WsiX11CreatePresentableImage;
    xglWsiX11QueuePresentType WsiX11QueuePresent;
#endif // WIN32
} XGL_LAYER_DISPATCH_TABLE;

// LL node for tree of dbg callback functions
typedef struct _XGL_LAYER_DBG_FUNCTION_NODE
{
    XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback;
    void *pUserData;
    struct _XGL_LAYER_DBG_FUNCTION_NODE *pNext;
} XGL_LAYER_DBG_FUNCTION_NODE;

typedef enum _XGL_LAYER_DBG_ACTION
{
    XGL_DBG_LAYER_ACTION_IGNORE = 0x0,
    XGL_DBG_LAYER_ACTION_CALLBACK = 0x1,
    XGL_DBG_LAYER_ACTION_LOG_MSG = 0x2,
    XGL_DBG_LAYER_ACTION_BREAK = 0x4
} XGL_LAYER_DBG_ACTION;

typedef enum _XGL_LAYER_DBG_REPORT_LEVEL
{

    XGL_DBG_LAYER_LEVEL_INFO = 0,
    XGL_DBG_LAYER_LEVEL_WARN,
    XGL_DBG_LAYER_LEVEL_PERF_WARN,
    XGL_DBG_LAYER_LEVEL_ERROR,
    XGL_DBG_LAYER_LEVEL_NONE,
} XGL_LAYER_DBG_REPORT_LEVEL;
// ------------------------------------------------------------------------------------------------
// API functions
