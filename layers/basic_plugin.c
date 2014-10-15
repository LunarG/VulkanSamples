#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include "xglLayer.h"

static XGL_LAYER_DISPATCH_TABLE myTable;
static XGL_BASE_LAYER_OBJECT *pCurObj;
static pthread_once_t tabOnce = PTHREAD_ONCE_INIT;

static void initLayerTable()
{
    GetProcAddrType fpGPA;

    //memset(&myTable, 0 , sizeof(myTable));

    //todo init the entire table to next entrypoint
    fpGPA = pCurObj->pGPA;
    assert(fpGPA);
    // Layer
    myTable.GetProcAddr = fpGPA;
    myTable.InitAndEnumerateGpus = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglInitAndEnumerateGpus");
    myTable.GetGpuInfo = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetGpuInfo");
    myTable.CreateDevice = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateDevice");
    myTable.DestroyDevice = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDestroyDevice");
    myTable.GetExtensionSupport = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetExtensionSupport");
    myTable.GetDeviceQueue = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetDeviceQueue");
    myTable.QueueSubmit = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglQueueSubmit");
    myTable.QueueSetGlobalMemReferences = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglQueueSetGlobalMemReferences");
    myTable.QueueWaitIdle = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglQueueWaitIdle");
    myTable.DeviceWaitIdle = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDeviceWaitIdle");
    myTable.GetMemoryHeapCount = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetMemoryHeapCount");
    myTable.GetMemoryHeapInfo = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetMemoryHeapInfo");
    myTable.AllocMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglAllocMemory");
    myTable.FreeMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglFreeMemory");
    myTable.SetMemoryPriority = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglSetMemoryPriority");
    myTable.MapMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglMapMemory");
    myTable.UnmapMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglUnmapMemory");
    myTable.PinSystemMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglPinSystemMemory");
    myTable.RemapVirtualMemoryPages = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglRemapVirtualMemoryPages");
    myTable.GetMultiGpuCompatibility = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetMultiGpuCompatibility");
    myTable.OpenSharedMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglOpenSharedMemory");
    myTable.OpenSharedQueueSemaphore = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglOpenSharedQueueSemaphore");
    myTable.OpenPeerMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglOpenPeerMemory");
    myTable.OpenPeerImage = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglOpenPeerImage");
    myTable.DestroyObject = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDestroyObject");
    myTable.GetObjectInfo = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetObjectInfo");
    myTable.BindObjectMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglBindObjectMemory");
    myTable.CreateFence = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateFence");
    myTable.GetFenceStatus = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetFenceStatus");
    myTable.WaitForFences = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglWaitForFences");
    myTable.CreateQueueSemaphore = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateQueueSemaphore");
    myTable.SignalQueueSemaphore = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglSignalQueueSemaphore");
    myTable.WaitQueueSemaphore = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglWaitQueueSemaphore");
    myTable.CreateEvent = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateEvent");
    myTable.GetEventStatus = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetEventStatus");
    myTable.SetEvent = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglSetEvent");
    myTable.ResetEvent = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglResetEvent");
    myTable.CreateQueryPool = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateQueryPool");
    myTable.GetQueryPoolResults = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetQueryPoolResults");
    myTable.GetFormatInfo = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetFormatInfo");
    myTable.CreateImage = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateImage");
    myTable.GetImageSubresourceInfo = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglGetImageSubresourceInfo");
    myTable.CreateImageView = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateImageView");
    myTable.CreateColorAttachmentView = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateColorAttachmentView");
    myTable.CreateDepthStencilView = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateDepthStencilView");
    myTable.CreateShader = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateShader");
    myTable.CreateGraphicsPipeline = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateGraphicsPipeline");
    myTable.CreateComputePipeline = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateComputePipeline");
    myTable.StorePipeline = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglStorePipeline");
    myTable.LoadPipeline = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglLoadPipeline");
    myTable.CreatePipelineDelta = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreatePipelineDelta");
    myTable.CreateSampler = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateSampler");
    myTable.CreateDescriptorSet = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateDescriptorSet");
    myTable.BeginDescriptorSetUpdate = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglBeginDescriptorSetUpdate");
    myTable.EndDescriptorSetUpdate = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglEndDescriptorSetUpdate");
    myTable.AttachSamplerDescriptors = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglAttachSamplerDescriptors");
    myTable.AttachImageViewDescriptors = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglAttachImageViewDescriptors");
    myTable.AttachMemoryViewDescriptors = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglAttachMemoryViewDescriptors");
    myTable.AttachNestedDescriptors = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglAttachNestedDescriptors");
    myTable.ClearDescriptorSetSlots = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglClearDescriptorSetSlots");
    myTable.CreateViewportState = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateViewportState");
    myTable.CreateRasterState = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateRasterState");
    myTable.CreateMsaaState = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateMsaaState");
    myTable.CreateColorBlendState = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateColorBlendState");
    myTable.CreateDepthStencilState = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateDepthStencilState");
    myTable.CreateCommandBuffer = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCreateCommandBuffer");
    myTable.BeginCommandBuffer = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglBeginCommandBuffer");
    myTable.EndCommandBuffer = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglEndCommandBuffer");
    myTable.ResetCommandBuffer = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglResetCommandBuffer");
    myTable.CmdBindPipeline = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdBindPipeline");
    myTable.CmdBindPipelineDelta = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdBindPipelineDelta");
    myTable.CmdBindStateObject = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdBindStateObject");
    myTable.CmdBindDescriptorSet = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdBindDescriptorSet");
    myTable.CmdBindDynamicMemoryView = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdBindDynamicMemoryView");
    myTable.CmdBindIndexData = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdBindIndexData");
    myTable.CmdBindAttachments = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdBindAttachments");
    myTable.CmdPrepareMemoryRegions = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdPrepareMemoryRegions");
    myTable.CmdPrepareImages = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdPrepareImages");
    myTable.CmdDraw = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdDraw");
    myTable.CmdDrawIndexed = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdDrawIndexed");
    myTable.CmdDrawIndirect = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdDrawIndirect");
    myTable.CmdDrawIndexedIndirect = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdDrawIndexedIndirect");
    myTable.CmdDispatch = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdDispatch");
    myTable.CmdDispatchIndirect = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdDispatchIndirect");
    myTable.CmdCopyMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdCopyMemory");
    myTable.CmdCopyImage = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdCopyImage");
    myTable.CmdCopyMemoryToImage = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdCopyMemoryToImage");
    myTable.CmdCopyImageToMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdCopyImageToMemory");
    myTable.CmdCloneImageData = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdCloneImageData");
    myTable.CmdUpdateMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdUpdateMemory");
    myTable.CmdFillMemory = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdFillMemory");
    myTable.CmdClearColorImage = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdClearColorImage");
    myTable.CmdClearColorImageRaw = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdClearColorImageRaw");
    myTable.CmdClearDepthStencil = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdClearDepthStencil");
    myTable.CmdResolveImage = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdResolveImage");
    myTable.CmdSetEvent = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdSetEvent");
    myTable.CmdResetEvent = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdResetEvent");
    myTable.CmdMemoryAtomic = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdMemoryAtomic");
    myTable.CmdBeginQuery = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdBeginQuery");
    myTable.CmdEndQuery = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdEndQuery");
    myTable.CmdResetQueryPool = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdResetQueryPool");
    myTable.CmdWriteTimestamp = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdWriteTimestamp");
    myTable.CmdInitAtomicCounters = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdInitAtomicCounters");
    myTable.CmdLoadAtomicCounters = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdLoadAtomicCounters");
    myTable.CmdSaveAtomicCounters = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdSaveAtomicCounters");
    myTable.DbgSetValidationLevel = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDbgSetValidationLevel");
    myTable.DbgRegisterMsgCallback = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDbgRegisterMsgCallback");
    myTable.DbgUnregisterMsgCallback = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDbgUnregisterMsgCallback");
    myTable.DbgSetMessageFilter = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDbgSetMessageFilter");
    myTable.DbgSetObjectTag = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDbgSetObjectTag");
    myTable.DbgSetGlobalOption = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDbgSetGlobalOption");
    myTable.DbgSetDeviceOption = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglDbgSetDeviceOption");
    myTable.CmdDbgMarkerBegin = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdDbgMarkerBegin");
    myTable.CmdDbgMarkerEnd = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglCmdDbgMarkerEnd");
    myTable.WsiX11AssociateConnection = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglWsiX11AssociateConnection");
    myTable.WsiX11GetMSC = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglWsiX11GetMSC");
    myTable.WsiX11CreatePresentableImage = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglWsiX11CreatePresentableImage");
    myTable.WsiX11QueuePresent = fpGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (const XGL_CHAR *) "xglWsiX11QueuePresent");
