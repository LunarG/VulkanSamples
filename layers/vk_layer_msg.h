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
#include <stdbool.h>
#include "vk_layer.h"

static VkLayerDbgFunctionNode *g_pDbgFunctionHead = NULL;
static VkFlags g_reportFlags = (VkFlags) 0;
static VkLayerDbgAction g_debugAction = VK_DBG_LAYER_ACTION_LOG_MSG;
static bool g_actionIsDefault = true;
static bool g_DEBUG_REPORT = false;
static FILE *g_logFile = NULL;

static void enable_debug_report(
        uint32_t                extension_count,
        const char * const *    ppEnabledExtensionNames)    // extension name to be enabled
{
    for (uint32_t i = 0; i < extension_count; i++) {
        /* TODO: Check other property fields */
        if (strcmp(ppEnabledExtensionNames[i], DEBUG_REPORT_EXTENSION_NAME) == 0) {
            g_DEBUG_REPORT = true;
        }
    }
}

static VkResult layer_create_msg_callback(
        VkInstance instance,
        VkLayerInstanceDispatchTable* nextTable,
        VkFlags msgFlags,
        const PFN_vkDbgMsgCallback pfnMsgCallback,
        void* pUserData,
        VkDbgMsgCallback* pMsgCallback)
{
    VkLayerDbgFunctionNode *pNewDbgFuncNode = (VkLayerDbgFunctionNode*)malloc(sizeof(VkLayerDbgFunctionNode));
    if (!pNewDbgFuncNode)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    VkResult result = nextTable->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    if (result == VK_SUCCESS) {
        pNewDbgFuncNode->msgCallback = *pMsgCallback;
        pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
        pNewDbgFuncNode->msgFlags = msgFlags;
        pNewDbgFuncNode->pUserData = pUserData;
        pNewDbgFuncNode->pNext = g_pDbgFunctionHead;

        /* TODO: This should be a per-instance resource */
        g_pDbgFunctionHead = pNewDbgFuncNode;
    } else {
        free(pNewDbgFuncNode);
    }
    return result;
}

static VkResult layer_destroy_msg_callback(
        VkInstance instance,
        VkLayerInstanceDispatchTable *nextTable,
        VkDbgMsgCallback msg_callback)
{
    VkLayerDbgFunctionNode *pTrav = g_pDbgFunctionHead;
    VkLayerDbgFunctionNode *pPrev = pTrav;

    VkResult result = nextTable->DbgDestroyMsgCallback(instance, msg_callback);

    while (pTrav) {
        if (pTrav->msgCallback == msg_callback) {
            pPrev->pNext = pTrav->pNext;
            if (g_pDbgFunctionHead == pTrav)
                g_pDbgFunctionHead = pTrav->pNext;
            free(pTrav);
            break;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }

    return result;
}

static inline void debug_report_init_instance_extension_dispatch_table(
        VkLayerInstanceDispatchTable *table,
        PFN_vkGetInstanceProcAddr gpa,
        VkInstance inst)
{
    table->DbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback) gpa(inst, "vkDbgCreateMsgCallback");
    table->DbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback) gpa(inst, "vkDbgDestroyMsgCallback");
}

static void* msg_callback_get_proc_addr(
        const char      *funcName)
{
    if (!g_DEBUG_REPORT) {
        return NULL;
    }

    if (!strcmp(funcName, "vkDbgCreateMsgCallback")) {
        return (void *) vkDbgCreateMsgCallback;
    }
    if (!strcmp(funcName, "vkDbgDestroyMsgCallback")) {
        return (void *) vkDbgDestroyMsgCallback;
    }

    return NULL;
}

// Utility function to handle reporting
//  If callbacks are enabled, use them, otherwise use printf
static void layerCbMsg(
    VkFlags             msgFlags,
    VkObjectType        objectType,
    VkObject            srcObject,
    size_t              location,
    int32_t             msgCode,
    const char*         pLayerPrefix,
    const char*         pMsg)
{
    if (g_logFile == NULL) {
	g_logFile = stdout;
    }

    VkLayerDbgFunctionNode *pTrav = g_pDbgFunctionHead;
    while (pTrav) {
        if (pTrav->msgFlags & msgFlags) {
            pTrav->pfnMsgCallback(msgFlags,
                                  objectType, srcObject,
                                  location,
                                  msgCode,
                                  pLayerPrefix,
                                  pMsg,
                                  (void *) pTrav->pUserData);
        }
        pTrav = pTrav->pNext;
    }
}
