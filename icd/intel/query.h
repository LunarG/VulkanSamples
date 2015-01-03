/*
 * XGL
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

#ifndef QUERY_H
#define QUERY_H

#include "intel.h"
#include "obj.h"

struct intel_query {
    struct intel_obj obj;

    XGL_QUERY_TYPE type;
    XGL_UINT slot_stride;
    XGL_UINT slot_count;
};

static inline struct intel_query *intel_query(XGL_QUERY_POOL pool)
{
    return (struct intel_query *) pool;
}

static inline struct intel_query *intel_query_from_base(struct intel_base *base)
{
    return (struct intel_query *) base;
}

static inline struct intel_query *intel_query_from_obj(struct intel_obj *obj)
{
    return intel_query_from_base(&obj->base);
}

XGL_RESULT intel_query_create(struct intel_dev *dev,
                              const XGL_QUERY_POOL_CREATE_INFO *info,
                              struct intel_query **query_ret);
void intel_query_destroy(struct intel_query *query);

XGL_RESULT intel_query_get_results(struct intel_query *query,
                                   XGL_UINT slot_start, XGL_UINT slot_count,
                                   void *results);

#endif /* QUERY_H */
