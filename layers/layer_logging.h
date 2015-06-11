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
#include <stdbool.h>
#include <unordered_map>
#include "vkLayer.h"
#include "layer_data.h"
#include "layers_table.h"

typedef struct debug_report_data {
    VkLayerDbgFunctionNode *g_pDbgFunctionHead;
    bool g_DEBUG_REPORT;
} data_rec;

static std::unordered_map<void *, struct debug_report_data *> debug_report_data_map;

template data_rec *get_my_data_ptr<data_rec>(
        void *data_key,
        std::unordered_map<void *, struct debug_report_data *> data_map);

static inline void debug_report_init_instance_extension_dispatch_table(
        VkLayerInstanceDispatchTable *table,
        PFN_vkGetInstanceProcAddr gpa,
        VkInstance inst)
{
    table->DbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback) gpa(inst, "vkDbgCreateMsgCallback");
    table->DbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback) gpa(inst, "vkDbgDestroyMsgCallback");
}

// Utility function to handle reporting
static void debug_report_log_msg(
    struct debug_report_data   *debug_data,
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

static inline void debug_report_create_instance(
        VkLayerInstanceDispatchTable   *instance_dispatch_ptr,
        uint32_t                        extension_count,
        const VkExtensionProperties*    pEnabledExtensions)    // layer or extension name to be enabled
{
    data_rec *debug_data = get_my_data_ptr(instance_dispatch_ptr);

    for (uint32_t i = 0; i < extension_count; i++) {
        /* TODO: Check other property fields */
        if (strcmp(pEnabledExtensions[i].name, DEBUG_REPORT_EXTENSION_NAME) == 0) {
            debug_data->g_DEBUG_REPORT = true;
        }
    }
}

static inline void layer_debug_report_destroy_instance(VkInstance instance)
{
    VkLayerInstanceDispatchTable *instance_key = instance_dispatch_table(instance);
    struct debug_report_data *debug_data =
            get_debug_data_ptr(instance_dispatch_table(instance));
    VkLayerDbgFunctionNode *pTrav = debug_data->g_pDbgFunctionHead;
    VkLayerDbgFunctionNode *pTravNext;

    /* Clear out any leftover callbacks */
    while (pTrav) {
        pTravNext = pTrav->pNext;

        debug_report_log_msg(
                    debug_data, VK_DBG_REPORT_WARN_BIT,
                    VK_OBJECT_TYPE_INSTANCE, instance,
                    0, DEBUG_REPORT_CALLBACK_REF,
                    "DebugReport",
                    "Debug Report callbacks not removed before DestroyInstance");

        free(pTrav);
        pTrav = pTravNext;
    }
    debug_data->g_pDbgFunctionHead = NULL;
    debug_report_data_map.erase((void *) instance_key);
    tableInstanceMap.erase((void *) instance);
}

static inline void layer_debug_report_create_device(
        VkLayerInstanceDispatchTable   *instance_dispatch_ptr,
        VkDevice                        device)
{
    std::unordered_map<void *, struct debug_report_data *>::const_iterator got;

    got = debug_report_data_map.find((void *) instance_dispatch_ptr);

    if ( got == debug_report_data_map.end() ) {
        // If we get here something is wrong
        // We should always be able to find the instance key
        assert(true);
    } else {
        VkLayerDispatchTable *device_key = device_dispatch_table(device);
        debug_report_data_map[(void *) device_key ] = got->second;
    }
}

static inline void layer_debug_report_destroy_device(VkDevice device)
{
    VkLayerDispatchTable *device_key = device_dispatch_table(device);
    debug_report_data_map.erase((void *) device_key);
    tableMap.erase((void *) device);
}

static inline VkResult layer_create_msg_callback(
        VkInstance                      instance,
        VkLayerInstanceDispatchTable   *nextTable,
        VkFlags                         msgFlags,
        const PFN_vkDbgMsgCallback      pfnMsgCallback,
        void                           *pUserData,
        VkDbgMsgCallback               *pMsgCallback)
{
    VkLayerDbgFunctionNode *pNewDbgFuncNode = (VkLayerDbgFunctionNode*)malloc(sizeof(VkLayerDbgFunctionNode));
    if (!pNewDbgFuncNode)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    VkResult result = nextTable->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);

    if (result == VK_SUCCESS) {
        struct debug_report_data *debug_data =
                get_debug_data_ptr(instance_dispatch_table(instance));

        pNewDbgFuncNode->msgCallback = *pMsgCallback;
        pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
        pNewDbgFuncNode->msgFlags = msgFlags;
        pNewDbgFuncNode->pUserData = pUserData;
        pNewDbgFuncNode->pNext = debug_data->g_pDbgFunctionHead;

        debug_data->g_pDbgFunctionHead = pNewDbgFuncNode;

        debug_report_log_msg(
                    debug_data, VK_DBG_REPORT_DEBUG_BIT,
                    VK_OBJECT_TYPE_INSTANCE, instance,
                    0, DEBUG_REPORT_CALLBACK_REF,
                    "DebugReport",
                    "Added callback");
    } else {
        free(pNewDbgFuncNode);
    }
    return result;
}

