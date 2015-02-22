/*
 * XGL
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
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "buf.h"
#include "cmd.h"
#include "dev.h"
#include "gpu.h"
#include "img.h"
#include "sampler.h"
#include "view.h"
#include "desc.h"

enum intel_desc_surface_type {
    INTEL_DESC_SURFACE_UNUSED,
    INTEL_DESC_SURFACE_BUF,
    INTEL_DESC_SURFACE_IMG,
};

struct intel_desc_surface {
    const struct intel_mem *mem;
    bool read_only;

    enum intel_desc_surface_type type;
    union {
        const void *unused;
        const struct intel_buf_view *buf;
        const struct intel_img_view *img;
    } u;
};

struct intel_desc_sampler {
    const struct intel_sampler *sampler;
};

static bool desc_pool_init_desc_sizes(struct intel_desc_pool *pool,
                                      const struct intel_gpu *gpu)
{
    pool->surface_desc_size = sizeof(struct intel_desc_surface);
    pool->sampler_desc_size = sizeof(struct intel_desc_sampler);

    return true;
}

XGL_RESULT intel_desc_pool_create(struct intel_dev *dev,
                                  struct intel_desc_pool **pool_ret)
{
    const uint32_t surface_count = 16384;
    const uint32_t sampler_count = 16384;
    struct intel_desc_pool *pool;

    pool = intel_alloc(dev, sizeof(*pool), 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!pool)
        return XGL_ERROR_OUT_OF_MEMORY;

    memset(pool, 0, sizeof(*pool));

    if (!desc_pool_init_desc_sizes(pool, dev->gpu)) {
        intel_free(dev, pool);
        return XGL_ERROR_UNKNOWN;
    }

    intel_desc_offset_set(&pool->size,
            pool->surface_desc_size * surface_count,
            pool->sampler_desc_size * sampler_count);

    pool->surfaces = intel_alloc(dev, pool->size.surface,
            64, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!pool->surfaces) {
        intel_free(dev, pool);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    pool->samplers = intel_alloc(dev, pool->size.sampler,
            64, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!pool->samplers) {
        intel_free(dev, pool->surfaces);
        intel_free(dev, pool);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    *pool_ret = pool;

    return XGL_SUCCESS;
}

void intel_desc_pool_destroy(struct intel_dev *dev,
                             struct intel_desc_pool *pool)
{
    intel_free(dev, pool->samplers);
    intel_free(dev, pool->surfaces);
    intel_free(dev, pool);
}

/**
 * Get the size of a descriptor in the pool.
 */
static XGL_RESULT desc_pool_get_desc_size(const struct intel_desc_pool *pool,
                                            XGL_DESCRIPTOR_TYPE type,
                                            struct intel_desc_offset *size)
{
    uint32_t surface_size = 0, sampler_size = 0;

    switch (type) {
    case XGL_DESCRIPTOR_TYPE_SAMPLER:
        sampler_size = pool->sampler_desc_size;
        break;
    case XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE:
        surface_size = pool->surface_desc_size;
        sampler_size = pool->sampler_desc_size;
        break;
    case XGL_DESCRIPTOR_TYPE_TEXTURE:
    case XGL_DESCRIPTOR_TYPE_TEXTURE_BUFFER:
    case XGL_DESCRIPTOR_TYPE_IMAGE:
    case XGL_DESCRIPTOR_TYPE_IMAGE_BUFFER:
    case XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER:
    case XGL_DESCRIPTOR_TYPE_RAW_BUFFER:
    case XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
    case XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER_DYNAMIC:
    case XGL_DESCRIPTOR_TYPE_RAW_BUFFER_DYNAMIC:
        surface_size = pool->surface_desc_size;
        break;
    default:
        assert(!"unknown descriptor type");
        return XGL_ERROR_INVALID_VALUE;
        break;
    }

    intel_desc_offset_set(size, surface_size, sampler_size);

    return XGL_SUCCESS;
}

XGL_RESULT intel_desc_pool_alloc(struct intel_desc_pool *pool,
                                 const XGL_DESCRIPTOR_REGION_CREATE_INFO *info,
                                 struct intel_desc_offset *begin,
                                 struct intel_desc_offset *end)
{
    uint32_t surface_size = 0, sampler_size = 0;
    struct intel_desc_offset alloc;
    uint32_t i;

