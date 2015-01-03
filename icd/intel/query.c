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
#include "mem.h"
#include "query.h"

static void query_destroy(struct intel_obj *obj)
{
    struct intel_query *query = intel_query_from_obj(obj);

    intel_query_destroy(query);
}

static XGL_RESULT query_get_info(struct intel_base *base, int type,
                                 XGL_SIZE *size, XGL_VOID *data)
{
    struct intel_query *query = intel_query_from_base(base);
    XGL_RESULT ret = XGL_SUCCESS;

    switch (type) {
    case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            XGL_MEMORY_REQUIREMENTS *mem_req = data;

            *size = sizeof(XGL_MEMORY_REQUIREMENTS);
            if (data == NULL)
                return ret;
            mem_req->size = query->slot_stride * query->slot_count;
            mem_req->alignment = 64;
            mem_req->heapCount = 1;
            mem_req->heaps[0] = 0;

        }
        break;
    default:
        ret = intel_base_get_info(base, type, size, data);
        break;
    }

    return ret;
}

XGL_RESULT intel_query_create(struct intel_dev *dev,
                              const XGL_QUERY_POOL_CREATE_INFO *info,
                              struct intel_query **query_ret)
{
    struct intel_query *query;

    query = (struct intel_query *) intel_base_create(dev, sizeof(*query),
            dev->base.dbg, XGL_DBG_OBJECT_QUERY_POOL, info, 0);
    if (!query)
        return XGL_ERROR_OUT_OF_MEMORY;

    query->type = info->queryType;
    query->slot_count = info->slots;

    /*
     * For each query type, the GPU will be asked to write the values of some
     * registers to a buffer before and after a sequence of commands.  We will
     * compare the differences to get the query results.
     */
    switch (info->queryType) {
    case XGL_QUERY_OCCLUSION:
        query->slot_stride = u_align(sizeof(uint64_t) * 2, 64);
        break;
    case XGL_QUERY_PIPELINE_STATISTICS:
        query->slot_stride =
            u_align(sizeof(XGL_PIPELINE_STATISTICS_DATA) * 2, 64);
        break;
    default:
        break;
    }

    if (!query->slot_stride) {
        intel_query_destroy(query);
        return XGL_ERROR_INVALID_VALUE;
    }

    query->obj.base.get_info = query_get_info;
    query->obj.destroy = query_destroy;

    *query_ret = query;

    return XGL_SUCCESS;
}

void intel_query_destroy(struct intel_query *query)
{
    intel_base_destroy(&query->obj.base);
}

static void
query_process_occlusion(const struct intel_query *query,
                        XGL_UINT count, const uint8_t *raw,
                        uint64_t *results)
{
    XGL_UINT i;

    for (i = 0; i < count; i++) {
        const uint32_t *pair = (const uint32_t *) raw;

        results[i] = pair[1] - pair[0];
        raw += query->slot_stride;
    }
}

static void
query_process_pipeline_statistics(const struct intel_query *query,
                                  XGL_UINT count, const uint8_t *raw,
                                  XGL_PIPELINE_STATISTICS_DATA *results)
{
    const XGL_UINT num_regs = sizeof(results[0]) / sizeof(uint64_t);
    XGL_UINT i, j;

    for (i = 0; i < count; i++) {
        const uint64_t *before = (const uint64_t *) raw;
        const uint64_t *after = before + num_regs;
        uint64_t *dst = (uint64_t *) (results + i);

        for (j = 0; j < num_regs; j++)
            dst[j] = after[j] - before[j];

        raw += query->slot_stride;
    }
}

XGL_RESULT intel_query_get_results(struct intel_query *query,
                                   XGL_UINT slot_start, XGL_UINT slot_count,
                                   void *results)
{
    const uint8_t *ptr;

    if (!query->obj.mem)
        return XGL_ERROR_MEMORY_NOT_BOUND;

    if (intel_mem_is_busy(query->obj.mem))
        return XGL_NOT_READY;

    ptr = (const uint8_t *) intel_mem_map_sync(query->obj.mem, false);
    if (!ptr)
        return XGL_ERROR_MEMORY_MAP_FAILED;

    ptr += query->obj.offset + query->slot_stride * slot_start;

    switch (query->type) {
    case XGL_QUERY_OCCLUSION:
        query_process_occlusion(query, slot_count, ptr, results);
        break;
    case XGL_QUERY_PIPELINE_STATISTICS:
        query_process_pipeline_statistics(query, slot_count, ptr, results);
        break;
    default:
        assert(0);
        break;
    }

    intel_mem_unmap(query->obj.mem);

    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateQueryPool(
    XGL_DEVICE                                  device,
    const XGL_QUERY_POOL_CREATE_INFO*           pCreateInfo,
    XGL_QUERY_POOL*                             pQueryPool)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_query_create(dev, pCreateInfo,
            (struct intel_query **) pQueryPool);
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetQueryPoolResults(
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    startQuery,
    XGL_UINT                                    queryCount,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    struct intel_query *query = intel_query(queryPool);

    switch (query->type) {
    case XGL_QUERY_OCCLUSION:
        *pDataSize = sizeof(uint64_t) * queryCount;
        break;
    case XGL_QUERY_PIPELINE_STATISTICS:
        *pDataSize = sizeof(XGL_PIPELINE_STATISTICS_DATA) * queryCount;
        break;
    default:
        return XGL_ERROR_INVALID_HANDLE;
        break;
    }

    if (pData)
        return intel_query_get_results(query, startQuery, queryCount, pData);
    else
        return XGL_SUCCESS;
}
