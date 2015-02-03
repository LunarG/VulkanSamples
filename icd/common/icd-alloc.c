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

#define _ISOC11_SOURCE /* for aligned_alloc() */
#include <stdlib.h>
#include <string.h>

#include "icd-utils.h"
#include "icd-alloc.h"

struct icd_allocator {
    bool initialized;
    XGL_ALLOC_CALLBACKS callbacks;
};

static void *default_alloc(void *user_data, size_t size, size_t alignment,
                           XGL_SYSTEM_ALLOC_TYPE allocType)
{
    if (alignment <= 1) {
        return malloc(size);
    } else if (u_is_pow2((uint32_t) alignment)) {
        if (alignment < sizeof(void *)) {
            assert(u_is_pow2(sizeof(void*)));
            alignment = sizeof(void *);
        }

        size = (size + alignment - 1) & ~(alignment - 1);

#if defined(PLATFORM_LINUX)
        return aligned_alloc(alignment, size);
#else
		return _aligned_malloc(size, alignment);
#endif
    }
    else {
        return NULL;
    }
}

static void default_free(void *user_data, void *ptr)
{
    free(ptr);
}

static struct icd_allocator icd_allocator = {
    .callbacks = {
        .pfnAlloc = default_alloc,
        .pfnFree = default_free,
    }
};

XGL_RESULT icd_allocator_init(const XGL_ALLOC_CALLBACKS *alloc_cb)
{
    if (icd_allocator.initialized) {
        const XGL_ALLOC_CALLBACKS default_cb = {
            NULL, default_alloc, default_free
        };

        if (!alloc_cb)
            alloc_cb = &default_cb;

        return (memcmp(&icd_allocator.callbacks, alloc_cb, sizeof(*alloc_cb)))
            ? XGL_ERROR_INVALID_POINTER : XGL_SUCCESS;
    }

    if (alloc_cb)
        icd_allocator.callbacks = *alloc_cb;
    icd_allocator.initialized = true;

    return XGL_SUCCESS;
}

uint32_t icd_allocator_get_id(void)
{
    return icd_allocator.initialized;
}

void *icd_alloc(size_t size, size_t alignment, XGL_SYSTEM_ALLOC_TYPE type)
{
    return icd_allocator.callbacks.pfnAlloc(icd_allocator.callbacks.pUserData,
            size, alignment, type);
}

void icd_free(void *ptr)
{
    icd_allocator.callbacks.pfnFree(icd_allocator.callbacks.pUserData, ptr);
}
