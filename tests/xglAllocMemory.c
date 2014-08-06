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
    XGL_UINT gpu_count, i, gpu_idx;
    XGL_RESULT err;
    XGL_MEMORY_ALLOC_INFO alloc_info;
    XGL_GPU_MEMORY gpu_mem;

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
        if (gpus[gpu_idx].dev.heap_count > 0) {
            memset(&alloc_info, 0, sizeof(alloc_info));

//                XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO
            alloc_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;

//                XGL_VOID*                               pNext;                      // Pointer to next structure
//                XGL_GPU_SIZE                            allocationSize;             // Size of memory allocation
//                XGL_GPU_SIZE                            alignment;
//                XGL_FLAGS                               flags;                      // XGL_MEMORY_ALLOC_FLAGS
//                XGL_UINT                                heapCount;
//                XGL_UINT                                heaps[XGL_MAX_MEMORY_HEAPS];

            alloc_info.allocationSize = 1024 * 1024; // 1MB
            alloc_info.alignment = 0;
            alloc_info.heapCount = 1;
            alloc_info.heaps[0] = 0; // TODO: Reference other heaps

// XGL_MEMORY_ALLOC_FLAGS
//                XGL_MEMORY_HEAP_CPU_VISIBLE_BIT                         = 0x00000001,
//                XGL_MEMORY_HEAP_CPU_GPU_COHERENT_BIT                    = 0x00000002,
//                XGL_MEMORY_HEAP_CPU_UNCACHED_BIT                        = 0x00000004,
//                XGL_MEMORY_HEAP_CPU_WRITE_COMBINED_BIT                  = 0x00000008,
//                XGL_MEMORY_HEAP_HOLDS_PINNED_BIT                        = 0x00000010,
//                XGL_MEMORY_HEAP_SHAREABLE_BIT                           = 0x00000020,
            // TODO: Pick heap properties indicated by heap info
            alloc_info.flags = XGL_MEMORY_HEAP_CPU_VISIBLE_BIT;

//   XGL_MEMORY_PRIORITY
//                XGL_MEMORY_PRIORITY_UNUSED                              = 0x0,
//                XGL_MEMORY_PRIORITY_VERY_LOW                            = 0x1,
//                XGL_MEMORY_PRIORITY_LOW                                 = 0x2,
//                XGL_MEMORY_PRIORITY_NORMAL                              = 0x3,
//                XGL_MEMORY_PRIORITY_HIGH                                = 0x4,
//                XGL_MEMORY_PRIORITY_VERY_HIGH                           = 0x5,

//                XGL_MEMORY_PRIORITY_BEGIN_RANGE                         = XGL_MEMORY_PRIORITY_UNUSED,
//                XGL_MEMORY_PRIORITY_END_RANGE                           = XGL_MEMORY_PRIORITY_VERY_HIGH,
//                XGL_NUM_MEMORY_PRIORITY                                 = (XGL_MEMORY_PRIORITY_END_RANGE - XGL_MEMORY_PRIORITY_BEGIN_RANGE + 1),
            // TODO: Try variety of memory priorities
            alloc_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

            err = xglAllocMemory(gpus[gpu_idx].dev.obj, &alloc_info, &gpu_mem);
            if (err)
                ERR_EXIT(err);

            err = xglFreeMemory(gpu_mem);
            if (err)
                ERR_EXIT(err);

        } else {
            debug_printf("No heaps available for GPU #%d: %s", gpu_idx, gpus[gpu_idx].props.gpuName);
        }
    }

    for (i = 0; i < gpu_count; i++)
        app_gpu_destroy(&gpus[i]);

    return 0;
}
