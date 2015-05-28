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
#include <unordered_map>

#include "loader_platform.h"
#include "vkLayer.h"
#include "vk_debug_marker_layer.h"
#include "layers_config.h"
#include "vk_enum_validate_helper.h"
#include "vk_struct_validate_helper.h"
//The following is #included again to catch certain OS-specific functions being used:
#include "loader_platform.h"

#include "layers_msg.h"

static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(initOnce);

static std::unordered_map<void *, VkLayerDispatchTable *>            tableMap;
static std::unordered_map<void *, VkLayerDebugMarkerDispatchTable *> tableDebugMarkerMap;
static std::unordered_map<void *, VkLayerInstanceDispatchTable *>    tableInstanceMap;

static inline VkLayerDispatchTable *device_dispatch_table(VkObject object) {
    VkLayerDispatchTable *pDisp  = *(VkLayerDispatchTable **) object;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    return pTable;
}

static inline VkLayerInstanceDispatchTable *instance_dispatch_table(VkObject object) {
    VkLayerInstanceDispatchTable **ppDisp    = (VkLayerInstanceDispatchTable **) object;
    VkLayerInstanceDispatchTable *pInstanceTable = tableInstanceMap[*ppDisp];
    return pInstanceTable;
}


#include "vk_dispatch_table_helper.h"

static VkLayerDispatchTable * initDeviceTable(const VkBaseLayerObject *devw)
{
    VkLayerDispatchTable *pTable;
    VkLayerDebugMarkerDispatchTable *pDebugMarkerTable;

    assert(devw);
    VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) (devw->baseObject);

    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap.find((void *) *ppDisp);
    if (it == tableMap.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap[(void *) *ppDisp] = pTable;
        pDebugMarkerTable = new VkLayerDebugMarkerDispatchTable;
        tableDebugMarkerMap[(void *) *ppDisp] = pDebugMarkerTable;
    } else
    {
        return it->second;
    }

    layer_initialize_dispatch_table(pTable, devw);

    VkDevice device = (VkDevice) devw->baseObject;
    pDebugMarkerTable->CmdDbgMarkerBegin = (PFN_vkCmdDbgMarkerBegin) devw->pGPA(device, "vkCmdDbgMarkerBegin");
    pDebugMarkerTable->CmdDbgMarkerEnd   = (PFN_vkCmdDbgMarkerEnd) devw->pGPA(device, "vkCmdDbgMarkerEnd");
    pDebugMarkerTable->DbgSetObjectTag   = (PFN_vkDbgSetObjectTag) devw->pGPA(device, "vkDbgSetObjectTag");
    pDebugMarkerTable->DbgSetObjectName  = (PFN_vkDbgSetObjectName) devw->pGPA(device, "vkDbgSetObjectName");
    pDebugMarkerTable->ext_enabled       = false;

    return pTable;
}

static VkLayerInstanceDispatchTable * initInstanceTable(const VkBaseLayerObject *instw)
{
    VkLayerInstanceDispatchTable *pTable;
    assert(instw);
    VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instw->baseObject;

    std::unordered_map<void *, VkLayerInstanceDispatchTable *>::const_iterator it = tableInstanceMap.find((void *) *ppDisp);
    if (it == tableInstanceMap.end())
    {
        pTable =  new VkLayerInstanceDispatchTable;
        tableInstanceMap[(void *) *ppDisp] = pTable;
    } else
    {
        return it->second;
    }

    layer_init_instance_dispatch_table(pTable, instw);

    return pTable;
}

static void initParamChecker(void)
{

    const char *strOpt;
    // initialize ParamChecker options
    getLayerOptionEnum("ParamCheckerReportLevel", (uint32_t *) &g_reportFlags);
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
}

void PreCreateInstance(const VkApplicationInfo* pAppInfo, const VkAllocCallbacks* pAllocCb)
{
    if(pAppInfo == nullptr)
    {
        char const str[] = "vkCreateInstance parameter, VkApplicationInfo* pAppInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pAppInfo->sType != VK_STRUCTURE_TYPE_APPLICATION_INFO)
    {
        char const str[] = "vkCreateInstance parameter, VK_STRUCTURE_TYPE_APPLICATION_INFO "\
            "pAppInfo->sType, is not VK_STRUCTURE_TYPE_APPLICATION_INFO (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
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
            layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
            return;
        }
    }
}

