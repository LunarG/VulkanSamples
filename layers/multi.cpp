/*
 * XGL
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
 *
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>
#include "xgl_dispatch_table_helper.h"
#include "xglLayer.h"

static void initLayerTable(const XGL_BASE_LAYER_OBJECT *gpuw, XGL_LAYER_DISPATCH_TABLE *pTable, const unsigned int layerNum);

/******************************** Layer multi1 functions **************************/
static std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *> tableMap1;
static bool layer1_first_activated = false;

static XGL_LAYER_DISPATCH_TABLE * getLayer1Table(const XGL_BASE_LAYER_OBJECT *gpuw)
{
    XGL_LAYER_DISPATCH_TABLE *pTable;

    assert(gpuw);
    std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *>::const_iterator it = tableMap1.find((XGL_VOID *) gpuw);
    if (it == tableMap1.end())
    {
        pTable =  new XGL_LAYER_DISPATCH_TABLE;
        tableMap1[(XGL_VOID *) gpuw] = pTable;
        initLayerTable(gpuw, pTable, 1);
        return pTable;
    } else
    {
        return it->second;
    }
}
#ifdef __cplusplus
extern "C" {
#endif


XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi1CreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo,
                                                      XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = getLayer1Table(gpuw);

    printf("At start of multi1 layer xglCreateDevice()\n");
    XGL_RESULT result = pTable->CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table
    tableMap1.emplace(*pDevice, pTable);
    printf("Completed multi1 layer xglCreateDevice()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi1CreateGraphicsPipeline(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
                                                                XGL_PIPELINE* pPipeline)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap1[device];

    printf("At start of multi1 layer xglCreateGraphicsPipeline()\n");
    XGL_RESULT result = pTable->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    // create a mapping for the pipeline object into the dispatch table
    tableMap1.emplace(*pPipeline, pTable);
    printf("Completed multi1 layer xglCreateGraphicsPipeline()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi1StorePipeline(XGL_PIPELINE pipeline, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap1[pipeline];

    printf("At start of multi1 layer xglStorePipeline()\n");
    XGL_RESULT result = pTable->StorePipeline(pipeline, pDataSize, pData);
    printf("Completed multi1 layer xglStorePipeline()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi1EnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize,
                                                         XGL_CHAR* const* pOutLayers, XGL_SIZE * pOutLayerCount,
                                                         XGL_VOID* pReserved)
{
    if (gpu == NULL)
        return xglEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayers, pOutLayerCount, pReserved);

    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = getLayer1Table(gpuw);

    printf("At start of multi1 layer xglEnumerateLayers()\n");
    XGL_RESULT result = pTable->EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayers, pOutLayerCount, pReserved);
    printf("Completed multi1 layer xglEnumerateLayers()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_VOID * XGLAPI multi1GetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    if (gpu == NULL)
        return NULL;
    XGL_LAYER_DISPATCH_TABLE* pTable;
    pTable = getLayer1Table(gpuw);
    if (!strncmp("xglInitAndEnumerateGpus", pName, sizeof("xglInitAndEnumerateGpus")))
        return (XGL_VOID *) pTable->InitAndEnumerateGpus;
    else if (!strncmp("xglGetGpuInfo", pName, sizeof ("xglGetGpuInfo")))
        return (XGL_VOID *) pTable->GetGpuInfo;
    else if (!strncmp("xglCreateDevice", pName, sizeof ("xglCreateDevice")))
        return (XGL_VOID *) multi1CreateDevice;
    else if (!strncmp("xglDestroyDevice", pName, sizeof ("xglDestroyDevice")))
        return (XGL_VOID *) pTable->DestroyDevice;
    else if (!strncmp("xglGetExtensionSupport", pName, sizeof ("xglGetExtensionSupport")))
        return (XGL_VOID *) pTable->GetExtensionSupport;
    else if (!strncmp("xglEnumerateLayers", pName, sizeof ("xglEnumerateLayers")))
        return (XGL_VOID *) multi1EnumerateLayers;
    else if (!strncmp("xglGetDeviceQueue", pName, sizeof ("xglGetDeviceQueue")))
        return (XGL_VOID *) pTable->GetDeviceQueue;
    else if (!strncmp("xglQueueSubmit", pName, sizeof ("xglQueueSubmit")))
        return (XGL_VOID *) pTable->QueueSubmit;
    else if (!strncmp("xglQueueSetGlobalMemReferences", pName, sizeof ("xglQueueSetGlobalMemReferences")))
        return (XGL_VOID *) pTable->QueueSetGlobalMemReferences;
    else if (!strncmp("xglQueueWaitIdle", pName, sizeof ("xglQueueWaitIdle")))
        return (XGL_VOID *) pTable->QueueWaitIdle;
    else if (!strncmp("xglDeviceWaitIdle", pName, sizeof ("xglDeviceWaitIdle")))
        return (XGL_VOID *) pTable->DeviceWaitIdle;
    else if (!strncmp("xglGetMemoryHeapCount", pName, sizeof ("xglGetMemoryHeapCount")))
        return (XGL_VOID *) pTable->GetMemoryHeapCount;
    else if (!strncmp("xglGetMemoryHeapInfo", pName, sizeof ("xglGetMemoryHeapInfo")))
        return (XGL_VOID *) pTable->GetMemoryHeapInfo;
    else if (!strncmp("xglAllocMemory", pName, sizeof ("xglAllocMemory")))
        return (XGL_VOID *) pTable->AllocMemory;
    else if (!strncmp("xglFreeMemory", pName, sizeof ("xglFreeMemory")))
        return (XGL_VOID *) pTable->FreeMemory;
    else if (!strncmp("xglSetMemoryPriority", pName, sizeof ("xglSetMemoryPriority")))
        return (XGL_VOID *) pTable->SetMemoryPriority;
    else if (!strncmp("xglMapMemory", pName, sizeof ("xglMapMemory")))
        return (XGL_VOID *) pTable->MapMemory;
    else if (!strncmp("xglUnmapMemory", pName, sizeof ("xglUnmapMemory")))
        return (XGL_VOID *) pTable->UnmapMemory;
    else if (!strncmp("xglPinSystemMemory", pName, sizeof ("xglPinSystemMemory")))
        return (XGL_VOID *) pTable->PinSystemMemory;
    else if (!strncmp("xglRemapVirtualMemoryPages", pName, sizeof ("xglRemapVirtualMemoryPages")))
        return (XGL_VOID *) pTable->RemapVirtualMemoryPages;
    else if (!strncmp("xglGetMultiGpuCompatibility", pName, sizeof ("xglGetMultiGpuCompatibility")))
        return (XGL_VOID *) pTable->GetMultiGpuCompatibility;
    else if (!strncmp("xglOpenSharedMemory", pName, sizeof ("xglOpenSharedMemory")))
        return (XGL_VOID *) pTable->OpenSharedMemory;
    else if (!strncmp("xglOpenSharedQueueSemaphore", pName, sizeof ("xglOpenSharedQueueSemaphore")))
        return (XGL_VOID *) pTable->OpenSharedQueueSemaphore;
    else if (!strncmp("xglOpenPeerMemory", pName, sizeof ("xglOpenPeerMemory")))
        return (XGL_VOID *) pTable->OpenPeerMemory;
    else if (!strncmp("xglOpenPeerImage", pName, sizeof ("xglOpenPeerImage")))
        return (XGL_VOID *) pTable->OpenPeerImage;
    else if (!strncmp("xglDestroyObject", pName, sizeof ("xglDestroyObject")))
        return (XGL_VOID *) pTable->DestroyObject;
    else if (!strncmp("xglGetObjectInfo", pName, sizeof ("xglGetObjectInfo")))
        return (XGL_VOID *) pTable->GetObjectInfo;
    else if (!strncmp("xglBindObjectMemory", pName, sizeof ("xglBindObjectMemory")))
        return (XGL_VOID *) pTable->BindObjectMemory;
    else if (!strncmp("xglCreateFence", pName, sizeof ("xgllCreateFence")))
        return (XGL_VOID *) pTable->CreateFence;
    else if (!strncmp("xglGetFenceStatus", pName, sizeof ("xglGetFenceStatus")))
        return (XGL_VOID *) pTable->GetFenceStatus;
    else if (!strncmp("xglWaitForFences", pName, sizeof ("xglWaitForFences")))
        return (XGL_VOID *) pTable->WaitForFences;
    else if (!strncmp("xglCreateQueueSemaphore", pName, sizeof ("xgllCreateQueueSemaphore")))
        return (XGL_VOID *) pTable->CreateQueueSemaphore;
    else if (!strncmp("xglSignalQueueSemaphore", pName, sizeof ("xglSignalQueueSemaphore")))
        return (XGL_VOID *) pTable->SignalQueueSemaphore;
    else if (!strncmp("xglWaitQueueSemaphore", pName, sizeof ("xglWaitQueueSemaphore")))
        return (XGL_VOID *) pTable->WaitQueueSemaphore;
    else if (!strncmp("xglCreateEvent", pName, sizeof ("xgllCreateEvent")))
        return (XGL_VOID *) pTable->CreateEvent;
    else if (!strncmp("xglGetEventStatus", pName, sizeof ("xglGetEventStatus")))
        return (XGL_VOID *) pTable->GetEventStatus;
    else if (!strncmp("xglSetEvent", pName, sizeof ("xglSetEvent")))
        return (XGL_VOID *) pTable->SetEvent;
    else if (!strncmp("xglResetEvent", pName, sizeof ("xgllResetEvent")))
        return (XGL_VOID *) pTable->ResetEvent;
    else if (!strncmp("xglCreateQueryPool", pName, sizeof ("xglCreateQueryPool")))
        return (XGL_VOID *) pTable->CreateQueryPool;
    else if (!strncmp("xglGetQueryPoolResults", pName, sizeof ("xglGetQueryPoolResults")))
        return (XGL_VOID *) pTable->GetQueryPoolResults;
    else if (!strncmp("xglGetFormatInfo", pName, sizeof ("xglGetFormatInfo")))
        return (XGL_VOID *) pTable->GetFormatInfo;
    else if (!strncmp("xglCreateImage", pName, sizeof ("xglCreateImage")))
        return (XGL_VOID *) pTable->CreateImage;
    else if (!strncmp("xglGetImageSubresourceInfo", pName, sizeof ("xglGetImageSubresourceInfo")))
        return (XGL_VOID *) pTable->GetImageSubresourceInfo;
    else if (!strncmp("xglCreateImageView", pName, sizeof ("xglCreateImageView")))
        return (XGL_VOID *) pTable->CreateImageView;
    else if (!strncmp("xglCreateColorAttachmentView", pName, sizeof ("xglCreateColorAttachmentView")))
        return (XGL_VOID *) pTable->CreateColorAttachmentView;
    else if (!strncmp("xglCreateDepthStencilView", pName, sizeof ("xglCreateDepthStencilView")))
        return (XGL_VOID *) pTable->CreateDepthStencilView;
    else if (!strncmp("xglCreateShader", pName, sizeof ("xglCreateShader")))
        return (XGL_VOID *) pTable->CreateShader;
    else if (!strncmp("xglCreateGraphicsPipeline", pName, sizeof ("xglCreateGraphicsPipeline")))
        return (XGL_VOID *) multi1CreateGraphicsPipeline;
    else if (!strncmp("xglCreateComputePipeline", pName, sizeof ("xglCreateComputePipeline")))
        return (XGL_VOID *) pTable->CreateComputePipeline;
    else if (!strncmp("xglStorePipeline", pName, sizeof ("xglStorePipeline")))
        return (XGL_VOID *) multi1StorePipeline;
    else if (!strncmp("xglLoadPipeline", pName, sizeof ("xglLoadPipeline")))
        return (XGL_VOID *) pTable->LoadPipeline;
    else if (!strncmp("xglCreatePipelineDelta", pName, sizeof ("xglCreatePipelineDelta")))
        return (XGL_VOID *) pTable->CreatePipelineDelta;
    else if (!strncmp("xglCreateSampler", pName, sizeof ("xglCreateSampler")))
        return (XGL_VOID *) pTable->CreateSampler;
    else if (!strncmp("xglCreateDescriptorSet", pName, sizeof ("xglCreateDescriptorSet")))
        return (XGL_VOID *) pTable->CreateDescriptorSet;
    else if (!strncmp("xglBeginDescriptorSetUpdate", pName, sizeof ("xglBeginDescriptorSetUpdate")))
        return (XGL_VOID *) pTable->BeginDescriptorSetUpdate;
    else if (!strncmp("xglEndDescriptorSetUpdate", pName, sizeof ("xglEndDescriptorSetUpdate")))
        return (XGL_VOID *) pTable->EndDescriptorSetUpdate;
    else if (!strncmp("xglAttachSamplerDescriptors", pName, sizeof ("xglAttachSamplerDescriptors")))
        return (XGL_VOID *) pTable->AttachSamplerDescriptors;
    else if (!strncmp("xglAttachImageViewDescriptors", pName, sizeof ("xglAttachImageViewDescriptors")))
        return (XGL_VOID *) pTable->AttachImageViewDescriptors;
    else if (!strncmp("xglAttachMemoryViewDescriptors", pName, sizeof ("xglAttachMemoryViewDescriptors")))
        return (XGL_VOID *) pTable->AttachMemoryViewDescriptors;
    else if (!strncmp("xglAttachNestedDescriptors", pName, sizeof ("xglAttachNestedDescriptors")))
        return (XGL_VOID *) pTable->AttachNestedDescriptors;
    else if (!strncmp("xglClearDescriptorSetSlots", pName, sizeof ("xglClearDescriptorSetSlots")))
        return (XGL_VOID *) pTable->ClearDescriptorSetSlots;
    else if (!strncmp("xglCreateViewportState", pName, sizeof ("xglCreateViewportState")))
        return (XGL_VOID *) pTable->CreateViewportState;
    else if (!strncmp("xglCreateRasterState", pName, sizeof ("xglCreateRasterState")))
        return (XGL_VOID *) pTable->CreateRasterState;
    else if (!strncmp("xglCreateMsaaState", pName, sizeof ("xglCreateMsaaState")))
        return (XGL_VOID *) pTable->CreateMsaaState;
    else if (!strncmp("xglCreateColorBlendState", pName, sizeof ("xglCreateColorBlendState")))
        return (XGL_VOID *) pTable->CreateColorBlendState;
    else if (!strncmp("xglCreateDepthStencilState", pName, sizeof ("xglCreateDepthStencilState")))
        return (XGL_VOID *) pTable->CreateDepthStencilState;
    else if (!strncmp("xglCreateCommandBuffer", pName, sizeof ("xglCreateCommandBuffer")))
        return (XGL_VOID *) pTable->CreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", pName, sizeof ("xglBeginCommandBuffer")))
        return (XGL_VOID *) pTable->BeginCommandBuffer;
    else if (!strncmp("xglEndCommandBuffer", pName, sizeof ("xglEndCommandBuffer")))
        return (XGL_VOID *) pTable->EndCommandBuffer;
    else if (!strncmp("xglResetCommandBuffer", pName, sizeof ("xglResetCommandBuffer")))
        return (XGL_VOID *) pTable->ResetCommandBuffer;
    else if (!strncmp("xglCmdBindPipeline", pName, sizeof ("xglCmdBindPipeline")))
        return (XGL_VOID *) pTable->CmdBindPipeline;
    else if (!strncmp("xglCmdBindPipelineDelta", pName, sizeof ("xglCmdBindPipelineDelta")))
        return (XGL_VOID *) pTable->CmdBindPipelineDelta;
    else if (!strncmp("xglCmdBindStateObject", pName, sizeof ("xglCmdBindStateObject")))
        return (XGL_VOID *) pTable->CmdBindStateObject;
    else if (!strncmp("xglCmdBindDescriptorSet", pName, sizeof ("xglCmdBindDescriptorSet")))
        return (XGL_VOID *) pTable->CmdBindDescriptorSet;
    else if (!strncmp("xglCmdBindDynamicMemoryView", pName, sizeof ("xglCmdBindDynamicMemoryView")))
        return (XGL_VOID *) pTable->CmdBindDynamicMemoryView;
    else if (!strncmp("xglCmdBindVertexData", pName, sizeof ("xglCmdBindVertexData")))
        return (XGL_VOID *) pTable->CmdBindVertexData;
    else if (!strncmp("xglCmdBindIndexData", pName, sizeof ("xglCmdBindIndexData")))
        return (XGL_VOID *) pTable->CmdBindIndexData;
    else if (!strncmp("xglCmdBindAttachments", pName, sizeof ("xglCmdBindAttachments")))
        return (XGL_VOID *) pTable->CmdBindAttachments;
    else if (!strncmp("xglCmdPrepareMemoryRegions", pName, sizeof ("xglCmdPrepareMemoryRegions")))
        return (XGL_VOID *) pTable->CmdPrepareMemoryRegions;
    else if (!strncmp("xglCmdPrepareImages", pName, sizeof ("xglCmdPrepareImages")))
        return (XGL_VOID *) pTable->CmdPrepareImages;
    else if (!strncmp("xglCmdDraw", pName, sizeof ("xglCmdDraw")))
        return (XGL_VOID *) pTable->CmdDraw;
    else if (!strncmp("xglCmdDrawIndexed", pName, sizeof ("xglCmdDrawIndexed")))
        return (XGL_VOID *) pTable->CmdDrawIndexed;
    else if (!strncmp("xglCmdDrawIndirect", pName, sizeof ("xglCmdDrawIndirect")))
        return (XGL_VOID *) pTable->CmdDrawIndirect;
    else if (!strncmp("xglCmdDrawIndexedIndirect", pName, sizeof ("xglCmdDrawIndexedIndirect")))
        return (XGL_VOID *) pTable->CmdDrawIndexedIndirect;
    else if (!strncmp("xglCmdDispatch", pName, sizeof ("xglCmdDispatch")))
        return (XGL_VOID *) pTable->CmdDispatch;
    else if (!strncmp("xglCmdDispatchIndirect", pName, sizeof ("xglCmdDispatchIndirect")))
        return (XGL_VOID *) pTable->CmdDispatchIndirect;
    else if (!strncmp("xglCmdCopyMemory", pName, sizeof ("xglCmdCopyMemory")))
        return (XGL_VOID *) pTable->CmdCopyMemory;
    else if (!strncmp("xglCmdCopyImage", pName, sizeof ("xglCmdCopyImage")))
        return (XGL_VOID *) pTable->CmdCopyImage;
    else if (!strncmp("xglCmdCopyMemoryToImage", pName, sizeof ("xglCmdCopyMemoryToImage")))
        return (XGL_VOID *) pTable->CmdCopyMemoryToImage;
    else if (!strncmp("xglCmdCopyImageToMemory", pName, sizeof ("xglCmdCopyImageToMemory")))
        return (XGL_VOID *) pTable->CmdCopyImageToMemory;
    else if (!strncmp("xglCmdCloneImageData", pName, sizeof ("xglCmdCloneImageData")))
        return (XGL_VOID *) pTable->CmdCloneImageData;
    else if (!strncmp("xglCmdUpdateMemory", pName, sizeof ("xglCmdUpdateMemory")))
        return (XGL_VOID *) pTable->CmdUpdateMemory;
    else if (!strncmp("xglCmdFillMemory", pName, sizeof ("xglCmdFillMemory")))
        return (XGL_VOID *) pTable->CmdFillMemory;
    else if (!strncmp("xglCmdClearColorImage", pName, sizeof ("xglCmdClearColorImage")))
        return (XGL_VOID *) pTable->CmdClearColorImage;
    else if (!strncmp("xglCmdClearColorImageRaw", pName, sizeof ("xglCmdClearColorImageRaw")))
        return (XGL_VOID *) pTable->CmdClearColorImageRaw;
    else if (!strncmp("xglCmdClearDepthStencil", pName, sizeof ("xglCmdClearDepthStencil")))
        return (XGL_VOID *) pTable->CmdClearDepthStencil;
    else if (!strncmp("xglCmdResolveImage", pName, sizeof ("xglCmdResolveImage")))
        return (XGL_VOID *) pTable->CmdResolveImage;
    else if (!strncmp("xglCmdSetEvent", pName, sizeof ("xglCmdSetEvent")))
        return (XGL_VOID *) pTable->CmdSetEvent;
    else if (!strncmp("xglCmdResetEvent", pName, sizeof ("xglCmdResetEvent")))
        return (XGL_VOID *) pTable->CmdResetEvent;
    else if (!strncmp("xglCmdMemoryAtomic", pName, sizeof ("xglCmdMemoryAtomic")))
        return (XGL_VOID *) pTable->CmdMemoryAtomic;
    else if (!strncmp("xglCmdBeginQuery", pName, sizeof ("xglCmdBeginQuery")))
        return (XGL_VOID *) pTable->CmdBeginQuery;
    else if (!strncmp("xglCmdEndQuery", pName, sizeof ("xglCmdEndQuery")))
        return (XGL_VOID *) pTable->CmdEndQuery;
    else if (!strncmp("xglCmdResetQueryPool", pName, sizeof ("xglCmdResetQueryPool")))
        return (XGL_VOID *) pTable->CmdResetQueryPool;
    else if (!strncmp("xglCmdWriteTimestamp", pName, sizeof ("xglCmdWriteTimestamp")))
        return (XGL_VOID *) pTable->CmdWriteTimestamp;
    else if (!strncmp("xglCmdInitAtomicCounters", pName, sizeof ("xglCmdInitAtomicCounters")))
        return (XGL_VOID *) pTable->CmdInitAtomicCounters;
    else if (!strncmp("xglCmdLoadAtomicCounters", pName, sizeof ("xglCmdLoadAtomicCounters")))
        return (XGL_VOID *) pTable->CmdLoadAtomicCounters;
    else if (!strncmp("xglCmdSaveAtomicCounters", pName, sizeof ("xglCmdSaveAtomicCounters")))
        return (XGL_VOID *) pTable->CmdSaveAtomicCounters;
    else if (!strncmp("xglDbgSetValidationLevel", pName, sizeof ("xglDbgSetValidationLevel")))
        return (XGL_VOID *) pTable->DbgSetValidationLevel;
    else if (!strncmp("xglDbgRegisterMsgCallback", pName, sizeof ("xglDbgRegisterMsgCallback")))
        return (XGL_VOID *) pTable->DbgRegisterMsgCallback;
    else if (!strncmp("xglDbgUnregisterMsgCallback", pName, sizeof ("xglDbgUnregisterMsgCallback")))
        return (XGL_VOID *) pTable->DbgUnregisterMsgCallback;
    else if (!strncmp("xglDbgSetMessageFilter", pName, sizeof ("xglDbgSetMessageFilter")))
        return (XGL_VOID *) pTable->DbgSetMessageFilter;
    else if (!strncmp("xglDbgSetObjectTag", pName, sizeof ("xglDbgSetObjectTag")))
        return (XGL_VOID *) pTable->DbgSetObjectTag;
    else if (!strncmp("xglDbgSetGlobalOption", pName, sizeof ("xglDbgSetGlobalOption")))
        return (XGL_VOID *) pTable->DbgSetGlobalOption;
    else if (!strncmp("xglDbgSetDeviceOption", pName, sizeof ("xglDbgSetDeviceOption")))
        return (XGL_VOID *) pTable->DbgSetDeviceOption;
    else if (!strncmp("xglCmdDbgMarkerBegin", pName, sizeof ("xglCmdDbgMarkerBegin")))
        return (XGL_VOID *) pTable->CmdDbgMarkerBegin;
    else if (!strncmp("xglCmdDbgMarkerEnd", pName, sizeof ("xglCmdDbgMarkerEnd")))
        return (XGL_VOID *) pTable->CmdDbgMarkerEnd;
    else if (!strncmp("xglWsiX11AssociateConnection", pName, sizeof("xglWsiX11AssociateConnection")))
        return (XGL_VOID *) pTable->WsiX11AssociateConnection;
    else if (!strncmp("xglWsiX11GetMSC", pName, sizeof("xglWsiX11GetMSC")))
        return (XGL_VOID *) pTable->WsiX11GetMSC;
    else if (!strncmp("xglWsiX11CreatePresentableImage", pName, sizeof("xglWsiX11CreatePresentableImage")))
        return (XGL_VOID *) pTable->WsiX11CreatePresentableImage;
    else if (!strncmp("xglWsiX11QueuePresent", pName, sizeof("xglWsiX11QueuePresent")))
        return (XGL_VOID *) pTable->WsiX11QueuePresent;
    else {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, pName);
    }

}

