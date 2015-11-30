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
 * Author: Chia-I Wu <olv@lunarg.com>
 *
 */

#define _ISOC11_SOURCE /* for aligned_alloc() */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "icd-instance.h"

static VKAPI_ATTR void * VKAPI_CALL default_alloc(void *user_data, size_t size,
                                   size_t alignment,
                                   VkSystemAllocationScope allocationScope)
{
    if (alignment <= 1) {
        return malloc(size);
    } else if (u_is_pow2((unsigned int) alignment)) {
        if (alignment < sizeof(void *)) {
            assert(u_is_pow2(sizeof(void*)));
            alignment = sizeof(void *);
        }

        size = (size + alignment - 1) & ~(alignment - 1);

#if defined(_WIN32)
        return _aligned_malloc(alignment, size);
#else
        return aligned_alloc(alignment, size);
#endif
    }
    else {
        return NULL;
    }
}

static VKAPI_ATTR void VKAPI_CALL default_free(void *user_data, void *ptr)
{
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

struct icd_instance *icd_instance_create(const VkApplicationInfo *app_info,
                                         const VkAllocationCallbacks *alloc_cb)
{
    static const VkAllocationCallbacks default_alloc_cb = {
        .pfnAllocation = default_alloc,
        .pfnFree = default_free,
    };
    struct icd_instance *instance;
    const char *name;
    size_t len;

    if (!alloc_cb)
        alloc_cb = &default_alloc_cb;

    instance = alloc_cb->pfnAllocation(alloc_cb->pUserData, sizeof(*instance), 0,
            VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
    if (!instance)
        return NULL;

    memset(instance, 0, sizeof(*instance));

    name = (app_info && app_info->pApplicationName) ? app_info->pApplicationName : "unnamed";
    len = strlen(name);
    instance->name = alloc_cb->pfnAllocation(alloc_cb->pUserData, len + 1, 0,
            VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
    if (!instance->name) {
        alloc_cb->pfnFree(alloc_cb->pUserData, instance);
        return NULL;
    }

    memcpy(instance->name, name, len);
    instance->name[len] = '\0';

    instance->alloc_cb = *alloc_cb;

    return instance;
}

void icd_instance_destroy(struct icd_instance *instance)
{
    struct icd_instance_logger *logger;

    for (logger = instance->loggers; logger; ) {
        struct icd_instance_logger *next = logger->next;

        icd_instance_free(instance, logger);
        logger = next;
    }

    icd_instance_free(instance, instance->name);
    icd_instance_free(instance, instance);
}

VkResult icd_instance_create_logger(
        struct icd_instance *instance,
        VkDebugReportCallbackCreateInfoLUNARG *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugReportCallbackLUNARG *msg_obj)
{
    struct icd_instance_logger *logger;

    /* TODOVV: Move this test to a validation layer */
//    if (msg_obj == NULL) {
//        return VK_ERROR_INVALID_POINTER;
//    }

    logger = icd_instance_alloc(instance, sizeof(*logger), 0,
            VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
    if (!logger)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    logger->func = pCreateInfo->pfnCallback;
    logger->flags = pCreateInfo->flags;
    logger->next = instance->loggers;
    instance->loggers = logger;

    logger->user_data = pCreateInfo->pUserData;

    *( struct icd_instance_logger **)msg_obj = logger;

    return VK_SUCCESS;
}

void icd_instance_destroy_logger(
        struct icd_instance *instance,
        const VkDebugReportCallbackLUNARG msg_obj,
        const VkAllocationCallbacks *pAllocator)
{
    struct icd_instance_logger *logger, *prev;
    VkDebugReportCallbackLUNARG local_msg_obj = msg_obj;

    for (prev = NULL, logger = instance->loggers; logger;
         prev = logger, logger = logger->next) {
        if (logger == *(struct icd_instance_logger **) &local_msg_obj)
            break;
    }

    /* TODOVV: Move this to validation layer */
//    if (!logger)
//        return VK_ERROR_INVALID_POINTER;

    if (prev)
        prev->next = logger->next;
    else
        instance->loggers = logger->next;

    icd_instance_free(instance, logger);
}

void icd_instance_log(const struct icd_instance *instance,
                      VkFlags msg_flags,
                      VkDebugReportObjectTypeLUNARG obj_type,
                      uint64_t src_object,
                      size_t location,
                      int32_t msg_code,
                      const char *msg)
{
    const struct icd_instance_logger *logger;

    if (!instance->loggers) {
        fputs(msg, stderr);
        fputc('\n', stderr);
        return;
    }

    for (logger = instance->loggers; logger; logger = logger->next) {
        if (msg_flags & logger->flags) {
            logger->func(msg_flags, obj_type, (uint64_t)src_object, location,
                         msg_code, instance->name, msg, logger->user_data);
        }
    }
}
