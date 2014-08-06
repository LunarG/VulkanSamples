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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "genhw/genhw.h"
#include "dispatch_tables.h"
#include "gpu.h"

static const char *gpu_get_name(const struct intel_gpu *gpu)
{
    const char *name = NULL;

    if (gen_is_hsw(gpu->devid)) {
        if (gen_is_desktop(gpu->devid))
            name = "Intel(R) Haswell Desktop";
        else if (gen_is_mobile(gpu->devid))
            name = "Intel(R) Haswell Mobile";
        else if (gen_is_server(gpu->devid))
            name = "Intel(R) Haswell Server";
    }
    else if (gen_is_ivb(gpu->devid)) {
        if (gen_is_desktop(gpu->devid))
            name = "Intel(R) Ivybridge Desktop";
        else if (gen_is_mobile(gpu->devid))
            name = "Intel(R) Ivybridge Mobile";
        else if (gen_is_server(gpu->devid))
            name = "Intel(R) Ivybridge Server";
    }
    else if (gen_is_snb(gpu->devid)) {
        if (gen_is_desktop(gpu->devid))
            name = "Intel(R) Sandybridge Desktop";
        else if (gen_is_mobile(gpu->devid))
            name = "Intel(R) Sandybridge Mobile";
        else if (gen_is_server(gpu->devid))
            name = "Intel(R) Sandybridge Server";
    }

    if (!name)
        name = "Unknown Intel Chipset";

    return name;
}

static int gpu_open_internal(struct intel_gpu *gpu)
{
    if (gpu->fd_internal < 0) {
        gpu->fd_internal = open(gpu->path, O_RDWR);
        if (gpu->fd_internal < 0) {
            icd_log(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0,
                    0, "failed to open %s", gpu->path);
        }
    }

    return gpu->fd_internal;
}

static void gpu_close_internal(struct intel_gpu *gpu)
{
    if (gpu->fd_internal >= 0) {
        close(gpu->fd_internal);
        gpu->fd_internal = -1;
    }
}

static struct intel_gpu *gpu_create(int gen, int devid, const char *path)
{
    struct intel_gpu *gpu;
    size_t path_len;

    gpu = icd_alloc(sizeof(*gpu), 0, XGL_SYSTEM_ALLOC_API_OBJECT);
    if (!gpu)
        return NULL;

    memset(gpu, 0, sizeof(*gpu));

    /* debug layer is always enabled for intel_gpu */
    gpu->dispatch = &intel_debug_dispatch_table;

    gpu->devid = devid;

    path_len = strlen(path);
    gpu->path = icd_alloc(path_len + 1, 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!gpu->path) {
        icd_free(gpu);
        return NULL;
    }
    memcpy(gpu->path, path, path_len + 1);

    gpu->gen_opaque = gen;

    /* 8192 dwords */
    gpu->batch_buffer_size = sizeof(uint32_t) * 8192;

    gpu->fd_internal = -1;
    gpu->fd = -1;

    return gpu;
}

static void gpu_destroy(struct intel_gpu *gpu)
{
    gpu_close_internal(gpu);
    icd_free(gpu->path);
    icd_free(gpu);
}

static struct intel_gpu *intel_gpus;

/**
 * Return true if \p gpu is a valid intel_gpu.
 */
bool intel_gpu_is_valid(const struct intel_gpu *gpu)
{
    const struct intel_gpu *iter = intel_gpus;

    while (iter) {
        if (iter == gpu)
            return true;
        iter = iter->next;
    }

    return false;
}

static int devid_to_gen(int devid)
{
    int gen;

    if (gen_is_hsw(devid))
        gen = INTEL_GEN(7.5);
    else if (gen_is_ivb(devid))
        gen = INTEL_GEN(7);
    else if (gen_is_snb(devid))
        gen = INTEL_GEN(6);
    else
        gen = -1;

    return gen;
}

XGL_RESULT intel_gpu_add(int devid, const char *path,
                         struct intel_gpu **gpu_ret)
{
    const int gen = devid_to_gen(devid);
    struct intel_gpu *gpu;

    if (gen < 0) {
        icd_log(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE,
                0, 0, "unsupported device id 0x%04x", devid);
        return XGL_ERROR_INITIALIZATION_FAILED;
    }

    gpu = gpu_create(gen, devid, path);
    if (!gpu)
        return XGL_ERROR_OUT_OF_MEMORY;