/******************************** Layer multi2 functions **************************/
static std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *> tableMap2;
static bool layer2_first_activated = false;

static XGL_LAYER_DISPATCH_TABLE * getLayer2Table(const XGL_BASE_LAYER_OBJECT *gpuw)
{
    XGL_LAYER_DISPATCH_TABLE *pTable;

    assert(gpuw);
    std::unordered_map<XGL_VOID *, XGL_LAYER_DISPATCH_TABLE *>::const_iterator it = tableMap2.find((XGL_VOID *) gpuw);
    if (it == tableMap2.end())
    {
        pTable =  new XGL_LAYER_DISPATCH_TABLE;
        tableMap2[(XGL_VOID *) gpuw] = pTable;
        initLayerTable(gpuw, pTable, 2);
        return pTable;
    } else
    {
        return it->second;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi2CreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo,
                                                      XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = getLayer2Table(gpuw);

    printf("At start of multi2 xglCreateDevice()\n");
    XGL_RESULT result = pTable->CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    // create a mapping for the device object into the dispatch table for layer2
    tableMap2.emplace(*pDevice, pTable);
    printf("Completed multi2 layer xglCreateDevice()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi2CreateCommandBuffer(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo,
                                                             XGL_CMD_BUFFER* pCmdBuffer)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap2[device];

    printf("At start of multi2 layer xglCreateCommandBuffer()\n");
    XGL_RESULT result = pTable->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    // create a mapping for CmdBuffer object into the dispatch table for layer 2
    tableMap2.emplace(*pCmdBuffer, pTable);
    printf("Completed multi2 layer xglCreateCommandBuffer()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi2BeginCommandBuffer( XGL_CMD_BUFFER cmdBuffer, XGL_FLAGS flags)
{
    XGL_LAYER_DISPATCH_TABLE* pTable = tableMap2[cmdBuffer];

    printf("At start of multi2 layer xglBeginCommandBuffer()\n");
    XGL_RESULT result = pTable->BeginCommandBuffer(cmdBuffer, flags);
    printf("Completed multi2 layer xglBeginCommandBuffer()\n");
    return result;

}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI multi2EnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize,
                                                         XGL_CHAR* const* pOutLayers, XGL_SIZE * pOutLayerCount,
                                                         XGL_VOID* pReserved)
{
    if (gpu == NULL)
        return xglEnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayers, pOutLayerCount, pReserved);

    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE* pTable = getLayer2Table(gpuw);

    printf("At start of multi2 layer xglEnumerateLayers()\n");
    XGL_RESULT result = pTable->EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayers, pOutLayerCount, pReserved);
    printf("Completed multi2 layer xglEnumerateLayers()\n");
    return result;
}

