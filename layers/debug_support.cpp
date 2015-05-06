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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unordered_map>
#include "loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vkLayer.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"

static std::unordered_map<void *, VkLayerDispatchTable *> tableMap;

static VkLayerDispatchTable * initLayerTable(const VkBaseLayerObject *gpuw)
{
    VkLayerDispatchTable *pTable;

    assert(gpuw);
    std::unordered_map<void *, VkLayerDispatchTable *>::const_iterator it = tableMap.find((void *) gpuw->baseObject);
    if (it == tableMap.end())
    {
        pTable =  new VkLayerDispatchTable;
        tableMap[(void *) gpuw->baseObject] = pTable;
    } else
    {
        return it->second;
    }

    layer_initialize_dispatch_table(pTable, gpuw->pGPA, (VkPhysicalDevice) gpuw->nextObject);

    return pTable;
}

VK_LAYER_EXPORT VkResult VKAPI vkLayerExtension1(VkDevice device)
{
    printf("In vkLayerExtension1() call w/ device: %p\n", (void*)device);
    printf("vkLayerExtension1 returning SUCCESS\n");
    return VK_SUCCESS;
}

struct extProps {
    uint32_t version;
    const char * const name;
};
#define DEBUG_SUPPORT_LAYER_EXT_ARRAY_SIZE 2
static const struct extProps debugReportExts[DEBUG_SUPPORT_LAYER_EXT_ARRAY_SIZE] = {
    0x10, "DebugReport",
    0x10, "DebugMarker",
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    /* This entrypoint is NOT going to init it's own dispatch table since loader calls here early */
    VkExtensionProperties *ext_props;
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = DEBUG_SUPPORT_LAYER_EXT_ARRAY_SIZE;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= DEBUG_SUPPORT_LAYER_EXT_ARRAY_SIZE)
                return VK_ERROR_INVALID_VALUE;
            ext_props = (VkExtensionProperties *) pData;
            ext_props->version = debugReportExts[extensionIndex].version;
            strncpy(ext_props->extName, debugReportExts[extensionIndex].name,
                                        VK_MAX_EXTENSION_NAME);
            ext_props->extName[VK_MAX_EXTENSION_NAME - 1] = '\0';
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

static VkResult VKAPI DbgCreateMsgCallback(
    VkInstance                          instance,
    VkFlags                             msgFlags,
    const PFN_vkDbgMsgCallback          pfnMsgCallback,
    const void*                         pUserData,
    const VkDbgMsgCallback*             pMsgCallback)
{

}

// DebugReport utility callback functions
static void VKAPI StringCallback(
    VkFlags                             msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData)
{

}

static void VKAPI StdioCallback(
    VkFlags                             msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData)
{

}

static void VKAPI BreakCallback(
    VkFlags                             msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData)
{

}

// DebugMarker extension entrypoints
static void VKAPI CmdDbgMarkerBegin(
    VkCmdBuffer                         cmdBuffer,
    const char*                         pMarker)
{

}

static void VKAPI CmdDbgMarkerEnd(
    VkCmdBuffer                         cmdBuffer)
{

}

static VkResult VKAPI DbgSetObjectTag(
    VkDevice                            device,
    VkObjectType                        objType,
    VkObject                            object,
    size_t                              tagSize,
    const void*                         pTag)
{

}

static VkResult VKAPI DbgSetObjectName(
    VkDevice                            device,
    VkObjectType                        objType,
    VkObject                            object,
    size_t                              nameSize,
    const char*                         pName)
{

}

VK_LAYER_EXPORT void * VKAPI vkGetProcAddr(VkPhysicalDevice gpu, const char* pName)
{
    if (gpu == NULL)
        return NULL;

    initLayerTable((const VkBaseLayerObject *) gpu);

    if (!strncmp("vkGetProcAddr", pName, sizeof("vkGetProcAddr")))
        return (void *) vkGetProcAddr;
    else if (!strcmp("vkDbgCreateMsgCallback", pName))
        return (void *) DbgCreateMsgCallback;
    else if (!strcmp("vkStringCallback", pName))
        return (void *) StringCallback;
    else if (!strcmp("vkStdioCallback", pName))
        return (void *) StdioCallback;
    else if (!strcmp("vkBreakCallback", pName))
        return (void *) BreakCallback;
    else if (!strcmp("vkCmdDbgMarkerBegin", pName))
        return (void *) CmdDbgMarkerBegin;
    else if (!strcmp("vkCmdDbgMarkerEnd", pName))
        return (void *) CmdDbgMarkerEnd;
    else if (!strcmp("vkDbgSetObjectTag", pName))
        return (void *) DbgSetObjectTag;
    else if (!strcmp("vkDbgSetObjectName", pName))
        return (void *) DbgSetObjectName;
    else {
        VkBaseLayerObject* gpuw = (VkBaseLayerObject *) gpu;
        if (gpuw->pGPA == NULL)
            return NULL;
        return gpuw->pGPA((VkPhysicalDevice) gpuw->nextObject, pName);
    }
}
