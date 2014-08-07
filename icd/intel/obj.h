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
 */

#ifndef OBJ_H
#define OBJ_H

#include "intel.h"

struct intel_mem;

struct intel_base_dbg {
    int alloc_id;
    XGL_DBG_OBJECT_TYPE type;

    void *create_info;
    XGL_SIZE create_info_size;

    void *tag;
    XGL_SIZE tag_size;
};

struct intel_base {
    const struct icd_dispatch_table *dispatch;

    struct intel_base_dbg *dbg;

    XGL_RESULT (*get_info)(struct intel_base *base, int type,
                           XGL_SIZE *size, XGL_VOID *data);
};

struct intel_obj {
    struct intel_base base;

    void (*destroy)(struct intel_obj *obj);

    /* for memory binding */
    struct intel_mem *mem;
    XGL_SIZE offset;
};

struct intel_state {
    struct intel_obj obj;
};

static inline struct intel_base *intel_base(XGL_BASE_OBJECT base)
{
    return (struct intel_base *) base;
}

static inline struct intel_obj *intel_obj(XGL_OBJECT obj)
{
    return (struct intel_obj *) obj;
}

static inline struct intel_state *intel_state(XGL_STATE_OBJECT state)
{
    return (struct intel_state *) state;
}

bool intel_base_is_valid(const struct intel_base *base);

XGL_RESULT intel_base_get_info(struct intel_base *base, int type,
                               XGL_SIZE *size, XGL_VOID *data);

struct intel_base_dbg *intel_base_dbg_create(XGL_DBG_OBJECT_TYPE type,
                                             const void *create_info,
                                             XGL_SIZE create_info_size,
                                             XGL_SIZE alloc_size);
void intel_base_dbg_destroy(struct intel_base_dbg *dbg);

XGL_RESULT XGLAPI intelDestroyObject(
    XGL_OBJECT                                  object);

XGL_RESULT XGLAPI intelGetObjectInfo(
    XGL_BASE_OBJECT                             object,
    XGL_OBJECT_INFO_TYPE                        infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

XGL_RESULT XGLAPI intelBindObjectMemory(
    XGL_OBJECT                                  object,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset);

#endif /* OBJ_H */
