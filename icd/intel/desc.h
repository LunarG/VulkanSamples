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

#ifndef DESC_H
#define DESC_H

#include "intel.h"
#include "obj.h"

struct intel_cmd;
struct intel_dev;

/**
 * Opaque descriptors.
 */
struct intel_desc_surface;
struct intel_desc_sampler;

/**
 * Descriptor pool offset (or size) in bytes.
 */
struct intel_desc_offset {
    uint32_t surface;
    uint32_t sampler;
};

/**
 * Per-device descriptor pool.
 */
struct intel_desc_pool {
    /* this is not an intel_obj */

    uint32_t surface_desc_size;
    uint32_t sampler_desc_size;

    /* surface descriptors (in system memory until RS is enabled) */
    struct intel_desc_surface *surfaces;
    /* sampler desciptors */
    struct intel_desc_sampler *samplers;

    struct intel_desc_offset size;
    struct intel_desc_offset cur;
};

struct intel_desc_region {
    struct intel_obj obj;

    struct intel_dev *dev;

    /* point to a continuous area in the device's pool */
    struct intel_desc_offset pool_begin;
    struct intel_desc_offset pool_end;

    struct intel_desc_offset cur;
};

struct intel_desc_layout;

struct intel_desc_set {
    struct intel_obj obj;

    /* suballocated from a region */
    struct intel_desc_pool *pool;
    struct intel_desc_offset pool_begin;
    struct intel_desc_offset pool_end;

    const struct intel_desc_layout *layout;
};

struct intel_desc_layout {
    struct intel_obj obj;

    /* a chain of layouts actually */
    const struct intel_desc_layout *prior_layout;

    /* where this layout binds to in shader stages */
    XGL_FLAGS stage_flags;
    uint32_t bind_point_vs;
    uint32_t bind_point_tcs;
    uint32_t bind_point_tes;
    uint32_t bind_point_gs;
    uint32_t bind_point_fs;
    uint32_t bind_point_cs;

    /* the continuous area in the descriptor set this layout is for */
    uint32_t begin;
    uint32_t end;

    /* homogeneous ranges in this layout */
    struct intel_desc_layout_range {
        XGL_DESCRIPTOR_TYPE type;
        const struct intel_sampler *immutable_sampler;

        /* to speed up intel_desc_layout_advance() */
        uint32_t begin;
        uint32_t end;
        struct intel_desc_offset offset;
        struct intel_desc_offset increment;
    } *ranges;
    uint32_t range_count;

    /* count of _DYNAMIC descriptors */
    uint32_t dynamic_desc_count;

    /* the size of the entire layout chain in the pool */
    struct intel_desc_offset pool_size;
};

struct intel_desc_layout_iter {
    /* current position in the chain of layouts */
    const struct intel_desc_layout *sublayout;
    const struct intel_desc_layout_range *range;
    uint32_t index;

    XGL_DESCRIPTOR_TYPE type;
    struct intel_desc_offset offset_begin;
    struct intel_desc_offset offset_end;
};

static inline struct intel_desc_region *intel_desc_region(XGL_DESCRIPTOR_REGION region)
{
    return (struct intel_desc_region *) region;
}

static inline struct intel_desc_region *intel_desc_region_from_obj(struct intel_obj *obj)
{
    return (struct intel_desc_region *) obj;
}

static inline struct intel_desc_set *intel_desc_set(XGL_DESCRIPTOR_SET set)
{
    return (struct intel_desc_set *) set;
}

static inline struct intel_desc_set *intel_desc_set_from_obj(struct intel_obj *obj)
{
    return (struct intel_desc_set *) obj;
}

static inline struct intel_desc_layout *intel_desc_layout(XGL_DESCRIPTOR_SET_LAYOUT layout)
{
    return (struct intel_desc_layout *) layout;
}

static inline struct intel_desc_layout *intel_desc_layout_from_obj(struct intel_obj *obj)
{
    return (struct intel_desc_layout *) obj;
}

static inline void intel_desc_offset_set(struct intel_desc_offset *offset,
                                         uint32_t surface_offset,
                                         uint32_t sampler_offset)
{
    offset->surface = surface_offset;
    offset->sampler = sampler_offset;
}

static inline void intel_desc_offset_add(struct intel_desc_offset *offset,
                                         const struct intel_desc_offset *lhs,
                                         const struct intel_desc_offset *rhs)
{
    offset->surface = lhs->surface + rhs->surface;
    offset->sampler = lhs->sampler + rhs->sampler;
}

static inline void intel_desc_offset_sub(struct intel_desc_offset *offset,
                                         const struct intel_desc_offset *lhs,
                                         const struct intel_desc_offset *rhs)
{
    offset->surface = lhs->surface - rhs->surface;
    offset->sampler = lhs->sampler - rhs->sampler;
}

static inline void intel_desc_offset_mad(struct intel_desc_offset *offset,
                                         const struct intel_desc_offset *lhs,
                                         const struct intel_desc_offset *rhs,
                                         uint32_t lhs_scale)
{
    offset->surface = lhs->surface * lhs_scale + rhs->surface;
    offset->sampler = lhs->sampler * lhs_scale + rhs->sampler;
}

