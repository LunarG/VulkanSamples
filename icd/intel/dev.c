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

#include <stdarg.h>
#include "kmd/winsys.h"
#include "dispatch.h"
#include "gpu.h"
#include "queue.h"
#include "dev.h"

static XGL_RESULT dev_create_queues(struct intel_dev *dev,
                                    const XGL_DEVICE_QUEUE_CREATE_INFO *queues,
                                    XGL_UINT count)
{
    XGL_UINT i;

    if (!count)
        return XGL_ERROR_INVALID_POINTER;

    for (i = 0; i < count; i++) {
        const XGL_DEVICE_QUEUE_CREATE_INFO *q = &queues[i];
        XGL_RESULT ret = XGL_SUCCESS;

        if (q->queueNodeIndex < INTEL_GPU_ENGINE_COUNT &&
            q->queueCount == 1 && !dev->queues[q->queueNodeIndex]) {
            ret = intel_queue_create(dev, q->queueNodeIndex,
                    &dev->queues[q->queueNodeIndex]);
        }
        else {
            ret = XGL_ERROR_INVALID_POINTER;
        }

        if (ret != XGL_SUCCESS) {
            XGL_UINT j;
            for (j = 0; j < i; j++)
                intel_queue_destroy(dev->queues[j]);

            return ret;
        }
    }

    return XGL_SUCCESS;
}

XGL_RESULT intel_dev_create(struct intel_gpu *gpu,
                            const XGL_DEVICE_CREATE_INFO *info,
                            struct intel_dev **dev_ret)
{
    struct intel_dev *dev;
    XGL_RESULT ret;

    if (info->extensionCount)
        return XGL_ERROR_INVALID_EXTENSION;

    if (gpu->fd >= 0)
        return XGL_ERROR_DEVICE_ALREADY_CREATED;

    dev = (struct intel_dev *) intel_base_create(NULL, sizeof(*dev),
            info->flags & XGL_DEVICE_CREATE_VALIDATION_BIT,
            XGL_DBG_OBJECT_DEVICE, info, sizeof(struct intel_dev_dbg));
    if (!dev)
        return XGL_ERROR_OUT_OF_MEMORY;

    dev->gpu = gpu;

    ret = intel_gpu_open(gpu);
    if (ret != XGL_SUCCESS) {
        intel_dev_destroy(dev);
        return ret;
    }

    dev->winsys = intel_winsys_create_for_fd(gpu->fd);
    if (!dev->winsys) {
        icd_log(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE,
                0, 0, "failed to create device winsys for %s", gpu->path);
        intel_dev_destroy(dev);
        return XGL_ERROR_UNKNOWN;
    }

    dev->cmd_scratch_bo = intel_winsys_alloc_buffer(dev->winsys,
            "command buffer scratch", 4096, false);
    if (!dev->cmd_scratch_bo) {
        intel_dev_destroy(dev);
        return XGL_ERROR_OUT_OF_GPU_MEMORY;
    }

    ret = dev_create_queues(dev, info->pRequestedQueues,
            info->queueRecordCount);
    if (ret != XGL_SUCCESS) {
        intel_dev_destroy(dev);
        return ret;
    }

    *dev_ret = dev;

    return XGL_SUCCESS;
}

static void dev_clear_msg_filters(struct intel_dev *dev)
{
    struct intel_dev_dbg *dbg = intel_dev_dbg(dev);
    struct intel_dev_dbg_msg_filter *filter;

    filter = dbg->filters;
    while (filter) {
        struct intel_dev_dbg_msg_filter *next = filter->next;
        icd_free(filter);
        filter = next;
    }

    dbg->filters = NULL;
}

void intel_dev_destroy(struct intel_dev *dev)
{
    XGL_UINT i;

    if (dev->base.dbg)
        dev_clear_msg_filters(dev);

    for (i = 0; i < ARRAY_SIZE(dev->queues); i++) {
        if (dev->queues[i])
            intel_queue_destroy(dev->queues[i]);
    }

    if (dev->cmd_scratch_bo)
        intel_bo_unreference(dev->cmd_scratch_bo);

    if (dev->winsys)
        intel_winsys_destroy(dev->winsys);

    if (dev->gpu->fd >= 0)
        intel_gpu_close(dev->gpu);

    intel_base_destroy(&dev->base);
}

