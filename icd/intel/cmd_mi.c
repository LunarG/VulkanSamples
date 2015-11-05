/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 *
 */

#include "kmd/winsys.h"
#include "buf.h"
#include "event.h"
#include "mem.h"
#include "obj.h"
#include "query.h"
#include "cmd_priv.h"

static void gen6_MI_STORE_REGISTER_MEM(struct intel_cmd *cmd,
                                       struct intel_bo *bo,
                                       uint32_t offset,
                                       uint32_t reg)
{
    const uint8_t cmd_len = 3;
    uint32_t dw0 = GEN6_MI_CMD(MI_STORE_REGISTER_MEM) |
                   (cmd_len - 2);
    uint32_t reloc_flags = INTEL_RELOC_WRITE;
    uint32_t *dw;
    uint32_t pos;

    if (cmd_gen(cmd) == INTEL_GEN(6)) {
        dw0 |= GEN6_MI_STORE_REGISTER_MEM_DW0_USE_GGTT;
        reloc_flags |= INTEL_RELOC_GGTT;
    }

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = reg;

    cmd_reserve_reloc(cmd, 1);
    cmd_batch_reloc(cmd, pos + 2, bo, offset, reloc_flags);
}

static void gen6_MI_STORE_DATA_IMM(struct intel_cmd *cmd,
                                   struct intel_bo *bo,
                                   uint32_t offset,
                                   uint64_t val)
{
    const uint8_t cmd_len = 5;
    uint32_t dw0 = GEN6_MI_CMD(MI_STORE_DATA_IMM) |
                   (cmd_len - 2);
    uint32_t reloc_flags = INTEL_RELOC_WRITE;
    uint32_t *dw;
    uint32_t pos;

    if (cmd_gen(cmd) == INTEL_GEN(6)) {
        dw0 |= GEN6_MI_STORE_DATA_IMM_DW0_USE_GGTT;
        reloc_flags |= INTEL_RELOC_GGTT;
    }

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = 0;
    dw[3] = (uint32_t) val;
    dw[4] = (uint32_t) (val >> 32);

    cmd_reserve_reloc(cmd, 1);
    cmd_batch_reloc(cmd, pos + 2, bo, offset, reloc_flags);
}

static void cmd_query_pipeline_statistics(struct intel_cmd *cmd,
                                          const struct intel_query *query,
                                          struct intel_bo *bo,
                                          VkDeviceSize offset)
{
    uint32_t i;

    cmd_batch_flush(cmd, GEN6_PIPE_CONTROL_CS_STALL);

    for (i = 0; i < query->reg_count; i++) {
        if (query->regs[i]) {
            /* store lower 32 bits */
            gen6_MI_STORE_REGISTER_MEM(cmd, bo, offset, query->regs[i]);
            /* store higher 32 bits */
            gen6_MI_STORE_REGISTER_MEM(cmd, bo, offset + 4, query->regs[i] + 4);
        } else {
            gen6_MI_STORE_DATA_IMM(cmd, bo, offset, 0);
        }

        offset += sizeof(uint64_t);
    }
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery(
    VkCommandBuffer                                 commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    slot,
    VkQueryControlFlags                         flags)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);
    struct intel_query *query = intel_query(queryPool);
    struct intel_bo *bo = query->obj.mem->bo;
    const VkDeviceSize offset = query->slot_stride * slot;

    switch (query->type) {
    case VK_QUERY_TYPE_OCCLUSION:
        cmd_batch_depth_count(cmd, bo, offset);
        break;
    case VK_QUERY_TYPE_PIPELINE_STATISTICS:
        cmd_query_pipeline_statistics(cmd, query, bo, offset);
        break;
    default:
        /* TODOVV: validate */
        cmd_fail(cmd, VK_ERROR_VALIDATION_FAILED);
        break;
    }
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery(
    VkCommandBuffer                                 commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    slot)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);
    struct intel_query *query = intel_query(queryPool);
    struct intel_bo *bo = query->obj.mem->bo;
    const VkDeviceSize offset = query->slot_stride * slot;

    switch (query->type) {
    case VK_QUERY_TYPE_OCCLUSION:
        cmd_batch_depth_count(cmd, bo, offset + sizeof(uint64_t));
        break;
    case VK_QUERY_TYPE_PIPELINE_STATISTICS:
        cmd_query_pipeline_statistics(cmd, query, bo, offset + query->slot_stride);
        break;
    default:
        /* TODOVV: validate */
        cmd_fail(cmd, VK_ERROR_VALIDATION_FAILED);
        break;
    }
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool(
    VkCommandBuffer                                 commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount)
{
    /* no-op */
}

static void cmd_write_event_value(struct intel_cmd *cmd, struct intel_event *event,
                            VkPipelineStageFlags stageMask, uint32_t value)
{
    uint32_t pipe_control_flags = 0;

    /* Event setting is done with PIPE_CONTROL post-sync write immediate.
     * With no other PIPE_CONTROL flags set, it behaves as VK_PIPE_EVENT_TOP_OF_PIPE.
     * All other pipeEvent values will behave as VK_PIPE_EVENT_COMMANDS_COMPLETE.
     */
    if (stageMask & ~VK_PIPELINE_STAGE_HOST_BIT) {
        pipe_control_flags = GEN6_PIPE_CONTROL_CS_STALL;
    }

    cmd_batch_immediate(cmd, pipe_control_flags, event->obj.mem->bo, 0, value);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent(
    VkCommandBuffer                              commandBuffer,
    VkEvent                                  event_,
    VkPipelineStageFlags                     stageMask)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);
    struct intel_event *event = intel_event(event_);

    cmd_write_event_value(cmd, event, stageMask, 1);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent(
    VkCommandBuffer                              commandBuffer,
    VkEvent                                  event_,
    VkPipelineStageFlags                     stageMask)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);
    struct intel_event *event = intel_event(event_);

    cmd_write_event_value(cmd, event, stageMask, 0);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults(
    VkCommandBuffer                                 commandBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    VkBuffer                                    dstBuffer,
    VkDeviceSize                                dstOffset,
    VkDeviceSize                                stride,
    VkFlags                                     flags)
{
    /* TODO: Fill in functionality here */
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp(
    VkCommandBuffer                              commandBuffer,
    VkPipelineStageFlagBits                     pipelineStage,
    VkQueryPool                                 queryPool,
    uint32_t                                    slot)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);
    struct intel_query *query = intel_query(queryPool);
    struct intel_bo *bo = query->obj.mem->bo;
    const VkDeviceSize offset = query->slot_stride * slot;

    if ((pipelineStage & ~VK_PIPELINE_STAGE_HOST_BIT) &&
        pipelineStage != VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
        cmd_batch_timestamp(cmd, bo, offset);
    } else {
        /* XXX we are not supposed to use two commands... */
        gen6_MI_STORE_REGISTER_MEM(cmd, bo, offset, GEN6_REG_TIMESTAMP);
        gen6_MI_STORE_REGISTER_MEM(cmd, bo, offset + 4,
                GEN6_REG_TIMESTAMP + 4);
    }
}