    gpu->next = intel_gpus;
    intel_gpus = gpu;

    *gpu_ret = gpu;

    return XGL_SUCCESS;
}

void intel_gpu_remove_all(void)
{
    struct intel_gpu *gpu = intel_gpus;

    while (gpu) {
        struct intel_gpu *next = gpu->next;

        gpu_destroy(gpu);
        gpu = next;
    }

    intel_gpus = NULL;
}

struct intel_gpu *intel_gpu_get_list(void)
{
    return intel_gpus;
}

void intel_gpu_get_props(const struct intel_gpu *gpu,
                         XGL_PHYSICAL_GPU_PROPERTIES *props)
{
    const char *name;
    size_t name_len;

    props->structSize = sizeof(*props);

    props->apiVersion = INTEL_API_VERSION;
    props->driverVersion = INTEL_DRIVER_VERSION;

    props->vendorId = 0x8086;
    props->deviceId = gpu->devid;

    props->gpuType = XGL_GPU_TYPE_INTEGRATED;

    /* copy GPU name */
    name = gpu_get_name(gpu);
    name_len = strlen(name);
    if (name_len > sizeof(props->gpuName) - 1)
        name_len = sizeof(props->gpuName) - 1;
    memcpy(props->gpuName, name, name_len);
    props->gpuName[name_len] = '\0';

    /* the winsys is prepared for one reloc every two dwords, then minus 2 */
    props->maxMemRefsPerSubmission =
        gpu->batch_buffer_size / sizeof(uint32_t) / 2 - 2;

    props->virtualMemPageSize = 4096;

    /* no size limit, but no bounded buffer could exceed 2GB */
    props->maxInlineMemoryUpdateSize = 2u << 30;

    props->maxBoundDescriptorSets = 1;
    props->maxThreadGroupSize = 512;

    /* incremented every 80ns */
    props->timestampFrequency = 1000 * 1000 * 1000 / 80;

    props->multiColorAttachmentClears = false;
}

void intel_gpu_get_perf(const struct intel_gpu *gpu,
                        XGL_PHYSICAL_GPU_PERFORMANCE *perf)
{
    /* TODO */
    perf->maxGpuClock = 1.0f;
    perf->aluPerClock = 1.0f;
    perf->texPerClock = 1.0f;
    perf->primsPerClock = 1.0f;
    perf->pixelsPerClock = 1.0f;
}

void intel_gpu_get_queue_props(const struct intel_gpu *gpu,
                               enum intel_gpu_engine_type engine,
                               XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *props)
{
    props->structSize = sizeof(*props);

    switch (engine) {
    case INTEL_GPU_ENGINE_3D:
        props->queueFlags = XGL_QUEUE_GRAPHICS_BIT | XGL_QUEUE_COMPUTE_BIT;
        props->queueCount = 1;
        props->maxAtomicCounters = 4096;
        props->supportsTimestamps = true;
        break;
    default:
        assert(!"unknown engine type");
        return;
    }
}

void intel_gpu_get_memory_props(const struct intel_gpu *gpu,
                                XGL_PHYSICAL_GPU_MEMORY_PROPERTIES *props)
{
    props->structSize = sizeof(*props);

    props->supportsMigration = false;

    /* no kernel support yet */
    props->supportsVirtualMemoryRemapping = false;

    /* no winsys support for DRM_I915_GEM_USERPTR yet */
    props->supportsPinning = false;
}

XGL_RESULT intel_gpu_open(struct intel_gpu *gpu)
{
    gpu->fd = gpu_open_internal(gpu);

    return (gpu->fd >= 0) ? XGL_SUCCESS : XGL_ERROR_UNKNOWN;
}

void intel_gpu_close(struct intel_gpu *gpu)
{
    gpu->fd = -1;
    gpu_close_internal(gpu);
}

bool intel_gpu_has_extension(const struct intel_gpu *gpu, const char *ext)
{
    return false;
}

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

XGL_RESULT XGLAPI intelGetExtensionSupport(
    XGL_PHYSICAL_GPU                            gpu_,
    const XGL_CHAR*                             pExtName)
{
    struct intel_gpu *gpu = intel_gpu(gpu_);

    return (intel_gpu_has_extension(gpu, (const char *) pExtName)) ?
        XGL_SUCCESS : XGL_ERROR_INVALID_EXTENSION;
}