    /* calculate sizes needed */
    for (i = 0; i < info->count; i++) {
        const XGL_DESCRIPTOR_TYPE_COUNT *tc = &info->pTypeCount[i];
        struct intel_desc_offset size;
        XGL_RESULT ret;

        ret = desc_pool_get_desc_size(pool, tc->type, &size);
        if (ret != XGL_SUCCESS)
            return ret;

        surface_size += size.surface * tc->count;
        sampler_size += size.sampler * tc->count;
    }

    intel_desc_offset_set(&alloc, surface_size, sampler_size);

    *begin = pool->cur;
    intel_desc_offset_add(end, &pool->cur, &alloc);

    if (!intel_desc_offset_within(end, &pool->size))
        return XGL_ERROR_OUT_OF_MEMORY;

    /* increment the writer pointer */
    pool->cur = *end;

    return XGL_SUCCESS;
}

static void desc_pool_validate_begin_end(const struct intel_desc_pool *pool,
                                         const struct intel_desc_offset *begin,
                                         const struct intel_desc_offset *end)
{
    assert(begin->surface % pool->surface_desc_size == 0 &&
           begin->sampler % pool->sampler_desc_size == 0);
    assert(end->surface % pool->surface_desc_size == 0 &&
           end->sampler % pool->sampler_desc_size == 0);
    assert(intel_desc_offset_within(end, &pool->size));
}

void intel_desc_pool_free(struct intel_desc_pool *pool,
                          const struct intel_desc_offset *begin,
                          const struct intel_desc_offset *end)
{
    desc_pool_validate_begin_end(pool, begin, end);

    /* is it ok not to reclaim? */
}

XGL_RESULT intel_desc_pool_begin_update(struct intel_desc_pool *pool,
                                        XGL_DESCRIPTOR_UPDATE_MODE mode)
{
    /* no-op */
    return XGL_SUCCESS;
}

XGL_RESULT intel_desc_pool_end_update(struct intel_desc_pool *pool,
                                      struct intel_cmd *cmd)
{
    /* No pipelined update.  cmd_draw() will do the work. */
    return XGL_SUCCESS;
}

void intel_desc_pool_clear(struct intel_desc_pool *pool,
                           const struct intel_desc_offset *begin,
                           const struct intel_desc_offset *end)
{
    uint32_t i;

    desc_pool_validate_begin_end(pool, begin, end);

    for (i = begin->surface; i < end->surface; i += pool->surface_desc_size) {
        struct intel_desc_surface *desc = (struct intel_desc_surface *)
            ((char *) pool->surfaces + i);

        desc->mem = NULL;
        desc->type = INTEL_DESC_SURFACE_UNUSED;
        desc->u.unused = NULL;
    }

    for (i = begin->sampler; i < end->sampler; i += pool->sampler_desc_size) {
        struct intel_desc_sampler *desc = (struct intel_desc_sampler *)
            ((char *) pool->samplers + i);

        desc->sampler = NULL;
    }
}

void intel_desc_pool_update(struct intel_desc_pool *pool,
                            const struct intel_desc_offset *begin,
                            const struct intel_desc_offset *end,
                            const struct intel_desc_surface *surfaces,
                            const struct intel_desc_sampler *samplers)
{
    desc_pool_validate_begin_end(pool, begin, end);

    if (begin->surface < end->surface) {
        memcpy((char *) pool->surfaces + begin->surface, surfaces,
                end->surface - begin->surface);
    }

    if (begin->sampler < end->sampler) {
        memcpy((char *) pool->samplers + begin->sampler, samplers,
                end->sampler - begin->sampler);
    }
}

void intel_desc_pool_copy(struct intel_desc_pool *pool,
                          const struct intel_desc_offset *begin,
                          const struct intel_desc_offset *end,
                          const struct intel_desc_offset *src)
{
    struct intel_desc_offset src_end;
    const struct intel_desc_surface *surfaces;
    const struct intel_desc_sampler *samplers;

    /* no overlap */
    assert(intel_desc_offset_within(src, begin) ||
           intel_desc_offset_within(end, src));

    /* no read past pool */
    intel_desc_offset_sub(&src_end, end, begin);
    intel_desc_offset_add(&src_end, src, &src_end);
    assert(intel_desc_offset_within(&src_end, &pool->size));

    surfaces = (const struct intel_desc_surface *)
        ((const char *) pool->surfaces + src->surface);
    samplers = (const struct intel_desc_sampler *)
        ((const char *) pool->samplers + src->sampler);

    intel_desc_pool_update(pool, begin, end, surfaces, samplers);
}

static void desc_region_destroy(struct intel_obj *obj)
{
    struct intel_desc_region *region = intel_desc_region_from_obj(obj);

    intel_desc_region_destroy(region);
}

