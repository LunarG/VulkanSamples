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

#include "common.h"

void app_dev_init_formats(struct app_dev *dev)
{
    XGL_CHANNEL_FORMAT ch;
    XGL_NUM_FORMAT num;

    for (ch = 0; ch < XGL_MAX_CH_FMT; ch++) {
        for (num = 0; num < XGL_MAX_NUM_FMT; num++) {
            const XGL_FORMAT fmt = {
                .channelFormat = ch,
                .numericFormat = num,
            };
            XGL_RESULT err;
            XGL_SIZE size;

            err = xglGetFormatInfo(dev->obj, fmt,
                                   XGL_INFO_TYPE_FORMAT_PROPERTIES,
                                   &size, &dev->format_props[ch][num]);
            if (err) {
                memset(&dev->format_props[ch][num], 0,
                       sizeof(dev->format_props[ch][num]));
            }
            else if (size != sizeof(dev->format_props[ch][num])) {
                ERR_EXIT(XGL_ERROR_UNKNOWN);
            }
        }
    }
}

void app_dev_init(struct app_dev *dev, struct app_gpu *gpu)
{
    XGL_DEVICE_CREATE_INFO info = {
        .sType = XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 0,
        .pRequestedQueues = NULL,
        .extensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .maxValidationLevel = XGL_VALIDATION_LEVEL_END_RANGE,
        .flags = XGL_DEVICE_CREATE_VALIDATION_BIT,
    };
    XGL_RESULT err;
    XGL_SIZE size;
    XGL_UINT i;

    /* XXX how to request queues? */

    /* enable all extensions */
    info.extensionCount = gpu->extension_count;
    info.ppEnabledExtensionNames = gpu->extensions;

    dev->gpu = gpu;
    err = xglCreateDevice(gpu->obj, &info, &dev->obj);
    if (err)
        ERR_EXIT(err);

    err = xglGetMemoryHeapCount(dev->obj, &dev->heap_count);
    if (err)
        ERR_EXIT(err);

    dev->heap_props =
            malloc(sizeof(dev->heap_props[0]) * dev->heap_count);
    if (!dev->heap_props)
        ERR_EXIT(XGL_ERROR_OUT_OF_MEMORY);

    for (i = 0; i < dev->heap_count; i++) {
        err = xglGetMemoryHeapInfo(dev->obj, i,
                                   XGL_INFO_TYPE_MEMORY_HEAP_PROPERTIES,
                                   &size, &dev->heap_props[i]);
        if (err || size != sizeof(dev->heap_props[0]))
            ERR_EXIT(err);
    }
}

void app_dev_destroy(struct app_dev *dev)
{
    free(dev->heap_props);
    xglDestroyDevice(dev->obj);
}

void app_gpu_init_extensions(struct app_gpu *gpu)
{
    XGL_RESULT err;
    XGL_UINT i;

    static const XGL_CHAR *known_extensions[] = {
        (const XGL_CHAR *) "some_extension",
    };

    for (i = 0; i < ARRAY_SIZE(known_extensions); i++) {
        err = xglGetExtensionSupport(gpu->obj, known_extensions[i]);
        if (!err)
            gpu->extension_count++;
    }

    gpu->extensions =
            malloc(sizeof(gpu->extensions[0]) * gpu->extension_count);
    if (!gpu->extensions)
        ERR_EXIT(XGL_ERROR_OUT_OF_MEMORY);

    gpu->extension_count = 0;
    for (i = 0; i < ARRAY_SIZE(known_extensions); i++) {
        err = xglGetExtensionSupport(gpu->obj, known_extensions[i]);
        if (!err)
            gpu->extensions[gpu->extension_count++] = known_extensions[i];
    }
}

void app_gpu_init(struct app_gpu *gpu, XGL_UINT id, XGL_PHYSICAL_GPU obj)
{
    XGL_SIZE size;
    XGL_RESULT err;
    int i;

    memset(gpu, 0, sizeof(*gpu));

    gpu->id = id;
    gpu->obj = obj;

    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
                        &size, &gpu->props);
    if (err || size != sizeof(gpu->props))
        ERR_EXIT(err);

    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE,
                        &size, &gpu->perf);
    if (err || size != sizeof(gpu->perf))
        ERR_EXIT(err);

    /* get queue count */
    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &size, NULL);
    if (err || size % sizeof(gpu->queue_props[0]))
        ERR_EXIT(err);
    gpu->queue_count = size / sizeof(gpu->queue_props[0]);

    gpu->queue_props =
            malloc(sizeof(gpu->queue_props[0]) * gpu->queue_count);
    if (!gpu->queue_props)
        ERR_EXIT(XGL_ERROR_OUT_OF_MEMORY);
    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES,
                        &size, gpu->queue_props);
    if (err || size != sizeof(gpu->queue_props[0]) * gpu->queue_count)
        ERR_EXIT(err);

    err = xglGetGpuInfo(gpu->obj,
                        XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES,
                        &size, &gpu->memory_props);
    if (err || size != sizeof(gpu->memory_props))
        ERR_EXIT(err);

    app_gpu_init_extensions(gpu);
    app_dev_init(&gpu->dev, gpu);
    app_dev_init_formats(&gpu->dev);
}

void app_gpu_destroy(struct app_gpu *gpu)
{
    app_dev_destroy(&gpu->dev);
    free(gpu->extensions);
}

