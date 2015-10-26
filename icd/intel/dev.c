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

#include <stdarg.h>
#include "kmd/winsys.h"
#include "desc.h"
#include "gpu.h"
#include "pipeline.h"
#include "queue.h"
#include "dev.h"

static void dev_destroy_meta_shaders(struct intel_dev *dev)
{
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(dev->cmd_meta_shaders); i++) {
        if (!dev->cmd_meta_shaders[i])
            break;

        intel_pipeline_shader_destroy(dev, dev->cmd_meta_shaders[i]);
        dev->cmd_meta_shaders[i] = NULL;
    }
}

static bool dev_create_meta_shaders(struct intel_dev *dev)
{
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(dev->cmd_meta_shaders); i++) {
        struct intel_pipeline_shader *sh;

        sh = intel_pipeline_shader_create_meta(dev, i);
        if (!sh) {
            dev_destroy_meta_shaders(dev);
            return false;
        }

        dev->cmd_meta_shaders[i] = sh;
    }

    return true;
}

static VkResult dev_create_queues(struct intel_dev *dev,
                                  const VkDeviceQueueCreateInfo *queues,
                                  uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        const VkDeviceQueueCreateInfo *q = &queues[i];
        VkResult ret = VK_SUCCESS;

        assert((q->queueFamilyIndex < INTEL_GPU_ENGINE_COUNT &&
            q->queuePriorityCount == 1 && !dev->queues[q->queueFamilyIndex]) && "Invalid Queue request");
        /* Help catch places where we forgot to initialize pQueuePriorities */
        assert(q->pQueuePriorities);
        ret = intel_queue_create(dev, q->queueFamilyIndex,
                    &dev->queues[q->queueFamilyIndex]);

        if (ret != VK_SUCCESS) {
            uint32_t j;
            for (j = 0; j < i; j++)
                intel_queue_destroy(dev->queues[j]);

            return ret;
        }
    }

    return VK_SUCCESS;
}

VkResult intel_dev_create(struct intel_gpu *gpu,
                          const VkDeviceCreateInfo *info,
                          struct intel_dev **dev_ret)
{
    struct intel_dev *dev;
    uint32_t i;
    VkResult ret;

    // ICD limited to a single virtual device
    if (gpu->winsys)
        return VK_ERROR_INITIALIZATION_FAILED;

    dev = (struct intel_dev *) intel_base_create(&gpu->handle,
            sizeof(*dev), false,
            VK_OBJECT_TYPE_DEVICE, info, sizeof(struct intel_dev_dbg));
    if (!dev)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    for (i = 0; i < info->enabledExtensionNameCount; i++) {
        const enum intel_phy_dev_ext_type ext =
            intel_gpu_lookup_phy_dev_extension(gpu,
                    info->ppEnabledExtensionNames[i]);

        if (ext != INTEL_PHY_DEV_EXT_INVALID)
            dev->phy_dev_exts[ext] = true;
    }

    dev->gpu = gpu;

    ret = intel_gpu_init_winsys(gpu);
    if (ret != VK_SUCCESS) {
        intel_dev_destroy(dev);
        return ret;
    }

    dev->winsys = gpu->winsys;

    dev->cmd_scratch_bo = intel_winsys_alloc_bo(dev->winsys,
            "command buffer scratch", 4096, false);
    if (!dev->cmd_scratch_bo) {
        intel_dev_destroy(dev);
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    if (!dev_create_meta_shaders(dev)) {
        intel_dev_destroy(dev);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    ret = intel_desc_region_create(dev, &dev->desc_region);
    if (ret != VK_SUCCESS) {
        intel_dev_destroy(dev);
        return ret;
    }

    intel_pipeline_init_default_sample_patterns(dev,
            (uint8_t *) &dev->sample_pattern_1x,
            (uint8_t *) &dev->sample_pattern_2x,
            (uint8_t *) &dev->sample_pattern_4x,
            (uint8_t *) dev->sample_pattern_8x,
            (uint8_t *) dev->sample_pattern_16x);

    ret = dev_create_queues(dev, info->pRequestedQueues,
            info->requestedQueueCount);
    if (ret != VK_SUCCESS) {
        intel_dev_destroy(dev);
        return ret;
    }

    *dev_ret = dev;

    return VK_SUCCESS;
}

void intel_dev_destroy(struct intel_dev *dev)
{
    struct intel_gpu *gpu = dev->gpu;
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(dev->queues); i++) {
        if (dev->queues[i])
            intel_queue_destroy(dev->queues[i]);
    }

    if (dev->desc_region)
        intel_desc_region_destroy(dev, dev->desc_region);

    dev_destroy_meta_shaders(dev);

    intel_bo_unref(dev->cmd_scratch_bo);

    intel_base_destroy(&dev->base);

    if (gpu->winsys)
        intel_gpu_cleanup_winsys(gpu);
}

void intel_dev_log(struct intel_dev *dev,
                   VkFlags msg_flags,
                   struct intel_base *src_object,
                   size_t location,
                   int32_t msg_code,
                   const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    intel_logv(dev, msg_flags,
               (src_object->dbg ? src_object->dbg->type : 0),
               (uint64_t) src_object,
               location, msg_code,
               format, ap);
    va_end(ap);
}

ICD_EXPORT VkResult VKAPI vkCreateDevice(
    VkPhysicalDevice                    gpu_,
    const VkDeviceCreateInfo*           pCreateInfo,
    VkDevice*                           pDevice)
{
    struct intel_gpu *gpu = intel_gpu(gpu_);

    return intel_dev_create(gpu, pCreateInfo, (struct intel_dev **) pDevice);
}

ICD_EXPORT void VKAPI vkDestroyDevice(
    VkDevice                                  device)
{
    struct intel_dev *dev = intel_dev(device);

    intel_dev_destroy(dev);
}

ICD_EXPORT void VKAPI vkGetDeviceQueue(
    VkDevice                                  device,
    uint32_t                                  queueFamilyIndex,
    uint32_t                                  queueIndex,
    VkQueue*                                  pQueue)
{
    struct intel_dev *dev = intel_dev(device);

    *pQueue = (VkQueue) dev->queues[queueFamilyIndex];
}

ICD_EXPORT VkResult VKAPI vkDeviceWaitIdle(
    VkDevice                                  device)
{
    struct intel_dev *dev = intel_dev(device);
    VkResult ret = VK_SUCCESS;
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(dev->queues); i++) {
        if (dev->queues[i]) {
            const VkResult r = intel_queue_wait(dev->queues[i], -1);
            if (r != VK_SUCCESS)
                ret = r;
        }
    }

    return ret;
}
