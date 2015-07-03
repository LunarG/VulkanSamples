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

#ifndef BUF_H
#define BUF_H

#include "intel.h"
#include "obj.h"

struct intel_buf {
    struct intel_obj obj;

    VkDeviceSize size;
    VkFlags usage;
};

static inline struct intel_buf *intel_buf(VkBuffer buf)
{
    return *(struct intel_buf **) &buf;
}

static inline struct intel_buf *intel_buf_from_base(struct intel_base *base)
{
    return (struct intel_buf *) base;
}

static inline struct intel_buf *intel_buf_from_obj(struct intel_obj *obj)
{
    return intel_buf_from_base(&obj->base);
}

VkResult intel_buf_create(struct intel_dev *dev,
                            const VkBufferCreateInfo *info,
                            struct intel_buf **buf_ret);

void intel_buf_destroy(struct intel_buf *buf);

#endif /* BUF_H */
