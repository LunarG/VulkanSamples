/*
 * Vulkan
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

static void semaphore_destroy(struct intel_obj *obj)
{
    struct intel_semaphore *semaphore = intel_semaphore_from_obj(obj);

    intel_semaphore_destroy(semaphore);
}

VkResult intel_semaphore_create(struct intel_dev *dev,
                                const VkSemaphoreCreateInfo *info,
                                struct intel_semaphore **semaphore_ret)
{
    struct intel_semaphore *semaphore;
    semaphore = (struct intel_semaphore *) intel_base_create(&dev->base.handle,
            sizeof(*semaphore), dev->base.dbg, VK_OBJECT_TYPE_SEMAPHORE, info, 0);

    if (!semaphore)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    semaphore->references = 0;
    *semaphore_ret = semaphore;
    semaphore->obj.destroy = semaphore_destroy;

    return VK_SUCCESS;
}

void intel_semaphore_destroy(struct intel_semaphore *semaphore)
{
    intel_base_destroy(&semaphore->obj.base);
}

static void queue_submit_hang(struct intel_queue *queue,
                              struct intel_cmd *cmd,
                              uint32_t active_lost,
                              uint32_t pending_lost)
{
    intel_cmd_decode(cmd, true);

    intel_dev_log(queue->dev, VK_DBG_REPORT_ERROR_BIT,
                  VK_NULL_HANDLE, 0, 0,
                  "GPU hanged with %d/%d active/pending command buffers lost",
                  active_lost, pending_lost);
}

static VkResult queue_submit_bo(struct intel_queue *queue,
                                  struct intel_bo *bo,
                                  VkDeviceSize used)
{
    struct intel_winsys *winsys = queue->dev->winsys;
    int err;

    if (intel_debug & INTEL_DEBUG_NOHW)
        err = 0;
    else
        err = intel_winsys_submit_bo(winsys, queue->ring, bo, used, 0);

    return (err) ? VK_ERROR_DEVICE_LOST : VK_SUCCESS;
}

static struct intel_bo *queue_create_bo(struct intel_queue *queue,
                                        VkDeviceSize size,
                                        const void *cmd,
                                        size_t cmd_len)
{
    struct intel_bo *bo;
    void *ptr;

    bo = intel_winsys_alloc_bo(queue->dev->winsys,
            "queue bo", size, true);
    if (!bo)
        return NULL;

    if (!cmd_len)
        return bo;

    ptr = intel_bo_map(bo, true);
    if (!ptr) {
        intel_bo_unref(bo);
        return NULL;
    }

    memcpy(ptr, cmd, cmd_len);
    intel_bo_unmap(bo);

    return bo;
}

static VkResult queue_select_pipeline(struct intel_queue *queue,
                                        int pipeline_select)
{
    uint32_t pipeline_select_cmd[] = {
        GEN6_RENDER_CMD(SINGLE_DW, PIPELINE_SELECT),
        GEN6_MI_CMD(MI_BATCH_BUFFER_END),
    };
    struct intel_bo *bo;
    VkResult ret;

    if (queue->ring != INTEL_RING_RENDER ||
        queue->last_pipeline_select == pipeline_select)
        return VK_SUCCESS;

    switch (pipeline_select) {
    case GEN6_PIPELINE_SELECT_DW0_SELECT_3D:
        bo = queue->select_graphics_bo;
        break;
    case GEN6_PIPELINE_SELECT_DW0_SELECT_MEDIA:
        bo = queue->select_compute_bo;
        break;
    default:
        assert(0 && "Invalid pipeline select");
        break;
    }

    if (!bo) {
        pipeline_select_cmd[0] |= pipeline_select;
        bo = queue_create_bo(queue, sizeof(pipeline_select_cmd),
                pipeline_select_cmd, sizeof(pipeline_select_cmd));
        if (!bo)
            return VK_ERROR_OUT_OF_DEVICE_MEMORY;

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
    if (ret == VK_SUCCESS)
        queue->last_pipeline_select = pipeline_select;

    return ret;
}

static VkResult queue_init_hw_and_atomic_bo(struct intel_queue *queue)
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
    VkResult ret;

    if (queue->ring != INTEL_RING_RENDER) {
        queue->last_pipeline_select = -1;
        queue->atomic_bo = queue_create_bo(queue,
                sizeof(uint32_t) * INTEL_QUEUE_ATOMIC_COUNTER_COUNT,
                NULL, 0);
        return (queue->atomic_bo) ? VK_SUCCESS : VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    bo = queue_create_bo(queue,
            sizeof(uint32_t) * INTEL_QUEUE_ATOMIC_COUNTER_COUNT,
            ctx_init_cmd, sizeof(ctx_init_cmd));
    if (!bo)
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;

    ret = queue_submit_bo(queue, bo, sizeof(ctx_init_cmd));
    if (ret != VK_SUCCESS) {
        intel_bo_unref(bo);
        return ret;
    }

    queue->last_pipeline_select = GEN6_PIPELINE_SELECT_DW0_SELECT_3D;
    /* reuse */
    queue->atomic_bo = bo;

    return VK_SUCCESS;
}

