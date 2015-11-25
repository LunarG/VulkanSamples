/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#ifndef WIN32
#include <signal.h>
#else
#endif
#include "vk_loader_platform.h"
#include "debug_report.h"
#include "vulkan/vk_layer.h"

typedef void (VKAPI_PTR *PFN_stringCallback)(char *message);

static const VkExtensionProperties debug_report_extension_info = {
        .extensionName = VK_EXT_LUNARG_DEBUG_REPORT_EXTENSION_NAME,
        .specVersion = VK_EXT_LUNARG_DEBUG_REPORT_EXTENSION_REVISION,
};

void debug_report_add_instance_extensions(
        const struct loader_instance *inst,
        struct loader_extension_list *ext_list)
{
    loader_add_to_ext_list(inst, ext_list, 1, &debug_report_extension_info);
}

void debug_report_create_instance(
        struct loader_instance *ptr_instance,
        const VkInstanceCreateInfo *pCreateInfo)
{
    ptr_instance->debug_report_enabled = false;

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_LUNARG_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            ptr_instance->debug_report_enabled = true;
            return;
        }
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL debug_report_DbgCreateMsgCallback(
        VkInstance instance,
        VkFlags msgFlags,
        const PFN_vkDbgMsgCallback pfnMsgCallback,
        void* pUserData,
        VkDbgMsgCallback* pMsgCallback)
{
    VkLayerDbgFunctionNode *pNewDbgFuncNode = (VkLayerDbgFunctionNode *) loader_heap_alloc((struct loader_instance *)instance, sizeof(VkLayerDbgFunctionNode), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (!pNewDbgFuncNode)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    struct loader_instance *inst = loader_get_instance(instance);
    loader_platform_thread_lock_mutex(&loader_lock);
    VkResult result = inst->disp->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    if (result == VK_SUCCESS) {
        pNewDbgFuncNode->msgCallback = *pMsgCallback;
        pNewDbgFuncNode->pfnMsgCallback = pfnMsgCallback;
        pNewDbgFuncNode->msgFlags = msgFlags;
        pNewDbgFuncNode->pUserData = pUserData;
        pNewDbgFuncNode->pNext = inst->DbgFunctionHead;
        inst->DbgFunctionHead = pNewDbgFuncNode;
    } else {
        loader_heap_free((struct loader_instance *) instance, pNewDbgFuncNode);
    }
    loader_platform_thread_unlock_mutex(&loader_lock);
    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL debug_report_DbgDestroyMsgCallback(
        VkInstance instance,
        VkDbgMsgCallback msg_callback)
{
    struct loader_instance *inst = loader_get_instance(instance);
    loader_platform_thread_lock_mutex(&loader_lock);
    VkLayerDbgFunctionNode *pTrav = inst->DbgFunctionHead;
    VkLayerDbgFunctionNode *pPrev = pTrav;

    VkResult result = inst->disp->DbgDestroyMsgCallback(instance, msg_callback);

    while (pTrav) {
        if (pTrav->msgCallback == msg_callback) {
            pPrev->pNext = pTrav->pNext;
            if (inst->DbgFunctionHead == pTrav)
                inst->DbgFunctionHead = pTrav->pNext;
            loader_heap_free((struct loader_instance *) instance, pTrav);
            break;
        }
        pPrev = pTrav;
        pTrav = pTrav->pNext;
    }

    loader_platform_thread_unlock_mutex(&loader_lock);
    return result;
}


/*
 * This is the instance chain terminator function
 * for DbgCreateMsgCallback
 */

VKAPI_ATTR VkResult VKAPI_CALL loader_DbgCreateMsgCallback(
        VkInstance                          instance,
        VkFlags                             msgFlags,
        const PFN_vkDbgMsgCallback          pfnMsgCallback,
        void*                               pUserData,
        VkDbgMsgCallback*                   pMsgCallback)
{
    VkDbgMsgCallback *icd_info;
    const struct loader_icd *icd;
    struct loader_instance *inst;
    VkResult res;
    uint32_t storage_idx;

    for (inst = loader.instances; inst; inst = inst->next) {
        if ((VkInstance) inst == instance)
            break;
    }

    icd_info = calloc(sizeof(VkDbgMsgCallback), inst->total_icd_count);
    if (!icd_info) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    storage_idx = 0;
    for (icd = inst->icds; icd; icd = icd->next) {
        if (!icd->DbgCreateMsgCallback) {
            continue;
        }

        res = icd->DbgCreateMsgCallback(
                  icd->instance,
                  msgFlags,
                  pfnMsgCallback,
                  pUserData,
                  &icd_info[storage_idx]);

        if (res != VK_SUCCESS) {
            break;
        }
        storage_idx++;
    }

    /* roll back on errors */
    if (icd) {
        storage_idx = 0;
        for (icd = inst->icds; icd; icd = icd->next) {
            if (icd_info[storage_idx]) {
                icd->DbgDestroyMsgCallback(
                      icd->instance,
                      icd_info[storage_idx]);
            }
            storage_idx++;
        }

        return res;
    }

    *(VkDbgMsgCallback **)pMsgCallback = icd_info;

    return VK_SUCCESS;
}

/*
 * This is the instance chain terminator function
 * for DbgDestroyMsgCallback
 */
VKAPI_ATTR VkResult VKAPI_CALL loader_DbgDestroyMsgCallback(
        VkInstance instance,
        VkDbgMsgCallback msgCallback)
{
    uint32_t storage_idx;
    VkDbgMsgCallback *icd_info;
    const struct loader_icd *icd;
    VkResult res = VK_SUCCESS;
    struct loader_instance *inst;

    for (inst = loader.instances; inst; inst = inst->next) {
        if ((VkInstance) inst == instance)
            break;
    }

    icd_info = *(VkDbgMsgCallback **) &msgCallback;
    storage_idx = 0;
    for (icd = inst->icds; icd; icd = icd->next) {
        if (icd_info[storage_idx]) {
            icd->DbgDestroyMsgCallback(
                  icd->instance,
                  icd_info[storage_idx]);
        }
        storage_idx++;
    }
    return res;
}

bool debug_report_instance_gpa(
        struct loader_instance *ptr_instance,
        const char* name,
        void **addr)
{
    // debug_report is currently advertised to be supported by the loader,
    // so always return the entry points if name matches and it's enabled
    *addr = NULL;

    if (!strcmp("vkDbgCreateMsgCallback", name)) {
        *addr = ptr_instance->debug_report_enabled ? (void *) debug_report_DbgCreateMsgCallback : NULL;
        return true;
    }
    if (!strcmp("vkDbgDestroyMsgCallback", name)) {
        *addr = ptr_instance->debug_report_enabled ? (void *) debug_report_DbgDestroyMsgCallback : NULL;
        return true;
    }
    return false;
}
