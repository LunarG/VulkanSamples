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
#include "debug.h"

XGL_RESULT getQueue(struct app_gpu *gpu, XGL_QUEUE_TYPE qtype, const char *qname)
{
    int que_idx;
    XGL_RESULT err = XGL_SUCCESS;
    XGL_QUEUE queue;

    for (que_idx = 0; que_idx < gpu->queue_props->queueCount; que_idx++) {
        err = xglGetDeviceQueue(gpu->dev.obj, qtype, que_idx, &queue);
        if (err) {
            printf("xglGetDeviceQueue: %s queue #%d: Failed\n", qname, que_idx);
            ERR_EXIT(err);
        }

        printf("xglGetDeviceQueue: %s queue #%d: Passed\n", qname, que_idx);
    }
    return err;
}

int main(int argc, char **argv)
{
    static const XGL_APPLICATION_INFO app_info = {
        .sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = (const XGL_CHAR *) "xglCreateDevice",
        .appVersion = 1,
        .pEngineName = (const XGL_CHAR *) "xglCreateDevice",
        .engineVersion = 1,
        .apiVersion = XGL_MAKE_VERSION(0, 22, 0),
    };
    struct app_gpu gpus[MAX_GPUS];
    struct app_gpu *gpu;
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count, i, gpu_idx, que_idx;
    XGL_RESULT err;

    err = xglInitAndEnumerateGpus(&app_info, NULL,
                                  MAX_GPUS, &gpu_count, objs);
    if (err)
        ERR_EXIT(err);

    if (gpu_count <= 0) {
        ERR_MSG_EXIT("No GPU avialable");
    }

    for (i = 0; i < gpu_count; i++) {
        app_gpu_init(&gpus[i], i, objs[i]);
    }

    for (gpu_idx = 0; gpu_idx < gpu_count; gpu_idx++) {
        gpu = &gpus[gpu_idx];
        if (gpu->queue_props->queueCount == 0) {
            debug_printf("No heaps available for GPU #%d: %s", gpu_idx, gpu->props.gpuName);
            continue;
        } else {

//            XGL_RESULT XGLAPI xglGetDeviceQueue(
//                XGL_DEVICE                                  device,
//                XGL_QUEUE_TYPE                              queueType,
//                XGL_UINT                                    queueIndex,
//                XGL_QUEUE*                                  pQueue);
            /*
             * queue handles are retrieved from the device by calling
             * xglGetDeviceQueue() with a queue type and a requested logical
             * queue ID. The logical queue ID is a sequential number starting
             * from zero and referencing up to the number of queues requested
             * at device creation. Each queue type has its own sequence of IDs
             * starting at zero.
             */

            for (que_idx = 0; que_idx < gpu->queue_props->queueCount; que_idx++) {

//                typedef enum _XGL_QUEUE_FLAGS
//                {
//                    XGL_QUEUE_GRAPHICS_BIT                                  = 0x00000001,   // Queue supports graphics operations
//                    XGL_QUEUE_COMPUTE_BIT                                   = 0x00000002,   // Queue supports compute operations
//                    XGL_QUEUE_DMA_BIT                                       = 0x00000004,   // Queue supports DMA operations
//                    XGL_QUEUE_EXTENDED_BIT                                  = 0x80000000    // Extended queue
//                } XGL_QUEUE_FLAGS;

//                typedef enum _XGL_QUEUE_TYPE
//                {
//                    XGL_QUEUE_TYPE_GRAPHICS                                 = 0x1,
//                    XGL_QUEUE_TYPE_COMPUTE                                  = 0x2,
//                    XGL_QUEUE_TYPE_DMA                                      = 0x3,
//                    XGL_MAX_ENUM(_XGL_QUEUE_TYPE)
//                } XGL_QUEUE_TYPE;

                if (gpu->queue_props->queueFlags & XGL_QUEUE_GRAPHICS_BIT) {
                    getQueue(gpu, XGL_QUEUE_TYPE_GRAPHICS, "Graphics");
                }

                if (gpu->queue_props->queueFlags & XGL_QUEUE_COMPUTE_BIT) {
                    getQueue(gpu, XGL_QUEUE_TYPE_GRAPHICS, "Compute");
                }

                if (gpu->queue_props->queueFlags & XGL_QUEUE_DMA_BIT) {
                    getQueue(gpu, XGL_QUEUE_TYPE_GRAPHICS, "DMA");
                }

                // TODO: What do we do about EXTENDED_BIT?

                /* Guide: pg 34:
                 * The queue objects cannot be destroyed explicitly by an application
                 * and are automatically destroyed when the associated device is destroyed.
                 * Once the device is destroyed, attempting to use a queue results in
                 * undefined behavior.
                 */
            }
        }
    }

    for (i = 0; i < gpu_count; i++)
        app_gpu_destroy(&gpus[i]);

    return 0;
}
