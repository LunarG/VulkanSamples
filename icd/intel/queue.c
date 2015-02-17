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

#include "genhw/genhw.h"
#include "kmd/winsys.h"
#include "cmd.h"
#include "dev.h"
#include "fence.h"
#include "queue.h"

static void queue_submit_hang(struct intel_queue *queue,
                              struct intel_cmd *cmd,
                              uint32_t active_lost,
                              uint32_t pending_lost)
{
    intel_cmd_decode(cmd, true);

    intel_dev_log(queue->dev, XGL_DBG_MSG_ERROR,
            XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0, 0,
            "GPU hanged with %d/%d active/pending command buffers lost",
            active_lost, pending_lost);
}

static XGL_RESULT queue_submit_bo(struct intel_queue *queue,
                                  struct intel_bo *bo,
                                  XGL_GPU_SIZE used)
{
    struct intel_winsys *winsys = queue->dev->winsys;
    int err;

    if (intel_debug & INTEL_DEBUG_NOHW)
        err = 0;
    else
        err = intel_winsys_submit_bo(winsys, queue->ring, bo, used, 0);

    return (err) ? XGL_ERROR_UNKNOWN : XGL_SUCCESS;
}

static struct intel_bo *queue_create_bo(struct intel_queue *queue,
                                        XGL_GPU_SIZE size,
                                        const void *cmd,
                                        size_t cmd_len)
{
    struct intel_bo *bo;
    void *ptr;

    bo = intel_winsys_alloc_buffer(queue->dev->winsys,
            "queue bo", size, true);
    if (!bo)
        return NULL;

    if (!cmd_len)
        return bo;

    ptr = intel_bo_map(bo, true);
    if (!ptr) {
        intel_bo_unreference(bo);
        return NULL;
    }

    memcpy(ptr, cmd, cmd_len);
    intel_bo_unmap(bo);

    return bo;
}

static XGL_RESULT queue_select_pipeline(struct intel_queue *queue,
                                        int pipeline_select)
{
    uint32_t pipeline_select_cmd[] = {
        GEN6_RENDER_CMD(SINGLE_DW, PIPELINE_SELECT),
        GEN6_MI_CMD(MI_BATCH_BUFFER_END),
    };
    struct intel_bo *bo;
    XGL_RESULT ret;

    if (queue->ring != INTEL_RING_RENDER ||
        queue->last_pipeline_select == pipeline_select)
        return XGL_SUCCESS;

    switch (pipeline_select) {
    case GEN6_PIPELINE_SELECT_DW0_SELECT_3D:
        bo = queue->select_graphics_bo;
        break;
    case GEN6_PIPELINE_SELECT_DW0_SELECT_MEDIA:
        bo = queue->select_compute_bo;
        break;
    default:
        return XGL_ERROR_INVALID_VALUE;
        break;
    }

    if (!bo) {
        pipeline_select_cmd[0] |= pipeline_select;
        bo = queue_create_bo(queue, sizeof(pipeline_select_cmd),
                pipeline_select_cmd, sizeof(pipeline_select_cmd));
        if (!bo)
            return XGL_ERROR_OUT_OF_GPU_MEMORY;

        switch (pipeline_select) {
        case GEN6_PIPELINE_SELECT_DW0_SELECT_3D:
            queue->select_graphics_bo = bo;
            break;
        case GEN6_PIPELINE_SELECT_DW0_SELECT_MEDIA:
            queue->select_compute_bo = bo;
            break;
        default:
            break;
        }
    }

    ret = queue_submit_bo(queue, bo, sizeof(pipeline_select_cmd));
    if (ret == XGL_SUCCESS)
        queue->last_pipeline_select = pipeline_select;

    return ret;
}

