/*
 * Vulkan
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
#include "queue.h"
#include "gpu.h"
#include "instance.h"
#include "wsi.h"
#include "vk_debug_report_lunarg.h"
#include "vk_debug_marker_lunarg.h"

static int gpu_open_primary_node(struct intel_gpu *gpu)
{
    if (gpu->primary_fd_internal < 0)
        gpu->primary_fd_internal = open(gpu->primary_node, O_RDWR);

    return gpu->primary_fd_internal;
}

static void gpu_close_primary_node(struct intel_gpu *gpu)
{
    if (gpu->primary_fd_internal >= 0) {
        close(gpu->primary_fd_internal);
        gpu->primary_fd_internal = -1;
    }
}

static int gpu_open_render_node(struct intel_gpu *gpu)
{
    if (gpu->render_fd_internal < 0 && gpu->render_node) {
        gpu->render_fd_internal = open(gpu->render_node, O_RDWR);
        if (gpu->render_fd_internal < 0) {
            intel_log(gpu, VK_DBG_REPORT_ERROR_BIT, 0, VK_NULL_HANDLE, 0,
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

void intel_gpu_destroy(struct intel_gpu *gpu)
{
    intel_wsi_gpu_cleanup(gpu);

    intel_gpu_cleanup_winsys(gpu);

    intel_free(gpu, gpu->primary_node);
    intel_free(gpu, gpu);
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

VkResult intel_gpu_create(const struct intel_instance *instance, int devid,
                            const char *primary_node, const char *render_node,
                            struct intel_gpu **gpu_ret)
{
    const int gen = devid_to_gen(devid);
    size_t primary_len, render_len;
    struct intel_gpu *gpu;

    if (gen < 0) {
        intel_log(instance, VK_DBG_REPORT_WARN_BIT, 0,
                VK_NULL_HANDLE, 0, 0, "unsupported device id 0x%04x", devid);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    gpu = intel_alloc(instance, sizeof(*gpu), 0, VK_SYSTEM_ALLOC_TYPE_API_OBJECT);
    if (!gpu)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    memset(gpu, 0, sizeof(*gpu));
    /* there is no VK_DBG_OBJECT_GPU */
    intel_handle_init(&gpu->handle, VK_OBJECT_TYPE_PHYSICAL_DEVICE, instance->icd);

    gpu->devid = devid;

    primary_len = strlen(primary_node);
    render_len = (render_node) ? strlen(render_node) : 0;

    gpu->primary_node = intel_alloc(gpu, primary_len + 1 +
            ((render_len) ? (render_len + 1) : 0), 0, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!gpu->primary_node) {
        intel_free(instance, gpu);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    memcpy(gpu->primary_node, primary_node, primary_len + 1);

    if (render_node) {
        gpu->render_node = gpu->primary_node + primary_len + 1;
        memcpy(gpu->render_node, render_node, render_len + 1);
    } else {
        gpu->render_node = gpu->primary_node;
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

    /* 150K dwords */
    gpu->max_batch_buffer_size = sizeof(uint32_t) * 150*1024;

    /* the winsys is prepared for one reloc every two dwords, then minus 2 */
    gpu->batch_buffer_reloc_count =
        gpu->max_batch_buffer_size / sizeof(uint32_t) / 2 - 2;

    gpu->primary_fd_internal = -1;
    gpu->render_fd_internal = -1;

    *gpu_ret = gpu;

    return VK_SUCCESS;
}

void intel_gpu_get_props(const struct intel_gpu *gpu,
                         VkPhysicalDeviceProperties *props)
{
    const char *name;
    size_t name_len;

    props->apiVersion = INTEL_API_VERSION;
    props->driverVersion = INTEL_DRIVER_VERSION;

    props->vendorId = 0x8086;
    props->deviceId = gpu->devid;

    props->deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

    /* copy GPU name */
    name = gpu_get_name(gpu);
    name_len = strlen(name);
    if (name_len > sizeof(props->deviceName) - 1)
        name_len = sizeof(props->deviceName) - 1;
    memcpy(props->deviceName, name, name_len);
    props->deviceName[name_len] = '\0';


    /* no size limit, but no bounded buffer could exceed 2GB */
    props->maxInlineMemoryUpdateSize = 2u << 30;
    props->maxBoundDescriptorSets = 1;
    props->maxThreadGroupSize = 512;

    /* incremented every 80ns */
    props->timestampFrequency = 1000 * 1000 * 1000 / 80;

    props->multiColorAttachmentClears = false;

    /* hardware is limited to 16 viewports */
    props->maxViewports = INTEL_MAX_VIEWPORTS;

    props->maxColorAttachments = INTEL_MAX_RENDER_TARGETS;

    /* ? */
    props->maxDescriptorSets = 2;
}

void intel_gpu_get_perf(const struct intel_gpu *gpu,
                        VkPhysicalDevicePerformance *perf)
{
    /* TODO */
    perf->maxDeviceClock = 1.0f;
    perf->aluPerClock = 1.0f;
    perf->texPerClock = 1.0f;
    perf->primsPerClock = 1.0f;
    perf->pixelsPerClock = 1.0f;
}

void intel_gpu_get_queue_props(const struct intel_gpu *gpu,
                               enum intel_gpu_engine_type engine,
                               VkPhysicalDeviceQueueProperties *props)
{
    switch (engine) {
    case INTEL_GPU_ENGINE_3D:
        props->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
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
                                VkPhysicalDeviceMemoryProperties *props)
{
    props->supportsMigration = false;
    props->supportsPinning = true;
}

int intel_gpu_get_max_threads(const struct intel_gpu *gpu,
                              VkShaderStage stage)
{
    switch (intel_gpu_gen(gpu)) {
    case INTEL_GEN(7.5):
        switch (stage) {
        case VK_SHADER_STAGE_VERTEX:
            return (gpu->gt >= 2) ? 280 : 70;
        case VK_SHADER_STAGE_GEOMETRY:
            /* values from ilo_gpe_init_gs_cso_gen7 */
            return (gpu->gt >= 2) ? 256 : 70;
        case VK_SHADER_STAGE_FRAGMENT:
            return (gpu->gt == 3) ? 408 :
                   (gpu->gt == 2) ? 204 : 102;
        default:
            break;
        }
        break;
    case INTEL_GEN(7):
        switch (stage) {
        case VK_SHADER_STAGE_VERTEX:
            return (gpu->gt == 2) ? 128 : 36;
        case VK_SHADER_STAGE_GEOMETRY:
            /* values from ilo_gpe_init_gs_cso_gen7 */
            return (gpu->gt == 2) ? 128 : 36;
        case VK_SHADER_STAGE_FRAGMENT:
            return (gpu->gt == 2) ? 172 : 48;
        default:
            break;
        }
        break;
    case INTEL_GEN(6):
        switch (stage) {
        case VK_SHADER_STAGE_VERTEX:
            return (gpu->gt == 2) ? 60 : 24;
        case VK_SHADER_STAGE_GEOMETRY:
            /* values from ilo_gpe_init_gs_cso_gen6 */
            return (gpu->gt == 2) ? 28 : 21;
        case VK_SHADER_STAGE_FRAGMENT:
            return (gpu->gt == 2) ? 80 : 40;
        default:
            break;
        }
        break;
    default:
        break;
    }

    intel_log(gpu, VK_DBG_REPORT_ERROR_BIT, 0, VK_NULL_HANDLE,
            0, 0, "unknown Gen or shader stage");

    switch (stage) {
    case VK_SHADER_STAGE_VERTEX:
        return 1;
    case VK_SHADER_STAGE_GEOMETRY:
        return 1;
    case VK_SHADER_STAGE_FRAGMENT:
        return 4;
    default:
        return 1;
    }
}

int intel_gpu_get_primary_fd(struct intel_gpu *gpu)
{
    return gpu_open_primary_node(gpu);
}

VkResult intel_gpu_init_winsys(struct intel_gpu *gpu)
{
    int fd;

    assert(!gpu->winsys);

    fd = gpu_open_render_node(gpu);
    if (fd < 0)
        return VK_ERROR_UNKNOWN;

    gpu->winsys = intel_winsys_create_for_fd(gpu->handle.icd, fd);
    if (!gpu->winsys) {
        intel_log(gpu, VK_DBG_REPORT_ERROR_BIT, 0,
                VK_NULL_HANDLE, 0, 0, "failed to create GPU winsys");
        gpu_close_render_node(gpu);
        return VK_ERROR_UNKNOWN;
    }

    return VK_SUCCESS;
}

void intel_gpu_cleanup_winsys(struct intel_gpu *gpu)
{
    if (gpu->winsys) {
        intel_winsys_destroy(gpu->winsys);
        gpu->winsys = NULL;
    }

    gpu_close_primary_node(gpu);
    gpu_close_render_node(gpu);
}

enum intel_phy_dev_ext_type intel_gpu_lookup_phy_dev_extension(
        const struct intel_gpu *gpu,
        const VkExtensionProperties *ext)
{
    enum intel_phy_dev_ext_type type;

    for (type = 0; type < ARRAY_SIZE(intel_phy_dev_gpu_exts); type++) {
        if (compare_vk_extension_properties(&intel_phy_dev_gpu_exts[type], ext))
            break;
    }

    assert(type < INTEL_PHY_DEV_EXT_COUNT || type == INTEL_PHY_DEV_EXT_INVALID);

    return type;
}

ICD_EXPORT VkResult VKAPI vkGetPhysicalDeviceInfo(
    VkPhysicalDevice                            gpu_,
    VkPhysicalDeviceInfoType                  infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    struct intel_gpu *gpu = intel_gpu(gpu_);
    VkResult ret = VK_SUCCESS;

    switch (infoType) {
    case VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES:
        *pDataSize = sizeof(VkPhysicalDeviceProperties);
        if (pData == NULL) {
            return ret;
        }
        intel_gpu_get_props(gpu, pData);
        break;

    case VK_PHYSICAL_DEVICE_INFO_TYPE_PERFORMANCE:
        *pDataSize = sizeof(VkPhysicalDevicePerformance);
        if (pData == NULL) {
            return ret;
        }
        intel_gpu_get_perf(gpu, pData);
        break;

    case VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES:
        /*
         * Vulkan Programmers guide, page 33:
         * to determine the data size an application calls
         * vkGetPhysicalDeviceInfo() with a NULL data pointer. The
         * expected data size for all queue property structures
         * is returned in pDataSize
         */
        *pDataSize = sizeof(VkPhysicalDeviceQueueProperties) *
            INTEL_GPU_ENGINE_COUNT;
        if (pData != NULL) {
            VkPhysicalDeviceQueueProperties *dst = pData;
            int engine;

            for (engine = 0; engine < INTEL_GPU_ENGINE_COUNT; engine++) {
                intel_gpu_get_queue_props(gpu, engine, dst);
                dst++;
            }
        }
        break;

    case VK_PHYSICAL_DEVICE_INFO_TYPE_MEMORY_PROPERTIES:
        *pDataSize = sizeof(VkPhysicalDeviceMemoryProperties);
        if (pData == NULL) {
            return ret;
        }
        intel_gpu_get_memory_props(gpu, pData);
        break;

    default:
        ret = intel_wsi_gpu_get_info(gpu, infoType, pDataSize, pData);
        break;
    }

    return ret;
}

ICD_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = INTEL_GLOBAL_EXT_COUNT;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            /* check that *pDataSize is big enough*/
            if (*pDataSize < sizeof(VkExtensionProperties))
                return VK_ERROR_INVALID_MEMORY_SIZE;

            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= INTEL_GLOBAL_EXT_COUNT)
                return VK_ERROR_INVALID_VALUE;
            memcpy((VkExtensionProperties *) pData, &intel_global_gpu_exts[extensionIndex], sizeof(VkExtensionProperties));
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionInfo(
                                               VkPhysicalDevice gpu,
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    /*
     * If/when we have device-specific extensions, should retrieve them
     * based on the passed-in physical device
     *
     *VkExtensionProperties *ext_props;
     */
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = INTEL_PHY_DEV_EXT_COUNT;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;

            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= INTEL_PHY_DEV_EXT_COUNT)
                return VK_ERROR_INVALID_VALUE;
            memcpy((VkExtensionProperties *) pData, &intel_phy_dev_gpu_exts[extensionIndex], sizeof(VkExtensionProperties));
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetMultiDeviceCompatibility(
    VkPhysicalDevice                            gpu0_,
    VkPhysicalDevice                            gpu1_,
    VkPhysicalDeviceCompatibilityInfo*                 pInfo)
{
    const struct intel_gpu *gpu0 = intel_gpu(gpu0_);
    const struct intel_gpu *gpu1 = intel_gpu(gpu1_);
    VkFlags compat = VK_PHYSICAL_DEVICE_COMPATIBILITY_IQ_MATCH_BIT |
                       VK_PHYSICAL_DEVICE_COMPATIBILITY_PEER_TRANSFER_BIT |
                       VK_PHYSICAL_DEVICE_COMPATIBILITY_SHARED_MEMORY_BIT |
                       VK_PHYSICAL_DEVICE_COMPATIBILITY_SHARED_DEVICE0_DISPLAY_BIT |
                       VK_PHYSICAL_DEVICE_COMPATIBILITY_SHARED_DEVICE1_DISPLAY_BIT;

    if (intel_gpu_gen(gpu0) == intel_gpu_gen(gpu1))
        compat |= VK_PHYSICAL_DEVICE_COMPATIBILITY_FEATURES_BIT;

    pInfo->compatibilityFlags = compat;

    return VK_SUCCESS;
}
