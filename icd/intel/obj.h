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

#ifndef OBJ_H
#define OBJ_H

#include "intel.h"

struct intel_dev;
struct intel_mem;

struct intel_base_dbg {
    VkDbgObjectType type;

    void *create_info;
    size_t create_info_size;

    void *tag;
    size_t tag_size;
};

struct intel_base {
    struct intel_handle handle;

    struct intel_base_dbg *dbg;

    VkResult (*get_memory_requirements)(struct intel_base *base,
                         VkMemoryRequirements *data);
};

struct intel_obj {
    struct intel_base base;

    void (*destroy)(struct intel_obj *obj);

    /* for memory binding */
    struct intel_mem *mem;
    size_t offset;
};

static inline struct intel_base *intel_base(uint64_t base)
{
    return (struct intel_base *) base;
}

static inline struct intel_obj *intel_obj(uint64_t obj)
{
    return (struct intel_obj *) obj;
}

static inline void intel_obj_bind_mem(struct intel_obj *obj,
                                      struct intel_mem *mem,
                                      VkDeviceSize offset)
{
    obj->mem = mem;
    obj->offset = offset;
}

VkResult intel_base_get_memory_requirements(struct intel_base *base, VkMemoryRequirements *data);

struct intel_base_dbg *intel_base_dbg_create(const struct intel_handle *handle,
                                             VkDbgObjectType type,
                                             const void *create_info,
                                             size_t dbg_size);
void intel_base_dbg_destroy(const struct intel_handle *handle,
                            struct intel_base_dbg *dbg);

struct intel_base *intel_base_create(const struct intel_handle *handle,
                                     size_t obj_size, bool debug,
                                     VkDbgObjectType type,
                                     const void *create_info,
                                     size_t dbg_size);
void intel_base_destroy(struct intel_base *base);

#endif /* OBJ_H */
