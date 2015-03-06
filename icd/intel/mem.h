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

#ifndef MEM_H
#define MEM_H

#include "kmd/winsys.h"
#include "intel.h"
#include "obj.h"

struct intel_mem {
    struct intel_base base;

    struct intel_bo *bo;
    XGL_GPU_SIZE size;
};

XGL_RESULT intel_mem_alloc(struct intel_dev *dev,
                           const XGL_MEMORY_ALLOC_INFO *info,
                           struct intel_mem **mem_ret);
void intel_mem_free(struct intel_mem *mem);

XGL_RESULT intel_mem_import_userptr(struct intel_dev *dev,
                                    const void *userptr,
                                    size_t size,
                                    struct intel_mem **mem_ret);

XGL_RESULT intel_mem_set_priority(struct intel_mem *mem,
                                  XGL_MEMORY_PRIORITY priority);

static inline void *intel_mem_map(struct intel_mem *mem, XGL_FLAGS flags)
{
    return intel_bo_map_async(mem->bo);
}

static inline void *intel_mem_map_sync(struct intel_mem *mem, bool rw)
{
    return intel_bo_map(mem->bo, rw);
}

static inline void intel_mem_unmap(struct intel_mem *mem)
{
    intel_bo_unmap(mem->bo);
}

static inline bool intel_mem_is_busy(struct intel_mem *mem)
{
    return intel_bo_is_busy(mem->bo);
}

static inline struct intel_mem *intel_mem(XGL_GPU_MEMORY mem)
{
    return (struct intel_mem *) mem;
}

#endif /* MEM_H */