void PostCreateInstance(VkResult result, const VkInstanceCreateInfo *pCreateInfo, VkInstance* pInstance)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateInstance failed (postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    enable_debug_report(pCreateInfo->extensionCount, pCreateInfo->pEnabledExtensions);

    if(pInstance == nullptr)
    {
        char const str[] = "vkCreateInstance parameter, VkInstance* pInstance, is nullptr "\
            "(postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, VkInstance* pInstance)
{
    PreCreateInstance(pCreateInfo->pAppInfo, pCreateInfo->pAllocCb);
    VkResult result = instance_dispatch_table(*pInstance)->CreateInstance(pCreateInfo, pInstance);
    PostCreateInstance(result, pCreateInfo, pInstance);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyInstance(VkInstance instance)
{
    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) instance;
    VkResult res = instance_dispatch_table(instance)->DestroyInstance(instance);
    tableInstanceMap.erase(pDisp);
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceInfo(VkPhysicalDevice gpu, VkPhysicalDeviceInfoType infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!validate_VkPhysicalDeviceInfoType(infoType)) {
        sprintf(str, "Parameter infoType to function GetPhysicalDeviceInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = instance_dispatch_table(gpu)->GetPhysicalDeviceInfo(gpu, infoType, pDataSize, pData);
    return result;
}

void PreCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo)
{
    if(gpu == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VkPhysicalDevice gpu, is nullptr "\
            "(precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VkDeviceCreateInfo* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType !=  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO)
    {
        char const str[] = "vkCreateDevice parameter, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO "\
            "pCreateInfo->sType, is not VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->queueRecordCount == 0)
    {
        char const str[] = "vkCreateDevice parameter, uint32_t pCreateInfo->queueRecordCount, is "\
            "zero (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pRequestedQueues == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VkDeviceQueueCreateInfo* pCreateInfo->pRequestedQueues, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->queueRecordCount; ++i)
    {
        if(!vk_validate_vkdevicequeuecreateinfo(&(pCreateInfo->pRequestedQueues[i])))
        {
            std::stringstream ss;
            ss << "vkCreateDevice parameter, VkDeviceQueueCreateInfo pCreateInfo->pRequestedQueues[" << i <<
                "], is invalid (precondition).";
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

}

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    uint32_t i, ext_idx;
    VkLayerDebugMarkerDispatchTable *pDisp =  *(VkLayerDebugMarkerDispatchTable **) device;
    VkLayerDebugMarkerDispatchTable *pTable = tableDebugMarkerMap[pDisp];

    for (i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->pEnabledExtensions[i].name, DEBUG_MARKER_EXTENSION_NAME) == 0) {
            /* Found a matching extension name, mark it enabled */
            pTable->ext_enabled = true;
        }

    }
}

void PostCreateDevice(VkResult result, const VkDeviceCreateInfo *pCreateInfo, VkDevice* pDevice)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateDevice failed (postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    enable_debug_report(pCreateInfo->extensionCount, pCreateInfo->pEnabledExtensions);
    createDeviceRegisterExtensions(pCreateInfo, *pDevice);

    if(pDevice == nullptr)
    {
        char const str[] = "vkCreateDevice parameter, VkDevice* pDevice, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    PreCreateDevice(gpu, pCreateInfo);
    VkResult result = instance_dispatch_table(gpu)->CreateDevice(gpu, pCreateInfo, pDevice);
    PostCreateDevice(result, pCreateInfo, pDevice);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(VkDevice device)
{
    VkLayerDispatchTable *pDisp  =  *(VkLayerDispatchTable **) device;
    VkLayerDispatchTable *pTable = tableMap[pDisp];
    VkResult              result = pTable->DestroyDevice(device);
    tableMap.erase(pDisp);
    return result;
}

#define PARAM_CHECKER_LAYER_EXT_ARRAY_SIZE 1
static const VkExtensionProperties pcExts[PARAM_CHECKER_LAYER_EXT_ARRAY_SIZE] = {
    {
        VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES,
        "ParamChecker",
        0x10,
        "Sample layer: ParamChecker",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
        VkExtensionInfoType infoType,
        uint32_t extensionIndex,
        size_t*  pDataSize,
        void*    pData)
{
    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = PARAM_CHECKER_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= PARAM_CHECKER_LAYER_EXT_ARRAY_SIZE)
                return VK_ERROR_INVALID_VALUE;
            memcpy((VkExtensionProperties *) pData, &pcExts[extensionIndex], sizeof(VkExtensionProperties));
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionInfo(
                                               VkPhysicalDevice gpu,
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    VkResult result = instance_dispatch_table(gpu)->GetPhysicalDeviceExtensionInfo(gpu, infoType, extensionIndex, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetDeviceQueue(VkDevice device, uint32_t queueNodeIndex, uint32_t queueIndex, VkQueue* pQueue)
{

    VkResult result = device_dispatch_table(device)->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(VkQueue queue, uint32_t cmdBufferCount, const VkCmdBuffer* pCmdBuffers, VkFence fence)
{
    VkResult result = device_dispatch_table(queue)->QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitIdle(VkQueue queue)
{

    VkResult result = device_dispatch_table(queue)->QueueWaitIdle(queue);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDeviceWaitIdle(VkDevice device)
{

    VkResult result = device_dispatch_table(device)->DeviceWaitIdle(device);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocMemory(VkDevice device, const VkMemoryAllocInfo* pAllocInfo, VkDeviceMemory* pMem)
{
    char str[1024];
    if (!pAllocInfo) {
        sprintf(str, "Struct ptr parameter pAllocInfo to function AllocMemory is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    } else if (!vk_validate_vkmemoryallocinfo(pAllocInfo)) {
        sprintf(str, "Parameter pAllocInfo to function AllocMemory contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->AllocMemory(device, pAllocInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkFreeMemory(VkDevice device, VkDeviceMemory mem)
{

    VkResult result = device_dispatch_table(device)->FreeMemory(device, mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkSetMemoryPriority(VkDevice device, VkDeviceMemory mem, VkMemoryPriority priority)
{
    char str[1024];
    if (!validate_VkMemoryPriority(priority)) {
        sprintf(str, "Parameter priority to function SetMemoryPriority has invalid value of %i.", (int)priority);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->SetMemoryPriority(device, mem, priority);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkFlags flags, void** ppData)
{

    VkResult result = device_dispatch_table(device)->MapMemory(device, mem, offset, size, flags, ppData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkUnmapMemory(VkDevice device, VkDeviceMemory mem)
{

    VkResult result = device_dispatch_table(device)->UnmapMemory(device, mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkFlushMappedMemoryRanges(
        VkDevice device,
        uint32_t memRangeCount,
        const VkMappedMemoryRange* pMemRanges)
{

    VkResult result = device_dispatch_table(device)->FlushMappedMemoryRanges(device, memRangeCount, pMemRanges);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkInvalidateMappedMemoryRanges(
        VkDevice device,
        uint32_t memRangeCount,
        const VkMappedMemoryRange* pMemRanges)
{

    VkResult result = device_dispatch_table(device)->InvalidateMappedMemoryRanges(device, memRangeCount, pMemRanges);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkPinSystemMemory(VkDevice device, const void* pSysMem, size_t memSize, VkDeviceMemory* pMem)
{

    VkResult result = device_dispatch_table(device)->PinSystemMemory(device, pSysMem, memSize, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetMultiDeviceCompatibility(VkPhysicalDevice gpu0, VkPhysicalDevice gpu1, VkPhysicalDeviceCompatibilityInfo* pInfo)
{

    VkResult result = instance_dispatch_table(gpu0)->GetMultiDeviceCompatibility(gpu0, gpu1, pInfo);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenSharedMemory(VkDevice device, const VkMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenSharedMemory is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkmemoryopeninfo(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenSharedMemory contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->OpenSharedMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenSharedSemaphore(VkDevice device, const VkSemaphoreOpenInfo* pOpenInfo, VkSemaphore* pSemaphore)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenSharedSemaphore is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vksemaphoreopeninfo(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenSharedSemaphore contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->OpenSharedSemaphore(device, pOpenInfo, pSemaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerMemory(VkDevice device, const VkPeerMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenPeerMemory is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkpeermemoryopeninfo(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenPeerMemory contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->OpenPeerMemory(device, pOpenInfo, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkOpenPeerImage(VkDevice device, const VkPeerImageOpenInfo* pOpenInfo, VkImage* pImage, VkDeviceMemory* pMem)
{
    char str[1024];
    if (!pOpenInfo) {
        sprintf(str, "Struct ptr parameter pOpenInfo to function OpenPeerImage is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkpeerimageopeninfo(pOpenInfo)) {
        sprintf(str, "Parameter pOpenInfo to function OpenPeerImage contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->OpenPeerImage(device, pOpenInfo, pImage, pMem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyObject(VkDevice device, VkObjectType objType, VkObject object)
{
    VkResult result = device_dispatch_table(device)->DestroyObject(device, objType, object);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetObjectInfo(VkDevice device, VkObjectType objType, VkObject object, VkObjectInfoType infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!validate_VkObjectInfoType(infoType)) {
        sprintf(str, "Parameter infoType to function GetObjectInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->GetObjectInfo(device, objType, object, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBindObjectMemory(VkDevice device, VkObjectType objType, VkObject object, VkDeviceMemory mem, VkDeviceSize offset)
{

    VkResult result = device_dispatch_table(device)->BindObjectMemory(device, objType, object, mem, offset);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseBufferMemory(VkQueue queue, VkBuffer buffer, VkDeviceSize rangeOffset, VkDeviceSize rangeSize, VkDeviceMemory mem, VkDeviceSize memOffset)
{

    VkResult result = device_dispatch_table(queue)->QueueBindSparseBufferMemory(queue, buffer, rangeOffset, rangeSize, mem, memOffset);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseImageMemory(VkQueue queue, VkImage image, const VkImageMemoryBindInfo* pBindInfo, VkDeviceMemory mem, VkDeviceSize memOffset)
{
    char str[1024];
    if (!pBindInfo) {
        sprintf(str, "Struct ptr parameter pBindInfo to function QueueBindSparseImageMemory is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkimagememorybindinfo(pBindInfo)) {
        sprintf(str, "Parameter pBindInfo to function BindImageMemoryRange contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(queue)->QueueBindSparseImageMemory(queue, image, pBindInfo, mem, memOffset);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, VkFence* pFence)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateFence is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkfencecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateFence contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateFence(device, pCreateInfo, pFence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFenceStatus(VkDevice device, VkFence fence)
{

    VkResult result = device_dispatch_table(device)->GetFenceStatus(device, fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, bool32_t waitAll, uint64_t timeout)
{

    VkResult result = device_dispatch_table(device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetFences(VkDevice device, uint32_t fenceCount, VkFence* pFences)
{

    VkResult result = device_dispatch_table(device)->ResetFences(device, fenceCount, pFences);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, VkSemaphore* pSemaphore)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateSemaphore is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vksemaphorecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateSemaphore contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateSemaphore(device, pCreateInfo, pSemaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSignalSemaphore(VkQueue queue, VkSemaphore semaphore)
{

    VkResult result = device_dispatch_table(queue)->QueueSignalSemaphore(queue, semaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitSemaphore(VkQueue queue, VkSemaphore semaphore)
{

    VkResult result = device_dispatch_table(queue)->QueueWaitSemaphore(queue, semaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, VkEvent* pEvent)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateEvent is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkeventcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateEvent contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateEvent(device, pCreateInfo, pEvent);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetEventStatus(VkDevice device, VkEvent event)
{

    VkResult result = device_dispatch_table(device)->GetEventStatus(device, event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkSetEvent(VkDevice device, VkEvent event)
{

    VkResult result = device_dispatch_table(device)->SetEvent(device, event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetEvent(VkDevice device, VkEvent event)
{

    VkResult result = device_dispatch_table(device)->ResetEvent(device, event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateQueryPool is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkquerypoolcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateQueryPool contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateQueryPool(device, pCreateInfo, pQueryPool);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData, VkQueryResultFlags flags)
{

    VkResult result = device_dispatch_table(device)->GetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFormatInfo(VkDevice device, VkFormat format, VkFormatInfoType infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!validate_VkFormat(format)) {
        sprintf(str, "Parameter format to function GetFormatInfo has invalid value of %i.", (int)format);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkFormatInfoType(infoType)) {
        sprintf(str, "Parameter infoType to function GetFormatInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->GetFormatInfo(device, format, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateBuffer is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkbuffercreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateBuffer contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateBuffer(device, pCreateInfo, pBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateBufferView is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkbufferviewcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateBufferView contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateBufferView(device, pCreateInfo, pView);
    return result;
}

void PreCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo)
{
    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateImage parameter, VkImageCreateInfo* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO)
    {
        char const str[] = "vkCreateImage parameter, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO "\
            "pCreateInfo->sType, is not VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_VkImageType(pCreateInfo->imageType))
    {
        char const str[] = "vkCreateImage parameter, VkImageType pCreateInfo->imageType, is "\
            "unrecognized (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_VkFormat(pCreateInfo->format))
    {
        char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, is "\
            "unrecognized (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    VkFormatProperties properties;
    size_t size = sizeof(properties);
    VkResult result = device_dispatch_table(device)->GetFormatInfo(device, pCreateInfo->format,
        VK_FORMAT_INFO_TYPE_PROPERTIES, &size, &properties);
    if(result != VK_SUCCESS)
    {
        char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, cannot be "\
            "validated (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
    {
        char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, contains "\
            "unsupported format (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    // TODO: Can we check device-specific limits?
    if (!vk_validate_vkextent3d(&pCreateInfo->extent))
    {
        char const str[] = "vkCreateImage parameter, VkExtent3D pCreateInfo->extent, is invalid "\
            "(precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if (!validate_VkImageTiling(pCreateInfo->tiling))
    {
        char const str[] = "vkCreateImage parameter, VkImageTiling pCreateInfo->tiling, is "\
            "unrecoginized (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateImage(VkResult result, VkImage* pImage)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateImage failed (postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pImage == nullptr)
    {
        char const str[] = "vkCreateImage parameter, VkImage* pImage, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    PreCreateImage(device, pCreateInfo);
    VkResult result = device_dispatch_table(device)->CreateImage(device, pCreateInfo, pImage);
    PostCreateImage(result, pImage);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetImageSubresourceInfo(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceInfoType infoType, size_t* pDataSize, void* pData)
{
    char str[1024];
    if (!pSubresource) {
        sprintf(str, "Struct ptr parameter pSubresource to function GetImageSubresourceInfo is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    } else if (!vk_validate_vkimagesubresource(pSubresource)) {
        sprintf(str, "Parameter pSubresource to function GetImageSubresourceInfo contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkSubresourceInfoType(infoType)) {
        sprintf(str, "Parameter infoType to function GetImageSubresourceInfo has invalid value of %i.", (int)infoType);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->GetImageSubresourceInfo(device, image, pSubresource, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateImageView is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkimageviewcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateImageView contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateImageView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateColorAttachmentView(VkDevice device, const VkColorAttachmentViewCreateInfo* pCreateInfo, VkColorAttachmentView* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateColorAttachmentView is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkcolorattachmentviewcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateColorAttachmentView contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateColorAttachmentView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDepthStencilView(VkDevice device, const VkDepthStencilViewCreateInfo* pCreateInfo, VkDepthStencilView* pView)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDepthStencilView is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdepthstencilviewcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDepthStencilView contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateDepthStencilView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateShader(VkDevice device, const VkShaderCreateInfo* pCreateInfo, VkShader* pShader)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateShader is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkshadercreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateShader contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateShader(device, pCreateInfo, pShader);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateGraphicsPipeline is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkgraphicspipelinecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateGraphicsPipeline contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelineDerivative(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline basePipeline, VkPipeline* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateGraphicsPipelineDerivative is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkgraphicspipelinecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateGraphicsPipelineDerivative contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateGraphicsPipelineDerivative(device, pCreateInfo, basePipeline, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateComputePipeline(VkDevice device, const VkComputePipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateComputePipeline is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkcomputepipelinecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateComputePipeline contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateComputePipeline(device, pCreateInfo, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkStorePipeline(VkDevice device, VkPipeline pipeline, size_t* pDataSize, void* pData)
{

    VkResult result = device_dispatch_table(device)->StorePipeline(device, pipeline, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkLoadPipeline(VkDevice device, size_t dataSize, const void* pData, VkPipeline* pPipeline)
{

    VkResult result = device_dispatch_table(device)->LoadPipeline(device, dataSize, pData, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkLoadPipelineDerivative(VkDevice device, size_t dataSize, const void* pData, VkPipeline basePipeline, VkPipeline* pPipeline)
{

    VkResult result = device_dispatch_table(device)->LoadPipelineDerivative(device, dataSize, pData, basePipeline, pPipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateSampler is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vksamplercreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateSampler contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateSampler(device, pCreateInfo, pSampler);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDescriptorSetLayout is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdescriptorsetlayoutcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDescriptorSetLayout contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, VkPipelineLayout* pPipelineLayout)
{
    VkResult result = device_dispatch_table(device)->CreatePipelineLayout(device, pCreateInfo, pPipelineLayout);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolUsage poolUsage, uint32_t maxSets, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool)
{
    char str[1024];
    if (!validate_VkDescriptorPoolUsage(poolUsage)) {
        sprintf(str, "Parameter poolUsage to function CreateDescriptorPool has invalid value of %i.", (int)poolUsage);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDescriptorPool is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdescriptorpoolcreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDescriptorPool contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{

    VkResult result = device_dispatch_table(device)->ResetDescriptorPool(device, descriptorPool);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetUsage setUsage, uint32_t count, const VkDescriptorSetLayout* pSetLayouts, VkDescriptorSet* pDescriptorSets, uint32_t* pCount)
{
    char str[1024];
    if (!validate_VkDescriptorSetUsage(setUsage)) {
        sprintf(str, "Parameter setUsage to function AllocDescriptorSets has invalid value of %i.", (int)setUsage);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->AllocDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkClearDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{

    device_dispatch_table(device)->ClearDescriptorSets(device, descriptorPool, count, pDescriptorSets);
}

VK_LAYER_EXPORT VkResult VKAPI vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t copyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{

    return device_dispatch_table(device)->UpdateDescriptorSets(device, writeCount, pDescriptorWrites, copyCount, pDescriptorCopies);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(VkDevice device, const VkDynamicVpStateCreateInfo* pCreateInfo, VkDynamicVpState* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicViewportState is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdynamicvpstatecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicViewportState contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateDynamicViewportState(device, pCreateInfo, pState);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicRasterState(VkDevice device, const VkDynamicRsStateCreateInfo* pCreateInfo, VkDynamicRsState* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicRasterState is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdynamicrsstatecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicRasterState contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateDynamicRasterState(device, pCreateInfo, pState);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(VkDevice device, const VkDynamicCbStateCreateInfo* pCreateInfo, VkDynamicCbState* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicColorBlendState is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdynamiccbstatecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicColorBlendState contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateDynamicColorBlendState(device, pCreateInfo, pState);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(VkDevice device, const VkDynamicDsStateCreateInfo* pCreateInfo, VkDynamicDsState* pState)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateDynamicDepthStencilState is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkdynamicdsstatecreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateDynamicDepthStencilState contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    return result;
}

void PreCreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo)
{
    if(device == nullptr)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VkDevice device, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VkCmdBufferCreateInfo* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO "\
            "pCreateInfo->sType, is not VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateCommandBuffer(VkResult result, VkCmdBuffer* pCmdBuffer)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateCommandBuffer failed (postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCmdBuffer == nullptr)
    {
        char const str[] = "vkCreateCommandBuffer parameter, VkCmdBuffer* pCmdBuffer, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandBuffer(VkDevice device,
    const VkCmdBufferCreateInfo* pCreateInfo, VkCmdBuffer* pCmdBuffer)
{
    PreCreateCommandBuffer(device, pCreateInfo);
    VkResult result = device_dispatch_table(device)->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    PostCreateCommandBuffer(result, pCmdBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo)
{
    char str[1024];
    if (!pBeginInfo) {
        sprintf(str, "Struct ptr parameter pBeginInfo to function BeginCommandBuffer is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkcmdbufferbegininfo(pBeginInfo)) {
        sprintf(str, "Parameter pBeginInfo to function BeginCommandBuffer contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(cmdBuffer)->BeginCommandBuffer(cmdBuffer, pBeginInfo);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(VkCmdBuffer cmdBuffer)
{

    VkResult result = device_dispatch_table(cmdBuffer)->EndCommandBuffer(cmdBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(VkCmdBuffer cmdBuffer)
{

    VkResult result = device_dispatch_table(cmdBuffer)->ResetCommandBuffer(cmdBuffer);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdBindPipeline has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDynamicStateObject(VkCmdBuffer cmdBuffer, VkStateBindPoint stateBindPoint, VkDynamicStateObject state)
{
    char str[1024];
    if (!validate_VkStateBindPoint(stateBindPoint)) {
        sprintf(str, "Parameter stateBindPoint to function CmdBindDynamicStateObject has invalid value of %i.", (int)stateBindPoint);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdBindDescriptorSets has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets)
{
    device_dispatch_table(cmdBuffer)->CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    char str[1024];
    if (!validate_VkIndexType(indexType)) {
        sprintf(str, "Parameter indexType to function CmdBindIndexBuffer has invalid value of %i.", (int)indexType);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdDraw(VkCmdBuffer cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{

    device_dispatch_table(cmdBuffer)->CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexed(VkCmdBuffer cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{

    device_dispatch_table(cmdBuffer)->CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{

    device_dispatch_table(cmdBuffer)->CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{

    device_dispatch_table(cmdBuffer)->CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatch(VkCmdBuffer cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{

    device_dispatch_table(cmdBuffer)->CmdDispatch(cmdBuffer, x, y, z);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset)
{

    device_dispatch_table(cmdBuffer)->CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer destBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    char str[1024];
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkbuffercopy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyBuffer contains an invalid value.", i);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    device_dispatch_table(cmdBuffer)->CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
    char str[1024];
    if (!validate_VkImageLayout(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCopyImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkImageLayout(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCopyImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkimagecopy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    device_dispatch_table(cmdBuffer)->CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkTexFilter filter)
{
    char str[1024];
    if (!validate_VkImageLayout(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdBlitImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkImageLayout(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdBlitImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkimageblit(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdBlitImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    //TODO:  Add additional check for limitation from header rev 96.
    // VK_TEX_FILTER_NEAREST if the format that srcImage was created with is an integer-based format.

    device_dispatch_table(cmdBuffer)->CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions, filter);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    char str[1024];
    if (!validate_VkImageLayout(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdCopyBufferToImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkbufferimagecopy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyBufferToImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    device_dispatch_table(cmdBuffer)->CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer destBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    char str[1024];
    if (!validate_VkImageLayout(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdCopyImageToBuffer has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkbufferimagecopy(&pRegions[i])) {
            sprintf(str, "Parameter pRegions[%i] to function CmdCopyImageToBuffer contains an invalid value.", i);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    device_dispatch_table(cmdBuffer)->CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize dataSize, const uint32_t* pData)
{

    device_dispatch_table(cmdBuffer)->CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize fillSize, uint32_t data)
{

    device_dispatch_table(cmdBuffer)->CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColor* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    char str[1024];
    if (!validate_VkImageLayout(imageLayout)) {
        sprintf(str, "Parameter imageLayout to function CmdClearColorImage has invalid value of %i.", (int)imageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rangeCount; i++) {
        if (!vk_validate_vkimagesubresourcerange(&pRanges[i])) {
            sprintf(str, "Parameter pRanges[%i] to function CmdClearColorImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    device_dispatch_table(cmdBuffer)->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencil(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    char str[1024];
    if (!validate_VkImageLayout(imageLayout)) {
        sprintf(str, "Parameter imageLayout to function CmdClearDepthStencil has invalid value of %i.", (int)imageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < rangeCount; i++) {
        if (!vk_validate_vkimagesubresourcerange(&pRanges[i])) {
            sprintf(str, "Parameter pRanges[%i] to function CmdClearDepthStencil contains an invalid value.", i);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    device_dispatch_table(cmdBuffer)->CmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageResolve* pRegions)
{
    char str[1024];
    if (!validate_VkImageLayout(srcImageLayout)) {
        sprintf(str, "Parameter srcImageLayout to function CmdResolveImage has invalid value of %i.", (int)srcImageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    if (!validate_VkImageLayout(destImageLayout)) {
        sprintf(str, "Parameter destImageLayout to function CmdResolveImage has invalid value of %i.", (int)destImageLayout);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    uint32_t i;
    for (i = 0; i < regionCount; i++) {
        if (!vk_validate_vkimageresolve(&pRegions[i])) {
            sprintf(str, "Parameter pRects[%i] to function CmdResolveImage contains an invalid value.", i);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        }
    }
    device_dispatch_table(cmdBuffer)->CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdSetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent)
{
    char str[1024];
    if (!validate_VkPipeEvent(pipeEvent)) {
        sprintf(str, "Parameter pipeEvent to function CmdSetEvent has invalid value of %i.", (int)pipeEvent);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdSetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent)
{
    char str[1024];
    if (!validate_VkPipeEvent(pipeEvent)) {
        sprintf(str, "Parameter pipeEvent to function CmdResetEvent has invalid value of %i.", (int)pipeEvent);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdResetEvent(cmdBuffer, event, pipeEvent);
}

VK_LAYER_EXPORT void VKAPI vkCmdWaitEvents(VkCmdBuffer cmdBuffer, VkWaitEvent waitEvent, uint32_t eventCount, const VkEvent* pEvents, uint32_t memBarrierCount, const void** ppMemBarriers)
{
    device_dispatch_table(cmdBuffer)->CmdWaitEvents(cmdBuffer, waitEvent, eventCount, pEvents, memBarrierCount, ppMemBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdPipelineBarrier(VkCmdBuffer cmdBuffer, VkWaitEvent waitEvent, uint32_t pipeEventCount, const VkPipeEvent* pPipeEvents, uint32_t memBarrierCount, const void** ppMemBarriers)
{
    device_dispatch_table(cmdBuffer)->CmdPipelineBarrier(cmdBuffer, waitEvent, pipeEventCount, pPipeEvents, memBarrierCount, ppMemBarriers);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
{

    device_dispatch_table(cmdBuffer)->CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot)
{

    device_dispatch_table(cmdBuffer)->CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount)
{

    device_dispatch_table(cmdBuffer)->CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT void VKAPI vkCmdWriteTimestamp(VkCmdBuffer cmdBuffer, VkTimestampType timestampType, VkBuffer destBuffer, VkDeviceSize destOffset)
{
    char str[1024];
    if (!validate_VkTimestampType(timestampType)) {
        sprintf(str, "Parameter timestampType to function CmdWriteTimestamp has invalid value of %i.", (int)timestampType);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyQueryPoolResults(
    VkCmdBuffer                                 cmdBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    VkBuffer                                    destBuffer,
    VkDeviceSize                                destOffset,
    VkDeviceSize                                destStride,
    VkQueryResultFlags                          flags)
{
    device_dispatch_table(cmdBuffer)->CmdCopyQueryPoolResults(cmdBuffer, queryPool, startQuery, queryCount, destBuffer, destOffset, destStride, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdInitAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdInitAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdLoadAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer srcBuffer, VkDeviceSize srcOffset)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdLoadAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

VK_LAYER_EXPORT void VKAPI vkCmdSaveAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer destBuffer, VkDeviceSize destOffset)
{
    char str[1024];
    if (!validate_VkPipelineBindPoint(pipelineBindPoint)) {
        sprintf(str, "Parameter pipelineBindPoint to function CmdSaveAtomicCounters has invalid value of %i.", (int)pipelineBindPoint);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer)
{
    char str[1024];
    if (!pCreateInfo) {
        sprintf(str, "Struct ptr parameter pCreateInfo to function CreateFramebuffer is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkframebuffercreateinfo(pCreateInfo)) {
        sprintf(str, "Parameter pCreateInfo to function CreateFramebuffer contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    VkResult result = device_dispatch_table(device)->CreateFramebuffer(device, pCreateInfo, pFramebuffer);
    return result;
}


void PreCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo)
{
    if(pCreateInfo == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkRenderPassCreateInfo* pCreateInfo, is "\
            "nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->sType != VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO "\
            "pCreateInfo->sType, is not VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO (precondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!vk_validate_vkrect(&pCreateInfo->renderArea))
    {
        char const str[] = "vkCreateRenderPass parameter, VkRect pCreateInfo->renderArea, is invalid "\
            "(precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!vk_validate_vkextent2d(&pCreateInfo->extent))
    {
        char const str[] = "vkCreateRenderPass parameter, VkExtent2D pCreateInfo->extent, is invalid "\
            "(precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pColorFormats == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkFormat* pCreateInfo->pColorFormats, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkFormat(pCreateInfo->pColorFormats[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkFormat pCreateInfo->pColorFormats[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

        VkFormatProperties properties;
        size_t size = sizeof(properties);
        VkResult result = device_dispatch_table(device)->GetFormatInfo(device, pCreateInfo->pColorFormats[i],
            VK_FORMAT_INFO_TYPE_PROPERTIES, &size, &properties);
        if(result != VK_SUCCESS)
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkFormat pCreateInfo->pColorFormats[" << i <<
                "], cannot be validated (precondition).";
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

        if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkFormat pCreateInfo->pColorFormats[" << i <<
                "], contains unsupported format (precondition).";
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }

    }

    if(pCreateInfo->pColorLayouts == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkImageLayout* pCreateInfo->pColorLayouts, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkImageLayout(pCreateInfo->pColorLayouts[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pColorLayouts[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadOps == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentLoadOp* pCreateInfo->pColorLoadOps, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkAttachmentLoadOp(pCreateInfo->pColorLoadOps[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->pColorLoadOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorStoreOps == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentStoreOp* pCreateInfo->pColorStoreOps, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkAttachmentStoreOp(pCreateInfo->pColorStoreOps[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->pColorStoreOps[" << i <<
                "], is unrecognized (precondition).";
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(pCreateInfo->pColorLoadClearValues == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkClearColor* pCreateInfo->"\
            "pColorLoadClearValues, is nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pColorStoreOps == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_ATTACHMENT_STORE_OP* pCreateInfo->pColorStoreOps, "\
            "is nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pCreateInfo->pColorLoadClearValues == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VK_CLEAR_COLOR* pCreateInfo->"\
            "pColorLoadClearValues, is nullptr (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!vk_validate_vkclearcolor(&(pCreateInfo->pColorLoadClearValues[i])))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkClearColor pCreateInfo->pColorLoadClearValues[" << i <<
                "], is invalid (precondition).";
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", ss.str().c_str());
            continue;
        }
    }

    if(!validate_VkFormat(pCreateInfo->depthStencilFormat))
    {
        char const str[] = "vkCreateRenderPass parameter, VkFormat pCreateInfo->"\
            "depthStencilFormat, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    VkFormatProperties properties;
    size_t size = sizeof(properties);
    VkResult result = device_dispatch_table(device)->GetFormatInfo(device, pCreateInfo->depthStencilFormat,
        VK_FORMAT_INFO_TYPE_PROPERTIES, &size, &properties);
    if(result != VK_SUCCESS)
    {
        char const str[] = "vkCreateRenderPass parameter, VkFormat pCreateInfo->"\
            "depthStencilFormat, cannot be validated (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
    {
        char const str[] = "vkCreateRenderPass parameter, VkFormat pCreateInfo->"\
            "depthStencilFormat, contains unsupported format (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkImageLayout(pCreateInfo->depthStencilLayout))
    {
        char const str[] = "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->"\
            "depthStencilLayout, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkAttachmentLoadOp(pCreateInfo->depthLoadOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->"\
            "depthLoadOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkAttachmentStoreOp(pCreateInfo->depthStoreOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->"\
            "depthStoreOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkAttachmentLoadOp(pCreateInfo->stencilLoadOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->"\
            "stencilLoadOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(!validate_VkAttachmentStoreOp(pCreateInfo->stencilStoreOp))
    {
        char const str[] = "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->"\
            "stencilStoreOp, is unrecognized (precondition).";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

void PostCreateRenderPass(VkResult result, VkRenderPass* pRenderPass)
{
    if(result != VK_SUCCESS)
    {
        // TODO: Spit out VkResult value.
        char const str[] = "vkCreateRenderPass failed (postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }

    if(pRenderPass == nullptr)
    {
        char const str[] = "vkCreateRenderPass parameter, VkRenderPass* pRenderPass, is nullptr (postcondition).";
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
        return;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    PreCreateRenderPass(device, pCreateInfo);
    VkResult result = device_dispatch_table(device)->CreateRenderPass(device, pCreateInfo, pRenderPass);
    PostCreateRenderPass(result, pRenderPass);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginRenderPass(VkCmdBuffer cmdBuffer, const VkRenderPassBegin* pRenderPassBegin)
{
    char str[1024];
    if (!pRenderPassBegin) {
        sprintf(str, "Struct ptr parameter pRenderPassBegin to function CmdBeginRenderPass is NULL.");
        layerCbMsg(VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    else if (!vk_validate_vkrenderpassbegin(pRenderPassBegin)) {
        sprintf(str, "Parameter pRenderPassBegin to function CmdBeginRenderPass contains an invalid value.");
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    device_dispatch_table(cmdBuffer)->CmdBeginRenderPass(cmdBuffer, pRenderPassBegin);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndRenderPass(VkCmdBuffer cmdBuffer, VkRenderPass renderPass)
{

    device_dispatch_table(cmdBuffer)->CmdEndRenderPass(cmdBuffer, renderPass);
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(
        VkInstance instance,
        VkFlags msgFlags,
        const PFN_vkDbgMsgCallback pfnMsgCallback,
        void* pUserData,
        VkDbgMsgCallback* pMsgCallback)
{
    VkLayerInstanceDispatchTable *pDisp = *(VkLayerInstanceDispatchTable **) instance;
    VkLayerInstanceDispatchTable *pTable = tableInstanceMap[pDisp];
    return layer_create_msg_callback(instance, pTable, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerBegin(VkCmdBuffer cmdBuffer, const char* pMarker)
{
    VkLayerDebugMarkerDispatchTable *pDisp  = *(VkLayerDebugMarkerDispatchTable **) cmdBuffer;
    VkLayerDebugMarkerDispatchTable *pTable = tableDebugMarkerMap[pDisp];
    if (!pTable->ext_enabled) {
        char const str[] = "Attempt to use CmdDbgMarkerBegin but extension disabled!";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    pTable->CmdDbgMarkerBegin(cmdBuffer, pMarker);
}

VK_LAYER_EXPORT void VKAPI vkCmdDbgMarkerEnd(VkCmdBuffer cmdBuffer)
{
    VkLayerDebugMarkerDispatchTable *pDisp  = *(VkLayerDebugMarkerDispatchTable **) cmdBuffer;
    VkLayerDebugMarkerDispatchTable *pTable = tableDebugMarkerMap[pDisp];
    if (!pTable->ext_enabled) {
        char const str[] = "Attempt to use CmdDbgMarkerEnd but extension disabled!";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    pTable->CmdDbgMarkerEnd(cmdBuffer);
}

VkResult VKAPI vkDbgSetObjectTag(VkDevice device, VkObjectType  objType, VkObject object, size_t tagSize, const void* pTag)
{
    VkLayerDebugMarkerDispatchTable *pDisp  = *(VkLayerDebugMarkerDispatchTable **) device;
    VkLayerDebugMarkerDispatchTable *pTable = tableDebugMarkerMap[pDisp];
    if (!pTable->ext_enabled) {
        char const str[] = "Attempt to use DbgSetObjectTag but extension disabled!";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    pTable->DbgSetObjectTag(device, objType, object, tagSize, pTag);
}

VkResult VKAPI vkDbgSetObjectName(VkDevice device, VkObjectType  objType, VkObject object, size_t nameSize, const char* pName)
{
    VkLayerDebugMarkerDispatchTable *pDisp  = *(VkLayerDebugMarkerDispatchTable **) device;
    VkLayerDebugMarkerDispatchTable *pTable = tableDebugMarkerMap[pDisp];

    if (!pTable->ext_enabled) {
        char const str[] = "Attempt to use DbgSetObjectName but extension disabled!";
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, (VkObjectType) 0, NULL, 0, 1, "PARAMCHECK", str);
    }
    pTable->DbgSetObjectName(device, objType, object, nameSize, pName);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetDisplayInfoWSI(VkDisplayWSI display, VkDisplayInfoTypeWSI infoType, size_t* pDataSize, void* pData)
{
    VkResult result = instance_dispatch_table(display)->GetDisplayInfoWSI(display, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSwapChainWSI(VkDevice device, const VkSwapChainCreateInfoWSI* pCreateInfo, VkSwapChainWSI* pSwapChain)
{
    VkResult result = device_dispatch_table(device)->CreateSwapChainWSI(device, pCreateInfo, pSwapChain);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroySwapChainWSI(VkSwapChainWSI swapChain)
{
    VkResult result = device_dispatch_table(swapChain)->DestroySwapChainWSI(swapChain);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetSwapChainInfoWSI(VkSwapChainWSI swapChain, VkSwapChainInfoTypeWSI infoType, size_t* pDataSize, void* pData)
{
    VkResult result = device_dispatch_table(swapChain)->GetSwapChainInfoWSI(swapChain, infoType, pDataSize, pData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueuePresentWSI(VkQueue queue, const VkPresentInfoWSI* pPresentInfo)
{
    VkResult result = device_dispatch_table(queue)->QueuePresentWSI(queue, pPresentInfo);
    return result;
}

static inline void* layer_intercept_proc(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "DestroyDevice"))
        return (void*) vkDestroyDevice;
    if (!strcmp(name, "GetDeviceQueue"))
        return (void*) vkGetDeviceQueue;
    if (!strcmp(name, "QueueSubmit"))
        return (void*) vkQueueSubmit;
    if (!strcmp(name, "QueueWaitIdle"))
        return (void*) vkQueueWaitIdle;
    if (!strcmp(name, "DeviceWaitIdle"))
        return (void*) vkDeviceWaitIdle;
    if (!strcmp(name, "AllocMemory"))
        return (void*) vkAllocMemory;
    if (!strcmp(name, "FreeMemory"))
        return (void*) vkFreeMemory;
    if (!strcmp(name, "SetMemoryPriority"))
        return (void*) vkSetMemoryPriority;
    if (!strcmp(name, "MapMemory"))
        return (void*) vkMapMemory;
    if (!strcmp(name, "UnmapMemory"))
        return (void*) vkUnmapMemory;
    if (!strcmp(name, "FlushMappedMemoryRanges"))
        return (void*) vkFlushMappedMemoryRanges;
    if (!strcmp(name, "InvalidateMappedMemoryRanges"))
        return (void*) vkInvalidateMappedMemoryRanges;
    if (!strcmp(name, "PinSystemMemory"))
        return (void*) vkPinSystemMemory;
    if (!strcmp(name, "OpenSharedMemory"))
        return (void*) vkOpenSharedMemory;
    if (!strcmp(name, "OpenSharedSemaphore"))
        return (void*) vkOpenSharedSemaphore;
    if (!strcmp(name, "OpenPeerMemory"))
        return (void*) vkOpenPeerMemory;
    if (!strcmp(name, "OpenPeerImage"))
        return (void*) vkOpenPeerImage;
    if (!strcmp(name, "DestroyObject"))
        return (void*) vkDestroyObject;
    if (!strcmp(name, "GetObjectInfo"))
        return (void*) vkGetObjectInfo;
    if (!strcmp(name, "CreateFence"))
        return (void*) vkCreateFence;
    if (!strcmp(name, "ResetFences"))
        return (void*) vkResetFences;
    if (!strcmp(name, "GetFenceStatus"))
        return (void*) vkGetFenceStatus;
    if (!strcmp(name, "WaitForFences"))
        return (void*) vkWaitForFences;
    if (!strcmp(name, "CreateSemaphore"))
        return (void*) vkCreateSemaphore;
    if (!strcmp(name, "QueueSignalSemaphore"))
        return (void*) vkQueueSignalSemaphore;
    if (!strcmp(name, "QueueWaitSemaphore"))
        return (void*) vkQueueWaitSemaphore;
    if (!strcmp(name, "CreateEvent"))
        return (void*) vkCreateEvent;
    if (!strcmp(name, "GetEventStatus"))
        return (void*) vkGetEventStatus;
    if (!strcmp(name, "SetEvent"))
        return (void*) vkSetEvent;
    if (!strcmp(name, "ResetEvent"))
        return (void*) vkResetEvent;
    if (!strcmp(name, "CreateQueryPool"))
        return (void*) vkCreateQueryPool;
    if (!strcmp(name, "GetQueryPoolResults"))
        return (void*) vkGetQueryPoolResults;
    if (!strcmp(name, "GetFormatInfo"))
        return (void*) vkGetFormatInfo;
    if (!strcmp(name, "CreateBuffer"))
        return (void*) vkCreateBuffer;
    if (!strcmp(name, "CreateBufferView"))
        return (void*) vkCreateBufferView;
    if (!strcmp(name, "CreateImage"))
        return (void*) vkCreateImage;
    if (!strcmp(name, "GetImageSubresourceInfo"))
        return (void*) vkGetImageSubresourceInfo;
    if (!strcmp(name, "CreateImageView"))
        return (void*) vkCreateImageView;
    if (!strcmp(name, "CreateColorAttachmentView"))
        return (void*) vkCreateColorAttachmentView;
    if (!strcmp(name, "CreateDepthStencilView"))
        return (void*) vkCreateDepthStencilView;
    if (!strcmp(name, "CreateShader"))
        return (void*) vkCreateShader;
    if (!strcmp(name, "CreateGraphicsPipeline"))
        return (void*) vkCreateGraphicsPipeline;
    if (!strcmp(name, "CreateGraphicsPipelineDerivative"))
        return (void*) vkCreateGraphicsPipelineDerivative;
    if (!strcmp(name, "CreateComputePipeline"))
        return (void*) vkCreateComputePipeline;
    if (!strcmp(name, "StorePipeline"))
        return (void*) vkStorePipeline;
    if (!strcmp(name, "LoadPipeline"))
        return (void*) vkLoadPipeline;
    if (!strcmp(name, "LoadPipelineDerivative"))
        return (void*) vkLoadPipelineDerivative;
    if (!strcmp(name, "CreatePipelineLayout"))
        return (void*) vkCreatePipelineLayout;
    if (!strcmp(name, "CreateSampler"))
        return (void*) vkCreateSampler;
    if (!strcmp(name, "CreateDescriptorSetLayout"))
        return (void*) vkCreateDescriptorSetLayout;
    if (!strcmp(name, "CreateDescriptorPool"))
        return (void*) vkCreateDescriptorPool;
    if (!strcmp(name, "ResetDescriptorPool"))
        return (void*) vkResetDescriptorPool;
    if (!strcmp(name, "AllocDescriptorSets"))
        return (void*) vkAllocDescriptorSets;
    if (!strcmp(name, "ClearDescriptorSets"))
        return (void*) vkClearDescriptorSets;
    if (!strcmp(name, "CreateDynamicViewportState"))
        return (void*) vkCreateDynamicViewportState;
    if (!strcmp(name, "CreateDynamicRasterState"))
        return (void*) vkCreateDynamicRasterState;
    if (!strcmp(name, "CreateDynamicColorBlendState"))
        return (void*) vkCreateDynamicColorBlendState;
    if (!strcmp(name, "CreateDynamicDepthStencilState"))
        return (void*) vkCreateDynamicDepthStencilState;
    if (!strcmp(name, "CreateCommandBuffer"))
        return (void*) vkCreateCommandBuffer;
    if (!strcmp(name, "BeginCommandBuffer"))
        return (void*) vkBeginCommandBuffer;
    if (!strcmp(name, "EndCommandBuffer"))
        return (void*) vkEndCommandBuffer;
    if (!strcmp(name, "ResetCommandBuffer"))
        return (void*) vkResetCommandBuffer;
    if (!strcmp(name, "CmdBindPipeline"))
        return (void*) vkCmdBindPipeline;
    if (!strcmp(name, "CmdBindDynamicStateObject"))
        return (void*) vkCmdBindDynamicStateObject;
    if (!strcmp(name, "CmdBindDescriptorSets"))
        return (void*) vkCmdBindDescriptorSets;
    if (!strcmp(name, "CmdBindVertexBuffers"))
        return (void*) vkCmdBindVertexBuffers;
    if (!strcmp(name, "CmdBindIndexBuffer"))
        return (void*) vkCmdBindIndexBuffer;
    if (!strcmp(name, "CmdDraw"))
        return (void*) vkCmdDraw;
    if (!strcmp(name, "CmdDrawIndexed"))
        return (void*) vkCmdDrawIndexed;
    if (!strcmp(name, "CmdDrawIndirect"))
        return (void*) vkCmdDrawIndirect;
    if (!strcmp(name, "CmdDrawIndexedIndirect"))
        return (void*) vkCmdDrawIndexedIndirect;
    if (!strcmp(name, "CmdDispatch"))
        return (void*) vkCmdDispatch;
    if (!strcmp(name, "CmdDispatchIndirect"))
        return (void*) vkCmdDispatchIndirect;
    if (!strcmp(name, "CmdCopyBuffer"))
        return (void*) vkCmdCopyBuffer;
    if (!strcmp(name, "CmdCopyImage"))
        return (void*) vkCmdCopyImage;
    if (!strcmp(name, "CmdBlitImage"))
        return (void*) vkCmdBlitImage;
    if (!strcmp(name, "CmdCopyBufferToImage"))
        return (void*) vkCmdCopyBufferToImage;
    if (!strcmp(name, "CmdCopyImageToBuffer"))
        return (void*) vkCmdCopyImageToBuffer;
    if (!strcmp(name, "CmdUpdateBuffer"))
        return (void*) vkCmdUpdateBuffer;
    if (!strcmp(name, "CmdFillBuffer"))
        return (void*) vkCmdFillBuffer;
    if (!strcmp(name, "CmdClearColorImage"))
        return (void*) vkCmdClearColorImage;
    if (!strcmp(name, "CmdClearDepthStencil"))
        return (void*) vkCmdClearDepthStencil;
    if (!strcmp(name, "CmdResolveImage"))
        return (void*) vkCmdResolveImage;
    if (!strcmp(name, "CmdSetEvent"))
        return (void*) vkCmdSetEvent;
    if (!strcmp(name, "CmdResetEvent"))
        return (void*) vkCmdResetEvent;
    if (!strcmp(name, "CmdWaitEvents"))
        return (void*) vkCmdWaitEvents;
    if (!strcmp(name, "CmdPipelineBarrier"))
        return (void*) vkCmdPipelineBarrier;
    if (!strcmp(name, "CmdBeginQuery"))
        return (void*) vkCmdBeginQuery;
    if (!strcmp(name, "CmdEndQuery"))
        return (void*) vkCmdEndQuery;
    if (!strcmp(name, "CmdResetQueryPool"))
        return (void*) vkCmdResetQueryPool;
    if (!strcmp(name, "CmdWriteTimestamp"))
        return (void*) vkCmdWriteTimestamp;
    if (!strcmp(name, "CmdCopyQueryPoolResults"))
        return (void*) vkCmdCopyQueryPoolResults;
    if (!strcmp(name, "CmdInitAtomicCounters"))
        return (void*) vkCmdInitAtomicCounters;
    if (!strcmp(name, "CmdLoadAtomicCounters"))
        return (void*) vkCmdLoadAtomicCounters;
    if (!strcmp(name, "CmdSaveAtomicCounters"))
        return (void*) vkCmdSaveAtomicCounters;
    if (!strcmp(name, "CreateFramebuffer"))
        return (void*) vkCreateFramebuffer;
    if (!strcmp(name, "CreateRenderPass"))
        return (void*) vkCreateRenderPass;
    if (!strcmp(name, "CmdBeginRenderPass"))
        return (void*) vkCmdBeginRenderPass;
    if (!strcmp(name, "CmdEndRenderPass"))
        return (void*) vkCmdEndRenderPass;
    if (!strcmp(name, "CmdDbgMarkerBegin"))
        return (void*) vkCmdDbgMarkerBegin;
    if (!strcmp(name, "CmdDbgMarkerEnd"))
        return (void*) vkCmdDbgMarkerEnd;
    if (!strcmp(name, "GetDisplayInfoWSI"))
        return (void*) vkGetDisplayInfoWSI;
    if (!strcmp(name, "CreateSwapChainWSI"))
        return (void*) vkCreateSwapChainWSI;
    if (!strcmp(name, "DestroySwapChainWSI"))
        return (void*) vkDestroySwapChainWSI;
    if (!strcmp(name, "GetSwapChainInfoWSI"))
        return (void*) vkGetSwapChainInfoWSI;
    if (!strcmp(name, "QueuePresentWSI"))
        return (void*) vkQueuePresentWSI;

    return NULL;
}

static inline void* layer_intercept_instance_proc(const char *name)
{
    if (!name || name[0] != 'v' || name[1] != 'k')
        return NULL;

    name += 2;
    if (!strcmp(name, "CreateInstance"))
        return (void*) vkCreateInstance;
    if (!strcmp(name, "DestroyInstance"))
        return (void*) vkDestroyInstance;
    if (!strcmp(name, "GetPhysicalDeviceInfo"))
        return (void*) vkGetPhysicalDeviceInfo;
    if (!strcmp(name, "CreateDevice"))
        return (void*) vkCreateDevice;
    if (!strcmp(name, "GetGlobalExtensionInfo"))
        return (void*) vkGetGlobalExtensionInfo;
    if (!strcmp(name, "GetPhysicalDeviceExtensionInfo"))
        return (void*) vkGetPhysicalDeviceExtensionInfo;
    if (!strcmp(name, "GetMultiDeviceCompatibility"))
        return (void*) vkGetMultiDeviceCompatibility;

    return NULL;
}

VK_LAYER_EXPORT void* VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    void* addr;
    if (device == NULL) {
        return NULL;
    }

    loader_platform_thread_once(&initOnce, initParamChecker);

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        initDeviceTable((const VkBaseLayerObject *) device);
        return (void*) vkGetDeviceProcAddr;
    }

    addr = layer_intercept_proc(funcName);
    if (addr) {
        return addr;
    }
    else {
        VkLayerDispatchTable **ppDisp = (VkLayerDispatchTable **) device;
        VkLayerDispatchTable* pTable = tableMap[*ppDisp];
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(device, funcName);
    }
}

VK_LAYER_EXPORT void* VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    void* addr;
    if (instance == NULL) {
        return NULL;
    }

    loader_platform_thread_once(&initOnce, initParamChecker);

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        initInstanceTable((const VkBaseLayerObject *) instance);
        return (void*) vkGetInstanceProcAddr;
    }

    addr = layer_intercept_instance_proc(funcName);
    if (addr) {
        return addr;
    }
    else {
        VkLayerInstanceDispatchTable **ppDisp = (VkLayerInstanceDispatchTable **) instance;
        VkLayerInstanceDispatchTable* pTable = tableInstanceMap[*ppDisp];
        if (pTable->GetInstanceProcAddr == NULL)
            return NULL;
        return pTable->GetInstanceProcAddr(instance, funcName);
    }
}