#if 0
    fpNextCD = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateDevice");
    myTable.CreateDevice = fpNextCD;
    fpNextGGI = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetGpuInfo");
    myTable.GetGpuInfo = fpNextGGI;
    fpNextGFI = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetFormatInfo");
    myTable.GetFormatInfo = fpNextGFI;
#endif
    return;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetGpuInfo(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    printf("At start of wrapped xglCreateDevice() call w/ gpu: %p\n", (void*)gpu);
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable);
    XGL_RESULT result = myTable.GetGpuInfo((XGL_PHYSICAL_GPU)gpuw->nextObject, infoType, pDataSize, pData);
    printf("Completed wrapped xglCreateDevice() call w/ gpu: %p\n", (void*)gpu);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    printf("At start of wrapped xglCreateDevice() call w/ gpu: %p\n", (void*)gpu);
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable); 
    XGL_RESULT result = myTable.CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    printf("Completed wrapped xglCreateDevice() call w/ pDevice: %p\n", (void*)pDevice);
    return result;
}
XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    printf("At start of wrapped xglGetFormatInfo() call w/ device: %p\n", (void*)device);
    XGL_RESULT result = myTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    printf("Completed wrapped xglGetFormatInfo() call w/ device: %p\n", (void*)device);
       return result;
}

