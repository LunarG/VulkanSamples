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
 *   Chia-I Wu <olv@lunarg.com>
 */

#ifndef INTEL_H
#define INTEL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <vulkan.h>
#include <vkDbg.h>
#include <vkWsiX11Ext.h>
#include <vkIcd.h>

#include "icd.h"
#include "icd-spv.h"
#include "icd-format.h"
#include "icd-instance.h"
#include "icd-utils.h"

#define INTEL_API_VERSION VK_API_VERSION
#define INTEL_DRIVER_VERSION 0

#define INTEL_GEN(gen) ((int) ((gen) * 100))

#define INTEL_MAX_VERTEX_BINDING_COUNT 33
#define INTEL_MAX_VERTEX_ELEMENT_COUNT (INTEL_MAX_VERTEX_BINDING_COUNT + 1)
#define INTEL_MAX_RENDER_TARGETS 8

enum intel_debug_flags {
    INTEL_DEBUG_BATCH       = 1 << 0,

    INTEL_DEBUG_NOHW        = 1 << 20,
    INTEL_DEBUG_NOCACHE     = 1 << 21,
    INTEL_DEBUG_NOHIZ       = 1 << 22,
    INTEL_DEBUG_HANG        = 1 << 23,
};

struct intel_handle {
    /* the loader expects a "void *" at the beginning */
    void *loader_data;

    uint32_t magic;
    const struct icd_instance *icd;
};

extern int intel_debug;

static const uint32_t intel_handle_magic = 0x494e544c;

static inline void intel_handle_init(struct intel_handle *handle,
                                     VK_DBG_OBJECT_TYPE type,
                                     const struct icd_instance *icd)
{
    set_loader_magic_value(handle);

    handle->magic = intel_handle_magic + type;
    handle->icd = icd;
}

/**
 * Return true if \p handle is a valid intel_handle.  This assumes the first
 * sizeof(intel_handle) bytes are readable, and they does not happen to have
 * our magic values.
 */
static inline bool intel_handle_validate(const void *handle)
{
    const uint32_t handle_type =
        ((const struct intel_handle *) handle)->magic - intel_handle_magic;

    return (handle_type <= VK_DBG_OBJECT_TYPE_END_RANGE);
}

/**
 * Return true if \p handle is a valid intel_handle of \p type.
 *
 * \see intel_handle_validate().
 */
static inline bool intel_handle_validate_type(const void *handle,
                                              VK_DBG_OBJECT_TYPE type)
{
    const uint32_t handle_type =
        ((const struct intel_handle *) handle)->magic - intel_handle_magic;

    return (handle_type == (uint32_t) type);
}

static inline void *intel_alloc(const void *handle,
                                size_t size, size_t alignment,
                                VK_SYSTEM_ALLOC_TYPE type)
{
    assert(intel_handle_validate(handle));
    return icd_instance_alloc(((const struct intel_handle *) handle)->icd,
            size, alignment, type);
}

static inline void intel_free(const void *handle, void *ptr)
{
    assert(intel_handle_validate(handle));
    icd_instance_free(((const struct intel_handle *) handle)->icd, ptr);
}

static inline void intel_logv(const void *handle,
                              VK_DBG_MSG_TYPE msg_type,
                              VK_VALIDATION_LEVEL validation_level,
                              VK_BASE_OBJECT src_object,
                              size_t location, int32_t msg_code,
                              const char *format, va_list ap)
{
    char msg[256];
    int ret;

    ret = vsnprintf(msg, sizeof(msg), format, ap);
    if (ret >= sizeof(msg) || ret < 0)
        msg[sizeof(msg) - 1] = '\0';

    assert(intel_handle_validate(handle));
    icd_instance_log(((const struct intel_handle *) handle)->icd,
            msg_type, validation_level, src_object, location, msg_code, msg);
}

static inline void intel_log(const void *handle,
                             VK_DBG_MSG_TYPE msg_type,
                             VK_VALIDATION_LEVEL validation_level,
                             VK_BASE_OBJECT src_object,
                             size_t location, int32_t msg_code,
                             const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    intel_logv(handle, msg_type, validation_level, src_object,
            location, msg_code, format, ap);
    va_end(ap);
}

#endif /* INTEL_H */
