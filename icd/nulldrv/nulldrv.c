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
 * Author: Cody Northrop <cody@lunarg.com>
 * Author: David Pinedo <david@lunarg.com>
 * Author: Ian Elliott <ian@LunarG.com>
 * Author: Tony Barbour <tony@LunarG.com>
 *
 */

#include "nulldrv.h"
#include <stdio.h>

#if 0
#define NULLDRV_LOG_FUNC \
    do { \
        fflush(stdout); \
        fflush(stderr); \
        printf("null driver: %s\n", __FUNCTION__); \
        fflush(stdout); \
    } while (0)
#else
    #define NULLDRV_LOG_FUNC do { } while (0)
#endif

// The null driver supports all WSI extenstions ... for now ...
static const char * const nulldrv_gpu_exts[NULLDRV_EXT_COUNT] = {
	[NULLDRV_EXT_KHR_SWAPCHAIN] = VK_KHR_SURFACE_EXTENSION_NAME,
};
static const VkExtensionProperties intel_gpu_exts[NULLDRV_EXT_COUNT] = {
    {
        .extensionName = VK_KHR_SURFACE_EXTENSION_NAME,
        .specVersion = VK_KHR_SURFACE_REVISION,
    }
};

static struct nulldrv_base *nulldrv_base(void* base)
{
    return (struct nulldrv_base *) base;
}

static struct nulldrv_base *nulldrv_base_create(
        struct nulldrv_dev *dev,
        size_t obj_size,
        VkDbgObjectType type)
{
    struct nulldrv_base *base;

    if (!obj_size)
        obj_size = sizeof(*base);

    assert(obj_size >= sizeof(*base));

    base = (struct nulldrv_base*)malloc(obj_size);
    if (!base)
        return NULL;

    memset(base, 0, obj_size);

    // Initialize pointer to loader's dispatch table with ICD_LOADER_MAGIC
    set_loader_magic_value(base);

    if (dev == NULL) {
        /*
         * dev is NULL when we are creating the base device object
         * Set dev now so that debug setup happens correctly
         */
        dev = (struct nulldrv_dev *) base;
    }


    base->get_memory_requirements = NULL;

    return base;
}

static VkResult nulldrv_gpu_add(int devid, const char *primary_node,
                         const char *render_node, struct nulldrv_gpu **gpu_ret)
{
    struct nulldrv_gpu *gpu;

	gpu = malloc(sizeof(*gpu));
    if (!gpu)
        return VK_ERROR_OUT_OF_HOST_MEMORY;
	memset(gpu, 0, sizeof(*gpu));

    // Initialize pointer to loader's dispatch table with ICD_LOADER_MAGIC
    set_loader_magic_value(gpu);

    *gpu_ret = gpu;

    return VK_SUCCESS;
}

static VkResult nulldrv_queue_create(struct nulldrv_dev *dev,
                              uint32_t node_index,
                              struct nulldrv_queue **queue_ret)
{
    struct nulldrv_queue *queue;

    queue = (struct nulldrv_queue *) nulldrv_base_create(dev, sizeof(*queue),
            VK_OBJECT_TYPE_QUEUE);
    if (!queue)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    queue->dev = dev;

    *queue_ret = queue;

    return VK_SUCCESS;
}

static VkResult dev_create_queues(struct nulldrv_dev *dev,
                                  const VkDeviceQueueCreateInfo *queues,
                                  uint32_t count)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        const VkDeviceQueueCreateInfo *q = &queues[i];
        VkResult ret = VK_SUCCESS;

        if (q->queueCount == 1 && !dev->queues[q->queueFamilyIndex]) {
            ret = nulldrv_queue_create(dev, q->queueFamilyIndex,
                    &dev->queues[q->queueFamilyIndex]);
        }

        if (ret != VK_SUCCESS) {
            return ret;
        }
    }

    return VK_SUCCESS;
}

static enum nulldrv_ext_type nulldrv_gpu_lookup_extension(
        const struct nulldrv_gpu *gpu,
        const char* extensionName)
{
    enum nulldrv_ext_type type;

    for (type = 0; type < ARRAY_SIZE(nulldrv_gpu_exts); type++) {
        if (strcmp(nulldrv_gpu_exts[type], extensionName) == 0)
            break;
    }

    assert(type < NULLDRV_EXT_COUNT || type == NULLDRV_EXT_INVALID);

    return type;
}

static VkResult nulldrv_desc_ooxx_create(struct nulldrv_dev *dev,
                                  struct nulldrv_desc_ooxx **ooxx_ret)
{
    struct nulldrv_desc_ooxx *ooxx;

    ooxx = malloc(sizeof(*ooxx));
    if (!ooxx) 
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    memset(ooxx, 0, sizeof(*ooxx));

    ooxx->surface_desc_size = 0;
    ooxx->sampler_desc_size = 0;

    *ooxx_ret = ooxx; 

    return VK_SUCCESS;
}

static VkResult nulldrv_dev_create(struct nulldrv_gpu *gpu,
                            const VkDeviceCreateInfo *info,
                            struct nulldrv_dev **dev_ret)
{
    struct nulldrv_dev *dev;
    uint32_t i;
    VkResult ret;

    dev = (struct nulldrv_dev *) nulldrv_base_create(NULL, sizeof(*dev),
            VK_OBJECT_TYPE_DEVICE);
    if (!dev)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    for (i = 0; i < info->enabledExtensionNameCount; i++) {
        const enum nulldrv_ext_type ext = nulldrv_gpu_lookup_extension(
                    gpu,
                    info->ppEnabledExtensionNames[i]);

        if (ext == NULLDRV_EXT_INVALID)
            return VK_ERROR_EXTENSION_NOT_PRESENT;

        dev->exts[ext] = true;
    }

