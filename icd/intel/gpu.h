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

#ifndef GPU_H
#define GPU_H

#include "intel.h"

#define INTEL_GPU_ASSERT(gpu, min_gen, max_gen)   \
       assert(intel_gpu_gen(gpu) >= INTEL_GEN(min_gen) && \
              intel_gpu_gen(gpu) <= INTEL_GEN(max_gen))

enum intel_gpu_engine_type {
    /* TODO BLT support */
    INTEL_GPU_ENGINE_3D,

    INTEL_GPU_ENGINE_COUNT
};

/*
 * intel_gpu is the only object that does not inherit from intel_base.
 */
struct intel_gpu {
    const struct icd_dispatch_table *dispatch;

    struct intel_gpu *next;

    int devid;          /* PCI device ID */
    char *path;         /* path to the render or legacy node, or NULL */
    int gen_opaque;     /* always read with intel_gpu_gen() */

    int batch_buffer_size;

    /*
     * The enabled hardware features could be limited by the kernel.  This
     * mutable internal fd allows us to talk to the kernel when we need to.
     */
    int fd_internal;

    int fd;
};

static inline struct intel_gpu *intel_gpu(XGL_PHYSICAL_GPU gpu)
{
    return (struct intel_gpu *) gpu;
}

static inline int intel_gpu_gen(const struct intel_gpu *gpu)
{
#ifdef INTEL_GEN_SPECIALIZED
    return INTEL_GEN(INTEL_GEN_SPECIALIZED);
#else
    return gpu->gen_opaque;
#endif
}

bool intel_gpu_is_valid(const struct intel_gpu *gpu);

XGL_RESULT intel_gpu_add(int devid, const char *path,
                         struct intel_gpu **gpu_ret);
void intel_gpu_remove_all(void);
struct intel_gpu *intel_gpu_get_list(void);

void intel_gpu_get_props(const struct intel_gpu *gpu,
                         XGL_PHYSICAL_GPU_PROPERTIES *props);
void intel_gpu_get_perf(const struct intel_gpu *gpu,
                        XGL_PHYSICAL_GPU_PERFORMANCE *perf);
void intel_gpu_get_queue_props(const struct intel_gpu *gpu,
                               enum intel_gpu_engine_type engine,
                               XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *props);
void intel_gpu_get_memory_props(const struct intel_gpu *gpu,
                                XGL_PHYSICAL_GPU_MEMORY_PROPERTIES *props);

XGL_RESULT intel_gpu_open(struct intel_gpu *gpu);
void intel_gpu_close(struct intel_gpu *gpu);

bool intel_gpu_has_extension(const struct intel_gpu *gpu, const char *ext);

XGL_RESULT XGLAPI intelGetGpuInfo(
    XGL_PHYSICAL_GPU                            gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE                  infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

XGL_RESULT XGLAPI intelGetExtensionSupport(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_CHAR*                             pExtName);

#endif /* GPU_H */
