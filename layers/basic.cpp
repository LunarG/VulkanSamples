#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>
#include "xglLayer.h"

static std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *> tableMap;

static XGL_LAYER_DISPATCH_TABLE * initLayerTable(const XGL_BASE_LAYER_OBJECT *gpuw)
{
    GetProcAddrType fpGPA;
    XGL_LAYER_DISPATCH_TABLE *pTable;

    assert(gpuw);
    std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *>::const_iterator it = tableMap.find((XGL_VOID *) gpuw);
    if (it == tableMap.end())
    {
        pTable =  new XGL_LAYER_DISPATCH_TABLE;
        tableMap[(XGL_VOID *) gpuw] = pTable;
    } else
    {
        return it->second;
    }
    fpGPA = gpuw->pGPA;
    assert(fpGPA);
    pTable->GetProcAddr = fpGPA;
    pTable->InitAndEnumerateGpus = (InitAndEnumerateGpusType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglInitAndEnumerateGpus");
    pTable->GetGpuInfo = (GetGpuInfoType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetGpuInfo");
    pTable->CreateDevice = (CreateDeviceType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateDevice");
    pTable->DestroyDevice = (DestroyDeviceType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDestroyDevice");
    pTable->GetExtensionSupport = (GetExtensionSupportType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetExtensionSupport");
    pTable->EnumerateLayers = (EnumerateLayersType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglEnumerateLayers");
    pTable->GetDeviceQueue = (GetDeviceQueueType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetDeviceQueue");
    pTable->QueueSubmit = (QueueSubmitType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglQueueSubmit");
    pTable->QueueSetGlobalMemReferences = (QueueSetGlobalMemReferencesType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglQueueSetGlobalMemReferences");
    pTable->QueueWaitIdle = (QueueWaitIdleType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglQueueWaitIdle");
    pTable->DeviceWaitIdle = (DeviceWaitIdleType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDeviceWaitIdle");
    pTable->GetMemoryHeapCount = (GetMemoryHeapCountType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetMemoryHeapCount");
    pTable->GetMemoryHeapInfo = (GetMemoryHeapInfoType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetMemoryHeapInfo");
    pTable->AllocMemory = (AllocMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglAllocMemory");
    pTable->FreeMemory = (FreeMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglFreeMemory");
    pTable->SetMemoryPriority = (SetMemoryPriorityType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglSetMemoryPriority");
    pTable->MapMemory = (MapMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglMapMemory");
    pTable->UnmapMemory = (UnmapMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglUnmapMemory");
    pTable->PinSystemMemory = (PinSystemMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglPinSystemMemory");
    pTable->RemapVirtualMemoryPages = (RemapVirtualMemoryPagesType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglRemapVirtualMemoryPages");
    pTable->GetMultiGpuCompatibility = (GetMultiGpuCompatibilityType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetMultiGpuCompatibility");
    pTable->OpenSharedMemory = (OpenSharedMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglOpenSharedMemory");
    pTable->OpenSharedQueueSemaphore = (OpenSharedQueueSemaphoreType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglOpenSharedQueueSemaphore");
    pTable->OpenPeerMemory = (OpenPeerMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglOpenPeerMemory");
    pTable->OpenPeerImage = (OpenPeerImageType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglOpenPeerImage");
    pTable->DestroyObject = (DestroyObjectType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDestroyObject");
    pTable->GetObjectInfo = (GetObjectInfoType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetObjectInfo");
    pTable->BindObjectMemory = (BindObjectMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglBindObjectMemory");
    pTable->CreateFence = (CreateFenceType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateFence");
    pTable->GetFenceStatus = (GetFenceStatusType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetFenceStatus");
    pTable->WaitForFences = (WaitForFencesType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglWaitForFences");
    pTable->CreateQueueSemaphore = (CreateQueueSemaphoreType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateQueueSemaphore");
    pTable->SignalQueueSemaphore = (SignalQueueSemaphoreType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglSignalQueueSemaphore");
    pTable->WaitQueueSemaphore = (WaitQueueSemaphoreType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglWaitQueueSemaphore");
    pTable->CreateEvent = (CreateEventType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateEvent");
    pTable->GetEventStatus = (GetEventStatusType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetEventStatus");
    pTable->SetEvent = (SetEventType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglSetEvent");
    pTable->ResetEvent = (ResetEventType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglResetEvent");
    pTable->CreateQueryPool = (CreateQueryPoolType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateQueryPool");
    pTable->GetQueryPoolResults = (GetQueryPoolResultsType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetQueryPoolResults");
    pTable->GetFormatInfo = (GetFormatInfoType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetFormatInfo");
    pTable->CreateImage = (CreateImageType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateImage");
    pTable->GetImageSubresourceInfo = (GetImageSubresourceInfoType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetImageSubresourceInfo");
    pTable->CreateImageView = (CreateImageViewType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateImageView");
    pTable->CreateColorAttachmentView = (CreateColorAttachmentViewType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateColorAttachmentView");
    pTable->CreateDepthStencilView = (CreateDepthStencilViewType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateDepthStencilView");
    pTable->CreateShader = (CreateShaderType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateShader");
    pTable->CreateGraphicsPipeline = (CreateGraphicsPipelineType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateGraphicsPipeline");
    pTable->CreateComputePipeline = (CreateComputePipelineType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateComputePipeline");
    pTable->StorePipeline = (StorePipelineType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglStorePipeline");
    pTable->LoadPipeline = (LoadPipelineType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglLoadPipeline");
    pTable->CreatePipelineDelta = (CreatePipelineDeltaType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreatePipelineDelta");
    pTable->CreateSampler = (CreateSamplerType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateSampler");
    pTable->CreateDescriptorSet = (CreateDescriptorSetType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateDescriptorSet");
    pTable->BeginDescriptorSetUpdate = (BeginDescriptorSetUpdateType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglBeginDescriptorSetUpdate");
    pTable->EndDescriptorSetUpdate = (EndDescriptorSetUpdateType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglEndDescriptorSetUpdate");
    pTable->AttachSamplerDescriptors = (AttachSamplerDescriptorsType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglAttachSamplerDescriptors");
    pTable->AttachImageViewDescriptors = (AttachImageViewDescriptorsType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglAttachImageViewDescriptors");
    pTable->AttachMemoryViewDescriptors = (AttachMemoryViewDescriptorsType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglAttachMemoryViewDescriptors");
    pTable->AttachNestedDescriptors = (AttachNestedDescriptorsType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglAttachNestedDescriptors");
    pTable->ClearDescriptorSetSlots = (ClearDescriptorSetSlotsType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglClearDescriptorSetSlots");
    pTable->CreateViewportState = (CreateViewportStateType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateViewportState");
    pTable->CreateRasterState = (CreateRasterStateType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateRasterState");
    pTable->CreateMsaaState = (CreateMsaaStateType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateMsaaState");
    pTable->CreateColorBlendState = (CreateColorBlendStateType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateColorBlendState");
    pTable->CreateDepthStencilState = (CreateDepthStencilStateType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateDepthStencilState");
    pTable->CreateCommandBuffer = (CreateCommandBufferType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCreateCommandBuffer");
    pTable->BeginCommandBuffer = (BeginCommandBufferType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglBeginCommandBuffer");
    pTable->EndCommandBuffer = (EndCommandBufferType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglEndCommandBuffer");
    pTable->ResetCommandBuffer = (ResetCommandBufferType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglResetCommandBuffer");
    pTable->CmdBindPipeline = (CmdBindPipelineType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdBindPipeline");
    pTable->CmdBindPipelineDelta = (CmdBindPipelineDeltaType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdBindPipelineDelta");
    pTable->CmdBindStateObject = (CmdBindStateObjectType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdBindStateObject");
    pTable->CmdBindDescriptorSet = (CmdBindDescriptorSetType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdBindDescriptorSet");
    pTable->CmdBindDynamicMemoryView = (CmdBindDynamicMemoryViewType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdBindDynamicMemoryView");
    pTable->CmdBindVertexData = (CmdBindVertexDataType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdBindVertexData");
    pTable->CmdBindIndexData = (CmdBindIndexDataType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdBindIndexData");
    pTable->CmdBindAttachments = (CmdBindAttachmentsType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdBindAttachments");
    pTable->CmdPrepareMemoryRegions = (CmdPrepareMemoryRegionsType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdPrepareMemoryRegions");
    pTable->CmdPrepareImages = (CmdPrepareImagesType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdPrepareImages");
    pTable->CmdDraw = (CmdDrawType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdDraw");
    pTable->CmdDrawIndexed = (CmdDrawIndexedType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdDrawIndexed");
    pTable->CmdDrawIndirect = (CmdDrawIndirectType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdDrawIndirect");
    pTable->CmdDrawIndexedIndirect = (CmdDrawIndexedIndirectType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdDrawIndexedIndirect");
    pTable->CmdDispatch = (CmdDispatchType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdDispatch");
    pTable->CmdDispatchIndirect = (CmdDispatchIndirectType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdDispatchIndirect");
    pTable->CmdCopyMemory = (CmdCopyMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdCopyMemory");
    pTable->CmdCopyImage = (CmdCopyImageType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdCopyImage");
    pTable->CmdCopyMemoryToImage = (CmdCopyMemoryToImageType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdCopyMemoryToImage");
    pTable->CmdCopyImageToMemory = (CmdCopyImageToMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdCopyImageToMemory");
    pTable->CmdCloneImageData = (CmdCloneImageDataType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdCloneImageData");
    pTable->CmdUpdateMemory = (CmdUpdateMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdUpdateMemory");
    pTable->CmdFillMemory = (CmdFillMemoryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdFillMemory");
    pTable->CmdClearColorImage = (CmdClearColorImageType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdClearColorImage");
    pTable->CmdClearColorImageRaw = (CmdClearColorImageRawType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdClearColorImageRaw");
    pTable->CmdClearDepthStencil = (CmdClearDepthStencilType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdClearDepthStencil");
    pTable->CmdResolveImage = (CmdResolveImageType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdResolveImage");
    pTable->CmdSetEvent = (CmdSetEventType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdSetEvent");
    pTable->CmdResetEvent = (CmdResetEventType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdResetEvent");
    pTable->CmdMemoryAtomic = (CmdMemoryAtomicType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdMemoryAtomic");
    pTable->CmdBeginQuery = (CmdBeginQueryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdBeginQuery");
    pTable->CmdEndQuery = (CmdEndQueryType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdEndQuery");
    pTable->CmdResetQueryPool = (CmdResetQueryPoolType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdResetQueryPool");
    pTable->CmdWriteTimestamp = (CmdWriteTimestampType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdWriteTimestamp");
    pTable->CmdInitAtomicCounters = (CmdInitAtomicCountersType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdInitAtomicCounters");
    pTable->CmdLoadAtomicCounters = (CmdLoadAtomicCountersType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdLoadAtomicCounters");
    pTable->CmdSaveAtomicCounters = (CmdSaveAtomicCountersType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdSaveAtomicCounters");
    pTable->DbgSetValidationLevel = (DbgSetValidationLevelType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDbgSetValidationLevel");
    pTable->DbgRegisterMsgCallback = (DbgRegisterMsgCallbackType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDbgRegisterMsgCallback");
    pTable->DbgUnregisterMsgCallback = (DbgUnregisterMsgCallbackType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDbgUnregisterMsgCallback");
    pTable->DbgSetMessageFilter = (DbgSetMessageFilterType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDbgSetMessageFilter");
    pTable->DbgSetObjectTag = (DbgSetObjectTagType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDbgSetObjectTag");
    pTable->DbgSetGlobalOption = (DbgSetGlobalOptionType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDbgSetGlobalOption");
    pTable->DbgSetDeviceOption = (DbgSetDeviceOptionType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglDbgSetDeviceOption");
    pTable->CmdDbgMarkerBegin = (CmdDbgMarkerBeginType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdDbgMarkerBegin");
    pTable->CmdDbgMarkerEnd = (CmdDbgMarkerEndType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglCmdDbgMarkerEnd");
    pTable->WsiX11AssociateConnection = (WsiX11AssociateConnectionType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglWsiX11AssociateConnection");
    pTable->WsiX11GetMSC = (WsiX11GetMSCType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglWsiX11GetMSC");
    pTable->WsiX11CreatePresentableImage = (WsiX11CreatePresentableImageType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglWsiX11CreatePresentableImage");
    pTable->WsiX11QueuePresent = (WsiX11QueuePresentType) fpGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglWsiX11QueuePresent");
    return pTable;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLayerExtension1(XGL_DEVICE device)
{
    printf("In xglLayerExtension1() call w/ device: %p\n", (void*)device);
    printf("xglLayerExtension1 returning SUCCESS\n");
    return XGL_SUCCESS;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pExtName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_RESULT result;
    XGL_LAYER_DISPATCH_TABLE* pTable = initLayerTable(gpuw);

    printf("At start of wrapped xglGetExtensionSupport() call w/ gpu: %p\n", (void*)gpu);
    if (!strncmp((const char *) pExtName, "xglLayerExtension1", strlen("xglLayerExtension1")))
        result = XGL_SUCCESS;
    else
        result = pTable->GetExtensionSupport((XGL_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    printf("Completed wrapped xglGetExtensionSupport() call w/ gpu: %p\n", (void*)gpu);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = initLayerTable(gpuw);

    printf("At start of wrapped xglCreateDevice() call w/ gpu: %p\n", (void*)gpu);
    XGL_RESULT result = pTable->CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table
    tableMap.emplace(*pDevice, pTable);
    printf("Completed wrapped xglCreateDevice() call w/ pDevice, Device %p: %p\n", (void*)pDevice, (void *) *pDevice);
    return result;
}
XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap[device];

    printf("At start of wrapped xglGetFormatInfo() call w/ device: %p\n", (void*)device);
    XGL_RESULT result = pTable->GetFormatInfo(device, format, infoType, pDataSize, pData);
    printf("Completed wrapped xglGetFormatInfo() call w/ device: %p\n", (void*)device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize, XGL_CHAR* const* pOutLayers, XGL_SIZE * pOutLayerCount, XGL_VOID* pReserved)
{
    if (gpu != NULL)
    {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        XGL_LAYER_DISPATCH_TABLE* pTable = initLayerTable(gpuw);

        printf("At start of wrapped xglEnumerateLayers() call w/ gpu: %p\n", gpu);
        XGL_RESULT result = pTable->EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayers, pOutLayerCount, pReserved);
        return result;
    } else
    {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL || pReserved == NULL)
            return XGL_ERROR_INVALID_POINTER;

        // Example of a layer that is only compatible with Intel's GPUs
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT*) pReserved;
        GetGpuInfoType fpGetGpuInfo;
        XGL_PHYSICAL_GPU_PROPERTIES gpuProps;
        XGL_SIZE dataSize = sizeof(XGL_PHYSICAL_GPU_PROPERTIES);
        fpGetGpuInfo = (GetGpuInfoType) gpuw->pGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, (const XGL_CHAR *) "xglGetGpuInfo");
        fpGetGpuInfo((XGL_PHYSICAL_GPU) gpuw->nextObject, XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES, &dataSize, &gpuProps);
        if (gpuProps.vendorId == 0x8086)
        {
            *pOutLayerCount = 1;
            strncpy((char *) pOutLayers[0], "Basic", maxStringSize);
        } else
        {
            *pOutLayerCount = 0;
        }
        return XGL_SUCCESS;
    }
}

XGL_LAYER_EXPORT XGL_VOID * XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pName) {
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable;
    if (gpu == NULL)
        return NULL;
    pTable = initLayerTable(gpuw);
    if (!strncmp("xglGetProcAddr", (const char *) pName, sizeof("xglGetProcAddr")))
        return (XGL_VOID *) xglGetProcAddr;
    else if (!strncmp("xglInitAndEnumerateGpus", (const char *) pName, sizeof("xglInitAndEnumerateGpus")))
        return (XGL_VOID *) pTable->InitAndEnumerateGpus;
    if (!strncmp("xglGetGpuInfo", (const char *) pName, sizeof ("xglGetGpuInfo")))
        return (XGL_VOID *) pTable->GetGpuInfo;
    else if (!strncmp("xglCreateDevice", (const char *) pName, sizeof ("xglCreateDevice")))
        return (XGL_VOID *) xglCreateDevice;
    else if (!strncmp("xglDestroyDevice", (const char *) pName, sizeof ("xglDestroyDevice")))
        return (XGL_VOID *) pTable->DestroyDevice;
    else if (!strncmp("xglGetExtensionSupport", (const char *) pName, sizeof ("xglGetExtensionSupport")))
        return (XGL_VOID *) xglGetExtensionSupport;
    else if (!strncmp("xglEnumerateLayers", (const char *) pName, sizeof ("xglEnumerateLayers")))
        return (XGL_VOID *) xglEnumerateLayers;
    else if (!strncmp("xglGetDeviceQueue", (const char *) pName, sizeof ("xglGetDeviceQueue")))
        return (XGL_VOID *) pTable->GetDeviceQueue;
    else if (!strncmp("xglQueueSubmit", (const char *) pName, sizeof ("xglQueueSubmit")))
        return (XGL_VOID *) pTable->QueueSubmit;
    else if (!strncmp("xglQueueSetGlobalMemReferences", (const char *) pName, sizeof ("xglQueueSetGlobalMemReferences")))
        return (XGL_VOID *) pTable->QueueSetGlobalMemReferences;
    else if (!strncmp("xglQueueWaitIdle", (const char *) pName, sizeof ("xglQueueWaitIdle")))
        return (XGL_VOID *) pTable->QueueWaitIdle;
    else if (!strncmp("xglDeviceWaitIdle", (const char *) pName, sizeof ("xglDeviceWaitIdle")))
        return (XGL_VOID *) pTable->DeviceWaitIdle;
    else if (!strncmp("xglGetMemoryHeapCount", (const char *) pName, sizeof ("xglGetMemoryHeapCount")))
        return (XGL_VOID *) pTable->GetMemoryHeapCount;
    else if (!strncmp("xglGetMemoryHeapInfo", (const char *) pName, sizeof ("xglGetMemoryHeapInfo")))
        return (XGL_VOID *) pTable->GetMemoryHeapInfo;
    else if (!strncmp("xglAllocMemory", (const char *) pName, sizeof ("xglAllocMemory")))
        return (XGL_VOID *) pTable->AllocMemory;
    else if (!strncmp("xglFreeMemory", (const char *) pName, sizeof ("xglFreeMemory")))
        return (XGL_VOID *) pTable->FreeMemory;
    else if (!strncmp("xglSetMemoryPriority", (const char *) pName, sizeof ("xglSetMemoryPriority")))
        return (XGL_VOID *) pTable->SetMemoryPriority;
    else if (!strncmp("xglMapMemory", (const char *) pName, sizeof ("xglMapMemory")))
        return (XGL_VOID *) pTable->MapMemory;
    else if (!strncmp("xglUnmapMemory", (const char *) pName, sizeof ("xglUnmapMemory")))
        return (XGL_VOID *) pTable->UnmapMemory;
    else if (!strncmp("xglPinSystemMemory", (const char *) pName, sizeof ("xglPinSystemMemory")))
        return (XGL_VOID *) pTable->PinSystemMemory;
    else if (!strncmp("xglRemapVirtualMemoryPages", (const char *) pName, sizeof ("xglRemapVirtualMemoryPages")))
        return (XGL_VOID *) pTable->RemapVirtualMemoryPages;
    else if (!strncmp("xglGetMultiGpuCompatibility", (const char *) pName, sizeof ("xglGetMultiGpuCompatibility")))
        return (XGL_VOID *) pTable->GetMultiGpuCompatibility;
    else if (!strncmp("xglOpenSharedMemory", (const char *) pName, sizeof ("xglOpenSharedMemory")))
        return (XGL_VOID *) pTable->OpenSharedMemory;
    else if (!strncmp("xglOpenSharedQueueSemaphore", (const char *) pName, sizeof ("xglOpenSharedQueueSemaphore")))
        return (XGL_VOID *) pTable->OpenSharedQueueSemaphore;
    else if (!strncmp("xglOpenPeerMemory", (const char *) pName, sizeof ("xglOpenPeerMemory")))
        return (XGL_VOID *) pTable->OpenPeerMemory;
    else if (!strncmp("xglOpenPeerImage", (const char *) pName, sizeof ("xglOpenPeerImage")))
        return (XGL_VOID *) pTable->OpenPeerImage;
    else if (!strncmp("xglDestroyObject", (const char *) pName, sizeof ("xglDestroyObject")))
        return (XGL_VOID *) pTable->DestroyObject;
    else if (!strncmp("xglGetObjectInfo", (const char *) pName, sizeof ("xglGetObjectInfo")))
        return (XGL_VOID *) pTable->GetObjectInfo;
    else if (!strncmp("xglBindObjectMemory", (const char *) pName, sizeof ("xglBindObjectMemory")))
        return (XGL_VOID *) pTable->BindObjectMemory;
    else if (!strncmp("xglCreateFence", (const char *) pName, sizeof ("xgllCreateFence")))
        return (XGL_VOID *) pTable->CreateFence;
    else if (!strncmp("xglGetFenceStatus", (const char *) pName, sizeof ("xglGetFenceStatus")))
        return (XGL_VOID *) pTable->GetFenceStatus;
    else if (!strncmp("xglWaitForFences", (const char *) pName, sizeof ("xglWaitForFences")))
        return (XGL_VOID *) pTable->WaitForFences;
    else if (!strncmp("xglCreateQueueSemaphore", (const char *) pName, sizeof ("xgllCreateQueueSemaphore")))
        return (XGL_VOID *) pTable->CreateQueueSemaphore;
    else if (!strncmp("xglSignalQueueSemaphore", (const char *) pName, sizeof ("xglSignalQueueSemaphore")))
        return (XGL_VOID *) pTable->SignalQueueSemaphore;
    else if (!strncmp("xglWaitQueueSemaphore", (const char *) pName, sizeof ("xglWaitQueueSemaphore")))
        return (XGL_VOID *) pTable->WaitQueueSemaphore;
    else if (!strncmp("xglCreateEvent", (const char *) pName, sizeof ("xgllCreateEvent")))
        return (XGL_VOID *) pTable->CreateEvent;
    else if (!strncmp("xglGetEventStatus", (const char *) pName, sizeof ("xglGetEventStatus")))
        return (XGL_VOID *) pTable->GetEventStatus;
    else if (!strncmp("xglSetEvent", (const char *) pName, sizeof ("xglSetEvent")))
        return (XGL_VOID *) pTable->SetEvent;
    else if (!strncmp("xglResetEvent", (const char *) pName, sizeof ("xgllResetEvent")))
        return (XGL_VOID *) pTable->ResetEvent;
    else if (!strncmp("xglCreateQueryPool", (const char *) pName, sizeof ("xglCreateQueryPool")))
        return (XGL_VOID *) pTable->CreateQueryPool;
    else if (!strncmp("xglGetQueryPoolResults", (const char *) pName, sizeof ("xglGetQueryPoolResults")))
        return (XGL_VOID *) pTable->GetQueryPoolResults;
    else if (!strncmp("xglGetFormatInfo", (const char *) pName, sizeof ("xglGetFormatInfo")))
        return (XGL_VOID *) xglGetFormatInfo;
    else if (!strncmp("xglCreateImage", (const char *) pName, sizeof ("xglCreateImage")))
        return (XGL_VOID *) pTable->CreateImage;
    else if (!strncmp("xglGetImageSubresourceInfo", (const char *) pName, sizeof ("xglGetImageSubresourceInfo")))
        return (XGL_VOID *) pTable->GetImageSubresourceInfo;
    else if (!strncmp("xglCreateImageView", (const char *) pName, sizeof ("xglCreateImageView")))
        return (XGL_VOID *) pTable->CreateImageView;
    else if (!strncmp("xglCreateColorAttachmentView", (const char *) pName, sizeof ("xglCreateColorAttachmentView")))
        return (XGL_VOID *) pTable->CreateColorAttachmentView;
    else if (!strncmp("xglCreateDepthStencilView", (const char *) pName, sizeof ("xglCreateDepthStencilView")))
        return (XGL_VOID *) pTable->CreateDepthStencilView;
    else if (!strncmp("xglCreateShader", (const char *) pName, sizeof ("xglCreateShader")))
        return (XGL_VOID *) pTable->CreateShader;
    else if (!strncmp("xglCreateGraphicsPipeline", (const char *) pName, sizeof ("xglCreateGraphicsPipeline")))
        return (XGL_VOID *) pTable->CreateGraphicsPipeline;
    else if (!strncmp("xglCreateComputePipeline", (const char *) pName, sizeof ("xglCreateComputePipeline")))
        return (XGL_VOID *) pTable->CreateComputePipeline;
    else if (!strncmp("xglStorePipeline", (const char *) pName, sizeof ("xglStorePipeline")))
        return (XGL_VOID *) pTable->StorePipeline;
    else if (!strncmp("xglLoadPipeline", (const char *) pName, sizeof ("xglLoadPipeline")))
        return (XGL_VOID *) pTable->LoadPipeline;
    else if (!strncmp("xglCreatePipelineDelta", (const char *) pName, sizeof ("xglCreatePipelineDelta")))
        return (XGL_VOID *) pTable->CreatePipelineDelta;
    else if (!strncmp("xglCreateSampler", (const char *) pName, sizeof ("xglCreateSampler")))
        return (XGL_VOID *) pTable->CreateSampler;
    else if (!strncmp("xglCreateDescriptorSet", (const char *) pName, sizeof ("xglCreateDescriptorSet")))
        return (XGL_VOID *) pTable->CreateDescriptorSet;
    else if (!strncmp("xglBeginDescriptorSetUpdate", (const char *) pName, sizeof ("xglBeginDescriptorSetUpdate")))
        return (XGL_VOID *) pTable->BeginDescriptorSetUpdate;
    else if (!strncmp("xglEndDescriptorSetUpdate", (const char *) pName, sizeof ("xglEndDescriptorSetUpdate")))
        return (XGL_VOID *) pTable->EndDescriptorSetUpdate;
    else if (!strncmp("xglAttachSamplerDescriptors", (const char *) pName, sizeof ("xglAttachSamplerDescriptors")))
        return (XGL_VOID *) pTable->AttachSamplerDescriptors;
    else if (!strncmp("xglAttachImageViewDescriptors", (const char *) pName, sizeof ("xglAttachImageViewDescriptors")))
        return (XGL_VOID *) pTable->AttachImageViewDescriptors;
    else if (!strncmp("xglAttachMemoryViewDescriptors", (const char *) pName, sizeof ("xglAttachMemoryViewDescriptors")))
        return (XGL_VOID *) pTable->AttachMemoryViewDescriptors;
    else if (!strncmp("xglAttachNestedDescriptors", (const char *) pName, sizeof ("xglAttachNestedDescriptors")))
        return (XGL_VOID *) pTable->AttachNestedDescriptors;
    else if (!strncmp("xglClearDescriptorSetSlots", (const char *) pName, sizeof ("xglClearDescriptorSetSlots")))
        return (XGL_VOID *) pTable->ClearDescriptorSetSlots;
    else if (!strncmp("xglCreateViewportState", (const char *) pName, sizeof ("xglCreateViewportState")))
        return (XGL_VOID *) pTable->CreateViewportState;
    else if (!strncmp("xglCreateRasterState", (const char *) pName, sizeof ("xglCreateRasterState")))
        return (XGL_VOID *) pTable->CreateRasterState;
    else if (!strncmp("xglCreateMsaaState", (const char *) pName, sizeof ("xglCreateMsaaState")))
        return (XGL_VOID *) pTable->CreateMsaaState;
    else if (!strncmp("xglCreateColorBlendState", (const char *) pName, sizeof ("xglCreateColorBlendState")))
        return (XGL_VOID *) pTable->CreateColorBlendState;
    else if (!strncmp("xglCreateDepthStencilState", (const char *) pName, sizeof ("xglCreateDepthStencilState")))
        return (XGL_VOID *) pTable->CreateDepthStencilState;
    else if (!strncmp("xglCreateCommandBuffer", (const char *) pName, sizeof ("xglCreateCommandBuffer")))
        return (XGL_VOID *) pTable->CreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", (const char *) pName, sizeof ("xglBeginCommandBuffer")))
        return (XGL_VOID *) pTable->BeginCommandBuffer;
    else if (!strncmp("xglEndCommandBuffer", (const char *) pName, sizeof ("xglEndCommandBuffer")))
        return (XGL_VOID *) pTable->EndCommandBuffer;
    else if (!strncmp("xglResetCommandBuffer", (const char *) pName, sizeof ("xglResetCommandBuffer")))
        return (XGL_VOID *) pTable->ResetCommandBuffer;
    else if (!strncmp("xglCmdBindPipeline", (const char *) pName, sizeof ("xglCmdBindPipeline")))
        return (XGL_VOID *) pTable->CmdBindPipeline;
    else if (!strncmp("xglCmdBindPipelineDelta", (const char *) pName, sizeof ("xglCmdBindPipelineDelta")))
        return (XGL_VOID *) pTable->CmdBindPipelineDelta;
    else if (!strncmp("xglCmdBindStateObject", (const char *) pName, sizeof ("xglCmdBindStateObject")))
        return (XGL_VOID *) pTable->CmdBindStateObject;
    else if (!strncmp("xglCmdBindDescriptorSet", (const char *) pName, sizeof ("xglCmdBindDescriptorSet")))
        return (XGL_VOID *) pTable->CmdBindDescriptorSet;
    else if (!strncmp("xglCmdBindDynamicMemoryView", (const char *) pName, sizeof ("xglCmdBindDynamicMemoryView")))
        return (XGL_VOID *) pTable->CmdBindDynamicMemoryView;
    else if (!strncmp("xglCmdBindVertexData", (const char *) pName, sizeof ("xglCmdBindVertexData")))
        return (XGL_VOID *) pTable->CmdBindVertexData;
    else if (!strncmp("xglCmdBindIndexData", (const char *) pName, sizeof ("xglCmdBindIndexData")))
        return (XGL_VOID *) pTable->CmdBindIndexData;
    else if (!strncmp("xglCmdBindAttachments", (const char *) pName, sizeof ("xglCmdBindAttachments")))
        return (XGL_VOID *) pTable->CmdBindAttachments;
    else if (!strncmp("xglCmdPrepareMemoryRegions", (const char *) pName, sizeof ("xglCmdPrepareMemoryRegions")))
        return (XGL_VOID *) pTable->CmdPrepareMemoryRegions;
    else if (!strncmp("xglCmdPrepareImages", (const char *) pName, sizeof ("xglCmdPrepareImages")))
        return (XGL_VOID *) pTable->CmdPrepareImages;
    else if (!strncmp("xglCmdDraw", (const char *) pName, sizeof ("xglCmdDraw")))
        return (XGL_VOID *) pTable->CmdDraw;
    else if (!strncmp("xglCmdDrawIndexed", (const char *) pName, sizeof ("xglCmdDrawIndexed")))
        return (XGL_VOID *) pTable->CmdDrawIndexed;
    else if (!strncmp("xglCmdDrawIndirect", (const char *) pName, sizeof ("xglCmdDrawIndirect")))
        return (XGL_VOID *) pTable->CmdDrawIndirect;
    else if (!strncmp("xglCmdDrawIndexedIndirect", (const char *) pName, sizeof ("xglCmdDrawIndexedIndirect")))
        return (XGL_VOID *) pTable->CmdDrawIndexedIndirect;
    else if (!strncmp("xglCmdDispatch", (const char *) pName, sizeof ("xglCmdDispatch")))
        return (XGL_VOID *) pTable->CmdDispatch;
    else if (!strncmp("xglCmdDispatchIndirect", (const char *) pName, sizeof ("xglCmdDispatchIndirect")))
        return (XGL_VOID *) pTable->CmdDispatchIndirect;
    else if (!strncmp("xglCmdCopyMemory", (const char *) pName, sizeof ("xglCmdCopyMemory")))
        return (XGL_VOID *) pTable->CmdCopyMemory;
    else if (!strncmp("xglCmdCopyImage", (const char *) pName, sizeof ("xglCmdCopyImage")))
        return (XGL_VOID *) pTable->CmdCopyImage;
    else if (!strncmp("xglCmdCopyMemoryToImage", (const char *) pName, sizeof ("xglCmdCopyMemoryToImage")))
        return (XGL_VOID *) pTable->CmdCopyMemoryToImage;
    else if (!strncmp("xglCmdCopyImageToMemory", (const char *) pName, sizeof ("xglCmdCopyImageToMemory")))
        return (XGL_VOID *) pTable->CmdCopyImageToMemory;
    else if (!strncmp("xglCmdCloneImageData", (const char *) pName, sizeof ("xglCmdCloneImageData")))
        return (XGL_VOID *) pTable->CmdCloneImageData;
    else if (!strncmp("xglCmdUpdateMemory", (const char *) pName, sizeof ("xglCmdUpdateMemory")))
        return (XGL_VOID *) pTable->CmdUpdateMemory;
    else if (!strncmp("xglCmdFillMemory", (const char *) pName, sizeof ("xglCmdFillMemory")))
        return (XGL_VOID *) pTable->CmdFillMemory;
    else if (!strncmp("xglCmdClearColorImage", (const char *) pName, sizeof ("xglCmdClearColorImage")))
        return (XGL_VOID *) pTable->CmdClearColorImage;
    else if (!strncmp("xglCmdClearColorImageRaw", (const char *) pName, sizeof ("xglCmdClearColorImageRaw")))
        return (XGL_VOID *) pTable->CmdClearColorImageRaw;
    else if (!strncmp("xglCmdClearDepthStencil", (const char *) pName, sizeof ("xglCmdClearDepthStencil")))
        return (XGL_VOID *) pTable->CmdClearDepthStencil;
    else if (!strncmp("xglCmdResolveImage", (const char *) pName, sizeof ("xglCmdResolveImage")))
        return (XGL_VOID *) pTable->CmdResolveImage;
    else if (!strncmp("xglCmdSetEvent", (const char *) pName, sizeof ("xglCmdSetEvent")))
        return (XGL_VOID *) pTable->CmdSetEvent;
    else if (!strncmp("xglCmdResetEvent", (const char *) pName, sizeof ("xglCmdResetEvent")))
        return (XGL_VOID *) pTable->CmdResetEvent;
    else if (!strncmp("xglCmdMemoryAtomic", (const char *) pName, sizeof ("xglCmdMemoryAtomic")))
        return (XGL_VOID *) pTable->CmdMemoryAtomic;
    else if (!strncmp("xglCmdBeginQuery", (const char *) pName, sizeof ("xglCmdBeginQuery")))
        return (XGL_VOID *) pTable->CmdBeginQuery;
    else if (!strncmp("xglCmdEndQuery", (const char *) pName, sizeof ("xglCmdEndQuery")))
        return (XGL_VOID *) pTable->CmdEndQuery;
    else if (!strncmp("xglCmdResetQueryPool", (const char *) pName, sizeof ("xglCmdResetQueryPool")))
        return (XGL_VOID *) pTable->CmdResetQueryPool;
    else if (!strncmp("xglCmdWriteTimestamp", (const char *) pName, sizeof ("xglCmdWriteTimestamp")))
        return (XGL_VOID *) pTable->CmdWriteTimestamp;
    else if (!strncmp("xglCmdInitAtomicCounters", (const char *) pName, sizeof ("xglCmdInitAtomicCounters")))
        return (XGL_VOID *) pTable->CmdInitAtomicCounters;
    else if (!strncmp("xglCmdLoadAtomicCounters", (const char *) pName, sizeof ("xglCmdLoadAtomicCounters")))
        return (XGL_VOID *) pTable->CmdLoadAtomicCounters;
    else if (!strncmp("xglCmdSaveAtomicCounters", (const char *) pName, sizeof ("xglCmdSaveAtomicCounters")))
        return (XGL_VOID *) pTable->CmdSaveAtomicCounters;
    else if (!strncmp("xglDbgSetValidationLevel", (const char *) pName, sizeof ("xglDbgSetValidationLevel")))
        return (XGL_VOID *) pTable->DbgSetValidationLevel;
    else if (!strncmp("xglDbgRegisterMsgCallback", (const char *) pName, sizeof ("xglDbgRegisterMsgCallback")))
        return (XGL_VOID *) pTable->DbgRegisterMsgCallback;
    else if (!strncmp("xglDbgUnregisterMsgCallback", (const char *) pName, sizeof ("xglDbgUnregisterMsgCallback")))
        return (XGL_VOID *) pTable->DbgUnregisterMsgCallback;
    else if (!strncmp("xglDbgSetMessageFilter", (const char *) pName, sizeof ("xglDbgSetMessageFilter")))
        return (XGL_VOID *) pTable->DbgSetMessageFilter;
    else if (!strncmp("xglDbgSetObjectTag", (const char *) pName, sizeof ("xglDbgSetObjectTag")))
        return (XGL_VOID *) pTable->DbgSetObjectTag;
    else if (!strncmp("xglDbgSetGlobalOption", (const char *) pName, sizeof ("xglDbgSetGlobalOption")))
        return (XGL_VOID *) pTable->DbgSetGlobalOption;
    else if (!strncmp("xglDbgSetDeviceOption", (const char *) pName, sizeof ("xglDbgSetDeviceOption")))
        return (XGL_VOID *) pTable->DbgSetDeviceOption;
    else if (!strncmp("xglCmdDbgMarkerBegin", (const char *) pName, sizeof ("xglCmdDbgMarkerBegin")))
        return (XGL_VOID *) pTable->CmdDbgMarkerBegin;
    else if (!strncmp("xglCmdDbgMarkerEnd", (const char *) pName, sizeof ("xglCmdDbgMarkerEnd")))
        return (XGL_VOID *) pTable->CmdDbgMarkerEnd;
    else if (!strncmp("xglWsiX11AssociateConnection", (const char *) pName, sizeof("xglWsiX11AssociateConnection")))
        return (XGL_VOID *) pTable->WsiX11AssociateConnection;
    else if (!strncmp("xglWsiX11GetMSC", (const char *) pName, sizeof("xglWsiX11GetMSC")))
        return (XGL_VOID *) pTable->WsiX11GetMSC;
    else if (!strncmp("xglWsiX11CreatePresentableImage", (const char *) pName, sizeof("xglWsiX11CreatePresentableImage")))
        return (XGL_VOID *) pTable->WsiX11CreatePresentableImage;
    else if (!strncmp("xglWsiX11QueuePresent", (const char *) pName, sizeof("xglWsiX11QueuePresent")))
        return (XGL_VOID *) pTable->WsiX11QueuePresent;
    else if (!strncmp("xglLayerExtension1", (const char *) pName, sizeof("xglLayerExtension1")))
        return (XGL_VOID *) xglLayerExtension1;
    else {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, pName);
    }
}