void intel_dev_get_heap_props(const struct intel_dev *dev,
                              XGL_MEMORY_HEAP_PROPERTIES *props)
{
    props->structSize = sizeof(XGL_MEMORY_HEAP_PROPERTIES);

    props->heapMemoryType = XGL_HEAP_MEMORY_LOCAL;

    props->heapSize = 0xffffffff; /* TODO system memory size */

    props->pageSize = 4096;
    props->flags = XGL_MEMORY_HEAP_CPU_VISIBLE_BIT |
                   XGL_MEMORY_HEAP_CPU_GPU_COHERENT_BIT |
                   XGL_MEMORY_HEAP_CPU_WRITE_COMBINED_BIT |
                   XGL_MEMORY_HEAP_HOLDS_PINNED_BIT |
                   XGL_MEMORY_HEAP_SHAREABLE_BIT;

    props->gpuReadPerfRating = 100.0f;
    props->gpuWritePerfRating = 100.0f;
    props->cpuReadPerfRating = 10.0f;
    props->cpuWritePerfRating = 80.0f;
}

XGL_RESULT intel_dev_add_msg_filter(struct intel_dev *dev,
                                    XGL_INT msg_code,
                                    XGL_DBG_MSG_FILTER filter)
{
    struct intel_dev_dbg *dbg = intel_dev_dbg(dev);
    struct intel_dev_dbg_msg_filter *f = dbg->filters;

    assert(filter != XGL_DBG_MSG_FILTER_NONE);

    while (f) {
        if (f->msg_code == msg_code)
            break;
        f = f->next;
    }

    if (f) {
        if (f->filter != filter) {
            f->filter = filter;
            f->triggered = false;
        }
    } else {
        f = icd_alloc(sizeof(*f), 0, XGL_SYSTEM_ALLOC_DEBUG);
        if (!f)
            return XGL_ERROR_OUT_OF_MEMORY;

        f->msg_code = msg_code;
        f->filter = filter;
        f->triggered = false;

        f->next = dbg->filters;
        dbg->filters = f;
    }

    return XGL_SUCCESS;
}

void intel_dev_remove_msg_filter(struct intel_dev *dev,
                                 XGL_INT msg_code)
{
    struct intel_dev_dbg *dbg = intel_dev_dbg(dev);
    struct intel_dev_dbg_msg_filter *f = dbg->filters, *prev = NULL;

    while (f) {
        if (f->msg_code == msg_code) {
            if (prev)
                prev->next = f->next;
            else
                dbg->filters = f->next;

            icd_free(f);
            break;
        }

        prev = f;
        f = f->next;
    }
}

static bool dev_filter_msg(struct intel_dev *dev,
                           XGL_INT msg_code)
{
    struct intel_dev_dbg *dbg = intel_dev_dbg(dev);
    struct intel_dev_dbg_msg_filter *filter;

    if (!dbg)
        return false;

    filter = dbg->filters;
    while (filter) {
        if (filter->msg_code != msg_code) {
            filter = filter->next;
            continue;
        }

        if (filter->filter == XGL_DBG_MSG_FILTER_ALL)
            return true;

        if (filter->filter == XGL_DBG_MSG_FILTER_REPEATED &&
            filter->triggered)
            return true;

        filter->triggered = true;
        break;
    }

    return false;
}

void intel_dev_log(struct intel_dev *dev,
                   XGL_DBG_MSG_TYPE msg_type,
                   XGL_VALIDATION_LEVEL validation_level,
                   struct intel_base *src_object,
                   XGL_SIZE location,
                   XGL_INT msg_code,
                   const char *format, ...)
{
    va_list ap;

    if (dev_filter_msg(dev, msg_code))
        return;

    va_start(ap, format);
    icd_vlog(msg_type, validation_level, (XGL_BASE_OBJECT) src_object,
            location, msg_code, format, ap);
    va_end(ap);
}

XGL_RESULT XGLAPI intelCreateDevice(
    XGL_PHYSICAL_GPU                            gpu_,
    const XGL_DEVICE_CREATE_INFO*               pCreateInfo,
    XGL_DEVICE*                                 pDevice)
{
    struct intel_gpu *gpu = intel_gpu(gpu_);

    return intel_dev_create(gpu, pCreateInfo, (struct intel_dev **) pDevice);
}

