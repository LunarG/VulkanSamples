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

#include "dispatch_tables.h"
#include "gpu.h"
#include "obj.h"

/**
 * Return true if an (not so) arbitrary pointer casted to intel_base points to
 * a valid intel_base.  This assumes at least the first sizeof(void*) bytes of
 * the address are accessible, and they does not happen to be our magic
 * values.
 */
bool intel_base_is_valid(const struct intel_base *base)
{
    if (base->dispatch != &intel_normal_dispatch_table &&
        base->dispatch != &intel_debug_dispatch_table)
        return false;

    return !intel_gpu_is_valid((const struct intel_gpu *) base);
}

XGL_RESULT intel_base_get_info(struct intel_base *base, int type,
                               XGL_SIZE *size, XGL_VOID *data)
{
    XGL_RESULT ret = XGL_SUCCESS;
    XGL_SIZE s;

    switch (type) {
    case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
        s = sizeof(XGL_MEMORY_REQUIREMENTS);
        memset(data, 0, s);
        *size = s;
        break;
    default:
        ret = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}

/**
 * Initialize intel_base_dbg.  It is assumed that the struct has already been
 * zero initialized.
 */
bool intel_base_dbg_init(struct intel_base_dbg *dbg,
                         XGL_DBG_OBJECT_TYPE type,
                         const void *create_info,
                         XGL_SIZE create_info_size)
{
    dbg->alloc_id = icd_get_allocator_id();
    dbg->type = type;

    if (create_info_size) {
        dbg->create_info =
            icd_alloc(create_info_size, 0, XGL_SYSTEM_ALLOC_DEBUG);
        if (!dbg->create_info)
            return false;

        memcpy(dbg->create_info, create_info, create_info_size);
    }

    return true;
}

void intel_base_dbg_cleanup(struct intel_base_dbg *dbg)
{
    if (dbg->tag)
        icd_free(dbg->tag);

    if (dbg->create_info)
        icd_free(dbg->create_info);
}

struct intel_base_dbg *intel_base_dbg_create(XGL_DBG_OBJECT_TYPE type,
                                             const void *create_info,
                                             XGL_SIZE create_info_size)
{
    struct intel_base_dbg *dbg;

    dbg = icd_alloc(sizeof(*dbg), 0, XGL_SYSTEM_ALLOC_DEBUG);
    if (!dbg)
        return NULL;

    memset(dbg, 0, sizeof(*dbg));
    if (!intel_base_dbg_init(dbg, type, create_info, create_info_size)) {
        icd_free(dbg);
        return NULL;
    }

    return dbg;
}

void intel_base_dbg_destroy(struct intel_base_dbg *dbg)
{
    intel_base_dbg_cleanup(dbg);
    icd_free(dbg);
}

XGL_RESULT XGLAPI intelDestroyObject(
    XGL_OBJECT                                  object)
{
    struct intel_obj *obj = intel_obj(object);

    obj->destroy(obj);

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelGetObjectInfo(
    XGL_BASE_OBJECT                             object,
    XGL_OBJECT_INFO_TYPE                        infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    struct intel_base *base = intel_base(object);

    return base->get_info(base, infoType, pDataSize, pData);
}

XGL_RESULT XGLAPI intelBindObjectMemory(
    XGL_OBJECT                                  object,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset)
{
    struct intel_obj *obj = intel_obj(object);

    obj->mem = mem;
    obj->offset = offset;

    return XGL_SUCCESS;
}
