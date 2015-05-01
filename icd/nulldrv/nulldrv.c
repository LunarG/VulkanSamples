/*
 * Vulkan null driver
 *
 * Copyright (C) 2015 LunarG, Inc.
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
	[NULLDRV_EXT_WSI_LUNARG] = "VK_WSI_LunarG",
};

static struct nulldrv_base *nulldrv_base(VkObject base)
{
    return (struct nulldrv_base *) base;
}

static VkResult nulldrv_base_get_info(struct nulldrv_base *base, int type,
                               size_t *size, void *data)
{
    VkResult ret = VK_SUCCESS;
    size_t s;
    uint32_t *count;

    switch (type) {
    case VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            s = sizeof(VkMemoryRequirements);
            *size = s;
            if (data == NULL)
                return ret;
            memset(data, 0, s);
            break;
        }
    case VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT:
        *size = sizeof(uint32_t);
        if (data == NULL)
            return ret;
        count = (uint32_t *) data;
        *count = 1;
        break;
    default:
        ret = VK_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}

static struct nulldrv_base *nulldrv_base_create(struct nulldrv_dev *dev,
                                     size_t obj_size,
                                     VK_DBG_OBJECT_TYPE type)
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
    set_loader_magic_value((VkObject) base);

    if (dev == NULL) {
        /*
         * dev is NULL when we are creating the base device object
         * Set dev now so that debug setup happens correctly
         */
        dev = (struct nulldrv_dev *) base;
    }


    base->get_info = NULL;

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
    set_loader_magic_value((VkObject) gpu);

    *gpu_ret = gpu;

    return VK_SUCCESS;
}

static VkResult nulldrv_queue_create(struct nulldrv_dev *dev,
                              uint32_t node_index,
                              struct nulldrv_queue **queue_ret)
{
    struct nulldrv_queue *queue;

    queue = (struct nulldrv_queue *) nulldrv_base_create(dev, sizeof(*queue),
            VK_DBG_OBJECT_QUEUE);
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

    if (!count)
        return VK_ERROR_INVALID_POINTER;

    for (i = 0; i < count; i++) {
        const VkDeviceQueueCreateInfo *q = &queues[i];
        VkResult ret = VK_SUCCESS;

        if (q->queueCount == 1 && !dev->queues[q->queueNodeIndex]) {
            ret = nulldrv_queue_create(dev, q->queueNodeIndex,
                    &dev->queues[q->queueNodeIndex]);
        }
        else {
            ret = VK_ERROR_INVALID_POINTER;
        }

        if (ret != VK_SUCCESS) {
            return ret;
        }
    }

    return VK_SUCCESS;
}

