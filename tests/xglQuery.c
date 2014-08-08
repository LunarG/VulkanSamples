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

#define MAX_QUERY_SLOTS 10

int main(int argc, char **argv)
{
    static const XGL_APPLICATION_INFO app_info = {
        .sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = (const XGL_CHAR *) "xglQuery",
        .appVersion = 1,
        .pEngineName = (const XGL_CHAR *) "xglQuery",
        .engineVersion = 1,
        .apiVersion = XGL_MAKE_VERSION(0, 22, 0),
    };
    struct app_gpu gpus[MAX_GPUS];
    struct app_gpu *gpu;
    XGL_PHYSICAL_GPU objs[MAX_GPUS];
    XGL_UINT gpu_count, i, gpu_idx;
    XGL_QUERY_POOL_CREATE_INFO query_info;
    XGL_QUERY_POOL query_pool;
    XGL_UINT data_size;
    XGL_MEMORY_REQUIREMENTS mem_req;
    XGL_UINT query_result_size;
    XGL_UINT *query_result_data;
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

//        typedef enum _XGL_QUERY_TYPE
//        {
//            XGL_QUERY_OCCLUSION                                     = 0x00000000,
//            XGL_QUERY_PIPELINE_STATISTICS                           = 0x00000001,

//            XGL_QUERY_TYPE_BEGIN_RANGE                              = XGL_QUERY_OCCLUSION,
//            XGL_QUERY_TYPE_END_RANGE                                = XGL_QUERY_PIPELINE_STATISTICS,
//            XGL_NUM_QUERY_TYPE                                      = (XGL_QUERY_TYPE_END_RANGE - XGL_QUERY_TYPE_BEGIN_RANGE + 1),
//            XGL_MAX_ENUM(_XGL_QUERY_TYPE)
//        } XGL_QUERY_TYPE;

//        typedef struct _XGL_QUERY_POOL_CREATE_INFO
//        {
//            XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO
//            const XGL_VOID*                         pNext;      // Pointer to next structure
//            XGL_QUERY_TYPE                          queryType;
//            XGL_UINT                                slots;
//        } XGL_QUERY_POOL_CREATE_INFO;

        memset(&query_info, 0, sizeof(query_info));
        query_info.sType = XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        query_info.queryType = XGL_QUERY_OCCLUSION;
        query_info.slots = MAX_QUERY_SLOTS;

//        XGL_RESULT XGLAPI xglCreateQueryPool(
//            XGL_DEVICE                                  device,
//            const XGL_QUERY_POOL_CREATE_INFO*           pCreateInfo,
//            XGL_QUERY_POOL*                             pQueryPool);

        err = xglCreateQueryPool(gpu->dev.obj, &query_info, &query_pool);
        if (err)
            ERR_EXIT(err);

        printf("xglCreateQueryPool: Passed");

        err = xglGetObjectInfo(query_pool, XGL_INFO_TYPE_MEMORY_REQUIREMENTS,
                               &data_size, &mem_req);
//        XGL_RESULT XGLAPI xglAllocMemory(
//            XGL_DEVICE                                  device,
//            const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
//            XGL_GPU_MEMORY*                             pMem);
        XGL_MEMORY_ALLOC_INFO mem_info;
        XGL_GPU_MEMORY query_mem;

        if (data_size == 0) {
            ERR_MSG_EXIT("xglGetObjectInfo (Event): Failed - expect events to require memory");
        }
        memset(&mem_info, 0, sizeof(mem_info));
        mem_info.sType = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
        // TODO: Is a simple multiple all that's needed here?
        mem_info.allocationSize = mem_req.size * MAX_QUERY_SLOTS;
        mem_info.alignment = mem_req.alignment;
        mem_info.heapCount = mem_req.heapCount;
        memcpy(mem_info.heaps, mem_req.heaps, sizeof(XGL_UINT)*XGL_MAX_MEMORY_HEAPS);
        mem_info.memPriority = XGL_MEMORY_PRIORITY_NORMAL;

        // TODO: are the flags right?
        // TODO: Should this be pinned? Or maybe a separate test with pinned.
        mem_info.flags = XGL_MEMORY_ALLOC_SHAREABLE_BIT;
        err = xglAllocMemory(gpu->dev.obj, &mem_info, &query_mem);
        if (err)
            ERR_EXIT(err);

        printf("xglAllocMemory (QueryPool): Passed\n");

        err = xglBindObjectMemory(query_pool, query_mem, 0);
        if (err)
            ERR_EXIT(err);

        printf("xglBindObjectMemory (QueryPool): Passed\n");

        // TODO: Test actual synchronization with command buffer event.
        // TODO: Create command buffer
        // TODO: xglCmdResetQueryPool
        // TODO: xglCmdBeginQuery
        // TODO: commands
        // TOOD: xglCmdEndQuery

        err = xglGetQueryPoolResults(query_pool, 0, MAX_QUERY_SLOTS,
                                     &query_result_size, XGL_NULL_HANDLE);
        if (err)
            ERR_EXIT(err);

        printf("xglGetQueryPoolResults: Passed (size = %d)\n", query_result_size);

        if (query_result_size > 0) {
            query_result_data = malloc(query_result_size);
            err = xglGetQueryPoolResults(query_pool, 0, MAX_QUERY_SLOTS,
                                         &query_result_size, query_result_data);

            if (err)
                ERR_EXIT(err);

            // TODO: Test Query result data.

            printf("xglGetQueryPoolResults: Passed\n");
        }

        // All done with QueryPool memory, clean up
        err = xglBindObjectMemory(query_pool, XGL_NULL_HANDLE, 0);
        if (err)
            ERR_EXIT(err);

        printf("xglBindObjectMemory (Unbind memory): Passed\n");

        err = xglDestroyObject(query_pool);
        if (err)
            ERR_EXIT(err);

        printf("xglDestroyObject (QueryPool): Passed\n");
    }

    for (i = 0; i < gpu_count; i++)
        app_gpu_destroy(&gpus[i]);

    return 0;
}