XGL_RESULT XGLAPI intelDestroyDevice(
    XGL_DEVICE                                  device)
{
    struct intel_dev *dev = intel_dev(device);

    intel_dev_destroy(dev);

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelGetMemoryHeapCount(
    XGL_DEVICE                                  device,
    XGL_UINT*                                   pCount)
{
    *pCount = 1;
    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelGetMemoryHeapInfo(
    XGL_DEVICE                                  device,
    XGL_UINT                                    heapId,
    XGL_MEMORY_HEAP_INFO_TYPE                   infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    struct intel_dev *dev = intel_dev(device);

    intel_dev_get_heap_props(dev, pData);
    *pDataSize = sizeof(XGL_MEMORY_HEAP_PROPERTIES);

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelGetDeviceQueue(
    XGL_DEVICE                                  device,
    XGL_QUEUE_TYPE                              queueType,
    XGL_UINT                                    queueIndex,
    XGL_QUEUE*                                  pQueue)
{
    struct intel_dev *dev = intel_dev(device);

    switch (queueType) {
    case XGL_QUEUE_TYPE_GRAPHICS:
    case XGL_QUEUE_TYPE_COMPUTE:
        if (queueIndex > 0)
            return XGL_ERROR_UNAVAILABLE;
        *pQueue = dev->queues[INTEL_GPU_ENGINE_3D];
        return XGL_SUCCESS;
    case XGL_QUEUE_TYPE_DMA:
    default:
        return XGL_ERROR_UNAVAILABLE;
    }
}

XGL_RESULT XGLAPI intelDeviceWaitIdle(
    XGL_DEVICE                                  device)
{
    struct intel_dev *dev = intel_dev(device);
    XGL_RESULT ret = XGL_SUCCESS;
    XGL_UINT i;

    for (i = 0; i < ARRAY_SIZE(dev->queues); i++) {
        if (dev->queues[i]) {
            const XGL_RESULT r = intel_queue_wait(dev->queues[i], -1);
            if (r != XGL_SUCCESS)
                ret = r;
        }
    }

    return ret;
}

XGL_RESULT XGLAPI intelDbgSetValidationLevel(
    XGL_DEVICE                                  device,
    XGL_VALIDATION_LEVEL                        validationLevel)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_dev_dbg *dbg = intel_dev_dbg(dev);

    if (dbg)
        dbg->validation_level = validationLevel;

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelDbgSetMessageFilter(
    XGL_DEVICE                                  device,
    XGL_INT                                     msgCode,
    XGL_DBG_MSG_FILTER                          filter)
{
    struct intel_dev *dev = intel_dev(device);

    if (!dev->base.dbg)
        return XGL_SUCCESS;

    if (filter == XGL_DBG_MSG_FILTER_NONE) {
        intel_dev_remove_msg_filter(dev, msgCode);
        return XGL_SUCCESS;
    }

    return intel_dev_add_msg_filter(dev, msgCode, filter);
}

XGL_RESULT XGLAPI intelDbgSetDeviceOption(
    XGL_DEVICE                                  device,
    XGL_DBG_DEVICE_OPTION                       dbgOption,
    XGL_SIZE                                    dataSize,
    const XGL_VOID*                             pData)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_dev_dbg *dbg = intel_dev_dbg(dev);
    XGL_RESULT ret = XGL_SUCCESS;

    if (dataSize == 0)
        return XGL_ERROR_INVALID_VALUE;

    switch (dbgOption) {
    case XGL_DBG_OPTION_DISABLE_PIPELINE_LOADS:
        if (dbg)
            dbg->disable_pipeline_loads = *((const bool *) pData);
        break;
    case XGL_DBG_OPTION_FORCE_OBJECT_MEMORY_REQS:
        if (dbg)
            dbg->force_object_memory_reqs = *((const bool *) pData);
        break;
    case XGL_DBG_OPTION_FORCE_LARGE_IMAGE_ALIGNMENT:
        if (dbg)
            dbg->force_large_image_alignment = *((const bool *) pData);
        break;
    default:
        ret = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}