static enum nulldrv_ext_type nulldrv_gpu_lookup_extension(const struct nulldrv_gpu *gpu,
                                               const char *ext)
{
    enum nulldrv_ext_type type;

    for (type = 0; type < ARRAY_SIZE(nulldrv_gpu_exts); type++) {
        if (nulldrv_gpu_exts[type] && strcmp(nulldrv_gpu_exts[type], ext) == 0)
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
            VK_DBG_OBJECT_DEVICE);
    if (!dev)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    for (i = 0; i < info->extensionCount; i++) {
        const enum nulldrv_ext_type ext = nulldrv_gpu_lookup_extension(gpu,
                info->ppEnabledExtensionNames[i]);

        if (ext == NULLDRV_EXT_INVALID)
            return VK_ERROR_INVALID_EXTENSION;

        dev->exts[ext] = true;
    }

    ret = nulldrv_desc_ooxx_create(dev, &dev->desc_ooxx);
    if (ret != VK_SUCCESS) {
        return ret;
    }

    ret = dev_create_queues(dev, info->pRequestedQueues,
            info->queueRecordCount);
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

static VkResult nulldrv_rt_view_create(struct nulldrv_dev *dev,
                                const VkColorAttachmentViewCreateInfo *info,
                                struct nulldrv_rt_view **view_ret)
{
    struct nulldrv_rt_view *view;

    view = (struct nulldrv_rt_view *) nulldrv_base_create(dev, sizeof(*view),
            VK_DBG_OBJECT_COLOR_TARGET_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *view_ret = view;

    return VK_SUCCESS;
}

static VkResult nulldrv_fence_create(struct nulldrv_dev *dev,
                              const VkFenceCreateInfo *info,
                              struct nulldrv_fence **fence_ret)
{
    struct nulldrv_fence *fence;

    fence = (struct nulldrv_fence *) nulldrv_base_create(dev, sizeof(*fence),
            VK_DBG_OBJECT_FENCE);
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


static VkResult img_get_info(struct nulldrv_base *base, int type,
                               size_t *size, void *data)
{
    struct nulldrv_img *img = nulldrv_img_from_base(base);
    VkResult ret = VK_SUCCESS;

    switch (type) {
    case VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            VkMemoryRequirements *mem_req = data;

            *size = sizeof(VkMemoryRequirements);
            if (data == NULL)
                return ret;
            mem_req->size = img->total_size;
            mem_req->alignment = 4096;
        }
        break;
    default:
        ret = nulldrv_base_get_info(base, type, size, data);
        break;
    }

    return ret;
}

static VkResult nulldrv_img_create(struct nulldrv_dev *dev,
                            const VkImageCreateInfo *info,
                            bool scanout,
                            struct nulldrv_img **img_ret)
{
    struct nulldrv_img *img;

    img = (struct nulldrv_img *) nulldrv_base_create(dev, sizeof(*img),
            VK_DBG_OBJECT_IMAGE);
    if (!img)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    img->type = info->imageType;
    img->depth = info->extent.depth;
    img->mip_levels = info->mipLevels;
    img->array_size = info->arraySize;
    img->usage = info->usage;
    img->samples = info->samples;

    img->obj.base.get_info = img_get_info;

    *img_ret = img;

    return VK_SUCCESS;
}

static struct nulldrv_img *nulldrv_img(VkImage image)
{
    return (struct nulldrv_img *) image;
}

static VkResult nulldrv_mem_alloc(struct nulldrv_dev *dev,
                           const VkMemoryAllocInfo *info,
                           struct nulldrv_mem **mem_ret)
{
    struct nulldrv_mem *mem;

    mem = (struct nulldrv_mem *) nulldrv_base_create(dev, sizeof(*mem),
            VK_DBG_OBJECT_GPU_MEMORY);
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

static VkResult nulldrv_ds_view_create(struct nulldrv_dev *dev,
                                const VkDepthStencilViewCreateInfo *info,
                                struct nulldrv_ds_view **view_ret)
{
    struct nulldrv_img *img = nulldrv_img(info->image);
    struct nulldrv_ds_view *view;

    view = (struct nulldrv_ds_view *) nulldrv_base_create(dev, sizeof(*view),
            VK_DBG_OBJECT_DEPTH_STENCIL_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    view->img = img;

    view->array_size = info->arraySize;

    *view_ret = view;

    return VK_SUCCESS;
}

static VkResult nulldrv_sampler_create(struct nulldrv_dev *dev,
                                const VkSamplerCreateInfo *info,
                                struct nulldrv_sampler **sampler_ret)
{
    struct nulldrv_sampler *sampler;

    sampler = (struct nulldrv_sampler *) nulldrv_base_create(dev,
            sizeof(*sampler), VK_DBG_OBJECT_SAMPLER);
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
            VK_DBG_OBJECT_IMAGE_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    view->img = img;
    view->min_lod = info->minLod;

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
    return (struct nulldrv_mem *) mem;
}

static struct nulldrv_buf *nulldrv_buf_from_base(struct nulldrv_base *base)
{
    return (struct nulldrv_buf *) base;
}

static VkResult buf_get_info(struct nulldrv_base *base, int type,
                               size_t *size, void *data)
{
    struct nulldrv_buf *buf = nulldrv_buf_from_base(base);
    VkResult ret = VK_SUCCESS;

    switch (type) {
    case VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            VkMemoryRequirements *mem_req = data;

            *size = sizeof(VkMemoryRequirements);
            if (data == NULL)
                return ret;

            mem_req->size = buf->size;
            mem_req->alignment = 4096;

        }
        break;
    default:
        ret = nulldrv_base_get_info(base, type, size, data);
        break;
    }

    return ret;
}

static VkResult nulldrv_buf_create(struct nulldrv_dev *dev,
                            const VkBufferCreateInfo *info,
                            struct nulldrv_buf **buf_ret)
{
    struct nulldrv_buf *buf;

    buf = (struct nulldrv_buf *) nulldrv_base_create(dev, sizeof(*buf),
            VK_DBG_OBJECT_BUFFER);
    if (!buf)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    buf->size = info->size;
    buf->usage = info->usage;

    buf->obj.base.get_info = buf_get_info;

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
                VK_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT);
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
                VK_DBG_OBJECT_PIPELINE_LAYOUT);
    if (!pipeline_layout)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *pipeline_layout_ret = pipeline_layout;

    return VK_SUCCESS;
}

static struct nulldrv_desc_layout *nulldrv_desc_layout(VkDescriptorSetLayout layout)
{
    return (struct nulldrv_desc_layout *) layout;
}

static VkResult shader_create(struct nulldrv_dev *dev,
                                const VkShaderCreateInfo *info,
                                struct nulldrv_shader **sh_ret)
{
    struct nulldrv_shader *sh;

    sh = (struct nulldrv_shader *) nulldrv_base_create(dev, sizeof(*sh),
            VK_DBG_OBJECT_SHADER);
    if (!sh)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *sh_ret = sh;

    return VK_SUCCESS;
}

static VkResult graphics_pipeline_create(struct nulldrv_dev *dev,
                                           const VkGraphicsPipelineCreateInfo *info_,
                                           struct nulldrv_pipeline **pipeline_ret)
{
    struct nulldrv_pipeline *pipeline;

    pipeline = (struct nulldrv_pipeline *)
        nulldrv_base_create(dev, sizeof(*pipeline), 
                VK_DBG_OBJECT_GRAPHICS_PIPELINE);
    if (!pipeline)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *pipeline_ret = pipeline;

    return VK_SUCCESS;
}

static VkResult nulldrv_viewport_state_create(struct nulldrv_dev *dev,
                                       const VkDynamicVpStateCreateInfo *info,
                                       struct nulldrv_dynamic_vp **state_ret)
{
    struct nulldrv_dynamic_vp *state;

    state = (struct nulldrv_dynamic_vp *) nulldrv_base_create(dev,
            sizeof(*state), VK_DBG_OBJECT_VIEWPORT_STATE);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *state_ret = state;

    return VK_SUCCESS;
}

static VkResult nulldrv_raster_state_create(struct nulldrv_dev *dev,
                                     const VkDynamicRsStateCreateInfo *info,
                                     struct nulldrv_dynamic_rs **state_ret)
{
    struct nulldrv_dynamic_rs *state;

    state = (struct nulldrv_dynamic_rs *) nulldrv_base_create(dev,
            sizeof(*state), VK_DBG_OBJECT_RASTER_STATE);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *state_ret = state;

    return VK_SUCCESS;
}

static VkResult nulldrv_blend_state_create(struct nulldrv_dev *dev,
                                    const VkDynamicCbStateCreateInfo *info,
                                    struct nulldrv_dynamic_cb **state_ret)
{
    struct nulldrv_dynamic_cb *state;

    state = (struct nulldrv_dynamic_cb *) nulldrv_base_create(dev,
            sizeof(*state), VK_DBG_OBJECT_COLOR_BLEND_STATE);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *state_ret = state;

    return VK_SUCCESS;
}

static VkResult nulldrv_ds_state_create(struct nulldrv_dev *dev,
                                 const VkDynamicDsStateCreateInfo *info,
                                 struct nulldrv_dynamic_ds **state_ret)
{
    struct nulldrv_dynamic_ds *state;

    state = (struct nulldrv_dynamic_ds *) nulldrv_base_create(dev,
            sizeof(*state), VK_DBG_OBJECT_DEPTH_STENCIL_STATE);
    if (!state)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *state_ret = state;

    return VK_SUCCESS;
}


static VkResult nulldrv_cmd_create(struct nulldrv_dev *dev,
                            const VkCmdBufferCreateInfo *info,
                            struct nulldrv_cmd **cmd_ret)
{
    struct nulldrv_cmd *cmd;

    cmd = (struct nulldrv_cmd *) nulldrv_base_create(dev, sizeof(*cmd),
            VK_DBG_OBJECT_CMD_BUFFER);
    if (!cmd)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *cmd_ret = cmd;

    return VK_SUCCESS;
}

static VkResult nulldrv_desc_pool_create(struct nulldrv_dev *dev,
                                    VkDescriptorPoolUsage usage,
                                    uint32_t max_sets,
                                    const VkDescriptorPoolCreateInfo *info,
                                    struct nulldrv_desc_pool **pool_ret)
{
    struct nulldrv_desc_pool *pool;

    pool = (struct nulldrv_desc_pool *)
        nulldrv_base_create(dev, sizeof(*pool),
                VK_DBG_OBJECT_DESCRIPTOR_POOL);
    if (!pool)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    pool->dev = dev;

    *pool_ret = pool;

    return VK_SUCCESS;
}

static VkResult nulldrv_desc_set_create(struct nulldrv_dev *dev,
                                 struct nulldrv_desc_pool *pool,
                                 VkDescriptorSetUsage usage,
                                 const struct nulldrv_desc_layout *layout,
                                 struct nulldrv_desc_set **set_ret)
{
    struct nulldrv_desc_set *set;

    set = (struct nulldrv_desc_set *)
        nulldrv_base_create(dev, sizeof(*set), 
                VK_DBG_OBJECT_DESCRIPTOR_SET);
    if (!set)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    set->ooxx = dev->desc_ooxx;
    set->layout = layout;
    *set_ret = set;

    return VK_SUCCESS;
}

static struct nulldrv_desc_pool *nulldrv_desc_pool(VkDescriptorPool pool)
{
    return (struct nulldrv_desc_pool *) pool;
}

static VkResult nulldrv_fb_create(struct nulldrv_dev *dev,
                           const VkFramebufferCreateInfo* info,
                           struct nulldrv_framebuffer ** fb_ret)
{

    struct nulldrv_framebuffer *fb;
    fb = (struct nulldrv_framebuffer *) nulldrv_base_create(dev, sizeof(*fb),
            VK_DBG_OBJECT_FRAMEBUFFER);
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
            VK_DBG_OBJECT_RENDER_PASS);
    if (!rp)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    *rp_ret = rp;

    return VK_SUCCESS;
}

