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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "genhw/genhw.h"
#include "kmd/winsys.h"
#include "dispatch.h"
#include "queue.h"
#include "gpu.h"
#include "wsi_x11.h"

static struct intel_gpu *intel_gpus;

static const char *intel_gpu_exts[INTEL_EXT_COUNT] = {
#ifdef ENABLE_WSI_X11
    [INTEL_EXT_WSI_X11] = "XGL_WSI_X11",
#endif
    [INTEL_EXT_COMPILE_GLSL] = "XGL_COMPILE_GLSL",
};

static int gpu_open_primary_node(struct intel_gpu *gpu)
{
    /* cannot not open gpu->primary_node directly */
    return gpu->primary_fd_internal;
}

static void gpu_close_primary_node(struct intel_gpu *gpu)
{
    if (gpu->primary_fd_internal >= 0)
        gpu->primary_fd_internal = -1;
}

static int gpu_open_render_node(struct intel_gpu *gpu)
{
    if (gpu->render_fd_internal < 0 && gpu->render_node) {
        gpu->render_fd_internal = open(gpu->render_node, O_RDWR);
        if (gpu->render_fd_internal < 0) {
            icd_log(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, NULL, 0,
                    0, "failed to open %s", gpu->render_node);
        }
    }

    return gpu->render_fd_internal;
}

static void gpu_close_render_node(struct intel_gpu *gpu)
{
    if (gpu->render_fd_internal >= 0) {
        close(gpu->render_fd_internal);
        gpu->render_fd_internal = -1;
    }
}

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

static struct intel_gpu *gpu_create(int gen, int devid,
                                    const char *primary_node,
                                    const char *render_node)
{
    struct intel_gpu *gpu;
    size_t primary_len, render_len;

    gpu = icd_alloc(sizeof(*gpu), 0, XGL_SYSTEM_ALLOC_API_OBJECT);
    if (!gpu)
        return NULL;

    memset(gpu, 0, sizeof(*gpu));

    /* debug layer is always enabled for intel_gpu */
    gpu->dispatch = intel_dispatch_get(true);

    gpu->devid = devid;

    primary_len = strlen(primary_node);
    render_len = (render_node) ? strlen(render_node) : 0;

    gpu->primary_node = icd_alloc(primary_len + 1 +
            ((render_len) ? (render_len + 1) : 0), 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!gpu->primary_node) {
        icd_free(gpu);
        return NULL;
    }

    memcpy(gpu->primary_node, primary_node, primary_len + 1);

    if (render_node) {
        gpu->render_node = gpu->primary_node + primary_len + 1;
        memcpy(gpu->render_node, render_node, render_len + 1);
    }

    gpu->gen_opaque = gen;

    switch (intel_gpu_gen(gpu)) {
    case INTEL_GEN(7.5):
        gpu->gt = gen_get_hsw_gt(devid);
        break;
    case INTEL_GEN(7):
        gpu->gt = gen_get_ivb_gt(devid);
        break;
    case INTEL_GEN(6):
        gpu->gt = gen_get_snb_gt(devid);
        break;
    }

    /* 8192 dwords */
    gpu->max_batch_buffer_size = sizeof(uint32_t) * 8192;

    /* the winsys is prepared for one reloc every two dwords, then minus 2 */
    gpu->batch_buffer_reloc_count =
        gpu->max_batch_buffer_size / sizeof(uint32_t) / 2 - 2;

    gpu->primary_fd_internal = -1;
    gpu->render_fd_internal = -1;

    return gpu;
}

static void gpu_destroy(struct intel_gpu *gpu)
{
    intel_gpu_close(gpu);

#ifdef ENABLE_WSI_X11
    if (gpu->x11)
        intel_wsi_x11_destroy(gpu->x11);
#endif

    icd_free(gpu->primary_node);
    icd_free(gpu);
}

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

#ifdef INTEL_GEN_SPECIALIZED
    if (gen != INTEL_GEN(INTEL_GEN_SPECIALIZED))
        gen = -1;
