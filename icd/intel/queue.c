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

    queue = (struct intel_queue *) intel_base_create(dev, sizeof(*queue),
            dev->base.dbg, XGL_DBG_OBJECT_QUEUE, NULL, 0);
    if (!queue)
        return XGL_ERROR_OUT_OF_MEMORY;

    queue->dev = dev;

    *queue_ret = queue;

    return XGL_SUCCESS;
}

void intel_queue_destroy(struct intel_queue *queue)
{
    intel_base_destroy(&queue->base);
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

XGL_RESULT XGLAPI intelQueueSubmit(
    XGL_QUEUE                                   queue,
    XGL_UINT                                    cmdBufferCount,
    const XGL_CMD_BUFFER*                       pCmdBuffers,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs,
    XGL_FENCE                                   fence)
{
    /* need XGL_CMD_BUFFER first */
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI intelOpenSharedQueueSemaphore(
    XGL_DEVICE                                  device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO*        pOpenInfo,
    XGL_QUEUE_SEMAPHORE*                        pSemaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI intelCreateQueueSemaphore(
    XGL_DEVICE                                  device,
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO*      pCreateInfo,
    XGL_QUEUE_SEMAPHORE*                        pSemaphore)
{
    /*
     * We want to find an unused semaphore register and initialize it.  Signal
     * will increment the register.  Wait will atomically decrement it and
     * block if the value is zero, or a large constant N if we do not want to
     * go negative.
     *
     * XXX However, MI_SEMAPHORE_MBOX does not seem to have the flexibility.
     */
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI intelSignalQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

XGL_RESULT XGLAPI intelWaitQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}
