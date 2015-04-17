/*
 * Vulkan
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
 * Descriptor region offset (or size) in bytes.
 */
struct intel_desc_offset {
    uint32_t surface;
    uint32_t sampler;
};

struct intel_desc_iter {
    VkDescriptorType type;
    struct intel_desc_offset increment;
    uint32_t size;

    struct intel_desc_offset begin;
    struct intel_desc_offset end;
    uint32_t cur;
};

/**
 * Per-device descriptor region.
 */
struct intel_desc_region {
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

struct intel_desc_pool {
    struct intel_obj obj;

    struct intel_dev *dev;

    /* point to a continuous area in the device's region */
    struct intel_desc_offset region_begin;
    struct intel_desc_offset region_end;

    struct intel_desc_offset cur;
};

struct intel_desc_layout;

struct intel_desc_set {
    struct intel_obj obj;

    /* suballocated from a pool */
    struct intel_desc_region *region;
    struct intel_desc_offset region_begin;
    struct intel_desc_offset region_end;

    const struct intel_desc_layout *layout;
};

struct intel_desc_layout {
    struct intel_obj obj;

    /* homogeneous bindings in this layout */
    struct intel_desc_layout_binding {
        VkDescriptorType type;
        uint32_t array_size;
        const struct intel_sampler **immutable_samplers;
        const struct intel_sampler *shared_immutable_sampler;

        /* to initialize intel_desc_iter */
        struct intel_desc_offset offset;
        struct intel_desc_offset increment;
    } *bindings;
    uint32_t binding_count;

    /* count of _DYNAMIC descriptors */
    uint32_t dynamic_desc_count;

    /* the size of the layout in the region */
    struct intel_desc_offset region_size;
};

struct intel_pipeline_layout {
    struct intel_obj obj;

    struct intel_desc_layout **layouts;
    uint32_t *dynamic_desc_indices;
    uint32_t layout_count;

    uint32_t total_dynamic_desc_count;
};

static inline struct intel_desc_pool *intel_desc_pool(VkDescriptorPool pool)
{
    return (struct intel_desc_pool *) pool;
}

static inline struct intel_desc_pool *intel_desc_pool_from_obj(struct intel_obj *obj)
{
    return (struct intel_desc_pool *) obj;
}

static inline struct intel_desc_set *intel_desc_set(VkDescriptorSet set)
{
    return (struct intel_desc_set *) set;
}

static inline struct intel_desc_set *intel_desc_set_from_obj(struct intel_obj *obj)
{
    return (struct intel_desc_set *) obj;
}

static inline struct intel_desc_layout *intel_desc_layout(VkDescriptorSetLayout layout)
{
    return (struct intel_desc_layout *) layout;
}

static inline struct intel_desc_layout *intel_desc_layout_from_obj(struct intel_obj *obj)
{
    return (struct intel_desc_layout *) obj;
}

static inline struct intel_pipeline_layout *intel_pipeline_layout(VkPipelineLayout pipeline_layout)
{
    return (struct intel_pipeline_layout *) pipeline_layout;
}

static inline struct intel_pipeline_layout *intel_pipeline_layout_from_obj(struct intel_obj *obj)
{
    return (struct intel_pipeline_layout *) obj;
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

bool intel_desc_iter_init_for_binding(struct intel_desc_iter *iter,
                                      const struct intel_desc_layout *layout,
                                      uint32_t binding_index, uint32_t array_base);

bool intel_desc_iter_advance(struct intel_desc_iter *iter);

VkResult intel_desc_region_create(struct intel_dev *dev,
                                    struct intel_desc_region **region_ret);
void intel_desc_region_destroy(struct intel_dev *dev,
                               struct intel_desc_region *region);

VkResult intel_desc_region_alloc(struct intel_desc_region *region,
                                   const VkDescriptorPoolCreateInfo *info,
                                   struct intel_desc_offset *begin,
                                   struct intel_desc_offset *end);
void intel_desc_region_free(struct intel_desc_region *region,
                            const struct intel_desc_offset *begin,
                            const struct intel_desc_offset *end);

VkResult intel_desc_region_begin_update(struct intel_desc_region *region,
                                          VkDescriptorUpdateMode mode);
VkResult intel_desc_region_end_update(struct intel_desc_region *region,
                                        struct intel_cmd *cmd);

void intel_desc_region_clear(struct intel_desc_region *region,
                             const struct intel_desc_offset *begin,
                             const struct intel_desc_offset *end);

void intel_desc_region_update(struct intel_desc_region *region,
                              const struct intel_desc_offset *begin,
                              const struct intel_desc_offset *end,
                              const struct intel_desc_surface *surfaces,
                              const struct intel_desc_sampler *samplers);

void intel_desc_region_copy(struct intel_desc_region *region,
                            const struct intel_desc_offset *begin,
                            const struct intel_desc_offset *end,
                            const struct intel_desc_offset *src);

void intel_desc_region_read_surface(const struct intel_desc_region *region,
                                    const struct intel_desc_offset *offset,
                                    VkShaderStage stage,
                                    const struct intel_mem **mem,
                                    bool *read_only,
                                    const uint32_t **cmd,
                                    uint32_t *cmd_len);
void intel_desc_region_read_sampler(const struct intel_desc_region *region,
                                    const struct intel_desc_offset *offset,
                                    const struct intel_sampler **sampler);

VkResult intel_desc_pool_create(struct intel_dev *dev,
                                  VkDescriptorPoolUsage usage,
                                  uint32_t max_sets,
                                  const VkDescriptorPoolCreateInfo *info,
                                  struct intel_desc_pool **pool_ret);
void intel_desc_pool_destroy(struct intel_desc_pool *pool);

VkResult intel_desc_pool_alloc(struct intel_desc_pool *pool,
                                 const struct intel_desc_layout *layout,
                                 struct intel_desc_offset *begin,
                                 struct intel_desc_offset *end);
void intel_desc_pool_reset(struct intel_desc_pool *pool);

VkResult intel_desc_set_create(struct intel_dev *dev,
                                 struct intel_desc_pool *pool,
                                 VkDescriptorSetUsage usage,
                                 const struct intel_desc_layout *layout,
                                 struct intel_desc_set **set_ret);
void intel_desc_set_destroy(struct intel_desc_set *set);

void intel_desc_set_update_samplers(struct intel_desc_set *set,
                                    const VkUpdateSamplers *update);
void intel_desc_set_update_sampler_textures(struct intel_desc_set *set,
                                            const VkUpdateSamplerTextures *update);
void intel_desc_set_update_images(struct intel_desc_set *set,
                                  const VkUpdateImages *update);
void intel_desc_set_update_buffers(struct intel_desc_set *set,
                                   const VkUpdateBuffers *update);
void intel_desc_set_update_as_copy(struct intel_desc_set *set,
                                   const VkUpdateAsCopy *update);

VkResult intel_desc_layout_create(struct intel_dev *dev,
                                    const VkDescriptorSetLayoutCreateInfo *info,
                                    struct intel_desc_layout **layout_ret);
void intel_desc_layout_destroy(struct intel_desc_layout *layout);

VkResult intel_pipeline_layout_create(struct intel_dev *dev,
                                          const VkPipelineLayoutCreateInfo  *pPipelineCreateInfo,
                                          struct intel_pipeline_layout **pipeline_layout_ret);
void intel_pipeline_layout_destroy(struct intel_pipeline_layout *pipeline_layout);

#endif /* DESC_H */
