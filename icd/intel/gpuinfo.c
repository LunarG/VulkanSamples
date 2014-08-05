/*
 * XGL 3-D graphics library
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
 *    Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#include "GitSHA1.h"
#include "gpu.h"
#include "intel.h"

XGL_RESULT XGLAPI intelGetGpuInfo(
    XGL_PHYSICAL_GPU                            gpu_,
    XGL_PHYSICAL_GPU_INFO_TYPE                  infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    const struct intel_gpu *gpu = intel_gpu(gpu_);
    XGL_RESULT ret = XGL_SUCCESS;

    switch (infoType) {
    case XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES:
        if (pData == NULL) {
            return XGL_ERROR_INVALID_POINTER;
        }
        *pDataSize = sizeof(XGL_PHYSICAL_GPU_PROPERTIES);
        intel_gpu_get_props(gpu, pData);
        break;

    case XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE:
        if (pData == NULL) {
            return XGL_ERROR_INVALID_POINTER;
        }
        *pDataSize = sizeof(XGL_PHYSICAL_GPU_PERFORMANCE);
        intel_gpu_get_perf(gpu, pData);
        break;

    case XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES:
        /*
         * XGL Programmers guide, page 33:
         * to determine the data size an application calls
         * xglGetGpuInfo() with a NULL data pointer. The
         * expected data size for all queue property structures
         * is returned in pDataSize
         */
        *pDataSize = sizeof(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES) *
            INTEL_GPU_ENGINE_COUNT;
        if (pData != NULL) {
            XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *dst = pData;
            int engine;

            for (engine = 0; engine < INTEL_GPU_ENGINE_COUNT; engine++) {
                intel_gpu_get_queue_props(gpu, engine, dst);
                dst++;
            }
        }
        break;

    case XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES:
        if (pData == NULL) {
            return XGL_ERROR_INVALID_POINTER;
        }
        *pDataSize = sizeof(XGL_PHYSICAL_GPU_MEMORY_PROPERTIES);
        intel_gpu_get_memory_props(gpu, pData);
        break;

    default:
        ret = XGL_ERROR_INVALID_VALUE;
    }

    return ret;
}