XGL_LAYER_EXPORT XGL_VOID * XGLAPI multi2GetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    if (gpu == NULL)
        return NULL;
    XGL_LAYER_DISPATCH_TABLE* pTable;
    pTable = getLayer2Table(gpuw);
    if (!strncmp("xglInitAndEnumerateGpus", pName, sizeof("xglInitAndEnumerateGpus")))
        return (XGL_VOID *) pTable->InitAndEnumerateGpus;
    else if (!strncmp("xglGetGpuInfo", pName, sizeof ("xglGetGpuInfo")))
        return (XGL_VOID *) pTable->GetGpuInfo;
    else if (!strncmp("xglCreateDevice", pName, sizeof ("xglCreateDevice")))
        return (XGL_VOID *) multi2CreateDevice;
    else if (!strncmp("xglDestroyDevice", pName, sizeof ("xglDestroyDevice")))
        return (XGL_VOID *) pTable->DestroyDevice;
    else if (!strncmp("xglGetExtensionSupport", pName, sizeof ("xglGetExtensionSupport")))
        return (XGL_VOID *) pTable->GetExtensionSupport;
    else if (!strncmp("xglEnumerateLayers", pName, sizeof ("xglEnumerateLayers")))
        return (XGL_VOID *) multi2EnumerateLayers;
    else if (!strncmp("xglGetDeviceQueue", pName, sizeof ("xglGetDeviceQueue")))
        return (XGL_VOID *) pTable->GetDeviceQueue;
    else if (!strncmp("xglQueueSubmit", pName, sizeof ("xglQueueSubmit")))
        return (XGL_VOID *) pTable->QueueSubmit;
    else if (!strncmp("xglQueueSetGlobalMemReferences", pName, sizeof ("xglQueueSetGlobalMemReferences")))
        return (XGL_VOID *) pTable->QueueSetGlobalMemReferences;
    else if (!strncmp("xglQueueWaitIdle", pName, sizeof ("xglQueueWaitIdle")))
        return (XGL_VOID *) pTable->QueueWaitIdle;
    else if (!strncmp("xglDeviceWaitIdle", pName, sizeof ("xglDeviceWaitIdle")))
        return (XGL_VOID *) pTable->DeviceWaitIdle;
    else if (!strncmp("xglGetMemoryHeapCount", pName, sizeof ("xglGetMemoryHeapCount")))
        return (XGL_VOID *) pTable->GetMemoryHeapCount;
    else if (!strncmp("xglGetMemoryHeapInfo", pName, sizeof ("xglGetMemoryHeapInfo")))
        return (XGL_VOID *) pTable->GetMemoryHeapInfo;
    else if (!strncmp("xglAllocMemory", pName, sizeof ("xglAllocMemory")))
        return (XGL_VOID *) pTable->AllocMemory;
    else if (!strncmp("xglFreeMemory", pName, sizeof ("xglFreeMemory")))
        return (XGL_VOID *) pTable->FreeMemory;
    else if (!strncmp("xglSetMemoryPriority", pName, sizeof ("xglSetMemoryPriority")))
        return (XGL_VOID *) pTable->SetMemoryPriority;
    else if (!strncmp("xglMapMemory", pName, sizeof ("xglMapMemory")))
        return (XGL_VOID *) pTable->MapMemory;
    else if (!strncmp("xglUnmapMemory", pName, sizeof ("xglUnmapMemory")))
        return (XGL_VOID *) pTable->UnmapMemory;
    else if (!strncmp("xglPinSystemMemory", pName, sizeof ("xglPinSystemMemory")))
        return (XGL_VOID *) pTable->PinSystemMemory;
    else if (!strncmp("xglRemapVirtualMemoryPages", pName, sizeof ("xglRemapVirtualMemoryPages")))
        return (XGL_VOID *) pTable->RemapVirtualMemoryPages;
    else if (!strncmp("xglGetMultiGpuCompatibility", pName, sizeof ("xglGetMultiGpuCompatibility")))
        return (XGL_VOID *) pTable->GetMultiGpuCompatibility;
    else if (!strncmp("xglOpenSharedMemory", pName, sizeof ("xglOpenSharedMemory")))
        return (XGL_VOID *) pTable->OpenSharedMemory;
    else if (!strncmp("xglOpenSharedQueueSemaphore", pName, sizeof ("xglOpenSharedQueueSemaphore")))
        return (XGL_VOID *) pTable->OpenSharedQueueSemaphore;
    else if (!strncmp("xglOpenPeerMemory", pName, sizeof ("xglOpenPeerMemory")))
        return (XGL_VOID *) pTable->OpenPeerMemory;
    else if (!strncmp("xglOpenPeerImage", pName, sizeof ("xglOpenPeerImage")))
        return (XGL_VOID *) pTable->OpenPeerImage;
    else if (!strncmp("xglDestroyObject", pName, sizeof ("xglDestroyObject")))
        return (XGL_VOID *) pTable->DestroyObject;
    else if (!strncmp("xglGetObjectInfo", pName, sizeof ("xglGetObjectInfo")))
        return (XGL_VOID *) pTable->GetObjectInfo;
    else if (!strncmp("xglBindObjectMemory", pName, sizeof ("xglBindObjectMemory")))
        return (XGL_VOID *) pTable->BindObjectMemory;
    else if (!strncmp("xglCreateFence", pName, sizeof ("xgllCreateFence")))
        return (XGL_VOID *) pTable->CreateFence;
    else if (!strncmp("xglGetFenceStatus", pName, sizeof ("xglGetFenceStatus")))
        return (XGL_VOID *) pTable->GetFenceStatus;
    else if (!strncmp("xglWaitForFences", pName, sizeof ("xglWaitForFences")))
        return (XGL_VOID *) pTable->WaitForFences;
    else if (!strncmp("xglCreateQueueSemaphore", pName, sizeof ("xgllCreateQueueSemaphore")))
        return (XGL_VOID *) pTable->CreateQueueSemaphore;
    else if (!strncmp("xglSignalQueueSemaphore", pName, sizeof ("xglSignalQueueSemaphore")))
        return (XGL_VOID *) pTable->SignalQueueSemaphore;
    else if (!strncmp("xglWaitQueueSemaphore", pName, sizeof ("xglWaitQueueSemaphore")))
        return (XGL_VOID *) pTable->WaitQueueSemaphore;
    else if (!strncmp("xglCreateEvent", pName, sizeof ("xgllCreateEvent")))
        return (XGL_VOID *) pTable->CreateEvent;
    else if (!strncmp("xglGetEventStatus", pName, sizeof ("xglGetEventStatus")))
        return (XGL_VOID *) pTable->GetEventStatus;
    else if (!strncmp("xglSetEvent", pName, sizeof ("xglSetEvent")))
        return (XGL_VOID *) pTable->SetEvent;
    else if (!strncmp("xglResetEvent", pName, sizeof ("xgllResetEvent")))
        return (XGL_VOID *) pTable->ResetEvent;
    else if (!strncmp("xglCreateQueryPool", pName, sizeof ("xglCreateQueryPool")))
        return (XGL_VOID *) pTable->CreateQueryPool;
    else if (!strncmp("xglGetQueryPoolResults", pName, sizeof ("xglGetQueryPoolResults")))
        return (XGL_VOID *) pTable->GetQueryPoolResults;
    else if (!strncmp("xglGetFormatInfo", pName, sizeof ("xglGetFormatInfo")))
        return (XGL_VOID *) pTable->GetFormatInfo;
    else if (!strncmp("xglCreateImage", pName, sizeof ("xglCreateImage")))
        return (XGL_VOID *) pTable->CreateImage;
    else if (!strncmp("xglGetImageSubresourceInfo", pName, sizeof ("xglGetImageSubresourceInfo")))
        return (XGL_VOID *) pTable->GetImageSubresourceInfo;
    else if (!strncmp("xglCreateImageView", pName, sizeof ("xglCreateImageView")))
        return (XGL_VOID *) pTable->CreateImageView;
    else if (!strncmp("xglCreateColorAttachmentView", pName, sizeof ("xglCreateColorAttachmentView")))
        return (XGL_VOID *) pTable->CreateColorAttachmentView;
    else if (!strncmp("xglCreateDepthStencilView", pName, sizeof ("xglCreateDepthStencilView")))
        return (XGL_VOID *) pTable->CreateDepthStencilView;
    else if (!strncmp("xglCreateShader", pName, sizeof ("xglCreateShader")))
        return (XGL_VOID *) pTable->CreateShader;
    else if (!strncmp("xglCreateGraphicsPipeline", pName, sizeof ("xglCreateGraphicsPipeline")))
        return (XGL_VOID *) pTable->CreateGraphicsPipeline;
    else if (!strncmp("xglCreateComputePipeline", pName, sizeof ("xglCreateComputePipeline")))
        return (XGL_VOID *) pTable->CreateComputePipeline;
    else if (!strncmp("xglStorePipeline", pName, sizeof ("xglStorePipeline")))
        return (XGL_VOID *) pTable->StorePipeline;
    else if (!strncmp("xglLoadPipeline", pName, sizeof ("xglLoadPipeline")))
        return (XGL_VOID *) pTable->LoadPipeline;
    else if (!strncmp("xglCreatePipelineDelta", pName, sizeof ("xglCreatePipelineDelta")))
        return (XGL_VOID *) pTable->CreatePipelineDelta;
    else if (!strncmp("xglCreateSampler", pName, sizeof ("xglCreateSampler")))
        return (XGL_VOID *) pTable->CreateSampler;
    else if (!strncmp("xglCreateDescriptorSet", pName, sizeof ("xglCreateDescriptorSet")))
        return (XGL_VOID *) pTable->CreateDescriptorSet;
    else if (!strncmp("xglBeginDescriptorSetUpdate", pName, sizeof ("xglBeginDescriptorSetUpdate")))
        return (XGL_VOID *) pTable->BeginDescriptorSetUpdate;
    else if (!strncmp("xglEndDescriptorSetUpdate", pName, sizeof ("xglEndDescriptorSetUpdate")))
        return (XGL_VOID *) pTable->EndDescriptorSetUpdate;
    else if (!strncmp("xglAttachSamplerDescriptors", pName, sizeof ("xglAttachSamplerDescriptors")))
        return (XGL_VOID *) pTable->AttachSamplerDescriptors;
    else if (!strncmp("xglAttachImageViewDescriptors", pName, sizeof ("xglAttachImageViewDescriptors")))
        return (XGL_VOID *) pTable->AttachImageViewDescriptors;
    else if (!strncmp("xglAttachMemoryViewDescriptors", pName, sizeof ("xglAttachMemoryViewDescriptors")))
        return (XGL_VOID *) pTable->AttachMemoryViewDescriptors;
    else if (!strncmp("xglAttachNestedDescriptors", pName, sizeof ("xglAttachNestedDescriptors")))
        return (XGL_VOID *) pTable->AttachNestedDescriptors;
    else if (!strncmp("xglClearDescriptorSetSlots", pName, sizeof ("xglClearDescriptorSetSlots")))
        return (XGL_VOID *) pTable->ClearDescriptorSetSlots;
    else if (!strncmp("xglCreateViewportState", pName, sizeof ("xglCreateViewportState")))
        return (XGL_VOID *) pTable->CreateViewportState;
    else if (!strncmp("xglCreateRasterState", pName, sizeof ("xglCreateRasterState")))
        return (XGL_VOID *) pTable->CreateRasterState;
    else if (!strncmp("xglCreateMsaaState", pName, sizeof ("xglCreateMsaaState")))
        return (XGL_VOID *) pTable->CreateMsaaState;
    else if (!strncmp("xglCreateColorBlendState", pName, sizeof ("xglCreateColorBlendState")))
        return (XGL_VOID *) pTable->CreateColorBlendState;
    else if (!strncmp("xglCreateDepthStencilState", pName, sizeof ("xglCreateDepthStencilState")))
        return (XGL_VOID *) pTable->CreateDepthStencilState;
    else if (!strncmp("xglCreateCommandBuffer", pName, sizeof ("xglCreateCommandBuffer")))
        return (XGL_VOID *) multi2CreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", pName, sizeof ("xglBeginCommandBuffer")))
        return (XGL_VOID *) multi2BeginCommandBuffer;
    else if (!strncmp("xglEndCommandBuffer", pName, sizeof ("xglEndCommandBuffer")))
        return (XGL_VOID *) pTable->EndCommandBuffer;
    else if (!strncmp("xglResetCommandBuffer", pName, sizeof ("xglResetCommandBuffer")))
        return (XGL_VOID *) pTable->ResetCommandBuffer;
    else if (!strncmp("xglCmdBindPipeline", pName, sizeof ("xglCmdBindPipeline")))
        return (XGL_VOID *) pTable->CmdBindPipeline;
    else if (!strncmp("xglCmdBindPipelineDelta", pName, sizeof ("xglCmdBindPipelineDelta")))
        return (XGL_VOID *) pTable->CmdBindPipelineDelta;
    else if (!strncmp("xglCmdBindStateObject", pName, sizeof ("xglCmdBindStateObject")))
        return (XGL_VOID *) pTable->CmdBindStateObject;
    else if (!strncmp("xglCmdBindDescriptorSet", pName, sizeof ("xglCmdBindDescriptorSet")))
        return (XGL_VOID *) pTable->CmdBindDescriptorSet;
    else if (!strncmp("xglCmdBindDynamicMemoryView", pName, sizeof ("xglCmdBindDynamicMemoryView")))
        return (XGL_VOID *) pTable->CmdBindDynamicMemoryView;
    else if (!strncmp("xglCmdBindVertexData", pName, sizeof ("xglCmdBindVertexData")))
        return (XGL_VOID *) pTable->CmdBindVertexData;
    else if (!strncmp("xglCmdBindIndexData", pName, sizeof ("xglCmdBindIndexData")))
        return (XGL_VOID *) pTable->CmdBindIndexData;
    else if (!strncmp("xglCmdBindAttachments", pName, sizeof ("xglCmdBindAttachments")))
        return (XGL_VOID *) pTable->CmdBindAttachments;
    else if (!strncmp("xglCmdPrepareMemoryRegions", pName, sizeof ("xglCmdPrepareMemoryRegions")))
        return (XGL_VOID *) pTable->CmdPrepareMemoryRegions;
    else if (!strncmp("xglCmdPrepareImages", pName, sizeof ("xglCmdPrepareImages")))
        return (XGL_VOID *) pTable->CmdPrepareImages;
    else if (!strncmp("xglCmdDraw", pName, sizeof ("xglCmdDraw")))
        return (XGL_VOID *) pTable->CmdDraw;
    else if (!strncmp("xglCmdDrawIndexed", pName, sizeof ("xglCmdDrawIndexed")))
        return (XGL_VOID *) pTable->CmdDrawIndexed;
    else if (!strncmp("xglCmdDrawIndirect", pName, sizeof ("xglCmdDrawIndirect")))
        return (XGL_VOID *) pTable->CmdDrawIndirect;
    else if (!strncmp("xglCmdDrawIndexedIndirect", pName, sizeof ("xglCmdDrawIndexedIndirect")))
        return (XGL_VOID *) pTable->CmdDrawIndexedIndirect;
    else if (!strncmp("xglCmdDispatch", pName, sizeof ("xglCmdDispatch")))
        return (XGL_VOID *) pTable->CmdDispatch;
    else if (!strncmp("xglCmdDispatchIndirect", pName, sizeof ("xglCmdDispatchIndirect")))
        return (XGL_VOID *) pTable->CmdDispatchIndirect;
    else if (!strncmp("xglCmdCopyMemory", pName, sizeof ("xglCmdCopyMemory")))
        return (XGL_VOID *) pTable->CmdCopyMemory;
    else if (!strncmp("xglCmdCopyImage", pName, sizeof ("xglCmdCopyImage")))
        return (XGL_VOID *) pTable->CmdCopyImage;
    else if (!strncmp("xglCmdCopyMemoryToImage", pName, sizeof ("xglCmdCopyMemoryToImage")))
        return (XGL_VOID *) pTable->CmdCopyMemoryToImage;
    else if (!strncmp("xglCmdCopyImageToMemory", pName, sizeof ("xglCmdCopyImageToMemory")))
        return (XGL_VOID *) pTable->CmdCopyImageToMemory;
    else if (!strncmp("xglCmdCloneImageData", pName, sizeof ("xglCmdCloneImageData")))
        return (XGL_VOID *) pTable->CmdCloneImageData;
    else if (!strncmp("xglCmdUpdateMemory", pName, sizeof ("xglCmdUpdateMemory")))
        return (XGL_VOID *) pTable->CmdUpdateMemory;
    else if (!strncmp("xglCmdFillMemory", pName, sizeof ("xglCmdFillMemory")))
        return (XGL_VOID *) pTable->CmdFillMemory;
    else if (!strncmp("xglCmdClearColorImage", pName, sizeof ("xglCmdClearColorImage")))
        return (XGL_VOID *) pTable->CmdClearColorImage;
    else if (!strncmp("xglCmdClearColorImageRaw", pName, sizeof ("xglCmdClearColorImageRaw")))
        return (XGL_VOID *) pTable->CmdClearColorImageRaw;
    else if (!strncmp("xglCmdClearDepthStencil", pName, sizeof ("xglCmdClearDepthStencil")))
        return (XGL_VOID *) pTable->CmdClearDepthStencil;
    else if (!strncmp("xglCmdResolveImage", pName, sizeof ("xglCmdResolveImage")))
        return (XGL_VOID *) pTable->CmdResolveImage;
    else if (!strncmp("xglCmdSetEvent", pName, sizeof ("xglCmdSetEvent")))
        return (XGL_VOID *) pTable->CmdSetEvent;
    else if (!strncmp("xglCmdResetEvent", pName, sizeof ("xglCmdResetEvent")))
        return (XGL_VOID *) pTable->CmdResetEvent;
    else if (!strncmp("xglCmdMemoryAtomic", pName, sizeof ("xglCmdMemoryAtomic")))
        return (XGL_VOID *) pTable->CmdMemoryAtomic;
    else if (!strncmp("xglCmdBeginQuery", pName, sizeof ("xglCmdBeginQuery")))
        return (XGL_VOID *) pTable->CmdBeginQuery;
    else if (!strncmp("xglCmdEndQuery", pName, sizeof ("xglCmdEndQuery")))
        return (XGL_VOID *) pTable->CmdEndQuery;
    else if (!strncmp("xglCmdResetQueryPool", pName, sizeof ("xglCmdResetQueryPool")))
        return (XGL_VOID *) pTable->CmdResetQueryPool;
    else if (!strncmp("xglCmdWriteTimestamp", pName, sizeof ("xglCmdWriteTimestamp")))
        return (XGL_VOID *) pTable->CmdWriteTimestamp;
    else if (!strncmp("xglCmdInitAtomicCounters", pName, sizeof ("xglCmdInitAtomicCounters")))
        return (XGL_VOID *) pTable->CmdInitAtomicCounters;
    else if (!strncmp("xglCmdLoadAtomicCounters", pName, sizeof ("xglCmdLoadAtomicCounters")))
        return (XGL_VOID *) pTable->CmdLoadAtomicCounters;
    else if (!strncmp("xglCmdSaveAtomicCounters", pName, sizeof ("xglCmdSaveAtomicCounters")))
        return (XGL_VOID *) pTable->CmdSaveAtomicCounters;
    else if (!strncmp("xglDbgSetValidationLevel", pName, sizeof ("xglDbgSetValidationLevel")))
        return (XGL_VOID *) pTable->DbgSetValidationLevel;
    else if (!strncmp("xglDbgRegisterMsgCallback", pName, sizeof ("xglDbgRegisterMsgCallback")))
        return (XGL_VOID *) pTable->DbgRegisterMsgCallback;
    else if (!strncmp("xglDbgUnregisterMsgCallback", pName, sizeof ("xglDbgUnregisterMsgCallback")))
        return (XGL_VOID *) pTable->DbgUnregisterMsgCallback;
    else if (!strncmp("xglDbgSetMessageFilter", pName, sizeof ("xglDbgSetMessageFilter")))
        return (XGL_VOID *) pTable->DbgSetMessageFilter;
    else if (!strncmp("xglDbgSetObjectTag", pName, sizeof ("xglDbgSetObjectTag")))
        return (XGL_VOID *) pTable->DbgSetObjectTag;
    else if (!strncmp("xglDbgSetGlobalOption", pName, sizeof ("xglDbgSetGlobalOption")))
        return (XGL_VOID *) pTable->DbgSetGlobalOption;
    else if (!strncmp("xglDbgSetDeviceOption", pName, sizeof ("xglDbgSetDeviceOption")))
        return (XGL_VOID *) pTable->DbgSetDeviceOption;
    else if (!strncmp("xglCmdDbgMarkerBegin", pName, sizeof ("xglCmdDbgMarkerBegin")))
        return (XGL_VOID *) pTable->CmdDbgMarkerBegin;
    else if (!strncmp("xglCmdDbgMarkerEnd", pName, sizeof ("xglCmdDbgMarkerEnd")))
        return (XGL_VOID *) pTable->CmdDbgMarkerEnd;
    else if (!strncmp("xglWsiX11AssociateConnection", pName, sizeof("xglWsiX11AssociateConnection")))
        return (XGL_VOID *) pTable->WsiX11AssociateConnection;
    else if (!strncmp("xglWsiX11GetMSC", pName, sizeof("xglWsiX11GetMSC")))
        return (XGL_VOID *) pTable->WsiX11GetMSC;
    else if (!strncmp("xglWsiX11CreatePresentableImage", pName, sizeof("xglWsiX11CreatePresentableImage")))
        return (XGL_VOID *) pTable->WsiX11CreatePresentableImage;
    else if (!strncmp("xglWsiX11QueuePresent", pName, sizeof("xglWsiX11QueuePresent")))
        return (XGL_VOID *) pTable->WsiX11QueuePresent;
    else {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((XGL_PHYSICAL_GPU) gpuw->nextObject, pName);
    }

}