    ret = nulldrv_desc_ooxx_create(dev, &dev->desc_ooxx);
    if (ret != VK_SUCCESS) {
        return ret;
    }

    ret = dev_create_queues(dev, info->pQueueCreateInfos,
            info->queueCreateInfoCount);
    if (ret != VK_SUCCESS) {
        return ret;
    }

    *dev_ret = dev;

    return VK_SUCCESS;
}

static struct nulldrv_gpu *nulldrv_gpu(VkPhysicalDevice gpu)
{
    return (struct nulldrv_gpu *) gpu;
}

static VkResult nulldrv_fence_create(struct nulldrv_dev *dev,
                              const VkFenceCreateInfo *info,
                              struct nulldrv_fence **fence_ret)
{
    struct nulldrv_fence *fence;

    fence = (struct nulldrv_fence *) nulldrv_base_create(dev, sizeof(*fence),
            VK_OBJECT_TYPE_FENCE);
    if (!fence)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *fence_ret = fence;

    return VK_SUCCESS;
}

static struct nulldrv_dev *nulldrv_dev(VkDevice dev)
{
    return (struct nulldrv_dev *) dev;
}

static struct nulldrv_img *nulldrv_img_from_base(struct nulldrv_base *base)
{
    return (struct nulldrv_img *) base;
}


static VkResult img_get_memory_requirements(struct nulldrv_base *base,
                               VkMemoryRequirements *pRequirements)
{
    struct nulldrv_img *img = nulldrv_img_from_base(base);
    VkResult ret = VK_SUCCESS;

    pRequirements->size = img->total_size;
    pRequirements->alignment = 4096;

    return ret;
}

static VkResult nulldrv_img_create(struct nulldrv_dev *dev,
                            const VkImageCreateInfo *info,
                            bool scanout,
                            struct nulldrv_img **img_ret)
{
    struct nulldrv_img *img;

    img = (struct nulldrv_img *) nulldrv_base_create(dev, sizeof(*img),
            VK_OBJECT_TYPE_IMAGE);
    if (!img)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    img->type = info->imageType;
    img->depth = info->extent.depth;
    img->mip_levels = info->mipLevels;
    img->array_size = info->arrayLayers;
    img->usage = info->usage;
    img->samples = info->samples;

    img->obj.base.get_memory_requirements = img_get_memory_requirements;

    *img_ret = img;

    return VK_SUCCESS;
}

static struct nulldrv_img *nulldrv_img(VkImage image)
{
    return *(struct nulldrv_img **) &image;
}

static VkResult nulldrv_mem_alloc(struct nulldrv_dev *dev,
                           const VkMemoryAllocateInfo *info,
                           struct nulldrv_mem **mem_ret)
{
    struct nulldrv_mem *mem;

    mem = (struct nulldrv_mem *) nulldrv_base_create(dev, sizeof(*mem),
            VK_OBJECT_TYPE_DEVICE_MEMORY);
    if (!mem)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    mem->bo = malloc(info->allocationSize);
    if (!mem->bo) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    mem->size = info->allocationSize;

    *mem_ret = mem;

    return VK_SUCCESS;
}

static VkResult nulldrv_sampler_create(struct nulldrv_dev *dev,
                                const VkSamplerCreateInfo *info,
                                struct nulldrv_sampler **sampler_ret)
{
    struct nulldrv_sampler *sampler;

    sampler = (struct nulldrv_sampler *) nulldrv_base_create(dev,
            sizeof(*sampler), VK_OBJECT_TYPE_SAMPLER);
    if (!sampler)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *sampler_ret = sampler;

    return VK_SUCCESS;
}

static VkResult nulldrv_img_view_create(struct nulldrv_dev *dev,
                                 const VkImageViewCreateInfo *info,
                                 struct nulldrv_img_view **view_ret)
{
    struct nulldrv_img *img = nulldrv_img(info->image);
    struct nulldrv_img_view *view;

    view = (struct nulldrv_img_view *) nulldrv_base_create(dev, sizeof(*view),
            VK_OBJECT_TYPE_IMAGE_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    view->img = img;

    view->cmd_len = 8;

    *view_ret = view;

    return VK_SUCCESS;
}

static void *nulldrv_mem_map(struct nulldrv_mem *mem, VkFlags flags)
{
    return mem->bo;
}

static struct nulldrv_mem *nulldrv_mem(VkDeviceMemory mem)
{
    return *(struct nulldrv_mem **) &mem;
}

static struct nulldrv_buf *nulldrv_buf_from_base(struct nulldrv_base *base)
{
    return (struct nulldrv_buf *) base;
}

static VkResult buf_get_memory_requirements(struct nulldrv_base *base,
                                VkMemoryRequirements* pMemoryRequirements)
{
    struct nulldrv_buf *buf = nulldrv_buf_from_base(base);

    if (pMemoryRequirements == NULL)
        return VK_SUCCESS;

    pMemoryRequirements->size = buf->size;
    pMemoryRequirements->alignment = 4096;

    return VK_SUCCESS;
}

static VkResult nulldrv_buf_create(struct nulldrv_dev *dev,
                            const VkBufferCreateInfo *info,
                            struct nulldrv_buf **buf_ret)
{
    struct nulldrv_buf *buf;

    buf = (struct nulldrv_buf *) nulldrv_base_create(dev, sizeof(*buf),
            VK_OBJECT_TYPE_BUFFER);
    if (!buf)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    buf->size = info->size;
    buf->usage = info->usage;

