/*
 * XGL
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
    XGL_DBG_MSG_CALLBACK_FUNCTION func;
    void *user_data;

    struct icd_instance_logger *next;
};

struct icd_instance {
    char *name;

    bool debug_echo_enable;
    bool break_on_error;
    bool break_on_warning;

    XGL_ALLOC_CALLBACKS alloc_cb;

    struct icd_instance_logger *loggers;
};

struct icd_instance *icd_instance_create(const XGL_APPLICATION_INFO *app_info,
                                         const XGL_ALLOC_CALLBACKS *alloc_cb);
void icd_instance_destroy(struct icd_instance *instance);

XGL_RESULT icd_instance_set_bool(struct icd_instance *instance,
                                 XGL_DBG_GLOBAL_OPTION option, bool yes);

static inline void *icd_instance_alloc(const struct icd_instance *instance,
                                       size_t size, size_t alignment,
                                       XGL_SYSTEM_ALLOC_TYPE type)
{
    return instance->alloc_cb.pfnAlloc(instance->alloc_cb.pUserData,
            size, alignment, type);
}

static inline void icd_instance_free(const struct icd_instance *instance,
                                     void *ptr)
{
    instance->alloc_cb.pfnFree(instance->alloc_cb.pUserData, ptr);
}

XGL_RESULT icd_instance_add_logger(struct icd_instance *instance,
                                   XGL_DBG_MSG_CALLBACK_FUNCTION func,
                                   void *user_data);
XGL_RESULT icd_instance_remove_logger(struct icd_instance *instance,
                                      XGL_DBG_MSG_CALLBACK_FUNCTION func);

void icd_instance_log(const struct icd_instance *instance,
                      XGL_DBG_MSG_TYPE msg_type,
                      XGL_VALIDATION_LEVEL validation_level,
                      XGL_BASE_OBJECT src_object,
                      size_t location, int32_t msg_code,
                      const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* ICD_INSTANCE_H */