/********************************* Common functions ********************************/
XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize,
                                                      XGL_CHAR* const* pOutLayers, XGL_SIZE * pOutLayerCount,
                                                      XGL_VOID* pReserved)
{
    if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL || pOutLayers[1] == NULL || pReserved == NULL)
        return XGL_ERROR_INVALID_POINTER;

    if (maxLayerCount < 2)
        return XGL_ERROR_INITIALIZATION_FAILED;
    *pOutLayerCount = 2;
    strncpy((char *) pOutLayers[0], "multi1", maxStringSize);
    strncpy((char *) pOutLayers[1], "multi2", maxStringSize);
    return XGL_SUCCESS;
}

XGL_LAYER_EXPORT XGL_VOID * XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pName)
{
    // to find each layers GPA routine Loader will search via "<layerName>GetProcAddr"
    if (!strncmp("multi1GetProcAddr", pName, sizeof("multi1GetProcAddr")))
        return (XGL_VOID *) multi1GetProcAddr;
    else if (!strncmp("multi2GetProcAddr", pName, sizeof("multi2GetProcAddr")))
        return (XGL_VOID *) multi2GetProcAddr;
    else if (!strncmp("xglGetProcAddr", pName, sizeof("xglGetProcAddr")))
        return (XGL_VOID *) xglGetProcAddr;

    // use first layer activated as GPA dispatch table activation happens in order
    else if (layer1_first_activated)
        return multi1GetProcAddr(gpu, pName);
    else if (layer2_first_activated)
        return multi2GetProcAddr(gpu, pName);
    else
        return NULL;

}

#ifdef __cplusplus
}    //extern "C"
#endif

static void initLayerTable(const XGL_BASE_LAYER_OBJECT *gpuw, XGL_LAYER_DISPATCH_TABLE *pTable, const unsigned int layerNum)
{
    if (layerNum == 2 && layer1_first_activated == false)
        layer2_first_activated = true;
    if (layerNum == 1 && layer2_first_activated == false)
        layer1_first_activated = true;

    layer_initialize_dispatch_table(pTable, gpuw->pGPA, (XGL_PHYSICAL_GPU) gpuw->nextObject);
}
