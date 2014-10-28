/* THIS FILE IS GENERATED.  DO NOT EDIT. */

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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include "xglLayer.h"

static XGL_LAYER_DISPATCH_TABLE nextTable;
static XGL_BASE_LAYER_OBJECT *pCurObj;
static pthread_once_t tabOnce = PTHREAD_ONCE_INIT;
static long long unsigned int object_track_index = 0;

typedef struct _objNode {
    XGL_VOID        *pObj;
    const char      *objType;
    uint64_t        numUses;
    struct _objNode *pNext;
} objNode;

static objNode *pObjLLHead = NULL;

static void ll_insert_obj(XGL_VOID* pObj, const char* type) {
    objNode* pNewObjNode = (objNode*)malloc(sizeof(objNode));
    pNewObjNode->pObj = pObj;
    pNewObjNode->objType = type;
    pNewObjNode->numUses = 0;
    pNewObjNode->pNext = pObjLLHead;
    pObjLLHead = pNewObjNode;
}

static void ll_increment_use_count(XGL_VOID* pObj) {
    objNode *pTrav = pObjLLHead;
    while (pTrav) {
        if (pTrav->pObj == pObj) {
            pTrav->numUses++;
            return;
        }
        pTrav = pTrav->pNext;
    }
    // If we do not find obj, insert it and then intrement count
    printf("OBJ WARN : Unable to increment count for obj %p, will add to list as UNKNOWN type and increment count\n", pObj);
    ll_insert_obj(pObj, "UNKNOWN");
    ll_increment_use_count(pObj);
}
static uint64_t ll_get_obj_uses(XGL_VOID* pObj) {
    objNode *pTrav = pObjLLHead;
    while (pTrav) {
        if (pTrav->pObj == pObj) {
            return pTrav->numUses;
        }
        pTrav = pTrav->pNext;
    }
    return 0;
}
static void ll_remove_obj(XGL_VOID* pObj) {
    objNode *pTrav = pObjLLHead;
    objNode *pPrev = pObjLLHead;
    while (pTrav) {
        if (pTrav->pObj == pObj) {
            pPrev->pNext = pTrav->pNext;
            if (pObjLLHead == pTrav)
                pObjLLHead = pTrav->pNext;
            printf("OBJ_STAT Removed %s obj %p that was used %lu times.\n", pTrav->objType, pTrav->pObj, pTrav->numUses);
            free(pTrav);
            return;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }
    printf("ERROR : Unable to remove obj %p\n", pObj);
}

static void initLayerTable()
{
    GetProcAddrType fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    GetProcAddrType fpGetProcAddr = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetProcAddr");
    nextTable.GetProcAddr = fpGetProcAddr;
    InitAndEnumerateGpusType fpInitAndEnumerateGpus = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglInitAndEnumerateGpus");
    nextTable.InitAndEnumerateGpus = fpInitAndEnumerateGpus;
    GetGpuInfoType fpGetGpuInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetGpuInfo");
    nextTable.GetGpuInfo = fpGetGpuInfo;
    CreateDeviceType fpCreateDevice = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateDevice");
    nextTable.CreateDevice = fpCreateDevice;
    DestroyDeviceType fpDestroyDevice = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDestroyDevice");
    nextTable.DestroyDevice = fpDestroyDevice;
    GetExtensionSupportType fpGetExtensionSupport = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetExtensionSupport");
    nextTable.GetExtensionSupport = fpGetExtensionSupport;
    EnumerateLayersType fpEnumerateLayers = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglEnumerateLayers");
    nextTable.EnumerateLayers = fpEnumerateLayers;
    GetDeviceQueueType fpGetDeviceQueue = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetDeviceQueue");
    nextTable.GetDeviceQueue = fpGetDeviceQueue;
    QueueSubmitType fpQueueSubmit = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglQueueSubmit");
    nextTable.QueueSubmit = fpQueueSubmit;
    QueueSetGlobalMemReferencesType fpQueueSetGlobalMemReferences = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglQueueSetGlobalMemReferences");
    nextTable.QueueSetGlobalMemReferences = fpQueueSetGlobalMemReferences;
    QueueWaitIdleType fpQueueWaitIdle = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglQueueWaitIdle");
    nextTable.QueueWaitIdle = fpQueueWaitIdle;
    DeviceWaitIdleType fpDeviceWaitIdle = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDeviceWaitIdle");
    nextTable.DeviceWaitIdle = fpDeviceWaitIdle;
    GetMemoryHeapCountType fpGetMemoryHeapCount = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetMemoryHeapCount");
    nextTable.GetMemoryHeapCount = fpGetMemoryHeapCount;
    GetMemoryHeapInfoType fpGetMemoryHeapInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetMemoryHeapInfo");
    nextTable.GetMemoryHeapInfo = fpGetMemoryHeapInfo;
    AllocMemoryType fpAllocMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAllocMemory");
    nextTable.AllocMemory = fpAllocMemory;
    FreeMemoryType fpFreeMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglFreeMemory");
    nextTable.FreeMemory = fpFreeMemory;
    SetMemoryPriorityType fpSetMemoryPriority = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglSetMemoryPriority");
    nextTable.SetMemoryPriority = fpSetMemoryPriority;
    MapMemoryType fpMapMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglMapMemory");
    nextTable.MapMemory = fpMapMemory;
    UnmapMemoryType fpUnmapMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglUnmapMemory");
    nextTable.UnmapMemory = fpUnmapMemory;
    PinSystemMemoryType fpPinSystemMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglPinSystemMemory");
    nextTable.PinSystemMemory = fpPinSystemMemory;
    RemapVirtualMemoryPagesType fpRemapVirtualMemoryPages = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglRemapVirtualMemoryPages");
    nextTable.RemapVirtualMemoryPages = fpRemapVirtualMemoryPages;
    GetMultiGpuCompatibilityType fpGetMultiGpuCompatibility = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetMultiGpuCompatibility");
    nextTable.GetMultiGpuCompatibility = fpGetMultiGpuCompatibility;
    OpenSharedMemoryType fpOpenSharedMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglOpenSharedMemory");
    nextTable.OpenSharedMemory = fpOpenSharedMemory;
    OpenSharedQueueSemaphoreType fpOpenSharedQueueSemaphore = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglOpenSharedQueueSemaphore");
    nextTable.OpenSharedQueueSemaphore = fpOpenSharedQueueSemaphore;
    OpenPeerMemoryType fpOpenPeerMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglOpenPeerMemory");
    nextTable.OpenPeerMemory = fpOpenPeerMemory;
    OpenPeerImageType fpOpenPeerImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglOpenPeerImage");
    nextTable.OpenPeerImage = fpOpenPeerImage;
    DestroyObjectType fpDestroyObject = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDestroyObject");
    nextTable.DestroyObject = fpDestroyObject;
    GetObjectInfoType fpGetObjectInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetObjectInfo");
    nextTable.GetObjectInfo = fpGetObjectInfo;
    BindObjectMemoryType fpBindObjectMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglBindObjectMemory");
    nextTable.BindObjectMemory = fpBindObjectMemory;
    CreateFenceType fpCreateFence = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateFence");
    nextTable.CreateFence = fpCreateFence;
    GetFenceStatusType fpGetFenceStatus = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetFenceStatus");
    nextTable.GetFenceStatus = fpGetFenceStatus;
    WaitForFencesType fpWaitForFences = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWaitForFences");
    nextTable.WaitForFences = fpWaitForFences;
    CreateQueueSemaphoreType fpCreateQueueSemaphore = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateQueueSemaphore");
    nextTable.CreateQueueSemaphore = fpCreateQueueSemaphore;
    SignalQueueSemaphoreType fpSignalQueueSemaphore = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglSignalQueueSemaphore");
    nextTable.SignalQueueSemaphore = fpSignalQueueSemaphore;
    WaitQueueSemaphoreType fpWaitQueueSemaphore = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWaitQueueSemaphore");
    nextTable.WaitQueueSemaphore = fpWaitQueueSemaphore;
    CreateEventType fpCreateEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateEvent");
    nextTable.CreateEvent = fpCreateEvent;
    GetEventStatusType fpGetEventStatus = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetEventStatus");
    nextTable.GetEventStatus = fpGetEventStatus;
    SetEventType fpSetEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglSetEvent");
    nextTable.SetEvent = fpSetEvent;
    ResetEventType fpResetEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglResetEvent");
    nextTable.ResetEvent = fpResetEvent;
    CreateQueryPoolType fpCreateQueryPool = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateQueryPool");
    nextTable.CreateQueryPool = fpCreateQueryPool;
    GetQueryPoolResultsType fpGetQueryPoolResults = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetQueryPoolResults");
    nextTable.GetQueryPoolResults = fpGetQueryPoolResults;
    GetFormatInfoType fpGetFormatInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetFormatInfo");
    nextTable.GetFormatInfo = fpGetFormatInfo;
    CreateImageType fpCreateImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateImage");
    nextTable.CreateImage = fpCreateImage;
    GetImageSubresourceInfoType fpGetImageSubresourceInfo = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglGetImageSubresourceInfo");
    nextTable.GetImageSubresourceInfo = fpGetImageSubresourceInfo;
    CreateImageViewType fpCreateImageView = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateImageView");
    nextTable.CreateImageView = fpCreateImageView;
    CreateColorAttachmentViewType fpCreateColorAttachmentView = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateColorAttachmentView");
    nextTable.CreateColorAttachmentView = fpCreateColorAttachmentView;
    CreateDepthStencilViewType fpCreateDepthStencilView = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateDepthStencilView");
    nextTable.CreateDepthStencilView = fpCreateDepthStencilView;
    CreateShaderType fpCreateShader = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateShader");
    nextTable.CreateShader = fpCreateShader;
    CreateGraphicsPipelineType fpCreateGraphicsPipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateGraphicsPipeline");
    nextTable.CreateGraphicsPipeline = fpCreateGraphicsPipeline;
    CreateComputePipelineType fpCreateComputePipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateComputePipeline");
    nextTable.CreateComputePipeline = fpCreateComputePipeline;
    StorePipelineType fpStorePipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglStorePipeline");
    nextTable.StorePipeline = fpStorePipeline;
    LoadPipelineType fpLoadPipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglLoadPipeline");
    nextTable.LoadPipeline = fpLoadPipeline;
    CreatePipelineDeltaType fpCreatePipelineDelta = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreatePipelineDelta");
    nextTable.CreatePipelineDelta = fpCreatePipelineDelta;
    CreateSamplerType fpCreateSampler = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateSampler");
    nextTable.CreateSampler = fpCreateSampler;
    CreateDescriptorSetType fpCreateDescriptorSet = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateDescriptorSet");
    nextTable.CreateDescriptorSet = fpCreateDescriptorSet;
    BeginDescriptorSetUpdateType fpBeginDescriptorSetUpdate = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglBeginDescriptorSetUpdate");
    nextTable.BeginDescriptorSetUpdate = fpBeginDescriptorSetUpdate;
    EndDescriptorSetUpdateType fpEndDescriptorSetUpdate = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglEndDescriptorSetUpdate");
    nextTable.EndDescriptorSetUpdate = fpEndDescriptorSetUpdate;
    AttachSamplerDescriptorsType fpAttachSamplerDescriptors = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAttachSamplerDescriptors");
    nextTable.AttachSamplerDescriptors = fpAttachSamplerDescriptors;
    AttachImageViewDescriptorsType fpAttachImageViewDescriptors = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAttachImageViewDescriptors");
    nextTable.AttachImageViewDescriptors = fpAttachImageViewDescriptors;
    AttachMemoryViewDescriptorsType fpAttachMemoryViewDescriptors = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAttachMemoryViewDescriptors");
    nextTable.AttachMemoryViewDescriptors = fpAttachMemoryViewDescriptors;
    AttachNestedDescriptorsType fpAttachNestedDescriptors = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglAttachNestedDescriptors");
    nextTable.AttachNestedDescriptors = fpAttachNestedDescriptors;
    ClearDescriptorSetSlotsType fpClearDescriptorSetSlots = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglClearDescriptorSetSlots");
    nextTable.ClearDescriptorSetSlots = fpClearDescriptorSetSlots;
    CreateViewportStateType fpCreateViewportState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateViewportState");
    nextTable.CreateViewportState = fpCreateViewportState;
    CreateRasterStateType fpCreateRasterState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateRasterState");
    nextTable.CreateRasterState = fpCreateRasterState;
    CreateMsaaStateType fpCreateMsaaState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateMsaaState");
    nextTable.CreateMsaaState = fpCreateMsaaState;
    CreateColorBlendStateType fpCreateColorBlendState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateColorBlendState");
    nextTable.CreateColorBlendState = fpCreateColorBlendState;
    CreateDepthStencilStateType fpCreateDepthStencilState = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateDepthStencilState");
    nextTable.CreateDepthStencilState = fpCreateDepthStencilState;
    CreateCommandBufferType fpCreateCommandBuffer = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCreateCommandBuffer");
    nextTable.CreateCommandBuffer = fpCreateCommandBuffer;
    BeginCommandBufferType fpBeginCommandBuffer = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglBeginCommandBuffer");
    nextTable.BeginCommandBuffer = fpBeginCommandBuffer;
    EndCommandBufferType fpEndCommandBuffer = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglEndCommandBuffer");
    nextTable.EndCommandBuffer = fpEndCommandBuffer;
    ResetCommandBufferType fpResetCommandBuffer = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglResetCommandBuffer");
    nextTable.ResetCommandBuffer = fpResetCommandBuffer;
    CmdBindPipelineType fpCmdBindPipeline = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindPipeline");
    nextTable.CmdBindPipeline = fpCmdBindPipeline;
    CmdBindPipelineDeltaType fpCmdBindPipelineDelta = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindPipelineDelta");
    nextTable.CmdBindPipelineDelta = fpCmdBindPipelineDelta;
    CmdBindStateObjectType fpCmdBindStateObject = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindStateObject");
    nextTable.CmdBindStateObject = fpCmdBindStateObject;
    CmdBindDescriptorSetType fpCmdBindDescriptorSet = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindDescriptorSet");
    nextTable.CmdBindDescriptorSet = fpCmdBindDescriptorSet;
    CmdBindDynamicMemoryViewType fpCmdBindDynamicMemoryView = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindDynamicMemoryView");
    nextTable.CmdBindDynamicMemoryView = fpCmdBindDynamicMemoryView;
    CmdBindIndexDataType fpCmdBindIndexData = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindIndexData");
    nextTable.CmdBindIndexData = fpCmdBindIndexData;
    CmdBindAttachmentsType fpCmdBindAttachments = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBindAttachments");
    nextTable.CmdBindAttachments = fpCmdBindAttachments;
    CmdPrepareMemoryRegionsType fpCmdPrepareMemoryRegions = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdPrepareMemoryRegions");
    nextTable.CmdPrepareMemoryRegions = fpCmdPrepareMemoryRegions;
    CmdPrepareImagesType fpCmdPrepareImages = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdPrepareImages");
    nextTable.CmdPrepareImages = fpCmdPrepareImages;
    CmdDrawType fpCmdDraw = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDraw");
    nextTable.CmdDraw = fpCmdDraw;
    CmdDrawIndexedType fpCmdDrawIndexed = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDrawIndexed");
    nextTable.CmdDrawIndexed = fpCmdDrawIndexed;
    CmdDrawIndirectType fpCmdDrawIndirect = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDrawIndirect");
    nextTable.CmdDrawIndirect = fpCmdDrawIndirect;
    CmdDrawIndexedIndirectType fpCmdDrawIndexedIndirect = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDrawIndexedIndirect");
    nextTable.CmdDrawIndexedIndirect = fpCmdDrawIndexedIndirect;
    CmdDispatchType fpCmdDispatch = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDispatch");
    nextTable.CmdDispatch = fpCmdDispatch;
    CmdDispatchIndirectType fpCmdDispatchIndirect = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDispatchIndirect");
    nextTable.CmdDispatchIndirect = fpCmdDispatchIndirect;
    CmdCopyMemoryType fpCmdCopyMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCopyMemory");
    nextTable.CmdCopyMemory = fpCmdCopyMemory;
    CmdCopyImageType fpCmdCopyImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCopyImage");
    nextTable.CmdCopyImage = fpCmdCopyImage;
    CmdCopyMemoryToImageType fpCmdCopyMemoryToImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCopyMemoryToImage");
    nextTable.CmdCopyMemoryToImage = fpCmdCopyMemoryToImage;
    CmdCopyImageToMemoryType fpCmdCopyImageToMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCopyImageToMemory");
    nextTable.CmdCopyImageToMemory = fpCmdCopyImageToMemory;
    CmdCloneImageDataType fpCmdCloneImageData = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdCloneImageData");
    nextTable.CmdCloneImageData = fpCmdCloneImageData;
    CmdUpdateMemoryType fpCmdUpdateMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdUpdateMemory");
    nextTable.CmdUpdateMemory = fpCmdUpdateMemory;
    CmdFillMemoryType fpCmdFillMemory = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdFillMemory");
    nextTable.CmdFillMemory = fpCmdFillMemory;
    CmdClearColorImageType fpCmdClearColorImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdClearColorImage");
    nextTable.CmdClearColorImage = fpCmdClearColorImage;
    CmdClearColorImageRawType fpCmdClearColorImageRaw = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdClearColorImageRaw");
    nextTable.CmdClearColorImageRaw = fpCmdClearColorImageRaw;
    CmdClearDepthStencilType fpCmdClearDepthStencil = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdClearDepthStencil");
    nextTable.CmdClearDepthStencil = fpCmdClearDepthStencil;
    CmdResolveImageType fpCmdResolveImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdResolveImage");
    nextTable.CmdResolveImage = fpCmdResolveImage;
    CmdSetEventType fpCmdSetEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdSetEvent");
    nextTable.CmdSetEvent = fpCmdSetEvent;
    CmdResetEventType fpCmdResetEvent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdResetEvent");
    nextTable.CmdResetEvent = fpCmdResetEvent;
    CmdMemoryAtomicType fpCmdMemoryAtomic = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdMemoryAtomic");
    nextTable.CmdMemoryAtomic = fpCmdMemoryAtomic;
    CmdBeginQueryType fpCmdBeginQuery = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdBeginQuery");
    nextTable.CmdBeginQuery = fpCmdBeginQuery;
    CmdEndQueryType fpCmdEndQuery = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdEndQuery");
    nextTable.CmdEndQuery = fpCmdEndQuery;
    CmdResetQueryPoolType fpCmdResetQueryPool = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdResetQueryPool");
    nextTable.CmdResetQueryPool = fpCmdResetQueryPool;
    CmdWriteTimestampType fpCmdWriteTimestamp = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdWriteTimestamp");
    nextTable.CmdWriteTimestamp = fpCmdWriteTimestamp;
    CmdInitAtomicCountersType fpCmdInitAtomicCounters = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdInitAtomicCounters");
    nextTable.CmdInitAtomicCounters = fpCmdInitAtomicCounters;
    CmdLoadAtomicCountersType fpCmdLoadAtomicCounters = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdLoadAtomicCounters");
    nextTable.CmdLoadAtomicCounters = fpCmdLoadAtomicCounters;
    CmdSaveAtomicCountersType fpCmdSaveAtomicCounters = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdSaveAtomicCounters");
    nextTable.CmdSaveAtomicCounters = fpCmdSaveAtomicCounters;
    DbgSetValidationLevelType fpDbgSetValidationLevel = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetValidationLevel");
    nextTable.DbgSetValidationLevel = fpDbgSetValidationLevel;
    DbgRegisterMsgCallbackType fpDbgRegisterMsgCallback = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgRegisterMsgCallback");
    nextTable.DbgRegisterMsgCallback = fpDbgRegisterMsgCallback;
    DbgUnregisterMsgCallbackType fpDbgUnregisterMsgCallback = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgUnregisterMsgCallback");
    nextTable.DbgUnregisterMsgCallback = fpDbgUnregisterMsgCallback;
    DbgSetMessageFilterType fpDbgSetMessageFilter = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetMessageFilter");
    nextTable.DbgSetMessageFilter = fpDbgSetMessageFilter;
    DbgSetObjectTagType fpDbgSetObjectTag = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetObjectTag");
    nextTable.DbgSetObjectTag = fpDbgSetObjectTag;
    DbgSetGlobalOptionType fpDbgSetGlobalOption = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetGlobalOption");
    nextTable.DbgSetGlobalOption = fpDbgSetGlobalOption;
    DbgSetDeviceOptionType fpDbgSetDeviceOption = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglDbgSetDeviceOption");
    nextTable.DbgSetDeviceOption = fpDbgSetDeviceOption;
    CmdDbgMarkerBeginType fpCmdDbgMarkerBegin = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDbgMarkerBegin");
    nextTable.CmdDbgMarkerBegin = fpCmdDbgMarkerBegin;
    CmdDbgMarkerEndType fpCmdDbgMarkerEnd = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglCmdDbgMarkerEnd");
    nextTable.CmdDbgMarkerEnd = fpCmdDbgMarkerEnd;
    WsiX11AssociateConnectionType fpWsiX11AssociateConnection = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWsiX11AssociateConnection");
    nextTable.WsiX11AssociateConnection = fpWsiX11AssociateConnection;
    WsiX11GetMSCType fpWsiX11GetMSC = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWsiX11GetMSC");
    nextTable.WsiX11GetMSC = fpWsiX11GetMSC;
    WsiX11CreatePresentableImageType fpWsiX11CreatePresentableImage = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWsiX11CreatePresentableImage");
    nextTable.WsiX11CreatePresentableImage = fpWsiX11CreatePresentableImage;
    WsiX11QueuePresentType fpWsiX11QueuePresent = fpNextGPA((XGL_PHYSICAL_GPU) pCurObj->nextObject, (XGL_CHAR *) "xglWsiX11QueuePresent");
    nextTable.WsiX11QueuePresent = fpWsiX11QueuePresent;
}


XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetGpuInfo(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    ll_increment_use_count((XGL_VOID*)gpu);
    printf("OBJ[%llu] : USING gpu object %p (%lu total uses)\n", object_track_index++, (void*)gpu, ll_get_obj_uses((XGL_VOID*)gpu));
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable);
    XGL_RESULT result = nextTable.GetGpuInfo((XGL_PHYSICAL_GPU)gpuw->nextObject, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    ll_increment_use_count((XGL_VOID*)gpu);
    printf("OBJ[%llu] : USING gpu object %p (%lu total uses)\n", object_track_index++, (void*)gpu, ll_get_obj_uses((XGL_VOID*)gpu));
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable);
    XGL_RESULT result = nextTable.CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    printf("OBJ[%llu] : CREATE XGL_DEVICE object %p\n", object_track_index++, (void*)*pDevice);
    ll_insert_obj((XGL_VOID*)*pDevice, "XGL_DEVICE");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyDevice(XGL_DEVICE device)
{
    XGL_RESULT result = nextTable.DestroyDevice(device);
    printf("OBJ[%llu] : DESTROY device object %p\n", object_track_index++, (void*)device);
    ll_remove_obj((XGL_VOID*)device);
    // Report any remaining objects in LL
    objNode *pTrav = pObjLLHead;
    while (pTrav) {
        printf("OBJ ERROR : %s object %p has not been destroyed (was used %lu times).\n", pTrav->objType, pTrav->pObj, pTrav->numUses);
        pTrav = pTrav->pNext;
    }
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* pExtName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    ll_increment_use_count((XGL_VOID*)gpu);
    printf("OBJ[%llu] : USING gpu object %p (%lu total uses)\n", object_track_index++, (void*)gpu, ll_get_obj_uses((XGL_VOID*)gpu));
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable);
    XGL_RESULT result = nextTable.GetExtensionSupport((XGL_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize, XGL_CHAR* const* pOutLayers, XGL_SIZE * pOutLayerCount)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    ll_increment_use_count((XGL_VOID*)gpu);
    printf("OBJ[%llu] : USING gpu object %p (%lu total uses)\n", object_track_index++, (void*)gpu, ll_get_obj_uses((XGL_VOID*)gpu));
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable);
    XGL_RESULT result = nextTable.EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayers, pOutLayerCount);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetDeviceQueue(XGL_DEVICE device, XGL_QUEUE_TYPE queueType, XGL_UINT queueIndex, XGL_QUEUE* pQueue)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.GetDeviceQueue(device, queueType, queueIndex, pQueue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(XGL_QUEUE queue, XGL_UINT cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers, XGL_UINT memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence)
{
    ll_increment_use_count((XGL_VOID*)queue);
    printf("OBJ[%llu] : USING queue object %p (%lu total uses)\n", object_track_index++, (void*)queue, ll_get_obj_uses((XGL_VOID*)queue));
    XGL_RESULT result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, memRefCount, pMemRefs, fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(XGL_QUEUE queue, XGL_UINT memRefCount, const XGL_MEMORY_REF* pMemRefs)
{
    ll_increment_use_count((XGL_VOID*)queue);
    printf("OBJ[%llu] : USING queue object %p (%lu total uses)\n", object_track_index++, (void*)queue, ll_get_obj_uses((XGL_VOID*)queue));
    XGL_RESULT result = nextTable.QueueSetGlobalMemReferences(queue, memRefCount, pMemRefs);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueWaitIdle(XGL_QUEUE queue)
{
    ll_increment_use_count((XGL_VOID*)queue);
    printf("OBJ[%llu] : USING queue object %p (%lu total uses)\n", object_track_index++, (void*)queue, ll_get_obj_uses((XGL_VOID*)queue));
    XGL_RESULT result = nextTable.QueueWaitIdle(queue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDeviceWaitIdle(XGL_DEVICE device)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.DeviceWaitIdle(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMemoryHeapCount(XGL_DEVICE device, XGL_UINT* pCount)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.GetMemoryHeapCount(device, pCount);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMemoryHeapInfo(XGL_DEVICE device, XGL_UINT heapId, XGL_MEMORY_HEAP_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.GetMemoryHeapInfo(device, heapId, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocMemory(XGL_DEVICE device, const XGL_MEMORY_ALLOC_INFO* pAllocInfo, XGL_GPU_MEMORY* pMem)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.AllocMemory(device, pAllocInfo, pMem);
    printf("OBJ[%llu] : CREATE XGL_GPU_MEMORY object %p\n", object_track_index++, (void*)*pMem);
    ll_insert_obj((XGL_VOID*)*pMem, "XGL_GPU_MEMORY");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglFreeMemory(XGL_GPU_MEMORY mem)
{
    XGL_RESULT result = nextTable.FreeMemory(mem);
    printf("OBJ[%llu] : DESTROY mem object %p\n", object_track_index++, (void*)mem);
    ll_remove_obj((XGL_VOID*)mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetMemoryPriority(XGL_GPU_MEMORY mem, XGL_MEMORY_PRIORITY priority)
{
    ll_increment_use_count((XGL_VOID*)mem);
    printf("OBJ[%llu] : USING mem object %p (%lu total uses)\n", object_track_index++, (void*)mem, ll_get_obj_uses((XGL_VOID*)mem));
    XGL_RESULT result = nextTable.SetMemoryPriority(mem, priority);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglMapMemory(XGL_GPU_MEMORY mem, XGL_FLAGS flags, XGL_VOID** ppData)
{
    ll_increment_use_count((XGL_VOID*)mem);
    printf("OBJ[%llu] : USING mem object %p (%lu total uses)\n", object_track_index++, (void*)mem, ll_get_obj_uses((XGL_VOID*)mem));
    XGL_RESULT result = nextTable.MapMemory(mem, flags, ppData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglUnmapMemory(XGL_GPU_MEMORY mem)
{
    ll_increment_use_count((XGL_VOID*)mem);
    printf("OBJ[%llu] : USING mem object %p (%lu total uses)\n", object_track_index++, (void*)mem, ll_get_obj_uses((XGL_VOID*)mem));
    XGL_RESULT result = nextTable.UnmapMemory(mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglPinSystemMemory(XGL_DEVICE device, const XGL_VOID* pSysMem, XGL_SIZE memSize, XGL_GPU_MEMORY* pMem)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglRemapVirtualMemoryPages(XGL_DEVICE device, XGL_UINT rangeCount, const XGL_VIRTUAL_MEMORY_REMAP_RANGE* pRanges, XGL_UINT preWaitSemaphoreCount, const XGL_QUEUE_SEMAPHORE* pPreWaitSemaphores, XGL_UINT postSignalSemaphoreCount, const XGL_QUEUE_SEMAPHORE* pPostSignalSemaphores)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.RemapVirtualMemoryPages(device, rangeCount, pRanges, preWaitSemaphoreCount, pPreWaitSemaphores, postSignalSemaphoreCount, pPostSignalSemaphores);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(XGL_PHYSICAL_GPU gpu0, XGL_PHYSICAL_GPU gpu1, XGL_GPU_COMPATIBILITY_INFO* pInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu0;
    ll_increment_use_count((XGL_VOID*)gpu0);
    printf("OBJ[%llu] : USING gpu0 object %p (%lu total uses)\n", object_track_index++, (void*)gpu0, ll_get_obj_uses((XGL_VOID*)gpu0));
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable);
    XGL_RESULT result = nextTable.GetMultiGpuCompatibility((XGL_PHYSICAL_GPU)gpuw->nextObject, gpu1, pInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedMemory(XGL_DEVICE device, const XGL_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedQueueSemaphore(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo, XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.OpenSharedQueueSemaphore(device, pOpenInfo, pSemaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerMemory(XGL_DEVICE device, const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerImage(XGL_DEVICE device, const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyObject(XGL_OBJECT object)
{
    XGL_RESULT result = nextTable.DestroyObject(object);
    printf("OBJ[%llu] : DESTROY object object %p\n", object_track_index++, (void*)object);
    ll_remove_obj((XGL_VOID*)object);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetObjectInfo(XGL_BASE_OBJECT object, XGL_OBJECT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    ll_increment_use_count((XGL_VOID*)object);
    printf("OBJ[%llu] : USING object object %p (%lu total uses)\n", object_track_index++, (void*)object, ll_get_obj_uses((XGL_VOID*)object));
    XGL_RESULT result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemory(XGL_OBJECT object, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    ll_increment_use_count((XGL_VOID*)object);
    printf("OBJ[%llu] : USING object object %p (%lu total uses)\n", object_track_index++, (void*)object, ll_get_obj_uses((XGL_VOID*)object));
    XGL_RESULT result = nextTable.BindObjectMemory(object, mem, offset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFence(XGL_DEVICE device, const XGL_FENCE_CREATE_INFO* pCreateInfo, XGL_FENCE* pFence)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateFence(device, pCreateInfo, pFence);
    printf("OBJ[%llu] : CREATE XGL_FENCE object %p\n", object_track_index++, (void*)*pFence);
    ll_insert_obj((XGL_VOID*)*pFence, "XGL_FENCE");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFenceStatus(XGL_FENCE fence)
{
    ll_increment_use_count((XGL_VOID*)fence);
    printf("OBJ[%llu] : USING fence object %p (%lu total uses)\n", object_track_index++, (void*)fence, ll_get_obj_uses((XGL_VOID*)fence));
    XGL_RESULT result = nextTable.GetFenceStatus(fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitForFences(XGL_DEVICE device, XGL_UINT fenceCount, const XGL_FENCE* pFences, XGL_BOOL waitAll, XGL_UINT64 timeout)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueueSemaphore(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo, XGL_QUEUE_SEMAPHORE* pSemaphore)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateQueueSemaphore(device, pCreateInfo, pSemaphore);
    printf("OBJ[%llu] : CREATE XGL_QUEUE_SEMAPHORE object %p\n", object_track_index++, (void*)*pSemaphore);
    ll_insert_obj((XGL_VOID*)*pSemaphore, "XGL_QUEUE_SEMAPHORE");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSignalQueueSemaphore(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore)
{
    ll_increment_use_count((XGL_VOID*)queue);
    printf("OBJ[%llu] : USING queue object %p (%lu total uses)\n", object_track_index++, (void*)queue, ll_get_obj_uses((XGL_VOID*)queue));
    XGL_RESULT result = nextTable.SignalQueueSemaphore(queue, semaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitQueueSemaphore(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore)
{
    ll_increment_use_count((XGL_VOID*)queue);
    printf("OBJ[%llu] : USING queue object %p (%lu total uses)\n", object_track_index++, (void*)queue, ll_get_obj_uses((XGL_VOID*)queue));
    XGL_RESULT result = nextTable.WaitQueueSemaphore(queue, semaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateEvent(XGL_DEVICE device, const XGL_EVENT_CREATE_INFO* pCreateInfo, XGL_EVENT* pEvent)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    printf("OBJ[%llu] : CREATE XGL_EVENT object %p\n", object_track_index++, (void*)*pEvent);
    ll_insert_obj((XGL_VOID*)*pEvent, "XGL_EVENT");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetEventStatus(XGL_EVENT event)
{
    ll_increment_use_count((XGL_VOID*)event);
    printf("OBJ[%llu] : USING event object %p (%lu total uses)\n", object_track_index++, (void*)event, ll_get_obj_uses((XGL_VOID*)event));
    XGL_RESULT result = nextTable.GetEventStatus(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetEvent(XGL_EVENT event)
{
    ll_increment_use_count((XGL_VOID*)event);
    printf("OBJ[%llu] : USING event object %p (%lu total uses)\n", object_track_index++, (void*)event, ll_get_obj_uses((XGL_VOID*)event));
    XGL_RESULT result = nextTable.SetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetEvent(XGL_EVENT event)
{
    ll_increment_use_count((XGL_VOID*)event);
    printf("OBJ[%llu] : USING event object %p (%lu total uses)\n", object_track_index++, (void*)event, ll_get_obj_uses((XGL_VOID*)event));
    XGL_RESULT result = nextTable.ResetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueryPool(XGL_DEVICE device, const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo, XGL_QUERY_POOL* pQueryPool)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    printf("OBJ[%llu] : CREATE XGL_QUERY_POOL object %p\n", object_track_index++, (void*)*pQueryPool);
    ll_insert_obj((XGL_VOID*)*pQueryPool, "XGL_QUERY_POOL");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetQueryPoolResults(XGL_QUERY_POOL queryPool, XGL_UINT startQuery, XGL_UINT queryCount, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    ll_increment_use_count((XGL_VOID*)queryPool);
    printf("OBJ[%llu] : USING queryPool object %p (%lu total uses)\n", object_track_index++, (void*)queryPool, ll_get_obj_uses((XGL_VOID*)queryPool));
    XGL_RESULT result = nextTable.GetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImage(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateImage(device, pCreateInfo, pImage);
    printf("OBJ[%llu] : CREATE XGL_IMAGE object %p\n", object_track_index++, (void*)*pImage);
    ll_insert_obj((XGL_VOID*)*pImage, "XGL_IMAGE");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetImageSubresourceInfo(XGL_IMAGE image, const XGL_IMAGE_SUBRESOURCE* pSubresource, XGL_SUBRESOURCE_INFO_TYPE infoType, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    ll_increment_use_count((XGL_VOID*)image);
    printf("OBJ[%llu] : USING image object %p (%lu total uses)\n", object_track_index++, (void*)image, ll_get_obj_uses((XGL_VOID*)image));
    XGL_RESULT result = nextTable.GetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImageView(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
    printf("OBJ[%llu] : CREATE XGL_IMAGE_VIEW object %p\n", object_track_index++, (void*)*pView);
    ll_insert_obj((XGL_VOID*)*pView, "XGL_IMAGE_VIEW");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorAttachmentView(XGL_DEVICE device, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, XGL_COLOR_ATTACHMENT_VIEW* pView)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    printf("OBJ[%llu] : CREATE XGL_COLOR_ATTACHMENT_VIEW object %p\n", object_track_index++, (void*)*pView);
    ll_insert_obj((XGL_VOID*)*pView, "XGL_COLOR_ATTACHMENT_VIEW");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilView(XGL_DEVICE device, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_VIEW* pView)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    printf("OBJ[%llu] : CREATE XGL_DEPTH_STENCIL_VIEW object %p\n", object_track_index++, (void*)*pView);
    ll_insert_obj((XGL_VOID*)*pView, "XGL_DEPTH_STENCIL_VIEW");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateShader(XGL_DEVICE device, const XGL_SHADER_CREATE_INFO* pCreateInfo, XGL_SHADER* pShader)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateShader(device, pCreateInfo, pShader);
    printf("OBJ[%llu] : CREATE XGL_SHADER object %p\n", object_track_index++, (void*)*pShader);
    ll_insert_obj((XGL_VOID*)*pShader, "XGL_SHADER");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateGraphicsPipeline(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    printf("OBJ[%llu] : CREATE XGL_PIPELINE object %p\n", object_track_index++, (void*)*pPipeline);
    ll_insert_obj((XGL_VOID*)*pPipeline, "XGL_PIPELINE");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateComputePipeline(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    printf("OBJ[%llu] : CREATE XGL_PIPELINE object %p\n", object_track_index++, (void*)*pPipeline);
    ll_insert_obj((XGL_VOID*)*pPipeline, "XGL_PIPELINE");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglStorePipeline(XGL_PIPELINE pipeline, XGL_SIZE* pDataSize, XGL_VOID* pData)
{
    ll_increment_use_count((XGL_VOID*)pipeline);
    printf("OBJ[%llu] : USING pipeline object %p (%lu total uses)\n", object_track_index++, (void*)pipeline, ll_get_obj_uses((XGL_VOID*)pipeline));
    XGL_RESULT result = nextTable.StorePipeline(pipeline, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLoadPipeline(XGL_DEVICE device, XGL_SIZE dataSize, const XGL_VOID* pData, XGL_PIPELINE* pPipeline)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.LoadPipeline(device, dataSize, pData, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreatePipelineDelta(XGL_DEVICE device, XGL_PIPELINE p1, XGL_PIPELINE p2, XGL_PIPELINE_DELTA* delta)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreatePipelineDelta(device, p1, p2, delta);
    printf("OBJ[%llu] : CREATE XGL_PIPELINE_DELTA object %p\n", object_track_index++, (void*)*delta);
    ll_insert_obj((XGL_VOID*)*delta, "XGL_PIPELINE_DELTA");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateSampler(XGL_DEVICE device, const XGL_SAMPLER_CREATE_INFO* pCreateInfo, XGL_SAMPLER* pSampler)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    printf("OBJ[%llu] : CREATE XGL_SAMPLER object %p\n", object_track_index++, (void*)*pSampler);
    ll_insert_obj((XGL_VOID*)*pSampler, "XGL_SAMPLER");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSet(XGL_DEVICE device, const XGL_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_SET* pDescriptorSet)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateDescriptorSet(device, pCreateInfo, pDescriptorSet);
    printf("OBJ[%llu] : CREATE XGL_DESCRIPTOR_SET object %p\n", object_track_index++, (void*)*pDescriptorSet);
    ll_insert_obj((XGL_VOID*)*pDescriptorSet, "XGL_DESCRIPTOR_SET");
    return result;
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglBeginDescriptorSetUpdate(XGL_DESCRIPTOR_SET descriptorSet)
{
    ll_increment_use_count((XGL_VOID*)descriptorSet);
    printf("OBJ[%llu] : USING descriptorSet object %p (%lu total uses)\n", object_track_index++, (void*)descriptorSet, ll_get_obj_uses((XGL_VOID*)descriptorSet));
    nextTable.BeginDescriptorSetUpdate(descriptorSet);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglEndDescriptorSetUpdate(XGL_DESCRIPTOR_SET descriptorSet)
{
    ll_increment_use_count((XGL_VOID*)descriptorSet);
    printf("OBJ[%llu] : USING descriptorSet object %p (%lu total uses)\n", object_track_index++, (void*)descriptorSet, ll_get_obj_uses((XGL_VOID*)descriptorSet));
    nextTable.EndDescriptorSetUpdate(descriptorSet);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachSamplerDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_SAMPLER* pSamplers)
{
    ll_increment_use_count((XGL_VOID*)descriptorSet);
    printf("OBJ[%llu] : USING descriptorSet object %p (%lu total uses)\n", object_track_index++, (void*)descriptorSet, ll_get_obj_uses((XGL_VOID*)descriptorSet));
    nextTable.AttachSamplerDescriptors(descriptorSet, startSlot, slotCount, pSamplers);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachImageViewDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews)
{
    ll_increment_use_count((XGL_VOID*)descriptorSet);
    printf("OBJ[%llu] : USING descriptorSet object %p (%lu total uses)\n", object_track_index++, (void*)descriptorSet, ll_get_obj_uses((XGL_VOID*)descriptorSet));
    nextTable.AttachImageViewDescriptors(descriptorSet, startSlot, slotCount, pImageViews);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachMemoryViewDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews)
{
    ll_increment_use_count((XGL_VOID*)descriptorSet);
    printf("OBJ[%llu] : USING descriptorSet object %p (%lu total uses)\n", object_track_index++, (void*)descriptorSet, ll_get_obj_uses((XGL_VOID*)descriptorSet));
    nextTable.AttachMemoryViewDescriptors(descriptorSet, startSlot, slotCount, pMemViews);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglAttachNestedDescriptors(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount, const XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets)
{
    ll_increment_use_count((XGL_VOID*)descriptorSet);
    printf("OBJ[%llu] : USING descriptorSet object %p (%lu total uses)\n", object_track_index++, (void*)descriptorSet, ll_get_obj_uses((XGL_VOID*)descriptorSet));
    nextTable.AttachNestedDescriptors(descriptorSet, startSlot, slotCount, pNestedDescriptorSets);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglClearDescriptorSetSlots(XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT startSlot, XGL_UINT slotCount)
{
    ll_increment_use_count((XGL_VOID*)descriptorSet);
    printf("OBJ[%llu] : USING descriptorSet object %p (%lu total uses)\n", object_track_index++, (void*)descriptorSet, ll_get_obj_uses((XGL_VOID*)descriptorSet));
    nextTable.ClearDescriptorSetSlots(descriptorSet, startSlot, slotCount);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateViewportState(XGL_DEVICE device, const XGL_VIEWPORT_STATE_CREATE_INFO* pCreateInfo, XGL_VIEWPORT_STATE_OBJECT* pState)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateViewportState(device, pCreateInfo, pState);
    printf("OBJ[%llu] : CREATE XGL_VIEWPORT_STATE_OBJECT object %p\n", object_track_index++, (void*)*pState);
    ll_insert_obj((XGL_VOID*)*pState, "XGL_VIEWPORT_STATE_OBJECT");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateRasterState(XGL_DEVICE device, const XGL_RASTER_STATE_CREATE_INFO* pCreateInfo, XGL_RASTER_STATE_OBJECT* pState)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateRasterState(device, pCreateInfo, pState);
    printf("OBJ[%llu] : CREATE XGL_RASTER_STATE_OBJECT object %p\n", object_track_index++, (void*)*pState);
    ll_insert_obj((XGL_VOID*)*pState, "XGL_RASTER_STATE_OBJECT");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateMsaaState(XGL_DEVICE device, const XGL_MSAA_STATE_CREATE_INFO* pCreateInfo, XGL_MSAA_STATE_OBJECT* pState)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateMsaaState(device, pCreateInfo, pState);
    printf("OBJ[%llu] : CREATE XGL_MSAA_STATE_OBJECT object %p\n", object_track_index++, (void*)*pState);
    ll_insert_obj((XGL_VOID*)*pState, "XGL_MSAA_STATE_OBJECT");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorBlendState(XGL_DEVICE device, const XGL_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo, XGL_COLOR_BLEND_STATE_OBJECT* pState)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateColorBlendState(device, pCreateInfo, pState);
    printf("OBJ[%llu] : CREATE XGL_COLOR_BLEND_STATE_OBJECT object %p\n", object_track_index++, (void*)*pState);
    ll_insert_obj((XGL_VOID*)*pState, "XGL_COLOR_BLEND_STATE_OBJECT");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilState(XGL_DEVICE device, const XGL_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_STATE_OBJECT* pState)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateDepthStencilState(device, pCreateInfo, pState);
    printf("OBJ[%llu] : CREATE XGL_DEPTH_STENCIL_STATE_OBJECT object %p\n", object_track_index++, (void*)*pState);
    ll_insert_obj((XGL_VOID*)*pState, "XGL_DEPTH_STENCIL_STATE_OBJECT");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateCommandBuffer(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, XGL_CMD_BUFFER* pCmdBuffer)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    printf("OBJ[%llu] : CREATE XGL_CMD_BUFFER object %p\n", object_track_index++, (void*)*pCmdBuffer);
    ll_insert_obj((XGL_VOID*)*pCmdBuffer, "XGL_CMD_BUFFER");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginCommandBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_FLAGS flags)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    XGL_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, flags);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    XGL_RESULT result = nextTable.EndCommandBuffer(cmdBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    XGL_RESULT result = nextTable.ResetCommandBuffer(cmdBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindPipeline(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE pipeline)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindPipelineDelta(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE_DELTA delta)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdBindPipelineDelta(cmdBuffer, pipelineBindPoint, delta);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindStateObject(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_STATE_OBJECT state)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdBindStateObject(cmdBuffer, stateBindPoint, state);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindDescriptorSet(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT index, XGL_DESCRIPTOR_SET descriptorSet, XGL_UINT slotOffset)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdBindDescriptorSet(cmdBuffer, pipelineBindPoint, index, descriptorSet, slotOffset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindDynamicMemoryView(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, const XGL_MEMORY_VIEW_ATTACH_INFO* pMemView)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdBindDynamicMemoryView(cmdBuffer, pipelineBindPoint, pMemView);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindIndexData(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdBindIndexData(cmdBuffer, mem, offset, indexType);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBindAttachments(XGL_CMD_BUFFER cmdBuffer, XGL_UINT colorAttachmentCount, const XGL_COLOR_ATTACHMENT_BIND_INFO* pColorAttachments, const XGL_DEPTH_STENCIL_BIND_INFO* pDepthStencilAttachment)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdBindAttachments(cmdBuffer, colorAttachmentCount, pColorAttachments, pDepthStencilAttachment);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdPrepareMemoryRegions(XGL_CMD_BUFFER cmdBuffer, XGL_UINT transitionCount, const XGL_MEMORY_STATE_TRANSITION* pStateTransitions)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdPrepareMemoryRegions(cmdBuffer, transitionCount, pStateTransitions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdPrepareImages(XGL_CMD_BUFFER cmdBuffer, XGL_UINT transitionCount, const XGL_IMAGE_STATE_TRANSITION* pStateTransitions)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdPrepareImages(cmdBuffer, transitionCount, pStateTransitions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDraw(XGL_CMD_BUFFER cmdBuffer, XGL_UINT firstVertex, XGL_UINT vertexCount, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDrawIndexed(XGL_CMD_BUFFER cmdBuffer, XGL_UINT firstIndex, XGL_UINT indexCount, XGL_INT vertexOffset, XGL_UINT firstInstance, XGL_UINT instanceCount)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDrawIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_UINT32 count, XGL_UINT32 stride)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdDrawIndirect(cmdBuffer, mem, offset, count, stride);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDrawIndexedIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset, XGL_UINT32 count, XGL_UINT32 stride)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdDrawIndexedIndirect(cmdBuffer, mem, offset, count, stride);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDispatch(XGL_CMD_BUFFER cmdBuffer, XGL_UINT x, XGL_UINT y, XGL_UINT z)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDispatchIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdDispatchIndirect(cmdBuffer, mem, offset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyMemory(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY srcMem, XGL_GPU_MEMORY destMem, XGL_UINT regionCount, const XGL_MEMORY_COPY* pRegions)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdCopyMemory(cmdBuffer, srcMem, destMem, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, XGL_UINT regionCount, const XGL_IMAGE_COPY* pRegions)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdCopyImage(cmdBuffer, srcImage, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyMemoryToImage(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY srcMem, XGL_IMAGE destImage, XGL_UINT regionCount, const XGL_MEMORY_IMAGE_COPY* pRegions)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdCopyMemoryToImage(cmdBuffer, srcMem, destImage, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCopyImageToMemory(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_GPU_MEMORY destMem, XGL_UINT regionCount, const XGL_MEMORY_IMAGE_COPY* pRegions)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdCopyImageToMemory(cmdBuffer, srcImage, destMem, regionCount, pRegions);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdCloneImageData(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_STATE srcImageState, XGL_IMAGE destImage, XGL_IMAGE_STATE destImageState)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageState, destImage, destImageState);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdUpdateMemory(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const XGL_UINT32* pData)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdUpdateMemory(cmdBuffer, destMem, destOffset, dataSize, pData);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdFillMemory(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, XGL_UINT32 data)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdFillMemory(cmdBuffer, destMem, destOffset, fillSize, data);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdClearColorImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const XGL_FLOAT color[4], XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdClearColorImage(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdClearColorImageRaw(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, const XGL_UINT32 color[4], XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdClearColorImageRaw(cmdBuffer, image, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdClearDepthStencil(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, XGL_FLOAT depth, XGL_UINT32 stencil, XGL_UINT rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdClearDepthStencil(cmdBuffer, image, depth, stencil, rangeCount, pRanges);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdResolveImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, XGL_UINT rectCount, const XGL_IMAGE_RESOLVE* pRects)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdResolveImage(cmdBuffer, srcImage, destImage, rectCount, pRects);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdSetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdSetEvent(cmdBuffer, event);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdResetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdResetEvent(cmdBuffer, event);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdMemoryAtomic(XGL_CMD_BUFFER cmdBuffer, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset, XGL_UINT64 srcData, XGL_ATOMIC_OP atomicOp)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdMemoryAtomic(cmdBuffer, destMem, destOffset, srcData, atomicOp);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdBeginQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT slot, XGL_FLAGS flags)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdEndQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT slot)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdResetQueryPool(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, XGL_UINT startQuery, XGL_UINT queryCount)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdWriteTimestamp(XGL_CMD_BUFFER cmdBuffer, XGL_TIMESTAMP_TYPE timestampType, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destMem, destOffset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdInitAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, const XGL_UINT32* pData)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdLoadAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, XGL_GPU_MEMORY srcMem, XGL_GPU_SIZE srcOffset)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcMem, srcOffset);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdSaveAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_UINT startCounter, XGL_UINT counterCount, XGL_GPU_MEMORY destMem, XGL_GPU_SIZE destOffset)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destMem, destOffset);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetValidationLevel(XGL_DEVICE device, XGL_VALIDATION_LEVEL validationLevel)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.DbgSetValidationLevel(device, validationLevel);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData)
{
    ll_increment_use_count((XGL_VOID*)pfnMsgCallback);
    printf("OBJ[%llu] : USING pfnMsgCallback object %p (%lu total uses)\n", object_track_index++, (void*)pfnMsgCallback, ll_get_obj_uses((XGL_VOID*)pfnMsgCallback));
    XGL_RESULT result = nextTable.DbgRegisterMsgCallback(pfnMsgCallback, pUserData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    ll_increment_use_count((XGL_VOID*)pfnMsgCallback);
    printf("OBJ[%llu] : USING pfnMsgCallback object %p (%lu total uses)\n", object_track_index++, (void*)pfnMsgCallback, ll_get_obj_uses((XGL_VOID*)pfnMsgCallback));
    XGL_RESULT result = nextTable.DbgUnregisterMsgCallback(pfnMsgCallback);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetMessageFilter(XGL_DEVICE device, XGL_INT msgCode, XGL_DBG_MSG_FILTER filter)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetObjectTag(XGL_BASE_OBJECT object, XGL_SIZE tagSize, const XGL_VOID* pTag)
{
    ll_increment_use_count((XGL_VOID*)object);
    printf("OBJ[%llu] : USING object object %p (%lu total uses)\n", object_track_index++, (void*)object, ll_get_obj_uses((XGL_VOID*)object));
    XGL_RESULT result = nextTable.DbgSetObjectTag(object, tagSize, pTag);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData)
{
    ll_increment_use_count((XGL_VOID*)dbgOption);
    printf("OBJ[%llu] : USING dbgOption object %p (%lu total uses)\n", object_track_index++, (void*)dbgOption, ll_get_obj_uses((XGL_VOID*)dbgOption));
    XGL_RESULT result = nextTable.DbgSetGlobalOption(dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetDeviceOption(XGL_DEVICE device, XGL_DBG_DEVICE_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDbgMarkerBegin(XGL_CMD_BUFFER cmdBuffer, const XGL_CHAR* pMarker)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

XGL_LAYER_EXPORT XGL_VOID XGLAPI xglCmdDbgMarkerEnd(XGL_CMD_BUFFER cmdBuffer)
{
    ll_increment_use_count((XGL_VOID*)cmdBuffer);
    printf("OBJ[%llu] : USING cmdBuffer object %p (%lu total uses)\n", object_track_index++, (void*)cmdBuffer, ll_get_obj_uses((XGL_VOID*)cmdBuffer));
    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11AssociateConnection(XGL_PHYSICAL_GPU gpu, const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    ll_increment_use_count((XGL_VOID*)gpu);
    printf("OBJ[%llu] : USING gpu object %p (%lu total uses)\n", object_track_index++, (void*)gpu, ll_get_obj_uses((XGL_VOID*)gpu));
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable);
    XGL_RESULT result = nextTable.WsiX11AssociateConnection((XGL_PHYSICAL_GPU)gpuw->nextObject, pConnectionInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11GetMSC(XGL_DEVICE device, xcb_randr_crtc_t crtc, XGL_UINT64* pMsc)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.WsiX11GetMSC(device, crtc, pMsc);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11CreatePresentableImage(XGL_DEVICE device, const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    ll_increment_use_count((XGL_VOID*)device);
    printf("OBJ[%llu] : USING device object %p (%lu total uses)\n", object_track_index++, (void*)device, ll_get_obj_uses((XGL_VOID*)device));
    XGL_RESULT result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    printf("OBJ[%llu] : CREATE XGL_GPU_MEMORY object %p\n", object_track_index++, (void*)*pMem);
    ll_insert_obj((XGL_VOID*)*pMem, "XGL_GPU_MEMORY");
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11QueuePresent(XGL_QUEUE queue, const XGL_WSI_X11_PRESENT_INFO* pPresentInfo, XGL_FENCE fence)
{
    ll_increment_use_count((XGL_VOID*)queue);
    printf("OBJ[%llu] : USING queue object %p (%lu total uses)\n", object_track_index++, (void*)queue, ll_get_obj_uses((XGL_VOID*)queue));
    XGL_RESULT result = nextTable.WsiX11QueuePresent(queue, pPresentInfo, fence);
    return result;
}

XGL_LAYER_EXPORT XGL_VOID* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR* funcName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    pthread_once(&tabOnce, initLayerTable);

    if (!strncmp("xglGetProcAddr", (const char *) funcName, sizeof("xglGetProcAddr")))
        return xglGetProcAddr;
    else if (!strncmp("xglInitAndEnumerateGpus", (const char *) funcName, sizeof("xglInitAndEnumerateGpus")))
        return nextTable.InitAndEnumerateGpus;
    else if (!strncmp("xglGetGpuInfo", (const char *) funcName, sizeof("xglGetGpuInfo")))
        return xglGetGpuInfo;
    else if (!strncmp("xglCreateDevice", (const char *) funcName, sizeof("xglCreateDevice")))
        return xglCreateDevice;
    else if (!strncmp("xglDestroyDevice", (const char *) funcName, sizeof("xglDestroyDevice")))
        return xglDestroyDevice;
    else if (!strncmp("xglGetExtensionSupport", (const char *) funcName, sizeof("xglGetExtensionSupport")))
        return xglGetExtensionSupport;
    else if (!strncmp("xglEnumerateLayers", (const char *) funcName, sizeof("xglEnumerateLayers")))
        return xglEnumerateLayers;
    else if (!strncmp("xglGetDeviceQueue", (const char *) funcName, sizeof("xglGetDeviceQueue")))
        return xglGetDeviceQueue;
    else if (!strncmp("xglQueueSubmit", (const char *) funcName, sizeof("xglQueueSubmit")))
        return xglQueueSubmit;
    else if (!strncmp("xglQueueSetGlobalMemReferences", (const char *) funcName, sizeof("xglQueueSetGlobalMemReferences")))
        return xglQueueSetGlobalMemReferences;
    else if (!strncmp("xglQueueWaitIdle", (const char *) funcName, sizeof("xglQueueWaitIdle")))
        return xglQueueWaitIdle;
    else if (!strncmp("xglDeviceWaitIdle", (const char *) funcName, sizeof("xglDeviceWaitIdle")))
        return xglDeviceWaitIdle;
    else if (!strncmp("xglGetMemoryHeapCount", (const char *) funcName, sizeof("xglGetMemoryHeapCount")))
        return xglGetMemoryHeapCount;
    else if (!strncmp("xglGetMemoryHeapInfo", (const char *) funcName, sizeof("xglGetMemoryHeapInfo")))
        return xglGetMemoryHeapInfo;
    else if (!strncmp("xglAllocMemory", (const char *) funcName, sizeof("xglAllocMemory")))
        return xglAllocMemory;
    else if (!strncmp("xglFreeMemory", (const char *) funcName, sizeof("xglFreeMemory")))
        return xglFreeMemory;
    else if (!strncmp("xglSetMemoryPriority", (const char *) funcName, sizeof("xglSetMemoryPriority")))
        return xglSetMemoryPriority;
    else if (!strncmp("xglMapMemory", (const char *) funcName, sizeof("xglMapMemory")))
        return xglMapMemory;
    else if (!strncmp("xglUnmapMemory", (const char *) funcName, sizeof("xglUnmapMemory")))
        return xglUnmapMemory;
    else if (!strncmp("xglPinSystemMemory", (const char *) funcName, sizeof("xglPinSystemMemory")))
        return xglPinSystemMemory;
    else if (!strncmp("xglRemapVirtualMemoryPages", (const char *) funcName, sizeof("xglRemapVirtualMemoryPages")))
        return xglRemapVirtualMemoryPages;
    else if (!strncmp("xglGetMultiGpuCompatibility", (const char *) funcName, sizeof("xglGetMultiGpuCompatibility")))
        return xglGetMultiGpuCompatibility;
    else if (!strncmp("xglOpenSharedMemory", (const char *) funcName, sizeof("xglOpenSharedMemory")))
        return xglOpenSharedMemory;
    else if (!strncmp("xglOpenSharedQueueSemaphore", (const char *) funcName, sizeof("xglOpenSharedQueueSemaphore")))
        return xglOpenSharedQueueSemaphore;
    else if (!strncmp("xglOpenPeerMemory", (const char *) funcName, sizeof("xglOpenPeerMemory")))
        return xglOpenPeerMemory;
    else if (!strncmp("xglOpenPeerImage", (const char *) funcName, sizeof("xglOpenPeerImage")))
        return xglOpenPeerImage;
    else if (!strncmp("xglDestroyObject", (const char *) funcName, sizeof("xglDestroyObject")))
        return xglDestroyObject;
    else if (!strncmp("xglGetObjectInfo", (const char *) funcName, sizeof("xglGetObjectInfo")))
        return xglGetObjectInfo;
    else if (!strncmp("xglBindObjectMemory", (const char *) funcName, sizeof("xglBindObjectMemory")))
        return xglBindObjectMemory;
    else if (!strncmp("xglCreateFence", (const char *) funcName, sizeof("xglCreateFence")))
        return xglCreateFence;
    else if (!strncmp("xglGetFenceStatus", (const char *) funcName, sizeof("xglGetFenceStatus")))
        return xglGetFenceStatus;
    else if (!strncmp("xglWaitForFences", (const char *) funcName, sizeof("xglWaitForFences")))
        return xglWaitForFences;
    else if (!strncmp("xglCreateQueueSemaphore", (const char *) funcName, sizeof("xglCreateQueueSemaphore")))
        return xglCreateQueueSemaphore;
    else if (!strncmp("xglSignalQueueSemaphore", (const char *) funcName, sizeof("xglSignalQueueSemaphore")))
        return xglSignalQueueSemaphore;
    else if (!strncmp("xglWaitQueueSemaphore", (const char *) funcName, sizeof("xglWaitQueueSemaphore")))
        return xglWaitQueueSemaphore;
    else if (!strncmp("xglCreateEvent", (const char *) funcName, sizeof("xglCreateEvent")))
        return xglCreateEvent;
    else if (!strncmp("xglGetEventStatus", (const char *) funcName, sizeof("xglGetEventStatus")))
        return xglGetEventStatus;
    else if (!strncmp("xglSetEvent", (const char *) funcName, sizeof("xglSetEvent")))
        return xglSetEvent;
    else if (!strncmp("xglResetEvent", (const char *) funcName, sizeof("xglResetEvent")))
        return xglResetEvent;
    else if (!strncmp("xglCreateQueryPool", (const char *) funcName, sizeof("xglCreateQueryPool")))
        return xglCreateQueryPool;
    else if (!strncmp("xglGetQueryPoolResults", (const char *) funcName, sizeof("xglGetQueryPoolResults")))
        return xglGetQueryPoolResults;
    else if (!strncmp("xglGetFormatInfo", (const char *) funcName, sizeof("xglGetFormatInfo")))
        return xglGetFormatInfo;
    else if (!strncmp("xglCreateImage", (const char *) funcName, sizeof("xglCreateImage")))
        return xglCreateImage;
    else if (!strncmp("xglGetImageSubresourceInfo", (const char *) funcName, sizeof("xglGetImageSubresourceInfo")))
        return xglGetImageSubresourceInfo;
    else if (!strncmp("xglCreateImageView", (const char *) funcName, sizeof("xglCreateImageView")))
        return xglCreateImageView;
    else if (!strncmp("xglCreateColorAttachmentView", (const char *) funcName, sizeof("xglCreateColorAttachmentView")))
        return xglCreateColorAttachmentView;
    else if (!strncmp("xglCreateDepthStencilView", (const char *) funcName, sizeof("xglCreateDepthStencilView")))
        return xglCreateDepthStencilView;
    else if (!strncmp("xglCreateShader", (const char *) funcName, sizeof("xglCreateShader")))
        return xglCreateShader;
    else if (!strncmp("xglCreateGraphicsPipeline", (const char *) funcName, sizeof("xglCreateGraphicsPipeline")))
        return xglCreateGraphicsPipeline;
    else if (!strncmp("xglCreateComputePipeline", (const char *) funcName, sizeof("xglCreateComputePipeline")))
        return xglCreateComputePipeline;
    else if (!strncmp("xglStorePipeline", (const char *) funcName, sizeof("xglStorePipeline")))
        return xglStorePipeline;
    else if (!strncmp("xglLoadPipeline", (const char *) funcName, sizeof("xglLoadPipeline")))
        return xglLoadPipeline;
    else if (!strncmp("xglCreatePipelineDelta", (const char *) funcName, sizeof("xglCreatePipelineDelta")))
        return xglCreatePipelineDelta;
    else if (!strncmp("xglCreateSampler", (const char *) funcName, sizeof("xglCreateSampler")))
        return xglCreateSampler;
    else if (!strncmp("xglCreateDescriptorSet", (const char *) funcName, sizeof("xglCreateDescriptorSet")))
        return xglCreateDescriptorSet;
    else if (!strncmp("xglBeginDescriptorSetUpdate", (const char *) funcName, sizeof("xglBeginDescriptorSetUpdate")))
        return xglBeginDescriptorSetUpdate;
    else if (!strncmp("xglEndDescriptorSetUpdate", (const char *) funcName, sizeof("xglEndDescriptorSetUpdate")))
        return xglEndDescriptorSetUpdate;
    else if (!strncmp("xglAttachSamplerDescriptors", (const char *) funcName, sizeof("xglAttachSamplerDescriptors")))
        return xglAttachSamplerDescriptors;
    else if (!strncmp("xglAttachImageViewDescriptors", (const char *) funcName, sizeof("xglAttachImageViewDescriptors")))
        return xglAttachImageViewDescriptors;
    else if (!strncmp("xglAttachMemoryViewDescriptors", (const char *) funcName, sizeof("xglAttachMemoryViewDescriptors")))
        return xglAttachMemoryViewDescriptors;
    else if (!strncmp("xglAttachNestedDescriptors", (const char *) funcName, sizeof("xglAttachNestedDescriptors")))
        return xglAttachNestedDescriptors;
    else if (!strncmp("xglClearDescriptorSetSlots", (const char *) funcName, sizeof("xglClearDescriptorSetSlots")))
        return xglClearDescriptorSetSlots;
    else if (!strncmp("xglCreateViewportState", (const char *) funcName, sizeof("xglCreateViewportState")))
        return xglCreateViewportState;
    else if (!strncmp("xglCreateRasterState", (const char *) funcName, sizeof("xglCreateRasterState")))
        return xglCreateRasterState;
    else if (!strncmp("xglCreateMsaaState", (const char *) funcName, sizeof("xglCreateMsaaState")))
        return xglCreateMsaaState;
    else if (!strncmp("xglCreateColorBlendState", (const char *) funcName, sizeof("xglCreateColorBlendState")))
        return xglCreateColorBlendState;
    else if (!strncmp("xglCreateDepthStencilState", (const char *) funcName, sizeof("xglCreateDepthStencilState")))
        return xglCreateDepthStencilState;
    else if (!strncmp("xglCreateCommandBuffer", (const char *) funcName, sizeof("xglCreateCommandBuffer")))
        return xglCreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", (const char *) funcName, sizeof("xglBeginCommandBuffer")))
        return xglBeginCommandBuffer;
    else if (!strncmp("xglEndCommandBuffer", (const char *) funcName, sizeof("xglEndCommandBuffer")))
        return xglEndCommandBuffer;
    else if (!strncmp("xglResetCommandBuffer", (const char *) funcName, sizeof("xglResetCommandBuffer")))
        return xglResetCommandBuffer;
    else if (!strncmp("xglCmdBindPipeline", (const char *) funcName, sizeof("xglCmdBindPipeline")))
        return xglCmdBindPipeline;
    else if (!strncmp("xglCmdBindPipelineDelta", (const char *) funcName, sizeof("xglCmdBindPipelineDelta")))
        return xglCmdBindPipelineDelta;
    else if (!strncmp("xglCmdBindStateObject", (const char *) funcName, sizeof("xglCmdBindStateObject")))
        return xglCmdBindStateObject;
    else if (!strncmp("xglCmdBindDescriptorSet", (const char *) funcName, sizeof("xglCmdBindDescriptorSet")))
        return xglCmdBindDescriptorSet;
    else if (!strncmp("xglCmdBindDynamicMemoryView", (const char *) funcName, sizeof("xglCmdBindDynamicMemoryView")))
        return xglCmdBindDynamicMemoryView;
    else if (!strncmp("xglCmdBindIndexData", (const char *) funcName, sizeof("xglCmdBindIndexData")))
        return xglCmdBindIndexData;
    else if (!strncmp("xglCmdBindAttachments", (const char *) funcName, sizeof("xglCmdBindAttachments")))
        return xglCmdBindAttachments;
    else if (!strncmp("xglCmdPrepareMemoryRegions", (const char *) funcName, sizeof("xglCmdPrepareMemoryRegions")))
        return xglCmdPrepareMemoryRegions;
    else if (!strncmp("xglCmdPrepareImages", (const char *) funcName, sizeof("xglCmdPrepareImages")))
        return xglCmdPrepareImages;
    else if (!strncmp("xglCmdDraw", (const char *) funcName, sizeof("xglCmdDraw")))
        return xglCmdDraw;
    else if (!strncmp("xglCmdDrawIndexed", (const char *) funcName, sizeof("xglCmdDrawIndexed")))
        return xglCmdDrawIndexed;
    else if (!strncmp("xglCmdDrawIndirect", (const char *) funcName, sizeof("xglCmdDrawIndirect")))
        return xglCmdDrawIndirect;
    else if (!strncmp("xglCmdDrawIndexedIndirect", (const char *) funcName, sizeof("xglCmdDrawIndexedIndirect")))
        return xglCmdDrawIndexedIndirect;
    else if (!strncmp("xglCmdDispatch", (const char *) funcName, sizeof("xglCmdDispatch")))
        return xglCmdDispatch;
    else if (!strncmp("xglCmdDispatchIndirect", (const char *) funcName, sizeof("xglCmdDispatchIndirect")))
        return xglCmdDispatchIndirect;
    else if (!strncmp("xglCmdCopyMemory", (const char *) funcName, sizeof("xglCmdCopyMemory")))
        return xglCmdCopyMemory;
    else if (!strncmp("xglCmdCopyImage", (const char *) funcName, sizeof("xglCmdCopyImage")))
        return xglCmdCopyImage;
    else if (!strncmp("xglCmdCopyMemoryToImage", (const char *) funcName, sizeof("xglCmdCopyMemoryToImage")))
        return xglCmdCopyMemoryToImage;
    else if (!strncmp("xglCmdCopyImageToMemory", (const char *) funcName, sizeof("xglCmdCopyImageToMemory")))
        return xglCmdCopyImageToMemory;
    else if (!strncmp("xglCmdCloneImageData", (const char *) funcName, sizeof("xglCmdCloneImageData")))
        return xglCmdCloneImageData;
    else if (!strncmp("xglCmdUpdateMemory", (const char *) funcName, sizeof("xglCmdUpdateMemory")))
        return xglCmdUpdateMemory;
    else if (!strncmp("xglCmdFillMemory", (const char *) funcName, sizeof("xglCmdFillMemory")))
        return xglCmdFillMemory;
    else if (!strncmp("xglCmdClearColorImage", (const char *) funcName, sizeof("xglCmdClearColorImage")))
        return xglCmdClearColorImage;
    else if (!strncmp("xglCmdClearColorImageRaw", (const char *) funcName, sizeof("xglCmdClearColorImageRaw")))
        return xglCmdClearColorImageRaw;
    else if (!strncmp("xglCmdClearDepthStencil", (const char *) funcName, sizeof("xglCmdClearDepthStencil")))
        return xglCmdClearDepthStencil;
    else if (!strncmp("xglCmdResolveImage", (const char *) funcName, sizeof("xglCmdResolveImage")))
        return xglCmdResolveImage;
    else if (!strncmp("xglCmdSetEvent", (const char *) funcName, sizeof("xglCmdSetEvent")))
        return xglCmdSetEvent;
    else if (!strncmp("xglCmdResetEvent", (const char *) funcName, sizeof("xglCmdResetEvent")))
        return xglCmdResetEvent;
    else if (!strncmp("xglCmdMemoryAtomic", (const char *) funcName, sizeof("xglCmdMemoryAtomic")))
        return xglCmdMemoryAtomic;
    else if (!strncmp("xglCmdBeginQuery", (const char *) funcName, sizeof("xglCmdBeginQuery")))
        return xglCmdBeginQuery;
    else if (!strncmp("xglCmdEndQuery", (const char *) funcName, sizeof("xglCmdEndQuery")))
        return xglCmdEndQuery;
    else if (!strncmp("xglCmdResetQueryPool", (const char *) funcName, sizeof("xglCmdResetQueryPool")))
        return xglCmdResetQueryPool;
    else if (!strncmp("xglCmdWriteTimestamp", (const char *) funcName, sizeof("xglCmdWriteTimestamp")))
        return xglCmdWriteTimestamp;
    else if (!strncmp("xglCmdInitAtomicCounters", (const char *) funcName, sizeof("xglCmdInitAtomicCounters")))
        return xglCmdInitAtomicCounters;
    else if (!strncmp("xglCmdLoadAtomicCounters", (const char *) funcName, sizeof("xglCmdLoadAtomicCounters")))
        return xglCmdLoadAtomicCounters;
    else if (!strncmp("xglCmdSaveAtomicCounters", (const char *) funcName, sizeof("xglCmdSaveAtomicCounters")))
        return xglCmdSaveAtomicCounters;
    else if (!strncmp("xglDbgSetValidationLevel", (const char *) funcName, sizeof("xglDbgSetValidationLevel")))
        return xglDbgSetValidationLevel;
    else if (!strncmp("xglDbgRegisterMsgCallback", (const char *) funcName, sizeof("xglDbgRegisterMsgCallback")))
        return xglDbgRegisterMsgCallback;
    else if (!strncmp("xglDbgUnregisterMsgCallback", (const char *) funcName, sizeof("xglDbgUnregisterMsgCallback")))
        return xglDbgUnregisterMsgCallback;
    else if (!strncmp("xglDbgSetMessageFilter", (const char *) funcName, sizeof("xglDbgSetMessageFilter")))
        return xglDbgSetMessageFilter;
    else if (!strncmp("xglDbgSetObjectTag", (const char *) funcName, sizeof("xglDbgSetObjectTag")))
        return xglDbgSetObjectTag;
    else if (!strncmp("xglDbgSetGlobalOption", (const char *) funcName, sizeof("xglDbgSetGlobalOption")))
        return xglDbgSetGlobalOption;
    else if (!strncmp("xglDbgSetDeviceOption", (const char *) funcName, sizeof("xglDbgSetDeviceOption")))
        return xglDbgSetDeviceOption;
    else if (!strncmp("xglCmdDbgMarkerBegin", (const char *) funcName, sizeof("xglCmdDbgMarkerBegin")))
        return xglCmdDbgMarkerBegin;
    else if (!strncmp("xglCmdDbgMarkerEnd", (const char *) funcName, sizeof("xglCmdDbgMarkerEnd")))
        return xglCmdDbgMarkerEnd;
    else if (!strncmp("xglWsiX11AssociateConnection", (const char *) funcName, sizeof("xglWsiX11AssociateConnection")))
        return xglWsiX11AssociateConnection;
    else if (!strncmp("xglWsiX11GetMSC", (const char *) funcName, sizeof("xglWsiX11GetMSC")))
        return xglWsiX11GetMSC;
    else if (!strncmp("xglWsiX11CreatePresentableImage", (const char *) funcName, sizeof("xglWsiX11CreatePresentableImage")))
        return xglWsiX11CreatePresentableImage;
    else if (!strncmp("xglWsiX11QueuePresent", (const char *) funcName, sizeof("xglWsiX11QueuePresent")))
        return xglWsiX11QueuePresent;
    else {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA(gpuw->nextObject, funcName);
    }
}