static XGL_RESULT queue_init_hw_and_atomic_bo(struct intel_queue *queue)
{
    const uint32_t ctx_init_cmd[] = {
        /* STATE_SIP */
        GEN6_RENDER_CMD(COMMON, STATE_SIP),
        0,
        /* PIPELINE_SELECT */
        GEN6_RENDER_CMD(SINGLE_DW, PIPELINE_SELECT) |
            GEN6_PIPELINE_SELECT_DW0_SELECT_3D,
        /* 3DSTATE_VF_STATISTICS */
        GEN6_RENDER_CMD(SINGLE_DW, 3DSTATE_VF_STATISTICS),
        /* end */
        GEN6_MI_CMD(MI_BATCH_BUFFER_END),
        GEN6_MI_CMD(MI_NOOP),
    };
    struct intel_bo *bo;
    XGL_RESULT ret;

    if (queue->ring != INTEL_RING_RENDER) {
        queue->last_pipeline_select = -1;
        queue->atomic_bo = queue_create_bo(queue,
                sizeof(uint32_t) * INTEL_QUEUE_ATOMIC_COUNTER_COUNT,
                NULL, 0);
        return (queue->atomic_bo) ? XGL_SUCCESS : XGL_ERROR_OUT_OF_GPU_MEMORY;
    }

    bo = queue_create_bo(queue,
            sizeof(uint32_t) * INTEL_QUEUE_ATOMIC_COUNTER_COUNT,
            ctx_init_cmd, sizeof(ctx_init_cmd));
    if (!bo)
        return XGL_ERROR_OUT_OF_GPU_MEMORY;

    ret = queue_submit_bo(queue, bo, sizeof(ctx_init_cmd));
    if (ret != XGL_SUCCESS) {
        intel_bo_unreference(bo);
        return ret;
    }

    queue->last_pipeline_select = GEN6_PIPELINE_SELECT_DW0_SELECT_3D;
    /* reuse */
    queue->atomic_bo = bo;

    return XGL_SUCCESS;
}

static XGL_RESULT queue_submit_cmd_prepare(struct intel_queue *queue,
                                           struct intel_cmd *cmd)
{
    if (unlikely(cmd->result != XGL_SUCCESS)) {
        intel_dev_log(cmd->dev, XGL_DBG_MSG_ERROR,
                XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0, 0,
                "invalid command buffer submitted");
        return cmd->result;
    }

    return queue_select_pipeline(queue, cmd->pipeline_select);
}

static XGL_RESULT queue_submit_cmd_debug(struct intel_queue *queue,
                                         struct intel_cmd *cmd)
{
    uint32_t active[2], pending[2];
    struct intel_bo *bo;
    XGL_GPU_SIZE used;
    XGL_RESULT ret;

    ret = queue_submit_cmd_prepare(queue, cmd);
    if (ret != XGL_SUCCESS)
        return ret;

    if (intel_debug & INTEL_DEBUG_HANG) {
        intel_winsys_read_reset_stats(queue->dev->winsys,
                &active[0], &pending[0]);
    }

    bo = intel_cmd_get_batch(cmd, &used);
    ret = queue_submit_bo(queue, bo, used);
    if (ret != XGL_SUCCESS)
        return ret;

    if (intel_debug & INTEL_DEBUG_HANG) {
        intel_bo_wait(bo, -1);
        intel_winsys_read_reset_stats(queue->dev->winsys,
                &active[1], &pending[1]);

        if (active[0] != active[1] || pending[0] != pending[1]) {
            queue_submit_hang(queue, cmd, active[1] - active[0],
                    pending[1] - pending[0]);
        }
    }

    if (intel_debug & INTEL_DEBUG_BATCH)
        intel_cmd_decode(cmd, false);

    return XGL_SUCCESS;
}

static XGL_RESULT queue_submit_cmd(struct intel_queue *queue,
                                   struct intel_cmd *cmd)
{
    struct intel_bo *bo;
    XGL_GPU_SIZE used;
    XGL_RESULT ret;

    ret = queue_submit_cmd_prepare(queue, cmd);
    if (ret == XGL_SUCCESS) {
        bo = intel_cmd_get_batch(cmd, &used);
        ret = queue_submit_bo(queue, bo, used);
    }

    return ret;
}

XGL_RESULT intel_queue_create(struct intel_dev *dev,
                              enum intel_gpu_engine_type engine,
                              struct intel_queue **queue_ret)
{
    struct intel_queue *queue;
    enum intel_ring_type ring;

    switch (engine) {
    case INTEL_GPU_ENGINE_3D:
        ring = INTEL_RING_RENDER;
        break;
    default:
        return XGL_ERROR_INVALID_VALUE;
        break;
    }

