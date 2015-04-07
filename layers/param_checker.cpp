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

#include <iostream>
#include <string>
#include <sstream>

#include "loader_platform.h"
#include "xglLayer.h"
#include "layers_config.h"
#include "xgl_enum_validate_helper.h"
#include "xgl_struct_validate_helper.h"
//The following is #included again to catch certain OS-specific functions being used:
#include "loader_platform.h"

#include "layers_msg.h"

static XGL_LAYER_DISPATCH_TABLE nextTable;
static XGL_BASE_LAYER_OBJECT *pCurObj;
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);

#include "xgl_dispatch_table_helper.h"
static void initParamChecker(void)
{

    const char *strOpt;
    // initialize ParamChecker options
    getLayerOptionEnum("ParamCheckerReportLevel", (uint32_t *) &g_reportingLevel);
    g_actionIsDefault = getLayerOptionEnum("ParamCheckerDebugAction", (uint32_t *) &g_debugAction);

    if (g_debugAction & XGL_DBG_LAYER_ACTION_LOG_MSG)
    {
        strOpt = getLayerOption("ParamCheckerLogFilename");
        if (strOpt)
        {
            g_logFile = fopen(strOpt, "w");
        }
        if (g_logFile == NULL)
            g_logFile = stdout;
    }

    xglGetProcAddrType fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (XGL_PHYSICAL_GPU) pCurObj->nextObject);
}