XGL_RESULT intel_desc_region_create(struct intel_dev *dev,
                                    XGL_DESCRIPTOR_REGION_USAGE usage,
                                    uint32_t max_sets,
                                    const XGL_DESCRIPTOR_REGION_CREATE_INFO *info,
                                    struct intel_desc_region **region_ret)
{
    struct intel_desc_region *region;
    XGL_RESULT ret;

    region = (struct intel_desc_region *) intel_base_create(&dev->base.handle,
            sizeof(*region), dev->base.dbg, XGL_DBG_OBJECT_DESCRIPTOR_REGION,
            info, 0);
    if (!region)
        return XGL_ERROR_OUT_OF_MEMORY;

    region->dev = dev;

    ret = intel_desc_pool_alloc(dev->desc_pool, info,
            &region->pool_begin, &region->pool_end);
    if (ret != XGL_SUCCESS) {
        intel_base_destroy(&region->obj.base);
        return ret;
    }

    /* point to head */
    region->cur = region->pool_begin;

    region->obj.destroy = desc_region_destroy;

    *region_ret = region;

    return XGL_SUCCESS;
}

void intel_desc_region_destroy(struct intel_desc_region *region)
{
    intel_desc_pool_free(region->dev->desc_pool,
            &region->pool_begin, &region->pool_end);
    intel_base_destroy(&region->obj.base);
}

XGL_RESULT intel_desc_region_alloc(struct intel_desc_region *region,
                                   const struct intel_desc_layout *layout,
                                   struct intel_desc_offset *begin,
                                   struct intel_desc_offset *end)
{
    *begin = region->cur;
    intel_desc_offset_add(end, &region->cur, &layout->pool_size);

    if (!intel_desc_offset_within(end, &region->pool_end))
        return XGL_ERROR_OUT_OF_MEMORY;

    /* increment the writer pointer */
    region->cur = *end;

    return XGL_SUCCESS;
}

void intel_desc_region_free_all(struct intel_desc_region *region)
{
    /* reset to head */
    region->cur = region->pool_begin;
}

static void desc_set_destroy(struct intel_obj *obj)
{
    struct intel_desc_set *set = intel_desc_set_from_obj(obj);

    intel_desc_set_destroy(set);
}

XGL_RESULT intel_desc_set_create(struct intel_dev *dev,
                                 struct intel_desc_region *region,
                                 XGL_DESCRIPTOR_SET_USAGE usage,
                                 const struct intel_desc_layout *layout,
                                 struct intel_desc_set **set_ret)
{
    struct intel_desc_set *set;
    XGL_RESULT ret;

    set = (struct intel_desc_set *) intel_base_create(&dev->base.handle,
            sizeof(*set), dev->base.dbg, XGL_DBG_OBJECT_DESCRIPTOR_SET,
            NULL, 0);
    if (!set)
        return XGL_ERROR_OUT_OF_MEMORY;

    set->pool = dev->desc_pool;
    ret = intel_desc_region_alloc(region, layout,
            &set->pool_begin, &set->pool_end);
    if (ret != XGL_SUCCESS) {
        intel_base_destroy(&set->obj.base);
        return ret;
    }

    set->layout = layout;

    set->obj.destroy = desc_set_destroy;

    *set_ret = set;

    return XGL_SUCCESS;
}

void intel_desc_set_destroy(struct intel_desc_set *set)
{
    intel_base_destroy(&set->obj.base);
}

static void desc_set_update(struct intel_desc_set *set,
                            const struct intel_desc_layout_iter *iter,
                            const struct intel_desc_surface *surfaces,
                            const struct intel_desc_sampler *samplers)
{
    struct intel_desc_offset begin, end;

    intel_desc_offset_add(&begin, &set->pool_begin, &iter->offset_begin);
    intel_desc_offset_add(&end, &set->pool_begin, &iter->offset_end);

    intel_desc_pool_update(set->pool, &begin, &end, surfaces, samplers);
}

static bool desc_set_img_layout_read_only(XGL_IMAGE_LAYOUT layout)
{
    switch (layout) {
    case XGL_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
    case XGL_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    case XGL_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL:
        return true;
    default:
        return false;
    }
}

