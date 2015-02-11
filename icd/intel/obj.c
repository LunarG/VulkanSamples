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

#include "dev.h"
#include "gpu.h"
#include "mem.h"
#include "obj.h"
#include "xglIcd.h"

static const uint32_t intel_base_magic = 0x494e544c;

/**
 * Return true if an (not so) arbitrary pointer casted to intel_base points to
 * a valid intel_base.  This assumes at least the first
 * sizeof(void*)+sizeof(uint32_t) bytes of the address are accessible, and
 * they does not happen to be our magic values.
 */
bool intel_base_is_valid(const struct intel_base *base,
                         XGL_DBG_OBJECT_TYPE type)
{
    return (base->magic == intel_base_magic + type);
}

XGL_RESULT intel_base_get_info(struct intel_base *base, int type,
                               size_t *size, void *data)
{
    XGL_RESULT ret = XGL_SUCCESS;
    size_t s;
    uint32_t *count;

    switch (type) {
    case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            XGL_MEMORY_REQUIREMENTS *mem_req = data;
            s = sizeof(XGL_MEMORY_REQUIREMENTS);
            *size = s;
            if (data == NULL)
                return ret;
            memset(data, 0, s);
            mem_req->memType =  XGL_MEMORY_TYPE_OTHER;
            break;
        }
    case XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT:
        *size = sizeof(uint32_t);
        if (data == NULL)
            return ret;
        count = (uint32_t *) data;
        *count = 1;
        break;
    case XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS:
        s = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
        *size = s;
        if (data == NULL)
            return ret;
        memset(data, 0, s);
        break;
    case XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS:
        s = sizeof(XGL_BUFFER_MEMORY_REQUIREMENTS);
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
            void *next;
        } *header;
    } info = { .ptr = create_info };
    size_t shallow_copy = 0;

    if (!create_info)
        return true;

    switch (dbg->type) {
    case XGL_DBG_OBJECT_DEVICE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_GPU_MEMORY:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO);
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
    case XGL_DBG_OBJECT_BUFFER:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        shallow_copy = sizeof(XGL_BUFFER_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_BUFFER_VIEW:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);
        shallow_copy = sizeof(XGL_BUFFER_VIEW_CREATE_INFO);
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
        /* no create info */
        break;
    case XGL_DBG_OBJECT_VIEWPORT_STATE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO);
        shallow_copy = sizeof(XGL_DYNAMIC_VP_STATE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_RASTER_STATE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO);
        shallow_copy = sizeof(XGL_DYNAMIC_RS_STATE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_COLOR_BLEND_STATE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO);
        shallow_copy = sizeof(XGL_DYNAMIC_CB_STATE_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_DEPTH_STENCIL_STATE:
        assert(info.header->struct_type == XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO);
        shallow_copy = sizeof(XGL_DYNAMIC_DS_STATE_CREATE_INFO);
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
    case XGL_DBG_OBJECT_FRAMEBUFFER:
        assert(info.header->struct_type ==  XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
        shallow_copy = sizeof(XGL_FRAMEBUFFER_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_RENDER_PASS:
        assert(info.header->struct_type ==  XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
        shallow_copy = sizeof(XGL_RENDER_PASS_CREATE_INFO);
        break;
    case XGL_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT:
        assert(info.header->struct_type ==  XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
        /* TODO */
        shallow_copy = sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) * 0;
        break;
    case XGL_DBG_OBJECT_DESCRIPTOR_REGION:
        assert(info.header->struct_type ==  XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO);
        shallow_copy = sizeof(XGL_DESCRIPTOR_REGION_CREATE_INFO);
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
        dbg->create_info = icd_alloc(shallow_copy, 0, XGL_SYSTEM_ALLOC_DEBUG);
        if (!dbg->create_info)
            return false;

        memcpy(dbg->create_info, create_info, shallow_copy);
        dbg->create_info_size = shallow_copy;
    } else if (info.header->struct_type ==
            XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO) {
        size_t size;
        const XGL_MEMORY_ALLOC_INFO *ptr_next, *src = info.ptr;
        XGL_MEMORY_ALLOC_INFO *dst;
        uint8_t *d;
        size = sizeof(*src);

        ptr_next = src->pNext;
        while (ptr_next != NULL) {
            switch (ptr_next->sType) {
                case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
                    size += sizeof(XGL_MEMORY_ALLOC_IMAGE_INFO);
                    break;
                case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
                    size += sizeof(XGL_MEMORY_ALLOC_BUFFER_INFO);
                    break;
                default:
                    intel_dev_log(dbg->dev, XGL_DBG_MSG_ERROR,
                          XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0, 0,
                          "Invalid Memory Alloc Create Info type: 0x%x",
                          ptr_next->sType);
                    return false;
            }
            ptr_next = (XGL_MEMORY_ALLOC_INFO *) ptr_next->pNext;
        }
        dbg->create_info_size = size;
        dst = icd_alloc(size, 0, XGL_SYSTEM_ALLOC_DEBUG);
        if (!dst)
            return false;
        memcpy(dst, src, sizeof(*src));

        ptr_next = src->pNext;
        d = (uint8_t *) dst;
        d += sizeof(*src);
        while (ptr_next != NULL) {
            switch (ptr_next->sType) {
            case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
                memcpy(d, ptr_next, sizeof(XGL_MEMORY_ALLOC_IMAGE_INFO));
                d += sizeof(XGL_MEMORY_ALLOC_IMAGE_INFO);
                break;
            case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
                memcpy(d, ptr_next, sizeof(XGL_MEMORY_ALLOC_BUFFER_INFO));
                d += sizeof(XGL_MEMORY_ALLOC_BUFFER_INFO);
                break;
            default:
                return false;
            }
            ptr_next = (XGL_MEMORY_ALLOC_INFO *) ptr_next->pNext;
        }
        dbg->create_info = dst;
    } else if (info.header->struct_type ==
            XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO) {
        const XGL_DEVICE_CREATE_INFO *src = info.ptr;
        XGL_DEVICE_CREATE_INFO *dst;
        uint8_t *d;
        size_t size;
        uint32_t i;

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
        dst->ppEnabledExtensionNames = (const char * const *) d;

        for (i = 0; i < src->extensionCount; i++) {
            const size_t len = strlen(src->ppEnabledExtensionNames[i]);

            memcpy(d + size, src->ppEnabledExtensionNames[i], len + 1);
            ((const char **) d)[i] = (const char *) (d + size);

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
                                             size_t dbg_size)
{
    struct intel_base_dbg *dbg;

    if (!dbg_size)
        dbg_size = sizeof(*dbg);

    assert(dbg_size >= sizeof(*dbg));

    dbg = icd_alloc(dbg_size, 0, XGL_SYSTEM_ALLOC_DEBUG);
    if (!dbg)
        return NULL;

    memset(dbg, 0, dbg_size);

    dbg->alloc_id = icd_allocator_get_id();
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
                                     size_t obj_size, bool debug,
                                     XGL_DBG_OBJECT_TYPE type,
                                     const void *create_info,
                                     size_t dbg_size)
{
    struct intel_base *base;

    if (!obj_size)
        obj_size = sizeof(*base);

    assert(obj_size >= sizeof(*base));

    base = icd_alloc(obj_size, 0, XGL_SYSTEM_ALLOC_API_OBJECT);
    if (!base)
        return NULL;

    memset(base, 0, obj_size);
    set_loader_magic_value(base);
    base->magic = intel_base_magic + type;

    if (dev == NULL) {
        /*
         * dev is NULL when we are creating the base device object
         * Set dev now so that debug setup happens correctly
         */
        dev = (struct intel_dev *) base;
    }

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

ICD_EXPORT XGL_RESULT XGLAPI xglDestroyObject(
    XGL_OBJECT                                  object)
{
    struct intel_obj *obj = intel_obj(object);

    obj->destroy(obj);

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetObjectInfo(
    XGL_BASE_OBJECT                             object,
    XGL_OBJECT_INFO_TYPE                        infoType,
    size_t*                                     pDataSize,
    void*                                       pData)
{
    struct intel_base *base = intel_base(object);

    return base->get_info(base, infoType, pDataSize, pData);
}

ICD_EXPORT XGL_RESULT XGLAPI xglBindObjectMemory(
    XGL_OBJECT                                  object,
    uint32_t                                    allocationIdx,
    XGL_GPU_MEMORY                              mem_,
    XGL_GPU_SIZE                                memOffset)
{
    struct intel_obj *obj = intel_obj(object);
    struct intel_mem *mem = intel_mem(mem_);

    intel_obj_bind_mem(obj, mem, memOffset);

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglBindObjectMemoryRange(
    XGL_OBJECT                                  object,
    uint32_t                                    allocationIdx,
    XGL_GPU_SIZE                                rangeOffset,
    XGL_GPU_SIZE                                rangeSize,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                memOffset)
{
    return XGL_ERROR_UNKNOWN;
}

ICD_EXPORT XGL_RESULT XGLAPI xglBindImageMemoryRange(
    XGL_IMAGE                                   image,
    uint32_t                                    allocationIdx,
    const XGL_IMAGE_MEMORY_BIND_INFO*           bindInfo,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                memOffset)
{
    return XGL_ERROR_UNKNOWN;
}

ICD_EXPORT XGL_RESULT XGLAPI xglDbgSetObjectTag(
    XGL_BASE_OBJECT                             object,
    size_t                                      tagSize,
    const void*                                 pTag)
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
