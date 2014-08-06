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

#ifndef DEV_H
#define DEV_H

#include "obj.h"
#include "gpu.h"
#include "intel.h"

struct intel_bo;
struct intel_gpu;
struct intel_queue;
struct intel_winsys;

struct intel_dev_dbg_msg_filter {
    XGL_INT msg_code;
    XGL_DBG_MSG_FILTER filter;
    bool triggered;

    struct intel_dev_dbg_msg_filter *next;
};

struct intel_dev_dbg {
    struct intel_base_dbg base;

    struct intel_dev_dbg_msg_filter *filters;
};

struct intel_dev {
    struct intel_base base;

    struct intel_gpu *gpu;
    struct intel_winsys *winsys;
    struct intel_queue *queues[INTEL_GPU_ENGINE_COUNT];
};

struct intel_queue {
    struct intel_base base;

    struct intel_dev *dev;
    struct intel_bo *last_submitted_bo;
};

static inline struct intel_dev *intel_dev(XGL_DEVICE dev)
{
    return (struct intel_dev *) dev;
}

static inline struct intel_dev_dbg *intel_dev_dbg(struct intel_dev *dev)
{
    return (struct intel_dev_dbg *) dev->base.dbg;
}

static inline struct intel_queue *intel_queue(XGL_QUEUE queue)
{
    return (struct intel_queue *) queue;
}

XGL_RESULT intel_dev_create(struct intel_gpu *gpu,
                            const XGL_DEVICE_CREATE_INFO *info,
                            struct intel_dev **dev_ret);
void intel_dev_destroy(struct intel_dev *dev);

void intel_dev_get_heap_props(const struct intel_dev *dev,
                              XGL_MEMORY_HEAP_PROPERTIES *props);

XGL_RESULT intel_dev_add_msg_filter(struct intel_dev *dev,
                                    XGL_INT msg_code,
                                    XGL_DBG_MSG_FILTER filter);

void intel_dev_remove_msg_filter(struct intel_dev *dev,
                                 XGL_INT msg_code);

XGL_RESULT XGLAPI intelCreateDevice(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_DEVICE_CREATE_INFO*               pCreateInfo,
    XGL_DEVICE*                                 pDevice);

XGL_RESULT XGLAPI intelDestroyDevice(
    XGL_DEVICE                                  device);

XGL_RESULT XGLAPI intelGetMemoryHeapCount(
    XGL_DEVICE                                  device,
    XGL_UINT*                                   pCount);

XGL_RESULT XGLAPI intelGetMemoryHeapInfo(
    XGL_DEVICE                                  device,
    XGL_UINT                                    heapId,
    XGL_MEMORY_HEAP_INFO_TYPE                   infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

XGL_RESULT XGLAPI intelGetDeviceQueue(
    XGL_DEVICE                                  device,
    XGL_QUEUE_TYPE                              queueType,
    XGL_UINT                                    queueIndex,
    XGL_QUEUE*                                  pQueue);

XGL_RESULT XGLAPI intelQueueSetGlobalMemReferences(
    XGL_QUEUE                                   queue,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs);

XGL_RESULT XGLAPI intelQueueWaitIdle(
    XGL_QUEUE                                   queue);

XGL_RESULT XGLAPI intelDeviceWaitIdle(
    XGL_DEVICE                                  device);

#endif /* DEV_H */
