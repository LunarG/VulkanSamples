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
        .pAppName = (const XGL_CHAR *) "xglCreateDevice",
        .appVersion = 1,
        .pEngineName = (const XGL_CHAR *) "xglCreateDevice",
        .engineVersion = 1,
        .apiVersion = XGL_MAKE_VERSION(0, 22, 0),
    };
    struct app_gpu gpus[MAX_GPUS];
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    struct app_gpu *gpu;
    XGL_UINT gpu_count, i, gpu_idx;
    XGL_RESULT err;
    XGL_FENCE_CREATE_INFO fence_info;
    XGL_FENCE fence;

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
        if (gpu->dev.heap_count == 0) {
            debug_printf("No heaps available for GPU #%d: %s", gpu_idx, gpu->props.gpuName);
            continue;
        }

        memset(&fence_info, 0, sizeof(fence_info));

//            typedef struct _XGL_FENCE_CREATE_INFO
//            {
//                XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO
//                const XGL_VOID*                         pNext;      // Pointer to next structure
//                XGL_FLAGS                               flags;      // Reserved
        fence_info.sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        err = xglCreateFence(gpu->dev.obj, &fence_info, &fence);
        if (err)
            ERR_EXIT(err);

        printf("xglCreateFence: Passed\n");

        err = xglGetFenceStatus(fence);
        // We've not submitted this fence on a command buffer so should get
        // XGL_ERROR_UNAVAILABLE
        if (err != XGL_ERROR_UNAVAILABLE) {
            ERR_EXIT(err);
        }

        // Test glxWaitForFences
//        XGL_RESULT XGLAPI xglWaitForFences(
//            XGL_DEVICE                                  device,
//            XGL_UINT                                    fenceCount,
//            const XGL_FENCE*                            pFences,
//            XGL_BOOL                                    waitAll,
//            XGL_UINT64                                  timeout);
        err = xglWaitForFences(gpu->dev.obj, 1, &fence, XGL_TRUE, 0);
        if (err != XGL_ERROR_UNAVAILABLE)
            ERR_EXIT(err);

        printf("xglWaitForFences(UNAVAILABLE): Passed\n");

        // TODO: Attached to command buffer and test GetFenceStatus
        // TODO: Add some commands and submit the command buffer

        err = xglDestroyObject(fence);
        if (err)
            ERR_EXIT(err);

        printf("xglDestoryObject(fence): Passed\n");
    }

    for (i = 0; i < gpu_count; i++)
        app_gpu_destroy(&gpus[i]);

    return 0;
}