static struct nulldrv_buf *nulldrv_buf(VkBuffer buf)
{
    return (struct nulldrv_buf *) buf;
}

static VkResult nulldrv_buf_view_create(struct nulldrv_dev *dev,
                                 const VkBufferViewCreateInfo *info,
                                 struct nulldrv_buf_view **view_ret)
{
    struct nulldrv_buf *buf = nulldrv_buf(info->buffer);
    struct nulldrv_buf_view *view;

    view = (struct nulldrv_buf_view *) nulldrv_base_create(dev, sizeof(*view),
            VK_DBG_OBJECT_BUFFER_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    view->buf = buf;

    *view_ret = view;

    return VK_SUCCESS;
}


//*********************************************
// Driver entry points
//*********************************************

ICD_EXPORT VkResult VKAPI vkCreateBuffer(
    VkDevice                                  device,
    const VkBufferCreateInfo*               pCreateInfo,
    VkBuffer*                                 pBuffer)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_buf_create(dev, pCreateInfo, (struct nulldrv_buf **) pBuffer);
}

ICD_EXPORT VkResult VKAPI vkCreateCommandBuffer(
    VkDevice                                  device,
    const VkCmdBufferCreateInfo*           pCreateInfo,
    VkCmdBuffer*                             pCmdBuffer)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_cmd_create(dev, pCreateInfo,
            (struct nulldrv_cmd **) pCmdBuffer);
}

