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

#ifndef ICD_LOG_H
#define ICD_LOG_H

#include <stdarg.h>
#include "icd-utils.h"
#include "icd.h"

XGL_RESULT icd_logger_set_bool(XGL_DBG_GLOBAL_OPTION option, bool enable);

XGL_RESULT icd_logger_add_callback(XGL_DBG_MSG_CALLBACK_FUNCTION func,
                                   void *user_data);
XGL_RESULT icd_logger_remove_callback(XGL_DBG_MSG_CALLBACK_FUNCTION func);
void icd_logger_clear_callbacks(void);

void icd_logv(XGL_DBG_MSG_TYPE msg_type,
              XGL_VALIDATION_LEVEL validation_level,
              XGL_BASE_OBJECT src_object,
              size_t location, int32_t msg_code,
              const char *format, va_list ap);

void icd_log(XGL_DBG_MSG_TYPE msg_type,
             XGL_VALIDATION_LEVEL validation_level,
             XGL_BASE_OBJECT src_object,
             size_t location, int32_t msg_code,
             const char *format, ...);

#endif /* ICD_LOG_H */
