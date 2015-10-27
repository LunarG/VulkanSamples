/*
 * Copyright © 2008, 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

/** @file standalone_utils.cpp
 *
 * This file contains the allocation and logging utils needed
 * by the standalone compiler.
 */

#include "instance.h"


void *intel_alloc(const void *handle,
                                size_t size, size_t alignment,
                                VkSystemAllocationScope scope)
{
    assert(intel_handle_validate(handle));
    return icd_instance_alloc(((const struct intel_handle *) handle)->instance->icd,
            size, alignment, scope);
}

void intel_free(const void *handle, void *ptr)
{
    assert(intel_handle_validate(handle));
    icd_instance_free(((const struct intel_handle *) handle)->instance->icd, ptr);
}

void intel_logv(const void *handle,
                VkFlags msg_flags,
                VkDbgObjectType obj_type, uint64_t src_object,
                size_t location, int32_t msg_code,
                const char *format, va_list ap)
{
    char msg[256];
    int ret;

    ret = vsnprintf(msg, sizeof(msg), format, ap);
    if (ret >= sizeof(msg) || ret < 0)
        msg[sizeof(msg) - 1] = '\0';

    assert(intel_handle_validate(handle));
    icd_instance_log(((const struct intel_handle *) handle)->instance->icd,
                     msg_flags,
                     obj_type, src_object,              /* obj_type, object */
                     location, msg_code,                /* location, msg_code */
                     msg);
}