void intel_desc_set_update_samplers(struct intel_desc_set *set,
                                    const XGL_UPDATE_SAMPLERS *update)
{
    struct intel_desc_layout_iter iter;
    uint32_t i;

    if (!intel_desc_layout_find_index(set->layout, update->index, &iter))
        return;

    for (i = 0; i < update->count; i++) {
        const struct intel_sampler *sampler =
            intel_sampler((XGL_SAMPLER) update->pSamplers[i]);
        struct intel_desc_sampler desc;

        if (iter.type != XGL_DESCRIPTOR_TYPE_SAMPLER)
            break;

        desc.sampler = sampler;
        desc_set_update(set, &iter, NULL, &desc);

        if (!intel_desc_layout_advance_iter(set->layout, &iter))
            break;
    }
}

void intel_desc_set_update_sampler_textures(struct intel_desc_set *set,
                                            const XGL_UPDATE_SAMPLER_TEXTURES *update)
{
    struct intel_desc_layout_iter iter;
    const struct intel_sampler *immutable_sampler = NULL;
    uint32_t i;

    if (!intel_desc_layout_find_index(set->layout, update->index, &iter))
        return;

    for (i = 0; i < update->count; i++) {
        const struct intel_sampler *sampler =
            intel_sampler(update->pSamplerImageViews[i].pSampler);
        const XGL_IMAGE_VIEW_ATTACH_INFO *info =
            update->pSamplerImageViews[i].pImageView;
        const struct intel_img_view *view = intel_img_view(info->view);
        struct intel_desc_surface view_desc;
        struct intel_desc_sampler sampler_desc;

        if (iter.type != XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE)
            return;

        /* update every immutable sampler once */
        if (immutable_sampler != iter.range->immutable_sampler) {
            immutable_sampler = iter.range->immutable_sampler;

            if (immutable_sampler) {
                struct intel_desc_offset begin, end;

                intel_desc_offset_add(&begin, &set->pool_begin,
                        &iter.offset_begin);
                intel_desc_offset_set(&end, begin.surface,
                        begin.sampler + set->pool->sampler_desc_size);

                sampler_desc.sampler = immutable_sampler;
                intel_desc_pool_update(set->pool, &begin, &end,
                        NULL, &sampler_desc);
            }
        }

        view_desc.mem = view->img->obj.mem;
        view_desc.read_only = desc_set_img_layout_read_only(info->layout);
        view_desc.type = INTEL_DESC_SURFACE_IMG;
        view_desc.u.img = view;

        sampler_desc.sampler = sampler;

        desc_set_update(set, &iter, &view_desc, &sampler_desc);

        if (!intel_desc_layout_advance_iter(set->layout, &iter))
            break;
    }
}

void intel_desc_set_update_images(struct intel_desc_set *set,
                                  const XGL_UPDATE_IMAGES *update)
{
    struct intel_desc_layout_iter iter;
    uint32_t i;

    if (!intel_desc_layout_find_index(set->layout, update->index, &iter))
        return;

    for (i = 0; i < update->count; i++) {
        const XGL_IMAGE_VIEW_ATTACH_INFO *info = update->pImageViews[i];
        const struct intel_img_view *view = intel_img_view(info->view);
        struct intel_desc_surface desc;

        if (iter.type != update->descriptorType)
            break;

        desc.mem = view->img->obj.mem;
        desc.read_only = desc_set_img_layout_read_only(info->layout);
        desc.type = INTEL_DESC_SURFACE_IMG;
        desc.u.img = view;
        desc_set_update(set, &iter, &desc, NULL);

        if (!intel_desc_layout_advance_iter(set->layout, &iter))
            break;
    }
}

void intel_desc_set_update_buffers(struct intel_desc_set *set,
                                   const XGL_UPDATE_BUFFERS *update)
{
    struct intel_desc_layout_iter iter;
    uint32_t i;

    if (!intel_desc_layout_find_index(set->layout, update->index, &iter))
        return;

    for (i = 0; i < update->count; i++) {
        const XGL_BUFFER_VIEW_ATTACH_INFO *info = update->pBufferViews[i];
        const struct intel_buf_view *view = intel_buf_view(info->view);
        struct intel_desc_surface desc;

        if (iter.type != update->descriptorType)
            break;

        desc.mem = view->buf->obj.mem;
        desc.read_only = false;
        desc.type = INTEL_DESC_SURFACE_BUF;
        desc.u.buf = view;
        desc_set_update(set, &iter, &desc, NULL);

        if (!intel_desc_layout_advance_iter(set->layout, &iter))
            break;
    }
}

