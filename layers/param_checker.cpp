/*
 * Vulkan
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
#include "vkLayer.h"
#include "layers_config.h"
#include "vk_enum_validate_helper.h"
#include "vk_struct_validate_helper.h"
//The following is #included again to catch certain OS-specific functions being used:
#include "loader_platform.h"

#include "layers_msg.h"

static VK_LAYER_DISPATCH_TABLE nextTable;
static VK_BASE_LAYER_OBJECT *pCurObj;
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(tabOnce);

#include "vk_dispatch_table_helper.h"
static void initParamChecker(void)
{

    const char *strOpt;
    // initialize ParamChecker options
    getLayerOptionEnum("ParamCheckerReportLevel", (uint32_t *) &g_reportingLevel);
    g_actionIsDefault = getLayerOptionEnum("ParamCheckerDebugAction", (uint32_t *) &g_debugAction);

    if (g_debugAction & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        strOpt = getLayerOption("ParamCheckerLogFilename");
        if (strOpt)
        {
            g_logFile = fopen(strOpt, "w");
        }
        if (g_logFile == NULL)
            g_logFile = stdout;
    }

    vkGetProcAddrType fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VK_PHYSICAL_GPU) pCurObj->nextObject);
}

void PreCreateInstance(const VK_APPLICATION_INFO* pAppInfo, const VK_ALLOC_CALLBACKS* pAllocCb)
{
    if(pAppInfo == nullptr)
    {
        char const str[] = "vkCreateInstance parameter, VK_APPLICATION_INFO* pAppInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pAppInfo->sType != VK_STRUCTURE_TYPE_APPLICATION_INFO)
    {
        char const str[] = "vkCreateInstance parameter, VK_STRUCTURE_TYPE_APPLICATION_INFO "\
            "pAppInfo->sType, is not VK_STRUCTURE_TYPE_APPLICATION_INFO (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    // TODO: What else can validated in pAppInfo?
    // TODO: VK_API_VERSION validation.

    // It's okay if pAllocCb is a nullptr.
    if(pAllocCb != nullptr)
    {
        if(!vk_validate_vk_alloc_callbacks(pAllocCb))
        {
            char const str[] = "vkCreateInstance parameter, VK_ALLOC_CALLBACKS* pAllocCb, "\
                "contains an invalid value (precondition).";
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
            return;
        }
    }
}

void PostCreateInstance(VK_RESULT result, VK_INSTANCE* pInstance)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VK_RESULT value.
        char const str[] = "vkCreateInstance failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pInstance == nullptr)
    {
        char const str[] = "vkCreateInstance parameter, VK_INSTANCE* pInstance, is nullptr "\
            "(postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateInstance(const VK_INSTANCE_CREATE_INFO* pCreateInfo, VK_INSTANCE* pInstance)
{
    PreCreateInstance(pCreateInfo->pAppInfo, pCreateInfo->pAllocCb);
    VK_RESULT result = nextTable.CreateInstance(pCreateInfo, pInstance);
    PostCreateInstance(result, pInstance);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDestroyInstance(VK_INSTANCE instance)
{

    VK_RESULT result = nextTable.DestroyInstance(instance);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkEnumerateGpus(VK_INSTANCE instance, uint32_t maxGpus, uint32_t* pGpuCount, VK_PHYSICAL_GPU* pGpus)
{

    VK_RESULT result = nextTable.EnumerateGpus(instance, maxGpus, pGpuCount, pGpus);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetGpuInfo(VK_PHYSICAL_GPU gpu, VK_PHYSICAL_GPU_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);
    char str[1024];
    if (!validate_VK_PHYSICAL_GPU_INFO_TYPE(infoType)) {
        sprintf(str, "Parameter infoType to function GetGpuInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.GetGpuInfo((VK_PHYSICAL_GPU)gpuw->nextObject, infoType, pDataSize, pData);
    return result;
}

void PreCreateDevice(VK_PHYSICAL_GPU gpu, const VK_DEVICE_CREATE_INFO* pCreateInfo)
{
    if(gpu == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VK_PHYSICAL_GPU gpu, is nullptr "\
            "(precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VK_DEVICE_CREATE_INFO* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType !=  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
    {
        char const str[] = "vkCreateDevice parameter, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO "\
            "pCreateInfo->sType, is not VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->queueRecordCount == 0)
    {
        char const str[] = "vkCreateDevice parameter, uint32_t pCreateInfo->queueRecordCount, is "\
            "zero (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pRequestedQueues == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VK_DEVICE_QUEUE_CREATE_INFO* pCreateInfo->pRequestedQueues, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->queueRecordCount; ++i)
    {
        if(!vk_validate_vk_device_queue_create_info(&(pCreateInfo->pRequestedQueues[i])))
        {
            std::stringstream ss;
            ss << "vkCreateDevice parameter, VK_DEVICE_QUEUE_CREATE_INFO pCreateInfo->pRequestedQueues[" << i <<
                "], is invalid (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(!validate_VK_VALIDATION_LEVEL(pCreateInfo->maxValidationLevel))
    {
        char const str[] = "vkCreateDevice parameter, VK_VALIDATION_LEVEL pCreateInfo->maxValidationLevel, is "\
            "unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateDevice(VK_RESULT result, VK_DEVICE* pDevice)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VK_RESULT value.
        char const str[] = "vkCreateDevice failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pDevice == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VK_DEVICE* pDevice, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDevice(VK_PHYSICAL_GPU gpu, const VK_DEVICE_CREATE_INFO* pCreateInfo, VK_DEVICE* pDevice)
{
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);
    PreCreateDevice(gpu, pCreateInfo);
    VK_RESULT result = nextTable.CreateDevice((VK_PHYSICAL_GPU)gpuw->nextObject, pCreateInfo, pDevice);
    PostCreateDevice(result, pDevice);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDestroyDevice(VK_DEVICE device)
{

    VK_RESULT result = nextTable.DestroyDevice(device);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetExtensionSupport(VK_PHYSICAL_GPU gpu, const char* pExtName)
{
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    VK_RESULT result = nextTable.GetExtensionSupport((VK_PHYSICAL_GPU)gpuw->nextObject, pExtName);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkEnumerateLayers(VK_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    char str[1024];
    if (gpu != NULL) {
        VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
        sprintf(str, "At start of layered EnumerateLayers\n");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, nullptr, 0, 0, "PARAMCHECK", str);
        pCurObj = gpuw;
        loader_platform_thread_once(&tabOnce, initParamChecker);
        VK_RESULT result = nextTable.EnumerateLayers((VK_PHYSICAL_GPU)gpuw->nextObject, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
        sprintf(str, "Completed layered EnumerateLayers\n");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, nullptr, 0, 0, "PARAMCHECK", str);
        fflush(stdout);
        return result;
    } else {
        if (pOutLayerCount == NULL || pOutLayers == NULL || pOutLayers[0] == NULL)
            return VK_ERROR_INVALID_POINTER;
        // This layer compatible with all GPUs
        *pOutLayerCount = 1;
        strncpy(pOutLayers[0], "ParamChecker", maxStringSize);
        return VK_SUCCESS;
    }
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetDeviceQueue(VK_DEVICE device, uint32_t queueNodeIndex, uint32_t queueIndex, VK_QUEUE* pQueue)
{

    VK_RESULT result = nextTable.GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkQueueSubmit(VK_QUEUE queue, uint32_t cmdBufferCount, const VK_CMD_BUFFER* pCmdBuffers, VK_FENCE fence)
{
    VK_RESULT result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkQueueAddMemReference(VK_QUEUE queue, VK_GPU_MEMORY mem)
{
    VK_RESULT result = nextTable.QueueAddMemReference(queue, mem);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkQueueRemoveMemReference(VK_QUEUE queue, VK_GPU_MEMORY mem)
{
    VK_RESULT result = nextTable.QueueRemoveMemReference(queue, mem);
    return result;
}
VK_LAYER_EXPORT VK_RESULT VKAPI vkQueueWaitIdle(VK_QUEUE queue)
{

    VK_RESULT result = nextTable.QueueWaitIdle(queue);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDeviceWaitIdle(VK_DEVICE device)
{

    VK_RESULT result = nextTable.DeviceWaitIdle(device);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkAllocMemory(VK_DEVICE device, const VK_MEMORY_ALLOC_INFO* pAllocInfo, VK_GPU_MEMORY* pMem)
{
    char str[1024];
    if (!pAllocInfo) {
        sprintf(str, "Struct ptr parameter pAllocInfo to function AllocMemory is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_memory_alloc_info(pAllocInfo)) {
        sprintf(str, "Parameter pAllocInfo to function AllocMemory contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.AllocMemory(device, pAllocInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkFreeMemory(VK_GPU_MEMORY mem)
{

    VK_RESULT result = nextTable.FreeMemory(mem);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkSetMemoryPriority(VK_GPU_MEMORY mem, VK_MEMORY_PRIORITY priority)
{
    char str[1024];
    if (!validate_VK_MEMORY_PRIORITY(priority)) {
        sprintf(str, "Parameter priority to function SetMemoryPriority has invalid value of %i.", (int)priority);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.SetMemoryPriority(mem, priority);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkMapMemory(VK_GPU_MEMORY mem, VK_FLAGS flags, void** ppData)
{

    VK_RESULT result = nextTable.MapMemory(mem, flags, ppData);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkUnmapMemory(VK_GPU_MEMORY mem)
{

    VK_RESULT result = nextTable.UnmapMemory(mem);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkPinSystemMemory(VK_DEVICE device, const void* pSysMem, size_t memSize, VK_GPU_MEMORY* pMem)
{

    VK_RESULT result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetMultiGpuCompatibility(VK_PHYSICAL_GPU gpu0, VK_PHYSICAL_GPU gpu1, VK_GPU_COMPATIBILITY_INFO* pInfo)
{
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu0;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    VK_RESULT result = nextTable.GetMultiGpuCompatibility((VK_PHYSICAL_GPU)gpuw->nextObject, gpu1, pInfo);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkOpenSharedMemory(VK_DEVICE device, const VK_MEMORY_OPEN_INFO* pOpenInfo, VK_GPU_MEMORY* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenSharedMemory is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_memory_open_info(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenSharedMemory contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkOpenSharedSemaphore(VK_DEVICE device, const VK_SEMAPHORE_OPEN_INFO* pOpenInfo, VK_SEMAPHORE* pSemaphore)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenSharedSemaphore is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_semaphore_open_info(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenSharedSemaphore contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.OpenSharedSemaphore(device, pOpenInfo, pSemaphore);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkOpenPeerMemory(VK_DEVICE device, const VK_PEER_MEMORY_OPEN_INFO* pOpenInfo, VK_GPU_MEMORY* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenPeerMemory is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_peer_memory_open_info(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenPeerMemory contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkOpenPeerImage(VK_DEVICE device, const VK_PEER_IMAGE_OPEN_INFO* pOpenInfo, VK_IMAGE* pImage, VK_GPU_MEMORY* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenPeerImage is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_peer_image_open_info(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenPeerImage contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDestroyObject(VK_OBJECT object)
{

    VK_RESULT result = nextTable.DestroyObject(object);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetObjectInfo(VK_BASE_OBJECT object, VK_OBJECT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!validate_VK_OBJECT_INFO_TYPE(infoType)) {
        sprintf(str, "Parameter infoType to function GetObjectInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkBindObjectMemory(VK_OBJECT object, uint32_t allocationIdx, VK_GPU_MEMORY mem, VK_GPU_SIZE offset)
{

    VK_RESULT result = nextTable.BindObjectMemory(object, allocationIdx, mem, offset);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkBindObjectMemoryRange(VK_OBJECT object, uint32_t allocationIdx, VK_GPU_SIZE rangeOffset, VK_GPU_SIZE rangeSize, VK_GPU_MEMORY mem, VK_GPU_SIZE memOffset)
{

    VK_RESULT result = nextTable.BindObjectMemoryRange(object, allocationIdx, rangeOffset, rangeSize, mem, memOffset);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkBindImageMemoryRange(VK_IMAGE image, uint32_t allocationIdx, const VK_IMAGE_MEMORY_BIND_INFO* bindInfo, VK_GPU_MEMORY mem, VK_GPU_SIZE memOffset)
{
    char str[1024];
    if (!bindInfo) {
        sprintf(str, "Struct ptr parameter bindInfo to function BindImageMemoryRange is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_image_memory_bind_info(bindInfo)) {
        sprintf(str, "Parameter bindInfo to function BindImageMemoryRange contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.BindImageMemoryRange(image, allocationIdx, bindInfo, mem, memOffset);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateFence(VK_DEVICE device, const VK_FENCE_CREATE_INFO* pCreateInfo, VK_FENCE* pFence)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateFence is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_fence_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateFence contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateFence(device, pCreateInfo, pFence);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetFenceStatus(VK_FENCE fence)
{

    VK_RESULT result = nextTable.GetFenceStatus(fence);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkWaitForFences(VK_DEVICE device, uint32_t fenceCount, const VK_FENCE* pFences, bool32_t waitAll, uint64_t timeout)
{

    VK_RESULT result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkResetFences(VK_DEVICE device, uint32_t fenceCount, VK_FENCE* pFences)
{

    VK_RESULT result = nextTable.ResetFences(device, fenceCount, pFences);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateSemaphore(VK_DEVICE device, const VK_SEMAPHORE_CREATE_INFO* pCreateInfo, VK_SEMAPHORE* pSemaphore)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateSemaphore is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_semaphore_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateSemaphore contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateSemaphore(device, pCreateInfo, pSemaphore);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkQueueSignalSemaphore(VK_QUEUE queue, VK_SEMAPHORE semaphore)
{

    VK_RESULT result = nextTable.QueueSignalSemaphore(queue, semaphore);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkQueueWaitSemaphore(VK_QUEUE queue, VK_SEMAPHORE semaphore)
{

    VK_RESULT result = nextTable.QueueWaitSemaphore(queue, semaphore);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateEvent(VK_DEVICE device, const VK_EVENT_CREATE_INFO* pCreateInfo, VK_EVENT* pEvent)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateEvent is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_event_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateEvent contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetEventStatus(VK_EVENT event)
{

    VK_RESULT result = nextTable.GetEventStatus(event);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkSetEvent(VK_EVENT event)
{

    VK_RESULT result = nextTable.SetEvent(event);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkResetEvent(VK_EVENT event)
{

    VK_RESULT result = nextTable.ResetEvent(event);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateQueryPool(VK_DEVICE device, const VK_QUERY_POOL_CREATE_INFO* pCreateInfo, VK_QUERY_POOL* pQueryPool)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateQueryPool is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_query_pool_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateQueryPool contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetQueryPoolResults(VK_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData)
{

    VK_RESULT result = nextTable.GetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetFormatInfo(VK_DEVICE device, VK_FORMAT format, VK_FORMAT_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!validate_VK_FORMAT(format)) {
        sprintf(str, "Parameter format to function GetFormatInfo has invalid value of %i.", (int)format);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VK_FORMAT_INFO_TYPE(infoType)) {
        sprintf(str, "Parameter infoType to function GetFormatInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateBuffer(VK_DEVICE device, const VK_BUFFER_CREATE_INFO* pCreateInfo, VK_BUFFER* pBuffer)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateBuffer is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_buffer_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateBuffer contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateBuffer(device, pCreateInfo, pBuffer);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateBufferView(VK_DEVICE device, const VK_BUFFER_VIEW_CREATE_INFO* pCreateInfo, VK_BUFFER_VIEW* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateBufferView is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_buffer_view_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateBufferView contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    return result;
}

void PreCreateImage(VK_DEVICE device, const VK_IMAGE_CREATE_INFO* pCreateInfo)
{
    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateImage parameter, VK_IMAGE_CREATE_INFO* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
    {
        char const str[] = "vkCreateImage parameter, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO "\
            "pCreateInfo->sType, is not VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_VK_IMAGE_TYPE(pCreateInfo->imageType))
    {
        char const str[] = "vkCreateImage parameter, VK_IMAGE_TYPE pCreateInfo->imageType, is "\
            "unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_VK_FORMAT(pCreateInfo->format))
    {
        char const str[] = "vkCreateImage parameter, VK_FORMAT pCreateInfo->format, is "\
            "unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    VK_FORMAT_PROPERTIES properties;
    size_t size = sizeof(properties);
    VK_RESULT result = nextTable.GetFormatInfo(device, pCreateInfo->format,
        VK_INFO_TYPE_FORMAT_PROPERTIES, &size, &properties);
    if(result != VK_SUCCESS)
    {
        char const str[] = "vkCreateImage parameter, VK_FORMAT pCreateInfo->format, cannot be "\
            "validated (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
    {
        char const str[] = "vkCreateImage parameter, VK_FORMAT pCreateInfo->format, contains "\
            "unsupported format (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    // TODO: Can we check device-specific limits?
    if (!vk_validate_vk_extent3d(&pCreateInfo->extent))
    {
        char const str[] = "vkCreateImage parameter, VK_EXTENT3D pCreateInfo->extent, is invalid "\
            "(precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_VK_IMAGE_TILING(pCreateInfo->tiling))
    {
        char const str[] = "vkCreateImage parameter, VK_IMAGE_TILING pCreateInfo->tiling, is "\
            "unrecoginized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateImage(VK_RESULT result, VK_IMAGE* pImage)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VK_RESULT value.
        char const str[] = "vkCreateImage failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pImage == nullptr)
    {
        char const str[] = "vkCreateImage parameter, VK_IMAGE* pImage, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateImage(VK_DEVICE device, const VK_IMAGE_CREATE_INFO* pCreateInfo, VK_IMAGE* pImage)
{
    PreCreateImage(device, pCreateInfo);
    VK_RESULT result = nextTable.CreateImage(device, pCreateInfo, pImage);
    PostCreateImage(result, pImage);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkGetImageSubresourceInfo(VK_IMAGE image, const VK_IMAGE_SUBRESOURCE* pSubresource, VK_SUBRESOURCE_INFO_TYPE infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!pSubresource) {
        sprintf(str, "Struct ptr parameter pSubresource to function GetImageSubresourceInfo is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_image_subresource(pSubresource)) {
        sprintf(str, "Parameter pSubresource to function GetImageSubresourceInfo contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VK_SUBRESOURCE_INFO_TYPE(infoType)) {
        sprintf(str, "Parameter infoType to function GetImageSubresourceInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.GetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateImageView(VK_DEVICE device, const VK_IMAGE_VIEW_CREATE_INFO* pCreateInfo, VK_IMAGE_VIEW* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateImageView is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_image_view_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateImageView contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateImageView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateColorAttachmentView(VK_DEVICE device, const VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, VK_COLOR_ATTACHMENT_VIEW* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateColorAttachmentView is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_color_attachment_view_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateColorAttachmentView contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDepthStencilView(VK_DEVICE device, const VK_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, VK_DEPTH_STENCIL_VIEW* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDepthStencilView is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_depth_stencil_view_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDepthStencilView contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateShader(VK_DEVICE device, const VK_SHADER_CREATE_INFO* pCreateInfo, VK_SHADER* pShader)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateShader is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_shader_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateShader contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateShader(device, pCreateInfo, pShader);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateGraphicsPipeline(VK_DEVICE device, const VK_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, VK_PIPELINE* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateGraphicsPipeline is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_graphics_pipeline_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateGraphicsPipeline contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateGraphicsPipelineDerivative(VK_DEVICE device, const VK_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, VK_PIPELINE basePipeline, VK_PIPELINE* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateGraphicsPipelineDerivative is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_graphics_pipeline_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateGraphicsPipelineDerivative contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateGraphicsPipelineDerivative(device, pCreateInfo, basePipeline, pPipeline);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateComputePipeline(VK_DEVICE device, const VK_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, VK_PIPELINE* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateComputePipeline is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_compute_pipeline_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateComputePipeline contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkStorePipeline(VK_PIPELINE pipeline, size_t* pDataSize, void* pData)
{

    VK_RESULT result = nextTable.StorePipeline(pipeline, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkLoadPipeline(VK_DEVICE device, size_t dataSize, const void* pData, VK_PIPELINE* pPipeline)
{

    VK_RESULT result = nextTable.LoadPipeline(device, dataSize, pData, pPipeline);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkLoadPipelineDerivative(VK_DEVICE device, size_t dataSize, const void* pData, VK_PIPELINE basePipeline, VK_PIPELINE* pPipeline)
{

    VK_RESULT result = nextTable.LoadPipelineDerivative(device, dataSize, pData, basePipeline, pPipeline);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateSampler(VK_DEVICE device, const VK_SAMPLER_CREATE_INFO* pCreateInfo, VK_SAMPLER* pSampler)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateSampler is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_sampler_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateSampler contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDescriptorSetLayout(VK_DEVICE device, const VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pCreateInfo, VK_DESCRIPTOR_SET_LAYOUT* pSetLayout)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDescriptorSetLayout is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_descriptor_set_layout_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDescriptorSetLayout contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDescriptorSetLayoutChain(VK_DEVICE device, uint32_t setLayoutArrayCount, const VK_DESCRIPTOR_SET_LAYOUT* pSetLayoutArray, VK_DESCRIPTOR_SET_LAYOUT_CHAIN* pLayoutChain)
{

    VK_RESULT result = nextTable.CreateDescriptorSetLayoutChain(device, setLayoutArrayCount, pSetLayoutArray, pLayoutChain);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkBeginDescriptorPoolUpdate(VK_DEVICE device, VK_DESCRIPTOR_UPDATE_MODE updateMode)
{
    char str[1024];
    if (!validate_VK_DESCRIPTOR_UPDATE_MODE(updateMode)) {
        sprintf(str, "Parameter updateMode to function BeginDescriptorPoolUpdate has invalid value of %i.", (int)updateMode);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.BeginDescriptorPoolUpdate(device, updateMode);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkEndDescriptorPoolUpdate(VK_DEVICE device, VK_CMD_BUFFER cmd)
{

    VK_RESULT result = nextTable.EndDescriptorPoolUpdate(device, cmd);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDescriptorPool(VK_DEVICE device, VK_DESCRIPTOR_POOL_USAGE poolUsage, uint32_t maxSets, const VK_DESCRIPTOR_POOL_CREATE_INFO* pCreateInfo, VK_DESCRIPTOR_POOL* pDescriptorPool)
{
    char str[1024];
    if (!validate_VK_DESCRIPTOR_POOL_USAGE(poolUsage)) {
        sprintf(str, "Parameter poolUsage to function CreateDescriptorPool has invalid value of %i.", (int)poolUsage);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDescriptorPool is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_descriptor_pool_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDescriptorPool contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkResetDescriptorPool(VK_DESCRIPTOR_POOL descriptorPool)
{

    VK_RESULT result = nextTable.ResetDescriptorPool(descriptorPool);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkAllocDescriptorSets(VK_DESCRIPTOR_POOL descriptorPool, VK_DESCRIPTOR_SET_USAGE setUsage, uint32_t count, const VK_DESCRIPTOR_SET_LAYOUT* pSetLayouts, VK_DESCRIPTOR_SET* pDescriptorSets, uint32_t* pCount)
{
    char str[1024];
    if (!validate_VK_DESCRIPTOR_SET_USAGE(setUsage)) {
        sprintf(str, "Parameter setUsage to function AllocDescriptorSets has invalid value of %i.", (int)setUsage);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.AllocDescriptorSets(descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkClearDescriptorSets(VK_DESCRIPTOR_POOL descriptorPool, uint32_t count, const VK_DESCRIPTOR_SET* pDescriptorSets)
{

    nextTable.ClearDescriptorSets(descriptorPool, count, pDescriptorSets);
}

VK_LAYER_EXPORT void VKAPI vkUpdateDescriptors(VK_DESCRIPTOR_SET descriptorSet, uint32_t updateCount, const void** ppUpdateArray)
{

    nextTable.UpdateDescriptors(descriptorSet, updateCount, ppUpdateArray);
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDynamicViewportState(VK_DEVICE device, const VK_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_VP_STATE_OBJECT* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicViewportState is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_dynamic_vp_state_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicViewportState contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateDynamicViewportState(device, pCreateInfo, pState);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDynamicRasterState(VK_DEVICE device, const VK_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_RS_STATE_OBJECT* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicRasterState is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_dynamic_rs_state_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicRasterState contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateDynamicRasterState(device, pCreateInfo, pState);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDynamicColorBlendState(VK_DEVICE device, const VK_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_CB_STATE_OBJECT* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicColorBlendState is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_dynamic_cb_state_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicColorBlendState contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateDynamicColorBlendState(device, pCreateInfo, pState);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateDynamicDepthStencilState(VK_DEVICE device, const VK_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_DS_STATE_OBJECT* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicDepthStencilState is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_dynamic_ds_state_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicDepthStencilState contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    return result;
}

void PreCreateCommandBuffer(VK_DEVICE device, const VK_CMD_BUFFER_CREATE_INFO* pCreateInfo)
{
    if(device == nullptr)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VK_DEVICE device, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VK_CMD_BUFFER_CREATE_INFO* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO "\
            "pCreateInfo->sType, is not VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateCommandBuffer(VK_RESULT result, VK_CMD_BUFFER* pCmdBuffer)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VK_RESULT value.
        char const str[] = "vkCreateCommandBuffer failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCmdBuffer == nullptr)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VK_CMD_BUFFER* pCmdBuffer, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateCommandBuffer(VK_DEVICE device,
    const VK_CMD_BUFFER_CREATE_INFO* pCreateInfo, VK_CMD_BUFFER* pCmdBuffer)
{
    PreCreateCommandBuffer(device, pCreateInfo);
    VK_RESULT result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    PostCreateCommandBuffer(result, pCmdBuffer);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkBeginCommandBuffer(VK_CMD_BUFFER cmdBuffer, const VK_CMD_BUFFER_BEGIN_INFO* pBeginInfo)
{
    char str[1024];
    if (!pBeginInfo) {
        sprintf(str, "Struct ptr parameter pBeginInfo to function BeginCommandBuffer is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_cmd_buffer_begin_info(pBeginInfo)) {
        sprintf(str, "Parameter pBeginInfo to function BeginCommandBuffer contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkEndCommandBuffer(VK_CMD_BUFFER cmdBuffer)
{

    VK_RESULT result = nextTable.EndCommandBuffer(cmdBuffer);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkResetCommandBuffer(VK_CMD_BUFFER cmdBuffer)
{

    VK_RESULT result = nextTable.ResetCommandBuffer(cmdBuffer);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, VK_PIPELINE pipeline)
{
    char str[1024];
    if (!validate_VK_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdBindPipeline has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicStateObject(VK_CMD_BUFFER cmdBuffer, VK_STATE_BIND_POINT stateBindPoint, VK_DYNAMIC_STATE_OBJECT state)
{
    char str[1024];
    if (!validate_VK_STATE_BIND_POINT(stateBindPoint)) {
        sprintf(str, "Parameter stateBindPoint to function CmdBindDynamicStateObject has invalid value of %i.", (int)stateBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, VK_DESCRIPTOR_SET_LAYOUT_CHAIN layoutChain, uint32_t layoutChainSlot, uint32_t count, const VK_DESCRIPTOR_SET* pDescriptorSets, const uint32_t* pUserData)
{
    char str[1024];
    if (!validate_VK_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdBindDescriptorSets has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layoutChain, layoutChainSlot, count, pDescriptorSets, pUserData);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, uint32_t binding)
{

    nextTable.CmdBindVertexBuffer(cmdBuffer, buffer, offset, binding);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, VK_INDEX_TYPE indexType)
{
    char str[1024];
    if (!validate_VK_INDEX_TYPE(indexType)) {
        sprintf(str, "Parameter indexType to function CmdBindIndexBuffer has invalid value of %i.", (int)indexType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(VK_CMD_BUFFER cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{

    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(VK_CMD_BUFFER cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{

    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, uint32_t count, uint32_t stride)
{

    nextTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, uint32_t count, uint32_t stride)
{

    nextTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(VK_CMD_BUFFER cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{

    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset)
{

    nextTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER srcBuffer, VK_BUFFER destBuffer, uint32_t regionCount, const VK_BUFFER_COPY* pRegions)
{
    char str[1024];
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vk_buffer_copy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyBuffer contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout, uint32_t regionCount, const VK_IMAGE_COPY* pRegions)
{
    char str[1024];
    if (!validate_VK_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCopyImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VK_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCopyImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vk_image_copy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout, uint32_t regionCount, const VK_IMAGE_BLIT* pRegions)
{
    char str[1024];
    if (!validate_VK_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdBlitImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VK_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdBlitImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vk_image_blit(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdBlitImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(VK_CMD_BUFFER cmdBuffer, VK_BUFFER srcBuffer, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout, uint32_t regionCount, const VK_BUFFER_IMAGE_COPY* pRegions)
{
    char str[1024];
    if (!validate_VK_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCopyBufferToImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vk_buffer_image_copy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyBufferToImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_BUFFER destBuffer, uint32_t regionCount, const VK_BUFFER_IMAGE_COPY* pRegions)
{
    char str[1024];
    if (!validate_VK_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCopyImageToBuffer has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vk_buffer_image_copy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyImageToBuffer contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCloneImageData(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout)
{
    char str[1024];
    if (!validate_VK_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCloneImageData has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VK_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCloneImageData has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset, VK_GPU_SIZE dataSize, const uint32_t* pData)
{

    nextTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(VK_CMD_BUFFER cmdBuffer, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset, VK_GPU_SIZE fillSize, uint32_t data)
{

    nextTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(VK_CMD_BUFFER cmdBuffer, VK_IMAGE image, VK_IMAGE_LAYOUT imageLayout, VK_CLEAR_COLOR color, uint32_t rangeCount, const VK_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    char str[1024];
    if (!validate_VK_IMAGE_LAYOUT(imageLayout)) {
        sprintf(str, "Parameter imageLayout to function CmdClearColorImage has invalid value of %i.", (int)imageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rangeCount; i++) {
        if (!vk_validate_vk_image_subresource_range(&pRanges[i])) {
            sprintf(str, "Parameter pRanges[%i] to function CmdClearColorImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdClearColorImage(cmdBuffer, image, imageLayout, color, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencil(VK_CMD_BUFFER cmdBuffer, VK_IMAGE image, VK_IMAGE_LAYOUT imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const VK_IMAGE_SUBRESOURCE_RANGE* pRanges)
{
    char str[1024];
    if (!validate_VK_IMAGE_LAYOUT(imageLayout)) {
        sprintf(str, "Parameter imageLayout to function CmdClearDepthStencil has invalid value of %i.", (int)imageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rangeCount; i++) {
        if (!vk_validate_vk_image_subresource_range(&pRanges[i])) {
            sprintf(str, "Parameter pRanges[%i] to function CmdClearDepthStencil contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout, uint32_t rectCount, const VK_IMAGE_RESOLVE* pRects)
{
    char str[1024];
    if (!validate_VK_IMAGE_LAYOUT(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdResolveImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VK_IMAGE_LAYOUT(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdResolveImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rectCount; i++) {
        if (!vk_validate_vk_image_resolve(&pRects[i])) {
            sprintf(str, "Parameter pRects[%i] to function CmdResolveImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, rectCount, pRects);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(VK_CMD_BUFFER cmdBuffer, VK_EVENT event, VK_PIPE_EVENT pipeEvent)
{
    char str[1024];
    if (!validate_VK_PIPE_EVENT(pipeEvent)) {
        sprintf(str, "Parameter pipeEvent to function CmdSetEvent has invalid value of %i.", (int)pipeEvent);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdSetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(VK_CMD_BUFFER cmdBuffer, VK_EVENT event, VK_PIPE_EVENT pipeEvent)
{
    char str[1024];
    if (!validate_VK_PIPE_EVENT(pipeEvent)) {
        sprintf(str, "Parameter pipeEvent to function CmdResetEvent has invalid value of %i.", (int)pipeEvent);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdResetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(VK_CMD_BUFFER cmdBuffer, const VK_EVENT_WAIT_INFO* pWaitInfo)
{
    char str[1024];
    if (!pWaitInfo) {
        sprintf(str, "Struct ptr parameter pWaitInfo to function CmdWaitEvents is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_event_wait_info(pWaitInfo)) {
        sprintf(str, "Parameter pWaitInfo to function CmdWaitEvents contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdWaitEvents(cmdBuffer, pWaitInfo);
}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(VK_CMD_BUFFER cmdBuffer, const VK_PIPELINE_BARRIER* pBarrier)
{
    char str[1024];
    if (!pBarrier) {
        sprintf(str, "Struct ptr parameter pBarrier to function CmdPipelineBarrier is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_pipeline_barrier(pBarrier)) {
        sprintf(str, "Parameter pBarrier to function CmdPipelineBarrier contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdPipelineBarrier(cmdBuffer, pBarrier);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(VK_CMD_BUFFER cmdBuffer, VK_QUERY_POOL queryPool, uint32_t slot, VK_FLAGS flags)
{

    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(VK_CMD_BUFFER cmdBuffer, VK_QUERY_POOL queryPool, uint32_t slot)
{

    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(VK_CMD_BUFFER cmdBuffer, VK_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount)
{

    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(VK_CMD_BUFFER cmdBuffer, VK_TIMESTAMP_TYPE timestampType, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset)
{
    char str[1024];
    if (!validate_VK_TIMESTAMP_TYPE(timestampType)) {
        sprintf(str, "Parameter timestampType to function CmdWriteTimestamp has invalid value of %i.", (int)timestampType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdInitAtomicCounters(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    char str[1024];
    if (!validate_VK_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdInitAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdLoadAtomicCounters(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VK_BUFFER srcBuffer, VK_GPU_SIZE srcOffset)
{
    char str[1024];
    if (!validate_VK_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdLoadAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdSaveAtomicCounters(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset)
{
    char str[1024];
    if (!validate_VK_PIPELINE_BIND_POINT(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdSaveAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateFramebuffer(VK_DEVICE device, const VK_FRAMEBUFFER_CREATE_INFO* pCreateInfo, VK_FRAMEBUFFER* pFramebuffer)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateFramebuffer is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_framebuffer_create_info(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateFramebuffer contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    return result;
}


void PreCreateRenderPass(VK_DEVICE device, const VK_RENDER_PASS_CREATE_INFO* pCreateInfo)
{
    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_RENDER_PASS_CREATE_INFO* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO "\
            "pCreateInfo->sType, is not VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!vk_validate_vk_rect(&pCreateInfo->renderArea))
    {
        char const str[] = "vkCreateRenderPass parameter, VK_RECT pCreateInfo->renderArea, is invalid "\
            "(precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!vk_validate_vk_extent2d(&pCreateInfo->extent))
    {
        char const str[] = "vkCreateRenderPass parameter, VK_EXTENT2D pCreateInfo->extent, is invalid "\
            "(precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pColorFormats == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_FORMAT* pCreateInfo->pColorFormats, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VK_FORMAT(pCreateInfo->pColorFormats[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VK_FORMAT pCreateInfo->pColorFormats[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

        VK_FORMAT_PROPERTIES properties;
        size_t size = sizeof(properties);
        VK_RESULT result = nextTable.GetFormatInfo(device, pCreateInfo->pColorFormats[i],
            VK_INFO_TYPE_FORMAT_PROPERTIES, &size, &properties);
        if(result != VK_SUCCESS)
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VK_FORMAT pCreateInfo->pColorFormats[" << i <<
                "], cannot be validated (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

        if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VK_FORMAT pCreateInfo->pColorFormats[" << i <<
                "], contains unsupported format (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

    }

    if(pCreateInfo->pColorLayouts == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_IMAGE_LAYOUT* pCreateInfo->pColorLayouts, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VK_IMAGE_LAYOUT(pCreateInfo->pColorLayouts[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VK_IMAGE_LAYOUT pCreateInfo->pColorLayouts[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadOps == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_ATTACHMENT_LOAD_OP* pCreateInfo->pColorLoadOps, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VK_ATTACHMENT_LOAD_OP(pCreateInfo->pColorLoadOps[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VK_ATTACHMENT_LOAD_OP pCreateInfo->pColorLoadOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorStoreOps == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_ATTACHMENT_STORE_OP* pCreateInfo->pColorStoreOps, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VK_ATTACHMENT_STORE_OP(pCreateInfo->pColorStoreOps[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VK_ATTACHMENT_STORE_OP pCreateInfo->pColorStoreOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadClearValues == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_CLEAR_COLOR* pCreateInfo->"\
            "pColorLoadClearValues, is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pColorStoreOps == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_ATTACHMENT_STORE_OP* pCreateInfo->pColorStoreOps, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VK_ATTACHMENT_STORE_OP(pCreateInfo->pColorStoreOps[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VK_ATTACHMENT_STORE_OP pCreateInfo->pColorStoreOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadClearValues == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_CLEAR_COLOR* pCreateInfo->"\
            "pColorLoadClearValues, is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
   
    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!vk_validate_vk_clear_color(&(pCreateInfo->pColorLoadClearValues[i])))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VK_CLEAR_COLOR pCreateInfo->pColorLoadClearValues[" << i <<
                "], is invalid (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(!validate_VK_FORMAT(pCreateInfo->depthStencilFormat))
    {
        char const str[] = "vkCreateRenderPass parameter, VK_FORMAT pCreateInfo->"\
            "depthStencilFormat, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    VK_FORMAT_PROPERTIES properties;
    size_t size = sizeof(properties);
    VK_RESULT result = nextTable.GetFormatInfo(device, pCreateInfo->depthStencilFormat,
        VK_INFO_TYPE_FORMAT_PROPERTIES, &size, &properties);
    if(result != VK_SUCCESS)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_FORMAT pCreateInfo->"\
            "depthStencilFormat, cannot be validated (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
    {
        char const str[] = "vkCreateRenderPass parameter, VK_FORMAT pCreateInfo->"\
            "depthStencilFormat, contains unsupported format (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VK_IMAGE_LAYOUT(pCreateInfo->depthStencilLayout))
    {
        char const str[] = "vkCreateRenderPass parameter, VK_IMAGE_LAYOUT pCreateInfo->"\
            "depthStencilLayout, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VK_ATTACHMENT_LOAD_OP(pCreateInfo->depthLoadOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VK_ATTACHMENT_LOAD_OP pCreateInfo->"\
            "depthLoadOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VK_ATTACHMENT_STORE_OP(pCreateInfo->depthStoreOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VK_ATTACHMENT_STORE_OP pCreateInfo->"\
            "depthStoreOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VK_ATTACHMENT_LOAD_OP(pCreateInfo->stencilLoadOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VK_ATTACHMENT_LOAD_OP pCreateInfo->"\
            "stencilLoadOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VK_ATTACHMENT_STORE_OP(pCreateInfo->stencilStoreOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VK_ATTACHMENT_STORE_OP pCreateInfo->"\
            "stencilStoreOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateRenderPass(VK_RESULT result, VK_RENDER_PASS* pRenderPass)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VK_RESULT value.
        char const str[] = "vkCreateRenderPass failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pRenderPass == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_RENDER_PASS* pRenderPass, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkCreateRenderPass(VK_DEVICE device, const VK_RENDER_PASS_CREATE_INFO* pCreateInfo, VK_RENDER_PASS* pRenderPass)
{
    PreCreateRenderPass(device, pCreateInfo);
    VK_RESULT result = nextTable.CreateRenderPass(device, pCreateInfo, pRenderPass);
    PostCreateRenderPass(result, pRenderPass);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(VK_CMD_BUFFER cmdBuffer, const VK_RENDER_PASS_BEGIN* pRenderPassBegin)
{
    char str[1024];
    if (!pRenderPassBegin) {
        sprintf(str, "Struct ptr parameter pRenderPassBegin to function CmdBeginRenderPass is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vk_render_pass_begin(pRenderPassBegin)) {
        sprintf(str, "Parameter pRenderPassBegin to function CmdBeginRenderPass contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBeginRenderPass(cmdBuffer, pRenderPassBegin);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(VK_CMD_BUFFER cmdBuffer, VK_RENDER_PASS renderPass)
{

    nextTable.CmdEndRenderPass(cmdBuffer, renderPass);
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDbgSetValidationLevel(VK_DEVICE device, VK_VALIDATION_LEVEL validationLevel)
{
    char str[1024];
    if (!validate_VK_VALIDATION_LEVEL(validationLevel)) {
        sprintf(str, "Parameter validationLevel to function DbgSetValidationLevel has invalid value of %i.", (int)validationLevel);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VK_RESULT result = nextTable.DbgSetValidationLevel(device, validationLevel);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDbgRegisterMsgCallback(VK_INSTANCE instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    // This layer intercepts callbacks
    VK_LAYER_DBG_FUNCTION_NODE *pNewDbgFuncNode = (VK_LAYER_DBG_FUNCTION_NODE*)malloc(sizeof(VK_LAYER_DBG_FUNCTION_NODE));
    if (!pNewDbgFuncNode)
        return VK_ERROR_OUT_OF_MEMORY;
    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
    pNewDbgFuncNode->pUserData = pUserData;
    pNewDbgFuncNode->pNext = g_pDbgFunctionHead;
    g_pDbgFunctionHead = pNewDbgFuncNode;
    // force callbacks if DebugAction hasn't been set already other than initial value
    if (g_actionIsDefault) {
        g_debugAction = VK_DBG_LAYER_ACTION_CALLBACK;
    }
    VK_RESULT result = nextTable.DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDbgUnregisterMsgCallback(VK_INSTANCE instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    VK_LAYER_DBG_FUNCTION_NODE *pTrav = g_pDbgFunctionHead;
    VK_LAYER_DBG_FUNCTION_NODE *pPrev = pTrav;
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
            g_debugAction = VK_DBG_LAYER_ACTION_LOG_MSG;
        else
            g_debugAction = (VK_LAYER_DBG_ACTION)(g_debugAction & ~((uint32_t)VK_DBG_LAYER_ACTION_CALLBACK));
    }
    VK_RESULT result = nextTable.DbgUnregisterMsgCallback(instance, pfnMsgCallback);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDbgSetMessageFilter(VK_DEVICE device, int32_t msgCode, VK_DBG_MSG_FILTER filter)
{

    VK_RESULT result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDbgSetObjectTag(VK_BASE_OBJECT object, size_t tagSize, const void* pTag)
{

    VK_RESULT result = nextTable.DbgSetObjectTag(object, tagSize, pTag);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDbgSetGlobalOption(VK_INSTANCE instance, VK_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData)
{

    VK_RESULT result = nextTable.DbgSetGlobalOption(instance, dbgOption, dataSize, pData);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkDbgSetDeviceOption(VK_DEVICE device, VK_DBG_DEVICE_OPTION dbgOption, size_t dataSize, const void* pData)
{

    VK_RESULT result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerBegin(VK_CMD_BUFFER cmdBuffer, const char* pMarker)
{

    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerEnd(VK_CMD_BUFFER cmdBuffer)
{

    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

#if defined(__linux__) || defined(XCB_NVIDIA)

VK_LAYER_EXPORT VK_RESULT VKAPI vkWsiX11AssociateConnection(VK_PHYSICAL_GPU gpu, const VK_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
    pCurObj = gpuw;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    VK_RESULT result = nextTable.WsiX11AssociateConnection((VK_PHYSICAL_GPU)gpuw->nextObject, pConnectionInfo);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkWsiX11GetMSC(VK_DEVICE device, xcb_window_t window, xcb_randr_crtc_t crtc, uint64_t* pMsc)
{

    VK_RESULT result = nextTable.WsiX11GetMSC(device, window, crtc, pMsc);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkWsiX11CreatePresentableImage(VK_DEVICE device, const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, VK_IMAGE* pImage, VK_GPU_MEMORY* pMem)
{

    VK_RESULT result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    return result;
}

VK_LAYER_EXPORT VK_RESULT VKAPI vkWsiX11QueuePresent(VK_QUEUE queue, const VK_WSI_X11_PRESENT_INFO* pPresentInfo, VK_FENCE fence)
{

    VK_RESULT result = nextTable.WsiX11QueuePresent(queue, pPresentInfo, fence);
    return result;
}

#endif

#include "vk_generic_intercept_proc_helper.h"
VK_LAYER_EXPORT void* VKAPI vkGetProcAddr(VK_PHYSICAL_GPU gpu, const char* funcName)
{
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
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
        return gpuw->pGPA((VK_PHYSICAL_GPU)gpuw->nextObject, funcName);
    }
}

