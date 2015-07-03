/*
 * Vulkan
 *
 * Copyright (C) 2014-2015 LunarG, Inc.
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
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ICD_INSTANCE_H
#define ICD_INSTANCE_H

#include "icd-utils.h"
#include "icd.h"

#ifdef __cplusplus
extern "C" {
#endif

struct icd_instance_logger {
    PFN_vkDbgMsgCallback func;
    void *user_data;
    VkFlags flags;

    struct icd_instance_logger *next;
};

struct icd_instance {
    char *name;

    VkAllocCallbacks alloc_cb;

    struct icd_instance_logger *loggers;
};

struct icd_instance *icd_instance_create(const VkApplicationInfo *app_info,
                                         const VkAllocCallbacks *alloc_cb);
void icd_instance_destroy(struct icd_instance *instance);

static inline void *icd_instance_alloc(const struct icd_instance *instance,
                                       size_t size, size_t alignment,
                                       VkSystemAllocType type)
{
    return instance->alloc_cb.pfnAlloc(instance->alloc_cb.pUserData,
            size, alignment, type);
}

static inline void icd_instance_free(const struct icd_instance *instance,
                                     void *ptr)
{
    instance->alloc_cb.pfnFree(instance->alloc_cb.pUserData, ptr);
}

VkResult icd_instance_create_logger(struct icd_instance *instance,
        VkFlags msg_flags,
        PFN_vkDbgMsgCallback func,
        void *user_data,
        VkDbgMsgCallback *msg_obj);

VkResult icd_instance_destroy_logger(
        struct icd_instance *instance,
        const VkDbgMsgCallback msg_obj);

void icd_instance_log(const struct icd_instance *instance,
                      VkFlags msg_flags,
                      VkDbgObjectType obj_type,
                      uint64_t src_object,
                      size_t location, int32_t msg_code,
                      const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* ICD_INSTANCE_H */
