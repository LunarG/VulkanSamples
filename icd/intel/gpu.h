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

#ifndef GPU_H
#define GPU_H

#include "intel.h"

#define INTEL_GPU_ASSERT(gpu, min_gen, max_gen)   \
       assert(intel_gpu_gen(gpu) >= INTEL_GEN(min_gen) && \
              intel_gpu_gen(gpu) <= INTEL_GEN(max_gen))

enum intel_ext_type {
    INTEL_EXT_WSI_X11,

    INTEL_EXT_COUNT,
    INTEL_EXT_INVALID = INTEL_EXT_COUNT,
};

enum intel_gpu_engine_type {
    /* TODO BLT support */
    INTEL_GPU_ENGINE_3D,

    INTEL_GPU_ENGINE_COUNT
};

struct intel_instance;
struct intel_wsi_display;
struct intel_winsys;

/*
 * intel_gpu is the only object that does not inherit from intel_base.
 */
struct intel_gpu {
    struct intel_handle handle;

    struct intel_gpu *next;

    int devid;          /* PCI device ID */
    char *primary_node; /* path to the primary node */
    char *render_node;  /* path to the render node */
    int gen_opaque;     /* always read this with intel_gpu_gen() */
    int gt;

    XGL_GPU_SIZE max_batch_buffer_size;
    uint32_t batch_buffer_reloc_count;

    /*
     * The enabled hardware features could be limited by the kernel.  These
     * mutable fds allows us to talk to the kernel before the device is
     * created.
     */
    int primary_fd_internal;
    int render_fd_internal;

    struct intel_winsys *winsys;

    void *wsi_data;

    struct intel_wsi_display **displays;
    uint32_t display_count;
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

XGL_RESULT intel_gpu_create(const struct intel_instance *instance, int devid,
                            const char *primary_node, const char *render_node,
                            struct intel_gpu **gpu_ret);
void intel_gpu_destroy(struct intel_gpu *gpu);

void intel_gpu_get_props(const struct intel_gpu *gpu,
                         XGL_PHYSICAL_GPU_PROPERTIES *props);
void intel_gpu_get_perf(const struct intel_gpu *gpu,
                        XGL_PHYSICAL_GPU_PERFORMANCE *perf);
void intel_gpu_get_queue_props(const struct intel_gpu *gpu,
                               enum intel_gpu_engine_type engine,
                               XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *props);
void intel_gpu_get_memory_props(const struct intel_gpu *gpu,
                                XGL_PHYSICAL_GPU_MEMORY_PROPERTIES *props);

int intel_gpu_get_max_threads(const struct intel_gpu *gpu,
                              XGL_PIPELINE_SHADER_STAGE stage);

int intel_gpu_get_primary_fd(struct intel_gpu *gpu);

XGL_RESULT intel_gpu_init_winsys(struct intel_gpu *gpu);
void intel_gpu_cleanup_winsys(struct intel_gpu *gpu);

enum intel_ext_type intel_gpu_lookup_extension(const struct intel_gpu *gpu,
                                               const char *ext);

#endif /* GPU_H */
