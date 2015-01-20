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

#include "dev.h"
#include "mem.h"

XGL_RESULT intel_mem_alloc(struct intel_dev *dev,
                           const XGL_MEMORY_ALLOC_INFO *info,
                           struct intel_mem **mem_ret)
{
    struct intel_mem *mem;

    /* ignore any IMAGE_INFO and BUFFER_INFO usage: they don't alter allocations */

    mem = (struct intel_mem *) intel_base_create(dev, sizeof(*mem),
            dev->base.dbg, XGL_DBG_OBJECT_GPU_MEMORY, info, 0);
    if (!mem)
        return XGL_ERROR_OUT_OF_MEMORY;

    mem->bo = intel_winsys_alloc_buffer(dev->winsys,
            "xgl-gpu-memory", info->allocationSize, 0);
    if (!mem->bo) {
        intel_mem_free(mem);
        return XGL_ERROR_UNKNOWN;
    }

    mem->size = info->allocationSize;

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

ICD_EXPORT XGL_RESULT XGLAPI xglAllocMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_mem_alloc(dev, pAllocInfo, (struct intel_mem **) pMem);
}

ICD_EXPORT XGL_RESULT XGLAPI xglFreeMemory(
    XGL_GPU_MEMORY                              mem_)
{
    struct intel_mem *mem = intel_mem(mem_);

    intel_mem_free(mem);

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglSetMemoryPriority(
    XGL_GPU_MEMORY                              mem_,
    XGL_MEMORY_PRIORITY                         priority)
{
    struct intel_mem *mem = intel_mem(mem_);

    return intel_mem_set_priority(mem, priority);
}

ICD_EXPORT XGL_RESULT XGLAPI xglMapMemory(
    XGL_GPU_MEMORY                              mem_,
    XGL_FLAGS                                   flags,
    XGL_VOID**                                  ppData)
{
    struct intel_mem *mem = intel_mem(mem_);
    void *ptr = intel_mem_map(mem, flags);

    *ppData = ptr;

    return (ptr) ? XGL_SUCCESS : XGL_ERROR_UNKNOWN;
}

ICD_EXPORT XGL_RESULT XGLAPI xglUnmapMemory(
    XGL_GPU_MEMORY                              mem_)
{
    struct intel_mem *mem = intel_mem(mem_);

    intel_mem_unmap(mem);

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglPinSystemMemory(
    XGL_DEVICE                                  device,
    const XGL_VOID*                             pSysMem,
    XGL_SIZE                                    memSize,
    XGL_GPU_MEMORY*                             pMem)
{
    /* add DRM_I915_GEM_USERPTR to wisys first */
    return XGL_ERROR_UNAVAILABLE;
}

ICD_EXPORT XGL_RESULT XGLAPI xglOpenSharedMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_OPEN_INFO*                 pOpenInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}

ICD_EXPORT XGL_RESULT XGLAPI xglOpenPeerMemory(
    XGL_DEVICE                                  device,
    const XGL_PEER_MEMORY_OPEN_INFO*            pOpenInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNAVAILABLE;
}
