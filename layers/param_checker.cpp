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

static VkLayerDispatchTable nextTable;
static VkBaseLayerObject *pCurObj;
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

    PFN_vkGetProcAddr fpNextGPA;
    fpNextGPA = pCurObj->pGPA;
    assert(fpNextGPA);

    layer_initialize_dispatch_table(&nextTable, fpNextGPA, (VkPhysicalGpu) pCurObj->nextObject);
}

void PreCreateInstance(const VkApplicationInfo* pAppInfo, const VkAllocCallbacks* pAllocCb)
{
    if(pAppInfo == nullptr)
    {
        char const str[] = "vkCreateInstance parameter, VkApplicationInfo* pAppInfo, is "\
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
        if(!vk_validate_vkalloccallbacks(pAllocCb))
        {
            char const str[] = "vkCreateInstance parameter, VkAllocCallbacks* pAllocCb, "\
                "contains an invalid value (precondition).";
            layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
            return;
        }
    }
}

void PostCreateInstance(VkResult result, VkInstance* pInstance)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateInstance failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pInstance == nullptr)
    {
        char const str[] = "vkCreateInstance parameter, VkInstance* pInstance, is nullptr "\
            "(postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, VkInstance* pInstance)
{
    PreCreateInstance(pCreateInfo->pAppInfo, pCreateInfo->pAllocCb);
    VkResult result = nextTable.CreateInstance(pCreateInfo, pInstance);
    PostCreateInstance(result, pInstance);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyInstance(VkInstance instance)
{

    VkResult result = nextTable.DestroyInstance(instance);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateGpus(VkInstance instance, uint32_t maxGpus, uint32_t* pGpuCount, VkPhysicalGpu* pGpus)
{

    VkResult result = nextTable.EnumerateGpus(instance, maxGpus, pGpuCount, pGpus);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetGpuInfo(VkPhysicalGpu gpu, VkPhysicalGpuInfoType infoType, size_t* pDataSize, void* pData)
{
    pCurObj = (VkBaseLayerObject *) gpu;
    loader_platform_thread_once(&tabOnce, initParamChecker);
    char str[1024];
    if (!validate_VkPhysicalGpuInfoType(infoType)) {
        sprintf(str, "Parameter infoType to function GetGpuInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.GetGpuInfo(gpu, infoType, pDataSize, pData);
    return result;
}

void PreCreateDevice(VkPhysicalGpu gpu, const VkDeviceCreateInfo* pCreateInfo)
{
    if(gpu == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VkPhysicalGpu gpu, is nullptr "\
            "(precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VkDeviceCreateInfo* pCreateInfo, is "\
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
        char const str[] = "vkCreateDevice parameter, VkDeviceQueueCreateInfo* pCreateInfo->pRequestedQueues, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->queueRecordCount; ++i)
    {
        if(!vk_validate_vkdevicequeuecreateinfo(&(pCreateInfo->pRequestedQueues[i])))
        {
            std::stringstream ss;
            ss << "vkCreateDevice parameter, VkDeviceQueueCreateInfo pCreateInfo->pRequestedQueues[" << i <<
                "], is invalid (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(!validate_VkValidationLevel(pCreateInfo->maxValidationLevel))
    {
        char const str[] = "vkCreateDevice parameter, VkValidationLevel pCreateInfo->maxValidationLevel, is "\
            "unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateDevice(VkResult result, VkDevice* pDevice)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateDevice failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pDevice == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VkDevice* pDevice, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalGpu gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    pCurObj = (VkBaseLayerObject *) gpu;
    loader_platform_thread_once(&tabOnce, initParamChecker);
    PreCreateDevice(gpu, pCreateInfo);
    VkResult result = nextTable.CreateDevice(gpu, pCreateInfo, pDevice);
    PostCreateDevice(result, pDevice);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(VkDevice device)
{

    VkResult result = nextTable.DestroyDevice(device);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetExtensionSupport(VkPhysicalGpu gpu, const char* pExtName)
{
    pCurObj = (VkBaseLayerObject *) gpu;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    VkResult result = nextTable.GetExtensionSupport(gpu, pExtName);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateLayers(VkPhysicalGpu gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    char str[1024];
    if (gpu != NULL) {
        sprintf(str, "At start of layered EnumerateLayers\n");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, nullptr, 0, 0, "PARAMCHECK", str);
        pCurObj = (VkBaseLayerObject *) gpu;
        loader_platform_thread_once(&tabOnce, initParamChecker);
        VkResult result = nextTable.EnumerateLayers(gpu, maxLayerCount, maxStringSize, pOutLayerCount, pOutLayers, pReserved);
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

VK_LAYER_EXPORT VkResult VKAPI vkGetDeviceQueue(VkDevice device, uint32_t queueNodeIndex, uint32_t queueIndex, VkQueue* pQueue)
{

    VkResult result = nextTable.GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(VkQueue queue, uint32_t cmdBufferCount, const VkCmdBuffer* pCmdBuffers, VkFence fence)
{
    VkResult result = nextTable.QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueAddMemReference(VkQueue queue, VkGpuMemory mem)
{
    VkResult result = nextTable.QueueAddMemReference(queue, mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueRemoveMemReference(VkQueue queue, VkGpuMemory mem)
{
    VkResult result = nextTable.QueueRemoveMemReference(queue, mem);
    return result;
}
VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitIdle(VkQueue queue)
{

    VkResult result = nextTable.QueueWaitIdle(queue);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDeviceWaitIdle(VkDevice device)
{

    VkResult result = nextTable.DeviceWaitIdle(device);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocMemory(VkDevice device, const VkMemoryAllocInfo* pAllocInfo, VkGpuMemory* pMem)
{
    char str[1024];
    if (!pAllocInfo) {
        sprintf(str, "Struct ptr parameter pAllocInfo to function AllocMemory is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    } else if (!vk_validate_vkmemoryallocinfo(pAllocInfo)) {
        sprintf(str, "Parameter pAllocInfo to function AllocMemory contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.AllocMemory(device, pAllocInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkFreeMemory(VkGpuMemory mem)
{

    VkResult result = nextTable.FreeMemory(mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkSetMemoryPriority(VkGpuMemory mem, VkMemoryPriority priority)
{
    char str[1024];
    if (!validate_VkMemoryPriority(priority)) {
        sprintf(str, "Parameter priority to function SetMemoryPriority has invalid value of %i.", (int)priority);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.SetMemoryPriority(mem, priority);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkMapMemory(VkGpuMemory mem, VkFlags flags, void** ppData)
{

    VkResult result = nextTable.MapMemory(mem, flags, ppData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkUnmapMemory(VkGpuMemory mem)
{

    VkResult result = nextTable.UnmapMemory(mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkPinSystemMemory(VkDevice device, const void* pSysMem, size_t memSize, VkGpuMemory* pMem)
{

    VkResult result = nextTable.PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetMultiGpuCompatibility(VkPhysicalGpu gpu0, VkPhysicalGpu gpu1, VkGpuCompatibilityInfo* pInfo)
{
    pCurObj = (VkBaseLayerObject *) gpu0;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    VkResult result = nextTable.GetMultiGpuCompatibility(gpu0, gpu1, pInfo);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenSharedMemory(VkDevice device, const VkMemoryOpenInfo* pOpenInfo, VkGpuMemory* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenSharedMemory is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkmemoryopeninfo(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenSharedMemory contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenSharedSemaphore(VkDevice device, const VkSemaphoreOpenInfo* pOpenInfo, VkSemaphore* pSemaphore)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenSharedSemaphore is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vksemaphoreopeninfo(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenSharedSemaphore contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.OpenSharedSemaphore(device, pOpenInfo, pSemaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerMemory(VkDevice device, const VkPeerMemoryOpenInfo* pOpenInfo, VkGpuMemory* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenPeerMemory is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkpeermemoryopeninfo(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenPeerMemory contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerImage(VkDevice device, const VkPeerImageOpenInfo* pOpenInfo, VkImage* pImage, VkGpuMemory* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenPeerImage is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkpeerimageopeninfo(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenPeerImage contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyObject(VkObject object)
{

    VkResult result = nextTable.DestroyObject(object);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetObjectInfo(VkBaseObject object, VkObjectInfoType infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!validate_VkObjectInfoType(infoType)) {
        sprintf(str, "Parameter infoType to function GetObjectInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.GetObjectInfo(object, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBindObjectMemory(VkObject object, uint32_t allocationIdx, VkGpuMemory mem, VkGpuSize offset)
{

    VkResult result = nextTable.BindObjectMemory(object, allocationIdx, mem, offset);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBindObjectMemoryRange(VkObject object, uint32_t allocationIdx, VkGpuSize rangeOffset, VkGpuSize rangeSize, VkGpuMemory mem, VkGpuSize memOffset)
{

    VkResult result = nextTable.BindObjectMemoryRange(object, allocationIdx, rangeOffset, rangeSize, mem, memOffset);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBindImageMemoryRange(VkImage image, uint32_t allocationIdx, const VkImageMemoryBindInfo* bindInfo, VkGpuMemory mem, VkGpuSize memOffset)
{
    char str[1024];
    if (!bindInfo) {
        sprintf(str, "Struct ptr parameter bindInfo to function BindImageMemoryRange is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkimagememorybindinfo(bindInfo)) {
        sprintf(str, "Parameter bindInfo to function BindImageMemoryRange contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.BindImageMemoryRange(image, allocationIdx, bindInfo, mem, memOffset);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, VkFence* pFence)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateFence is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkfencecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateFence contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateFence(device, pCreateInfo, pFence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFenceStatus(VkFence fence)
{

    VkResult result = nextTable.GetFenceStatus(fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, bool32_t waitAll, uint64_t timeout)
{

    VkResult result = nextTable.WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetFences(VkDevice device, uint32_t fenceCount, VkFence* pFences)
{

    VkResult result = nextTable.ResetFences(device, fenceCount, pFences);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, VkSemaphore* pSemaphore)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateSemaphore is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vksemaphorecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateSemaphore contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateSemaphore(device, pCreateInfo, pSemaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSignalSemaphore(VkQueue queue, VkSemaphore semaphore)
{

    VkResult result = nextTable.QueueSignalSemaphore(queue, semaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitSemaphore(VkQueue queue, VkSemaphore semaphore)
{

    VkResult result = nextTable.QueueWaitSemaphore(queue, semaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, VkEvent* pEvent)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateEvent is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkeventcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateEvent contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateEvent(device, pCreateInfo, pEvent);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetEventStatus(VkEvent event)
{

    VkResult result = nextTable.GetEventStatus(event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkSetEvent(VkEvent event)
{

    VkResult result = nextTable.SetEvent(event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetEvent(VkEvent event)
{

    VkResult result = nextTable.ResetEvent(event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateQueryPool is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkquerypoolcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateQueryPool contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateQueryPool(device, pCreateInfo, pQueryPool);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetQueryPoolResults(VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData)
{

    VkResult result = nextTable.GetQueryPoolResults(queryPool, startQuery, queryCount, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFormatInfo(VkDevice device, VkFormat format, VkFormatInfoType infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!validate_VkFormat(format)) {
        sprintf(str, "Parameter format to function GetFormatInfo has invalid value of %i.", (int)format);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkFormatInfoType(infoType)) {
        sprintf(str, "Parameter infoType to function GetFormatInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateBuffer is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkbuffercreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateBuffer contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateBuffer(device, pCreateInfo, pBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateBufferView is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkbufferviewcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateBufferView contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateBufferView(device, pCreateInfo, pView);
    return result;
}

void PreCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo)
{
    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateImage parameter, VkImageCreateInfo* pCreateInfo, is "\
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

    if (!validate_VkImageType(pCreateInfo->imageType))
    {
        char const str[] = "vkCreateImage parameter, VkImageType pCreateInfo->imageType, is "\
            "unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_VkFormat(pCreateInfo->format))
    {
        char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, is "\
            "unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    VkFormatProperties properties;
    size_t size = sizeof(properties);
    VkResult result = nextTable.GetFormatInfo(device, pCreateInfo->format,
        VK_INFO_TYPE_FORMAT_PROPERTIES, &size, &properties);
    if(result != VK_SUCCESS)
    {
        char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, cannot be "\
            "validated (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
    {
        char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, contains "\
            "unsupported format (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    // TODO: Can we check device-specific limits?
    if (!vk_validate_vkextent3d(&pCreateInfo->extent))
    {
        char const str[] = "vkCreateImage parameter, VkExtent3D pCreateInfo->extent, is invalid "\
            "(precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_VkImageTiling(pCreateInfo->tiling))
    {
        char const str[] = "vkCreateImage parameter, VkImageTiling pCreateInfo->tiling, is "\
            "unrecoginized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateImage(VkResult result, VkImage* pImage)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateImage failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pImage == nullptr)
    {
        char const str[] = "vkCreateImage parameter, VkImage* pImage, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    PreCreateImage(device, pCreateInfo);
    VkResult result = nextTable.CreateImage(device, pCreateInfo, pImage);
    PostCreateImage(result, pImage);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetImageSubresourceInfo(VkImage image, const VkImageSubresource* pSubresource, VkSubresourceInfoType infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!pSubresource) {
        sprintf(str, "Struct ptr parameter pSubresource to function GetImageSubresourceInfo is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    } else if (!vk_validate_vkimagesubresource(pSubresource)) {
        sprintf(str, "Parameter pSubresource to function GetImageSubresourceInfo contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkSubresourceInfoType(infoType)) {
        sprintf(str, "Parameter infoType to function GetImageSubresourceInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.GetImageSubresourceInfo(image, pSubresource, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateImageView is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkimageviewcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateImageView contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateImageView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateColorAttachmentView(VkDevice device, const VkColorAttachmentViewCreateInfo* pCreateInfo, VkColorAttachmentView* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateColorAttachmentView is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkcolorattachmentviewcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateColorAttachmentView contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateColorAttachmentView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDepthStencilView(VkDevice device, const VkDepthStencilViewCreateInfo* pCreateInfo, VkDepthStencilView* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDepthStencilView is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdepthstencilviewcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDepthStencilView contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateDepthStencilView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateShader(VkDevice device, const VkShaderCreateInfo* pCreateInfo, VkShader* pShader)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateShader is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkshadercreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateShader contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateShader(device, pCreateInfo, pShader);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateGraphicsPipeline is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkgraphicspipelinecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateGraphicsPipeline contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelineDerivative(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline basePipeline, VkPipeline* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateGraphicsPipelineDerivative is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkgraphicspipelinecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateGraphicsPipelineDerivative contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateGraphicsPipelineDerivative(device, pCreateInfo, basePipeline, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateComputePipeline(VkDevice device, const VkComputePipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateComputePipeline is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkcomputepipelinecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateComputePipeline contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateComputePipeline(device, pCreateInfo, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkStorePipeline(VkPipeline pipeline, size_t* pDataSize, void* pData)
{

    VkResult result = nextTable.StorePipeline(pipeline, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkLoadPipeline(VkDevice device, size_t dataSize, const void* pData, VkPipeline* pPipeline)
{

    VkResult result = nextTable.LoadPipeline(device, dataSize, pData, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkLoadPipelineDerivative(VkDevice device, size_t dataSize, const void* pData, VkPipeline basePipeline, VkPipeline* pPipeline)
{

    VkResult result = nextTable.LoadPipelineDerivative(device, dataSize, pData, basePipeline, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateSampler is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vksamplercreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateSampler contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateSampler(device, pCreateInfo, pSampler);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDescriptorSetLayout is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdescriptorsetlayoutcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDescriptorSetLayout contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayoutChain(VkDevice device, uint32_t setLayoutArrayCount, const VkDescriptorSetLayout* pSetLayoutArray, VkDescriptorSetLayoutChain* pLayoutChain)
{

    VkResult result = nextTable.CreateDescriptorSetLayoutChain(device, setLayoutArrayCount, pSetLayoutArray, pLayoutChain);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginDescriptorPoolUpdate(VkDevice device, VkDescriptorUpdateMode updateMode)
{
    char str[1024];
    if (!validate_VkDescriptorUpdateMode(updateMode)) {
        sprintf(str, "Parameter updateMode to function BeginDescriptorPoolUpdate has invalid value of %i.", (int)updateMode);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.BeginDescriptorPoolUpdate(device, updateMode);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndDescriptorPoolUpdate(VkDevice device, VkCmdBuffer cmd)
{

    VkResult result = nextTable.EndDescriptorPoolUpdate(device, cmd);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolUsage poolUsage, uint32_t maxSets, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool)
{
    char str[1024];
    if (!validate_VkDescriptorPoolUsage(poolUsage)) {
        sprintf(str, "Parameter poolUsage to function CreateDescriptorPool has invalid value of %i.", (int)poolUsage);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDescriptorPool is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdescriptorpoolcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDescriptorPool contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetDescriptorPool(VkDescriptorPool descriptorPool)
{

    VkResult result = nextTable.ResetDescriptorPool(descriptorPool);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetUsage setUsage, uint32_t count, const VkDescriptorSetLayout* pSetLayouts, VkDescriptorSet* pDescriptorSets, uint32_t* pCount)
{
    char str[1024];
    if (!validate_VkDescriptorSetUsage(setUsage)) {
        sprintf(str, "Parameter setUsage to function AllocDescriptorSets has invalid value of %i.", (int)setUsage);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.AllocDescriptorSets(descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkClearDescriptorSets(VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{

    nextTable.ClearDescriptorSets(descriptorPool, count, pDescriptorSets);
}

VK_LAYER_EXPORT void VKAPI vkUpdateDescriptors(VkDescriptorSet descriptorSet, uint32_t updateCount, const void** ppUpdateArray)
{

    nextTable.UpdateDescriptors(descriptorSet, updateCount, ppUpdateArray);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(VkDevice device, const VkDynamicVpStateCreateInfo* pCreateInfo, VkDynamicVpState* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicViewportState is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdynamicvpstatecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicViewportState contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateDynamicViewportState(device, pCreateInfo, pState);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicRasterState(VkDevice device, const VkDynamicRsStateCreateInfo* pCreateInfo, VkDynamicRsState* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicRasterState is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdynamicrsstatecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicRasterState contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateDynamicRasterState(device, pCreateInfo, pState);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(VkDevice device, const VkDynamicCbStateCreateInfo* pCreateInfo, VkDynamicCbState* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicColorBlendState is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdynamiccbstatecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicColorBlendState contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateDynamicColorBlendState(device, pCreateInfo, pState);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(VkDevice device, const VkDynamicDsStateCreateInfo* pCreateInfo, VkDynamicDsState* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicDepthStencilState is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdynamicdsstatecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicDepthStencilState contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    return result;
}

void PreCreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VkDevice device, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VkCmdBufferCreateInfo* pCreateInfo, is "\
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

void PostCreateCommandBuffer(VkResult result, VkCmdBuffer* pCmdBuffer)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateCommandBuffer failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCmdBuffer == nullptr)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VkCmdBuffer* pCmdBuffer, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandBuffer(VkDevice device,
    const VkCmdBufferCreateInfo* pCreateInfo, VkCmdBuffer* pCmdBuffer)
{
    PreCreateCommandBuffer(device, pCreateInfo);
    VkResult result = nextTable.CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    PostCreateCommandBuffer(result, pCmdBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo)
{
    char str[1024];
    if (!pBeginInfo) {
        sprintf(str, "Struct ptr parameter pBeginInfo to function BeginCommandBuffer is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkcmdbufferbegininfo(pBeginInfo)) {
        sprintf(str, "Parameter pBeginInfo to function BeginCommandBuffer contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.BeginCommandBuffer(cmdBuffer, pBeginInfo);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(VkCmdBuffer cmdBuffer)
{

    VkResult result = nextTable.EndCommandBuffer(cmdBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(VkCmdBuffer cmdBuffer)
{

    VkResult result = nextTable.ResetCommandBuffer(cmdBuffer);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdBindPipeline has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicStateObject(VkCmdBuffer cmdBuffer, VkStateBindPoint stateBindPoint, VkDynamicStateObject state)
{
    char str[1024];
    if (!validate_VkStateBindPoint(stateBindPoint)) {
        sprintf(str, "Parameter stateBindPoint to function CmdBindDynamicStateObject has invalid value of %i.", (int)stateBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkDescriptorSetLayoutChain layoutChain, uint32_t layoutChainSlot, uint32_t count, const VkDescriptorSet* pDescriptorSets, const uint32_t* pUserData)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdBindDescriptorSets has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layoutChain, layoutChainSlot, count, pDescriptorSets, pUserData);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffer(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkGpuSize offset, uint32_t binding)
{

    nextTable.CmdBindVertexBuffer(cmdBuffer, buffer, offset, binding);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkGpuSize offset, VkIndexType indexType)
{
    char str[1024];
    if (!validate_VkIndexType(indexType)) {
        sprintf(str, "Parameter indexType to function CmdBindIndexBuffer has invalid value of %i.", (int)indexType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(VkCmdBuffer cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{

    nextTable.CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(VkCmdBuffer cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{

    nextTable.CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkGpuSize offset, uint32_t count, uint32_t stride)
{

    nextTable.CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkGpuSize offset, uint32_t count, uint32_t stride)
{

    nextTable.CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(VkCmdBuffer cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{

    nextTable.CmdDispatch(cmdBuffer, x, y, z);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkGpuSize offset)
{

    nextTable.CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer destBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    char str[1024];
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkbuffercopy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyBuffer contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
    char str[1024];
    if (!validate_VkImageLayout(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCopyImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkImageLayout(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCopyImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkimagecopy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageBlit* pRegions)
{
    char str[1024];
    if (!validate_VkImageLayout(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdBlitImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkImageLayout(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdBlitImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkimageblit(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdBlitImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    char str[1024];
    if (!validate_VkImageLayout(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCopyBufferToImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkbufferimagecopy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyBufferToImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer destBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    char str[1024];
    if (!validate_VkImageLayout(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCopyImageToBuffer has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkbufferimagecopy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyImageToBuffer contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCloneImageData(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout)
{
    char str[1024];
    if (!validate_VkImageLayout(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCloneImageData has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkImageLayout(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCloneImageData has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdCloneImageData(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkGpuSize destOffset, VkGpuSize dataSize, const uint32_t* pData)
{

    nextTable.CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkGpuSize destOffset, VkGpuSize fillSize, uint32_t data)
{

    nextTable.CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, VkClearColor color, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    char str[1024];
    if (!validate_VkImageLayout(imageLayout)) {
        sprintf(str, "Parameter imageLayout to function CmdClearColorImage has invalid value of %i.", (int)imageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rangeCount; i++) {
        if (!vk_validate_vkimagesubresourcerange(&pRanges[i])) {
            sprintf(str, "Parameter pRanges[%i] to function CmdClearColorImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdClearColorImage(cmdBuffer, image, imageLayout, color, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencil(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    char str[1024];
    if (!validate_VkImageLayout(imageLayout)) {
        sprintf(str, "Parameter imageLayout to function CmdClearDepthStencil has invalid value of %i.", (int)imageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rangeCount; i++) {
        if (!vk_validate_vkimagesubresourcerange(&pRanges[i])) {
            sprintf(str, "Parameter pRanges[%i] to function CmdClearDepthStencil contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t rectCount, const VkImageResolve* pRects)
{
    char str[1024];
    if (!validate_VkImageLayout(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdResolveImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkImageLayout(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdResolveImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rectCount; i++) {
        if (!vk_validate_vkimageresolve(&pRects[i])) {
            sprintf(str, "Parameter pRects[%i] to function CmdResolveImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    nextTable.CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, rectCount, pRects);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent)
{
    char str[1024];
    if (!validate_VkPipeEvent(pipeEvent)) {
        sprintf(str, "Parameter pipeEvent to function CmdSetEvent has invalid value of %i.", (int)pipeEvent);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdSetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent)
{
    char str[1024];
    if (!validate_VkPipeEvent(pipeEvent)) {
        sprintf(str, "Parameter pipeEvent to function CmdResetEvent has invalid value of %i.", (int)pipeEvent);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdResetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(VkCmdBuffer cmdBuffer, const VkEventWaitInfo* pWaitInfo)
{
    char str[1024];
    if (!pWaitInfo) {
        sprintf(str, "Struct ptr parameter pWaitInfo to function CmdWaitEvents is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkeventwaitinfo(pWaitInfo)) {
        sprintf(str, "Parameter pWaitInfo to function CmdWaitEvents contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdWaitEvents(cmdBuffer, pWaitInfo);
}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(VkCmdBuffer cmdBuffer, const VkPipelineBarrier* pBarrier)
{
    char str[1024];
    if (!pBarrier) {
        sprintf(str, "Struct ptr parameter pBarrier to function CmdPipelineBarrier is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkpipelinebarrier(pBarrier)) {
        sprintf(str, "Parameter pBarrier to function CmdPipelineBarrier contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdPipelineBarrier(cmdBuffer, pBarrier);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
{

    nextTable.CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot)
{

    nextTable.CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount)
{

    nextTable.CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(VkCmdBuffer cmdBuffer, VkTimestampType timestampType, VkBuffer destBuffer, VkGpuSize destOffset)
{
    char str[1024];
    if (!validate_VkTimestampType(timestampType)) {
        sprintf(str, "Parameter timestampType to function CmdWriteTimestamp has invalid value of %i.", (int)timestampType);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdInitAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdInitAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdLoadAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer srcBuffer, VkGpuSize srcOffset)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdLoadAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdSaveAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer destBuffer, VkGpuSize destOffset)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdSaveAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateFramebuffer is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkframebuffercreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateFramebuffer contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    return result;
}


void PreCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo)
{
    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkRenderPassCreateInfo* pCreateInfo, is "\
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

    if(!vk_validate_vkrect(&pCreateInfo->renderArea))
    {
        char const str[] = "vkCreateRenderPass parameter, VkRect pCreateInfo->renderArea, is invalid "\
            "(precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!vk_validate_vkextent2d(&pCreateInfo->extent))
    {
        char const str[] = "vkCreateRenderPass parameter, VkExtent2D pCreateInfo->extent, is invalid "\
            "(precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pColorFormats == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkFormat* pCreateInfo->pColorFormats, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkFormat(pCreateInfo->pColorFormats[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkFormat pCreateInfo->pColorFormats[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

        VkFormatProperties properties;
        size_t size = sizeof(properties);
        VkResult result = nextTable.GetFormatInfo(device, pCreateInfo->pColorFormats[i],
            VK_INFO_TYPE_FORMAT_PROPERTIES, &size, &properties);
        if(result != VK_SUCCESS)
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkFormat pCreateInfo->pColorFormats[" << i <<
                "], cannot be validated (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

        if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkFormat pCreateInfo->pColorFormats[" << i <<
                "], contains unsupported format (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

    }

    if(pCreateInfo->pColorLayouts == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkImageLayout* pCreateInfo->pColorLayouts, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkImageLayout(pCreateInfo->pColorLayouts[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pColorLayouts[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadOps == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentLoadOp* pCreateInfo->pColorLoadOps, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkAttachmentLoadOp(pCreateInfo->pColorLoadOps[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->pColorLoadOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorStoreOps == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentStoreOp* pCreateInfo->pColorStoreOps, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkAttachmentStoreOp(pCreateInfo->pColorStoreOps[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->pColorStoreOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadClearValues == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkClearColor* pCreateInfo->"\
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
        if(!vk_validate_vkclearcolor(&(pCreateInfo->pColorLoadClearValues[i])))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkClearColor pCreateInfo->pColorLoadClearValues[" << i <<
                "], is invalid (precondition).";
            layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(!validate_VkFormat(pCreateInfo->depthStencilFormat))
    {
        char const str[] = "vkCreateRenderPass parameter, VkFormat pCreateInfo->"\
            "depthStencilFormat, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    VkFormatProperties properties;
    size_t size = sizeof(properties);
    VkResult result = nextTable.GetFormatInfo(device, pCreateInfo->depthStencilFormat,
        VK_INFO_TYPE_FORMAT_PROPERTIES, &size, &properties);
    if(result != VK_SUCCESS)
    {
        char const str[] = "vkCreateRenderPass parameter, VkFormat pCreateInfo->"\
            "depthStencilFormat, cannot be validated (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
    {
        char const str[] = "vkCreateRenderPass parameter, VkFormat pCreateInfo->"\
            "depthStencilFormat, contains unsupported format (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkImageLayout(pCreateInfo->depthStencilLayout))
    {
        char const str[] = "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->"\
            "depthStencilLayout, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkAttachmentLoadOp(pCreateInfo->depthLoadOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->"\
            "depthLoadOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkAttachmentStoreOp(pCreateInfo->depthStoreOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->"\
            "depthStoreOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkAttachmentLoadOp(pCreateInfo->stencilLoadOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->"\
            "stencilLoadOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkAttachmentStoreOp(pCreateInfo->stencilStoreOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->"\
            "stencilStoreOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateRenderPass(VkResult result, VkRenderPass* pRenderPass)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateRenderPass failed (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pRenderPass == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkRenderPass* pRenderPass, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    PreCreateRenderPass(device, pCreateInfo);
    VkResult result = nextTable.CreateRenderPass(device, pCreateInfo, pRenderPass);
    PostCreateRenderPass(result, pRenderPass);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(VkCmdBuffer cmdBuffer, const VkRenderPassBegin* pRenderPassBegin)
{
    char str[1024];
    if (!pRenderPassBegin) {
        sprintf(str, "Struct ptr parameter pRenderPassBegin to function CmdBeginRenderPass is NULL.");
        layerCbMsg(VK_DBG_MSG_UNKNOWN, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkrenderpassbegin(pRenderPassBegin)) {
        sprintf(str, "Parameter pRenderPassBegin to function CmdBeginRenderPass contains an invalid value.");
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    nextTable.CmdBeginRenderPass(cmdBuffer, pRenderPassBegin);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(VkCmdBuffer cmdBuffer, VkRenderPass renderPass)
{

    nextTable.CmdEndRenderPass(cmdBuffer, renderPass);
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetValidationLevel(VkDevice device, VkValidationLevel validationLevel)
{
    char str[1024];
    if (!validate_VkValidationLevel(validationLevel)) {
        sprintf(str, "Parameter validationLevel to function DbgSetValidationLevel has invalid value of %i.", (int)validationLevel);
        layerCbMsg(VK_DBG_MSG_ERROR, VK_VALIDATION_LEVEL_0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = nextTable.DbgSetValidationLevel(device, validationLevel);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgRegisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
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
    VkResult result = nextTable.DbgRegisterMsgCallback(instance, pfnMsgCallback, pUserData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgUnregisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
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
    VkResult result = nextTable.DbgUnregisterMsgCallback(instance, pfnMsgCallback);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetMessageFilter(VkDevice device, int32_t msgCode, VK_DBG_MSG_FILTER filter)
{

    VkResult result = nextTable.DbgSetMessageFilter(device, msgCode, filter);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetObjectTag(VkBaseObject object, size_t tagSize, const void* pTag)
{

    VkResult result = nextTable.DbgSetObjectTag(object, tagSize, pTag);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetGlobalOption(VkInstance instance, VK_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData)
{

    VkResult result = nextTable.DbgSetGlobalOption(instance, dbgOption, dataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgSetDeviceOption(VkDevice device, VK_DBG_DEVICE_OPTION dbgOption, size_t dataSize, const void* pData)
{

    VkResult result = nextTable.DbgSetDeviceOption(device, dbgOption, dataSize, pData);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerBegin(VkCmdBuffer cmdBuffer, const char* pMarker)
{

    nextTable.CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerEnd(VkCmdBuffer cmdBuffer)
{

    nextTable.CmdDbgMarkerEnd(cmdBuffer);
}

#if defined(__linux__) || defined(XCB_NVIDIA)

VK_LAYER_EXPORT VkResult VKAPI vkWsiX11AssociateConnection(VkPhysicalGpu gpu, const VK_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    pCurObj = (VkBaseLayerObject *) gpu;
    loader_platform_thread_once(&tabOnce, initParamChecker);

    VkResult result = nextTable.WsiX11AssociateConnection(gpu, pConnectionInfo);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWsiX11GetMSC(VkDevice device, xcb_window_t window, xcb_randr_crtc_t crtc, uint64_t* pMsc)
{

    VkResult result = nextTable.WsiX11GetMSC(device, window, crtc, pMsc);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWsiX11CreatePresentableImage(VkDevice device, const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, VkImage* pImage, VkGpuMemory* pMem)
{

    VkResult result = nextTable.WsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWsiX11QueuePresent(VkQueue queue, const VK_WSI_X11_PRESENT_INFO* pPresentInfo, VkFence fence)
{

    VkResult result = nextTable.WsiX11QueuePresent(queue, pPresentInfo, fence);
    return result;
}

#endif

#include "vk_generic_intercept_proc_helper.h"
VK_LAYER_EXPORT void* VKAPI vkGetProcAddr(VkPhysicalGpu gpu, const char* funcName)
{
    VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
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
        return gpuw->pGPA((VkPhysicalGpu)gpuw->nextObject, funcName);
    }
}

