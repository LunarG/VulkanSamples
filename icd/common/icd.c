/*
 * XGL
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
 *   Chia-I Wu <olv@lunarg.com>
 */

#define _ISOC11_SOURCE /* for aligned_alloc */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <xgl.h>
#include <xglDbg.h>

#include "icd.h"

struct icd_msg_callback {
    XGL_DBG_MSG_CALLBACK_FUNCTION func;
    XGL_VOID *data;

    struct icd_msg_callback *next;
};

struct icd {
    struct icd_msg_callback *msg_callbacks;

    bool debug_echo_enable;
    bool break_on_error;
    bool break_on_warning;

    XGL_ALLOC_CALLBACKS alloc_callbacks;
    int init_count;
};

static XGL_VOID *default_alloc(XGL_VOID *user_data,
                               XGL_SIZE size,
                               XGL_SIZE alignment,
                               XGL_SYSTEM_ALLOC_TYPE allocType);

static XGL_VOID default_free(XGL_VOID *user_data,
                             XGL_VOID *mem);

static struct icd icd = {
    .alloc_callbacks = {
        .pfnAlloc = default_alloc,
        .pfnFree = default_free,
    }
};

void icd_msg(XGL_DBG_MSG_TYPE msg_type,
             XGL_VALIDATION_LEVEL validation_level,
             XGL_BASE_OBJECT src_object,
             XGL_SIZE location,
             XGL_INT msg_code,
             const char *msg)
{
    const struct icd_msg_callback *cb = icd.msg_callbacks;

    if (icd.debug_echo_enable || !cb) {
        fputs(msg, stderr);
        fputc('\n', stderr);
    }

    while (cb) {
        cb->func(msg_type, XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0,
                msg_code, (const XGL_CHAR *) msg, cb->data);
        cb = cb->next;
    }

    switch (msg_type) {
    case XGL_DBG_MSG_ERROR:
        if (icd.break_on_error) {
            exit(1);
        }
        /* fall through */
    case XGL_DBG_MSG_WARNING:
        if (icd.break_on_warning) {
            exit(1);
        }
        break;
    default:
        break;
    }
}

void icd_vlog(XGL_DBG_MSG_TYPE msg_type,
              XGL_VALIDATION_LEVEL validation_level,
              XGL_BASE_OBJECT src_object,
              XGL_SIZE location,
              XGL_INT msg_code,
              const char *format, va_list ap)
{
    char msg[256];
    int ret;

    ret = vsnprintf(msg, sizeof(msg), format, ap);
    if (ret >= sizeof(msg) || ret < 0) {
        msg[sizeof(msg) - 1] = '\0';
    }

    icd_msg(msg_type, validation_level, src_object, location, msg_code, msg);
}

void icd_log(XGL_DBG_MSG_TYPE msg_type,
             XGL_VALIDATION_LEVEL validation_level,
             XGL_BASE_OBJECT src_object,
             XGL_SIZE location,
             XGL_INT msg_code,
             const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    icd_vlog(msg_type, validation_level, src_object,
            location, msg_code, format, ap);
    va_end(ap);
}

void icd_clear_msg_callbacks(void)
{
    struct icd_msg_callback *cb = icd.msg_callbacks;

    while (cb) {
        struct icd_msg_callback *next = cb->next;
        free(cb);
        cb = next;
    }

    icd.msg_callbacks = NULL;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData)
{
    struct icd_msg_callback *cb;

    cb = malloc(sizeof(*cb));
    if (!cb)
        return XGL_ERROR_OUT_OF_MEMORY;

    cb->func = pfnMsgCallback;
    cb->data = pUserData;

    cb->next = icd.msg_callbacks;
    icd.msg_callbacks = cb;

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    struct icd_msg_callback *cb = icd.msg_callbacks;

    /*
     * Find the first match (last registered).
     *
     * XXX What if the same callback function is registered more than once?
     */
    while (cb) {
        if (cb->func == pfnMsgCallback) {
            break;
        }

        cb = cb->next;
    }

    if (!cb)
        return XGL_ERROR_INVALID_POINTER;

    free(cb);

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData)
{
    XGL_RESULT res = XGL_SUCCESS;

    if (dataSize == 0)
        return XGL_ERROR_INVALID_VALUE;

    switch (dbgOption) {
    case XGL_DBG_OPTION_DEBUG_ECHO_ENABLE:
        icd.debug_echo_enable = *((const bool *) pData);
        break;
    case XGL_DBG_OPTION_BREAK_ON_ERROR:
        icd.break_on_error = *((const bool *) pData);
        break;
    case XGL_DBG_OPTION_BREAK_ON_WARNING:
        icd.break_on_warning = *((const bool *) pData);
        break;
    default:
        res = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return res;
}

XGL_RESULT icd_set_allocator(const XGL_ALLOC_CALLBACKS *alloc_cb)
{
    if (icd.init_count) {
        const XGL_ALLOC_CALLBACKS default_cb = {
            NULL, default_alloc, default_free
        };

        if (!alloc_cb)
            alloc_cb = &default_cb;

        /*
         * The spec says: Changing the callbacks on subsequent calls to
         * xglInitAndEnumerateGpus() causes it to fail with
         * XGL_ERROR_INVALID_POINTER error.
         */
        return (memcmp(&icd.alloc_callbacks, alloc_cb, sizeof(*alloc_cb))) ?
            XGL_ERROR_INVALID_POINTER : XGL_SUCCESS;
    }

    if (alloc_cb)
        icd.alloc_callbacks = *alloc_cb;

    icd.init_count++;

    return XGL_SUCCESS;
}

int icd_get_allocator_id(void)
{
    return icd.init_count;
}

void *icd_alloc(XGL_SIZE size, XGL_SIZE alignment,
                XGL_SYSTEM_ALLOC_TYPE type)
{
    return icd.alloc_callbacks.pfnAlloc(icd.alloc_callbacks.pUserData,
            size, alignment, type);
}

void icd_free(void *mem)
{
    icd.alloc_callbacks.pfnFree(icd.alloc_callbacks.pUserData, mem);
}

static bool is_power_of_two(XGL_SIZE v)
{
    return !(v & (v - 1));
}

static XGL_VOID *default_alloc(XGL_VOID *user_data,
                               XGL_SIZE size,
                               XGL_SIZE alignment,
                               XGL_SYSTEM_ALLOC_TYPE allocType)
{
    if (alignment <= 1) {
        return malloc(size);
    } else if (is_power_of_two(alignment)) {
        if (alignment < sizeof(void *)) {
            assert(is_power_of_two(sizeof(void *)));
            alignment = sizeof(void *);
        }

        size = (size + alignment - 1) & ~(alignment - 1);

        return aligned_alloc(alignment, size);
    }
    else {
        return NULL;
    }
}

static XGL_VOID default_free(XGL_VOID *user_data,
                             XGL_VOID *mem)
{
    free(mem);
}
