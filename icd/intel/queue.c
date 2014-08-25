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

#include "genhw/genhw.h"
#include "kmd/winsys.h"
#include "cmd.h"
#include "dev.h"
#include "fence.h"
#include "queue.h"

/* must match intel_cmd::pipeline_select */
enum queue_state {
    QUEUE_STATE_GRAPHICS_SELECTED = GEN6_PIPELINE_SELECT_DW0_SELECT_3D,
    QUEUE_STATE_COMPUTE_SELECTED = GEN6_PIPELINE_SELECT_DW0_SELECT_MEDIA,
    QUEUE_STATE_INITIALIZED = -1,
};

static XGL_RESULT queue_submit_bo(struct intel_queue *queue,
                                  struct intel_bo *bo,
                                  XGL_GPU_SIZE used)
{
    struct intel_winsys *winsys = queue->dev->winsys;
    int err;

    if (intel_debug & INTEL_DEBUG_BATCH)
        intel_winsys_decode_bo(winsys, bo, used);

    if (intel_debug & INTEL_DEBUG_NOHW)
        err = 0;
    else
        err = intel_winsys_submit_bo(winsys, queue->ring, bo, used, 0);

    return (err) ? XGL_ERROR_UNKNOWN : XGL_SUCCESS;
}

static XGL_RESULT queue_set_state(struct intel_queue *queue,
                                  enum queue_state state)
{
    static const uint32_t queue_state_init[] = {
        /* STATE_SIP */
        GEN_RENDER_CMD(COMMON, GEN6, STATE_SIP),
        0,
        /* PIPELINE_SELECT */
        GEN_RENDER_CMD(SINGLE_DW, GEN6, PIPELINE_SELECT) |
            GEN6_PIPELINE_SELECT_DW0_SELECT_3D,
        /* 3DSTATE_VF_STATISTICS */
        GEN_RENDER_CMD(SINGLE_DW, GEN6, 3DSTATE_VF_STATISTICS),
        /* end */
        GEN_MI_CMD(MI_BATCH_BUFFER_END),
        GEN_MI_CMD(MI_NOOP),
    };
    static const uint32_t queue_state_select_graphics[] = {
        /* PIPELINE_SELECT */
        GEN_RENDER_CMD(SINGLE_DW, GEN6, PIPELINE_SELECT) |
            GEN6_PIPELINE_SELECT_DW0_SELECT_3D,
        /* end */
        GEN_MI_CMD(MI_BATCH_BUFFER_END),
    };
    static const uint32_t queue_state_select_compute[] = {
        /* PIPELINE_SELECT */
        GEN_RENDER_CMD(SINGLE_DW, GEN6, PIPELINE_SELECT) |
            GEN6_PIPELINE_SELECT_DW0_SELECT_MEDIA,
        /* end */
        GEN_MI_CMD(MI_BATCH_BUFFER_END),
    };
    struct intel_bo *bo;
    XGL_GPU_SIZE size;
    XGL_RESULT ret;

    if (queue->last_pipeline_select == state)
        return XGL_SUCCESS;

    switch (state) {
    case QUEUE_STATE_GRAPHICS_SELECTED:
        bo = queue->select_graphics_bo;
        size = sizeof(queue_state_select_graphics);
        break;
    case QUEUE_STATE_COMPUTE_SELECTED:
        bo = queue->select_compute_bo;
        size = sizeof(queue_state_select_compute);
        break;
    case QUEUE_STATE_INITIALIZED:
        /* will be reused for the atomic counters */
        assert(!queue->atomic_bo);
        bo = NULL;
        size = sizeof(queue_state_init);
        break;
    default:
        return XGL_ERROR_INVALID_VALUE;
        break;
    }

    if (!bo) {
        const void *cmd;
        void *ptr;

        bo = intel_winsys_alloc_buffer(queue->dev->winsys,
                "queue bo", 4096, INTEL_DOMAIN_CPU);
        if (!bo)
            return XGL_ERROR_OUT_OF_GPU_MEMORY;

        /* do the allocation only */
        if (queue->ring != INTEL_RING_RENDER) {
            assert(state == QUEUE_STATE_INITIALIZED);
            queue->atomic_bo = bo;
            queue->last_pipeline_select = QUEUE_STATE_INITIALIZED;
            return XGL_SUCCESS;
        }

        ptr = intel_bo_map(bo, true);
        if (!ptr) {
            intel_bo_unreference(bo);
            return XGL_ERROR_MEMORY_MAP_FAILED;
        }

        switch (state) {
        case QUEUE_STATE_GRAPHICS_SELECTED:
            queue->select_graphics_bo = bo;
            cmd = queue_state_select_graphics;
            break;
        case QUEUE_STATE_COMPUTE_SELECTED:
            queue->select_compute_bo = bo;
            cmd = queue_state_select_compute;
            break;
        case QUEUE_STATE_INITIALIZED:
            /* reused for the atomic counters */
            queue->atomic_bo = bo;
            cmd = queue_state_init;
            break;
        default:
            break;
        }

        memcpy(ptr, cmd, size);
        intel_bo_unmap(bo);
    }

    assert(queue->ring == INTEL_RING_RENDER);

    ret = queue_submit_bo(queue, bo, size);
    if (ret == XGL_SUCCESS) {
        if (state == QUEUE_STATE_INITIALIZED)
            state = QUEUE_STATE_GRAPHICS_SELECTED;
        queue->last_pipeline_select = state;
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

    if (queue_set_state(queue, QUEUE_STATE_INITIALIZED) != XGL_SUCCESS) {
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
    XGL_QUEUE                                   queue_,
    XGL_UINT                                    cmdBufferCount,
    const XGL_CMD_BUFFER*                       pCmdBuffers,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs,
    XGL_FENCE                                   fence_)
{
    struct intel_queue *queue = intel_queue(queue_);
    XGL_RESULT ret = XGL_SUCCESS;
    XGL_UINT i;

    for (i = 0; i < cmdBufferCount; i++) {
        struct intel_cmd *cmd = intel_cmd(pCmdBuffers[i]);
        struct intel_bo *bo;
        XGL_GPU_SIZE used;
        XGL_RESULT ret;

        ret = queue_set_state(queue, cmd->pipeline_select);
        if (ret != XGL_SUCCESS)
            break;

        bo = intel_cmd_get_batch(cmd, &used);
        ret = queue_submit_bo(queue, bo, used);
        queue->last_submitted_cmd = cmd;

        if (ret != XGL_SUCCESS)
            break;
    }

    if (ret == XGL_SUCCESS && fence_ != XGL_NULL_HANDLE) {
        struct intel_fence *fence = intel_fence(fence_);
        intel_fence_set_cmd(fence, queue->last_submitted_cmd);
    }

    /* XGL_MEMORY_REFs are ignored as the winsys already knows them */

    return ret;
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