#endif

    return gen;
}

XGL_RESULT intel_gpu_add(int devid, const char *primary_node,
                         const char *render_node, struct intel_gpu **gpu_ret)
{
    const int gen = devid_to_gen(devid);
    struct intel_gpu *gpu;

    if (gen < 0) {
        icd_log(XGL_DBG_MSG_WARNING, XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE,
                0, 0, "unsupported device id 0x%04x", devid);
        return XGL_ERROR_INITIALIZATION_FAILED;
    }

    gpu = gpu_create(gen, devid, primary_node, render_node);
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

    props->maxMemRefsPerSubmission = gpu->batch_buffer_reloc_count;

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
        props->maxAtomicCounters = INTEL_QUEUE_ATOMIC_COUNTER_COUNT;
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

void intel_gpu_associate_x11(struct intel_gpu *gpu,
                             struct intel_wsi_x11 *x11,
                             int fd)
{
#ifdef ENABLE_WSI_X11
    gpu->x11 = x11;
    gpu->primary_fd_internal = fd;
#endif
}

XGL_RESULT intel_gpu_open(struct intel_gpu *gpu)
{
    int fd;

    assert(!gpu->winsys);

    fd = gpu_open_primary_node(gpu);
    if (fd < 0)
        fd = gpu_open_render_node(gpu);
    if (fd < 0)
        return XGL_ERROR_UNKNOWN;

    gpu->winsys = intel_winsys_create_for_fd(fd);
    if (!gpu->winsys) {
        icd_log(XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE,
                0, 0, "failed to create GPU winsys");
        intel_gpu_close(gpu);
        return XGL_ERROR_UNKNOWN;
    }

    return XGL_SUCCESS;
}

void intel_gpu_close(struct intel_gpu *gpu)
{
    if (gpu->winsys) {
        intel_winsys_destroy(gpu->winsys);
        gpu->winsys = NULL;
    }

    gpu_close_primary_node(gpu);
    gpu_close_render_node(gpu);
}

enum intel_ext_type intel_gpu_lookup_extension(const struct intel_gpu *gpu,
                                               const char *ext)
{
    enum intel_ext_type type;

    for (type = 0; type < ARRAY_SIZE(intel_gpu_exts); type++) {
        if (intel_gpu_exts[type] && strcmp(intel_gpu_exts[type], ext) == 0)
            break;
    }

    assert(type < INTEL_EXT_COUNT || type == INTEL_EXT_INVALID);

    return type;
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
    const enum intel_ext_type ext = intel_gpu_lookup_extension(gpu,
            (const char *) pExtName);

    return (ext != INTEL_EXT_INVALID) ?
        XGL_SUCCESS : XGL_ERROR_INVALID_EXTENSION;
}

XGL_RESULT XGLAPI intelGetMultiGpuCompatibility(
    XGL_PHYSICAL_GPU                            gpu0_,
    XGL_PHYSICAL_GPU                            gpu1_,
    XGL_GPU_COMPATIBILITY_INFO*                 pInfo)
{
    const struct intel_gpu *gpu0 = intel_gpu(gpu0_);
    const struct intel_gpu *gpu1 = intel_gpu(gpu1_);
    XGL_FLAGS compat = XGL_GPU_COMPAT_IQ_MATCH_BIT |
                       XGL_GPU_COMPAT_PEER_TRANSFER_BIT |
                       XGL_GPU_COMPAT_SHARED_MEMORY_BIT |
                       XGL_GPU_COMPAT_SHARED_GPU0_DISPLAY_BIT |
                       XGL_GPU_COMPAT_SHARED_GPU1_DISPLAY_BIT;

    if (intel_gpu_gen(gpu0) == intel_gpu_gen(gpu1))
        compat |= XGL_GPU_COMPAT_ASIC_FEATURES_BIT;

    pInfo->compatibilityFlags = compat;

    return XGL_SUCCESS;
}
