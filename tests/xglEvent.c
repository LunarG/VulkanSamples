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

int main(int argc, char **argv)
{
    static const XGL_APPLICATION_INFO app_info = {
        .sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = (const XGL_CHAR *) "xglEvent",
        .appVersion = 1,
        .pEngineName = (const XGL_CHAR *) "xglEvent",
        .engineVersion = 1,
        .apiVersion = XGL_MAKE_VERSION(0, 22, 0),
    };
    struct app_gpu gpus[MAX_GPUS];
    struct app_gpu *gpu;
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count, i, gpu_idx;
    XGL_EVENT_CREATE_INFO event_info;
    XGL_EVENT event;
    XGL_UINT data_size;
    XGL_MEMORY_REQUIREMENTS mem_req;
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
        app_dev_init_queue(&gpus[i].dev, XGL_QUEUE_TYPE_GRAPHICS);
    }

    for (gpu_idx = 0; gpu_idx < gpu_count; gpu_idx++) {

        gpu = &gpus[gpu_idx];

//        typedef struct _XGL_EVENT_CREATE_INFO
//        {
//            XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO
//            const XGL_VOID*                         pNext;      // Pointer to next structure
//            XGL_FLAGS                               flags;      // Reserved
//        } XGL_EVENT_CREATE_INFO;
        memset(&event_info, 0, sizeof(event_info));
        event_info.sType = XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO;

        err = xglCreateEvent(gpu->dev.obj, &event_info, &event);
        if (err)
            ERR_EXIT(err);

        printf("xglCreateEvent: Passed");

        err = xglGetObjectInfo(event, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                               &data_size, &mem_req);
//        XGL_RESULT XGLAPI xglAllocMemory(
//            XGL_DEVICE                                  device,
//            const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
//            XGL_GPU_MEMORY*                             pMem);
        XGL_MEMORY_ALLOC_INFO mem_info;
        XGL_GPU_MEMORY event_mem;

        if (mem_req.size == 0) {
            ERR_MSG_EXIT("xglGetObjectInfo (Event): Failed - expect events to require memory");
        }
        memset(&mem_info, 0, sizeof(mem_info));
        mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
        mem_info.allocationSize = mem_req.size;
        mem_info.alignment = mem_req.alignment;
        mem_info.heapCount = mem_req.heapCount;
        memcpy(mem_info.heaps, mem_req.heaps, sizeof(XGL_UINT)*XGL_MAX_MEMORY_HEAPS);
        mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;
        mem_info.flags = XGL_MEMORY_ALLOC_SHAREABLE_BIT;
        err = xglAllocMemory(gpu->dev.obj, &mem_info, &event_mem);
        if (err)
            ERR_EXIT(err);

        printf("xglAllocMemory (Event): Passed\n");

        err = xglBindObjectMemory(event, event_mem, 0);
        if (err)
            ERR_EXIT(err);

        printf("xglBindObjectMemory (Event): Passed\n");

        err = xglResetEvent(event);
        if (err)
            ERR_EXIT(err);

        printf("xglResetEvent: Passed\n");

        err = xglGetEventStatus(event);
        if (err != XGL_EVENT_RESET)
            ERR_EXIT(err);

        printf("xglGetEventStatus (reset): Passed\n");

        err = xglSetEvent(event);
        if (err)
            ERR_EXIT(err);

        printf("xglSetEvent: Passed\n");

        err = xglGetEventStatus(event);
        if (err != XGL_EVENT_SET)
            ERR_EXIT(err);

        printf("xglGetEventStatus (set): Passed\n");

        // TODO: Test actual synchronization with command buffer event.

        // All done with event memory, clean up
        err = xglBindObjectMemory(event, XGL_NULL_HANDLE, 0);
        if (err)
            ERR_EXIT(err);

        printf("xglBindObjectMemory (Unbind memory): Passed\n");

        err = xglDestroyObject(event);
        if (err)
            ERR_EXIT(err);

        printf("xglDestroyObject (Event): Passed\n");
    }

    for (i = 0; i < gpu_count; i++)
        app_gpu_destroy(&gpus[i]);

    return 0;
}
