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
 */

#ifndef MEM_H
#define MEM_H

#include "intel.h"

struct intel_bo;

struct intel_mem {
    struct intel_base base;

    struct intel_bo *bo;
};

XGL_RESULT intel_mem_alloc(struct intel_dev *dev,
                           const XGL_MEMORY_ALLOC_INFO *info,
                           struct intel_mem **mem_ret);
void intel_mem_free(struct intel_mem *mem);

XGL_RESULT intel_mem_set_priority(struct intel_mem *mem,
                                  XGL_MEMORY_PRIORITY priority);

void *intel_mem_map(struct intel_mem *mem, XGL_FLAGS flags);
void intel_mem_unmap(struct intel_mem *mem);

static inline struct intel_mem *intel_mem(XGL_GPU_MEMORY mem)
{
    return (struct intel_mem *) mem;
}

XGL_RESULT XGLAPI intelAllocMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    XGL_GPU_MEMORY*                             pMem);

XGL_RESULT XGLAPI intelFreeMemory(
    XGL_GPU_MEMORY                              mem);

XGL_RESULT XGLAPI intelSetMemoryPriority(
    XGL_GPU_MEMORY                              mem,
    XGL_MEMORY_PRIORITY                         priority);

XGL_RESULT XGLAPI intelMapMemory(
    XGL_GPU_MEMORY                              mem,
    XGL_FLAGS                                   flags,
    XGL_VOID**                                  ppData);

XGL_RESULT XGLAPI intelUnmapMemory(
    XGL_GPU_MEMORY                              mem);

#endif /* MEM_H */