ICD_EXPORT VkResult VKAPI vkBeginCommandBuffer(
    VkCmdBuffer                              cmdBuffer,
    const VkCmdBufferBeginInfo            *info)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkEndCommandBuffer(
    VkCmdBuffer                              cmdBuffer)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkResetCommandBuffer(
    VkCmdBuffer                              cmdBuffer)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT void VKAPI vkCmdInitAtomicCounters(
    VkCmdBuffer                              cmdBuffer,
    VkPipelineBindPoint                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    const uint32_t*                             pData)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdLoadAtomicCounters(
    VkCmdBuffer                              cmdBuffer,
    VkPipelineBindPoint                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    VkBuffer                                  srcBuffer,
    VkDeviceSize                                srcOffset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdSaveAtomicCounters(
    VkCmdBuffer                              cmdBuffer,
    VkPipelineBindPoint                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    VkBuffer                                  destBuffer,
    VkDeviceSize                                destOffset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDbgMarkerBegin(
    VkCmdBuffer                              cmdBuffer,
    const char*                                 pMarker)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDbgMarkerEnd(
    VkCmdBuffer                              cmdBuffer)
{
    NULLDRV_LOG_FUNC;
}

static const VkFormat nulldrv_presentable_formats[] = {
    VK_FORMAT_B8G8R8A8_UNORM,
};

ICD_EXPORT VkResult VKAPI vkGetDisplayInfoWSI(
    VkDisplayWSI                            display,
    VkDisplayInfoTypeWSI                    infoType,
    size_t*                                 pDataSize,
    void*                                   pData)
{
    VkResult ret = VK_SUCCESS;

    NULLDRV_LOG_FUNC;

    if (!pDataSize)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
    case VK_DISPLAY_INFO_TYPE_FORMAT_PROPERTIES_WSI:
       {
            VkDisplayFormatPropertiesWSI *dst = pData;
            size_t size_ret;
            uint32_t i;

            size_ret = sizeof(*dst) * ARRAY_SIZE(nulldrv_presentable_formats);

            if (dst && *pDataSize < size_ret)
                return VK_ERROR_INVALID_VALUE;

            *pDataSize = size_ret;
            if (!dst)
                return VK_SUCCESS;

            for (i = 0; i < ARRAY_SIZE(nulldrv_presentable_formats); i++)
                dst[i].swapChainFormat = nulldrv_presentable_formats[i];
        }
        break;
    default:
        ret = VK_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}

ICD_EXPORT VkResult VKAPI vkCreateSwapChainWSI(
    VkDevice                                device,
    const VkSwapChainCreateInfoWSI*         pCreateInfo,
    VkSwapChainWSI*                         pSwapChain)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);
    struct nulldrv_swap_chain *sc;

    sc = (struct nulldrv_swap_chain *) nulldrv_base_create(dev, sizeof(*sc),
            /*VK_OBJECT_TYPE_SWAP_CHAIN_WSI*//* FIXME: DELETE THIS HACK: */VK_DBG_OBJECT_QUEUE);
    if (!sc) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    sc->dev = dev;

    *pSwapChain = (VkSwapChainWSI) sc;

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDestroySwapChainWSI(
    VkSwapChainWSI                          swapChain)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_swap_chain *sc = (struct nulldrv_swap_chain *) swapChain;

    free(sc);

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetSwapChainInfoWSI(
    VkSwapChainWSI                          swapChain,
    VkSwapChainInfoTypeWSI                  infoType,
    size_t*                                 pDataSize,
    void*                                   pData)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_swap_chain *sc = (struct nulldrv_swap_chain *) swapChain;
    struct nulldrv_dev *dev = sc->dev;
    VkResult ret = VK_SUCCESS;

    if (!pDataSize)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
    case VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_WSI:
        {
            VkSwapChainImageInfoWSI *images;
            const size_t size = sizeof(*images) * 2;
            uint32_t i;

            if (pData && *pDataSize < size)
                return VK_ERROR_INVALID_VALUE;

            *pDataSize = size;
            if (!pData)
                return VK_SUCCESS;

            images = (VkSwapChainImageInfoWSI *) pData;
            for (i = 0; i < 2; i++) {
                struct nulldrv_img *img;
                struct nulldrv_mem *mem;

                img = (struct nulldrv_img *) nulldrv_base_create(dev,
                        sizeof(*img),
                        VK_DBG_OBJECT_IMAGE);
                if (!img)
                    return VK_ERROR_OUT_OF_HOST_MEMORY;

                mem = (struct nulldrv_mem *) nulldrv_base_create(dev,
                        sizeof(*mem),
                        VK_DBG_OBJECT_GPU_MEMORY);
                if (!mem)
                    return VK_ERROR_OUT_OF_HOST_MEMORY;

                images[i].image = (VkImage) img;
                images[i].memory = (VkDeviceMemory) mem;
            }
        }
        break;
    default:
        ret = VK_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}

ICD_EXPORT VkResult VKAPI vkQueuePresentWSI(
    VkQueue                                 queue_,
    const VkPresentInfoWSI*                 pPresentInfo)
{
    NULLDRV_LOG_FUNC;

    return VK_SUCCESS;
}