void intel_desc_set_update_as_copy(struct intel_desc_set *set,
                                   const XGL_UPDATE_AS_COPY *update)
{
    const struct intel_desc_set *src_set =
        intel_desc_set(update->descriptorSet);
    struct intel_desc_layout_iter iter, src_iter;
    struct intel_desc_offset begin, end, src_begin;
    uint32_t i;

    /* disallow combined sampler textures */
    if (update->descriptorType == XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE)
        return;

    /* no update->index? */
    if (!intel_desc_layout_find_index(set->layout, 0, &iter))
        return;
    if (!intel_desc_layout_find_index(src_set->layout,
                update->descriptorIndex, &src_iter))
        return;

    intel_desc_offset_add(&begin, &set->pool_begin, &iter.offset_begin);
    intel_desc_offset_add(&src_begin, &src_set->pool_begin,
            &src_iter.offset_end);

    /* advance to end */
    for (i = 0; i < update->count; i++) {
        if (iter.type != update->descriptorType ||
            src_iter.type != update->descriptorType ||
            !intel_desc_layout_advance_iter(set->layout, &iter) ||
            !intel_desc_layout_advance_iter(src_set->layout, &src_iter))
            break;
    }
    if (i < update->count)
        return;

    intel_desc_offset_add(&end, &src_set->pool_begin, &iter.offset_end);

    intel_desc_pool_copy(src_set->pool, &begin, &end, &src_begin);
}

static void desc_set_read(const struct intel_desc_set *set,
                          const struct intel_desc_offset *offset,
                          const struct intel_desc_surface **surface,
                          const struct intel_desc_sampler **sampler)
{
    struct intel_desc_offset begin, end;

    intel_desc_offset_add(&begin, &set->pool_begin, offset);
    intel_desc_offset_set(&end, 0, 0);

    if (surface) {
        *surface = (const struct intel_desc_surface *)
            ((const char *) set->pool->surfaces + begin.surface);

        end.surface = set->pool->surface_desc_size;
    }

    if (sampler) {
        *sampler = (const struct intel_desc_sampler *)
            ((const char *) set->pool->samplers + begin.sampler);

        end.sampler = set->pool->sampler_desc_size;
    }

    intel_desc_offset_add(&end, &begin, &end);
    desc_pool_validate_begin_end(set->pool, &begin, &end);
}

void intel_desc_set_read_surface(const struct intel_desc_set *set,
                                 const struct intel_desc_offset *offset,
                                 XGL_PIPELINE_SHADER_STAGE stage,
                                 const struct intel_mem **mem,
                                 bool *read_only,
                                 const uint32_t **cmd,
                                 uint32_t *cmd_len)
{
    const struct intel_desc_surface *desc;

    desc_set_read(set, offset, &desc, NULL);

    *mem = desc->mem;
    *read_only = desc->read_only;
    switch (desc->type) {
    case INTEL_DESC_SURFACE_BUF:
        *cmd = (stage == XGL_SHADER_STAGE_FRAGMENT) ?
            desc->u.buf->fs_cmd : desc->u.buf->cmd;
        *cmd_len = desc->u.buf->cmd_len;
        break;
    case INTEL_DESC_SURFACE_IMG:
        *cmd = desc->u.img->cmd;
        *cmd_len = desc->u.img->cmd_len;
        break;
    case INTEL_DESC_SURFACE_UNUSED:
    default:
        *cmd = NULL;
        *cmd_len = 0;
        break;
    }
}

void intel_desc_set_read_sampler(const struct intel_desc_set *set,
                                 const struct intel_desc_offset *offset,
                                 const struct intel_sampler **sampler)
{
    const struct intel_desc_sampler *desc;

    desc_set_read(set, offset, NULL, &desc);

    *sampler = desc->sampler;
}

static void desc_layout_destroy(struct intel_obj *obj)
{
    struct intel_desc_layout *layout = intel_desc_layout_from_obj(obj);

    intel_desc_layout_destroy(layout);
}

static XGL_RESULT desc_layout_alloc_ranges(struct intel_desc_layout *layout,
                                           const struct intel_desc_pool *pool,
                                           const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO *info)
{
    /* calculate counts */
    while (info) {
        if (info->sType !=
                XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO)
            return XGL_ERROR_INVALID_POINTER;

        switch (info->descriptorType) {
        case XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER_DYNAMIC:
        case XGL_DESCRIPTOR_TYPE_RAW_BUFFER_DYNAMIC:
            layout->dynamic_desc_count += info->count;
            break;
        default:
            break;
        }

        layout->range_count++;
        info = info->pNext;
    }

    layout->ranges = intel_alloc(layout, sizeof(layout->ranges[0]) *
            layout->range_count, 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!layout->ranges)
        return XGL_ERROR_OUT_OF_MEMORY;

    memset(layout->ranges, 0, sizeof(layout->ranges[0]) *
            layout->range_count);

    return XGL_SUCCESS;
}