static VkResult queue_submit_cmd_prepare(struct intel_queue *queue,
                                           struct intel_cmd *cmd)
{
    if (unlikely(cmd->result != VK_SUCCESS || !cmd->primary)) {
        intel_dev_log(cmd->dev, VK_DBG_REPORT_ERROR_BIT,
                      &cmd->obj.base, 0, 0,
                      "invalid command buffer submitted");
    }

    return queue_select_pipeline(queue, cmd->pipeline_select);
}

static VkResult queue_submit_cmd_debug(struct intel_queue *queue,
                                         struct intel_cmd *cmd)
{
    uint32_t active[2], pending[2];
    struct intel_bo *bo;
    VkDeviceSize used;
    VkResult ret;

    ret = queue_submit_cmd_prepare(queue, cmd);
    if (ret != VK_SUCCESS)
        return ret;

    if (intel_debug & INTEL_DEBUG_HANG) {
        intel_winsys_get_reset_stats(queue->dev->winsys,
                &active[0], &pending[0]);
    }

    bo = intel_cmd_get_batch(cmd, &used);
    ret = queue_submit_bo(queue, bo, used);
    if (ret != VK_SUCCESS)
        return ret;

    if (intel_debug & INTEL_DEBUG_HANG) {
        intel_bo_wait(bo, -1);
        intel_winsys_get_reset_stats(queue->dev->winsys,
                &active[1], &pending[1]);

        if (active[0] != active[1] || pending[0] != pending[1]) {
            queue_submit_hang(queue, cmd, active[1] - active[0],
                    pending[1] - pending[0]);
        }
    }

    if (intel_debug & INTEL_DEBUG_BATCH)
        intel_cmd_decode(cmd, false);

    return VK_SUCCESS;
}

static VkResult queue_submit_cmd(struct intel_queue *queue,
                                   struct intel_cmd *cmd)
{
    struct intel_bo *bo;
    VkDeviceSize used;
    VkResult ret;

    ret = queue_submit_cmd_prepare(queue, cmd);
    if (ret == VK_SUCCESS) {
        bo = intel_cmd_get_batch(cmd, &used);
        ret = queue_submit_bo(queue, bo, used);
    }

    return ret;
}

VkResult intel_queue_create(struct intel_dev *dev,
                            enum intel_gpu_engine_type engine,
                            struct intel_queue **queue_ret)
{
    struct intel_queue *queue;
    enum intel_ring_type ring;
    VkFenceCreateInfo fence_info;
    VkResult ret;

    switch (engine) {
    case INTEL_GPU_ENGINE_3D:
        ring = INTEL_RING_RENDER;
        break;
    default:
        intel_dev_log(dev, VK_DBG_REPORT_ERROR_BIT,
                      &dev->base, 0, 0,
                      "invalid engine type");
        return VK_ERROR_VALIDATION_FAILED;
        break;
    }

    queue = (struct intel_queue *) intel_base_create(&dev->base.handle,
            sizeof(*queue), dev->base.dbg, VK_OBJECT_TYPE_QUEUE, NULL, 0);
    if (!queue)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    queue->dev = dev;
    queue->ring = ring;