void PreCreateInstance(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb)
{
    if(pAppInfo == nullptr)
    {
        char const str[] = "xglCreateInstance parameter, XGL_APPLICATION_INFO* pAppInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pAppInfo->sType != XGL_STRUCTURE_TYPE_APPLICATION_INFO)
    {
        char const str[] = "xglCreateInstance parameter, XGL_STRUCTURE_TYPE_APPLICATION_INFO "\
            "pAppInfo->sType, is not XGL_STRUCTURE_TYPE_APPLICATION_INFO (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    // TODO: What else can validated in pAppInfo?
    // TODO: XGL_API_VERSION validation.

    // It's okay if pAllocCb is a nullptr.
    if(pAllocCb != nullptr)
    {
        if(!xgl_validate_xgl_alloc_callbacks(pAllocCb))
        {
            char const str[] = "xglCreateInstance parameter, XGL_ALLOC_CALLBACKS* pAllocCb, "\
                "contains an invalid value (precondition).";
            layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
            return;
        }
    }
}

void PostCreateInstance(XGL_RESULT result, XGL_INSTANCE* pInstance)
{
    if(result != XGL_SUCCESS)
    {
        // TODO: Spit out XGL_RESULT value.
        char const str[] = "xglCreateInstance failed (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pInstance == nullptr)
    {
        char const str[] = "xglCreateInstance parameter, XGL_INSTANCE* pInstance, is nullptr (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateInstance(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, XGL_INSTANCE* pInstance)
{
    PreCreateInstance(pAppInfo, pAllocCb);
    XGL_RESULT result = nextTable.CreateInstance(pAppInfo, pAllocCb, pInstance);
    PostCreateInstance(result, pInstance);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyInstance(XGL_INSTANCE instance)
{

    XGL_RESULT result = nextTable.DestroyInstance(instance);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateGpus(XGL_INSTANCE instance, uint32_t maxGpus, uint32_t* pGpuCount, XGL_PHYSICAL_GPU* pGpus)
{

    XGL_RESULT result = nextTable.EnumerateGpus(instance, maxGpus, pGpuCount, pGpus);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetGpuInfo(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);
    char str[1024];
    if (!validate_XGL_PHYSICAL_GPU_INFO_TYPE(infoType)) {
        sprintf(str, "Parameter infoType to function GetGpuInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.GetGpuInfo((XGL_PHYSICAL_GPU)gpuw->nextObject, infoType, pDataSize, pData);
    return result;
}

void PreCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo)
{
    if(gpu == nullptr)
    {
        char const str[] = "xglCreateDevice parameter, XGL_PHYSICAL_GPU gpu, is nullptr "\
            "(precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo == nullptr)
    {
        char const str[] = "xglCreateDevice parameter, XGL_DEVICE_CREATE_INFO* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType !=  XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
    {
        char const str[] = "xglCreateDevice parameter, XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO "\
            "pCreateInfo->sType, is not XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->queueRecordCount == 0)
    {
        char const str[] = "xglCreateDevice parameter, uint32_t pCreateInfo->queueRecordCount, is "\
            "zero (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pRequestedQueues == nullptr)
    {
        char const str[] = "xglCreateDevice parameter, XGL_DEVICE_QUEUE_CREATE_INFO* pCreateInfo->pRequestedQueues, is "\
            "nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->queueRecordCount; ++i)
    {
        if(!xgl_validate_xgl_device_queue_create_info(&(pCreateInfo->pRequestedQueues[i])))
        {
            std::stringstream ss;
            ss << "xglCreateDevice parameter, XGL_DEVICE_QUEUE_CREATE_INFO pCreateInfo->pRequestedQueues[" << i <<
                "], is invalid (precondition).";
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(!validate_XGL_VALIDATION_LEVEL(pCreateInfo->maxValidationLevel))
    {
        char const str[] = "xglCreateDevice parameter, XGL_VALIDATION_LEVEL pCreateInfo->maxValidationLevel, is "\
            "unrecognized (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateDevice(XGL_RESULT result, XGL_DEVICE* pDevice)
{
    if(result != XGL_SUCCESS)
    {
        // TODO: Spit out XGL_RESULT value.
        char const str[] = "xglCreateDevice failed (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pDevice == nullptr)
    {
        char const str[] = "xglCreateDevice parameter, XGL_DEVICE* pDevice, is nullptr (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDevice(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);
    PreCreateDevice(gpu, pCreateInfo);
    XGL_RESULT result = nextTable.CreateDevice((XGL_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    PostCreateDevice(result, pDevice);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyDevice(XGL_DEVICE device)
{

    XGL_RESULT result = nextTable.DestroyDevice(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(XGL_PHYSICAL_GPU gpu, const char* pExtName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    XGL_RESULT result = nextTable.GetExtensionSupport((XGL_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    char str[1024];
    if (gpu != NULL) {
        XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
        sprintf(str, "At start of layered EnumerateLayers\n");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, nullptr, 0, 0, "PARAMCHECK", str);
        pCurObj = gpuw;
        loader_platform_thread_once(&tabOnce, initParamChecker);
        XGL_RESULT result = nextTable.EnumerateLayers((XGL_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
        sprintf(str, "Completed layered EnumerateLayers\n");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, nullptr, 0, 0, "PARAMCHECK", str);
        fflush(stdout);
        return result;
    } else {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)
            return XGL_ERROR_INVALID_POINTER;
        // This layer compatible with all GPUs
        *pOutLayerCount = 1;
        strncpy(pOutLayers[0], "ParamChecker", maxStringSize);
        return XGL_SUCCESS;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetDeviceQueue(XGL_DEVICE device, uint32_t queueNodeIndex, uint32_t queueIndex, XGL_QUEUE* pQueue)
{

    XGL_RESULT result = nextTable.GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(XGL_QUEUE queue, uint32_t cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence)
{
    char str[1024];
    uint32_t i;
    XGL_RESULT result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, memRefCount, pMemRefs, fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueWaitIdle(XGL_QUEUE queue)
{

    XGL_RESULT result = nextTable.QueueWaitIdle(queue);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDeviceWaitIdle(XGL_DEVICE device)
{

    XGL_RESULT result = nextTable.DeviceWaitIdle(device);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocMemory(XGL_DEVICE device, const XGL_MEMORY_ALLOC_INFO* pAllocInfo, XGL_GPU_MEMORY* pMem)
{
    char str[1024];
    if (!pAllocInfo) {
        sprintf(str, "Struct ptr parameter pAllocInfo to function AllocMemory is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_memory_alloc_info(pAllocInfo)) {
        sprintf(str, "Parameter pAllocInfo to function AllocMemory contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.AllocMemory(device, pAllocInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglFreeMemory(XGL_GPU_MEMORY mem)
{

    XGL_RESULT result = nextTable.FreeMemory(mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetMemoryPriority(XGL_GPU_MEMORY mem, XGL_MEMORY_PRIORITY priority)
{
    char str[1024];
    if (!validate_XGL_MEMORY_PRIORITY(priority)) {
        sprintf(str, "Parameter priority to function SetMemoryPriority has invalid value of %i.", (int)priority);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.SetMemoryPriority(mem, priority);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglMapMemory(XGL_GPU_MEMORY mem, XGL_FLAGS flags, void** ppData)
{

    XGL_RESULT result = nextTable.MapMemory(mem, flags, ppData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglUnmapMemory(XGL_GPU_MEMORY mem)
{

    XGL_RESULT result = nextTable.UnmapMemory(mem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglPinSystemMemory(XGL_DEVICE device, const void* pSysMem, size_t memSize, XGL_GPU_MEMORY* pMem)
{

    XGL_RESULT result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(XGL_PHYSICAL_GPU gpu0, XGL_PHYSICAL_GPU gpu1, XGL_GPU_COMPATIBILITY_INFO* pInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu0;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    XGL_RESULT result = nextTable.GetMultiGpuCompatibility((XGL_PHYSICAL_GPU)gpuw->nextObject, gpu1, pInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedMemory(XGL_DEVICE device, const XGL_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenSharedMemory is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_memory_open_info(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenSharedMemory contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenSharedSemaphore(XGL_DEVICE device, const XGL_SEMAPHORE_OPEN_INFO* pOpenInfo, XGL_SEMAPHORE* pSemaphore)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenSharedSemaphore is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_semaphore_open_info(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenSharedSemaphore contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.OpenSharedSemaphore(device, pOpenInfo, pSemaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerMemory(XGL_DEVICE device, const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenPeerMemory is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_peer_memory_open_info(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenPeerMemory contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglOpenPeerImage(XGL_DEVICE device, const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenPeerImage is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_peer_image_open_info(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenPeerImage contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDestroyObject(XGL_OBJECT object)
{

    XGL_RESULT result = nextTable.DestroyObject(object);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetObjectInfo(XGL_BASE_OBJECT object, XGL_OBJECT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!validate_XGL_OBJECT_INFO_TYPE(infoType)) {
        sprintf(str, "Parameter infoType to function GetObjectInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemory(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset)
{

    XGL_RESULT result = nextTable.BindObjectMemory(object, allocationIdx, mem, offset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindObjectMemoryRange(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_SIZE rangeOffset, XGL_GPU_SIZE rangeSize, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset)
{

    XGL_RESULT result = nextTable.BindObjectMemoryRange(object, allocationIdx, rangeOffset, rangeSize, mem, memOffset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBindImageMemoryRange(XGL_IMAGE image, uint32_t allocationIdx, const XGL_IMAGE_MEMORY_BIND_INFO* bindInfo, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset)
{
    char str[1024];
    if (!bindInfo) {
        sprintf(str, "Struct ptr parameter bindInfo to function BindImageMemoryRange is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_image_memory_bind_info(bindInfo)) {
        sprintf(str, "Parameter bindInfo to function BindImageMemoryRange contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.BindImageMemoryRange(image, allocationIdx, bindInfo, mem, memOffset);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFence(XGL_DEVICE device, const XGL_FENCE_CREATE_INFO* pCreateInfo, XGL_FENCE* pFence)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateFence is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_fence_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateFence contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateFence(device, pCreateInfo, pFence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFenceStatus(XGL_FENCE fence)
{

    XGL_RESULT result = nextTable.GetFenceStatus(fence);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWaitForFences(XGL_DEVICE device, uint32_t fenceCount, const XGL_FENCE* pFences, bool32_t waitAll, uint64_t timeout)
{

    XGL_RESULT result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateSemaphore(XGL_DEVICE device, const XGL_SEMAPHORE_CREATE_INFO* pCreateInfo, XGL_SEMAPHORE* pSemaphore)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateSemaphore is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_semaphore_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateSemaphore contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateSemaphore(device, pCreateInfo, pSemaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueSignalSemaphore(XGL_QUEUE queue, XGL_SEMAPHORE semaphore)
{

    XGL_RESULT result = nextTable.QueueSignalSemaphore(queue, semaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglQueueWaitSemaphore(XGL_QUEUE queue, XGL_SEMAPHORE semaphore)
{

    XGL_RESULT result = nextTable.QueueWaitSemaphore(queue, semaphore);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateEvent(XGL_DEVICE device, const XGL_EVENT_CREATE_INFO* pCreateInfo, XGL_EVENT* pEvent)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateEvent is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_event_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateEvent contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetEventStatus(XGL_EVENT event)
{

    XGL_RESULT result = nextTable.GetEventStatus(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglSetEvent(XGL_EVENT event)
{

    XGL_RESULT result = nextTable.SetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetEvent(XGL_EVENT event)
{

    XGL_RESULT result = nextTable.ResetEvent(event);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateQueryPool(XGL_DEVICE device, const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo, XGL_QUERY_POOL* pQueryPool)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateQueryPool is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_query_pool_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateQueryPool contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetQueryPoolResults(XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData)
{

    XGL_RESULT result = nextTable.GetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!validate_XGL_FORMAT(format)) {
        sprintf(str, "Parameter format to function GetFormatInfo has invalid value of %i.", (int)format);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_XGL_FORMAT_INFO_TYPE(infoType)) {
        sprintf(str, "Parameter infoType to function GetFormatInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBuffer(XGL_DEVICE device, const XGL_BUFFER_CREATE_INFO* pCreateInfo, XGL_BUFFER* pBuffer)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateBuffer is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_buffer_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateBuffer contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateBuffer(device, pCreateInfo, pBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateBufferView(XGL_DEVICE device, const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo, XGL_BUFFER_VIEW* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateBufferView is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_buffer_view_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateBufferView contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    return result;
}

void PreCreateImage(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo)
{
    if(pCreateInfo == nullptr)
    {
        char const str[] = "xglCreateImage parameter, XGL_IMAGE_CREATE_INFO* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType != XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
    {
        char const str[] = "xglCreateImage parameter, XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO "\
            "pCreateInfo->sType, is not XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_XGL_IMAGE_TYPE(pCreateInfo->imageType))
    {
        char const str[] = "xglCreateImage parameter, XGL_IMAGE_TYPE pCreateInfo->imageType, is "\
            "unrecognized (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_XGL_FORMAT(pCreateInfo->format))
    {
        char const str[] = "xglCreateImage parameter, XGL_FORMAT pCreateInfo->format, is "\
            "unrecognized (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    XGL_FORMAT_PROPERTIES properties;
    size_t size = sizeof(properties);
    XGL_RESULT result = nextTable.GetFormatInfo(device, pCreateInfo->format,
        XGL_INFO_TYPE_FORMAT_PROPERTIES, &size, &properties);
    if(result != XGL_SUCCESS)
    {
        char const str[] = "xglCreateImage parameter, XGL_FORMAT pCreateInfo->format, cannot be "\
            "validated (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
    {
        char const str[] = "xglCreateImage parameter, XGL_FORMAT pCreateInfo->format, contains "\
            "unsupported format (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    // TODO: Can we check device-specific limits?
    if (!xgl_validate_xgl_extent3d(&pCreateInfo->extent))
    {
        char const str[] = "xglCreateImage parameter, XGL_EXTENT3D pCreateInfo->extent, is invalid "\
            "(precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_XGL_IMAGE_TILING(pCreateInfo->tiling))
    {
        char const str[] = "xglCreateImage parameter, XGL_IMAGE_TILING pCreateInfo->tiling, is "\
            "unrecoginized (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateImage(XGL_RESULT result, XGL_IMAGE* pImage)
{
    if(result != XGL_SUCCESS)
    {
        // TODO: Spit out XGL_RESULT value.
        char const str[] = "xglCreateImage failed (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pImage == nullptr)
    {
        char const str[] = "xglCreateImage parameter, XGL_IMAGE* pImage, is nullptr (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImage(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage)
{
    PreCreateImage(device, pCreateInfo);
    XGL_RESULT result = nextTable.CreateImage(device, pCreateInfo, pImage);
    PostCreateImage(result, pImage);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglGetImageSubresourceInfo(XGL_IMAGE image, const XGL_IMAGE_SUBRESOURCE* pSubresource, XGL_SUBRESOURCE_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!pSubresource) {
        sprintf(str, "Struct ptr parameter pSubresource to function GetImageSubresourceInfo is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_image_subresource(pSubresource)) {
        sprintf(str, "Parameter pSubresource to function GetImageSubresourceInfo contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_XGL_SUBRESOURCE_INFO_TYPE(infoType)) {
        sprintf(str, "Parameter infoType to function GetImageSubresourceInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.GetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateImageView(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateImageView is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_image_view_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateImageView contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateColorAttachmentView(XGL_DEVICE device, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, XGL_COLOR_ATTACHMENT_VIEW* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateColorAttachmentView is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_color_attachment_view_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateColorAttachmentView contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilView(XGL_DEVICE device, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_VIEW* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDepthStencilView is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_depth_stencil_view_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDepthStencilView contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateShader(XGL_DEVICE device, const XGL_SHADER_CREATE_INFO* pCreateInfo, XGL_SHADER* pShader)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateShader is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_shader_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateShader contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateShader(device, pCreateInfo, pShader);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateGraphicsPipeline(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateGraphicsPipeline is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_graphics_pipeline_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateGraphicsPipeline contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateGraphicsPipelineDerivative(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE basePipeline, XGL_PIPELINE* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateGraphicsPipelineDerivative is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_graphics_pipeline_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateGraphicsPipelineDerivative contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateGraphicsPipelineDerivative(device, pCreateInfo, basePipeline, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateComputePipeline(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateComputePipeline is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_compute_pipeline_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateComputePipeline contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglStorePipeline(XGL_PIPELINE pipeline, size_t* pDataSize, void* pData)
{

    XGL_RESULT result = nextTable.StorePipeline(pipeline, pDataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLoadPipeline(XGL_DEVICE device, size_t dataSize, const void* pData, XGL_PIPELINE* pPipeline)
{

    XGL_RESULT result = nextTable.LoadPipeline(device, dataSize, pData, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglLoadPipelineDerivative(XGL_DEVICE device, size_t dataSize, const void* pData, XGL_PIPELINE basePipeline, XGL_PIPELINE* pPipeline)
{

    XGL_RESULT result = nextTable.LoadPipelineDerivative(device, dataSize, pData, basePipeline, pPipeline);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateSampler(XGL_DEVICE device, const XGL_SAMPLER_CREATE_INFO* pCreateInfo, XGL_SAMPLER* pSampler)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateSampler is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_sampler_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateSampler contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayout(XGL_DEVICE device, const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_SET_LAYOUT* pSetLayout)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDescriptorSetLayout is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_descriptor_set_layout_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDescriptorSetLayout contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayoutChain(XGL_DEVICE device, uint32_t setLayoutArrayCount, const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayoutArray, XGL_DESCRIPTOR_SET_LAYOUT_CHAIN* pLayoutChain)
{

    XGL_RESULT result = nextTable.CreateDescriptorSetLayoutChain(device, setLayoutArrayCount, pSetLayoutArray, pLayoutChain);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginDescriptorPoolUpdate(XGL_DEVICE device, XGL_DESCRIPTOR_UPDATE_MODE updateMode)
{
    char str[1024];
    if (!validate_XGL_DESCRIPTOR_UPDATE_MODE(updateMode)) {
        sprintf(str, "Parameter updateMode to function BeginDescriptorPoolUpdate has invalid value of %i.", (int)updateMode);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.BeginDescriptorPoolUpdate(device, updateMode);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndDescriptorPoolUpdate(XGL_DEVICE device, XGL_CMD_BUFFER cmd)
{

    XGL_RESULT result = nextTable.EndDescriptorPoolUpdate(device, cmd);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorPool(XGL_DEVICE device, XGL_DESCRIPTOR_POOL_USAGE poolUsage, uint32_t maxSets, const XGL_DESCRIPTOR_POOL_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_POOL* pDescriptorPool)
{
    char str[1024];
    if (!validate_XGL_DESCRIPTOR_POOL_USAGE(poolUsage)) {
        sprintf(str, "Parameter poolUsage to function CreateDescriptorPool has invalid value of %i.", (int)poolUsage);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDescriptorPool is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_descriptor_pool_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDescriptorPool contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetDescriptorPool(XGL_DESCRIPTOR_POOL descriptorPool)
{

    XGL_RESULT result = nextTable.ResetDescriptorPool(descriptorPool);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglAllocDescriptorSets(XGL_DESCRIPTOR_POOL descriptorPool, XGL_DESCRIPTOR_SET_USAGE setUsage, uint32_t count, const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayouts, XGL_DESCRIPTOR_SET* pDescriptorSets, uint32_t* pCount)
{
    char str[1024];
    if (!validate_XGL_DESCRIPTOR_SET_USAGE(setUsage)) {
        sprintf(str, "Parameter setUsage to function AllocDescriptorSets has invalid value of %i.", (int)setUsage);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.AllocDescriptorSets(descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglClearDescriptorSets(XGL_DESCRIPTOR_POOL descriptorPool, uint32_t count, const XGL_DESCRIPTOR_SET* pDescriptorSets)
{

    nextTable.ClearDescriptorSets(descriptorPool, count, pDescriptorSets);
}

XGL_LAYER_EXPORT void XGLAPI xglUpdateDescriptors(XGL_DESCRIPTOR_SET descriptorSet, uint32_t updateCount, const void** ppUpdateArray)
{

    nextTable.UpdateDescriptors(descriptorSet, updateCount, ppUpdateArray);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicViewportState(XGL_DEVICE device, const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_VP_STATE_OBJECT* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicViewportState is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_dynamic_vp_state_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicViewportState contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateDynamicViewportState(device, pCreateInfo, pState);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicRasterState(XGL_DEVICE device, const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_RS_STATE_OBJECT* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicRasterState is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_dynamic_rs_state_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicRasterState contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateDynamicRasterState(device, pCreateInfo, pState);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicColorBlendState(XGL_DEVICE device, const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_CB_STATE_OBJECT* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicColorBlendState is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_dynamic_cb_state_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicColorBlendState contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateDynamicColorBlendState(device, pCreateInfo, pState);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateDynamicDepthStencilState(XGL_DEVICE device, const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_DS_STATE_OBJECT* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicDepthStencilState is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_dynamic_ds_state_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicDepthStencilState contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    return result;
}

void PreCreateCommandBuffer(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo)
{
    if(device == nullptr)
    {
        char const str[] = "xglCreateCommandBuffer parameter, XGL_DEVICE device, is "\
            "nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo == nullptr)
    {
        char const str[] = "xglCreateCommandBuffer parameter, XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType != XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO)
    {
        char const str[] = "xglCreateCommandBuffer parameter, XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO "\
            "pCreateInfo->sType, is not XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateCommandBuffer(XGL_RESULT result, XGL_CMD_BUFFER* pCmdBuffer)
{
    if(result != XGL_SUCCESS)
    {
        // TODO: Spit out XGL_RESULT value.
        char const str[] = "xglCreateCommandBuffer failed (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCmdBuffer == nullptr)
    {
        char const str[] = "xglCreateCommandBuffer parameter, XGL_CMD_BUFFER* pCmdBuffer, is nullptr (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateCommandBuffer(XGL_DEVICE device,
    const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, XGL_CMD_BUFFER* pCmdBuffer)
{
    PreCreateCommandBuffer(device, pCreateInfo);
    XGL_RESULT result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    PostCreateCommandBuffer(result, pCmdBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglBeginCommandBuffer(XGL_CMD_BUFFER cmdBuffer, const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo)
{
    char str[1024];
    if (!pBeginInfo) {
        sprintf(str, "Struct ptr parameter pBeginInfo to function BeginCommandBuffer is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_cmd_buffer_begin_info(pBeginInfo)) {
        sprintf(str, "Parameter pBeginInfo to function BeginCommandBuffer contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglEndCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{

    XGL_RESULT result = nextTable.EndCommandBuffer(cmdBuffer);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglResetCommandBuffer(XGL_CMD_BUFFER cmdBuffer)
{

    XGL_RESULT result = nextTable.ResetCommandBuffer(cmdBuffer);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindPipeline(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE pipeline)
{
    char str[1024];
    if (!validate_XGL_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdBindPipeline has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDynamicStateObject(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_DYNAMIC_STATE_OBJECT state)
{
    char str[1024];
    if (!validate_XGL_STATE_BIND_POINT(stateBindPoint)) {
        sprintf(str, "Parameter stateBindPoint to function CmdBindDynamicStateObject has invalid value of %i.", (int)stateBindPoint);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindDescriptorSets(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_DESCRIPTOR_SET_LAYOUT_CHAIN layoutChain, uint32_t layoutChainSlot, uint32_t count, const XGL_DESCRIPTOR_SET* pDescriptorSets, const uint32_t* pUserData)
{
    char str[1024];
    if (!validate_XGL_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdBindDescriptorSets has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layoutChain, layoutChainSlot, count, pDescriptorSets, pUserData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindVertexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t binding)
{

    nextTable.CmdBindVertexBuffer(cmdBuffer, buffer, offset, binding);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBindIndexBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType)
{
    char str[1024];
    if (!validate_XGL_INDEX_TYPE(indexType)) {
        sprintf(str, "Parameter indexType to function CmdBindIndexBuffer has invalid value of %i.", (int)indexType);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDraw(XGL_CMD_BUFFER cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{

    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndexed(XGL_CMD_BUFFER cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{

    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{

    nextTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDrawIndexedIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride)
{

    nextTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDispatch(XGL_CMD_BUFFER cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{

    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDispatchIndirect(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset)
{

    nextTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_COPY* pRegions)
{
    char str[1024];
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!xgl_validate_xgl_buffer_copy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyBuffer contains an invalid value.", i);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout, uint32_t regionCount, const XGL_IMAGE_COPY* pRegions)
{
    char str[1024];
    if (!validate_XGL_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCopyImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_XGL_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCopyImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!xgl_validate_xgl_image_copy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyImage contains an invalid value.", i);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBlitImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout, uint32_t regionCount, const XGL_IMAGE_BLIT* pRegions)
{
    char str[1024];
    if (!validate_XGL_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdBlitImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_XGL_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdBlitImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!xgl_validate_xgl_image_blit(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdBlitImage contains an invalid value.", i);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyBufferToImage(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    char str[1024];
    if (!validate_XGL_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCopyBufferToImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!xgl_validate_xgl_buffer_image_copy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyBufferToImage contains an invalid value.", i);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCopyImageToBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions)
{
    char str[1024];
    if (!validate_XGL_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCopyImageToBuffer has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!xgl_validate_xgl_buffer_image_copy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyImageToBuffer contains an invalid value.", i);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdCloneImageData(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout)
{
    char str[1024];
    if (!validate_XGL_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCloneImageData has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_XGL_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCloneImageData has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdUpdateBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const uint32_t* pData)
{

    nextTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdFillBuffer(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, uint32_t data)
{

    nextTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearColorImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, XGL_IMAGE_LAYOUT imageLayout, XGL_CLEAR_COLOR color, uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    char str[1024];
    if (!validate_XGL_IMAGE_LAYOUT(imageLayout)) {
        sprintf(str, "Parameter imageLayout to function CmdClearColorImage has invalid value of %i.", (int)imageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rangeCount; i++) {
        if (!xgl_validate_xgl_image_subresource_range(&pRanges[i])) {
            sprintf(str, "Parameter pRanges[%i] to function CmdClearColorImage contains an invalid value.", i);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdClearColorImage(cmdBuffer, image, imageLayout, color, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdClearDepthStencil(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, XGL_IMAGE_LAYOUT imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    char str[1024];
    if (!validate_XGL_IMAGE_LAYOUT(imageLayout)) {
        sprintf(str, "Parameter imageLayout to function CmdClearDepthStencil has invalid value of %i.", (int)imageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rangeCount; i++) {
        if (!xgl_validate_xgl_image_subresource_range(&pRanges[i])) {
            sprintf(str, "Parameter pRanges[%i] to function CmdClearDepthStencil contains an invalid value.", i);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResolveImage(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout, uint32_t rectCount, const XGL_IMAGE_RESOLVE* pRects)
{
    char str[1024];
    if (!validate_XGL_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdResolveImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_XGL_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdResolveImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rectCount; i++) {
        if (!xgl_validate_xgl_image_resolve(&pRects[i])) {
            sprintf(str, "Parameter pRects[%i] to function CmdResolveImage contains an invalid value.", i);
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, rectCount, pRects);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdSetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event, XGL_PIPE_EVENT pipeEvent)
{
    char str[1024];
    if (!validate_XGL_PIPE_EVENT(pipeEvent)) {
        sprintf(str, "Parameter pipeEvent to function CmdSetEvent has invalid value of %i.", (int)pipeEvent);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdSetEvent(cmdBuffer, event, pipeEvent);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResetEvent(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event, XGL_PIPE_EVENT pipeEvent)
{
    char str[1024];
    if (!validate_XGL_PIPE_EVENT(pipeEvent)) {
        sprintf(str, "Parameter pipeEvent to function CmdResetEvent has invalid value of %i.", (int)pipeEvent);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdResetEvent(cmdBuffer, event, pipeEvent);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdWaitEvents(XGL_CMD_BUFFER cmdBuffer, const XGL_EVENT_WAIT_INFO* pWaitInfo)
{
    char str[1024];
    if (!pWaitInfo) {
        sprintf(str, "Struct ptr parameter pWaitInfo to function CmdWaitEvents is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_event_wait_info(pWaitInfo)) {
        sprintf(str, "Parameter pWaitInfo to function CmdWaitEvents contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdWaitEvents(cmdBuffer, pWaitInfo);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdPipelineBarrier(XGL_CMD_BUFFER cmdBuffer, const XGL_PIPELINE_BARRIER* pBarrier)
{
    char str[1024];
    if (!pBarrier) {
        sprintf(str, "Struct ptr parameter pBarrier to function CmdPipelineBarrier is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_pipeline_barrier(pBarrier)) {
        sprintf(str, "Parameter pBarrier to function CmdPipelineBarrier contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdPipelineBarrier(cmdBuffer, pBarrier);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBeginQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot, XGL_FLAGS flags)
{

    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdEndQuery(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot)
{

    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdResetQueryPool(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount)
{

    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdWriteTimestamp(XGL_CMD_BUFFER cmdBuffer, XGL_TIMESTAMP_TYPE timestampType, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset)
{
    char str[1024];
    if (!validate_XGL_TIMESTAMP_TYPE(timestampType)) {
        sprintf(str, "Parameter timestampType to function CmdWriteTimestamp has invalid value of %i.", (int)timestampType);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdInitAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    char str[1024];
    if (!validate_XGL_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdInitAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdLoadAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER srcBuffer, XGL_GPU_SIZE srcOffset)
{
    char str[1024];
    if (!validate_XGL_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdLoadAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdSaveAtomicCounters(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset)
{
    char str[1024];
    if (!validate_XGL_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdSaveAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateFramebuffer(XGL_DEVICE device, const XGL_FRAMEBUFFER_CREATE_INFO* pCreateInfo, XGL_FRAMEBUFFER* pFramebuffer)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateFramebuffer is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_framebuffer_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateFramebuffer contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    return result;
}


void PreCreateRenderPass(XGL_DEVICE device, const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo)
{
    if(pCreateInfo == nullptr)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_RENDER_PASS_CREATE_INFO* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType != XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO "\
            "pCreateInfo->sType, is not XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO (precondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!xgl_validate_xgl_rect(&pCreateInfo->renderArea))
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_RECT pCreateInfo->renderArea, is invalid "\
            "(precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!xgl_validate_xgl_extent2d(&pCreateInfo->extent))
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_EXTENT2D pCreateInfo->extent, is invalid "\
            "(precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pColorFormats == nullptr)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_FORMAT* pCreateInfo->pColorFormats, "\
            "is nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_XGL_FORMAT(pCreateInfo->pColorFormats[i]))
        {
            std::stringstream ss;
            ss << "xglCreateRenderPass parameter, XGL_FORMAT pCreateInfo->pColorFormats[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

        XGL_FORMAT_PROPERTIES properties;
        size_t size = sizeof(properties);
        XGL_RESULT result = nextTable.GetFormatInfo(device, pCreateInfo->pColorFormats[i],
            XGL_INFO_TYPE_FORMAT_PROPERTIES, &size, &properties);
        if(result != XGL_SUCCESS)
        {
            std::stringstream ss;
            ss << "xglCreateRenderPass parameter, XGL_FORMAT pCreateInfo->pColorFormats[" << i <<
                "], cannot be validated (precondition).";
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

        if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
        {
            std::stringstream ss;
            ss << "xglCreateRenderPass parameter, XGL_FORMAT pCreateInfo->pColorFormats[" << i <<
                "], contains unsupported format (precondition).";
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

    }

    if(pCreateInfo->pColorLayouts == nullptr)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_IMAGE_LAYOUT* pCreateInfo->pColorLayouts, "\
            "is nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_XGL_IMAGE_LAYOUT(pCreateInfo->pColorLayouts[i]))
        {
            std::stringstream ss;
            ss << "xglCreateRenderPass parameter, XGL_IMAGE_LAYOUT pCreateInfo->pColorLayouts[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadOps == nullptr)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_ATTACHMENT_LOAD_OP* pCreateInfo->pColorLoadOps, "\
            "is nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_XGL_ATTACHMENT_LOAD_OP(pCreateInfo->pColorLoadOps[i]))
        {
            std::stringstream ss;
            ss << "xglCreateRenderPass parameter, XGL_ATTACHMENT_LOAD_OP pCreateInfo->pColorLoadOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorStoreOps == nullptr)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_ATTACHMENT_STORE_OP* pCreateInfo->pColorStoreOps, "\
            "is nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_XGL_ATTACHMENT_STORE_OP(pCreateInfo->pColorStoreOps[i]))
        {
            std::stringstream ss;
            ss << "xglCreateRenderPass parameter, XGL_ATTACHMENT_STORE_OP pCreateInfo->pColorStoreOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadClearValues == nullptr)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_CLEAR_COLOR* pCreateInfo->"\
            "pColorLoadClearValues, is nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pColorStoreOps == nullptr)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_ATTACHMENT_STORE_OP* pCreateInfo->pColorStoreOps, \
            is nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_XGL_ATTACHMENT_STORE_OP(pCreateInfo->pColorStoreOps[i]))
        {
            std::stringstream ss;
            ss << "xglCreateRenderPass parameter, XGL_ATTACHMENT_STORE_OP pCreateInfo->pColorStoreOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadClearValues == nullptr)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_CLEAR_COLOR* pCreateInfo->\
            pColorLoadClearValues, is nullptr (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
   
    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!xgl_validate_xgl_clear_color(&(pCreateInfo->pColorLoadClearValues[i])))
        {
            std::stringstream ss;
            ss << "xglCreateRenderPass parameter, XGL_CLEAR_COLOR pCreateInfo->pColorLoadClearValues[" << i <<
                "], is invalid (precondition).";
            layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(!validate_XGL_FORMAT(pCreateInfo->depthStencilFormat))
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_FORMAT pCreateInfo->"\
            "depthStencilFormat, is unrecognized (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    XGL_FORMAT_PROPERTIES properties;
    size_t size = sizeof(properties);
    XGL_RESULT result = nextTable.GetFormatInfo(device, pCreateInfo->depthStencilFormat,
        XGL_INFO_TYPE_FORMAT_PROPERTIES, &size, &properties);
    if(result != XGL_SUCCESS)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_FORMAT pCreateInfo->"\
            "depthStencilFormat, cannot be validated (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_FORMAT pCreateInfo->"\
            "depthStencilFormat, contains unsupported format (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_XGL_IMAGE_LAYOUT(pCreateInfo->depthStencilLayout))
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_IMAGE_LAYOUT pCreateInfo->"\
            "depthStencilLayout, is unrecognized (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_XGL_ATTACHMENT_LOAD_OP(pCreateInfo->depthLoadOp))
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_ATTACHMENT_LOAD_OP pCreateInfo->"\
            "depthLoadOp, is unrecognized (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_XGL_ATTACHMENT_STORE_OP(pCreateInfo->depthStoreOp))
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_ATTACHMENT_STORE_OP pCreateInfo->"\
            "depthStoreOp, is unrecognized (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_XGL_ATTACHMENT_LOAD_OP(pCreateInfo->stencilLoadOp))
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_ATTACHMENT_LOAD_OP pCreateInfo->"\
            "stencilLoadOp, is unrecognized (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_XGL_ATTACHMENT_STORE_OP(pCreateInfo->stencilStoreOp))
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_ATTACHMENT_STORE_OP pCreateInfo->"\
            "stencilStoreOp, is unrecognized (precondition).";
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateRenderPass(XGL_RESULT result, XGL_RENDER_PASS* pRenderPass)
{
    if(result != XGL_SUCCESS)
    {
        // TODO: Spit out XGL_RESULT value.
        char const str[] = "xglCreateRenderPass failed (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pRenderPass == nullptr)
    {
        char const str[] = "xglCreateRenderPass parameter, XGL_RENDER_PASS* pRenderPass, is nullptr (postcondition).";
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglCreateRenderPass(XGL_DEVICE device, const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo, XGL_RENDER_PASS* pRenderPass)
{
    PreCreateRenderPass(device, pCreateInfo);
    XGL_RESULT result = nextTable.CreateRenderPass(device, pCreateInfo, pRenderPass);
    PostCreateRenderPass(result, pRenderPass);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdBeginRenderPass(XGL_CMD_BUFFER cmdBuffer, const XGL_RENDER_PASS_BEGIN* pRenderPassBegin)
{
    char str[1024];
    if (!pRenderPassBegin) {
        sprintf(str, "Struct ptr parameter pRenderPassBegin to function CmdBeginRenderPass is NULL.");
        layerCbMsg(XGL_DBG_MSG_UNKNOWN, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!xgl_validate_xgl_render_pass_begin(pRenderPassBegin)) {
        sprintf(str, "Parameter pRenderPassBegin to function CmdBeginRenderPass contains an invalid value.");
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBeginRenderPass(cmdBuffer, pRenderPassBegin);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdEndRenderPass(XGL_CMD_BUFFER cmdBuffer, XGL_RENDER_PASS renderPass)
{

    nextTable.CmdEndRenderPass(cmdBuffer, renderPass);
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetValidationLevel(XGL_DEVICE device, XGL_VALIDATION_LEVEL validationLevel)
{
    char str[1024];
    if (!validate_XGL_VALIDATION_LEVEL(validationLevel)) {
        sprintf(str, "Parameter validationLevel to function DbgSetValidationLevel has invalid value of %i.", (int)validationLevel);
        layerCbMsg(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    XGL_RESULT result = nextTable.DbgSetValidationLevel(device, validationLevel);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_INSTANCE instance, XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    // This layer intercepts callbacks
    XGL_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = (XGL_LAYER_DBG_FUNCTION_NODE*)malloc(sizeof(XGL_LAYER_DBG_FUNCTION_NODE));
    if (!pNewDbgFuncNode)
        return XGL_ERROR_OUT_OF_MEMORY;
    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
    pNewDbgFuncNode->pUserData = pUserData;
    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;
    g_pDbgFunctionHead = pNewDbgFuncNode;
    // force callbacks if DebugAction hasn't been set already other than initial value
    if (g_actionIsDefault) {
        g_debugAction = XGL_DBG_LAYER_ACTION_CALLBACK;
    }
    XGL_RESULT result = nextTable.DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(XGL_INSTANCE instance, XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    XGL_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;
    XGL_LAYER_DBG_FUNCTION_NODE *pPrev = pTrav;
    while (pTrav) {
        if (pTrav->pfnMsgCallback == pfnMsgCallback) {
            pPrev->pNext = pTrav->pNext;
            if (g_pDbgFunctionHead == pTrav)
                g_pDbgFunctionHead = pTrav->pNext;
            free(pTrav);
            break;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }
    if (g_pDbgFunctionHead == NULL)
    {
        if (g_actionIsDefault)
            g_debugAction = XGL_DBG_LAYER_ACTION_LOG_MSG;
        else
            g_debugAction = (XGL_LAYER_DBG_ACTION)(g_debugAction & ~((uint32_t)XGL_DBG_LAYER_ACTION_CALLBACK));
    }
    XGL_RESULT result = nextTable.DbgUnregisterMsgCallback(instance, pfnMsgCallback);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetMessageFilter(XGL_DEVICE device, int32_t msgCode, XGL_DBG_MSG_FILTER filter)
{

    XGL_RESULT result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetObjectTag(XGL_BASE_OBJECT object, size_t tagSize, const void* pTag)
{

    XGL_RESULT result = nextTable.DbgSetObjectTag(object, tagSize, pTag);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_INSTANCE instance, XGL_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData)
{

    XGL_RESULT result = nextTable.DbgSetGlobalOption(instance, dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglDbgSetDeviceOption(XGL_DEVICE device, XGL_DBG_DEVICE_OPTION dbgOption, size_t dataSize, const void* pData)
{

    XGL_RESULT result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDbgMarkerBegin(XGL_CMD_BUFFER cmdBuffer, const char* pMarker)
{

    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

XGL_LAYER_EXPORT void XGLAPI xglCmdDbgMarkerEnd(XGL_CMD_BUFFER cmdBuffer)
{

    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

#if defined(__linux__) || defined(XCB_NVIDIA)

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11AssociateConnection(XGL_PHYSICAL_GPU gpu, const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    XGL_RESULT result = nextTable.WsiX11AssociateConnection((XGL_PHYSICAL_GPU)gpuw->nextObject, pConnectionInfo);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11GetMSC(XGL_DEVICE device, xcb_window_t window, xcb_randr_crtc_t crtc, uint64_t* pMsc)
{

    XGL_RESULT result = nextTable.WsiX11GetMSC(device, window, crtc, pMsc);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11CreatePresentableImage(XGL_DEVICE device, const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem)
{

    XGL_RESULT result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    return result;
}

XGL_LAYER_EXPORT XGL_RESULT XGLAPI xglWsiX11QueuePresent(XGL_QUEUE queue, const XGL_WSI_X11_PRESENT_INFO* pPresentInfo, XGL_FENCE fence)
{

    XGL_RESULT result = nextTable.WsiX11QueuePresent(queue, pPresentInfo, fence);
    return result;
}

#endif

#include "xgl_generic_intercept_proc_helper.h"
XGL_LAYER_EXPORT void* XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const char* funcName)
{
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    void* addr;
    if (gpu == NULL)
        return NULL;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    addr = layer_intercept_proc(funcName);
    if (addr)
        return addr;
    else {
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((XGL_PHYSICAL_GPU)gpuw->nextObject, funcName);
    }
}