static XGL_RESULT desc_layout_init_ranges(struct intel_desc_layout *layout,
                                          const struct intel_desc_pool *pool,
                                          const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO *info)
{
    struct intel_desc_offset offset;
    uint32_t index, i;
    XGL_RESULT ret;

    ret = desc_layout_alloc_ranges(layout, pool, info);
    if (ret != XGL_SUCCESS)
        return ret;

    if (layout->prior_layout) {
        index = layout->prior_layout->end;
        offset = layout->prior_layout->pool_size;
    } else {
        index = 0;
        intel_desc_offset_set(&offset, 0, 0);
    }

    layout->begin = index;

    /* initialize the ranges */
    for (i = 0; i < layout->range_count; i++, info = info->pNext) {
        struct intel_desc_layout_range *range = &layout->ranges[i];
        struct intel_desc_offset size;

        ret = desc_pool_get_desc_size(pool, info->descriptorType, &size);
        if (ret != XGL_SUCCESS)
            return ret;

        /* info->stageFlags does not gain us anything */
        range->type = info->descriptorType;

        range->begin = index;
        range->offset = offset;

        if (info->immutableSampler != XGL_NULL_HANDLE) {
            range->immutable_sampler = intel_sampler(info->immutableSampler);
            /* do not increment sampler offset */
            intel_desc_offset_set(&range->increment, size.surface, 0);
        } else {
            range->immutable_sampler = NULL;
            range->increment = size;
        }

        /* increment index and offset */
        index += info->count;
        intel_desc_offset_mad(&size, &range->increment, &size,
                info->count - 1);
        intel_desc_offset_add(&offset, &offset, &size);

        range->end = index;
    }

    layout->end = index;
    layout->pool_size = offset;

    return XGL_SUCCESS;
}

static XGL_RESULT desc_layout_init_bind_points(struct intel_desc_layout *layout,
                                               XGL_FLAGS stage_flags,
                                               const uint32_t *bind_points)
{
    if (!bind_points)
        return XGL_ERROR_INVALID_POINTER;

    layout->stage_flags = stage_flags;

    if (stage_flags == XGL_SHADER_STAGE_FLAGS_ALL) {
        layout->bind_point_vs = *bind_points;
        layout->bind_point_tcs = *bind_points;
        layout->bind_point_tes = *bind_points;
        layout->bind_point_gs = *bind_points;
        layout->bind_point_fs = *bind_points;
        layout->bind_point_cs = *bind_points;
    } else {
        if (stage_flags & XGL_SHADER_STAGE_FLAGS_VERTEX_BIT)
            layout->bind_point_vs = *bind_points++;
        if (stage_flags & XGL_SHADER_STAGE_FLAGS_TESS_CONTROL_BIT)
            layout->bind_point_tcs = *bind_points++;
        if (stage_flags & XGL_SHADER_STAGE_FLAGS_TESS_EVALUATION_BIT)
            layout->bind_point_tes = *bind_points++;
        if (stage_flags & XGL_SHADER_STAGE_FLAGS_GEOMETRY_BIT)
            layout->bind_point_gs = *bind_points++;
        if (stage_flags & XGL_SHADER_STAGE_FLAGS_FRAGMENT_BIT)
            layout->bind_point_fs = *bind_points++;
        if (stage_flags & XGL_SHADER_STAGE_FLAGS_COMPUTE_BIT)
            layout->bind_point_cs = *bind_points++;
    }

    return XGL_SUCCESS;
}

XGL_RESULT intel_desc_layout_create(struct intel_dev *dev,
                                    XGL_FLAGS stage_flags,
                                    const uint32_t *bind_points,
                                    const struct intel_desc_layout *prior_layout,
                                    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO *info,
                                    struct intel_desc_layout **layout_ret)
{
    struct intel_desc_layout *layout;
    XGL_RESULT ret;

    layout = (struct intel_desc_layout *) intel_base_create(&dev->base.handle,
            sizeof(*layout), dev->base.dbg,
            XGL_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT, info, 0);
    if (!layout)
        return XGL_ERROR_OUT_OF_MEMORY;

    layout->prior_layout = prior_layout;

    ret = desc_layout_init_bind_points(layout, stage_flags, bind_points);
    if (ret == XGL_SUCCESS)
        ret = desc_layout_init_ranges(layout, dev->desc_pool, info);
    if (ret != XGL_SUCCESS) {
        intel_desc_layout_destroy(layout);
        return ret;
    }

