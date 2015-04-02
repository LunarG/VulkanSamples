/*
 * XGL null driver
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
	[NULLDRV_EXT_WSI_X11] = "XGL_WSI_X11",
	[NULLDRV_EXT_WSI_WINDOWS] = "XGL_WSI_WINDOWS"
};

static struct nulldrv_base *nulldrv_base(XGL_BASE_OBJECT base)
{
    return (struct nulldrv_base *) base;
}

static XGL_RESULT nulldrv_base_get_info(struct nulldrv_base *base, int type,
                               size_t *size, void *data)
{
    XGL_RESULT ret = XGL_SUCCESS;
    size_t s;
    uint32_t *count;

    switch (type) {
    case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            XGL_MEMORY_REQUIREMENTS *mem_req = data;
            s = sizeof(XGL_MEMORY_REQUIREMENTS);
            *size = s;
            if (data == NULL)
                return ret;
            memset(data, 0, s);
            mem_req->memType =  XGL_MEMORY_TYPE_OTHER;
            break;
        }
    case XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT:
        *size = sizeof(uint32_t);
        if (data == NULL)
            return ret;
        count = (uint32_t *) data;
        *count = 1;
        break;
    case XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS:
        s = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
        *size = s;
        if (data == NULL)
            return ret;
        memset(data, 0, s);
        break;
    case XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS:
        s = sizeof(XGL_BUFFER_MEMORY_REQUIREMENTS);
        *size = s;
        if (data == NULL)
            return ret;
        memset(data, 0, s);
        break;
    default:
        ret = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}

static struct nulldrv_base *nulldrv_base_create(struct nulldrv_dev *dev,
                                     size_t obj_size,
                                     XGL_DBG_OBJECT_TYPE type)
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

static XGL_RESULT nulldrv_gpu_add(int devid, const char *primary_node,
                         const char *render_node, struct nulldrv_gpu **gpu_ret)
{
    struct nulldrv_gpu *gpu;

	gpu = malloc(sizeof(*gpu));
    if (!gpu)
        return XGL_ERROR_OUT_OF_MEMORY;
	memset(gpu, 0, sizeof(*gpu));
    
    // Initialize pointer to loader's dispatch table with ICD_LOADER_MAGIC
    set_loader_magic_value(gpu);

    *gpu_ret = gpu;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_queue_create(struct nulldrv_dev *dev,
                              uint32_t node_index,
                              struct nulldrv_queue **queue_ret)
{
    struct nulldrv_queue *queue;

    queue = (struct nulldrv_queue *) nulldrv_base_create(dev, sizeof(*queue),
            XGL_DBG_OBJECT_QUEUE);
    if (!queue)
        return XGL_ERROR_OUT_OF_MEMORY;

    queue->dev = dev;

    *queue_ret = queue;

    return XGL_SUCCESS;
}

static XGL_RESULT dev_create_queues(struct nulldrv_dev *dev,
                                    const XGL_DEVICE_QUEUE_CREATE_INFO *queues,
                                    uint32_t count)
{
    uint32_t i;

    if (!count)
        return XGL_ERROR_INVALID_POINTER;

    for (i = 0; i < count; i++) {
        const XGL_DEVICE_QUEUE_CREATE_INFO *q = &queues[i];
        XGL_RESULT ret = XGL_SUCCESS;

        if (q->queueCount == 1 && !dev->queues[q->queueNodeIndex]) {
            ret = nulldrv_queue_create(dev, q->queueNodeIndex,
                    &dev->queues[q->queueNodeIndex]);
        }
        else {
            ret = XGL_ERROR_INVALID_POINTER;
        }

        if (ret != XGL_SUCCESS) {
            return ret;
        }
    }

    return XGL_SUCCESS;
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

static XGL_RESULT nulldrv_desc_ooxx_create(struct nulldrv_dev *dev,
                                  struct nulldrv_desc_ooxx **ooxx_ret)
{
    struct nulldrv_desc_ooxx *ooxx;

    ooxx = malloc(sizeof(*ooxx));
    if (!ooxx) 
        return XGL_ERROR_OUT_OF_MEMORY;

    memset(ooxx, 0, sizeof(*ooxx));

    ooxx->surface_desc_size = 0;
    ooxx->sampler_desc_size = 0;

    *ooxx_ret = ooxx; 

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_dev_create(struct nulldrv_gpu *gpu,
                            const XGL_DEVICE_CREATE_INFO *info,
                            struct nulldrv_dev **dev_ret)
{
    struct nulldrv_dev *dev;
    uint32_t i;
    XGL_RESULT ret;

    dev = (struct nulldrv_dev *) nulldrv_base_create(NULL, sizeof(*dev),
            XGL_DBG_OBJECT_DEVICE);
    if (!dev)
        return XGL_ERROR_OUT_OF_MEMORY;

    for (i = 0; i < info->extensionCount; i++) {
        const enum nulldrv_ext_type ext = nulldrv_gpu_lookup_extension(gpu,
                info->ppEnabledExtensionNames[i]);

        if (ext == NULLDRV_EXT_INVALID)
            return XGL_ERROR_INVALID_EXTENSION;

        dev->exts[ext] = true;
    }

    ret = nulldrv_desc_ooxx_create(dev, &dev->desc_ooxx);
    if (ret != XGL_SUCCESS) {
        return ret;
    }

    ret = dev_create_queues(dev, info->pRequestedQueues,
            info->queueRecordCount);
    if (ret != XGL_SUCCESS) {
        return ret;
    }

    *dev_ret = dev;

    return XGL_SUCCESS;
}

static struct nulldrv_gpu *nulldrv_gpu(XGL_PHYSICAL_GPU gpu)
{
    return (struct nulldrv_gpu *) gpu;
}

static XGL_RESULT nulldrv_rt_view_create(struct nulldrv_dev *dev,
                                const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO *info,
                                struct nulldrv_rt_view **view_ret)
{
    struct nulldrv_rt_view *view;

    view = (struct nulldrv_rt_view *) nulldrv_base_create(dev, sizeof(*view),
            XGL_DBG_OBJECT_COLOR_TARGET_VIEW);
    if (!view)
        return XGL_ERROR_OUT_OF_MEMORY;

    *view_ret = view;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_fence_create(struct nulldrv_dev *dev,
                              const XGL_FENCE_CREATE_INFO *info,
                              struct nulldrv_fence **fence_ret)
{
    struct nulldrv_fence *fence;

    fence = (struct nulldrv_fence *) nulldrv_base_create(dev, sizeof(*fence),
            XGL_DBG_OBJECT_FENCE);
    if (!fence)
        return XGL_ERROR_OUT_OF_MEMORY;

    *fence_ret = fence;

    return XGL_SUCCESS;
}

static struct nulldrv_dev *nulldrv_dev(XGL_DEVICE dev)
{
    return (struct nulldrv_dev *) dev;
}

static struct nulldrv_img *nulldrv_img_from_base(struct nulldrv_base *base)
{
    return (struct nulldrv_img *) base;
}


static XGL_RESULT img_get_info(struct nulldrv_base *base, int type,
                               size_t *size, void *data)
{
    struct nulldrv_img *img = nulldrv_img_from_base(base);
    XGL_RESULT ret = XGL_SUCCESS;

    switch (type) {
    case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            XGL_MEMORY_REQUIREMENTS *mem_req = data;

            *size = sizeof(XGL_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;
            mem_req->size = img->total_size;
            mem_req->alignment = 4096;
            mem_req->memType = XGL_MEMORY_TYPE_IMAGE;
        }
        break;
    case XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS:
        {
            XGL_IMAGE_MEMORY_REQUIREMENTS *img_req = data;

            *size = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;
            img_req->usage = img->usage;
            img_req->formatClass = img->format_class;
            img_req->samples = img->samples;
        }
        break;
    case XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS:
        {
            XGL_BUFFER_MEMORY_REQUIREMENTS *buf_req = data;

            *size = sizeof(XGL_BUFFER_MEMORY_REQUIREMENTS);
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

static XGL_RESULT nulldrv_img_create(struct nulldrv_dev *dev,
                            const XGL_IMAGE_CREATE_INFO *info,
                            bool scanout,
                            struct nulldrv_img **img_ret)
{
    struct nulldrv_img *img;

    img = (struct nulldrv_img *) nulldrv_base_create(dev, sizeof(*img),
            XGL_DBG_OBJECT_IMAGE);
    if (!img)
        return XGL_ERROR_OUT_OF_MEMORY;

    img->type = info->imageType;
    img->depth = info->extent.depth;
    img->mip_levels = info->mipLevels;
    img->array_size = info->arraySize;
    img->usage = info->usage;
    if (info->tiling == XGL_LINEAR_TILING)
        img->format_class = XGL_IMAGE_FORMAT_CLASS_LINEAR;
    else
        img->format_class = icd_format_get_class(info->format);
    img->samples = info->samples;

    img->obj.base.get_info = img_get_info;

    *img_ret = img;

    return XGL_SUCCESS;
}

static struct nulldrv_img *nulldrv_img(XGL_IMAGE image)
{
    return (struct nulldrv_img *) image;
}

static XGL_RESULT nulldrv_mem_alloc(struct nulldrv_dev *dev,
                           const XGL_MEMORY_ALLOC_INFO *info,
                           struct nulldrv_mem **mem_ret)
{
    struct nulldrv_mem *mem;

    mem = (struct nulldrv_mem *) nulldrv_base_create(dev, sizeof(*mem),
            XGL_DBG_OBJECT_GPU_MEMORY);
    if (!mem)
        return XGL_ERROR_OUT_OF_MEMORY;

    mem->bo = malloc(info->allocationSize);
    if (!mem->bo) {
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    mem->size = info->allocationSize;

    *mem_ret = mem;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_ds_view_create(struct nulldrv_dev *dev,
                                const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO *info,
                                struct nulldrv_ds_view **view_ret)
{
    struct nulldrv_img *img = nulldrv_img(info->image);
    struct nulldrv_ds_view *view;

    view = (struct nulldrv_ds_view *) nulldrv_base_create(dev, sizeof(*view),
            XGL_DBG_OBJECT_DEPTH_STENCIL_VIEW);
    if (!view)
        return XGL_ERROR_OUT_OF_MEMORY;

    view->img = img;

    view->array_size = info->arraySize;

    *view_ret = view;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_sampler_create(struct nulldrv_dev *dev,
                                const XGL_SAMPLER_CREATE_INFO *info,
                                struct nulldrv_sampler **sampler_ret)
{
    struct nulldrv_sampler *sampler;

    sampler = (struct nulldrv_sampler *) nulldrv_base_create(dev,
            sizeof(*sampler), XGL_DBG_OBJECT_SAMPLER);
    if (!sampler)
        return XGL_ERROR_OUT_OF_MEMORY;

    *sampler_ret = sampler;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_img_view_create(struct nulldrv_dev *dev,
                                 const XGL_IMAGE_VIEW_CREATE_INFO *info,
                                 struct nulldrv_img_view **view_ret)
{
    struct nulldrv_img *img = nulldrv_img(info->image);
    struct nulldrv_img_view *view;

    view = (struct nulldrv_img_view *) nulldrv_base_create(dev, sizeof(*view),
            XGL_DBG_OBJECT_IMAGE_VIEW);
    if (!view)
        return XGL_ERROR_OUT_OF_MEMORY;

    view->img = img;
    view->min_lod = info->minLod;

    view->cmd_len = 8;

    *view_ret = view;

    return XGL_SUCCESS;
}

static void *nulldrv_mem_map(struct nulldrv_mem *mem, XGL_FLAGS flags)
{
    return mem->bo;
}

static struct nulldrv_mem *nulldrv_mem(XGL_GPU_MEMORY mem)
{
    return (struct nulldrv_mem *) mem;
}

static struct nulldrv_buf *nulldrv_buf_from_base(struct nulldrv_base *base)
{
    return (struct nulldrv_buf *) base;
}

static XGL_RESULT buf_get_info(struct nulldrv_base *base, int type,
                               size_t *size, void *data)
{
    struct nulldrv_buf *buf = nulldrv_buf_from_base(base);
    XGL_RESULT ret = XGL_SUCCESS;

    switch (type) {
    case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            XGL_MEMORY_REQUIREMENTS *mem_req = data;

            *size = sizeof(XGL_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;

            mem_req->size = buf->size;
            mem_req->alignment = 4096;
            mem_req->memType = XGL_MEMORY_TYPE_BUFFER;

        }
        break;
        case XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS:
        {
            XGL_BUFFER_MEMORY_REQUIREMENTS *buf_req = data;

            *size = sizeof(XGL_BUFFER_MEMORY_REQUIREMENTS);
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

static XGL_RESULT nulldrv_buf_create(struct nulldrv_dev *dev,
                            const XGL_BUFFER_CREATE_INFO *info,
                            struct nulldrv_buf **buf_ret)
{
    struct nulldrv_buf *buf;

    buf = (struct nulldrv_buf *) nulldrv_base_create(dev, sizeof(*buf),
            XGL_DBG_OBJECT_BUFFER);
    if (!buf)
        return XGL_ERROR_OUT_OF_MEMORY;

    buf->size = info->size;
    buf->usage = info->usage;

    buf->obj.base.get_info = buf_get_info;

    *buf_ret = buf;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_desc_layout_create(struct nulldrv_dev *dev,
                                    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO *info,
                                    struct nulldrv_desc_layout **layout_ret)
{
    struct nulldrv_desc_layout *layout;

    layout = (struct nulldrv_desc_layout *)
        nulldrv_base_create(dev, sizeof(*layout),
                XGL_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT);
    if (!layout)
        return XGL_ERROR_OUT_OF_MEMORY;

    *layout_ret = layout;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_desc_layout_chain_create(struct nulldrv_dev *dev,
                                    uint32_t setLayoutArrayCount,
                                    const XGL_DESCRIPTOR_SET_LAYOUT *pSetLayoutArray,
                                    struct nulldrv_desc_layout_chain **chain_ret)
{
    struct nulldrv_desc_layout_chain *chain;

    chain = (struct nulldrv_desc_layout_chain *)
        nulldrv_base_create(dev, sizeof(*chain),
                XGL_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT_CHAIN);
    if (!chain)
        return XGL_ERROR_OUT_OF_MEMORY;

    *chain_ret = chain;

    return XGL_SUCCESS;
}

static struct nulldrv_desc_layout *nulldrv_desc_layout(XGL_DESCRIPTOR_SET_LAYOUT layout)
{
    return (struct nulldrv_desc_layout *) layout;
}

static XGL_RESULT shader_create(struct nulldrv_dev *dev,
                                const XGL_SHADER_CREATE_INFO *info,
                                struct nulldrv_shader **sh_ret)
{
    struct nulldrv_shader *sh;

    sh = (struct nulldrv_shader *) nulldrv_base_create(dev, sizeof(*sh),
            XGL_DBG_OBJECT_SHADER);
    if (!sh)
        return XGL_ERROR_OUT_OF_MEMORY;

    *sh_ret = sh;

    return XGL_SUCCESS;
}

static XGL_RESULT graphics_pipeline_create(struct nulldrv_dev *dev,
                                           const XGL_GRAPHICS_PIPELINE_CREATE_INFO *info_,
                                           struct nulldrv_pipeline **pipeline_ret)
{
    struct nulldrv_pipeline *pipeline;

    pipeline = (struct nulldrv_pipeline *)
        nulldrv_base_create(dev, sizeof(*pipeline), 
                XGL_DBG_OBJECT_GRAPHICS_PIPELINE);
    if (!pipeline)
        return XGL_ERROR_OUT_OF_MEMORY;

    *pipeline_ret = pipeline;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_viewport_state_create(struct nulldrv_dev *dev,
                                       const XGL_DYNAMIC_VP_STATE_CREATE_INFO *info,
                                       struct nulldrv_dynamic_vp **state_ret)
{
    struct nulldrv_dynamic_vp *state;

    state = (struct nulldrv_dynamic_vp *) nulldrv_base_create(dev,
            sizeof(*state), XGL_DBG_OBJECT_VIEWPORT_STATE);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    *state_ret = state;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_raster_state_create(struct nulldrv_dev *dev,
                                     const XGL_DYNAMIC_RS_STATE_CREATE_INFO *info,
                                     struct nulldrv_dynamic_rs **state_ret)
{
    struct nulldrv_dynamic_rs *state;

    state = (struct nulldrv_dynamic_rs *) nulldrv_base_create(dev,
            sizeof(*state), XGL_DBG_OBJECT_RASTER_STATE);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    *state_ret = state;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_blend_state_create(struct nulldrv_dev *dev,
                                    const XGL_DYNAMIC_CB_STATE_CREATE_INFO *info,
                                    struct nulldrv_dynamic_cb **state_ret)
{
    struct nulldrv_dynamic_cb *state;

    state = (struct nulldrv_dynamic_cb *) nulldrv_base_create(dev,
            sizeof(*state), XGL_DBG_OBJECT_COLOR_BLEND_STATE);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    *state_ret = state;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_ds_state_create(struct nulldrv_dev *dev,
                                 const XGL_DYNAMIC_DS_STATE_CREATE_INFO *info,
                                 struct nulldrv_dynamic_ds **state_ret)
{
    struct nulldrv_dynamic_ds *state;

    state = (struct nulldrv_dynamic_ds *) nulldrv_base_create(dev,
            sizeof(*state), XGL_DBG_OBJECT_DEPTH_STENCIL_STATE);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    *state_ret = state;

    return XGL_SUCCESS;
}


static XGL_RESULT nulldrv_cmd_create(struct nulldrv_dev *dev,
                            const XGL_CMD_BUFFER_CREATE_INFO *info,
                            struct nulldrv_cmd **cmd_ret)
{
    struct nulldrv_cmd *cmd;

    cmd = (struct nulldrv_cmd *) nulldrv_base_create(dev, sizeof(*cmd),
            XGL_DBG_OBJECT_CMD_BUFFER);
    if (!cmd)
        return XGL_ERROR_OUT_OF_MEMORY;

    *cmd_ret = cmd;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_desc_pool_create(struct nulldrv_dev *dev,
                                    XGL_DESCRIPTOR_POOL_USAGE usage,
                                    uint32_t max_sets,
                                    const XGL_DESCRIPTOR_POOL_CREATE_INFO *info,
                                    struct nulldrv_desc_pool **pool_ret)
{
    struct nulldrv_desc_pool *pool;

    pool = (struct nulldrv_desc_pool *)
        nulldrv_base_create(dev, sizeof(*pool),
                XGL_DBG_OBJECT_DESCRIPTOR_POOL);
    if (!pool)
        return XGL_ERROR_OUT_OF_MEMORY;

    pool->dev = dev;

    *pool_ret = pool;

    return XGL_SUCCESS;
}

static XGL_RESULT nulldrv_desc_set_create(struct nulldrv_dev *dev,
                                 struct nulldrv_desc_pool *pool,
                                 XGL_DESCRIPTOR_SET_USAGE usage,
                                 const struct nulldrv_desc_layout *layout,
                                 struct nulldrv_desc_set **set_ret)
{
    struct nulldrv_desc_set *set;

    set = (struct nulldrv_desc_set *)
        nulldrv_base_create(dev, sizeof(*set), 
                XGL_DBG_OBJECT_DESCRIPTOR_SET);
    if (!set)
        return XGL_ERROR_OUT_OF_MEMORY;

    set->ooxx = dev->desc_ooxx;
    set->layout = layout;
    *set_ret = set;

    return XGL_SUCCESS;
}

static struct nulldrv_desc_pool *nulldrv_desc_pool(XGL_DESCRIPTOR_POOL pool)
{
    return (struct nulldrv_desc_pool *) pool;
}

static XGL_RESULT nulldrv_fb_create(struct nulldrv_dev *dev,
                           const XGL_FRAMEBUFFER_CREATE_INFO* info,
                           struct nulldrv_framebuffer ** fb_ret)
{

    struct nulldrv_framebuffer *fb;
    fb = (struct nulldrv_framebuffer *) nulldrv_base_create(dev, sizeof(*fb),
            XGL_DBG_OBJECT_FRAMEBUFFER);
    if (!fb)
        return XGL_ERROR_OUT_OF_MEMORY;

    *fb_ret = fb;

    return XGL_SUCCESS;

}

static XGL_RESULT nulldrv_render_pass_create(struct nulldrv_dev *dev,
                           const XGL_RENDER_PASS_CREATE_INFO* info,
                           struct nulldrv_render_pass** rp_ret)
{
    struct nulldrv_render_pass *rp;
    rp = (struct nulldrv_render_pass *) nulldrv_base_create(dev, sizeof(*rp),
            XGL_DBG_OBJECT_RENDER_PASS);
    if (!rp)
        return XGL_ERROR_OUT_OF_MEMORY;

    *rp_ret = rp;

    return XGL_SUCCESS;
}

static struct nulldrv_buf *nulldrv_buf(XGL_BUFFER buf)
{
    return (struct nulldrv_buf *) buf;
}

static XGL_RESULT nulldrv_buf_view_create(struct nulldrv_dev *dev,
                                 const XGL_BUFFER_VIEW_CREATE_INFO *info,
                                 struct nulldrv_buf_view **view_ret)
{
    struct nulldrv_buf *buf = nulldrv_buf(info->buffer);
    struct nulldrv_buf_view *view;

    view = (struct nulldrv_buf_view *) nulldrv_base_create(dev, sizeof(*view),
            XGL_DBG_OBJECT_BUFFER_VIEW);
    if (!view)
        return XGL_ERROR_OUT_OF_MEMORY;

    view->buf = buf;

    *view_ret = view;

    return XGL_SUCCESS;
}


//*********************************************
// Driver entry points
//*********************************************

ICD_EXPORT XGL_RESULT XGLAPI xglCreateBuffer(
    XGL_DEVICE                                  device,
    const XGL_BUFFER_CREATE_INFO*               pCreateInfo,
    XGL_BUFFER*                                 pBuffer)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_buf_create(dev, pCreateInfo, (struct nulldrv_buf **) pBuffer);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateCommandBuffer(
    XGL_DEVICE                                  device,
    const XGL_CMD_BUFFER_CREATE_INFO*           pCreateInfo,
    XGL_CMD_BUFFER*                             pCmdBuffer)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_cmd_create(dev, pCreateInfo,
            (struct nulldrv_cmd **) pCmdBuffer);
}

ICD_EXPORT XGL_RESULT XGLAPI xglBeginCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_CMD_BUFFER_BEGIN_INFO            *info)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglEndCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglResetCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT void XGLAPI xglCmdInitAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    const uint32_t*                             pData)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdLoadAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    XGL_BUFFER                                  srcBuffer,
    XGL_GPU_SIZE                                srcOffset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdSaveAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdDbgMarkerBegin(
    XGL_CMD_BUFFER                              cmdBuffer,
    const char*                                 pMarker)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdDbgMarkerEnd(
    XGL_CMD_BUFFER                              cmdBuffer)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdCopyBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  srcBuffer,
    XGL_BUFFER                                  destBuffer,
    uint32_t                                    regionCount,
    const XGL_BUFFER_COPY*                      pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdCopyImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_LAYOUT                            srcImageLayout,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_LAYOUT                            destImageLayout,
    uint32_t                                    regionCount,
    const XGL_IMAGE_COPY*                       pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdBlitImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_LAYOUT                            srcImageLayout,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_LAYOUT                            destImageLayout,
    uint32_t                                    regionCount,
    const XGL_IMAGE_BLIT*                       pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdCopyBufferToImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  srcBuffer,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_LAYOUT                            destImageLayout,
    uint32_t                                    regionCount,
    const XGL_BUFFER_IMAGE_COPY*                pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdCopyImageToBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_LAYOUT                            srcImageLayout,
    XGL_BUFFER                                  destBuffer,
    uint32_t                                    regionCount,
    const XGL_BUFFER_IMAGE_COPY*                pRegions)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdCloneImageData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_LAYOUT                            srcImageLayout,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_LAYOUT                            destImageLayout)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdUpdateBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                dataSize,
    const uint32_t*                             pData)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdFillBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                fillSize,
    uint32_t                                    data)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdClearColorImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    XGL_IMAGE_LAYOUT                            imageLayout,
    XGL_CLEAR_COLOR                             color,
    uint32_t                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdClearDepthStencil(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    XGL_IMAGE_LAYOUT                            imageLayout,
    float                                       depth,
    uint32_t                                    stencil,
    uint32_t                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdResolveImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_LAYOUT                            srcImageLayout,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_LAYOUT                            destImageLayout,
    uint32_t                                    rectCount,
    const XGL_IMAGE_RESOLVE*                    pRects)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdBeginQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    uint32_t                                    slot,
    XGL_FLAGS                                   flags)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdEndQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    uint32_t                                    slot)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdResetQueryPool(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdSetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event_,
    XGL_PIPE_EVENT                              pipeEvent)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdResetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event_,
    XGL_PIPE_EVENT                              pipeEvent)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdWriteTimestamp(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_TIMESTAMP_TYPE                          timestampType,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdBindPipeline(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE                                pipeline)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdBindDynamicStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_DYNAMIC_STATE_OBJECT                    state)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdBindDescriptorSets(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_DESCRIPTOR_SET_LAYOUT_CHAIN             layoutChain,
    uint32_t                                    layoutChainSlot,
    uint32_t                                    count,
    const XGL_DESCRIPTOR_SET*                   pDescriptorSets,
    const uint32_t*                             pUserData)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdBindVertexBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    uint32_t                                    binding)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdBindIndexBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdDraw(
    XGL_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    firstVertex,
    uint32_t                                    vertexCount,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdDrawIndexed(
    XGL_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    firstIndex,
    uint32_t                                    indexCount,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdDrawIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    uint32_t                                    count,
    uint32_t                                    stride)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    uint32_t                                    count,
    uint32_t                                    stride)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdDispatch(
    XGL_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdDispatchIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdWaitEvents(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_EVENT_WAIT_INFO*                  pWaitInfo)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdPipelineBarrier(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_PIPELINE_BARRIER*                 pBarrier)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDevice(
    XGL_PHYSICAL_GPU                            gpu_,
    const XGL_DEVICE_CREATE_INFO*               pCreateInfo,
    XGL_DEVICE*                                 pDevice)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_gpu *gpu = nulldrv_gpu(gpu_);
    return nulldrv_dev_create(gpu, pCreateInfo, (struct nulldrv_dev**)pDevice);
}

ICD_EXPORT XGL_RESULT XGLAPI xglDestroyDevice(
    XGL_DEVICE                                  device)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetDeviceQueue(
    XGL_DEVICE                                  device,
    uint32_t                                    queueNodeIndex,
    uint32_t                                    queueIndex,
    XGL_QUEUE*                                  pQueue)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);
    *pQueue = dev->queues[0];
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDeviceWaitIdle(
    XGL_DEVICE                                  device)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgSetValidationLevel(
    XGL_DEVICE                                  device,
    XGL_VALIDATION_LEVEL                        validationLevel)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgSetMessageFilter(
    XGL_DEVICE                                  device,
    int32_t                                     msgCode,
    XGL_DBG_MSG_FILTER                          filter)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgSetDeviceOption(
    XGL_DEVICE                                  device,
    XGL_DBG_DEVICE_OPTION                       dbgOption,
    size_t                                      dataSize,
    const void*                                 pData)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateEvent(
    XGL_DEVICE                                  device,
    const XGL_EVENT_CREATE_INFO*                pCreateInfo,
    XGL_EVENT*                                  pEvent)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetEventStatus(
    XGL_EVENT                                   event_)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglSetEvent(
    XGL_EVENT                                   event_)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglResetEvent(
    XGL_EVENT                                   event_)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateFence(
    XGL_DEVICE                                  device,
    const XGL_FENCE_CREATE_INFO*                pCreateInfo,
    XGL_FENCE*                                  pFence)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_fence_create(dev, pCreateInfo,
            (struct nulldrv_fence **) pFence);
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetFenceStatus(
    XGL_FENCE                                   fence_)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglWaitForFences(
    XGL_DEVICE                                  device,
    uint32_t                                    fenceCount,
    const XGL_FENCE*                            pFences,
    bool32_t                                    waitAll,
    uint64_t                                    timeout)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetFormatInfo(
    XGL_DEVICE                                  device,
    XGL_FORMAT                                  format,
    XGL_FORMAT_INFO_TYPE                        infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetGpuInfo(
    XGL_PHYSICAL_GPU                            gpu_,
    XGL_PHYSICAL_GPU_INFO_TYPE                  infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetExtensionSupport(
    XGL_PHYSICAL_GPU                            gpu_,
    const char*                                 pExtName)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(
    XGL_PHYSICAL_GPU                            gpu0_,
    XGL_PHYSICAL_GPU                            gpu1_,
    XGL_GPU_COMPATIBILITY_INFO*                 pInfo)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglOpenPeerImage(
    XGL_DEVICE                                  device,
    const XGL_PEER_IMAGE_OPEN_INFO*             pOpenInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateImage(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
    XGL_IMAGE*                                  pImage)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_img_create(dev, pCreateInfo, false,
            (struct nulldrv_img **) pImage);
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetImageSubresourceInfo(
    XGL_IMAGE                                   image,
    const XGL_IMAGE_SUBRESOURCE*                pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE                   infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    XGL_RESULT ret = XGL_SUCCESS;

    switch (infoType) {
    case XGL_INFO_TYPE_SUBRESOURCE_LAYOUT:
        {
            XGL_SUBRESOURCE_LAYOUT *layout = (XGL_SUBRESOURCE_LAYOUT *) pData;

            *pDataSize = sizeof(XGL_SUBRESOURCE_LAYOUT);

            if (pData == NULL)
                return ret;
            layout->offset = 0;
            layout->size = 1;
            layout->rowPitch = 4;
            layout->depthPitch = 4;
        }
        break;
    default:
        ret = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}

ICD_EXPORT XGL_RESULT XGLAPI xglAllocMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_mem_alloc(dev, pAllocInfo, (struct nulldrv_mem **) pMem);
}

ICD_EXPORT XGL_RESULT XGLAPI xglFreeMemory(
    XGL_GPU_MEMORY                              mem_)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglSetMemoryPriority(
    XGL_GPU_MEMORY                              mem_,
    XGL_MEMORY_PRIORITY                         priority)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglMapMemory(
    XGL_GPU_MEMORY                              mem_,
    XGL_FLAGS                                   flags,
    void**                                      ppData)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_mem *mem = nulldrv_mem(mem_);
    void *ptr = nulldrv_mem_map(mem, flags);

    *ppData = ptr;

    return (ptr) ? XGL_SUCCESS : XGL_ERROR_UNKNOWN;
}

ICD_EXPORT XGL_RESULT XGLAPI xglUnmapMemory(
    XGL_GPU_MEMORY                              mem_)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglPinSystemMemory(
    XGL_DEVICE                                  device,
    const void*                                 pSysMem,
    size_t                                      memSize,
    XGL_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglOpenSharedMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_OPEN_INFO*                 pOpenInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglOpenPeerMemory(
    XGL_DEVICE                                  device,
    const XGL_PEER_MEMORY_OPEN_INFO*            pOpenInfo,
    XGL_GPU_MEMORY*                             pMem)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateInstance(
    const XGL_APPLICATION_INFO*                 pAppInfo,
    const XGL_ALLOC_CALLBACKS*                  pAllocCb,
    XGL_INSTANCE*                               pInstance)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_instance *inst;

    inst = (struct nulldrv_instance *) nulldrv_base_create(NULL, sizeof(*inst),
                XGL_DBG_OBJECT_INSTANCE);
    if (!inst)
        return XGL_ERROR_OUT_OF_MEMORY;

    inst->obj.base.get_info = NULL;

    *pInstance = (XGL_INSTANCE*)inst;

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDestroyInstance(
    XGL_INSTANCE                                pInstance)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglEnumerateGpus(
    XGL_INSTANCE                                instance,
    uint32_t                                    maxGpus,
    uint32_t*                                   pGpuCount,
    XGL_PHYSICAL_GPU*                           pGpus)
{
    NULLDRV_LOG_FUNC;
    XGL_RESULT ret;
    struct nulldrv_gpu *gpu;
    *pGpuCount = 1;
    ret = nulldrv_gpu_add(0, 0, 0, &gpu);
    if (ret == XGL_SUCCESS)
        pGpus[0] = (XGL_PHYSICAL_GPU) gpu;
    return ret;
}

ICD_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(
    XGL_PHYSICAL_GPU                            gpu,
    size_t                                      maxLayerCount,
    size_t                                      maxStringSize,
    size_t*                                     pOutLayerCount,
    char* const*                                pOutLayers,
    void*                                       pReserved)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(
    XGL_INSTANCE                                instance,
    XGL_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback,
    void*                                       pUserData)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(
    XGL_INSTANCE                                instance,
    XGL_DBG_MSG_CALLBACK_FUNCTION               pfnMsgCallback)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(
    XGL_INSTANCE                                instance,
    XGL_DBG_GLOBAL_OPTION                       dbgOption,
    size_t                                      dataSize,
    const void*                                 pData)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDestroyObject(
    XGL_OBJECT                                  object)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetObjectInfo(
    XGL_BASE_OBJECT                             object,
    XGL_OBJECT_INFO_TYPE                        infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_base *base = nulldrv_base(object);

    return base->get_info(base, infoType, pDataSize, pData);
}

ICD_EXPORT XGL_RESULT XGLAPI xglBindObjectMemory(
    XGL_OBJECT                                  object,
    uint32_t                                    allocationIdx,
    XGL_GPU_MEMORY                              mem_,
    XGL_GPU_SIZE                                memOffset)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglBindObjectMemoryRange(
    XGL_OBJECT                                  object,
    uint32_t                                    allocationIdx,
    XGL_GPU_SIZE                                rangeOffset,
    XGL_GPU_SIZE                                rangeSize,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                memOffset)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglBindImageMemoryRange(
    XGL_IMAGE                                   image,
    uint32_t                                    allocationIdx,
    const XGL_IMAGE_MEMORY_BIND_INFO*           bindInfo,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                memOffset)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgSetObjectTag(
    XGL_BASE_OBJECT                             object,
    size_t                                      tagSize,
    const void*                                 pTag)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateGraphicsPipeline(
    XGL_DEVICE                                  device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    XGL_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return graphics_pipeline_create(dev, pCreateInfo,
            (struct nulldrv_pipeline **) pPipeline);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateGraphicsPipelineDerivative(
    XGL_DEVICE                                  device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    XGL_PIPELINE                                basePipeline,
    XGL_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return graphics_pipeline_create(dev, pCreateInfo,
            (struct nulldrv_pipeline **) pPipeline);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateComputePipeline(
    XGL_DEVICE                                  device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO*     pCreateInfo,
    XGL_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglStorePipeline(
    XGL_PIPELINE                                pipeline,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglLoadPipeline(
    XGL_DEVICE                                  device,
    size_t                                    	dataSize,
    const void*                                 pData,
    XGL_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglLoadPipelineDerivative(
    XGL_DEVICE                                  device,
    size_t                                    	dataSize,
    const void*                                 pData,
    XGL_PIPELINE				basePipeline,
    XGL_PIPELINE*                               pPipeline)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateQueryPool(
    XGL_DEVICE                                  device,
    const XGL_QUERY_POOL_CREATE_INFO*           pCreateInfo,
    XGL_QUERY_POOL*                             pQueryPool)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetQueryPoolResults(
    XGL_QUERY_POOL                              queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(
    XGL_QUEUE                                   queue,
    uint32_t                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueAddMemReference(
    XGL_QUEUE                                   queue,
    XGL_GPU_MEMORY                              mem)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueRemoveMemReference(
    XGL_QUEUE                                   queue,
    XGL_GPU_MEMORY                              mem)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueWaitIdle(
    XGL_QUEUE                                   queue_)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(
    XGL_QUEUE                                   queue_,
    uint32_t                                    cmdBufferCount,
    const XGL_CMD_BUFFER*                       pCmdBuffers,
    uint32_t                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs,
    XGL_FENCE                                   fence_)
{
    NULLDRV_LOG_FUNC;
   return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglOpenSharedSemaphore(
    XGL_DEVICE                                  device,
    const XGL_SEMAPHORE_OPEN_INFO*              pOpenInfo,
    XGL_SEMAPHORE*                              pSemaphore)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateSemaphore(
    XGL_DEVICE                                  device,
    const XGL_SEMAPHORE_CREATE_INFO*            pCreateInfo,
    XGL_SEMAPHORE*                              pSemaphore)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueSignalSemaphore(
    XGL_QUEUE                                   queue,
    XGL_SEMAPHORE                               semaphore)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueWaitSemaphore(
    XGL_QUEUE                                   queue,
    XGL_SEMAPHORE                               semaphore)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateSampler(
    XGL_DEVICE                                  device,
    const XGL_SAMPLER_CREATE_INFO*              pCreateInfo,
    XGL_SAMPLER*                                pSampler)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_sampler_create(dev, pCreateInfo,
            (struct nulldrv_sampler **) pSampler);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateShader(
        XGL_DEVICE                                  device,
        const XGL_SHADER_CREATE_INFO*               pCreateInfo,
        XGL_SHADER*                                 pShader)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return shader_create(dev, pCreateInfo, (struct nulldrv_shader **) pShader);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDynamicViewportState(
    XGL_DEVICE                                  device,
    const XGL_DYNAMIC_VP_STATE_CREATE_INFO*       pCreateInfo,
    XGL_DYNAMIC_VP_STATE_OBJECT*                  pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_viewport_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_vp **) pState);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDynamicRasterState(
    XGL_DEVICE                                  device,
    const XGL_DYNAMIC_RS_STATE_CREATE_INFO*         pCreateInfo,
    XGL_DYNAMIC_RS_STATE_OBJECT*                    pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_raster_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_rs **) pState);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDynamicColorBlendState(
    XGL_DEVICE                                  device,
    const XGL_DYNAMIC_CB_STATE_CREATE_INFO*    pCreateInfo,
    XGL_DYNAMIC_CB_STATE_OBJECT*               pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_blend_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_cb **) pState);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDynamicDepthStencilState(
    XGL_DEVICE                                  device,
    const XGL_DYNAMIC_DS_STATE_CREATE_INFO*  pCreateInfo,
    XGL_DYNAMIC_DS_STATE_OBJECT*             pState)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_ds_state_create(dev, pCreateInfo,
            (struct nulldrv_dynamic_ds **) pState);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateBufferView(
    XGL_DEVICE                                  device,
    const XGL_BUFFER_VIEW_CREATE_INFO*          pCreateInfo,
    XGL_BUFFER_VIEW*                            pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_buf_view_create(dev, pCreateInfo,
            (struct nulldrv_buf_view **) pView);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateImageView(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_VIEW_CREATE_INFO*           pCreateInfo,
    XGL_IMAGE_VIEW*                             pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_img_view_create(dev, pCreateInfo,
            (struct nulldrv_img_view **) pView);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateColorAttachmentView(
    XGL_DEVICE                                  device,
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW*                  pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_rt_view_create(dev, pCreateInfo,
            (struct nulldrv_rt_view **) pView);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDepthStencilView(
    XGL_DEVICE                                  device,
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*   pCreateInfo,
    XGL_DEPTH_STENCIL_VIEW*                     pView)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_ds_view_create(dev, pCreateInfo,
            (struct nulldrv_ds_view **) pView);

}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayout(
    XGL_DEVICE                                   device,
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pCreateInfo,
    XGL_DESCRIPTOR_SET_LAYOUT*                   pSetLayout)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_layout_create(dev, pCreateInfo,
            (struct nulldrv_desc_layout **) pSetLayout);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayoutChain(
    XGL_DEVICE                                   device,
    uint32_t                                     setLayoutArrayCount,
    const XGL_DESCRIPTOR_SET_LAYOUT*             pSetLayoutArray,
    XGL_DESCRIPTOR_SET_LAYOUT_CHAIN*             pLayoutChain)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_layout_chain_create(dev,
            setLayoutArrayCount, pSetLayoutArray,
            (struct nulldrv_desc_layout_chain **) pLayoutChain);
}

ICD_EXPORT XGL_RESULT XGLAPI xglBeginDescriptorPoolUpdate(
    XGL_DEVICE                                   device,
    XGL_DESCRIPTOR_UPDATE_MODE                   updateMode)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglEndDescriptorPoolUpdate(
    XGL_DEVICE                                   device,
    XGL_CMD_BUFFER                               cmd_)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorPool(
    XGL_DEVICE                                   device,
    XGL_DESCRIPTOR_POOL_USAGE                  poolUsage,
    uint32_t                                     maxSets,
    const XGL_DESCRIPTOR_POOL_CREATE_INFO*     pCreateInfo,
    XGL_DESCRIPTOR_POOL*                       pDescriptorPool)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_desc_pool_create(dev, poolUsage, maxSets, pCreateInfo,
            (struct nulldrv_desc_pool **) pDescriptorPool);
}

ICD_EXPORT XGL_RESULT XGLAPI xglResetDescriptorPool(
    XGL_DESCRIPTOR_POOL                        descriptorPool)
{
    NULLDRV_LOG_FUNC;
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglAllocDescriptorSets(
    XGL_DESCRIPTOR_POOL                        descriptorPool,
    XGL_DESCRIPTOR_SET_USAGE                     setUsage,
    uint32_t                                     count,
    const XGL_DESCRIPTOR_SET_LAYOUT*             pSetLayouts,
    XGL_DESCRIPTOR_SET*                          pDescriptorSets,
    uint32_t*                                    pCount)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_desc_pool *pool = nulldrv_desc_pool(descriptorPool);
    struct nulldrv_dev *dev = pool->dev;
    XGL_RESULT ret = XGL_SUCCESS;
    uint32_t i;

    for (i = 0; i < count; i++) {
        const struct nulldrv_desc_layout *layout =
            nulldrv_desc_layout((XGL_DESCRIPTOR_SET_LAYOUT) pSetLayouts[i]);

        ret = nulldrv_desc_set_create(dev, pool, setUsage, layout,
                (struct nulldrv_desc_set **) &pDescriptorSets[i]);
        if (ret != XGL_SUCCESS)
            break;
    }

    if (pCount)
        *pCount = i;

    return ret;
}

ICD_EXPORT void XGLAPI xglClearDescriptorSets(
    XGL_DESCRIPTOR_POOL                        descriptorPool,
    uint32_t                                     count,
    const XGL_DESCRIPTOR_SET*                    pDescriptorSets)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglUpdateDescriptors(
    XGL_DESCRIPTOR_SET                           descriptorSet,
    uint32_t                                     updateCount,
    const void**                                 ppUpdateArray)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateFramebuffer(
    XGL_DEVICE                                  device,
    const XGL_FRAMEBUFFER_CREATE_INFO*          info,
    XGL_FRAMEBUFFER*                            fb_ret)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_fb_create(dev, info, (struct nulldrv_framebuffer **) fb_ret);
}


ICD_EXPORT XGL_RESULT XGLAPI xglCreateRenderPass(
    XGL_DEVICE                                  device,
    const XGL_RENDER_PASS_CREATE_INFO*          info,
    XGL_RENDER_PASS*                            rp_ret)
{
    NULLDRV_LOG_FUNC;
    struct nulldrv_dev *dev = nulldrv_dev(device);

    return nulldrv_render_pass_create(dev, info, (struct nulldrv_render_pass **) rp_ret);
}

ICD_EXPORT void XGLAPI xglCmdBeginRenderPass(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_RENDER_PASS_BEGIN*                pRenderPassBegin)
{
    NULLDRV_LOG_FUNC;
}

ICD_EXPORT void XGLAPI xglCmdEndRenderPass(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_RENDER_PASS                             renderPass)
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

ICD_EXPORT XGL_RESULT xcbQueuePresent(void *queue, void *image, void* fence)
{
    return XGL_SUCCESS;
}