static inline bool intel_desc_offset_within(const struct intel_desc_offset *offset,
                                            const struct intel_desc_offset *other)
{
    return (offset->surface <= other->surface &&
            offset->sampler <= other->sampler);
}

XGL_RESULT intel_desc_pool_create(struct intel_dev *dev,
                                  struct intel_desc_pool **pool_ret);
void intel_desc_pool_destroy(struct intel_desc_pool *pool);

XGL_RESULT intel_desc_pool_alloc(struct intel_desc_pool *pool,
                                 const XGL_DESCRIPTOR_REGION_CREATE_INFO *info,
                                 struct intel_desc_offset *begin,
                                 struct intel_desc_offset *end);
void intel_desc_pool_free(struct intel_desc_pool *pool,
                          const struct intel_desc_offset *begin,
                          const struct intel_desc_offset *end);

XGL_RESULT intel_desc_pool_begin_update(struct intel_desc_pool *pool,
                                        XGL_DESCRIPTOR_UPDATE_MODE mode);
XGL_RESULT intel_desc_pool_end_update(struct intel_desc_pool *pool,
                                      struct intel_cmd *cmd);

void intel_desc_pool_clear(struct intel_desc_pool *pool,
                           const struct intel_desc_offset *begin,
                           const struct intel_desc_offset *end);

void intel_desc_pool_update(struct intel_desc_pool *pool,
                            const struct intel_desc_offset *begin,
                            const struct intel_desc_offset *end,
                            const struct intel_desc_surface *surfaces,
                            const struct intel_desc_sampler *samplers);

void intel_desc_pool_copy(struct intel_desc_pool *pool,
                          const struct intel_desc_offset *begin,
                          const struct intel_desc_offset *end,
                          const struct intel_desc_offset *src);

void intel_desc_pool_read_surface(const struct intel_desc_pool *pool,
                                  const struct intel_desc_offset *offset,
                                  XGL_PIPELINE_SHADER_STAGE stage,
                                  const struct intel_mem **mem,
                                  bool *read_only,
                                  const uint32_t **cmd,
                                  uint32_t *cmd_len);
void intel_desc_pool_read_sampler(const struct intel_desc_pool *pool,
                                  const struct intel_desc_offset *offset,
                                  const struct intel_sampler **sampler);

XGL_RESULT intel_desc_region_create(struct intel_dev *dev,
                                    XGL_DESCRIPTOR_REGION_USAGE usage,
                                    uint32_t max_sets,
                                    const XGL_DESCRIPTOR_REGION_CREATE_INFO *info,
                                    struct intel_desc_region **region_ret);
void intel_desc_region_destroy(struct intel_desc_region *region);

XGL_RESULT intel_desc_region_alloc(struct intel_desc_region *region,
                                   const struct intel_desc_layout *layout,
                                   struct intel_desc_offset *begin,
                                   struct intel_desc_offset *end);
void intel_desc_region_free_all(struct intel_desc_region *region);

XGL_RESULT intel_desc_set_create(struct intel_dev *dev,
                                 struct intel_desc_region *region,
                                 XGL_DESCRIPTOR_SET_USAGE usage,
                                 const struct intel_desc_layout *layout,
                                 struct intel_desc_set **set_ret);
void intel_desc_set_destroy(struct intel_desc_set *set);

void intel_desc_set_update_samplers(struct intel_desc_set *set,
                                    const XGL_UPDATE_SAMPLERS *update);
void intel_desc_set_update_sampler_textures(struct intel_desc_set *set,
                                            const XGL_UPDATE_SAMPLER_TEXTURES *update);
void intel_desc_set_update_images(struct intel_desc_set *set,
                                  const XGL_UPDATE_IMAGES *update);
void intel_desc_set_update_buffers(struct intel_desc_set *set,
                                   const XGL_UPDATE_BUFFERS *update);
void intel_desc_set_update_as_copy(struct intel_desc_set *set,
                                   const XGL_UPDATE_AS_COPY *update);

XGL_RESULT intel_desc_layout_create(struct intel_dev *dev,
                                    XGL_FLAGS stage_flags,
                                    const uint32_t *bind_points,
                                    const struct intel_desc_layout *prior_layout,
                                    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO *info,
                                    struct intel_desc_layout **layout_ret);
void intel_desc_layout_destroy(struct intel_desc_layout *layout);

bool intel_desc_layout_find_bind_point(const struct intel_desc_layout *layout,
                                       XGL_PIPELINE_SHADER_STAGE stage,
                                       uint32_t set, uint32_t binding,
                                       struct intel_desc_layout_iter *iter);

bool intel_desc_layout_find_index(const struct intel_desc_layout *layout,
                                  uint32_t index,
                                  struct intel_desc_layout_iter *iter);

bool intel_desc_layout_advance_iter(const struct intel_desc_layout *layout,
                                    struct intel_desc_layout_iter *iter);

#endif /* DESC_H */
