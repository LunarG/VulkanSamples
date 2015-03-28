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

bool intel_desc_iter_init_for_binding(struct intel_desc_iter *iter,
                                      const struct intel_desc_layout *layout,
                                      uint32_t binding_index, uint32_t array_base)
{
    const struct intel_desc_layout_binding *binding;

    if (binding_index >= layout->binding_count ||
        array_base >= layout->bindings[binding_index].array_size)
        return false;

    binding = &layout->bindings[binding_index];

    iter->type = binding->type;
    iter->increment = binding->increment;
    iter->size = binding->array_size;

    intel_desc_offset_mad(&iter->begin, &binding->increment,
            &binding->offset, array_base);
    intel_desc_offset_add(&iter->end, &iter->begin, &binding->increment);
    iter->cur = array_base;

    return true;
}

static bool desc_iter_init_for_update(struct intel_desc_iter *iter,
                                      const struct intel_desc_set *set,
                                      XGL_DESCRIPTOR_TYPE type,
                                      uint32_t binding_index, uint32_t array_base)
{
    if (!intel_desc_iter_init_for_binding(iter, set->layout,
                binding_index, array_base) || iter->type != type)
        return false;

    intel_desc_offset_add(&iter->begin, &iter->begin, &set->region_begin);
    intel_desc_offset_add(&iter->end, &iter->end, &set->region_begin);

    return true;
}

bool intel_desc_iter_advance(struct intel_desc_iter *iter)
{
    if (iter->cur >= iter->size)
        return false;

    iter->cur++;

    iter->begin = iter->end;
    intel_desc_offset_add(&iter->end, &iter->end, &iter->increment);

    return true;
}

static bool desc_region_init_desc_sizes(struct intel_desc_region *region,
                                        const struct intel_gpu *gpu)
{
    region->surface_desc_size = sizeof(struct intel_desc_surface);
    region->sampler_desc_size = sizeof(struct intel_desc_sampler);

    return true;
}

XGL_RESULT intel_desc_region_create(struct intel_dev *dev,
                                    struct intel_desc_region **region_ret)
{
    const uint32_t surface_count = 16384;
    const uint32_t sampler_count = 16384;
    struct intel_desc_region *region;

    region = intel_alloc(dev, sizeof(*region), 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!region)
        return XGL_ERROR_OUT_OF_MEMORY;

    memset(region, 0, sizeof(*region));

    if (!desc_region_init_desc_sizes(region, dev->gpu)) {
        intel_free(dev, region);
        return XGL_ERROR_UNKNOWN;
    }

    intel_desc_offset_set(&region->size,
            region->surface_desc_size * surface_count,
            region->sampler_desc_size * sampler_count);