    buf->obj.base.get_memory_requirements = buf_get_memory_requirements;

    *buf_ret = buf;

    return VK_SUCCESS;
}

static VkResult nulldrv_desc_layout_create(struct nulldrv_dev *dev,
                                    const VkDescriptorSetLayoutCreateInfo *info,
                                    struct nulldrv_desc_layout **layout_ret)
{
    struct nulldrv_desc_layout *layout;

    layout = (struct nulldrv_desc_layout *)
        nulldrv_base_create(dev, sizeof(*layout),
                VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT);
    if (!layout)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *layout_ret = layout;

    return VK_SUCCESS;
}

static VkResult nulldrv_pipeline_layout_create(struct nulldrv_dev *dev,
                                    const VkPipelineLayoutCreateInfo* pCreateInfo,
                                    struct nulldrv_pipeline_layout **pipeline_layout_ret)
{
    struct nulldrv_pipeline_layout *pipeline_layout;

    pipeline_layout = (struct nulldrv_pipeline_layout *)
        nulldrv_base_create(dev, sizeof(*pipeline_layout),
                VK_OBJECT_TYPE_PIPELINE_LAYOUT);
    if (!pipeline_layout)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *pipeline_layout_ret = pipeline_layout;

    return VK_SUCCESS;
}

static struct nulldrv_desc_layout *nulldrv_desc_layout(const VkDescriptorSetLayout layout)
{
    return *(struct nulldrv_desc_layout **) &layout;
}

static VkResult graphics_pipeline_create(struct nulldrv_dev *dev,
                                           const VkGraphicsPipelineCreateInfo *info_,
                                           struct nulldrv_pipeline **pipeline_ret)
{
    struct nulldrv_pipeline *pipeline;

    pipeline = (struct nulldrv_pipeline *)
        nulldrv_base_create(dev, sizeof(*pipeline), 
                VK_OBJECT_TYPE_PIPELINE);
    if (!pipeline)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *pipeline_ret = pipeline;

    return VK_SUCCESS;
}

static VkResult nulldrv_cmd_create(struct nulldrv_dev *dev,
                            const VkCommandBufferAllocateInfo *info,
                            struct nulldrv_cmd **cmd_ret)
{
    struct nulldrv_cmd *cmd;
    uint32_t num_allocated = 0;

    for (uint32_t i = 0; i < info->bufferCount; i++) {
        cmd = (struct nulldrv_cmd *) nulldrv_base_create(dev, sizeof(*cmd),
                VK_OBJECT_TYPE_COMMAND_BUFFER);
        if (!cmd) {
            for (uint32_t j = 0; j < num_allocated; j++) {
                free(cmd_ret[j]);
            }
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        num_allocated++;
        cmd_ret[i] = cmd;
    }

    return VK_SUCCESS;
}

static VkResult nulldrv_desc_pool_create(struct nulldrv_dev *dev,
                                    const VkDescriptorPoolCreateInfo *info,
                                    struct nulldrv_desc_pool **pool_ret)
{
    struct nulldrv_desc_pool *pool;

    pool = (struct nulldrv_desc_pool *)
        nulldrv_base_create(dev, sizeof(*pool),
                VK_OBJECT_TYPE_DESCRIPTOR_POOL);
    if (!pool)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    pool->dev = dev;

    *pool_ret = pool;

    return VK_SUCCESS;
}

static VkResult nulldrv_desc_set_create(struct nulldrv_dev *dev,
                                 struct nulldrv_desc_pool *pool,
                                 const struct nulldrv_desc_layout *layout,
                                 struct nulldrv_desc_set **set_ret)
{
    struct nulldrv_desc_set *set;

    set = (struct nulldrv_desc_set *)
        nulldrv_base_create(dev, sizeof(*set), 
                VK_OBJECT_TYPE_DESCRIPTOR_SET);
    if (!set)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    set->ooxx = dev->desc_ooxx;
    set->layout = layout;
    *set_ret = set;

    return VK_SUCCESS;
}

static struct nulldrv_desc_pool *nulldrv_desc_pool(VkDescriptorPool pool)
{
    return *(struct nulldrv_desc_pool **) &pool;
}

static VkResult nulldrv_fb_create(struct nulldrv_dev *dev,
                           const VkFramebufferCreateInfo* info,
                           struct nulldrv_framebuffer ** fb_ret)
{

    struct nulldrv_framebuffer *fb;
    fb = (struct nulldrv_framebuffer *) nulldrv_base_create(dev, sizeof(*fb),
            VK_OBJECT_TYPE_FRAMEBUFFER);
    if (!fb)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *fb_ret = fb;

    return VK_SUCCESS;

}

static VkResult nulldrv_render_pass_create(struct nulldrv_dev *dev,
                           const VkRenderPassCreateInfo* info,
                           struct nulldrv_render_pass** rp_ret)
{
    struct nulldrv_render_pass *rp;
    rp = (struct nulldrv_render_pass *) nulldrv_base_create(dev, sizeof(*rp),
            VK_OBJECT_TYPE_RENDER_PASS);
    if (!rp)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *rp_ret = rp;

    return VK_SUCCESS;
}

static struct nulldrv_buf *nulldrv_buf(VkBuffer buf)
{
    return *(struct nulldrv_buf **) &buf;
}

static VkResult nulldrv_buf_view_create(struct nulldrv_dev *dev,
                                 const VkBufferViewCreateInfo *info,
                                 struct nulldrv_buf_view **view_ret)
{
    struct nulldrv_buf *buf = nulldrv_buf(info->buffer);
    struct nulldrv_buf_view *view;

