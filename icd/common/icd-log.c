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

#include <stdlib.h>
#include <stdio.h>
#include "icd-log.h"

struct icd_logger_callback {
    XGL_DBG_MSG_CALLBACK_FUNCTION func;
    void *user_data;

    struct icd_logger_callback *next;
};

struct icd_logger {
    bool debug_echo_enable;
    bool break_on_error;
    bool break_on_warning;

    struct icd_logger_callback *callbacks;
};

static struct icd_logger icd_logger;

XGL_RESULT icd_logger_set_bool(XGL_DBG_GLOBAL_OPTION option, bool enable)
{
    XGL_RESULT res = XGL_SUCCESS;

    switch (option) {
    case XGL_DBG_OPTION_DEBUG_ECHO_ENABLE:
        icd_logger.debug_echo_enable = enable;
        break;
    case XGL_DBG_OPTION_BREAK_ON_ERROR:
        icd_logger.break_on_error = enable;
        break;
    case XGL_DBG_OPTION_BREAK_ON_WARNING:
        icd_logger.break_on_warning = enable;
        break;
    default:
        res = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return res;
}

XGL_RESULT icd_logger_add_callback(XGL_DBG_MSG_CALLBACK_FUNCTION func,
                                   void *user_data)
{
    struct icd_logger_callback *cb;

    /* use malloc() as allocator may not be initialized yet */
    cb = malloc(sizeof(*cb));
    if (!cb)
        return XGL_ERROR_OUT_OF_MEMORY;

    cb->func = func;
    cb->user_data = user_data;

    cb->next = icd_logger.callbacks;
    icd_logger.callbacks = cb;

    return XGL_SUCCESS;
}

XGL_RESULT icd_logger_remove_callback(XGL_DBG_MSG_CALLBACK_FUNCTION func)
{
    struct icd_logger_callback *cb = icd_logger.callbacks;
    bool found = false;

    /* remove all matches */
    while (cb) {
        struct icd_logger_callback *next = cb->next;

        if (cb->func == func) {
            free(cb);
            found = true;
        }

        cb = next;
    }

    return (found) ? XGL_SUCCESS : XGL_ERROR_INVALID_POINTER;
}

void icd_logger_clear_callbacks(void)
{
    struct icd_logger_callback *cb = icd_logger.callbacks;

    while (cb) {
        struct icd_logger_callback *next = cb->next;
        free(cb);
        cb = next;
    }

    icd_logger.callbacks = NULL;
}

static void icd_log_str(XGL_DBG_MSG_TYPE msg_type,
                        XGL_VALIDATION_LEVEL validation_level,
                        XGL_BASE_OBJECT src_object,
                        size_t location, int32_t msg_code,
                        const char *msg)
{
    const struct icd_logger_callback *cb = icd_logger.callbacks;

    if (icd_logger.debug_echo_enable || !cb) {
        fputs(msg, stderr);
        fputc('\n', stderr);
    }

    while (cb) {
        cb->func(msg_type, XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0,
                msg_code, msg, cb->user_data);
        cb = cb->next;
    }

    switch (msg_type) {
    case XGL_DBG_MSG_ERROR:
        if (icd_logger.break_on_error)
            abort();
        /* fall through */
    case XGL_DBG_MSG_WARNING:
        if (icd_logger.break_on_warning)
            abort();
        break;
    default:
        break;
    }
}

void icd_logv(XGL_DBG_MSG_TYPE msg_type,
              XGL_VALIDATION_LEVEL validation_level,
              XGL_BASE_OBJECT src_object,
              size_t location, int32_t msg_code,
              const char *format, va_list ap)
{
    char msg[256];
    int ret;

    ret = vsnprintf(msg, sizeof(msg), format, ap);
    if (ret >= sizeof(msg) || ret < 0)
        msg[sizeof(msg) - 1] = '\0';

    icd_log_str(msg_type, validation_level, src_object,
            location, msg_code, msg);
}

void icd_log(XGL_DBG_MSG_TYPE msg_type,
             XGL_VALIDATION_LEVEL validation_level,
             XGL_BASE_OBJECT src_object,
             size_t location, int32_t msg_code,
             const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    icd_logv(msg_type, validation_level, src_object,
            location, msg_code, format, ap);
    va_end(ap);
}