    layout->obj.destroy = desc_layout_destroy;

    *layout_ret = layout;

    return XGL_SUCCESS;
}

void intel_desc_layout_destroy(struct intel_desc_layout *layout)
{
    intel_free(layout, layout->ranges);
    intel_base_destroy(&layout->obj.base);
}

static void desc_layout_init_iter(const struct intel_desc_layout *sublayout,
                                  uint32_t index,
                                  struct intel_desc_layout_iter *iter)
{
    const struct intel_desc_layout_range *range;

    assert(index >= sublayout->begin && index < sublayout->end);

    /* find the range the index is in */
    for (range = sublayout->ranges;; range++) {
        assert(range < sublayout->ranges + sublayout->range_count);
        if (index < range->end)
            break;
    }

    /* current position */
    iter->sublayout = sublayout;
    iter->range = range;
    iter->index = index;

    iter->type = iter->range->type;
    intel_desc_offset_mad(&iter->offset_begin, &range->increment,
            &range->offset, iter->index - range->begin);
    intel_desc_offset_add(&iter->offset_end, &iter->offset_begin,
            &range->increment);
}

bool intel_desc_layout_find_bind_point(const struct intel_desc_layout *layout,
                                       XGL_PIPELINE_SHADER_STAGE stage,
                                       uint32_t set, uint32_t binding,
                                       struct intel_desc_layout_iter *iter)
{
    /* find the layout at the bind point */
    switch (stage) {
#define CASE(stage, s)                                              \
    case XGL_SHADER_STAGE_ ##stage:                                 \
        while (layout) {                                            \
            if ((layout->stage_flags &                              \
                        XGL_SHADER_STAGE_FLAGS_ ##stage## _BIT) &&  \
                layout->bind_point_ ##s == set)                     \
                break;                                              \
            layout = layout->prior_layout;                          \
        }                                                           \
        break
    CASE(VERTEX, vs);
    CASE(TESS_CONTROL, tcs);
    CASE(TESS_EVALUATION, tes);
    CASE(GEOMETRY, gs);
    CASE(FRAGMENT, fs);
    CASE(COMPUTE, cs);
#undef CASE
    default:
        assert(!"unknown shader stage");
        layout = NULL;
        break;
    }

    if (!layout || layout->begin + binding >= layout->end) {
        memset(iter, 0, sizeof(*iter));
        return false;
    }

    desc_layout_init_iter(layout, layout->begin + binding, iter);

    return true;
}

bool intel_desc_layout_find_index(const struct intel_desc_layout *layout,
                                  uint32_t index,
                                  struct intel_desc_layout_iter *iter)
{
    if (index >= layout->begin) {
        /* out of bound */
        if (index >= layout->end)
            layout = NULL;
    } else {
        while (true) {
            layout = layout->prior_layout;
            if (index >= layout->begin) {
                assert(index < layout->end);
                break;
            }
        }
    }

    if (!layout) {
        memset(iter, 0, sizeof(*iter));
        return false;
    }

    desc_layout_init_iter(layout, index, iter);

    return true;
}

bool intel_desc_layout_advance_iter(const struct intel_desc_layout *layout,
                                    struct intel_desc_layout_iter *iter)
{
    /* all descriptors traversed */
    if (!iter->sublayout)
        return false;

    iter->index++;
    if (iter->index >= iter->range->end) {
        /* move to the next range */
        iter->range++;

        if (iter->range >= iter->sublayout->ranges +
                           iter->sublayout->range_count) {
            /* find again as the chain is not doubly-linked */
            const bool ret = intel_desc_layout_find_index(layout,
                    iter->index, iter);
            if (!ret) {
                iter->sublayout = NULL;
                iter->range = NULL;
            }

            return ret;
        }

        iter->type = iter->range->type;
    }

    iter->offset_begin = iter->offset_end;
    intel_desc_offset_add(&iter->offset_end, &iter->offset_end,
            &iter->range->increment);

    return true;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayout(
    XGL_DEVICE                                   device,
    XGL_FLAGS                                    stageFlags,
    const uint32_t*                              pSetBindPoints,
    XGL_DESCRIPTOR_SET_LAYOUT                    priorSetLayout,
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList,
    XGL_DESCRIPTOR_SET_LAYOUT*                   pSetLayout)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_desc_layout *prior_layout = intel_desc_layout(priorSetLayout);

    return intel_desc_layout_create(dev, stageFlags, pSetBindPoints,
            prior_layout, pSetLayoutInfoList,
            (struct intel_desc_layout **) pSetLayout);
}

ICD_EXPORT XGL_RESULT XGLAPI xglBeginDescriptorRegionUpdate(
    XGL_DEVICE                                   device,
    XGL_DESCRIPTOR_UPDATE_MODE                   updateMode)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_desc_pool *pool = dev->desc_pool;

    return intel_desc_pool_begin_update(pool, updateMode);
}

ICD_EXPORT XGL_RESULT XGLAPI xglEndDescriptorRegionUpdate(
    XGL_DEVICE                                   device,
    XGL_CMD_BUFFER                               cmd_)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_desc_pool *pool = dev->desc_pool;
    struct intel_cmd *cmd = intel_cmd(cmd_);

