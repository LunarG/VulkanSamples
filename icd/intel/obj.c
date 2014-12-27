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

#include "dispatch.h"
#include "dev.h"
#include "gpu.h"
#include "mem.h"
#include "obj.h"

/**
 * Return true if an (not so) arbitrary pointer casted to intel_base points to
 * a valid intel_base.  This assumes at least the first sizeof(void*) bytes of
 * the address are accessible, and they does not happen to be our magic
 * values.
 */
bool intel_base_is_valid(const struct intel_base *base)
{
    if (base->dispatch != intel_dispatch_get(true) &&
        base->dispatch != intel_dispatch_get(false))
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
        *size = s;
        if (data == NULL)
            return ret;
        memset(data, 0, s);

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

    switch (dbg->type) {
    case XGL_DBG_OBJECT_DEVICE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_GPU_MEMORY:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO);
        shallow_copy = sizeof(XGL_MEMORY_ALLOC_INFO);
        break;
    case XGL_DBG_OBJECT_EVENT:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO);
        shallow_copy = sizeof(XGL_EVENT_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_FENCE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        shallow_copy = sizeof(XGL_FENCE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_QUERY_POOL:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);
        shallow_copy = sizeof(XGL_QUERY_POOL_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_IMAGE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        shallow_copy = sizeof(XGL_IMAGE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_IMAGE_VIEW:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        shallow_copy = sizeof(XGL_IMAGE_VIEW_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_COLOR_TARGET_VIEW:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO);
        shallow_copy = sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_DEPTH_STENCIL_VIEW:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO);
        shallow_copy = sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_SAMPLER:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        shallow_copy = sizeof(XGL_SAMPLER_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_DESCRIPTOR_SET:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO);
        shallow_copy = sizeof(XGL_DESCRIPTOR_SET_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_VIEWPORT_STATE:
        /* no struct header! */
        shallow_copy = sizeof(XGL_VIEWPORT_STATE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_RASTER_STATE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO);
        shallow_copy = sizeof(XGL_RASTER_STATE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_MSAA_STATE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO);
        shallow_copy = sizeof(XGL_MSAA_STATE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_COLOR_BLEND_STATE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO);
        shallow_copy = sizeof(XGL_COLOR_BLEND_STATE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_DEPTH_STENCIL_STATE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO);
        shallow_copy = sizeof(XGL_DEPTH_STENCIL_STATE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_CMD_BUFFER:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO);
        shallow_copy = sizeof(XGL_CMD_BUFFER_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_GRAPHICS_PIPELINE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_SHADER:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO);
        shallow_copy = sizeof(XGL_SHADER_CREATE_INFO);
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
        /* XGL_VIEWPORT_STATE_CREATE_INFO has no header */
        if (dbg->type != XGL_DBG_OBJECT_VIEWPORT_STATE)
            assert(!info.header->next);

        dbg->create_info = icd_alloc(shallow_copy, 0, XGL_SYSTEM_ALLOC_DEBUG);
        if (!dbg->create_info)
            return false;

        memcpy(dbg->create_info, create_info, shallow_copy);
        dbg->create_info_size = shallow_copy;
    } else if (info.header->struct_type ==
            XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO) {
        const XGL_DEVICE_CREATE_INFO *src = info.ptr;
        XGL_DEVICE_CREATE_INFO *dst;
        uint8_t *d;
        XGL_SIZE size;
        XGL_UINT i;

        size = sizeof(*src);
        dbg->create_info_size = size;

        size += sizeof(src->pRequestedQueues[0]) * src->queueRecordCount;
        size += sizeof(src->ppEnabledExtensionNames[0]) * src->extensionCount;
        for (i = 0; i < src->extensionCount; i++) {
            size += 1 + strlen(src->ppEnabledExtensionNames[i]);
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
            const XGL_SIZE len = strlen(src->ppEnabledExtensionNames[i]);

            memcpy(d + size, src->ppEnabledExtensionNames[i], len + 1);
            ((const XGL_CHAR **) d)[i] = (const XGL_CHAR *) (d + size);

            size += len + 1;
        }
        dbg->create_info = dst;
    } else if (info.header->struct_type == XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO) {
        // TODO: What do we want to copy here?
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

    base->dispatch = intel_dispatch_get(debug);
    if (debug) {
        base->dbg = intel_base_dbg_create(dev, type, create_info, dbg_size);
        if (!base->dbg) {
            icd_free(base);
            return NULL;
        }
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
    XGL_GPU_MEMORY                              mem_,
    XGL_GPU_SIZE                                offset)
{
    struct intel_obj *obj = intel_obj(object);
    struct intel_mem *mem = intel_mem(mem_);

    intel_obj_bind_mem(obj, mem, offset);

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelDbgSetObjectTag(
    XGL_BASE_OBJECT                             object,
    XGL_SIZE                                    tagSize,
    const XGL_VOID*                             pTag)
{
    struct intel_base *base = intel_base(object);
    struct intel_base_dbg *dbg = base->dbg;
    void *tag;

    if (!dbg)
        return XGL_SUCCESS;

    tag = icd_alloc(tagSize, 0, XGL_SYSTEM_ALLOC_DEBUG);
    if (!tag)
        return XGL_ERROR_OUT_OF_MEMORY;

    memcpy(tag, pTag, tagSize);

    if (dbg->tag)
        icd_free(dbg->tag);

    dbg->tag = tag;
    dbg->tag_size = tagSize;

    return XGL_SUCCESS;
}
