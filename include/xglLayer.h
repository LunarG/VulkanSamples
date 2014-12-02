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
    GetProcAddrType pGPA;
    XGL_BASE_OBJECT nextObject;
    XGL_BASE_OBJECT baseObject;
} XGL_BASE_LAYER_OBJECT;

typedef struct _XGL_LAYER_DISPATCH_TABLE
{
    GetProcAddrType GetProcAddr;
    InitAndEnumerateGpusType InitAndEnumerateGpus;
    GetGpuInfoType GetGpuInfo;
    CreateDeviceType CreateDevice;
    DestroyDeviceType DestroyDevice;
    GetExtensionSupportType GetExtensionSupport;
    EnumerateLayersType EnumerateLayers;
    GetDeviceQueueType GetDeviceQueue;
    QueueSubmitType QueueSubmit;
    QueueSetGlobalMemReferencesType QueueSetGlobalMemReferences;
    QueueWaitIdleType QueueWaitIdle;
    DeviceWaitIdleType DeviceWaitIdle;
    GetMemoryHeapCountType GetMemoryHeapCount;
    GetMemoryHeapInfoType GetMemoryHeapInfo;
    AllocMemoryType AllocMemory;
    FreeMemoryType FreeMemory;
    SetMemoryPriorityType SetMemoryPriority;
    MapMemoryType MapMemory;
    UnmapMemoryType UnmapMemory;
    PinSystemMemoryType PinSystemMemory;
    RemapVirtualMemoryPagesType RemapVirtualMemoryPages;
    GetMultiGpuCompatibilityType GetMultiGpuCompatibility;
    OpenSharedMemoryType OpenSharedMemory;
    OpenSharedQueueSemaphoreType OpenSharedQueueSemaphore;
    OpenPeerMemoryType OpenPeerMemory;
    OpenPeerImageType OpenPeerImage;
    DestroyObjectType DestroyObject;
    GetObjectInfoType GetObjectInfo;
    BindObjectMemoryType BindObjectMemory;
    CreateFenceType CreateFence;
    GetFenceStatusType GetFenceStatus;
    WaitForFencesType WaitForFences;
    CreateQueueSemaphoreType CreateQueueSemaphore;
    SignalQueueSemaphoreType SignalQueueSemaphore;
    WaitQueueSemaphoreType WaitQueueSemaphore;
    CreateEventType CreateEvent;
    GetEventStatusType GetEventStatus;
    SetEventType SetEvent;
    ResetEventType ResetEvent;
    CreateQueryPoolType CreateQueryPool;
    GetQueryPoolResultsType GetQueryPoolResults;
    GetFormatInfoType GetFormatInfo;
    CreateImageType CreateImage;
    GetImageSubresourceInfoType GetImageSubresourceInfo;
    CreateImageViewType CreateImageView;
    CreateColorAttachmentViewType CreateColorAttachmentView;
    CreateDepthStencilViewType CreateDepthStencilView;
    CreateShaderType CreateShader;
    CreateGraphicsPipelineType CreateGraphicsPipeline;
    CreateComputePipelineType CreateComputePipeline;
    StorePipelineType StorePipeline;
    LoadPipelineType LoadPipeline;
    CreatePipelineDeltaType CreatePipelineDelta;
    CreateSamplerType CreateSampler;
    CreateDescriptorSetType CreateDescriptorSet;
    BeginDescriptorSetUpdateType BeginDescriptorSetUpdate;
    EndDescriptorSetUpdateType EndDescriptorSetUpdate;
    AttachSamplerDescriptorsType AttachSamplerDescriptors;
    AttachImageViewDescriptorsType AttachImageViewDescriptors;
    AttachMemoryViewDescriptorsType AttachMemoryViewDescriptors;
    AttachNestedDescriptorsType AttachNestedDescriptors;
    ClearDescriptorSetSlotsType ClearDescriptorSetSlots;
    CreateViewportStateType CreateViewportState;
    CreateRasterStateType CreateRasterState;
    CreateMsaaStateType CreateMsaaState;
    CreateColorBlendStateType CreateColorBlendState;
    CreateDepthStencilStateType CreateDepthStencilState;
    CreateCommandBufferType CreateCommandBuffer;
    BeginCommandBufferType BeginCommandBuffer;
    EndCommandBufferType EndCommandBuffer;
    ResetCommandBufferType ResetCommandBuffer;
    CmdBindPipelineType CmdBindPipeline;
    CmdBindPipelineDeltaType CmdBindPipelineDelta;
    CmdBindStateObjectType CmdBindStateObject;
    CmdBindDescriptorSetType CmdBindDescriptorSet;
    CmdBindDynamicMemoryViewType CmdBindDynamicMemoryView;
    CmdBindVertexDataType CmdBindVertexData;
    CmdBindIndexDataType CmdBindIndexData;
    CmdBindAttachmentsType CmdBindAttachments;
    CmdPrepareMemoryRegionsType CmdPrepareMemoryRegions;
    CmdPrepareImagesType CmdPrepareImages;
    CmdDrawType CmdDraw;
    CmdDrawIndexedType CmdDrawIndexed;
    CmdDrawIndirectType CmdDrawIndirect;
    CmdDrawIndexedIndirectType CmdDrawIndexedIndirect;
    CmdDispatchType CmdDispatch;
    CmdDispatchIndirectType CmdDispatchIndirect;
    CmdCopyMemoryType CmdCopyMemory;
    CmdCopyImageType CmdCopyImage;
    CmdCopyMemoryToImageType CmdCopyMemoryToImage;
    CmdCopyImageToMemoryType CmdCopyImageToMemory;
    CmdCloneImageDataType CmdCloneImageData;
    CmdUpdateMemoryType CmdUpdateMemory;
    CmdFillMemoryType CmdFillMemory;
    CmdClearColorImageType CmdClearColorImage;
    CmdClearColorImageRawType CmdClearColorImageRaw;
    CmdClearDepthStencilType CmdClearDepthStencil;
    CmdResolveImageType CmdResolveImage;
    CmdSetEventType CmdSetEvent;
    CmdResetEventType CmdResetEvent;
    CmdMemoryAtomicType CmdMemoryAtomic;
    CmdBeginQueryType CmdBeginQuery;
    CmdEndQueryType CmdEndQuery;
    CmdResetQueryPoolType CmdResetQueryPool;
    CmdWriteTimestampType CmdWriteTimestamp;
    CmdInitAtomicCountersType CmdInitAtomicCounters;
    CmdLoadAtomicCountersType CmdLoadAtomicCounters;
    CmdSaveAtomicCountersType CmdSaveAtomicCounters;
    DbgSetValidationLevelType DbgSetValidationLevel;
    DbgRegisterMsgCallbackType DbgRegisterMsgCallback;
    DbgUnregisterMsgCallbackType DbgUnregisterMsgCallback;
    DbgSetMessageFilterType DbgSetMessageFilter;
    DbgSetObjectTagType DbgSetObjectTag;
    DbgSetGlobalOptionType DbgSetGlobalOption;
    DbgSetDeviceOptionType DbgSetDeviceOption;
    CmdDbgMarkerBeginType CmdDbgMarkerBegin;
    CmdDbgMarkerEndType CmdDbgMarkerEnd;
    WsiX11AssociateConnectionType WsiX11AssociateConnection;
    WsiX11GetMSCType WsiX11GetMSC;
    WsiX11CreatePresentableImageType WsiX11CreatePresentableImage;
    WsiX11QueuePresentType WsiX11QueuePresent;
} XGL_LAYER_DISPATCH_TABLE;

// LL node for tree of dbg callback functions
typedef struct _XGL_LAYER_DBG_FUNCTION_NODE
{
    XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback;
    XGL_VOID *pUserData;
    struct _XGL_LAYER_DBG_FUNCTION_NODE *pNext;
} XGL_LAYER_DBG_FUNCTION_NODE;

// ------------------------------------------------------------------------------------------------
// API functions
