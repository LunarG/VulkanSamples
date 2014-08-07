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

#include "kmd/winsys.h"
#include "dev.h"
#include "mem.h"

XGL_RESULT intel_mem_alloc(struct intel_dev *dev,
                           const XGL_MEMORY_ALLOC_INFO *info,
                           struct intel_mem **mem_ret)
{
    struct intel_mem *mem;

    if ((info->alignment != 0) && (4096 % info->alignment))
        return XGL_ERROR_INVALID_ALIGNMENT;
    if (info->heapCount != 1 || info->heaps[0] != 0)
        return XGL_ERROR_INVALID_POINTER;

    mem = (struct intel_mem *) intel_base_create(sizeof(*mem),
            dev->base.dbg, XGL_DBG_OBJECT_GPU_MEMORY, info, 0);
    if (!mem)
        return XGL_ERROR_OUT_OF_MEMORY;

    mem->bo = intel_winsys_alloc_buffer(dev->winsys,
            "xgl-gpu-memory", info->allocationSize, 0);
    if (!mem->bo) {
        intel_mem_free(mem);
        return XGL_ERROR_UNKNOWN;
    }

    *mem_ret = mem;

    return XGL_SUCCESS;
}

void intel_mem_free(struct intel_mem *mem)
{
    if (mem->bo)
        intel_bo_unreference(mem->bo);

    intel_base_destroy(&mem->base);
}

XGL_RESULT intel_mem_set_priority(struct intel_mem *mem,
                                  XGL_MEMORY_PRIORITY priority)
{
    /* pin the bo when XGL_MEMORY_PRIORITY_VERY_HIGH? */
    return XGL_SUCCESS;
}

void *intel_mem_map(struct intel_mem *mem, XGL_FLAGS flags)
{
    return intel_bo_map_unsynchronized(mem->bo);
}

void intel_mem_unmap(struct intel_mem *mem)
{
    intel_bo_unmap(mem->bo);
}

XGL_RESULT XGLAPI intelAllocMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_mem_alloc(dev, pAllocInfo, (struct intel_mem **) pMem);
}

XGL_RESULT XGLAPI intelFreeMemory(
    XGL_GPU_MEMORY                              mem_)
{
    struct intel_mem *mem = intel_mem(mem_);

    intel_mem_free(mem);

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelSetMemoryPriority(
    XGL_GPU_MEMORY                              mem_,
    XGL_MEMORY_PRIORITY                         priority)
{
    struct intel_mem *mem = intel_mem(mem_);

    return intel_mem_set_priority(mem, priority);
}

XGL_RESULT XGLAPI intelMapMemory(
    XGL_GPU_MEMORY                              mem_,
    XGL_FLAGS                                   flags,
    XGL_VOID**                                  ppData)
{
    struct intel_mem *mem = intel_mem(mem_);
    void *ptr = intel_mem_map(mem, flags);

    *ppData = ptr;

    return (ptr) ? XGL_SUCCESS : XGL_ERROR_UNKNOWN;
}

XGL_RESULT XGLAPI intelUnmapMemory(
    XGL_GPU_MEMORY                              mem_)
{
    struct intel_mem *mem = intel_mem(mem_);

    intel_mem_unmap(mem);

    return XGL_SUCCESS;
}
