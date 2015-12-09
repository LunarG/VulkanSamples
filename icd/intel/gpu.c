/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 *
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
#include <vulkan/vk_lunarg_debug_report.h>
#include "vulkan/vk_lunarg_debug_marker.h"

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
            intel_log(gpu, VK_DEBUG_REPORT_ERROR_BIT, 0, VK_NULL_HANDLE, 0,
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
        intel_log(instance, VK_DEBUG_REPORT_WARN_BIT, 0,
                VK_NULL_HANDLE, 0, 0, "unsupported device id 0x%04x", devid);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    gpu = intel_alloc(instance, sizeof(*gpu), 0, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
    if (!gpu)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    memset(gpu, 0, sizeof(*gpu));
    /* there is no VK_DBG_OBJECT_GPU */
    intel_handle_init(&gpu->handle, VK_OBJECT_TYPE_PHYSICAL_DEVICE, instance);

    gpu->devid = devid;

    primary_len = strlen(primary_node);
    render_len = (render_node) ? strlen(render_node) : 0;

    gpu->primary_node = intel_alloc(gpu, primary_len + 1 +
            ((render_len) ? (render_len + 1) : 0), 0, VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
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

void intel_gpu_get_limits(VkPhysicalDeviceLimits *pLimits)
{
    // TODO: fill out more limits
    memset(pLimits, 0, sizeof(*pLimits));

    // no size limit, but no bounded buffer could exceed 2GB
    pLimits->maxBoundDescriptorSets         = 1;
    pLimits->maxComputeWorkGroupInvocations = 512;

    // incremented every 80ns
    pLimits->timestampPeriod = 80.0f;

    // hardware is limited to 16 viewports
    pLimits->maxViewports        = INTEL_MAX_VIEWPORTS;
    pLimits->maxColorAttachments = INTEL_MAX_RENDER_TARGETS;

    // ?
    pLimits->maxImageDimension1D   = 8192;
    pLimits->maxImageDimension2D   = 8192;
    pLimits->maxImageDimension3D   = 8192;
    pLimits->maxImageDimensionCube = 8192;
    pLimits->maxImageArrayLayers   = 2048;
    pLimits->maxTexelBufferElements = 128 * 1024 * 1024;  // 128M texels hard limit
    pLimits->maxUniformBufferRange = 64 * 1024;          // not hard limit

    /* HW has two per-stage resource tables:
     * - samplers, 16 per stage on IVB; blocks of 16 on HSW+ via shader hack, as the
     *   table base ptr used by the sampler hw is under shader sw control.
     *
     * - binding table entries, 250 total on all gens, shared between
     *   textures, RT, images, SSBO, UBO, ...
     *   the top few indices (250-255) are used for 'stateless' access with various cache
     *   options, and for SLM access.
     */
    pLimits->maxPerStageDescriptorSamplers       = 16;        // technically more on HSW+..
    pLimits->maxDescriptorSetSamplers            = 16;

    pLimits->maxPerStageDescriptorUniformBuffers = 128;
    pLimits->maxDescriptorSetUniformBuffers      = 128;

    pLimits->maxPerStageDescriptorSampledImages  = 128;
    pLimits->maxDescriptorSetSampledImages       = 128;

    // storage images and buffers not implemented; left at zero

    pLimits->maxPerStageResources  = 250;

    // required to support at least two queue priorities
    pLimits->discreteQueuePriorities = 2;
}

void intel_gpu_get_props(const struct intel_gpu *gpu,
                         VkPhysicalDeviceProperties *props)
{
    const char *name;
    size_t name_len;

    props->apiVersion = INTEL_API_VERSION;
    props->driverVersion = INTEL_DRIVER_VERSION;

    props->vendorID = 0x8086;
    props->deviceID = gpu->devid;

    props->deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

    /* copy GPU name */
    name = gpu_get_name(gpu);
    name_len = strlen(name);
    if (name_len > sizeof(props->deviceName) - 1)
        name_len = sizeof(props->deviceName) - 1;
    memcpy(props->deviceName, name, name_len);
    props->deviceName[name_len] = '\0';

    intel_gpu_get_limits(&props->limits);

    intel_gpu_get_sparse_properties(&props->sparseProperties);
}

void intel_gpu_get_queue_props(const struct intel_gpu *gpu,
                               enum intel_gpu_engine_type engine,
                               VkQueueFamilyProperties *props)
{
    switch (engine) {
    case INTEL_GPU_ENGINE_3D:
        props->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        props->queueCount = 1;
        props->timestampValidBits = 0;
        props->minImageTransferGranularity.width = 1;
        props->minImageTransferGranularity.height = 1;
        props->minImageTransferGranularity.depth = 1;
        break;
    default:
        assert(!"unknown engine type");
        return;
    }
}

void intel_gpu_get_memory_props(const struct intel_gpu *gpu,
                                VkPhysicalDeviceMemoryProperties *props)
{
    memset(props, 0, sizeof(VkPhysicalDeviceMemoryProperties));
    props->memoryTypeCount = INTEL_MEMORY_TYPE_COUNT;
    props->memoryHeapCount = INTEL_MEMORY_HEAP_COUNT;

    // For now, Intel will support one memory type
    for (uint32_t i = 0; i < props->memoryTypeCount; i++) {
        assert(props->memoryTypeCount == 1);
        props->memoryTypes[i].propertyFlags = INTEL_MEMORY_PROPERTY_ALL;
        props->memoryTypes[i].heapIndex     = i;
    }

    // For now, Intel will support a single heap with all available memory
    for (uint32_t i = 0; i < props->memoryHeapCount; i++) {
        assert(props->memoryHeapCount == 1);
        props->memoryHeaps[0].size = INTEL_MEMORY_HEAP_SIZE;
    }
}

int intel_gpu_get_max_threads(const struct intel_gpu *gpu,
                              VkShaderStageFlagBits stage)
{
    switch (intel_gpu_gen(gpu)) {
    case INTEL_GEN(7.5):
        switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return (gpu->gt >= 2) ? 280 : 70;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            /* values from ilo_gpe_init_gs_cso_gen7 */
            return (gpu->gt >= 2) ? 256 : 70;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return (gpu->gt == 3) ? 408 :
                   (gpu->gt == 2) ? 204 : 102;
        default:
            break;
        }
        break;
    case INTEL_GEN(7):
        switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return (gpu->gt == 2) ? 128 : 36;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            /* values from ilo_gpe_init_gs_cso_gen7 */
            return (gpu->gt == 2) ? 128 : 36;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return (gpu->gt == 2) ? 172 : 48;
        default:
            break;
        }
        break;
    case INTEL_GEN(6):
        switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            return (gpu->gt == 2) ? 60 : 24;
        case VK_SHADER_STAGE_GEOMETRY_BIT:
            /* values from ilo_gpe_init_gs_cso_gen6 */
            return (gpu->gt == 2) ? 28 : 21;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return (gpu->gt == 2) ? 80 : 40;
        default:
            break;
        }
        break;
    default:
        break;
    }

    intel_log(gpu, VK_DEBUG_REPORT_ERROR_BIT, 0, VK_NULL_HANDLE,
            0, 0, "unknown Gen or shader stage");

    switch (stage) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return 1;
    case VK_SHADER_STAGE_GEOMETRY_BIT:
        return 1;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
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
        return VK_ERROR_INITIALIZATION_FAILED;

    gpu->winsys = intel_winsys_create_for_fd(gpu->handle.instance->icd, fd);
    if (!gpu->winsys) {
        intel_log(gpu, VK_DEBUG_REPORT_ERROR_BIT, 0,
                VK_NULL_HANDLE, 0, 0, "failed to create GPU winsys");
        gpu_close_render_node(gpu);
        return VK_ERROR_INITIALIZATION_FAILED;
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
        const char *ext)
{
    uint32_t type;
    uint32_t array_size = ARRAY_SIZE(intel_phy_dev_gpu_exts);

    for (type = 0; type < array_size; type++) {
        if (compare_vk_extension_properties(&intel_phy_dev_gpu_exts[type], ext))
            break;
    }

    assert(type < array_size || type == INTEL_PHY_DEV_EXT_INVALID);

    return type;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice gpu_,
    VkPhysicalDeviceProperties* pProperties)
{
    struct intel_gpu *gpu = intel_gpu(gpu_);

    intel_gpu_get_props(gpu, pProperties);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice gpu_,
    uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties* pProperties)
{
   struct intel_gpu *gpu = intel_gpu(gpu_);
   int engine;

   if (pProperties == NULL) {
       *pQueueFamilyPropertyCount = INTEL_GPU_ENGINE_COUNT;
       return;
   }

   for (engine = 0; engine < *pQueueFamilyPropertyCount; engine++) {
       intel_gpu_get_queue_props(gpu, engine, pProperties);
       pProperties++;
   }
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice gpu_,
    VkPhysicalDeviceMemoryProperties* pProperties)
{
   struct intel_gpu *gpu = intel_gpu(gpu_);

   intel_gpu_get_memory_props(gpu, pProperties);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(
                                               VkPhysicalDevice physicalDevice,
                                               VkPhysicalDeviceFeatures* pFeatures)
{
    /* TODO: fill out features */
    memset(pFeatures, 0, sizeof(*pFeatures));
    pFeatures->occlusionQueryPrecise = 1;
}

void intel_gpu_get_sparse_properties(VkPhysicalDeviceSparseProperties *pProps)
{
    memset(pProps, 0, sizeof(*pProps));
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pPropertyCount,
        VkExtensionProperties*                      pProperties)
{
    uint32_t copy_size;
    uint32_t extension_count = ARRAY_SIZE(intel_phy_dev_gpu_exts);

    if (pProperties == NULL) {
        *pPropertyCount = INTEL_PHY_DEV_EXT_COUNT;
        return VK_SUCCESS;
    }

    copy_size = *pPropertyCount < extension_count ? *pPropertyCount : extension_count;
    memcpy(pProperties, intel_phy_dev_gpu_exts, copy_size * sizeof(VkExtensionProperties));
    *pPropertyCount = copy_size;
    if (copy_size < extension_count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pPropertyCount,
        VkLayerProperties*                          pProperties)
{
    *pPropertyCount = 0;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties(
    VkPhysicalDevice                            physicalDevice,
    VkFormat                                    format,
    VkImageType                                 type,
    VkSampleCountFlagBits                       samples,
    VkImageUsageFlags                           usage,
    VkImageTiling                               tiling,
    uint32_t*                                   pPropertyCount,
    VkSparseImageFormatProperties*              pProperties)
{
    *pPropertyCount = 0;
}