static VkResult layer_destroy_msg_callback(
        VkInstance                      instance,
        VkLayerInstanceDispatchTable   *nextTable,
        VkDbgMsgCallback                msg_callback)
{
    struct debug_report_data *debug_data =
            get_debug_data_ptr(instance_dispatch_table(instance));
    VkLayerDbgFunctionNode *pTrav = debug_data->g_pDbgFunctionHead;
    VkLayerDbgFunctionNode *pPrev = pTrav;

    VkResult result = nextTable->DbgDestroyMsgCallback(instance, msg_callback);

    while (pTrav) {
        if (pTrav->msgCallback == msg_callback) {
            pPrev->pNext = pTrav->pNext;
            if (debug_data->g_pDbgFunctionHead == pTrav) {
                debug_data->g_pDbgFunctionHead = pTrav->pNext;
            }
            free(pTrav);
            debug_report_log_msg(
                        debug_data, VK_DBG_REPORT_DEBUG_BIT,
                        VK_OBJECT_TYPE_INSTANCE, instance,
                        0, DEBUG_REPORT_CALLBACK_REF,
                        "DebugReport",
                        "Removed callback");
            break;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }

    return result;
}

static void* debug_report_get_instance_proc_addr(
        VkInstance                      instance,
        const char                     *funcName)
{
    struct debug_report_data *debug_data =
            get_debug_data_ptr(instance_dispatch_table(instance));
    if (!debug_data->g_DEBUG_REPORT) {
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
 * Devices, Queue, SwapChain and Command buffers all
 * use the same device dispatch table.
 */
static void device_log_msg(
    VkObject                    object,
    VkFlags                     msgFlags,
    VkObjectType                objectType,
    VkObject                    srcObject,
    size_t                      location,
    int32_t                     msgCode,
    const char*                 pLayerPrefix,
    const char*                 pMsg)
{
    struct debug_report_data *debug_data;
    debug_data = get_device_debug_data_ptr(device_dispatch_table(object));
    debug_report_log_msg(debug_data, msgFlags, objectType,
                         srcObject, location, msgCode,
                         pLayerPrefix, pMsg);
}

static void instance_log_msg(
    VkInstance                  instance,
    VkFlags                     msgFlags,
    VkObjectType                objectType,
    VkObject                    srcObject,
    size_t                      location,
    int32_t                     msgCode,
    const char*                 pLayerPrefix,
    const char*                 pMsg)
{
    struct debug_report_data *debug_data;

    debug_data = get_instance_debug_data_ptr(instance_dispatch_table(instance));
    debug_report_log_msg(debug_data, msgFlags, objectType,
                         srcObject, location, msgCode,
                         pLayerPrefix, pMsg);
}

#endif // LAYER_LOGGING_H