XGL_LAYER_EXPORT void * XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pName) {
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable);
    if (!strncmp("xglGetProcAddr", (const char *) pName, sizeof("xglGetProcAddr")))
        return xglGetProcAddr;
    else if (!strncmp("xglInitAndEnumerateGpus", (const char *) pName, sizeof("xglInitAndEnumerateGpus")))
        return myTable.InitAndEnumerateGpus;
    if (!strncmp("xglGetGpuInfo", (const char *) pName, sizeof ("xglGetGpuInfo")))
        return xglGetGpuInfo;
    else if (!strncmp("xglCreateDevice", (const char *) pName, sizeof ("xglCreateDevice")))
        return xglCreateDevice;
    else if (!strncmp("xglDestroyDevice", (const char *) pName, sizeof ("xglDestroyDevice")))
        return myTable.DestroyDevice;
    else if (!strncmp("xglGetExtensionSupport", (const char *) pName, sizeof ("xglGetExtensionSupport")))
        return myTable.GetExtensionSupport;
    else if (!strncmp("xglGetDeviceQueue", (const char *) pName, sizeof ("xglGetDeviceQueue")))
        return myTable.GetDeviceQueue;
    else if (!strncmp("xglQueueSubmit", (const char *) pName, sizeof ("xglQueueSubmit")))
        return myTable.QueueSubmit;
    else if (!strncmp("xglQueueSetGlobalMemReferences", (const char *) pName, sizeof ("xglQueueSetGlobalMemReferences")))
        return myTable.QueueSetGlobalMemReferences;
    else if (!strncmp("xglQueueWaitIdle", (const char *) pName, sizeof ("xglQueueWaitIdle")))
        return myTable.QueueWaitIdle;
    else if (!strncmp("xglDeviceWaitIdle", (const char *) pName, sizeof ("xglDeviceWaitIdle")))
        return myTable.DeviceWaitIdle;
    else if (!strncmp("xglGetMemoryHeapCount", (const char *) pName, sizeof ("xglGetMemoryHeapCount")))
        return myTable.GetMemoryHeapCount;
    else if (!strncmp("xglGetMemoryHeapInfo", (const char *) pName, sizeof ("xglGetMemoryHeapInfo")))
        return myTable.GetMemoryHeapInfo;
    else if (!strncmp("xglAllocMemory", (const char *) pName, sizeof ("xglAllocMemory")))
        return myTable.AllocMemory;
    else if (!strncmp("xglFreeMemory", (const char *) pName, sizeof ("xglFreeMemory")))
        return myTable.FreeMemory;
    else if (!strncmp("xglSetMemoryPriority", (const char *) pName, sizeof ("xglSetMemoryPriority")))
        return myTable.SetMemoryPriority;
    else if (!strncmp("xglMapMemory", (const char *) pName, sizeof ("xglMapMemory")))
        return myTable.MapMemory;
    else if (!strncmp("xglUnmapMemory", (const char *) pName, sizeof ("xglUnmapMemory")))
        return myTable.UnmapMemory;
    else if (!strncmp("xglPinSystemMemory", (const char *) pName, sizeof ("xglPinSystemMemory")))
        return myTable.PinSystemMemory;
    else if (!strncmp("xglRemapVirtualMemoryPages", (const char *) pName, sizeof ("xglRemapVirtualMemoryPages")))
        return myTable.RemapVirtualMemoryPages;
    else if (!strncmp("xglGetMultiGpuCompatibility", (const char *) pName, sizeof ("xglGetMultiGpuCompatibility")))
        return myTable.GetMultiGpuCompatibility;
    else if (!strncmp("xglOpenSharedMemory", (const char *) pName, sizeof ("xglOpenSharedMemory")))
        return myTable.OpenSharedMemory;
    else if (!strncmp("xglOpenSharedQueueSemaphore", (const char *) pName, sizeof ("xglOpenSharedQueueSemaphore")))
        return myTable.OpenSharedQueueSemaphore;
    else if (!strncmp("xglOpenPeerMemory", (const char *) pName, sizeof ("xglOpenPeerMemory")))
        return myTable.OpenPeerMemory;
    else if (!strncmp("xglOpenPeerImage", (const char *) pName, sizeof ("xglOpenPeerImage")))
        return myTable.OpenPeerImage;
    else if (!strncmp("xglDestroyObject", (const char *) pName, sizeof ("xglDestroyObject")))
        return myTable.DestroyObject;
    else if (!strncmp("xglGetObjectInfo", (const char *) pName, sizeof ("xglGetObjectInfo")))
        return myTable.GetObjectInfo;
    else if (!strncmp("xglBindObjectMemory", (const char *) pName, sizeof ("xglBindObjectMemory")))
        return myTable.BindObjectMemory;
    else if (!strncmp("xglCreateFence", (const char *) pName, sizeof ("xgllCreateFence")))
        return myTable.CreateFence;
    else if (!strncmp("xglGetFenceStatus", (const char *) pName, sizeof ("xglGetFenceStatus")))
        return myTable.GetFenceStatus;
    else if (!strncmp("xglWaitForFences", (const char *) pName, sizeof ("xglWaitForFences")))
        return myTable.WaitForFences;
    else if (!strncmp("xglCreateQueueSemaphore", (const char *) pName, sizeof ("xgllCreateQueueSemaphore")))
        return myTable.CreateQueueSemaphore;
    else if (!strncmp("xglSignalQueueSemaphore", (const char *) pName, sizeof ("xglSignalQueueSemaphore")))
        return myTable.SignalQueueSemaphore;
    else if (!strncmp("xglWaitQueueSemaphore", (const char *) pName, sizeof ("xglWaitQueueSemaphore")))
        return myTable.WaitQueueSemaphore;
    else if (!strncmp("xglCreateEvent", (const char *) pName, sizeof ("xgllCreateEvent")))
        return myTable.CreateEvent;
    else if (!strncmp("xglGetEventStatus", (const char *) pName, sizeof ("xglGetEventStatus")))
        return myTable.GetEventStatus;
    else if (!strncmp("xglSetEvent", (const char *) pName, sizeof ("xglSetEvent")))
        return myTable.SetEvent;
    else if (!strncmp("xglResetEvent", (const char *) pName, sizeof ("xgllResetEvent")))
        return myTable.ResetEvent;
    else if (!strncmp("xglCreateQueryPool", (const char *) pName, sizeof ("xglCreateQueryPool")))
        return myTable.CreateQueryPool;
    else if (!strncmp("xglGetQueryPoolResults", (const char *) pName, sizeof ("xglGetQueryPoolResults")))
        return myTable.GetQueryPoolResults;
    else if (!strncmp("xglGetFormatInfo", (const char *) pName, sizeof ("xgllGetFormatInfo")))
        return xglGetFormatInfo;
    else if (!strncmp("xglCreateImage", (const char *) pName, sizeof ("xglCreateImage")))
        return myTable.CreateImage;
    else if (!strncmp("xglGetImageSubresourceInfo", (const char *) pName, sizeof ("xglGetImageSubresourceInfo")))
        return myTable.GetImageSubresourceInfo;
    else if (!strncmp("xglCreateImageView", (const char *) pName, sizeof ("xglCreateImageView")))
        return myTable.CreateImageView;
    else if (!strncmp("xglCreateColorAttachmentView", (const char *) pName, sizeof ("xglCreateColorAttachmentView")))
        return myTable.CreateColorAttachmentView;
    else if (!strncmp("xglCreateDepthStencilView", (const char *) pName, sizeof ("xglCreateDepthStencilView")))
        return myTable.CreateDepthStencilView;
    else if (!strncmp("xglCreateShader", (const char *) pName, sizeof ("xglCreateShader")))
        return myTable.CreateShader;
    else if (!strncmp("xglCreateGraphicsPipeline", (const char *) pName, sizeof ("xglCreateGraphicsPipeline")))
        return myTable.CreateGraphicsPipeline;
    else if (!strncmp("xglCreateComputePipeline", (const char *) pName, sizeof ("xglCreateComputePipeline")))
        return myTable.CreateComputePipeline;
    else if (!strncmp("xglStorePipeline", (const char *) pName, sizeof ("xglStorePipeline")))
        return myTable.StorePipeline;
    else if (!strncmp("xglLoadPipeline", (const char *) pName, sizeof ("xglLoadPipeline")))
        return myTable.LoadPipeline;
    else if (!strncmp("xglCreatePipelineDelta", (const char *) pName, sizeof ("xglCreatePipelineDelta")))
        return myTable.CreatePipelineDelta;
    else if (!strncmp("xglCreateSampler", (const char *) pName, sizeof ("xglCreateSampler")))
        return myTable.CreateSampler;
    else if (!strncmp("xglCreateDescriptorSet", (const char *) pName, sizeof ("xglCreateDescriptorSet")))
        return myTable.CreateDescriptorSet;
    else if (!strncmp("xglBeginDescriptorSetUpdate", (const char *) pName, sizeof ("xglBeginDescriptorSetUpdate")))
        return myTable.BeginDescriptorSetUpdate;
    else if (!strncmp("xglEndDescriptorSetUpdate", (const char *) pName, sizeof ("xglEndDescriptorSetUpdate")))
        return myTable.EndDescriptorSetUpdate;
    else if (!strncmp("xglAttachSamplerDescriptors", (const char *) pName, sizeof ("xglAttachSamplerDescriptors")))
        return myTable.AttachSamplerDescriptors;
    else if (!strncmp("xglAttachImageViewDescriptors", (const char *) pName, sizeof ("xglAttachImageViewDescriptors")))
        return myTable.AttachImageViewDescriptors;
    else if (!strncmp("xglAttachMemoryViewDescriptors", (const char *) pName, sizeof ("xglAttachMemoryViewDescriptors")))
        return myTable.AttachMemoryViewDescriptors;
    else if (!strncmp("xglAttachNestedDescriptors", (const char *) pName, sizeof ("xglAttachNestedDescriptors")))
        return myTable.AttachNestedDescriptors;
    else if (!strncmp("xglClearDescriptorSetSlots", (const char *) pName, sizeof ("xglClearDescriptorSetSlots")))
        return myTable.ClearDescriptorSetSlots;
    else if (!strncmp("xglCreateViewportState", (const char *) pName, sizeof ("xglCreateViewportState")))
        return myTable.CreateViewportState;
    else if (!strncmp("xglCreateRasterState", (const char *) pName, sizeof ("xglCreateRasterState")))
        return myTable.CreateRasterState;
    else if (!strncmp("xglCreateMsaaState", (const char *) pName, sizeof ("xglCreateMsaaState")))
        return myTable.CreateMsaaState;
    else if (!strncmp("xglCreateColorBlendState", (const char *) pName, sizeof ("xglCreateColorBlendState")))
        return myTable.CreateColorBlendState;
    else if (!strncmp("xglCreateDepthStencilState", (const char *) pName, sizeof ("xglCreateDepthStencilState")))
        return myTable.CreateDepthStencilState;
    else if (!strncmp("xglCreateCommandBuffer", (const char *) pName, sizeof ("xglCreateCommandBuffer")))
        return myTable.CreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", (const char *) pName, sizeof ("xglBeginCommandBuffer")))
        return myTable.BeginCommandBuffer;
    else if (!strncmp("xglEndCommandBuffer", (const char *) pName, sizeof ("xglEndCommandBuffer")))
        return myTable.EndCommandBuffer;
    else if (!strncmp("xglResetCommandBuffer", (const char *) pName, sizeof ("xglResetCommandBuffer")))
        return myTable.ResetCommandBuffer;
    else if (!strncmp("xglCmdBindPipeline", (const char *) pName, sizeof ("xglCmdBindPipeline")))
        return myTable.CmdBindPipeline;
    else if (!strncmp("xglCmdBindPipelineDelta", (const char *) pName, sizeof ("xglCmdBindPipelineDelta")))
        return myTable.CmdBindPipelineDelta;
    else if (!strncmp("xglCmdBindStateObject", (const char *) pName, sizeof ("xglCmdBindStateObject")))
        return myTable.CmdBindStateObject;
    else if (!strncmp("xglCmdBindDescriptorSet", (const char *) pName, sizeof ("xglCmdBindDescriptorSet")))
        return myTable.CmdBindDescriptorSet;
    else if (!strncmp("xglCmdBindDynamicMemoryView", (const char *) pName, sizeof ("xglCmdBindDynamicMemoryView")))
        return myTable.CmdBindDynamicMemoryView;
    else if (!strncmp("xglCmdBindIndexData", (const char *) pName, sizeof ("xglCmdBindIndexData")))
        return myTable.CmdBindIndexData;
    else if (!strncmp("xglCmdBindAttachments", (const char *) pName, sizeof ("xglCmdBindAttachments")))
        return myTable.CmdBindAttachments;
    else if (!strncmp("xglCmdPrepareMemoryRegions", (const char *) pName, sizeof ("xglCmdPrepareMemoryRegions")))
        return myTable.CmdPrepareMemoryRegions;
    else if (!strncmp("xglCmdPrepareImages", (const char *) pName, sizeof ("xglCmdPrepareImages")))
        return myTable.CmdPrepareImages;
    else if (!strncmp("xglCmdDraw", (const char *) pName, sizeof ("xglCmdDraw")))
        return myTable.CmdDraw;
    else if (!strncmp("xglCmdDrawIndexed", (const char *) pName, sizeof ("xglCmdDrawIndexed")))
        return myTable.CmdDrawIndexed;
    else if (!strncmp("xglCmdDrawIndirect", (const char *) pName, sizeof ("xglCmdDrawIndirect")))
        return myTable.CmdDrawIndirect;
    else if (!strncmp("xglCmdDrawIndexedIndirect", (const char *) pName, sizeof ("xglCmdDrawIndexedIndirect")))
        return myTable.CmdDrawIndexedIndirect;
    else if (!strncmp("xglCmdDispatch", (const char *) pName, sizeof ("xglCmdDispatch")))
        return myTable.CmdDispatch;
    else if (!strncmp("xglCmdDispatchIndirect", (const char *) pName, sizeof ("xglCmdDispatchIndirect")))
        return myTable.CmdDispatchIndirect;
    else if (!strncmp("xglCmdCopyMemory", (const char *) pName, sizeof ("xglCmdCopyMemory")))
        return myTable.CmdCopyMemory;
    else if (!strncmp("xglCmdCopyImage", (const char *) pName, sizeof ("xglCmdCopyImage")))
        return myTable.CmdCopyImage;
    else if (!strncmp("xglCmdCopyMemoryToImage", (const char *) pName, sizeof ("xglCmdCopyMemoryToImage")))
        return myTable.CmdCopyMemoryToImage;
    else if (!strncmp("xglCmdCopyImageToMemory", (const char *) pName, sizeof ("xglCmdCopyImageToMemory")))
        return myTable.CmdCopyImageToMemory;
    else if (!strncmp("xglCmdCloneImageData", (const char *) pName, sizeof ("xglCmdCloneImageData")))
        return myTable.CmdCloneImageData;
    else if (!strncmp("xglCmdUpdateMemory", (const char *) pName, sizeof ("xglCmdUpdateMemory")))
        return myTable.CmdUpdateMemory;
    else if (!strncmp("xglCmdFillMemory", (const char *) pName, sizeof ("xglCmdFillMemory")))
        return myTable.CmdFillMemory;
    else if (!strncmp("xglCmdClearColorImage", (const char *) pName, sizeof ("xglCmdClearColorImage")))
        return myTable.CmdClearColorImage;
    else if (!strncmp("xglCmdClearColorImageRaw", (const char *) pName, sizeof ("xglCmdClearColorImageRaw")))
        return myTable.CmdClearColorImageRaw;
    else if (!strncmp("xglCmdClearDepthStencil", (const char *) pName, sizeof ("xglCmdClearDepthStencil")))
        return myTable.CmdClearDepthStencil;
    else if (!strncmp("xglCmdResolveImage", (const char *) pName, sizeof ("xglCmdResolveImage")))
        return myTable.CmdResolveImage;
    else if (!strncmp("xglCmdSetEvent", (const char *) pName, sizeof ("xglCmdSetEvent")))
        return myTable.CmdSetEvent;
    else if (!strncmp("xglCmdResetEvent", (const char *) pName, sizeof ("xglCmdResetEvent")))
        return myTable.CmdResetEvent;
    else if (!strncmp("xglCmdMemoryAtomic", (const char *) pName, sizeof ("xglCmdMemoryAtomic")))
        return myTable.CmdMemoryAtomic;
    else if (!strncmp("xglCmdBeginQuery", (const char *) pName, sizeof ("xglCmdBeginQuery")))
        return myTable.CmdBeginQuery;
    else if (!strncmp("xglCmdEndQuery", (const char *) pName, sizeof ("xglCmdEndQuery")))
        return myTable.CmdEndQuery;
    else if (!strncmp("xglCmdResetQueryPool", (const char *) pName, sizeof ("xglCmdResetQueryPool")))
        return myTable.CmdResetQueryPool;
    else if (!strncmp("xglCmdWriteTimestamp", (const char *) pName, sizeof ("xglCmdWriteTimestamp")))
        return myTable.CmdWriteTimestamp;
    else if (!strncmp("xglCmdInitAtomicCounters", (const char *) pName, sizeof ("xglCmdInitAtomicCounters")))
        return myTable.CmdInitAtomicCounters;
    else if (!strncmp("xglCmdLoadAtomicCounters", (const char *) pName, sizeof ("xglCmdLoadAtomicCounters")))
        return myTable.CmdLoadAtomicCounters;
    else if (!strncmp("xglCmdSaveAtomicCounters", (const char *) pName, sizeof ("xglCmdSaveAtomicCounters")))
        return myTable.CmdSaveAtomicCounters;
    else if (!strncmp("xglDbgSetValidationLevel", (const char *) pName, sizeof ("xglDbgSetValidationLevel")))
        return myTable.DbgSetValidationLevel;
	else if (!strncmp("xglDbgRegisterMsgCallback", (const char *) pName, sizeof ("xglDbgRegisterMsgCallback")))
        return myTable.DbgRegisterMsgCallback;
	else if (!strncmp("xglDbgUnregisterMsgCallback", (const char *) pName, sizeof ("xglDbgUnregisterMsgCallback")))
        return myTable.DbgUnregisterMsgCallback;
    else if (!strncmp("xglDbgSetMessageFilter", (const char *) pName, sizeof ("xglDbgSetMessageFilter")))
        return myTable.DbgSetMessageFilter;
    else if (!strncmp("xglDbgSetObjectTag", (const char *) pName, sizeof ("xglDbgSetObjectTag")))
        return myTable.DbgSetObjectTag;
    else if (!strncmp("xglDbgSetGlobalOption", (const char *) pName, sizeof ("xglDbgSetGlobalOption")))
        return myTable.DbgSetGlobalOption;
    else if (!strncmp("xglDbgSetDeviceOption", (const char *) pName, sizeof ("xglDbgSetDeviceOption")))
        return myTable.DbgSetDeviceOption;
    else if (!strncmp("xglCmdDbgMarkerBegin", (const char *) pName, sizeof ("xglCmdDbgMarkerBegin")))
        return myTable.CmdDbgMarkerBegin;
    else if (!strncmp("xglCmdDbgMarkerEnd", (const char *) pName, sizeof ("xglCmdDbgMarkerEnd")))
        return myTable.CmdDbgMarkerEnd;
    else if (!strncmp("xglWsiX11AssociateConnection", (const char *) pName, sizeof("xglWsiX11AssociateConnection")))
        return myTable.WsiX11AssociateConnection;
    else if (!strncmp("xglWsiX11GetMSC", (const char *) pName, sizeof("xglWsiX11GetMSC")))
        return myTable.WsiX11GetMSC;
    else if (!strncmp("xglWsiX11CreatePresentableImage", (const char *) pName, sizeof("xglWsiX11CreatePresentableImage")))
        return myTable.WsiX11CreatePresentableImage;
    else if (!strncmp("xglWsiX11QueuePresent", (const char *) pName, sizeof("xglWsiX11QueuePresent")))
        return myTable.WsiX11QueuePresent;
    else {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA(gpuw->nextObject, pName);
    }

}

