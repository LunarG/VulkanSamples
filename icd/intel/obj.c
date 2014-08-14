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
#include "dev.h"
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

static bool base_dbg_copy_create_info(struct intel_base_dbg *dbg,
                                      const void *create_info)
{
    const union {
        const void *ptr;
        const struct {
            XGL_STRUCTURE_TYPE struct_type;
            XGL_VOID *next;
        } *header;
    } info = { .ptr = create_info };
    XGL_SIZE shallow_copy = 0;

    if (!create_info)
        return true;

    switch (info.header->struct_type) {
    case XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
        assert(dbg->type == XGL_DBG_OBJECT_DEVICE);
        break;
    case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO:
        assert(dbg->type == XGL_DBG_OBJECT_GPU_MEMORY);
        shallow_copy = sizeof(XGL_MEMORY_ALLOC_INFO);
        break;
    case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
        assert(dbg->type == XGL_DBG_OBJECT_EVENT);
        shallow_copy = sizeof(XGL_EVENT_CREATE_INFO);
        break;
    case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
        assert(dbg->type == XGL_DBG_OBJECT_FENCE);
        shallow_copy = sizeof(XGL_FENCE_CREATE_INFO);
        break;
    case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
        assert(dbg->type == XGL_DBG_OBJECT_QUERY_POOL);
        shallow_copy = sizeof(XGL_QUERY_POOL_CREATE_INFO);
        break;
    case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
        assert(dbg->type == XGL_DBG_OBJECT_IMAGE);
        shallow_copy = sizeof(XGL_IMAGE_CREATE_INFO);
        break;
    default:
        // log debug message regarding invalid struct_type?
        intel_dev_log(dbg->dev, XGL_DBG_MSG_ERROR,
                      XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0, 0,
                      "Invalid Create Info type: 0x%x", info.header->struct_type);
        return false;
        break;
    }

    if (shallow_copy) {
        assert(!info.header->next);

        dbg->create_info = icd_alloc(shallow_copy, 0, XGL_SYSTEM_ALLOC_DEBUG);
        if (!dbg->create_info)
            return false;

        memcpy(dbg->create_info, create_info, shallow_copy);
    } else if (info.header->struct_type ==
            XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO) {
        const XGL_DEVICE_CREATE_INFO *src = info.ptr;
        XGL_DEVICE_CREATE_INFO *dst;
        uint8_t *d;
        XGL_SIZE size;
        XGL_UINT i;

        size = sizeof(*src);
        size += sizeof(src->pRequestedQueues[0]) * src->queueRecordCount;
        size += sizeof(src->ppEnabledExtensionNames[0]) * src->extensionCount;
        for (i = 0; i < src->extensionCount; i++) {
            size += 1 +
                strlen((const char *) src->ppEnabledExtensionNames[i]);
        }

        dst = icd_alloc(size, 0, XGL_SYSTEM_ALLOC_DEBUG);
        if (!dst)
            return false;

        memcpy(dst, src, sizeof(*src));

        d = (uint8_t *) dst;
        d += sizeof(*src);

        size = sizeof(src->pRequestedQueues[0]) * src->queueRecordCount;
        memcpy(d, src->pRequestedQueues, size);
        dst->pRequestedQueues = (const XGL_DEVICE_QUEUE_CREATE_INFO *) d;
        d += size;

        size = sizeof(src->ppEnabledExtensionNames[0]) * src->extensionCount;
        dst->ppEnabledExtensionNames = (const XGL_CHAR * const *) d;

        for (i = 0; i < src->extensionCount; i++) {
            const XGL_SIZE len =
                strlen((const char *) src->ppEnabledExtensionNames[i]);

            memcpy(d + size, src->ppEnabledExtensionNames[i], len + 1);
            ((const XGL_CHAR **) d)[i] = (const XGL_CHAR *) (d + size);

            size += len + 1;
        }
    }

    return true;
}

/**
 * Create an intel_base_dbg.  When dbg_size is non-zero, a buffer of that
 * size is allocated and zeroed.
 */
struct intel_base_dbg *intel_base_dbg_create(struct intel_dev *dev,
                                             XGL_DBG_OBJECT_TYPE type,
                                             const void *create_info,
                                             XGL_SIZE dbg_size)
{
    struct intel_base_dbg *dbg;

    if (!dbg_size)
        dbg_size = sizeof(*dbg);

    assert(dbg_size >= sizeof(*dbg));

    dbg = icd_alloc(dbg_size, 0, XGL_SYSTEM_ALLOC_DEBUG);
    if (!dbg)
        return NULL;

    memset(dbg, 0, dbg_size);

    dbg->alloc_id = icd_get_allocator_id();
    dbg->type = type;
    dbg->dev = dev;

    if (!base_dbg_copy_create_info(dbg, create_info)) {
        icd_free(dbg);
        return NULL;
    }

    return dbg;
}

void intel_base_dbg_destroy(struct intel_base_dbg *dbg)
{
    if (dbg->tag)
        icd_free(dbg->tag);

    if (dbg->create_info)
        icd_free(dbg->create_info);

    icd_free(dbg);
}

/**
 * Create an intel_base.  obj_size and dbg_size specify the real sizes of the
 * object and the debug metadata.  Memories are zeroed.
 */
struct intel_base *intel_base_create(struct intel_dev *dev,
                                     XGL_SIZE obj_size, bool debug,
                                     XGL_DBG_OBJECT_TYPE type,
                                     const void *create_info,
                                     XGL_SIZE dbg_size)
{
    struct intel_base *base;

    if (!obj_size)
        obj_size = sizeof(*base);

    assert(obj_size >= sizeof(*base));

    base = icd_alloc(obj_size, 0, XGL_SYSTEM_ALLOC_API_OBJECT);
    if (!base)
        return NULL;

    if (dev == NULL) {
        /*
         * dev is NULL when we are creating the base device object
         * Set dev now so that debug setup happens correctly
         */
        dev = (struct intel_dev *) base;
    }

    memset(base, 0, obj_size);

    if (debug) {
        base->dispatch = &intel_debug_dispatch_table;
        base->dbg = intel_base_dbg_create(dev, type, create_info, dbg_size);
        if (!base->dbg) {
            icd_free(base);
            return NULL;
        }
    }
    else {
        base->dispatch = &intel_normal_dispatch_table;
    }
    base->get_info = intel_base_get_info;

    return base;
}

void intel_base_destroy(struct intel_base *base)
{
    if (base->dbg)
        intel_base_dbg_destroy(base->dbg);
    icd_free(base);
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
