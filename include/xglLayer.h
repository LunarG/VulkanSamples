/* Need to define dispatch table
 * Core struct can then have ptr to dispatch table at the top
 * Along with object ptrs for current and next OBJ
 */
#pragma once

#include "xgl.h"
#include "xglDbg.h"
#include "xglWsiX11Ext.h"
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
    xglInitAndEnumerateGpusType InitAndEnumerateGpus;
    xglGetGpuInfoType GetGpuInfo;
    xglCreateDeviceType CreateDevice;
    xglDestroyDeviceType DestroyDevice;
    xglGetExtensionSupportType GetExtensionSupport;
    xglEnumerateLayersType EnumerateLayers;
    xglGetDeviceQueueType GetDeviceQueue;
    xglQueueSubmitType QueueSubmit;
    xglQueueSetGlobalMemReferencesType QueueSetGlobalMemReferences;
    xglQueueWaitIdleType QueueWaitIdle;
    xglDeviceWaitIdleType DeviceWaitIdle;
    xglGetMemoryHeapCountType GetMemoryHeapCount;
    xglGetMemoryHeapInfoType GetMemoryHeapInfo;
    xglAllocMemoryType AllocMemory;
    xglFreeMemoryType FreeMemory;
    xglSetMemoryPriorityType SetMemoryPriority;
    xglMapMemoryType MapMemory;
    xglUnmapMemoryType UnmapMemory;
    xglPinSystemMemoryType PinSystemMemory;
    xglRemapVirtualMemoryPagesType RemapVirtualMemoryPages;
    xglGetMultiGpuCompatibilityType GetMultiGpuCompatibility;
    xglOpenSharedMemoryType OpenSharedMemory;
    xglOpenSharedQueueSemaphoreType OpenSharedQueueSemaphore;
    xglOpenPeerMemoryType OpenPeerMemory;
    xglOpenPeerImageType OpenPeerImage;
    xglDestroyObjectType DestroyObject;
    xglGetObjectInfoType GetObjectInfo;
    xglBindObjectMemoryType BindObjectMemory;
    xglCreateFenceType CreateFence;
    xglGetFenceStatusType GetFenceStatus;
    xglWaitForFencesType WaitForFences;
    xglCreateQueueSemaphoreType CreateQueueSemaphore;
    xglSignalQueueSemaphoreType SignalQueueSemaphore;
    xglWaitQueueSemaphoreType WaitQueueSemaphore;
    xglCreateEventType CreateEvent;
    xglGetEventStatusType GetEventStatus;
    xglSetEventType SetEvent;
    xglResetEventType ResetEvent;
    xglCreateQueryPoolType CreateQueryPool;
    xglGetQueryPoolResultsType GetQueryPoolResults;
    xglGetFormatInfoType GetFormatInfo;
    xglCreateImageType CreateImage;
    xglGetImageSubresourceInfoType GetImageSubresourceInfo;
    xglCreateImageViewType CreateImageView;
    xglCreateColorAttachmentViewType CreateColorAttachmentView;
    xglCreateDepthStencilViewType CreateDepthStencilView;
    xglCreateShaderType CreateShader;
    xglCreateGraphicsPipelineType CreateGraphicsPipeline;
    xglCreateComputePipelineType CreateComputePipeline;
    xglStorePipelineType StorePipeline;
    xglLoadPipelineType LoadPipeline;
    xglCreatePipelineDeltaType CreatePipelineDelta;
    xglCreateSamplerType CreateSampler;
    xglCreateDescriptorSetType CreateDescriptorSet;
    xglBeginDescriptorSetUpdateType BeginDescriptorSetUpdate;
    xglEndDescriptorSetUpdateType EndDescriptorSetUpdate;
    xglAttachSamplerDescriptorsType AttachSamplerDescriptors;
    xglAttachImageViewDescriptorsType AttachImageViewDescriptors;
    xglAttachMemoryViewDescriptorsType AttachMemoryViewDescriptors;
    xglAttachNestedDescriptorsType AttachNestedDescriptors;
    xglClearDescriptorSetSlotsType ClearDescriptorSetSlots;
    xglCreateViewportStateType CreateViewportState;
    xglCreateRasterStateType CreateRasterState;
    xglCreateMsaaStateType CreateMsaaState;
    xglCreateColorBlendStateType CreateColorBlendState;
    xglCreateDepthStencilStateType CreateDepthStencilState;
    xglCreateCommandBufferType CreateCommandBuffer;
    xglBeginCommandBufferType BeginCommandBuffer;
    xglEndCommandBufferType EndCommandBuffer;
    xglResetCommandBufferType ResetCommandBuffer;
    xglCmdBindPipelineType CmdBindPipeline;
    xglCmdBindPipelineDeltaType CmdBindPipelineDelta;
    xglCmdBindStateObjectType CmdBindStateObject;
    xglCmdBindDescriptorSetType CmdBindDescriptorSet;
    xglCmdBindDynamicMemoryViewType CmdBindDynamicMemoryView;
    xglCmdBindVertexDataType CmdBindVertexData;
    xglCmdBindIndexDataType CmdBindIndexData;
    xglCmdPrepareMemoryRegionsType CmdPrepareMemoryRegions;
    xglCmdPrepareImagesType CmdPrepareImages;
    xglCmdDrawType CmdDraw;
    xglCmdDrawIndexedType CmdDrawIndexed;
    xglCmdDrawIndirectType CmdDrawIndirect;
    xglCmdDrawIndexedIndirectType CmdDrawIndexedIndirect;
    xglCmdDispatchType CmdDispatch;
    xglCmdDispatchIndirectType CmdDispatchIndirect;
    xglCmdCopyMemoryType CmdCopyMemory;
    xglCmdCopyImageType CmdCopyImage;
    xglCmdCopyMemoryToImageType CmdCopyMemoryToImage;
    xglCmdCopyImageToMemoryType CmdCopyImageToMemory;
    xglCmdCloneImageDataType CmdCloneImageData;
    xglCmdUpdateMemoryType CmdUpdateMemory;
    xglCmdFillMemoryType CmdFillMemory;
    xglCmdClearColorImageType CmdClearColorImage;
    xglCmdClearColorImageRawType CmdClearColorImageRaw;
    xglCmdClearDepthStencilType CmdClearDepthStencil;
    xglCmdResolveImageType CmdResolveImage;
    xglCmdSetEventType CmdSetEvent;
    xglCmdResetEventType CmdResetEvent;
    xglCmdMemoryAtomicType CmdMemoryAtomic;
    xglCmdBeginQueryType CmdBeginQuery;
    xglCmdEndQueryType CmdEndQuery;
    xglCmdResetQueryPoolType CmdResetQueryPool;
    xglCmdWriteTimestampType CmdWriteTimestamp;
    xglCmdInitAtomicCountersType CmdInitAtomicCounters;
    xglCmdLoadAtomicCountersType CmdLoadAtomicCounters;
    xglCmdSaveAtomicCountersType CmdSaveAtomicCounters;
    xglCreateFramebufferType CreateFramebuffer;
    xglCreateRenderPassType CreateRenderPass;
    xglDbgSetValidationLevelType DbgSetValidationLevel;
    xglDbgRegisterMsgCallbackType DbgRegisterMsgCallback;
    xglDbgUnregisterMsgCallbackType DbgUnregisterMsgCallback;
    xglDbgSetMessageFilterType DbgSetMessageFilter;
    xglDbgSetObjectTagType DbgSetObjectTag;
    xglDbgSetGlobalOptionType DbgSetGlobalOption;
    xglDbgSetDeviceOptionType DbgSetDeviceOption;
    xglCmdDbgMarkerBeginType CmdDbgMarkerBegin;
    xglCmdDbgMarkerEndType CmdDbgMarkerEnd;
    xglWsiX11AssociateConnectionType WsiX11AssociateConnection;
    xglWsiX11GetMSCType WsiX11GetMSC;
    xglWsiX11CreatePresentableImageType WsiX11CreatePresentableImage;
    xglWsiX11QueuePresentType WsiX11QueuePresent;
} XGL_LAYER_DISPATCH_TABLE;

// LL node for tree of dbg callback functions
typedef struct _XGL_LAYER_DBG_FUNCTION_NODE
{
    XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback;
    XGL_VOID *pUserData;
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