    region->surfaces = intel_alloc(dev, region->size.surface,
            64, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!region->surfaces) {
        intel_free(dev, region);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    region->samplers = intel_alloc(dev, region->size.sampler,
            64, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!region->samplers) {
        intel_free(dev, region->surfaces);
        intel_free(dev, region);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    *region_ret = region;

    return XGL_SUCCESS;
}

void intel_desc_region_destroy(struct intel_dev *dev,
                               struct intel_desc_region *region)
{
    intel_free(dev, region->samplers);
    intel_free(dev, region->surfaces);
    intel_free(dev, region);
}

/**
 * Get the size of a descriptor in the region.
 */
static XGL_RESULT desc_region_get_desc_size(const struct intel_desc_region *region,
                                            XGL_DESCRIPTOR_TYPE type,
                                            struct intel_desc_offset *size)
{
    uint32_t surface_size = 0, sampler_size = 0;

    switch (type) {
    case XGL_DESCRIPTOR_TYPE_SAMPLER:
        sampler_size = region->sampler_desc_size;
        break;
    case XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE:
        surface_size = region->surface_desc_size;
        sampler_size = region->sampler_desc_size;
        break;
    case XGL_DESCRIPTOR_TYPE_TEXTURE:
    case XGL_DESCRIPTOR_TYPE_TEXTURE_BUFFER:
    case XGL_DESCRIPTOR_TYPE_IMAGE:
    case XGL_DESCRIPTOR_TYPE_IMAGE_BUFFER:
    case XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER:
    case XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
    case XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER_DYNAMIC:
        surface_size = region->surface_desc_size;
        break;
    default:
        assert(!"unknown descriptor type");
        return XGL_ERROR_INVALID_VALUE;
        break;
    }

    intel_desc_offset_set(size, surface_size, sampler_size);

    return XGL_SUCCESS;
}

XGL_RESULT intel_desc_region_alloc(struct intel_desc_region *region,
                                   const XGL_DESCRIPTOR_POOL_CREATE_INFO *info,
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

        ret = desc_region_get_desc_size(region, tc->type, &size);
        if (ret != XGL_SUCCESS)
            return ret;

        surface_size += size.surface * tc->count;
        sampler_size += size.sampler * tc->count;
    }

    intel_desc_offset_set(&alloc, surface_size, sampler_size);

    *begin = region->cur;
    intel_desc_offset_add(end, &region->cur, &alloc);

    if (!intel_desc_offset_within(end, &region->size))
        return XGL_ERROR_OUT_OF_MEMORY;

    /* increment the writer pointer */
    region->cur = *end;

    return XGL_SUCCESS;
}

static void desc_region_validate_begin_end(const struct intel_desc_region *region,
                                           const struct intel_desc_offset *begin,
                                           const struct intel_desc_offset *end)
{
    assert(begin->surface % region->surface_desc_size == 0 &&
           begin->sampler % region->sampler_desc_size == 0);
    assert(end->surface % region->surface_desc_size == 0 &&
           end->sampler % region->sampler_desc_size == 0);
    assert(intel_desc_offset_within(end, &region->size));
}

void intel_desc_region_free(struct intel_desc_region *region,
                            const struct intel_desc_offset *begin,
                            const struct intel_desc_offset *end)
{
    desc_region_validate_begin_end(region, begin, end);

    /* is it ok not to reclaim? */
}

XGL_RESULT intel_desc_region_begin_update(struct intel_desc_region *region,
                                          XGL_DESCRIPTOR_UPDATE_MODE mode)
{
    /* no-op */
    return XGL_SUCCESS;
}

XGL_RESULT intel_desc_region_end_update(struct intel_desc_region *region,
                                        struct intel_cmd *cmd)
{
    /* No pipelined update.  cmd_draw() will do the work. */
    return XGL_SUCCESS;
}

void intel_desc_region_clear(struct intel_desc_region *region,
                             const struct intel_desc_offset *begin,
                             const struct intel_desc_offset *end)
{
    uint32_t i;

    desc_region_validate_begin_end(region, begin, end);

    for (i = begin->surface; i < end->surface; i += region->surface_desc_size) {
        struct intel_desc_surface *desc = (struct intel_desc_surface *)
            ((char *) region->surfaces + i);

        desc->mem = NULL;
        desc->type = INTEL_DESC_SURFACE_UNUSED;
        desc->u.unused = NULL;
    }

    for (i = begin->sampler; i < end->sampler; i += region->sampler_desc_size) {
        struct intel_desc_sampler *desc = (struct intel_desc_sampler *)
            ((char *) region->samplers + i);

        desc->sampler = NULL;
    }
}

void intel_desc_region_update(struct intel_desc_region *region,
                              const struct intel_desc_offset *begin,
                              const struct intel_desc_offset *end,
                              const struct intel_desc_surface *surfaces,
                              const struct intel_desc_sampler *samplers)
{
    desc_region_validate_begin_end(region, begin, end);

    if (begin->surface < end->surface) {
        memcpy((char *) region->surfaces + begin->surface, surfaces,
                end->surface - begin->surface);
    }

    if (begin->sampler < end->sampler) {
        memcpy((char *) region->samplers + begin->sampler, samplers,
                end->sampler - begin->sampler);
    }
}

void intel_desc_region_copy(struct intel_desc_region *region,
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

    /* no read past region */
    intel_desc_offset_sub(&src_end, end, begin);
    intel_desc_offset_add(&src_end, src, &src_end);
    assert(intel_desc_offset_within(&src_end, &region->size));

    surfaces = (const struct intel_desc_surface *)
        ((const char *) region->surfaces + src->surface);
    samplers = (const struct intel_desc_sampler *)
        ((const char *) region->samplers + src->sampler);

    intel_desc_region_update(region, begin, end, surfaces, samplers);
}

void intel_desc_region_read_surface(const struct intel_desc_region *region,
                                    const struct intel_desc_offset *offset,
                                    XGL_PIPELINE_SHADER_STAGE stage,
                                    const struct intel_mem **mem,
                                    bool *read_only,
                                    const uint32_t **cmd,
                                    uint32_t *cmd_len)
{
    const struct intel_desc_surface *desc;
    struct intel_desc_offset end;

    intel_desc_offset_set(&end,
            offset->surface + region->surface_desc_size, offset->sampler);
    desc_region_validate_begin_end(region, offset, &end);

    desc = (const struct intel_desc_surface *)
        ((const char *) region->surfaces + offset->surface);

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

void intel_desc_region_read_sampler(const struct intel_desc_region *region,
                                    const struct intel_desc_offset *offset,
                                    const struct intel_sampler **sampler)
{
    const struct intel_desc_sampler *desc;
    struct intel_desc_offset end;

    intel_desc_offset_set(&end,
            offset->surface, offset->sampler + region->sampler_desc_size);
    desc_region_validate_begin_end(region, offset, &end);

    desc = (const struct intel_desc_sampler *)
        ((const char *) region->samplers + offset->sampler);

    *sampler = desc->sampler;
}

static void desc_pool_destroy(struct intel_obj *obj)
{
    struct intel_desc_pool *pool = intel_desc_pool_from_obj(obj);

    intel_desc_pool_destroy(pool);
}

XGL_RESULT intel_desc_pool_create(struct intel_dev *dev,
                                  XGL_DESCRIPTOR_POOL_USAGE usage,
                                  uint32_t max_sets,
                                  const XGL_DESCRIPTOR_POOL_CREATE_INFO *info,
                                  struct intel_desc_pool **pool_ret)
{
    struct intel_desc_pool *pool;
    XGL_RESULT ret;

    pool = (struct intel_desc_pool *) intel_base_create(&dev->base.handle,
            sizeof(*pool), dev->base.dbg, XGL_DBG_OBJECT_DESCRIPTOR_POOL,
            info, 0);
    if (!pool)
        return XGL_ERROR_OUT_OF_MEMORY;

    pool->dev = dev;

    ret = intel_desc_region_alloc(dev->desc_region, info,
            &pool->region_begin, &pool->region_end);
    if (ret != XGL_SUCCESS) {
        intel_base_destroy(&pool->obj.base);
        return ret;
    }

    /* point to head */
    pool->cur = pool->region_begin;

    pool->obj.destroy = desc_pool_destroy;

    *pool_ret = pool;

    return XGL_SUCCESS;
}

void intel_desc_pool_destroy(struct intel_desc_pool *pool)
{
    intel_desc_region_free(pool->dev->desc_region,
            &pool->region_begin, &pool->region_end);
    intel_base_destroy(&pool->obj.base);
}

XGL_RESULT intel_desc_pool_alloc(struct intel_desc_pool *pool,
                                 const struct intel_desc_layout *layout,
                                 struct intel_desc_offset *begin,
                                 struct intel_desc_offset *end)
{
    *begin = pool->cur;
    intel_desc_offset_add(end, &pool->cur, &layout->region_size);

    if (!intel_desc_offset_within(end, &pool->region_end))
        return XGL_ERROR_OUT_OF_MEMORY;

    /* increment the writer pointer */
    pool->cur = *end;

    return XGL_SUCCESS;
}

void intel_desc_pool_reset(struct intel_desc_pool *pool)
{
    /* reset to head */
    pool->cur = pool->region_begin;
}

static void desc_set_destroy(struct intel_obj *obj)
{
    struct intel_desc_set *set = intel_desc_set_from_obj(obj);

    intel_desc_set_destroy(set);
}

XGL_RESULT intel_desc_set_create(struct intel_dev *dev,
                                 struct intel_desc_pool *pool,
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

    set->region = dev->desc_region;
    ret = intel_desc_pool_alloc(pool, layout,
            &set->region_begin, &set->region_end);
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
    struct intel_desc_iter iter;
    uint32_t i;

    if (!desc_iter_init_for_update(&iter, set, XGL_DESCRIPTOR_TYPE_SAMPLER,
                update->binding, update->arrayIndex))
        return;

    for (i = 0; i < update->count; i++) {
        const struct intel_sampler *sampler =
            intel_sampler((XGL_SAMPLER) update->pSamplers[i]);
        struct intel_desc_sampler desc;

        desc.sampler = sampler;
        intel_desc_region_update(set->region, &iter.begin, &iter.end,
                NULL, &desc);

        if (!intel_desc_iter_advance(&iter))
            break;
    }
}

void intel_desc_set_update_sampler_textures(struct intel_desc_set *set,
                                            const XGL_UPDATE_SAMPLER_TEXTURES *update)
{
    struct intel_desc_iter iter;
    const struct intel_desc_layout_binding *binding;
    uint32_t i;

    if (!desc_iter_init_for_update(&iter, set, XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE,
                update->binding, update->arrayIndex))
        return;

    binding = &set->layout->bindings[update->binding];

    if (binding->shared_immutable_sampler) {
        struct intel_desc_offset end;
        struct intel_desc_sampler sampler_desc;

        assert(!iter.increment.sampler);
        intel_desc_offset_set(&end, iter.begin.surface,
                iter.begin.sampler + set->region->sampler_desc_size);

        sampler_desc.sampler = binding->shared_immutable_sampler;
        intel_desc_region_update(set->region, &iter.begin, &end,
                NULL, &sampler_desc);
    }

    for (i = 0; i < update->count; i++) {
        const struct intel_sampler *sampler = (binding->immutable_samplers) ?
            binding->immutable_samplers[update->arrayIndex + i] :
            intel_sampler(update->pSamplerImageViews[i].sampler);
        const XGL_IMAGE_VIEW_ATTACH_INFO *info =
            update->pSamplerImageViews[i].pImageView;
        const struct intel_img_view *view = intel_img_view(info->view);
        struct intel_desc_surface view_desc;
        struct intel_desc_sampler sampler_desc;

        view_desc.mem = view->img->obj.mem;
        view_desc.read_only = desc_set_img_layout_read_only(info->layout);
        view_desc.type = INTEL_DESC_SURFACE_IMG;
        view_desc.u.img = view;

        sampler_desc.sampler = sampler;

        intel_desc_region_update(set->region, &iter.begin, &iter.end,
                &view_desc, &sampler_desc);

        if (!intel_desc_iter_advance(&iter))
            break;
    }
}

void intel_desc_set_update_images(struct intel_desc_set *set,
                                  const XGL_UPDATE_IMAGES *update)
{
    struct intel_desc_iter iter;
    uint32_t i;

    if (!desc_iter_init_for_update(&iter, set, update->descriptorType,
                update->binding, update->arrayIndex))
        return;

    for (i = 0; i < update->count; i++) {
        const XGL_IMAGE_VIEW_ATTACH_INFO *info = &update->pImageViews[i];
        const struct intel_img_view *view = intel_img_view(info->view);
        struct intel_desc_surface desc;

        desc.mem = view->img->obj.mem;
        desc.read_only = desc_set_img_layout_read_only(info->layout);
        desc.type = INTEL_DESC_SURFACE_IMG;
        desc.u.img = view;
        intel_desc_region_update(set->region, &iter.begin, &iter.end,
                &desc, NULL);

        if (!intel_desc_iter_advance(&iter))
            break;
    }
}

void intel_desc_set_update_buffers(struct intel_desc_set *set,
                                   const XGL_UPDATE_BUFFERS *update)
{
    struct intel_desc_iter iter;
    uint32_t i;

    if (!desc_iter_init_for_update(&iter, set, update->descriptorType,
                update->binding, update->arrayIndex))
        return;

    for (i = 0; i < update->count; i++) {
        const XGL_BUFFER_VIEW_ATTACH_INFO *info = &update->pBufferViews[i];
        const struct intel_buf_view *view = intel_buf_view(info->view);
        struct intel_desc_surface desc;

        desc.mem = view->buf->obj.mem;
        desc.read_only = false;
        desc.type = INTEL_DESC_SURFACE_BUF;
        desc.u.buf = view;
        intel_desc_region_update(set->region, &iter.begin, &iter.end,
                &desc, NULL);

        if (!intel_desc_iter_advance(&iter))
            break;
    }
}

void intel_desc_set_update_as_copy(struct intel_desc_set *set,
                                   const XGL_UPDATE_AS_COPY *update)
{
    const struct intel_desc_set *src_set =
        intel_desc_set(update->descriptorSet);
    struct intel_desc_iter iter, src_iter;
    struct intel_desc_offset begin, src_begin;
    uint32_t i;

    /* disallow combined sampler textures */
    if (update->descriptorType == XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE)
        return;

    if (!desc_iter_init_for_update(&iter, set, update->descriptorType,
                update->binding, update->arrayElement) ||
        !desc_iter_init_for_update(&src_iter, src_set, update->descriptorType,
                update->binding, update->arrayElement))
        return;

    /* save the begin offsets */
    begin = iter.begin;
    src_begin = src_iter.begin;

    /* advance to the end */
    for (i = 0; i < update->count; i++) {
        if (!intel_desc_iter_advance(&iter) ||
            !intel_desc_iter_advance(&src_iter)) {
            /* out of bound */
            return;
        }
    }

    intel_desc_region_copy(set->region, &begin, &iter.end, &src_begin);
}

static void desc_layout_destroy(struct intel_obj *obj)
{
    struct intel_desc_layout *layout = intel_desc_layout_from_obj(obj);

    intel_desc_layout_destroy(layout);
}

static XGL_RESULT desc_layout_init_bindings(struct intel_desc_layout *layout,
                                            const struct intel_desc_region *region,
                                            const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO *info)
{
    struct intel_desc_offset offset;
    uint32_t i;
    XGL_RESULT ret;

    intel_desc_offset_set(&offset, 0, 0);

    /* allocate bindings */
    layout->bindings = intel_alloc(layout, sizeof(layout->bindings[0]) *
            info->count, 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!layout->bindings)
        return XGL_ERROR_OUT_OF_MEMORY;

    memset(layout->bindings, 0, sizeof(layout->bindings[0]) * info->count);
    layout->binding_count = info->count;

    /* initialize bindings */
    for (i = 0; i < info->count; i++) {
        const XGL_DESCRIPTOR_SET_LAYOUT_BINDING *lb = &info->pBinding[i];
        struct intel_desc_layout_binding *binding = &layout->bindings[i];
        struct intel_desc_offset size;

        switch (lb->descriptorType) {
        case XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER_DYNAMIC:
            layout->dynamic_desc_count += lb->count;
            break;
        default:
            break;
        }

        /* lb->stageFlags does not gain us anything */
        binding->type = lb->descriptorType;
        binding->array_size = lb->count;
        binding->offset = offset;

        ret = desc_region_get_desc_size(region,
                lb->descriptorType, &size);
        if (ret != XGL_SUCCESS)
            return ret;

        binding->increment = size;

        /* copy immutable samplers */
        if (lb->pImmutableSamplers) {
            bool shared = true;
            uint32_t j;

            for (j = 1; j < lb->count; j++) {
                if (lb->pImmutableSamplers[j] != lb->pImmutableSamplers[0]) {
                    shared = false;
                    break;
                }
            }

            if (shared) {
                binding->shared_immutable_sampler =
                    intel_sampler((XGL_SAMPLER) lb->pImmutableSamplers[0]);
                /* set sampler offset increment to 0 */
                intel_desc_offset_set(&binding->increment,
                        binding->increment.surface, 0);
            } else {
                binding->immutable_samplers = intel_alloc(layout,
                        sizeof(binding->immutable_samplers[0]) * lb->count,
                        0, XGL_SYSTEM_ALLOC_INTERNAL);
                if (!binding->immutable_samplers)
                    return XGL_ERROR_OUT_OF_MEMORY;

                for (j = 0; j < lb->count; j++) {
                    binding->immutable_samplers[j] =
                        intel_sampler((XGL_SAMPLER) lb->pImmutableSamplers[j]);
                }
            }
        }

        /* increment offset */
        intel_desc_offset_mad(&size, &binding->increment, &size,
                lb->count - 1);
        intel_desc_offset_add(&offset, &offset, &size);
    }

    layout->region_size = offset;

    return XGL_SUCCESS;
}

XGL_RESULT intel_desc_layout_create(struct intel_dev *dev,
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

    ret = desc_layout_init_bindings(layout, dev->desc_region, info);
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
    uint32_t i;

    for (i = 0; i < layout->binding_count; i++) {
        struct intel_desc_layout_binding *binding = &layout->bindings[i];

        if (binding->immutable_samplers)
            intel_free(layout, binding->immutable_samplers);
    }
    intel_free(layout, layout->bindings);
    intel_base_destroy(&layout->obj.base);
}

static void desc_layout_chain_destroy(struct intel_obj *obj)
{
    struct intel_desc_layout_chain *chain =
        intel_desc_layout_chain_from_obj(obj);

    intel_desc_layout_chain_destroy(chain);
}

XGL_RESULT intel_desc_layout_chain_create(struct intel_dev *dev,
                                          const XGL_DESCRIPTOR_SET_LAYOUT *layouts,
                                          uint32_t count,
                                          struct intel_desc_layout_chain **chain_ret)
{
    struct intel_desc_layout_chain *chain;
    uint32_t i;

    chain = (struct intel_desc_layout_chain *)
        intel_base_create(&dev->base.handle, sizeof(*chain), dev->base.dbg,
                XGL_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT_CHAIN, NULL, 0);
    if (!chain)
        return XGL_ERROR_OUT_OF_MEMORY;

    chain->layouts = intel_alloc(chain, sizeof(chain->layouts[0]) * count,
            0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!chain) {
        intel_desc_layout_chain_destroy(chain);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    chain->dynamic_desc_indices = intel_alloc(chain,
            sizeof(chain->dynamic_desc_indices[0]) * count,
            0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!chain->dynamic_desc_indices) {
        intel_desc_layout_chain_destroy(chain);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    for (i = 0; i < count; i++) {
        chain->layouts[i] = intel_desc_layout(layouts[i]);
        chain->dynamic_desc_indices[i] = chain->total_dynamic_desc_count;

        chain->total_dynamic_desc_count +=
            chain->layouts[i]->dynamic_desc_count;
    }

    chain->layout_count = count;

    chain->obj.destroy = desc_layout_chain_destroy;

    *chain_ret = chain;

    return XGL_SUCCESS;
}

void intel_desc_layout_chain_destroy(struct intel_desc_layout_chain *chain)
{
    if (chain->dynamic_desc_indices)
        intel_free(chain, chain->dynamic_desc_indices);
    if (chain->layouts)
        intel_free(chain, chain->layouts);
    intel_base_destroy(&chain->obj.base);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayout(
    XGL_DEVICE                                   device,
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pCreateInfo,
    XGL_DESCRIPTOR_SET_LAYOUT*                   pSetLayout)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_desc_layout_create(dev, pCreateInfo,
            (struct intel_desc_layout **) pSetLayout);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorSetLayoutChain(
    XGL_DEVICE                                   device,
    uint32_t                                     setLayoutArrayCount,
    const XGL_DESCRIPTOR_SET_LAYOUT*             pSetLayoutArray,
    XGL_DESCRIPTOR_SET_LAYOUT_CHAIN*             pLayoutChain)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_desc_layout_chain_create(dev,
            pSetLayoutArray, setLayoutArrayCount,
            (struct intel_desc_layout_chain **) pLayoutChain);
}

ICD_EXPORT XGL_RESULT XGLAPI xglBeginDescriptorPoolUpdate(
    XGL_DEVICE                                   device,
    XGL_DESCRIPTOR_UPDATE_MODE                   updateMode)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_desc_region *region = dev->desc_region;

    return intel_desc_region_begin_update(region, updateMode);
}

ICD_EXPORT XGL_RESULT XGLAPI xglEndDescriptorPoolUpdate(
    XGL_DEVICE                                   device,
    XGL_CMD_BUFFER                               cmd_)
{
    struct intel_dev *dev = intel_dev(device);
    struct intel_desc_region *region = dev->desc_region;
    struct intel_cmd *cmd = intel_cmd(cmd_);

    return intel_desc_region_end_update(region, cmd);
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateDescriptorPool(
    XGL_DEVICE                                   device,
    XGL_DESCRIPTOR_POOL_USAGE                    poolUsage,
    uint32_t                                     maxSets,
    const XGL_DESCRIPTOR_POOL_CREATE_INFO*       pCreateInfo,
    XGL_DESCRIPTOR_POOL*                         pDescriptorPool)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_desc_pool_create(dev, poolUsage, maxSets, pCreateInfo,
            (struct intel_desc_pool **) pDescriptorPool);
}

ICD_EXPORT XGL_RESULT XGLAPI xglResetDescriptorPool(
    XGL_DESCRIPTOR_POOL                          descriptorPool)
{
    struct intel_desc_pool *pool = intel_desc_pool(descriptorPool);

    intel_desc_pool_reset(pool);

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglAllocDescriptorSets(
    XGL_DESCRIPTOR_POOL                          descriptorPool,
    XGL_DESCRIPTOR_SET_USAGE                     setUsage,
    uint32_t                                     count,
    const XGL_DESCRIPTOR_SET_LAYOUT*             pSetLayouts,
    XGL_DESCRIPTOR_SET*                          pDescriptorSets,
    uint32_t*                                    pCount)
{
    struct intel_desc_pool *pool = intel_desc_pool(descriptorPool);
    struct intel_dev *dev = pool->dev;
    XGL_RESULT ret = XGL_SUCCESS;
    uint32_t i;

    for (i = 0; i < count; i++) {
        const struct intel_desc_layout *layout =
            intel_desc_layout((XGL_DESCRIPTOR_SET_LAYOUT) pSetLayouts[i]);

        ret = intel_desc_set_create(dev, pool, setUsage, layout,
                (struct intel_desc_set **) &pDescriptorSets[i]);
        if (ret != XGL_SUCCESS)
            break;
    }

    if (pCount)
        *pCount = i;

    return ret;
}

ICD_EXPORT void XGLAPI xglClearDescriptorSets(
    XGL_DESCRIPTOR_POOL                          descriptorPool,
    uint32_t                                     count,
    const XGL_DESCRIPTOR_SET*                    pDescriptorSets)
{
    uint32_t i;

    for (i = 0; i < count; i++) {
        struct intel_desc_set *set =
            intel_desc_set((XGL_DESCRIPTOR_SET) pDescriptorSets[i]);

        intel_desc_region_clear(set->region, &set->region_begin, &set->region_end);
    }
}

ICD_EXPORT void XGLAPI xglUpdateDescriptors(
    XGL_DESCRIPTOR_SET                           descriptorSet,
    uint32_t                                     updateCount,
    const void**                                 ppUpdateArray)
{
    struct intel_desc_set *set = intel_desc_set(descriptorSet);
    uint32_t i;

    for (i = 0; i < updateCount; i++) {
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
        } *u = ppUpdateArray[i];

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
    }
}
