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
	[NULLDRV_EXT_WSI_X11] = "VK_WSI_X11",
	[NULLDRV_EXT_WSI_WINDOWS] = "VK_WSI_WINDOWS"
};

static struct nulldrv_base *nulldrv_base(VK_BASE_OBJECT base)
{
    return (struct nulldrv_base *) base;
}

static VK_RESULT nulldrv_base_get_info(struct nulldrv_base *base, int type,
                               size_t *size, void *data)
{
    VK_RESULT ret = VK_SUCCESS;
    size_t s;
    uint32_t *count;

    switch (type) {
    case VK_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            VK_MEMORY_REQUIREMENTS *mem_req = data;
            s = sizeof(VK_MEMORY_REQUIREMENTS);
            *size = s;
            if (data == NULL)
                return ret;
            memset(data, 0, s);
            mem_req->memType =  VK_MEMORY_TYPE_OTHER;
            break;
        }
    case VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT:
        *size = sizeof(uint32_t);
        if (data == NULL)
            return ret;
        count = (uint32_t *) data;
        *count = 1;
        break;
    case VK_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS:
        s = sizeof(VK_IMAGE_MEMORY_REQUIREMENTS);
        *size = s;
        if (data == NULL)
            return ret;
        memset(data, 0, s);
        break;
    case VK_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS:
        s = sizeof(VK_BUFFER_MEMORY_REQUIREMENTS);
        *size = s;
        if (data == NULL)
            return ret;
        memset(data, 0, s);
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
    set_loader_magic_value(base);

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

static VK_RESULT nulldrv_gpu_add(int devid, const char *primary_node,
                         const char *render_node, struct nulldrv_gpu **gpu_ret)
{
    struct nulldrv_gpu *gpu;

	gpu = malloc(sizeof(*gpu));
    if (!gpu)
        return VK_ERROR_OUT_OF_MEMORY;
	memset(gpu, 0, sizeof(*gpu));
    
    // Initialize pointer to loader's dispatch table with ICD_LOADER_MAGIC
    set_loader_magic_value(gpu);

    *gpu_ret = gpu;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_queue_create(struct nulldrv_dev *dev,
                              uint32_t node_index,
                              struct nulldrv_queue **queue_ret)
{
    struct nulldrv_queue *queue;

    queue = (struct nulldrv_queue *) nulldrv_base_create(dev, sizeof(*queue),
            VK_DBG_OBJECT_QUEUE);
    if (!queue)
        return VK_ERROR_OUT_OF_MEMORY;

    queue->dev = dev;

    *queue_ret = queue;

    return VK_SUCCESS;
}

static VK_RESULT dev_create_queues(struct nulldrv_dev *dev,
                                    const VkDeviceQueueCreateInfo *queues,
                                    uint32_t count)
{
    uint32_t i;

    if (!count)
        return VK_ERROR_INVALID_POINTER;

    for (i = 0; i < count; i++) {
        const VkDeviceQueueCreateInfo *q = &queues[i];
        VK_RESULT ret = VK_SUCCESS;

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

static VK_RESULT nulldrv_desc_ooxx_create(struct nulldrv_dev *dev,
                                  struct nulldrv_desc_ooxx **ooxx_ret)
{
    struct nulldrv_desc_ooxx *ooxx;

    ooxx = malloc(sizeof(*ooxx));
    if (!ooxx) 
        return VK_ERROR_OUT_OF_MEMORY;

    memset(ooxx, 0, sizeof(*ooxx));

    ooxx->surface_desc_size = 0;
    ooxx->sampler_desc_size = 0;

    *ooxx_ret = ooxx; 

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_dev_create(struct nulldrv_gpu *gpu,
                            const VkDeviceCreateInfo *info,
                            struct nulldrv_dev **dev_ret)
{
    struct nulldrv_dev *dev;
    uint32_t i;
    VK_RESULT ret;

    dev = (struct nulldrv_dev *) nulldrv_base_create(NULL, sizeof(*dev),
            VK_DBG_OBJECT_DEVICE);
    if (!dev)
        return VK_ERROR_OUT_OF_MEMORY;

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

static struct nulldrv_gpu *nulldrv_gpu(VK_PHYSICAL_GPU gpu)
{
    return (struct nulldrv_gpu *) gpu;
}

static VK_RESULT nulldrv_rt_view_create(struct nulldrv_dev *dev,
                                const VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO *info,
                                struct nulldrv_rt_view **view_ret)
{
    struct nulldrv_rt_view *view;

    view = (struct nulldrv_rt_view *) nulldrv_base_create(dev, sizeof(*view),
            VK_DBG_OBJECT_COLOR_TARGET_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_MEMORY;

    *view_ret = view;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_fence_create(struct nulldrv_dev *dev,
                              const VK_FENCE_CREATE_INFO *info,
                              struct nulldrv_fence **fence_ret)
{
    struct nulldrv_fence *fence;

    fence = (struct nulldrv_fence *) nulldrv_base_create(dev, sizeof(*fence),
            VK_DBG_OBJECT_FENCE);
    if (!fence)
        return VK_ERROR_OUT_OF_MEMORY;

    *fence_ret = fence;

    return VK_SUCCESS;
}

static struct nulldrv_dev *nulldrv_dev(VK_DEVICE dev)
{
    return (struct nulldrv_dev *) dev;
}

static struct nulldrv_img *nulldrv_img_from_base(struct nulldrv_base *base)
{
    return (struct nulldrv_img *) base;
}


static VK_RESULT img_get_info(struct nulldrv_base *base, int type,
                               size_t *size, void *data)
{
    struct nulldrv_img *img = nulldrv_img_from_base(base);
    VK_RESULT ret = VK_SUCCESS;

    switch (type) {
    case VK_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            VK_MEMORY_REQUIREMENTS *mem_req = data;

            *size = sizeof(VK_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;
            mem_req->size = img->total_size;
            mem_req->alignment = 4096;
            mem_req->memType = VK_MEMORY_TYPE_IMAGE;
        }
        break;
    case VK_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS:
        {
            VK_IMAGE_MEMORY_REQUIREMENTS *img_req = data;

            *size = sizeof(VK_IMAGE_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;
            img_req->usage = img->usage;
            img_req->formatClass = img->format_class;
            img_req->samples = img->samples;
        }
        break;
    case VK_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS:
        {
            VK_BUFFER_MEMORY_REQUIREMENTS *buf_req = data;

            *size = sizeof(VK_BUFFER_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;
            buf_req->usage = img->usage;
        }
        break;
    default:
        ret = nulldrv_base_get_info(base, type, size, data);
        break;
    }

    return ret;
}

static VK_RESULT nulldrv_img_create(struct nulldrv_dev *dev,
                            const VK_IMAGE_CREATE_INFO *info,
                            bool scanout,
                            struct nulldrv_img **img_ret)
{
    struct nulldrv_img *img;

    img = (struct nulldrv_img *) nulldrv_base_create(dev, sizeof(*img),
            VK_DBG_OBJECT_IMAGE);
    if (!img)
        return VK_ERROR_OUT_OF_MEMORY;

    img->type = info->imageType;
    img->depth = info->extent.depth;
    img->mip_levels = info->mipLevels;
    img->array_size = info->arraySize;
    img->usage = info->usage;
    if (info->tiling == VK_LINEAR_TILING)
        img->format_class = VK_IMAGE_FORMAT_CLASS_LINEAR;
    else
        img->format_class = icd_format_get_class(info->format);
    img->samples = info->samples;

    img->obj.base.get_info = img_get_info;

    *img_ret = img;

    return VK_SUCCESS;
}

static struct nulldrv_img *nulldrv_img(VK_IMAGE image)
{
    return (struct nulldrv_img *) image;
}

static VK_RESULT nulldrv_mem_alloc(struct nulldrv_dev *dev,
                           const VkMemoryAllocInfo *info,
                           struct nulldrv_mem **mem_ret)
{
    struct nulldrv_mem *mem;

    mem = (struct nulldrv_mem *) nulldrv_base_create(dev, sizeof(*mem),
            VK_DBG_OBJECT_GPU_MEMORY);
    if (!mem)
        return VK_ERROR_OUT_OF_MEMORY;

    mem->bo = malloc(info->allocationSize);
    if (!mem->bo) {
        return VK_ERROR_OUT_OF_MEMORY;
    }

    mem->size = info->allocationSize;

    *mem_ret = mem;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_ds_view_create(struct nulldrv_dev *dev,
                                const VK_DEPTH_STENCIL_VIEW_CREATE_INFO *info,
                                struct nulldrv_ds_view **view_ret)
{
    struct nulldrv_img *img = nulldrv_img(info->image);
    struct nulldrv_ds_view *view;

    view = (struct nulldrv_ds_view *) nulldrv_base_create(dev, sizeof(*view),
            VK_DBG_OBJECT_DEPTH_STENCIL_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_MEMORY;

    view->img = img;

    view->array_size = info->arraySize;

    *view_ret = view;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_sampler_create(struct nulldrv_dev *dev,
                                const VK_SAMPLER_CREATE_INFO *info,
                                struct nulldrv_sampler **sampler_ret)
{
    struct nulldrv_sampler *sampler;

    sampler = (struct nulldrv_sampler *) nulldrv_base_create(dev,
            sizeof(*sampler), VK_DBG_OBJECT_SAMPLER);
    if (!sampler)
        return VK_ERROR_OUT_OF_MEMORY;

    *sampler_ret = sampler;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_img_view_create(struct nulldrv_dev *dev,
                                 const VK_IMAGE_VIEW_CREATE_INFO *info,
                                 struct nulldrv_img_view **view_ret)
{
    struct nulldrv_img *img = nulldrv_img(info->image);
    struct nulldrv_img_view *view;

    view = (struct nulldrv_img_view *) nulldrv_base_create(dev, sizeof(*view),
            VK_DBG_OBJECT_IMAGE_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_MEMORY;

    view->img = img;
    view->min_lod = info->minLod;

    view->cmd_len = 8;

    *view_ret = view;

    return VK_SUCCESS;
}

static void *nulldrv_mem_map(struct nulldrv_mem *mem, VK_FLAGS flags)
{
    return mem->bo;
}

static struct nulldrv_mem *nulldrv_mem(VK_GPU_MEMORY mem)
{
    return (struct nulldrv_mem *) mem;
}

static struct nulldrv_buf *nulldrv_buf_from_base(struct nulldrv_base *base)
{
    return (struct nulldrv_buf *) base;
}

static VK_RESULT buf_get_info(struct nulldrv_base *base, int type,
                               size_t *size, void *data)
{
    struct nulldrv_buf *buf = nulldrv_buf_from_base(base);
    VK_RESULT ret = VK_SUCCESS;

    switch (type) {
    case VK_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            VK_MEMORY_REQUIREMENTS *mem_req = data;

            *size = sizeof(VK_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;

            mem_req->size = buf->size;
            mem_req->alignment = 4096;
            mem_req->memType = VK_MEMORY_TYPE_BUFFER;

        }
        break;
        case VK_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS:
        {
            VK_BUFFER_MEMORY_REQUIREMENTS *buf_req = data;

            *size = sizeof(VK_BUFFER_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;
            buf_req->usage = buf->usage;
        }
        break;
    default:
        ret = nulldrv_base_get_info(base, type, size, data);
        break;
    }

    return ret;
}

static VK_RESULT nulldrv_buf_create(struct nulldrv_dev *dev,
                            const VkBufferCreateInfo *info,
                            struct nulldrv_buf **buf_ret)
{
    struct nulldrv_buf *buf;

    buf = (struct nulldrv_buf *) nulldrv_base_create(dev, sizeof(*buf),
            VK_DBG_OBJECT_BUFFER);
    if (!buf)
        return VK_ERROR_OUT_OF_MEMORY;

    buf->size = info->size;
    buf->usage = info->usage;

    buf->obj.base.get_info = buf_get_info;

    *buf_ret = buf;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_desc_layout_create(struct nulldrv_dev *dev,
                                    const VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO *info,
                                    struct nulldrv_desc_layout **layout_ret)
{
    struct nulldrv_desc_layout *layout;

    layout = (struct nulldrv_desc_layout *)
        nulldrv_base_create(dev, sizeof(*layout),
                VK_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT);
    if (!layout)
        return VK_ERROR_OUT_OF_MEMORY;

    *layout_ret = layout;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_desc_layout_chain_create(struct nulldrv_dev *dev,
                                    uint32_t setLayoutArrayCount,
                                    const VK_DESCRIPTOR_SET_LAYOUT *pSetLayoutArray,
                                    struct nulldrv_desc_layout_chain **chain_ret)
{
    struct nulldrv_desc_layout_chain *chain;

    chain = (struct nulldrv_desc_layout_chain *)
        nulldrv_base_create(dev, sizeof(*chain),
                VK_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT_CHAIN);
    if (!chain)
        return VK_ERROR_OUT_OF_MEMORY;

    *chain_ret = chain;

    return VK_SUCCESS;
}

static struct nulldrv_desc_layout *nulldrv_desc_layout(VK_DESCRIPTOR_SET_LAYOUT layout)
{
    return (struct nulldrv_desc_layout *) layout;
}

static VK_RESULT shader_create(struct nulldrv_dev *dev,
                                const VK_SHADER_CREATE_INFO *info,
                                struct nulldrv_shader **sh_ret)
{
    struct nulldrv_shader *sh;

    sh = (struct nulldrv_shader *) nulldrv_base_create(dev, sizeof(*sh),
            VK_DBG_OBJECT_SHADER);
    if (!sh)
        return VK_ERROR_OUT_OF_MEMORY;

    *sh_ret = sh;

    return VK_SUCCESS;
}

static VK_RESULT graphics_pipeline_create(struct nulldrv_dev *dev,
                                           const VK_GRAPHICS_PIPELINE_CREATE_INFO *info_,
                                           struct nulldrv_pipeline **pipeline_ret)
{
    struct nulldrv_pipeline *pipeline;

    pipeline = (struct nulldrv_pipeline *)
        nulldrv_base_create(dev, sizeof(*pipeline), 
                VK_DBG_OBJECT_GRAPHICS_PIPELINE);
    if (!pipeline)
        return VK_ERROR_OUT_OF_MEMORY;

    *pipeline_ret = pipeline;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_viewport_state_create(struct nulldrv_dev *dev,
                                       const VK_DYNAMIC_VP_STATE_CREATE_INFO *info,
                                       struct nulldrv_dynamic_vp **state_ret)
{
    struct nulldrv_dynamic_vp *state;

    state = (struct nulldrv_dynamic_vp *) nulldrv_base_create(dev,
            sizeof(*state), VK_DBG_OBJECT_VIEWPORT_STATE);
    if (!state)
        return VK_ERROR_OUT_OF_MEMORY;

    *state_ret = state;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_raster_state_create(struct nulldrv_dev *dev,
                                     const VK_DYNAMIC_RS_STATE_CREATE_INFO *info,
                                     struct nulldrv_dynamic_rs **state_ret)
{
    struct nulldrv_dynamic_rs *state;

    state = (struct nulldrv_dynamic_rs *) nulldrv_base_create(dev,
            sizeof(*state), VK_DBG_OBJECT_RASTER_STATE);
    if (!state)
        return VK_ERROR_OUT_OF_MEMORY;

    *state_ret = state;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_blend_state_create(struct nulldrv_dev *dev,
                                    const VK_DYNAMIC_CB_STATE_CREATE_INFO *info,
                                    struct nulldrv_dynamic_cb **state_ret)
{
    struct nulldrv_dynamic_cb *state;

    state = (struct nulldrv_dynamic_cb *) nulldrv_base_create(dev,
            sizeof(*state), VK_DBG_OBJECT_COLOR_BLEND_STATE);
    if (!state)
        return VK_ERROR_OUT_OF_MEMORY;

    *state_ret = state;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_ds_state_create(struct nulldrv_dev *dev,
                                 const VK_DYNAMIC_DS_STATE_CREATE_INFO *info,
                                 struct nulldrv_dynamic_ds **state_ret)
{
    struct nulldrv_dynamic_ds *state;

    state = (struct nulldrv_dynamic_ds *) nulldrv_base_create(dev,
            sizeof(*state), VK_DBG_OBJECT_DEPTH_STENCIL_STATE);
    if (!state)
        return VK_ERROR_OUT_OF_MEMORY;

    *state_ret = state;

    return VK_SUCCESS;
}


static VK_RESULT nulldrv_cmd_create(struct nulldrv_dev *dev,
                            const VK_CMD_BUFFER_CREATE_INFO *info,
                            struct nulldrv_cmd **cmd_ret)
{
    struct nulldrv_cmd *cmd;

    cmd = (struct nulldrv_cmd *) nulldrv_base_create(dev, sizeof(*cmd),
            VK_DBG_OBJECT_CMD_BUFFER);
    if (!cmd)
        return VK_ERROR_OUT_OF_MEMORY;

    *cmd_ret = cmd;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_desc_pool_create(struct nulldrv_dev *dev,
                                    VK_DESCRIPTOR_POOL_USAGE usage,
                                    uint32_t max_sets,
                                    const VK_DESCRIPTOR_POOL_CREATE_INFO *info,
                                    struct nulldrv_desc_pool **pool_ret)
{
    struct nulldrv_desc_pool *pool;

    pool = (struct nulldrv_desc_pool *)
        nulldrv_base_create(dev, sizeof(*pool),
                VK_DBG_OBJECT_DESCRIPTOR_POOL);
    if (!pool)
        return VK_ERROR_OUT_OF_MEMORY;

    pool->dev = dev;

    *pool_ret = pool;

    return VK_SUCCESS;
}

static VK_RESULT nulldrv_desc_set_create(struct nulldrv_dev *dev,
                                 struct nulldrv_desc_pool *pool,
                                 VK_DESCRIPTOR_SET_USAGE usage,
                                 const struct nulldrv_desc_layout *layout,
                                 struct nulldrv_desc_set **set_ret)
{
    struct nulldrv_desc_set *set;

    set = (struct nulldrv_desc_set *)
        nulldrv_base_create(dev, sizeof(*set), 
                VK_DBG_OBJECT_DESCRIPTOR_SET);
    if (!set)
        return VK_ERROR_OUT_OF_MEMORY;

    set->ooxx = dev->desc_ooxx;
    set->layout = layout;
    *set_ret = set;

    return VK_SUCCESS;
}

static struct nulldrv_desc_pool *nulldrv_desc_pool(VK_DESCRIPTOR_POOL pool)
{
    return (struct nulldrv_desc_pool *) pool;
}

static VK_RESULT nulldrv_fb_create(struct nulldrv_dev *dev,
                           const VK_FRAMEBUFFER_CREATE_INFO* info,
                           struct nulldrv_framebuffer ** fb_ret)
{

    struct nulldrv_framebuffer *fb;
    fb = (struct nulldrv_framebuffer *) nulldrv_base_create(dev, sizeof(*fb),
            VK_DBG_OBJECT_FRAMEBUFFER);
    if (!fb)
        return VK_ERROR_OUT_OF_MEMORY;

    *fb_ret = fb;

    return VK_SUCCESS;

}

static VK_RESULT nulldrv_render_pass_create(struct nulldrv_dev *dev,
                           const VK_RENDER_PASS_CREATE_INFO* info,
                           struct nulldrv_render_pass** rp_ret)
{
    struct nulldrv_render_pass *rp;
    rp = (struct nulldrv_render_pass *) nulldrv_base_create(dev, sizeof(*rp),
            VK_DBG_OBJECT_RENDER_PASS);
    if (!rp)
        return VK_ERROR_OUT_OF_MEMORY;

    *rp_ret = rp;

    return VK_SUCCESS;
}

static struct nulldrv_buf *nulldrv_buf(VK_BUFFER buf)
{
    return (struct nulldrv_buf *) buf;
}

static VK_RESULT nulldrv_buf_view_create(struct nulldrv_dev *dev,
                                 const VkBufferViewCreateInfo *info,
                                 struct nulldrv_buf_view **view_ret)
{
    struct nulldrv_buf *buf = nulldrv_buf(info->buffer);
    struct nulldrv_buf_view *view;

    view = (struct nulldrv_buf_view *) nulldrv_base_create(dev, sizeof(*view),
            VK_DBG_OBJECT_BUFFER_VIEW);
    if (!view)
        return VK_ERROR_OUT_OF_MEMORY;

    view->buf = buf;

    *view_ret = view;

    return VK_SUCCESS;
}


//*********************************************
// Driver entry points
//*********************************************

ICD_EXPORT VK_RESULT VKAPI vkCreateBuffer(
    VK_DEVICE                                  device,
    const VkBufferCreateInfo*               pCreateInfo,
    VK_BUFFER*                                 pBuffer)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_buf_create(dev, pCreateInfo, (struct nulldrv_buf **) pBuffer);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateCommandBuffer(
    VK_DEVICE                                  device,
    const VK_CMD_BUFFER_CREATE_INFO*           pCreateInfo,
    VK_CMD_BUFFER*                             pCmdBuffer)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_cmd_create(dev, pCreateInfo,
            (struct nulldrv_cmd **) pCmdBuffer);
}

ICD_EXPORT VK_RESULT VKAPI vkBeginCommandBuffer(
    VK_CMD_BUFFER                              cmdBuffer,
    const VK_CMD_BUFFER_BEGIN_INFO            *info)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkEndCommandBuffer(
    VK_CMD_BUFFER                              cmdBuffer)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkResetCommandBuffer(
    VK_CMD_BUFFER                              cmdBuffer)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT void VKAPI vkCmdInitAtomicCounters(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_PIPELINE_BIND_POINT                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    const uint32_t*                             pData)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdLoadAtomicCounters(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_PIPELINE_BIND_POINT                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    VK_BUFFER                                  srcBuffer,
    VK_GPU_SIZE                                srcOffset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdSaveAtomicCounters(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_PIPELINE_BIND_POINT                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    VK_BUFFER                                  destBuffer,
    VK_GPU_SIZE                                destOffset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDbgMarkerBegin(
    VK_CMD_BUFFER                              cmdBuffer,
    const char*                                 pMarker)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDbgMarkerEnd(
    VK_CMD_BUFFER                              cmdBuffer)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCopyBuffer(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_BUFFER                                  srcBuffer,
    VK_BUFFER                                  destBuffer,
    uint32_t                                    regionCount,
    const VK_BUFFER_COPY*                      pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCopyImage(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_IMAGE                                   srcImage,
    VK_IMAGE_LAYOUT                            srcImageLayout,
    VK_IMAGE                                   destImage,
    VK_IMAGE_LAYOUT                            destImageLayout,
    uint32_t                                    regionCount,
    const VK_IMAGE_COPY*                       pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBlitImage(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_IMAGE                                   srcImage,
    VK_IMAGE_LAYOUT                            srcImageLayout,
    VK_IMAGE                                   destImage,
    VK_IMAGE_LAYOUT                            destImageLayout,
    uint32_t                                    regionCount,
    const VK_IMAGE_BLIT*                       pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCopyBufferToImage(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_BUFFER                                  srcBuffer,
    VK_IMAGE                                   destImage,
    VK_IMAGE_LAYOUT                            destImageLayout,
    uint32_t                                    regionCount,
    const VK_BUFFER_IMAGE_COPY*                pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCopyImageToBuffer(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_IMAGE                                   srcImage,
    VK_IMAGE_LAYOUT                            srcImageLayout,
    VK_BUFFER                                  destBuffer,
    uint32_t                                    regionCount,
    const VK_BUFFER_IMAGE_COPY*                pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdCloneImageData(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_IMAGE                                   srcImage,
    VK_IMAGE_LAYOUT                            srcImageLayout,
    VK_IMAGE                                   destImage,
    VK_IMAGE_LAYOUT                            destImageLayout)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdUpdateBuffer(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_BUFFER                                  destBuffer,
    VK_GPU_SIZE                                destOffset,
    VK_GPU_SIZE                                dataSize,
    const uint32_t*                             pData)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdFillBuffer(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_BUFFER                                  destBuffer,
    VK_GPU_SIZE                                destOffset,
    VK_GPU_SIZE                                fillSize,
    uint32_t                                    data)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdClearColorImage(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_IMAGE                                   image,
    VK_IMAGE_LAYOUT                            imageLayout,
    VK_CLEAR_COLOR                             color,
    uint32_t                                    rangeCount,
    const VK_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdClearDepthStencil(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_IMAGE                                   image,
    VK_IMAGE_LAYOUT                            imageLayout,
    float                                       depth,
    uint32_t                                    stencil,
    uint32_t                                    rangeCount,
    const VK_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdResolveImage(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_IMAGE                                   srcImage,
    VK_IMAGE_LAYOUT                            srcImageLayout,
    VK_IMAGE                                   destImage,
    VK_IMAGE_LAYOUT                            destImageLayout,
    uint32_t                                    rectCount,
    const VK_IMAGE_RESOLVE*                    pRects)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBeginQuery(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_QUERY_POOL                              queryPool,
    uint32_t                                    slot,
    VK_FLAGS                                   flags)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdEndQuery(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_QUERY_POOL                              queryPool,
    uint32_t                                    slot)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdResetQueryPool(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_QUERY_POOL                              queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdSetEvent(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_EVENT                                   event_,
    VK_PIPE_EVENT                              pipeEvent)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdResetEvent(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_EVENT                                   event_,
    VK_PIPE_EVENT                              pipeEvent)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdWriteTimestamp(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_TIMESTAMP_TYPE                          timestampType,
    VK_BUFFER                                  destBuffer,
    VK_GPU_SIZE                                destOffset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindPipeline(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_PIPELINE_BIND_POINT                     pipelineBindPoint,
    VK_PIPELINE                                pipeline)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindDynamicStateObject(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_STATE_BIND_POINT                        stateBindPoint,
    VK_DYNAMIC_STATE_OBJECT                    state)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindDescriptorSets(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_PIPELINE_BIND_POINT                     pipelineBindPoint,
    VK_DESCRIPTOR_SET_LAYOUT_CHAIN             layoutChain,
    uint32_t                                    layoutChainSlot,
    uint32_t                                    count,
    const VK_DESCRIPTOR_SET*                   pDescriptorSets,
    const uint32_t*                             pUserData)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindVertexBuffer(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_BUFFER                                  buffer,
    VK_GPU_SIZE                                offset,
    uint32_t                                    binding)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdBindIndexBuffer(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_BUFFER                                  buffer,
    VK_GPU_SIZE                                offset,
    VK_INDEX_TYPE                              indexType)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDraw(
    VK_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    firstVertex,
    uint32_t                                    vertexCount,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDrawIndexed(
    VK_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    firstIndex,
    uint32_t                                    indexCount,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDrawIndirect(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_BUFFER                                  buffer,
    VK_GPU_SIZE                                offset,
    uint32_t                                    count,
    uint32_t                                    stride)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDrawIndexedIndirect(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_BUFFER                                  buffer,
    VK_GPU_SIZE                                offset,
    uint32_t                                    count,
    uint32_t                                    stride)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDispatch(
    VK_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdDispatchIndirect(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_BUFFER                                  buffer,
    VK_GPU_SIZE                                offset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdWaitEvents(
    VK_CMD_BUFFER                              cmdBuffer,
    const VK_EVENT_WAIT_INFO*                  pWaitInfo)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdPipelineBarrier(
    VK_CMD_BUFFER                              cmdBuffer,
    const VK_PIPELINE_BARRIER*                 pBarrier)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateDevice(
    VK_PHYSICAL_GPU                            gpu_,
    const VkDeviceCreateInfo*               pCreateInfo,
    VK_DEVICE*                                 pDevice)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_gpu *gpu = nulldrv_gpu(gpu_);
    return nulldrv_dev_create(gpu, pCreateInfo, (struct nulldrv_dev**)pDevice);
}

ICD_EXPORT VK_RESULT VKAPI vkDestroyDevice(
    VK_DEVICE                                  device)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkGetDeviceQueue(
    VK_DEVICE                                  device,
    uint32_t                                    queueNodeIndex,
    uint32_t                                    queueIndex,
    VK_QUEUE*                                  pQueue)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);
    *pQueue = dev->queues[0];
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDeviceWaitIdle(
    VK_DEVICE                                  device)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDbgSetValidationLevel(
    VK_DEVICE                                  device,
    VK_VALIDATION_LEVEL                        validationLevel)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDbgSetMessageFilter(
    VK_DEVICE                                  device,
    int32_t                                     msgCode,
    VK_DBG_MSG_FILTER                          filter)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDbgSetDeviceOption(
    VK_DEVICE                                  device,
    VK_DBG_DEVICE_OPTION                       dbgOption,
    size_t                                      dataSize,
    const void*                                 pData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateEvent(
    VK_DEVICE                                  device,
    const VK_EVENT_CREATE_INFO*                pCreateInfo,
    VK_EVENT*                                  pEvent)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkGetEventStatus(
    VK_EVENT                                   event_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkSetEvent(
    VK_EVENT                                   event_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkResetEvent(
    VK_EVENT                                   event_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateFence(
    VK_DEVICE                                  device,
    const VK_FENCE_CREATE_INFO*                pCreateInfo,
    VK_FENCE*                                  pFence)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_fence_create(dev, pCreateInfo,
            (struct nulldrv_fence **) pFence);
}

ICD_EXPORT VK_RESULT VKAPI vkGetFenceStatus(
    VK_FENCE                                   fence_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkWaitForFences(
    VK_DEVICE                                  device,
    uint32_t                                    fenceCount,
    const VK_FENCE*                            pFences,
    bool32_t                                    waitAll,
    uint64_t                                    timeout)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkGetFormatInfo(
    VK_DEVICE                                  device,
    VK_FORMAT                                  format,
    VK_FORMAT_INFO_TYPE                        infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkGetGpuInfo(
    VK_PHYSICAL_GPU                            gpu_,
    VK_PHYSICAL_GPU_INFO_TYPE                  infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkGetExtensionSupport(
    VK_PHYSICAL_GPU                            gpu_,
    const char*                                 pExtName)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkGetMultiGpuCompatibility(
    VK_PHYSICAL_GPU                            gpu0_,
    VK_PHYSICAL_GPU                            gpu1_,
    VK_GPU_COMPATIBILITY_INFO*                 pInfo)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkOpenPeerImage(
    VK_DEVICE                                  device,
    const VK_PEER_IMAGE_OPEN_INFO*             pOpenInfo,
    VK_IMAGE*                                  pImage,
    VK_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateImage(
    VK_DEVICE                                  device,
    const VK_IMAGE_CREATE_INFO*                pCreateInfo,
    VK_IMAGE*                                  pImage)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_img_create(dev, pCreateInfo, false,
            (struct nulldrv_img **) pImage);
}

ICD_EXPORT VK_RESULT VKAPI vkGetImageSubresourceInfo(
    VK_IMAGE                                   image,
    const VK_IMAGE_SUBRESOURCE*                pSubresource,
    VK_SUBRESOURCE_INFO_TYPE                   infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    VK_RESULT ret = VK_SUCCESS;

    switch (infoType) {
    case VK_INFO_TYPE_SUBRESOURCE_LAYOUT:
        {
            VK_SUBRESOURCE_LAYOUT *layout = (VK_SUBRESOURCE_LAYOUT *) pData;

            *pDataSize = sizeof(VK_SUBRESOURCE_LAYOUT);

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

ICD_EXPORT VK_RESULT VKAPI vkAllocMemory(
    VK_DEVICE                                  device,
    const VkMemoryAllocInfo*                pAllocInfo,
    VK_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_mem_alloc(dev, pAllocInfo, (struct nulldrv_mem **) pMem);
}

ICD_EXPORT VK_RESULT VKAPI vkFreeMemory(
    VK_GPU_MEMORY                              mem_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkSetMemoryPriority(
    VK_GPU_MEMORY                              mem_,
    VK_MEMORY_PRIORITY                         priority)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkMapMemory(
    VK_GPU_MEMORY                              mem_,
    VK_FLAGS                                   flags,
    void**                                      ppData)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_mem *mem = nulldrv_mem(mem_);
    void *ptr = nulldrv_mem_map(mem, flags);

    *ppData = ptr;

    return (ptr) ? VK_SUCCESS : VK_ERROR_UNKNOWN;
}

ICD_EXPORT VK_RESULT VKAPI vkUnmapMemory(
    VK_GPU_MEMORY                              mem_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkPinSystemMemory(
    VK_DEVICE                                  device,
    const void*                                 pSysMem,
    size_t                                      memSize,
    VK_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkOpenSharedMemory(
    VK_DEVICE                                  device,
    const VK_MEMORY_OPEN_INFO*                 pOpenInfo,
    VK_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkOpenPeerMemory(
    VK_DEVICE                                  device,
    const VK_PEER_MEMORY_OPEN_INFO*            pOpenInfo,
    VK_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateInstance(
    const VkInstanceCreateInfo*             pCreateInfo,
    VK_INSTANCE*                               pInstance)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_instance *inst;

    inst = (struct nulldrv_instance *) nulldrv_base_create(NULL, sizeof(*inst),
                VK_DBG_OBJECT_INSTANCE);
    if (!inst)
        return VK_ERROR_OUT_OF_MEMORY;

    inst->obj.base.get_info = NULL;

    *pInstance = (VK_INSTANCE*)inst;

    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDestroyInstance(
    VK_INSTANCE                                pInstance)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkEnumerateGpus(
    VK_INSTANCE                                instance,
    uint32_t                                    maxGpus,
    uint32_t*                                   pGpuCount,
    VK_PHYSICAL_GPU*                           pGpus)
{
    NULLDRV_LOG_FUNC;
    VK_RESULT ret;
    struct nulldrv_gpu *gpu;
    *pGpuCount = 1;
    ret = nulldrv_gpu_add(0, 0, 0, &gpu);
    if (ret == VK_SUCCESS)
        pGpus[0] = (VK_PHYSICAL_GPU) gpu;
    return ret;
}

ICD_EXPORT VK_RESULT VKAPI vkEnumerateLayers(
    VK_PHYSICAL_GPU                            gpu,
    size_t                                      maxLayerCount,
    size_t                                      maxStringSize,
    size_t*                                     pOutLayerCount,
    char* const*                                pOutLayers,
    void*                                       pReserved)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDbgRegisterMsgCallback(
    VK_INSTANCE                                instance,
    VK_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback,
    void*                                       pUserData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDbgUnregisterMsgCallback(
    VK_INSTANCE                                instance,
    VK_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDbgSetGlobalOption(
    VK_INSTANCE                                instance,
    VK_DBG_GLOBAL_OPTION                       dbgOption,
    size_t                                      dataSize,
    const void*                                 pData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDestroyObject(
    VK_OBJECT                                  object)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkGetObjectInfo(
    VK_BASE_OBJECT                             object,
    VK_OBJECT_INFO_TYPE                        infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_base *base = nulldrv_base(object);

    return base->get_info(base, infoType, pDataSize, pData);
}

ICD_EXPORT VK_RESULT VKAPI vkBindObjectMemory(
    VK_OBJECT                                  object,
    uint32_t                                    allocationIdx,
    VK_GPU_MEMORY                              mem_,
    VK_GPU_SIZE                                memOffset)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkBindObjectMemoryRange(
    VK_OBJECT                                  object,
    uint32_t                                    allocationIdx,
    VK_GPU_SIZE                                rangeOffset,
    VK_GPU_SIZE                                rangeSize,
    VK_GPU_MEMORY                              mem,
    VK_GPU_SIZE                                memOffset)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkBindImageMemoryRange(
    VK_IMAGE                                   image,
    uint32_t                                    allocationIdx,
    const VK_IMAGE_MEMORY_BIND_INFO*           bindInfo,
    VK_GPU_MEMORY                              mem,
    VK_GPU_SIZE                                memOffset)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkDbgSetObjectTag(
    VK_BASE_OBJECT                             object,
    size_t                                      tagSize,
    const void*                                 pTag)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateGraphicsPipeline(
    VK_DEVICE                                  device,
    const VK_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    VK_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return graphics_pipeline_create(dev, pCreateInfo,
            (struct nulldrv_pipeline **) pPipeline);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateGraphicsPipelineDerivative(
    VK_DEVICE                                  device,
    const VK_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    VK_PIPELINE                                basePipeline,
    VK_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return graphics_pipeline_create(dev, pCreateInfo,
            (struct nulldrv_pipeline **) pPipeline);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateComputePipeline(
    VK_DEVICE                                  device,
    const VK_COMPUTE_PIPELINE_CREATE_INFO*     pCreateInfo,
    VK_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkStorePipeline(
    VK_PIPELINE                                pipeline,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkLoadPipeline(
    VK_DEVICE                                  device,
    size_t                                    	dataSize,
    const void*                                 pData,
    VK_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkLoadPipelineDerivative(
    VK_DEVICE                                  device,
    size_t                                    	dataSize,
    const void*                                 pData,
    VK_PIPELINE				basePipeline,
    VK_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateQueryPool(
    VK_DEVICE                                  device,
    const VK_QUERY_POOL_CREATE_INFO*           pCreateInfo,
    VK_QUERY_POOL*                             pQueryPool)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkGetQueryPoolResults(
    VK_QUERY_POOL                              queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkQueueAddMemReference(
    VK_QUEUE                                   queue,
    VK_GPU_MEMORY                              mem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkQueueRemoveMemReference(
    VK_QUEUE                                   queue,
    VK_GPU_MEMORY                              mem)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkQueueWaitIdle(
    VK_QUEUE                                   queue_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkQueueSubmit(
    VK_QUEUE                                   queue_,
    uint32_t                                    cmdBufferCount,
    const VK_CMD_BUFFER*                       pCmdBuffers,
    VK_FENCE                                   fence_)
{
    NULLDRV_LOG_FUNC;
   return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkOpenSharedSemaphore(
    VK_DEVICE                                  device,
    const VK_SEMAPHORE_OPEN_INFO*              pOpenInfo,
    VK_SEMAPHORE*                              pSemaphore)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateSemaphore(
    VK_DEVICE                                  device,
    const VK_SEMAPHORE_CREATE_INFO*            pCreateInfo,
    VK_SEMAPHORE*                              pSemaphore)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkQueueSignalSemaphore(
    VK_QUEUE                                   queue,
    VK_SEMAPHORE                               semaphore)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkQueueWaitSemaphore(
    VK_QUEUE                                   queue,
    VK_SEMAPHORE                               semaphore)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateSampler(
    VK_DEVICE                                  device,
    const VK_SAMPLER_CREATE_INFO*              pCreateInfo,
    VK_SAMPLER*                                pSampler)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_sampler_create(dev, pCreateInfo,
            (struct nulldrv_sampler **) pSampler);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateShader(
        VK_DEVICE                                  device,
        const VK_SHADER_CREATE_INFO*               pCreateInfo,
        VK_SHADER*                                 pShader)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return shader_create(dev, pCreateInfo, (struct nulldrv_shader **) pShader);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateDynamicViewportState(
    VK_DEVICE                                  device,
    const VK_DYNAMIC_VP_STATE_CREATE_INFO*       pCreateInfo,
    VK_DYNAMIC_VP_STATE_OBJECT*                  pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_viewport_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_vp **) pState);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateDynamicRasterState(
    VK_DEVICE                                  device,
    const VK_DYNAMIC_RS_STATE_CREATE_INFO*         pCreateInfo,
    VK_DYNAMIC_RS_STATE_OBJECT*                    pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_raster_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_rs **) pState);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateDynamicColorBlendState(
    VK_DEVICE                                  device,
    const VK_DYNAMIC_CB_STATE_CREATE_INFO*    pCreateInfo,
    VK_DYNAMIC_CB_STATE_OBJECT*               pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_blend_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_cb **) pState);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateDynamicDepthStencilState(
    VK_DEVICE                                  device,
    const VK_DYNAMIC_DS_STATE_CREATE_INFO*  pCreateInfo,
    VK_DYNAMIC_DS_STATE_OBJECT*             pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_ds_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_ds **) pState);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateBufferView(
    VK_DEVICE                                  device,
    const VkBufferViewCreateInfo*          pCreateInfo,
    VK_BUFFER_VIEW*                            pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_buf_view_create(dev, pCreateInfo,
            (struct nulldrv_buf_view **) pView);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateImageView(
    VK_DEVICE                                  device,
    const VK_IMAGE_VIEW_CREATE_INFO*           pCreateInfo,
    VK_IMAGE_VIEW*                             pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_img_view_create(dev, pCreateInfo,
            (struct nulldrv_img_view **) pView);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateColorAttachmentView(
    VK_DEVICE                                  device,
    const VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    VK_COLOR_ATTACHMENT_VIEW*                  pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_rt_view_create(dev, pCreateInfo,
            (struct nulldrv_rt_view **) pView);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateDepthStencilView(
    VK_DEVICE                                  device,
    const VK_DEPTH_STENCIL_VIEW_CREATE_INFO*   pCreateInfo,
    VK_DEPTH_STENCIL_VIEW*                     pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_ds_view_create(dev, pCreateInfo,
            (struct nulldrv_ds_view **) pView);

}

ICD_EXPORT VK_RESULT VKAPI vkCreateDescriptorSetLayout(
    VK_DEVICE                                   device,
    const VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pCreateInfo,
    VK_DESCRIPTOR_SET_LAYOUT*                   pSetLayout)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_layout_create(dev, pCreateInfo,
            (struct nulldrv_desc_layout **) pSetLayout);
}

ICD_EXPORT VK_RESULT VKAPI vkCreateDescriptorSetLayoutChain(
    VK_DEVICE                                   device,
    uint32_t                                     setLayoutArrayCount,
    const VK_DESCRIPTOR_SET_LAYOUT*             pSetLayoutArray,
    VK_DESCRIPTOR_SET_LAYOUT_CHAIN*             pLayoutChain)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_layout_chain_create(dev,
            setLayoutArrayCount, pSetLayoutArray,
            (struct nulldrv_desc_layout_chain **) pLayoutChain);
}

ICD_EXPORT VK_RESULT VKAPI vkBeginDescriptorPoolUpdate(
    VK_DEVICE                                   device,
    VK_DESCRIPTOR_UPDATE_MODE                   updateMode)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkEndDescriptorPoolUpdate(
    VK_DEVICE                                   device,
    VK_CMD_BUFFER                               cmd_)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateDescriptorPool(
    VK_DEVICE                                   device,
    VK_DESCRIPTOR_POOL_USAGE                  poolUsage,
    uint32_t                                     maxSets,
    const VK_DESCRIPTOR_POOL_CREATE_INFO*     pCreateInfo,
    VK_DESCRIPTOR_POOL*                       pDescriptorPool)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_pool_create(dev, poolUsage, maxSets, pCreateInfo,
            (struct nulldrv_desc_pool **) pDescriptorPool);
}

ICD_EXPORT VK_RESULT VKAPI vkResetDescriptorPool(
    VK_DESCRIPTOR_POOL                        descriptorPool)
{
    NULLDRV_LOG_FUNC;
    return VK_SUCCESS;
}

ICD_EXPORT VK_RESULT VKAPI vkAllocDescriptorSets(
    VK_DESCRIPTOR_POOL                        descriptorPool,
    VK_DESCRIPTOR_SET_USAGE                     setUsage,
    uint32_t                                     count,
    const VK_DESCRIPTOR_SET_LAYOUT*             pSetLayouts,
    VK_DESCRIPTOR_SET*                          pDescriptorSets,
    uint32_t*                                    pCount)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_desc_pool *pool = nulldrv_desc_pool(descriptorPool);
    struct nulldrv_dev *dev = pool->dev;
    VK_RESULT ret = VK_SUCCESS;
    uint32_t i;

    for (i = 0; i < count; i++) {
        const struct nulldrv_desc_layout *layout =
            nulldrv_desc_layout((VK_DESCRIPTOR_SET_LAYOUT) pSetLayouts[i]);

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
    VK_DESCRIPTOR_POOL                        descriptorPool,
    uint32_t                                     count,
    const VK_DESCRIPTOR_SET*                    pDescriptorSets)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkUpdateDescriptors(
    VK_DESCRIPTOR_SET                           descriptorSet,
    uint32_t                                     updateCount,
    const void**                                 ppUpdateArray)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT VK_RESULT VKAPI vkCreateFramebuffer(
    VK_DEVICE                                  device,
    const VK_FRAMEBUFFER_CREATE_INFO*          info,
    VK_FRAMEBUFFER*                            fb_ret)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_fb_create(dev, info, (struct nulldrv_framebuffer **) fb_ret);
}


ICD_EXPORT VK_RESULT VKAPI vkCreateRenderPass(
    VK_DEVICE                                  device,
    const VK_RENDER_PASS_CREATE_INFO*          info,
    VK_RENDER_PASS*                            rp_ret)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_render_pass_create(dev, info, (struct nulldrv_render_pass **) rp_ret);
}

ICD_EXPORT void VKAPI vkCmdBeginRenderPass(
    VK_CMD_BUFFER                              cmdBuffer,
    const VK_RENDER_PASS_BEGIN*                pRenderPassBegin)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void VKAPI vkCmdEndRenderPass(
    VK_CMD_BUFFER                              cmdBuffer,
    VK_RENDER_PASS                             renderPass)
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

ICD_EXPORT VK_RESULT xcbQueuePresent(void *queue, void *image, void* fence)
{
    return VK_SUCCESS;
}