    return intel_desc_pool_end_update(pool, cmd);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorRegion(
    XGL_DEVICE                                   device,
    XGL_DESCRIPTOR_REGION_USAGE                  regionUsage,
    uint32_t                                     maxSets,
    const XGL_DESCRIPTOR_REGION_CREATE_INFO*     pCreateInfo,
    XGL_DESCRIPTOR_REGION*                       pDescriptorRegion)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_desc_region_create(dev, regionUsage, maxSets, pCreateInfo,
            (struct intel_desc_region **) pDescriptorRegion);
}

ICD_EXPORT XGL_RESULT XGLAPI xglClearDescriptorRegion(
    XGL_DESCRIPTOR_REGION                        descriptorRegion)
{
    struct intel_desc_region *region = intel_desc_region(descriptorRegion);

    intel_desc_region_free_all(region);

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglAllocDescriptorSets(
    XGL_DESCRIPTOR_REGION                        descriptorRegion,
    XGL_DESCRIPTOR_SET_USAGE                     setUsage,
    uint32_t                                     count,
    const XGL_DESCRIPTOR_SET_LAYOUT*             pSetLayouts,
    XGL_DESCRIPTOR_SET*                          pDescriptorSets,
    uint32_t*                                    pCount)
{
    struct intel_desc_region *region = intel_desc_region(descriptorRegion);
    struct intel_dev *dev = region->dev;
    XGL_RESULT ret = XGL_SUCCESS;
    uint32_t i;

    for (i = 0; i < count; i++) {
        const struct intel_desc_layout *layout =
            intel_desc_layout((XGL_DESCRIPTOR_SET_LAYOUT) pSetLayouts[i]);

        ret = intel_desc_set_create(dev, region, setUsage, layout,
                (struct intel_desc_set **) &pDescriptorSets[i]);
        if (ret != XGL_SUCCESS)
            break;
    }

    if (pCount)
        *pCount = i;

    return ret;
}

ICD_EXPORT void XGLAPI xglClearDescriptorSets(
    XGL_DESCRIPTOR_REGION                        descriptorRegion,
    uint32_t                                     count,
    const XGL_DESCRIPTOR_SET*                    pDescriptorSets)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        struct intel_desc_set *set =
            intel_desc_set((XGL_DESCRIPTOR_SET) pDescriptorSets[i]);

        intel_desc_pool_clear(set->pool, &set->pool_begin, &set->pool_end);
    }
}

ICD_EXPORT void XGLAPI xglUpdateDescriptors(
    XGL_DESCRIPTOR_SET                           descriptorSet,
    const void*                                  pUpdateChain)
{
    struct intel_desc_set *set = intel_desc_set(descriptorSet);
    const union {
        struct {
            XGL_STRUCTURE_TYPE                      sType;
            const void*                             pNext;
        } common;

        XGL_UPDATE_SAMPLERS samplers;
        XGL_UPDATE_SAMPLER_TEXTURES sampler_textures;
        XGL_UPDATE_IMAGES images;
        XGL_UPDATE_BUFFERS buffers;
        XGL_UPDATE_AS_COPY as_copy;
    } *u = pUpdateChain;

    while (u) {
        switch (u->common.sType) {
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            intel_desc_set_update_samplers(set, &u->samplers);
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            intel_desc_set_update_sampler_textures(set, &u->sampler_textures);
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            intel_desc_set_update_images(set, &u->images);
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            intel_desc_set_update_buffers(set, &u->buffers);
            break;
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
            intel_desc_set_update_as_copy(set, &u->as_copy);
            break;
        default:
            assert(!"unknown descriptor update");
            break;
        }

        u = u->common.pNext;
    }
}