ICD_EXPORT void VKAPI vkCmdCopyBuffer(
    VkCmdBuffer                              cmdBuffer,
    VkBuffer                                  srcBuffer,
    VkBuffer                                  destBuffer,
    uint32_t                                    regionCount,
    const VkBufferCopy*                      pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCopyImage(
    VkCmdBuffer                              cmdBuffer,
    VkImage                                   srcImage,
    VkImageLayout                            srcImageLayout,
    VkImage                                   destImage,
    VkImageLayout                            destImageLayout,
    uint32_t                                    regionCount,
    const VkImageCopy*                       pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBlitImage(
    VkCmdBuffer                              cmdBuffer,
    VkImage                                   srcImage,
    VkImageLayout                            srcImageLayout,
    VkImage                                   destImage,
    VkImageLayout                            destImageLayout,
    uint32_t                                    regionCount,
    const VkImageBlit*                       pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCopyBufferToImage(
    VkCmdBuffer                              cmdBuffer,
    VkBuffer                                  srcBuffer,
    VkImage                                   destImage,
    VkImageLayout                            destImageLayout,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCopyImageToBuffer(
    VkCmdBuffer                              cmdBuffer,
    VkImage                                   srcImage,
    VkImageLayout                            srcImageLayout,
    VkBuffer                                  destBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCloneImageData(
    VkCmdBuffer                              cmdBuffer,
    VkImage                                   srcImage,
    VkImageLayout                            srcImageLayout,
    VkImage                                   destImage,
    VkImageLayout                            destImageLayout)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdUpdateBuffer(
    VkCmdBuffer                              cmdBuffer,
    VkBuffer                                  destBuffer,
    VkDeviceSize                                destOffset,
    VkDeviceSize                                dataSize,
    const uint32_t*                             pData)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdFillBuffer(
    VkCmdBuffer                              cmdBuffer,
    VkBuffer                                  destBuffer,
    VkDeviceSize                                destOffset,
    VkDeviceSize                                fillSize,
    uint32_t                                    data)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdClearColorImage(
    VkCmdBuffer                         cmdBuffer,
    VkImage                             image,
    VkImageLayout                       imageLayout,
    const VkClearColor                 *pColor,
    uint32_t                            rangeCount,
    const VkImageSubresourceRange*      pRanges)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdClearDepthStencil(
    VkCmdBuffer                              cmdBuffer,
    VkImage                                   image,
    VkImageLayout                            imageLayout,
    float                                       depth,
    uint32_t                                    stencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*          pRanges)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdResolveImage(
    VkCmdBuffer                              cmdBuffer,
    VkImage                                   srcImage,
    VkImageLayout                            srcImageLayout,
    VkImage                                   destImage,
    VkImageLayout                            destImageLayout,
    uint32_t                                    regionCount,
    const VkImageResolve*                    pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBeginQuery(
    VkCmdBuffer                              cmdBuffer,
    VkQueryPool                              queryPool,
    uint32_t                                    slot,
    VkFlags                                   flags)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdEndQuery(
    VkCmdBuffer                              cmdBuffer,
    VkQueryPool                              queryPool,
    uint32_t                                    slot)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdResetQueryPool(
    VkCmdBuffer                              cmdBuffer,
    VkQueryPool                              queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdSetEvent(
    VkCmdBuffer                              cmdBuffer,
    VkEvent                                   event_,
    VkPipeEvent                              pipeEvent)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdResetEvent(
    VkCmdBuffer                              cmdBuffer,
    VkEvent                                   event_,
    VkPipeEvent                              pipeEvent)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCopyQueryPoolResults(
    VkCmdBuffer                                 cmdBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    VkBuffer                                    destBuffer,
    VkDeviceSize                                destOffset,
    VkDeviceSize                                destStride,
    VkFlags                                     flags)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdWriteTimestamp(
    VkCmdBuffer                              cmdBuffer,
    VkTimestampType                          timestampType,
    VkBuffer                                  destBuffer,
    VkDeviceSize                                destOffset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindPipeline(
    VkCmdBuffer                              cmdBuffer,
    VkPipelineBindPoint                     pipelineBindPoint,
    VkPipeline                                pipeline)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindDynamicStateObject(
    VkCmdBuffer                              cmdBuffer,
    VkStateBindPoint                        stateBindPoint,
    VkDynamicStateObject                    state)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindDescriptorSets(
    VkCmdBuffer                              cmdBuffer,
    VkPipelineBindPoint                     pipelineBindPoint,
    uint32_t                                firstSet,
    uint32_t                                setCount,
    const VkDescriptorSet*                  pDescriptorSets,
    uint32_t                                dynamicOffsetCount,
    const uint32_t*                         pDynamicOffsets)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                            pOffsets)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindIndexBuffer(
    VkCmdBuffer                              cmdBuffer,
    VkBuffer                                  buffer,
    VkDeviceSize                                offset,
    VkIndexType                              indexType)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDraw(
    VkCmdBuffer                              cmdBuffer,
    uint32_t                                    firstVertex,
    uint32_t                                    vertexCount,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDrawIndexed(
    VkCmdBuffer                              cmdBuffer,
    uint32_t                                    firstIndex,
    uint32_t                                    indexCount,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDrawIndirect(
    VkCmdBuffer                              cmdBuffer,
    VkBuffer                                  buffer,
    VkDeviceSize                                offset,
    uint32_t                                    count,
    uint32_t                                    stride)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDrawIndexedIndirect(
    VkCmdBuffer                              cmdBuffer,
    VkBuffer                                  buffer,
    VkDeviceSize                                offset,
    uint32_t                                    count,
    uint32_t                                    stride)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDispatch(
    VkCmdBuffer                              cmdBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDispatchIndirect(
    VkCmdBuffer                              cmdBuffer,
    VkBuffer                                  buffer,
    VkDeviceSize                                offset)
{
    NULLDRV_LOG_FUNC;
}

void VKAPI vkCmdWaitEvents(
    VkCmdBuffer                                 cmdBuffer,
    VkWaitEvent                                 waitEvent,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    uint32_t                                    memBarrierCount,
    const void**                                ppMemBarriers)
{
    NULLDRV_LOG_FUNC;
}

void VKAPI vkCmdPipelineBarrier(
    VkCmdBuffer                                 cmdBuffer,
    VkWaitEvent                                 waitEvent,
    uint32_t                                    pipeEventCount,
    const VkPipeEvent*                          pPipeEvents,
    uint32_t                                    memBarrierCount,
    const void**                                ppMemBarriers)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VkResult VKAPI vkCreateDevice(
    VkPhysicalDevice                            gpu_,
    const VkDeviceCreateInfo*               pCreateInfo,
    VkDevice*                                 pDevice)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_gpu *gpu = nulldrv_gpu(gpu_);
    return nulldrv_dev_create(gpu, pCreateInfo, (struct nulldrv_dev**)pDevice);
}

ICD_EXPORT VkResult VKAPI vkDestroyDevice(
    VkDevice                                  device)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetDeviceQueue(
    VkDevice                                  device,
    uint32_t                                    queueNodeIndex,
    uint32_t                                    queueIndex,
    VkQueue*                                  pQueue)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);
    *pQueue = (VkQueue) dev->queues[0];
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDeviceWaitIdle(
    VkDevice                                  device)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDbgSetValidationLevel(
    VkDevice                                  device,
    VkValidationLevel                        validationLevel)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDbgSetMessageFilter(
    VkDevice                                  device,
    int32_t                                     msgCode,
    VK_DBG_MSG_FILTER                          filter)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDbgSetDeviceOption(
    VkDevice                                  device,
    VK_DBG_DEVICE_OPTION                       dbgOption,
    size_t                                      dataSize,
    const void*                                 pData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateEvent(
    VkDevice                                  device,
    const VkEventCreateInfo*                pCreateInfo,
    VkEvent*                                  pEvent)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetEventStatus(
    VkDevice                                  device,
    VkEvent                                   event_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkSetEvent(
    VkDevice                                  device,
    VkEvent                                   event_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkResetEvent(
    VkDevice                                  device,
    VkEvent                                   event_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateFence(
    VkDevice                                  device,
    const VkFenceCreateInfo*                pCreateInfo,
    VkFence*                                  pFence)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_fence_create(dev, pCreateInfo,
            (struct nulldrv_fence **) pFence);
}

ICD_EXPORT VkResult VKAPI vkGetFenceStatus(
    VkDevice                                  device,
    VkFence                                   fence_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkResetFences(
    VkDevice                                  device,
    uint32_t                                   fenceCount,
    VkFence*                                  pFences)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkWaitForFences(
    VkDevice                                  device,
    uint32_t                                    fenceCount,
    const VkFence*                            pFences,
    bool32_t                                    waitAll,
    uint64_t                                    timeout)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetFormatInfo(
    VkDevice                                  device,
    VkFormat                                  format,
    VkFormatInfoType                        infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    VkFormatProperties *fmt = (VkFormatProperties *) pData;
    VkResult ret = VK_SUCCESS;

    switch (infoType) {
    case VK_FORMAT_INFO_TYPE_PROPERTIES:
        *pDataSize = sizeof(VkFormatProperties);
        if (pData == NULL)
            return ret;
        fmt->linearTilingFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        fmt->optimalTilingFeatures = fmt->linearTilingFeatures;
        break;
    default:
        ret = VK_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}

ICD_EXPORT VkResult VKAPI vkGetPhysicalDeviceInfo(
    VkPhysicalDevice                            gpu_,
    VkPhysicalDeviceInfoType                  infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    VkResult ret = VK_SUCCESS;

    switch (infoType) {
    case VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES:
      {
        VkPhysicalDeviceProperties *props =
            (VkPhysicalDeviceProperties *) pData;
        *pDataSize = sizeof(VkPhysicalDeviceProperties);
        if (pData == NULL) {
            return ret;
        }
        props->apiVersion = VK_API_VERSION;
        props->driverVersion = 0; // Appropriate that the nulldrv have 0's
        props->vendorId = 0;
        props->deviceId = 0;
        props->deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
        strncpy(props->deviceName, "nulldrv", strlen("nulldrv"));
        props->maxInlineMemoryUpdateSize = 0;
        props->maxBoundDescriptorSets = 0;
        props->maxThreadGroupSize = 0;
        props->timestampFrequency = 0;
        props->multiColorAttachmentClears = false;
        break;
      }
    case VK_PHYSICAL_DEVICE_INFO_TYPE_PERFORMANCE:
      {
        VkPhysicalDevicePerformance *perf =
            (VkPhysicalDevicePerformance *) pData;
        *pDataSize = sizeof(VkPhysicalDevicePerformance);
        if (pData == NULL) {
            return ret;
        }
        perf->maxDeviceClock = 1.0f;
        perf->aluPerClock = 1.0f;
        perf->texPerClock = 1.0f;
        perf->primsPerClock = 1.0f;
        perf->pixelsPerClock = 1.0f;
        break;
      }
    case VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES:
      {
        VkPhysicalDeviceQueueProperties *props =
            (VkPhysicalDeviceQueueProperties *) pData;
        *pDataSize = sizeof(VkPhysicalDeviceQueueProperties);
        if (pData == NULL) {
            return ret;
        }
        props->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_MEMMGR_BIT;
        props->queueCount = 1;
        props->maxAtomicCounters = 1;
        props->supportsTimestamps = false;
        break;
      }
    default:
/* FIXME: WRITE THE REAL CODE*/return ret;
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
    VkExtensionProperties *ext_props;
    uint32_t *count;

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = 1;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            else {
                ext_props = (VkExtensionProperties *) pData;
                ext_props->version = VK_WSI_LUNARG_REVISION;
                strncpy(ext_props->extName, "VK_WSI_LunarG",
                        strlen("VK_WSI_LunarG")+1);
                return VK_SUCCESS;
            }
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

VkResult VKAPI vkGetPhysicalDeviceExtensionInfo(
                                               VkPhysicalDevice gpu,
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
            *count = 0;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = 0;
            if (pData == NULL)
                return VK_SUCCESS;
            return VK_ERROR_INVALID_EXTENSION;
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
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkOpenPeerImage(
    VkDevice                                  device,
    const VkPeerImageOpenInfo*             pOpenInfo,
    VkImage*                                  pImage,
    VkDeviceMemory*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateImage(
    VkDevice                                  device,
    const VkImageCreateInfo*                pCreateInfo,
    VkImage*                                  pImage)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_img_create(dev, pCreateInfo, false,
            (struct nulldrv_img **) pImage);
}

ICD_EXPORT VkResult VKAPI vkGetImageSubresourceInfo(
    VkDevice                                    device,
    VkImage                                     image,
    const VkImageSubresource*                   pSubresource,
    VkSubresourceInfoType                       infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    VkResult ret = VK_SUCCESS;

    switch (infoType) {
    case VK_SUBRESOURCE_INFO_TYPE_LAYOUT:
        {
            VkSubresourceLayout *layout = (VkSubresourceLayout *) pData;

            *pDataSize = sizeof(VkSubresourceLayout);

            if (pData == NULL)
                return ret;
            layout->offset = 0;
            layout->size = 1;
            layout->rowPitch = 4;
            layout->depthPitch = 4;
        }
        break;
    default:
        ret = VK_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}

ICD_EXPORT VkResult VKAPI vkAllocMemory(
    VkDevice                                  device,
    const VkMemoryAllocInfo*                pAllocInfo,
    VkDeviceMemory*                             pMem)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_mem_alloc(dev, pAllocInfo, (struct nulldrv_mem **) pMem);
}

ICD_EXPORT VkResult VKAPI vkFreeMemory(
    VkDevice                                    device,
    VkDeviceMemory                              mem_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkSetMemoryPriority(
    VkDevice                                    device,
    VkDeviceMemory                              mem_,
    VkMemoryPriority                            priority)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkMapMemory(
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

    return (ptr) ? VK_SUCCESS : VK_ERROR_UNKNOWN;
}

ICD_EXPORT VkResult VKAPI vkUnmapMemory(
    VkDevice                                    device,
    VkDeviceMemory                              mem_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkFlushMappedMemoryRanges(
    VkDevice                                  device,
    uint32_t                                  memRangeCount,
    const VkMappedMemoryRange*                pMemRanges)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkInvalidateMappedMemoryRanges(
    VkDevice                                  device,
    uint32_t                                  memRangeCount,
    const VkMappedMemoryRange*                pMemRanges)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkPinSystemMemory(
    VkDevice                                  device,
    const void*                                 pSysMem,
    size_t                                      memSize,
    VkDeviceMemory*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkOpenSharedMemory(
    VkDevice                                  device,
    const VkMemoryOpenInfo*                 pOpenInfo,
    VkDeviceMemory*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkOpenPeerMemory(
    VkDevice                                  device,
    const VkPeerMemoryOpenInfo*            pOpenInfo,
    VkDeviceMemory*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateInstance(
    const VkInstanceCreateInfo*             pCreateInfo,
    VkInstance*                               pInstance)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_instance *inst;

    inst = (struct nulldrv_instance *) nulldrv_base_create(NULL, sizeof(*inst),
                VK_DBG_OBJECT_INSTANCE);
    if (!inst)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    inst->obj.base.get_info = NULL;

    *pInstance = (VkInstance) inst;

    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDestroyInstance(
    VkInstance                                pInstance)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(
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

ICD_EXPORT VkResult VKAPI vkEnumerateLayers(
    VkPhysicalDevice                            gpu,
    size_t                                      maxStringSize,
    size_t*                                     pLayerCount,
    char* const*                                pOutLayers,
    void*                                       pReserved)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDbgRegisterMsgCallback(
    VkInstance                                instance,
    VK_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback,
    void*                                       pUserData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDbgUnregisterMsgCallback(
    VkInstance                                instance,
    VK_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDbgSetGlobalOption(
    VkInstance                                instance,
    VK_DBG_GLOBAL_OPTION                       dbgOption,
    size_t                                      dataSize,
    const void*                                 pData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDestroyObject(
    VkDevice                                  device,
    VkObjectType                                objType,
    VkObject                                  object)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetObjectInfo(
    VkDevice                                    device,
    VkObjectType                                objType,
    VkObject                                    object,
    VkObjectInfoType                            infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_base *base = nulldrv_base(object);

    return base->get_info(base, infoType, pDataSize, pData);
}

ICD_EXPORT VkResult VKAPI vkQueueBindObjectMemory(
    VkQueue                                     queue,
    VkObjectType                                objType,
    VkObject                                    object,
    uint32_t                                    allocationIdx,
    VkDeviceMemory                              mem_,
    VkDeviceSize                                memOffset)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkQueueBindObjectMemoryRange(
    VkQueue                                     queue,
    VkObjectType                                objType,
    VkObject                                    object,
    uint32_t                                    allocationIdx,
    VkDeviceSize                                rangeOffset,
    VkDeviceSize                                rangeSize,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkQueueBindImageMemoryRange(
    VkQueue                                   queue,
    VkImage                                   image,
    uint32_t                                  allocationIdx,
    const VkImageMemoryBindInfo*              pBindInfo,
    VkDeviceMemory                            mem,
    VkDeviceSize                              memOffset)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkDbgSetObjectTag(
    VkDevice                                    device,
    VkObject                                    object,
    size_t                                      tagSize,
    const void*                                 pTag)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(
    VkDevice                                  device,
    const VkGraphicsPipelineCreateInfo*    pCreateInfo,
    VkPipeline*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return graphics_pipeline_create(dev, pCreateInfo,
            (struct nulldrv_pipeline **) pPipeline);
}

ICD_EXPORT VkResult VKAPI vkCreateGraphicsPipelineDerivative(
    VkDevice                                  device,
    const VkGraphicsPipelineCreateInfo*    pCreateInfo,
    VkPipeline                                basePipeline,
    VkPipeline*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return graphics_pipeline_create(dev, pCreateInfo,
            (struct nulldrv_pipeline **) pPipeline);
}

ICD_EXPORT VkResult VKAPI vkCreateComputePipeline(
    VkDevice                                  device,
    const VkComputePipelineCreateInfo*     pCreateInfo,
    VkPipeline*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkStorePipeline(
    VkDevice                                  device,
    VkPipeline                                pipeline,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkLoadPipeline(
    VkDevice                                  device,
    size_t                                    	dataSize,
    const void*                                 pData,
    VkPipeline*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkLoadPipelineDerivative(
    VkDevice                                  device,
    size_t                                    	dataSize,
    const void*                                 pData,
    VkPipeline				basePipeline,
    VkPipeline*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateQueryPool(
    VkDevice                                  device,
    const VkQueryPoolCreateInfo*           pCreateInfo,
    VkQueryPool*                             pQueryPool)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkGetQueryPoolResults(
    VkDevice                                    device,
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    size_t*                                     pDataSize,
    void*                                       pData,
    VkQueryResultFlags                          flags)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkQueueWaitIdle(
    VkQueue                                   queue_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkQueueSubmit(
    VkQueue                                   queue_,
    uint32_t                                    cmdBufferCount,
    const VkCmdBuffer*                       pCmdBuffers,
    VkFence                                   fence_)
{
    NULLDRV_LOG_FUNC;
   return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkOpenSharedSemaphore(
    VkDevice                                  device,
    const VkSemaphoreOpenInfo*              pOpenInfo,
    VkSemaphore*                              pSemaphore)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateSemaphore(
    VkDevice                                  device,
    const VkSemaphoreCreateInfo*            pCreateInfo,
    VkSemaphore*                              pSemaphore)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkQueueSignalSemaphore(
    VkQueue                                   queue,
    VkSemaphore                               semaphore)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkQueueWaitSemaphore(
    VkQueue                                   queue,
    VkSemaphore                               semaphore)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateSampler(
    VkDevice                                  device,
    const VkSamplerCreateInfo*              pCreateInfo,
    VkSampler*                                pSampler)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_sampler_create(dev, pCreateInfo,
            (struct nulldrv_sampler **) pSampler);
}

ICD_EXPORT VkResult VKAPI vkCreateShader(
        VkDevice                                  device,
        const VkShaderCreateInfo*               pCreateInfo,
        VkShader*                                 pShader)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return shader_create(dev, pCreateInfo, (struct nulldrv_shader **) pShader);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicViewportState(
    VkDevice                                  device,
    const VkDynamicVpStateCreateInfo*       pCreateInfo,
    VkDynamicVpState*                  pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_viewport_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_vp **) pState);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicRasterState(
    VkDevice                                  device,
    const VkDynamicRsStateCreateInfo*         pCreateInfo,
    VkDynamicRsState*                    pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_raster_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_rs **) pState);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(
    VkDevice                                  device,
    const VkDynamicCbStateCreateInfo*    pCreateInfo,
    VkDynamicCbState*               pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_blend_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_cb **) pState);
}

ICD_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(
    VkDevice                                  device,
    const VkDynamicDsStateCreateInfo*  pCreateInfo,
    VkDynamicDsState*             pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_ds_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_ds **) pState);
}

ICD_EXPORT VkResult VKAPI vkCreateBufferView(
    VkDevice                                  device,
    const VkBufferViewCreateInfo*          pCreateInfo,
    VkBufferView*                            pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_buf_view_create(dev, pCreateInfo,
            (struct nulldrv_buf_view **) pView);
}

ICD_EXPORT VkResult VKAPI vkCreateImageView(
    VkDevice                                  device,
    const VkImageViewCreateInfo*           pCreateInfo,
    VkImageView*                             pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_img_view_create(dev, pCreateInfo,
            (struct nulldrv_img_view **) pView);
}

ICD_EXPORT VkResult VKAPI vkCreateColorAttachmentView(
    VkDevice                                  device,
    const VkColorAttachmentViewCreateInfo* pCreateInfo,
    VkColorAttachmentView*                  pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_rt_view_create(dev, pCreateInfo,
            (struct nulldrv_rt_view **) pView);
}

ICD_EXPORT VkResult VKAPI vkCreateDepthStencilView(
    VkDevice                                  device,
    const VkDepthStencilViewCreateInfo*   pCreateInfo,
    VkDepthStencilView*                     pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_ds_view_create(dev, pCreateInfo,
            (struct nulldrv_ds_view **) pView);

}

ICD_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(
    VkDevice                                   device,
    const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    VkDescriptorSetLayout*                   pSetLayout)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_layout_create(dev, pCreateInfo,
            (struct nulldrv_desc_layout **) pSetLayout);
}

ICD_EXPORT VkResult VKAPI  vkCreatePipelineLayout(
    VkDevice                                device,
    const VkPipelineLayoutCreateInfo*       pCreateInfo,
    VkPipelineLayout*                       pPipelineLayout)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_pipeline_layout_create(dev,
            pCreateInfo,
            (struct nulldrv_pipeline_layout **) pPipelineLayout);
}

ICD_EXPORT VkResult VKAPI vkBeginDescriptorPoolUpdate(
    VkDevice                                   device,
    VkDescriptorUpdateMode                   updateMode)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkEndDescriptorPoolUpdate(
    VkDevice                                   device,
    VkCmdBuffer                               cmd_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkCreateDescriptorPool(
    VkDevice                                   device,
    VkDescriptorPoolUsage                  poolUsage,
    uint32_t                                     maxSets,
    const VkDescriptorPoolCreateInfo*     pCreateInfo,
    VkDescriptorPool*                       pDescriptorPool)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_pool_create(dev, poolUsage, maxSets, pCreateInfo,
            (struct nulldrv_desc_pool **) pDescriptorPool);
}

ICD_EXPORT VkResult VKAPI vkResetDescriptorPool(
    VkDevice                                device,
    VkDescriptorPool                        descriptorPool)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkAllocDescriptorSets(
    VkDevice                                device,
    VkDescriptorPool                        descriptorPool,
    VkDescriptorSetUsage                     setUsage,
    uint32_t                                     count,
    const VkDescriptorSetLayout*             pSetLayouts,
    VkDescriptorSet*                          pDescriptorSets,
    uint32_t*                                    pCount)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_desc_pool *pool = nulldrv_desc_pool(descriptorPool);
    struct nulldrv_dev *dev = pool->dev;
    VkResult ret = VK_SUCCESS;
    uint32_t i;

    for (i = 0; i < count; i++) {
        const struct nulldrv_desc_layout *layout =
            nulldrv_desc_layout((VkDescriptorSetLayout) pSetLayouts[i]);

        ret = nulldrv_desc_set_create(dev, pool, setUsage, layout,
                (struct nulldrv_desc_set **) &pDescriptorSets[i]);
        if (ret != VK_SUCCESS)
            break;
    }

    if (pCount)
        *pCount = i;

    return ret;
}

ICD_EXPORT void VKAPI vkClearDescriptorSets(
    VkDevice                                device,
    VkDescriptorPool                        descriptorPool,
    uint32_t                                     count,
    const VkDescriptorSet*                    pDescriptorSets)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkUpdateDescriptors(
    VkDevice                                  device,
    VkDescriptorSet                           descriptorSet,
    uint32_t                                     updateCount,
    const void**                                 ppUpdateArray)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VkResult VKAPI vkCreateFramebuffer(
    VkDevice                                  device,
    const VkFramebufferCreateInfo*          info,
    VkFramebuffer*                            fb_ret)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_fb_create(dev, info, (struct nulldrv_framebuffer **) fb_ret);
}


ICD_EXPORT VkResult VKAPI vkCreateRenderPass(
    VkDevice                                  device,
    const VkRenderPassCreateInfo*          info,
    VkRenderPass*                            rp_ret)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_render_pass_create(dev, info, (struct nulldrv_render_pass **) rp_ret);
}

ICD_EXPORT void VKAPI vkCmdBeginRenderPass(
    VkCmdBuffer                              cmdBuffer,
    const VkRenderPassBegin*                pRenderPassBegin)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdEndRenderPass(
    VkCmdBuffer                              cmdBuffer,
    VkRenderPass                             renderPass)
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