    if (queue_init_hw_and_atomic_bo(queue) != VK_SUCCESS) {
        intel_queue_destroy(queue);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    memset(&fence_info, 0, sizeof(fence_info));
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    ret = intel_fence_create(dev, &fence_info, &queue->fence);
    if (ret != VK_SUCCESS) {
        intel_queue_destroy(queue);
        return ret;
    }

    *queue_ret = queue;

    return VK_SUCCESS;
}

void intel_queue_destroy(struct intel_queue *queue)
{
    if (queue->fence)
        intel_fence_destroy(queue->fence);

    intel_bo_unref(queue->atomic_bo);
    intel_bo_unref(queue->select_graphics_bo);
    intel_bo_unref(queue->select_compute_bo);

    intel_base_destroy(&queue->base);
}

VkResult intel_queue_wait(struct intel_queue *queue, int64_t timeout)
{
    /* return VK_SUCCESS instead of VK_ERROR_UNAVAILABLE */
    if (!queue->fence->seqno_bo)
        return VK_SUCCESS;

    return intel_fence_wait(queue->fence, timeout);
}

static void intel_wait_queue_semaphore(struct intel_queue *queue, struct intel_semaphore *semaphore)
{
    intel_queue_wait(queue, -1);
    semaphore->references--;
}

static void intel_signal_queue_semaphore(struct intel_queue *queue, struct intel_semaphore *semaphore)
{
    semaphore->references++;
}

ICD_EXPORT VkResult VKAPI vkQueueWaitIdle(
    VkQueue                                   queue_)
{
    struct intel_queue *queue = intel_queue(queue_);

    return intel_queue_wait(queue, -1);
}

ICD_EXPORT VkResult VKAPI vkQueueSubmit(
    VkQueue                                   queue_,
    uint32_t                                  submitCount,
    const VkSubmitInfo*                       pSubmitInfo,
    VkFence                                   fence_)
{
    struct intel_queue *queue = intel_queue(queue_);
    VkResult ret = VK_SUCCESS;
    struct intel_cmd *last_cmd;
    uint32_t i;

    for (uint32_t submit_idx = 0; submit_idx < submitCount; submit_idx++) {

        const VkSubmitInfo *submit = &pSubmitInfo[submit_idx];

        for (i = 0; i < submit->waitSemCount; i++) {
            struct intel_semaphore *pSemaphore = intel_semaphore(submit->pWaitSemaphores[i]);
            intel_wait_queue_semaphore(queue, pSemaphore);
        }

        if (unlikely(intel_debug)) {
            for (i = 0; i < submit->cmdBufferCount; i++) {
                struct intel_cmd *cmd = intel_cmd(submit->pCommandBuffers[i]);
                ret = queue_submit_cmd_debug(queue, cmd);
                if (ret != VK_SUCCESS)
                    break;
            }
        } else {
            for (i = 0; i < submit->cmdBufferCount; i++) {
                struct intel_cmd *cmd = intel_cmd(submit->pCommandBuffers[i]);
                ret = queue_submit_cmd(queue, cmd);
                if (ret != VK_SUCCESS)
                    break;
            }
        }

        /* no cmd submitted */
        if (i == 0)
            return ret;

        last_cmd = intel_cmd(submit->pCommandBuffers[i - 1]);

        if (ret == VK_SUCCESS) {
            intel_fence_set_seqno(queue->fence,
                    intel_cmd_get_batch(last_cmd, NULL));

            if (fence_ != VK_NULL_HANDLE) {
                struct intel_fence *fence = intel_fence(fence_);
                intel_fence_copy(fence, queue->fence);
            }
        } else {
            struct intel_bo *last_bo;

            /* unbusy submitted BOs */
            last_bo = intel_cmd_get_batch(last_cmd, NULL);
            intel_bo_wait(last_bo, -1);
        }

        for (i = 0; i < submit->signalSemCount; i++) {
            struct intel_semaphore *pSemaphore = intel_semaphore(submit->pSignalSemaphores[i]);
            intel_signal_queue_semaphore(queue, pSemaphore);
        }
    }

    return ret;
}

ICD_EXPORT VkResult VKAPI vkCreateSemaphore(
    VkDevice                                device,
    const VkSemaphoreCreateInfo            *pCreateInfo,
    VkSemaphore                            *pSemaphore)
{
    /*
     * We want to find an unused semaphore register and initialize it.  Signal
     * will increment the register.  Wait will atomically decrement it and
     * block if the value is zero, or a large constant N if we do not want to
     * go negative.
     *
     * XXX However, MI_SEMAPHORE_MBOX does not seem to have the flexibility.
     */

    // TODO: fully support semaphores (mean time, simulate it):
    struct intel_dev *dev = intel_dev(device);

    return intel_semaphore_create(dev, pCreateInfo,
           (struct intel_semaphore **) pSemaphore);
}

ICD_EXPORT void VKAPI vkDestroySemaphore(
    VkDevice                                    device,
    VkSemaphore                                 semaphore)
{
    struct intel_obj *obj = intel_obj(semaphore);
    obj->destroy(obj);
}