    view = (struct nulldrv_buf_view *) nulldrv_base_create(dev, sizeof(*view),
            VK_OBJECT_TYPE_BUFFER_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    view->buf = buf;

    *view_ret = view;

    return VK_SUCCESS;
}


//*********************************************
// Driver entry points
//*********************************************

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer(
    VkDevice                                  device,
    const VkBufferCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkBuffer*                                 pBuffer)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_buf_create(dev, pCreateInfo, (struct nulldrv_buf **) pBuffer);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer(
    VkDevice                                  device,
    VkBuffer                                  buffer,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool(
    VkDevice                                    device,
    const VkCommandPoolCreateInfo*                  pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkCommandPool*                                  pCommandPool)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool(
    VkDevice                                    device,
    VkCommandPool                                   commandPool,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool(
    VkDevice                                    device,
    VkCommandPool                                   commandPool,
    VkCommandPoolResetFlags                         flags)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers(
    VkDevice                                  device,
    const VkCommandBufferAllocateInfo*               pAllocateInfo,
    VkCommandBuffer*                              pCommandBuffers)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_cmd_create(dev, pAllocateInfo,
            (struct nulldrv_cmd **) pCommandBuffers);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers(
    VkDevice                                  device,
    VkCommandPool                                 commandPool,
    uint32_t                                  commandBufferCount,
    const VkCommandBuffer*                        pCommandBuffers)
{
    NULLDRV_LOG_FUNC;
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        free(pCommandBuffers[i]);
    }
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer(
    VkCommandBuffer                              commandBuffer,
    const VkCommandBufferBeginInfo            *info)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer(
    VkCommandBuffer                              commandBuffer)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer(
    VkCommandBuffer                              commandBuffer,
    VkCommandBufferResetFlags flags)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

static const VkFormat nulldrv_presentable_formats[] = {
    VK_FORMAT_B8G8R8A8_UNORM,
};

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice                        physicalDevice,
    uint32_t                                queueFamilyIndex,
    VkSurfaceKHR                            surface,
    VkBool32*                               pSupported)
{
    NULLDRV_LOG_FUNC;

    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    VkPhysicalDevice                         physicalDevice,
    VkSurfaceKHR                             surface,
    VkSurfaceCapabilitiesKHR*                pSurfaceCapabilities)
{
    NULLDRV_LOG_FUNC;

    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR(
    VkPhysicalDevice                         physicalDevice,
    VkSurfaceKHR                             surface,
    uint32_t*                                pSurfaceFormatCount,
    VkSurfaceFormatKHR*                      pSurfaceFormats)
{
    NULLDRV_LOG_FUNC;

    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR(
    VkPhysicalDevice                         physicalDevice,
    VkSurfaceKHR                             surface,
    uint32_t*                                pPresentModeCount,
    VkPresentModeKHR*                        pPresentModes)
{
    NULLDRV_LOG_FUNC;

    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR(
    VkDevice                                device,
    const VkSwapchainCreateInfoKHR*         pCreateInfo,
    const VkAllocationCallbacks*            pAllocator,
    VkSwapchainKHR*                         pSwapchain)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);
    struct nulldrv_swap_chain *sc;

    sc = (struct nulldrv_swap_chain *) nulldrv_base_create(dev, sizeof(*sc),
            VK_OBJECT_TYPE_SWAPCHAIN_KHR);
    if (!sc) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    sc->dev = dev;

    *(VkSwapchainKHR **)pSwapchain = *(VkSwapchainKHR **)&sc;

    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR(
    VkDevice                                device,
    VkSwapchainKHR                          swapchain,
    const VkAllocationCallbacks*            pAllocator)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_swap_chain *sc = *(struct nulldrv_swap_chain **) &swapchain;

    free(sc);
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR(
    VkDevice                                 device,
    VkSwapchainKHR                           swapchain,
    uint32_t*                                pSwapchainImageCount,
    VkImage*                                 pSwapchainImages)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_swap_chain *sc = *(struct nulldrv_swap_chain **) &swapchain;
    struct nulldrv_dev *dev = sc->dev;
    VkResult ret = VK_SUCCESS;

    *pSwapchainImageCount = 2;
    if (pSwapchainImages) {
        uint32_t i;
        for (i = 0; i < 2; i++) {
                struct nulldrv_img *img;

                img = (struct nulldrv_img *) nulldrv_base_create(dev,
                        sizeof(*img),
                        VK_OBJECT_TYPE_IMAGE);
                if (!img)
                    return VK_ERROR_OUT_OF_HOST_MEMORY;
            pSwapchainImages[i] = (VkImage) img;
        }
    }

    return ret;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR(
    VkDevice                                 device,
    VkSwapchainKHR                           swapchain,
    uint64_t                                 timeout,
    VkSemaphore                              semaphore,
    VkFence                                  fence,
    uint32_t*                                pImageIndex)
{
    NULLDRV_LOG_FUNC;

    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR(
    VkQueue                                  queue_,
    const VkPresentInfoKHR*                  pPresentInfo)
{
    NULLDRV_LOG_FUNC;

    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer(
    VkCommandBuffer                              commandBuffer,
    VkBuffer                                  srcBuffer,
    VkBuffer                                  dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferCopy*                      pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage(
    VkCommandBuffer                              commandBuffer,
    VkImage                                   srcImage,
    VkImageLayout                            srcImageLayout,
    VkImage                                   dstImage,
    VkImageLayout                            dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageCopy*                       pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage(
    VkCommandBuffer                              commandBuffer,
    VkImage                                  srcImage,
    VkImageLayout                            srcImageLayout,
    VkImage                                  dstImage,
    VkImageLayout                            dstImageLayout,
    uint32_t                                 regionCount,
    const VkImageBlit*                       pRegions,
    VkFilter                                 filter)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage(
    VkCommandBuffer                              commandBuffer,
    VkBuffer                                  srcBuffer,
    VkImage                                   dstImage,
    VkImageLayout                            dstImageLayout,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer(
    VkCommandBuffer                              commandBuffer,
    VkImage                                   srcImage,
    VkImageLayout                            srcImageLayout,
    VkBuffer                                  dstBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer(
    VkCommandBuffer                              commandBuffer,
    VkBuffer                                  dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                dataSize,
    const uint32_t*                             pData)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer(
    VkCommandBuffer                              commandBuffer,
    VkBuffer                                  dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                size,
    uint32_t                                    data)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage(
    VkCommandBuffer                                 commandBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    const VkClearDepthStencilValue*             pDepthStencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    attachmentCount,
    const VkClearAttachment*                    pAttachments,
    uint32_t                                    rectCount,
    const VkClearRect*                          pRects)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage(
    VkCommandBuffer                         commandBuffer,
    VkImage                             image,
    VkImageLayout                       imageLayout,
    const VkClearColorValue            *pColor,
    uint32_t                            rangeCount,
    const VkImageSubresourceRange*      pRanges)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencil(
    VkCommandBuffer                              commandBuffer,
    VkImage                                   image,
    VkImageLayout                            imageLayout,
    float                                       depth,
    uint32_t                                    stencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*          pRanges)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage(
    VkCommandBuffer                              commandBuffer,
    VkImage                                   srcImage,
    VkImageLayout                            srcImageLayout,
    VkImage                                   dstImage,
    VkImageLayout                            dstImageLayout,
    uint32_t                                    regionCount,
    const VkImageResolve*                    pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(
    VkCommandBuffer                              commandBuffer,
    VkQueryPool                              queryPool,
    uint32_t                                    slot,
    VkFlags                                   flags)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(
    VkCommandBuffer                              commandBuffer,
    VkQueryPool                              queryPool,
    uint32_t                                    slot)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(
    VkCommandBuffer                              commandBuffer,
    VkQueryPool                              queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(
    VkCommandBuffer                              commandBuffer,
    VkEvent                                  event_,
    VkPipelineStageFlags                     stageMask)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(
    VkCommandBuffer                              commandBuffer,
    VkEvent                                  event_,
    VkPipelineStageFlags                     stageMask)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(
    VkCommandBuffer                                 commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                destStride,
    VkFlags                                     flags)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(
    VkCommandBuffer                              commandBuffer,
    VkPipelineStageFlagBits                     pipelineStage,
    VkQueryPool                                 queryPool,
    uint32_t                                    slot)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline(
    VkCommandBuffer                              commandBuffer,
    VkPipelineBindPoint                      pipelineBindPoint,
    VkPipeline                               pipeline)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t viewportCount, const VkViewport* pViewports)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t scissorCount, const VkRect2D* pScissors)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets(
    VkCommandBuffer                              commandBuffer,
    VkPipelineBindPoint                     pipelineBindPoint,
    VkPipelineLayout                        layout,
    uint32_t                                firstSet,
    uint32_t                                descriptorSetCount,
    const VkDescriptorSet*                  pDescriptorSets,
    uint32_t                                dynamicOffsetCount,
    const uint32_t*                         pDynamicOffsets)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                            pOffsets)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer(
    VkCommandBuffer                              commandBuffer,
    VkBuffer                                  buffer,
    VkDeviceSize                                offset,
    VkIndexType                              indexType)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDraw(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    vertexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstVertex,
    uint32_t                                    firstInstance)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed(
    VkCommandBuffer                              commandBuffer,
    uint32_t                                    indexCount,
    uint32_t                                    instanceCount,
    uint32_t                                    firstIndex,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect(
    VkCommandBuffer                              commandBuffer,
    VkBuffer                                  buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect(
    VkCommandBuffer                              commandBuffer,
    VkBuffer                                  buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatch(
    VkCommandBuffer                              commandBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect(
    VkCommandBuffer                              commandBuffer,
    VkBuffer                                  buffer,
    VkDeviceSize                                offset)
{
    NULLDRV_LOG_FUNC;
}

VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    VkPipelineStageFlags                        sourceStageMask,
    VkPipelineStageFlags                        dstStageMask,
    uint32_t                                    memoryBarrierCount,
    const void* const*                          ppMemoryBarriers)
{
    NULLDRV_LOG_FUNC;
}

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(
    VkCommandBuffer                                 commandBuffer,
    VkPipelineStageFlags                        srcStageMask,
    VkPipelineStageFlags                        dstStageMask,
    VkDependencyFlags                           dependencyFlags,
    uint32_t                                    memoryBarrierCount,
    const void* const*                          ppMemoryBarriers)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice(
    VkPhysicalDevice                            gpu_,
    const VkDeviceCreateInfo*               pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkDevice*                                 pDevice)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_gpu *gpu = nulldrv_gpu(gpu_);
    return nulldrv_dev_create(gpu, pCreateInfo, (struct nulldrv_dev**)pDevice);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDevice(
    VkDevice                                  device,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue(
    VkDevice                                  device,
    uint32_t                                    queueNodeIndex,
    uint32_t                                    queueIndex,
    VkQueue*                                  pQueue)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);
    *pQueue = (VkQueue) dev->queues[0];
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle(
    VkDevice                                  device)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateEvent(
    VkDevice                                  device,
    const VkEventCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkEvent*                                  pEvent)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyEvent(
    VkDevice                                  device,
    VkEvent                                   event,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus(
    VkDevice                                  device,
    VkEvent                                   event_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent(
    VkDevice                                  device,
    VkEvent                                   event_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent(
    VkDevice                                  device,
    VkEvent                                   event_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence(
    VkDevice                                  device,
    const VkFenceCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkFence*                                  pFence)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_fence_create(dev, pCreateInfo,
            (struct nulldrv_fence **) pFence);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFence(
    VkDevice                                  device,
    VkFence                                  fence,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus(
    VkDevice                                  device,
    VkFence                                   fence_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetFences(
    VkDevice                    device,
    uint32_t                    fenceCount,
    const VkFence*              pFences)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences(
    VkDevice                                  device,
    uint32_t                                    fenceCount,
    const VkFence*                            pFences,
    VkBool32                                    waitAll,
    uint64_t                                    timeout)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties(
    VkPhysicalDevice                             gpu_,
    VkPhysicalDeviceProperties*                  pProperties)
{
    NULLDRV_LOG_FUNC;

    pProperties->apiVersion = VK_API_VERSION;
    pProperties->driverVersion = 0; // Appropriate that the nulldrv have 0's
    pProperties->vendorID = 0;
    pProperties->deviceID = 0;
    pProperties->deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
    strncpy(pProperties->deviceName, "nulldrv", strlen("nulldrv"));

    /* TODO: fill out limits */
    memset(&pProperties->limits, 0, sizeof(VkPhysicalDeviceLimits));
    memset(&pProperties->sparseProperties, 0, sizeof(VkPhysicalDeviceSparseProperties));
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures(
    VkPhysicalDevice                          physicalDevice,
    VkPhysicalDeviceFeatures*                 pFeatures)
{
    NULLDRV_LOG_FUNC;

    /* TODO: fill out features */
    memset(pFeatures, 0, sizeof(*pFeatures));
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties(
    VkPhysicalDevice                          physicalDevice,
    VkFormat                                  format,
    VkFormatProperties*                       pFormatInfo)
{
    NULLDRV_LOG_FUNC;

    pFormatInfo->linearTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    pFormatInfo->optimalTilingFeatures = pFormatInfo->linearTilingFeatures;
    pFormatInfo->bufferFeatures = 0;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice                             gpu_,
    uint32_t*                                    pQueueFamilyPropertyCount,
    VkQueueFamilyProperties*                     pProperties)
 {
    if (pProperties == NULL) {
        *pQueueFamilyPropertyCount = 1;
        return;
    }
    pProperties->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_SPARSE_BINDING_BIT;
    pProperties->queueCount = 1;
    pProperties->timestampValidBits = 0;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice gpu_,
    VkPhysicalDeviceMemoryProperties* pProperties)
{
    // TODO: Fill in with real data
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pPropertyCount,
        VkLayerProperties*                          pProperties)
{
    // TODO: Fill in with real data
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(
    const char*                                 pLayerName,
    uint32_t*                                   pPropertyCount,
    VkExtensionProperties*                      pProperties)
{
    uint32_t copy_size;

    if (pProperties == NULL) {
        *pPropertyCount = NULLDRV_EXT_COUNT;
        return VK_SUCCESS;
    }

    copy_size = *pPropertyCount < NULLDRV_EXT_COUNT ? *pPropertyCount : NULLDRV_EXT_COUNT;
    memcpy(pProperties, intel_gpu_exts, copy_size * sizeof(VkExtensionProperties));
    *pPropertyCount = copy_size;
    if (copy_size < NULLDRV_EXT_COUNT) {
        return VK_INCOMPLETE;
    }
    return VK_SUCCESS;
}
ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(
        uint32_t*                                   pPropertyCount,
        VkLayerProperties*                          pProperties)
{
    // TODO: Fill in with real data
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice                            physicalDevice,
    const char*                                 pLayerName,
    uint32_t*                                   pPropertyCount,
    VkExtensionProperties*                      pProperties)
{

    *pPropertyCount = 0;

    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage(
    VkDevice                                  device,
    const VkImageCreateInfo*                pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkImage*                                  pImage)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_img_create(dev, pCreateInfo, false,
            (struct nulldrv_img **) pImage);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImage(
    VkDevice                                  device,
    VkImage                                   image,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout(
    VkDevice                                    device,
    VkImage                                     image,
    const VkImageSubresource*                   pSubresource,
    VkSubresourceLayout*                         pLayout)
{
    NULLDRV_LOG_FUNC;

    pLayout->offset = 0;
    pLayout->size = 1;
    pLayout->rowPitch = 4;
    pLayout->depthPitch = 4;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory(
    VkDevice                                  device,
    const VkMemoryAllocateInfo*                pAllocateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkDeviceMemory*                             pMemory)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_mem_alloc(dev, pAllocateInfo, (struct nulldrv_mem **) pMemory);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkFreeMemory(
    VkDevice                                    device,
    VkDeviceMemory                              mem_,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory(
    VkDevice                                    device,
    VkDeviceMemory                              mem_,
    VkDeviceSize                                offset,
    VkDeviceSize                                size,
    VkFlags                                     flags,
    void**                                      ppData)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_mem *mem = nulldrv_mem(mem_);
    void *ptr = nulldrv_mem_map(mem, flags);

    *ppData = ptr;

    return (ptr) ? VK_SUCCESS : VK_ERROR_MEMORY_MAP_FAILED;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkUnmapMemory(
    VkDevice                                    device,
    VkDeviceMemory                              mem_)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFlushMappedMemoryRanges(
    VkDevice                                  device,
    uint32_t                                  memoryRangeCount,
    const VkMappedMemoryRange*                pMemoryRanges)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges(
    VkDevice                                  device,
    uint32_t                                  memoryRangeCount,
    const VkMappedMemoryRange*                pMemoryRanges)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetDeviceMemoryCommitment(
    VkDevice                                  device,
    VkDeviceMemory                            memory,
    VkDeviceSize*                             pCommittedMemoryInBytes)
{
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(
    const VkInstanceCreateInfo*             pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkInstance*                               pInstance)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_instance *inst;

    inst = (struct nulldrv_instance *) nulldrv_base_create(NULL, sizeof(*inst),
                VK_OBJECT_TYPE_INSTANCE);
    if (!inst)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    inst->obj.base.get_memory_requirements = NULL;

    *pInstance = (VkInstance) inst;

    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyInstance(
    VkInstance                                pInstance,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices(
    VkInstance                                instance,
    uint32_t*                                   pGpuCount,
    VkPhysicalDevice*                           pGpus)
{
    NULLDRV_LOG_FUNC;
    VkResult ret;
    struct nulldrv_gpu *gpu;
    *pGpuCount = 1;
    ret = nulldrv_gpu_add(0, 0, 0, &gpu);
    if (ret == VK_SUCCESS && pGpus)
        pGpus[0] = (VkPhysicalDevice) gpu;
    return ret;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateLayers(
    VkPhysicalDevice                            gpu,
    size_t                                      maxStringSize,
    size_t*                                     pLayerCount,
    char* const*                                pOutLayers,
    void*                                       pReserved)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_base *base = nulldrv_base((void*)buffer);

    base->get_memory_requirements(base, pMemoryRequirements);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements(
    VkDevice                                    device,
    VkImage                                     image,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_base *base = nulldrv_base((void*)image);

    base->get_memory_requirements(base, pMemoryRequirements);
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkDeviceMemory                              mem_,
    VkDeviceSize                                memoryOffset)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory(
    VkDevice                                    device,
    VkImage                                     image,
    VkDeviceMemory                              mem_,
    VkDeviceSize                                memoryOffset)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements(
    VkDevice                                    device,
    VkImage                                     image,
    uint32_t*                                   pSparseMemoryRequirementCount,
    VkSparseImageMemoryRequirements*            pSparseMemoryRequirements)
{
    NULLDRV_LOG_FUNC;
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
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse(
    VkQueue                                     queue,
    uint32_t                                    bindInfoCount,
    const VkBindSparseInfo*                     pBindInfo,
    VkFence                                     fence)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache(
    VkDevice                                    device,
    const VkPipelineCacheCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipelineCache*                            pPipelineCache)
{

    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline(
    VkDevice                                  device,
    VkPipeline                                pipeline,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData(
    VkDevice                                    device,
    VkPipelineCache                             pipelineCache,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return VK_ERROR_INITIALIZATION_FAILED;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches(
    VkDevice                                    device,
    VkPipelineCache                             dstCache,
    uint32_t                                    srcCacheCount,
    const VkPipelineCache*                      pSrcCaches)
{
    NULLDRV_LOG_FUNC;
    return VK_ERROR_INITIALIZATION_FAILED;
}
ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines(
    VkDevice                                  device,
    VkPipelineCache                           pipelineCache,
    uint32_t                                  createInfoCount,
    const VkGraphicsPipelineCreateInfo*    pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipeline*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return graphics_pipeline_create(dev, pCreateInfo,
            (struct nulldrv_pipeline **) pPipeline);
}



ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines(
    VkDevice                                  device,
    VkPipelineCache                           pipelineCache,
    uint32_t                                  createInfoCount,
    const VkComputePipelineCreateInfo*     pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipeline*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}





ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool(
    VkDevice                                  device,
    const VkQueryPoolCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkQueryPool*                             pQueryPool)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool(
    VkDevice                                  device,
    VkQueryPool                               queryPoool,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults(
    VkDevice                                    device,
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    size_t                                      dataSize,
    void*                                       pData,
    size_t                                      stride,
    VkQueryResultFlags                          flags)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle(
    VkQueue                                   queue_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit(
    VkQueue                                   queue_,
    uint32_t                                  submitCount,
    const VkSubmitInfo*                       pSubmits,
    VkFence                                   fence_)
{
    NULLDRV_LOG_FUNC;
   return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore(
    VkDevice                                  device,
    const VkSemaphoreCreateInfo*            pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkSemaphore*                              pSemaphore)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore(
    VkDevice                                  device,
    VkSemaphore                               semaphore,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler(
    VkDevice                                  device,
    const VkSamplerCreateInfo*              pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkSampler*                                pSampler)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_sampler_create(dev, pCreateInfo,
            (struct nulldrv_sampler **) pSampler);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroySampler(
    VkDevice                                  device,
    VkSampler                                 sampler,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule(
    VkDevice                                    device,
    const VkShaderModuleCreateInfo*             pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkShaderModule*                             pShaderModule)
{
    // TODO: Fill in with real data
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule(
    VkDevice                                    device,
    VkShaderModule                              shaderModule,
    const VkAllocationCallbacks*                     pAllocator)
{
    // TODO: Fill in with real data
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView(
    VkDevice                                  device,
    const VkBufferViewCreateInfo*          pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkBufferView*                            pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_buf_view_create(dev, pCreateInfo,
            (struct nulldrv_buf_view **) pView);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyBufferView(
    VkDevice                                  device,
    VkBufferView                              bufferView,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView(
    VkDevice                                  device,
    const VkImageViewCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkImageView*                             pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_img_view_create(dev, pCreateInfo,
            (struct nulldrv_img_view **) pView);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyImageView(
    VkDevice                                  device,
    VkImageView                               imageView,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout(
    VkDevice                                   device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkDescriptorSetLayout*                   pSetLayout)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_layout_create(dev, pCreateInfo,
            (struct nulldrv_desc_layout **) pSetLayout);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout(
    VkDevice                                  device,
    VkDescriptorSetLayout                     descriptorSetLayout,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL  vkCreatePipelineLayout(
    VkDevice                                device,
    const VkPipelineLayoutCreateInfo*       pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkPipelineLayout*                       pPipelineLayout)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_pipeline_layout_create(dev,
            pCreateInfo,
            (struct nulldrv_pipeline_layout **) pPipelineLayout);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout(
    VkDevice                                  device,
    VkPipelineLayout                          pipelineLayout,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool(
    VkDevice                                    device,
    const VkDescriptorPoolCreateInfo*           pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkDescriptorPool*                           pDescriptorPool)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_pool_create(dev, pCreateInfo,
            (struct nulldrv_desc_pool **) pDescriptorPool);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool(
    VkDevice                                  device,
    VkDescriptorPool                          descriptorPool,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool(
    VkDevice                                device,
    VkDescriptorPool                        descriptorPool,
    VkDescriptorPoolResetFlags              flags)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets(
    VkDevice                                device,
    const VkDescriptorSetAllocateInfo*         pAllocateInfo,
    VkDescriptorSet*                        pDescriptorSets)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_desc_pool *pool = nulldrv_desc_pool(pAllocateInfo->descriptorPool);
    struct nulldrv_dev *dev = pool->dev;
    VkResult ret = VK_SUCCESS;
    uint32_t i;

    for (i = 0; i < pAllocateInfo->setLayoutCount; i++) {
        const struct nulldrv_desc_layout *layout =
            nulldrv_desc_layout(pAllocateInfo->pSetLayouts[i]);

        ret = nulldrv_desc_set_create(dev, pool, layout,
                (struct nulldrv_desc_set **) &pDescriptorSets[i]);
        if (ret != VK_SUCCESS)
            break;
    }

    return ret;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets(
    VkDevice                                    device,
    VkDescriptorPool                            descriptorPool,
    uint32_t                                    descriptorSetCount,
    const VkDescriptorSet*                      pDescriptorSets)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets(
    VkDevice                                    device,
    uint32_t                                    descriptorWriteCount,
    const VkWriteDescriptorSet*                 pDescriptorWrites,
    uint32_t                                    descriptorCopyCount,
    const VkCopyDescriptorSet*                  pDescriptorCopies)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer(
    VkDevice                                  device,
    const VkFramebufferCreateInfo*          info,
    const VkAllocationCallbacks*                     pAllocator,
    VkFramebuffer*                            fb_ret)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_fb_create(dev, info, (struct nulldrv_framebuffer **) fb_ret);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer(
    VkDevice                                  device,
    VkFramebuffer                             framebuffer,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass(
    VkDevice                                  device,
    const VkRenderPassCreateInfo*          info,
    const VkAllocationCallbacks*                     pAllocator,
    VkRenderPass*                            rp_ret)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_render_pass_create(dev, info, (struct nulldrv_render_pass **) rp_ret);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass(
    VkDevice                                  device,
    VkRenderPass                              renderPass,
    const VkAllocationCallbacks*                     pAllocator)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants(
    VkCommandBuffer                                 commandBuffer,
    VkPipelineLayout                            layout,
    VkShaderStageFlags                          stageFlags,
    uint32_t                                    offset,
    uint32_t                                    size,
    const void*                                 pValues)
{
    /* TODO: Implement */
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkGetRenderAreaGranularity(
    VkDevice                                    device,
    VkRenderPass                                renderPass,
    VkExtent2D*                                 pGranularity)
{
    pGranularity->height = 1;
    pGranularity->width = 1;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass(
    VkCommandBuffer                                 commandBuffer,
    const VkRenderPassBeginInfo*                pRenderPassBegin,
    VkSubpassContents                        contents)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass(
    VkCommandBuffer                                 commandBuffer,
    VkSubpassContents                        contents)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass(
    VkCommandBuffer                              commandBuffer)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    commandBuffersCount,
    const VkCommandBuffer*                          pCommandBuffers)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void* xcbCreateWindow(
    uint16_t         width,
    uint16_t         height)
{
    static uint32_t  window;  // Kludge to the max
    NULLDRV_LOG_FUNC;
    return &window;
}

// May not be needed, if we stub out stuf in tri.c
ICD_EXPORT void xcbDestroyWindow()
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT int xcbGetMessage(void *msg)
{
    NULLDRV_LOG_FUNC;
    return 0;
}

ICD_EXPORT VkResult xcbQueuePresent(void *queue, void *image, void* fence)
{
    return VK_SUCCESS;
}

ICD_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties(
    VkPhysicalDevice                            physicalDevice,
    VkFormat                                    format,
    VkImageType                                 type,
    VkImageTiling                               tiling,
    VkImageUsageFlags                           usage,
    VkImageCreateFlags                          flags,
    VkImageFormatProperties*                    pImageFormatProperties)
{
    return VK_SUCCESS;
}
