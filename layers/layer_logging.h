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
 *
 * Authors:
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#ifndef LAYER_LOGGING_H
#define LAYER_LOGGING_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unordered_map>
#include "vk_layer.h"
#include "layer_data.h"
#include "layers_table.h"

typedef struct _debug_report_data {
    VkLayerDbgFunctionNode *g_pDbgFunctionHead;
    VkFlags active_flags;
    bool g_DEBUG_REPORT;
} debug_report_data;

template debug_report_data *get_my_data_ptr<debug_report_data>(
        void *data_key,
        std::unordered_map<void *, debug_report_data *> &data_map);

// Utility function to handle reporting
static inline void debug_report_log_msg(
    debug_report_data          *debug_data,
    VkFlags                     msgFlags,
    VkObjectType                objectType,
    VkObject                    srcObject,
    size_t                      location,
    int32_t                     msgCode,
    const char*                 pLayerPrefix,
    const char*                 pMsg)
{
    VkLayerDbgFunctionNode *pTrav = debug_data->g_pDbgFunctionHead;
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

static inline debug_report_data *debug_report_create_instance(
        VkLayerInstanceDispatchTable   *table,
        VkInstance                      inst,
        uint32_t                        extension_count,
        const VkExtensionProperties*    pEnabledExtensions)    // layer or extension name to be enabled
{
    debug_report_data              *debug_data;
    PFN_vkGetInstanceProcAddr gpa = table->GetInstanceProcAddr;

    table->DbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback) gpa(inst, "vkDbgCreateMsgCallback");
    table->DbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback) gpa(inst, "vkDbgDestroyMsgCallback");

    debug_data = (debug_report_data *) malloc(sizeof(debug_report_data));
    if (!debug_data) return NULL;

    memset(debug_data, 0, sizeof(debug_report_data));
    for (uint32_t i = 0; i < extension_count; i++) {
        /* TODO: Check other property fields */
        if (strcmp(pEnabledExtensions[i].name, DEBUG_REPORT_EXTENSION_NAME) == 0) {
            debug_data->g_DEBUG_REPORT = true;
        }
    }
    return debug_data;
}

static inline void layer_debug_report_destroy_instance(debug_report_data *debug_data)
{
    VkLayerDbgFunctionNode *pTrav;
    VkLayerDbgFunctionNode *pTravNext;

    if (!debug_data) {
        return;
    }

    pTrav = debug_data->g_pDbgFunctionHead;
    /* Clear out any leftover callbacks */
    while (pTrav) {
        pTravNext = pTrav->pNext;

        debug_report_log_msg(
                    debug_data, VK_DBG_REPORT_WARN_BIT,
                    VK_OBJECT_TYPE_MSG_CALLBACK, pTrav->msgCallback,
                    0, DEBUG_REPORT_CALLBACK_REF,
                    "DebugReport",
                    "Debug Report callbacks not removed before DestroyInstance");

        free(pTrav);
        pTrav = pTravNext;
    }
    debug_data->g_pDbgFunctionHead = NULL;

    free(debug_data);
}

static inline debug_report_data *layer_debug_report_create_device(
        debug_report_data              *instance_debug_data,
        VkDevice                        device)
{
    /* DEBUG_REPORT shares data between Instance and Device,
     * so just return instance's data pointer */
    return instance_debug_data;
}

static inline void layer_debug_report_destroy_device(VkDevice device)
{
    /* Nothing to do since we're using instance data record */
}

static inline VkResult layer_create_msg_callback(
        debug_report_data              *debug_data,
        VkFlags                         msgFlags,
        const PFN_vkDbgMsgCallback      pfnMsgCallback,
        void                           *pUserData,
        VkDbgMsgCallback               *pMsgCallback)
{
    VkLayerDbgFunctionNode *pNewDbgFuncNode = (VkLayerDbgFunctionNode*)malloc(sizeof(VkLayerDbgFunctionNode));
    if (!pNewDbgFuncNode)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    pNewDbgFuncNode->msgCallback = *pMsgCallback;
    pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
    pNewDbgFuncNode->msgFlags = msgFlags;
    pNewDbgFuncNode->pUserData = pUserData;
    pNewDbgFuncNode->pNext = debug_data->g_pDbgFunctionHead;

    debug_data->g_pDbgFunctionHead = pNewDbgFuncNode;
    debug_data->active_flags |= msgFlags;

    debug_report_log_msg(
                debug_data, VK_DBG_REPORT_DEBUG_BIT,
                VK_OBJECT_TYPE_MSG_CALLBACK, *pMsgCallback,
                0, DEBUG_REPORT_CALLBACK_REF,
                "DebugReport",
                "Added callback");
    return VK_SUCCESS;
}

static inline void layer_destroy_msg_callback(
        debug_report_data              *debug_data,
        VkDbgMsgCallback                msg_callback)
{
    VkLayerDbgFunctionNode *pTrav = debug_data->g_pDbgFunctionHead;
    VkLayerDbgFunctionNode *pPrev = pTrav;

    debug_data->active_flags = 0;
    while (pTrav) {
        if (pTrav->msgCallback == msg_callback) {
            pPrev->pNext = pTrav->pNext;
            if (debug_data->g_pDbgFunctionHead == pTrav) {
                debug_data->g_pDbgFunctionHead = pTrav->pNext;
            }
            free(pTrav);
            debug_report_log_msg(
                        debug_data, VK_DBG_REPORT_DEBUG_BIT,
                        VK_OBJECT_TYPE_MSG_CALLBACK, pTrav->msgCallback,
                        0, DEBUG_REPORT_NONE,
                        "DebugReport",
                        "Destroyed callback");
        }
        debug_data->active_flags |= pTrav->msgFlags;
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }
}

static inline void* debug_report_get_instance_proc_addr(
        debug_report_data              *debug_data,
        const char                     *funcName)
{
    if (!debug_data || !debug_data->g_DEBUG_REPORT) {
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

/*
 * Output log message via DEBUG_REPORT
 * Takes format and variable arg list so that output string
 * is only computed if a message needs to be logged
 */
static inline void log_msg(
    debug_report_data          *debug_data,
    VkFlags                     msgFlags,
    VkObjectType                objectType,
    VkObject                    srcObject,
    size_t                      location,
    int32_t                     msgCode,
    const char*                 pLayerPrefix,
    const char*                 format,
    ...)
{
    if (!debug_data || !(debug_data->active_flags & msgFlags)) {
        /* message is not wanted */
        return;
    }

    char str[1024];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(str, 1024, format, argptr);
    va_end(argptr);
    debug_report_log_msg(debug_data, msgFlags, objectType,
                         srcObject, location, msgCode,
                         pLayerPrefix, str);
}

static inline void VKAPI log_callback(
    VkFlags                             msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData)
{
    char msg_flags[30];

    print_msg_flags(msgFlags, msg_flags);

    fprintf((FILE *) pUserData, "%s(%s): object: 0x%p type: %d location: %zu msgCode: %d: %s\n",
             pLayerPrefix, msg_flags, srcObject, objType, location, msgCode, pMsg);
}
#endif // LAYER_LOGGING_H