    queue = (struct intel_queue *) intel_base_create(dev, sizeof(*queue),
            dev->base.dbg, XGL_DBG_OBJECT_QUEUE, NULL, 0);
    if (!queue)
        return XGL_ERROR_OUT_OF_MEMORY;

    queue->dev = dev;
    queue->ring = ring;

    if (queue_init_hw_and_atomic_bo(queue) != XGL_SUCCESS) {
        intel_queue_destroy(queue);
        return XGL_ERROR_INITIALIZATION_FAILED;
    }

    *queue_ret = queue;

    return XGL_SUCCESS;
}

void intel_queue_destroy(struct intel_queue *queue)
{
    if (queue->atomic_bo)
        intel_bo_unreference(queue->atomic_bo);
    if (queue->select_graphics_bo)
        intel_bo_unreference(queue->select_graphics_bo);
    if (queue->select_compute_bo)
        intel_bo_unreference(queue->select_compute_bo);
    intel_base_destroy(&queue->base);
}

XGL_RESULT intel_queue_wait(struct intel_queue *queue, int64_t timeout)
{
    struct intel_bo *bo = (queue->last_submitted_cmd) ?
        intel_cmd_get_batch(queue->last_submitted_cmd, NULL) : NULL;

    return (!bo || intel_bo_wait(bo, timeout) == 0) ?
        XGL_SUCCESS : XGL_ERROR_UNKNOWN;
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(
    XGL_QUEUE                                   queue,
    uint32_t                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs)
{
    /*
     * The winwys maintains the list of memory references.  These are ignored
     * until we move away from the winsys.
     */
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueWaitIdle(
    XGL_QUEUE                                   queue_)
{
    struct intel_queue *queue = intel_queue(queue_);

    return intel_queue_wait(queue, -1);
}

ICD_EXPORT XGL_RESULT XGLAPI xglQueueSubmit(
    XGL_QUEUE                                   queue_,
    uint32_t                                    cmdBufferCount,
    const XGL_CMD_BUFFER*                       pCmdBuffers,
    uint32_t                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs,
    XGL_FENCE                                   fence_)
{
    struct intel_queue *queue = intel_queue(queue_);
    XGL_RESULT ret = XGL_SUCCESS;
    struct intel_cmd *last_cmd;
    uint32_t i;

    /* XGL_MEMORY_REFs are ignored as the winsys already knows them */
    if (unlikely(intel_debug)) {
        for (i = 0; i < cmdBufferCount; i++) {
            struct intel_cmd *cmd = intel_cmd(pCmdBuffers[i]);
            ret = queue_submit_cmd_debug(queue, cmd);
            if (ret != XGL_SUCCESS)
                break;
        }
    } else {
        for (i = 0; i < cmdBufferCount; i++) {
            struct intel_cmd *cmd = intel_cmd(pCmdBuffers[i]);
            ret = queue_submit_cmd(queue, cmd);
            if (ret != XGL_SUCCESS)
                break;
        }
    }

    /* no cmd submitted */
    if (i == 0)
        return ret;

    last_cmd = intel_cmd(pCmdBuffers[i - 1]);

    if (ret == XGL_SUCCESS) {
        queue->last_submitted_cmd = last_cmd;

        if (fence_ != XGL_NULL_HANDLE) {
            struct intel_fence *fence = intel_fence(fence_);
            intel_fence_set_cmd(fence, last_cmd);
        }
    } else {
        struct intel_bo *last_bo;

        /* unbusy submitted BOs */
        last_bo = intel_cmd_get_batch(last_cmd, NULL);
        intel_bo_wait(last_bo, -1);
    }

    return ret;
}

ICD_EXPORT XGL_RESULT XGLAPI xglOpenSharedQueueSemaphore(
    XGL_DEVICE                                  device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO*        pOpenInfo,
    XGL_QUEUE_SEMAPHORE*                        pSemaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateQueueSemaphore(
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

ICD_EXPORT XGL_RESULT XGLAPI xglSignalQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}

ICD_EXPORT XGL_RESULT XGLAPI xglWaitQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore)
{
    return XGL_ERROR_UNAVAILABLE;
}
