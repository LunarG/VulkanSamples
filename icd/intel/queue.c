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

#include "kmd/winsys.h"
#include "dev.h"
#include "queue.h"

XGL_RESULT intel_queue_create(struct intel_dev *dev,
                              XGL_QUEUE_TYPE type,
                              struct intel_queue **queue_ret)
{
    struct intel_queue *queue;

    queue = icd_alloc(sizeof(*queue), 0, XGL_SYSTEM_ALLOC_API_OBJECT);
    if (!queue)
        return XGL_ERROR_OUT_OF_MEMORY;

    memset(queue, 0, sizeof(*queue));
    queue->dev = dev;

    queue->base.dispatch = dev->base.dispatch;
    if (dev->base.dbg) {
        queue->base.dbg = intel_base_dbg_create(XGL_DBG_OBJECT_QUEUE,
                NULL, 0, 0);
        if (!queue->base.dbg) {
            icd_free(queue);
            return XGL_ERROR_OUT_OF_MEMORY;
        }
    }
    queue->base.get_info = intel_base_get_info;

    *queue_ret = queue;

    return XGL_SUCCESS;
}

void intel_queue_destroy(struct intel_queue *queue)
{
    if (queue->base.dbg)
        intel_base_dbg_destroy(queue->base.dbg);
    icd_free(queue);
}

XGL_RESULT intel_queue_wait(struct intel_queue *queue, int64_t timeout)
{
    struct intel_bo *bo = queue->last_submitted_bo;

    return (!bo || intel_bo_wait(bo, timeout) == 0) ?
        XGL_SUCCESS : XGL_ERROR_UNKNOWN;
}

XGL_RESULT XGLAPI intelQueueSetGlobalMemReferences(
    XGL_QUEUE                                   queue,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs)
{
    /*
     * The winwys maintains the list of memory references.  These are ignored
     * until we move away from the winsys.
     */
    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI intelQueueWaitIdle(
    XGL_QUEUE                                   queue_)
{
    struct intel_queue *queue = intel_queue(queue_);

    return intel_queue_wait(queue, -1);
}
