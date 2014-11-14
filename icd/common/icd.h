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
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ICD_H
#define ICD_H

#include <stdarg.h>

#include <xgl.h>
#include <xglDbg.h>
#include <xglLayer.h>

#include "icd-dispatch-table.h"

#if defined(__GNUC__) && __GNUC__ >= 4
#  define ICD_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#  define ICD_EXPORT __attribute__((visibility("default")))
#else
#  define ICD_EXPORT
#endif

XGL_RESULT XGLAPI icdDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData);
XGL_RESULT XGLAPI icdDbgUnregisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);
XGL_RESULT XGLAPI icdDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData);

void icd_msg(XGL_DBG_MSG_TYPE msg_type,
             XGL_VALIDATION_LEVEL validation_level,
             XGL_BASE_OBJECT src_object,
             XGL_SIZE location,
             XGL_INT msg_code,
             const char *msg);

void icd_vlog(XGL_DBG_MSG_TYPE msg_type,
              XGL_VALIDATION_LEVEL validation_level,
              XGL_BASE_OBJECT src_object,
              XGL_SIZE location,
              XGL_INT msg_code,
              const char *format, va_list ap);

void icd_log(XGL_DBG_MSG_TYPE msg_type,
             XGL_VALIDATION_LEVEL validation_level,
             XGL_BASE_OBJECT src_object,
             XGL_SIZE location,
             XGL_INT msg_code,
             const char *format, ...);

void icd_clear_msg_callbacks(void);

XGL_RESULT icd_set_allocator(const XGL_ALLOC_CALLBACKS *alloc_cb);
int icd_get_allocator_id(void);

#ifdef __cplusplus
extern "C" {
#endif

void *icd_alloc(XGL_SIZE size, XGL_SIZE alignment,
                XGL_SYSTEM_ALLOC_TYPE type);

void icd_free(void *mem);

#ifdef __cplusplus
}
#endif

#endif /* ICD_H */
