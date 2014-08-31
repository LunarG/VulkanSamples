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

#include "mem.h"
#include "event.h"
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

    if (cmd_gen(cmd) == INTEL_GEN(6)) {
        dw0 |= GEN6_MI_STORE_REGISTER_MEM_DW0_USE_GGTT;
        reloc_flags |= INTEL_RELOC_GGTT;
    }

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, reg);
    cmd_batch_reloc(cmd, offset, bo, reloc_flags);
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

    if (cmd_gen(cmd) == INTEL_GEN(6)) {
        dw0 |= GEN6_MI_STORE_DATA_IMM_DW0_USE_GGTT;
        reloc_flags |= INTEL_RELOC_GGTT;
    }

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, 0);
    cmd_batch_reloc(cmd, offset, bo, reloc_flags);
    cmd_batch_write(cmd, (uint32_t) val);
    cmd_batch_write(cmd, (uint32_t) (val >> 32));
}

static void cmd_query_pipeline_statistics(struct intel_cmd *cmd,
                                          struct intel_bo *bo,
                                          XGL_GPU_SIZE offset)
{
    const uint32_t regs[] = {
        GEN6_REG_PS_INVOCATION_COUNT,
        GEN6_REG_CL_PRIMITIVES_COUNT,
        GEN6_REG_CL_INVOCATION_COUNT,
        GEN6_REG_VS_INVOCATION_COUNT,
        GEN6_REG_GS_INVOCATION_COUNT,
        GEN6_REG_GS_PRIMITIVES_COUNT,
        /* well, we do not enable 3DSTATE_VF_STATISTICS yet */
        GEN6_REG_IA_PRIMITIVES_COUNT,
        GEN6_REG_IA_VERTICES_COUNT,
        (cmd_gen(cmd) >= INTEL_GEN(7)) ? GEN6_REG_HS_INVOCATION_COUNT : 0,
        (cmd_gen(cmd) >= INTEL_GEN(7)) ? GEN6_REG_DS_INVOCATION_COUNT : 0,
        0,
    };
    XGL_UINT i;

    cmd_batch_flush(cmd, GEN6_PIPE_CONTROL_CS_STALL);

    for (i = 0; i < ARRAY_SIZE(regs); i++) {
        if (regs[i]) {
            /* store lower 32 bits */
            gen6_MI_STORE_REGISTER_MEM(cmd, bo, offset, regs[i]);
            /* store higher 32 bits */
            gen6_MI_STORE_REGISTER_MEM(cmd, bo, offset + 4, regs[i] + 4);
        } else {
            gen6_MI_STORE_DATA_IMM(cmd, bo, offset, 0);
        }

        offset += sizeof(uint64_t);
    }
}

XGL_VOID XGLAPI intelCmdBeginQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot,
    XGL_FLAGS                                   flags)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_query *query = intel_query(queryPool);
    struct intel_bo *bo = query->obj.mem->bo;
    const XGL_GPU_SIZE offset = query->slot_stride * slot;

    switch (query->type) {
    case XGL_QUERY_OCCLUSION:
        cmd_batch_depth_count(cmd, bo, offset);
        break;
    case XGL_QUERY_PIPELINE_STATISTICS:
        cmd_query_pipeline_statistics(cmd, bo, offset);
        break;
    default:
        cmd->result = XGL_ERROR_UNKNOWN;
        break;
    }
}

XGL_VOID XGLAPI intelCmdEndQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_query *query = intel_query(queryPool);
    struct intel_bo *bo = query->obj.mem->bo;
    const XGL_GPU_SIZE offset = query->slot_stride * slot;

    switch (query->type) {
    case XGL_QUERY_OCCLUSION:
        cmd_batch_depth_count(cmd, bo, offset + sizeof(uint64_t));
        break;
    case XGL_QUERY_PIPELINE_STATISTICS:
        cmd_query_pipeline_statistics(cmd, bo,
                offset + sizeof(XGL_PIPELINE_STATISTICS_DATA));
        break;
    default:
        cmd->result = XGL_ERROR_UNKNOWN;
        break;
    }
}

XGL_VOID XGLAPI intelCmdResetQueryPool(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    startQuery,
    XGL_UINT                                    queryCount)
{
    /* no-op */
}

XGL_VOID XGLAPI intelCmdSetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event_)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_event *event = intel_event(event_);

    cmd_batch_immediate(cmd, event->obj.mem->bo, 0, 1);
}

XGL_VOID XGLAPI intelCmdResetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event_)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_event *event = intel_event(event_);

    cmd_batch_immediate(cmd, event->obj.mem->bo, 0, 0);
}

XGL_VOID XGLAPI intelCmdWriteTimestamp(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_TIMESTAMP_TYPE                          timestampType,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_mem *mem = intel_mem(destMem);

    switch (timestampType) {
    case XGL_TIMESTAMP_TOP:
        /* XXX we are not supposed to use two commands... */
        gen6_MI_STORE_REGISTER_MEM(cmd, mem->bo, destOffset, GEN6_REG_TIMESTAMP);
        gen6_MI_STORE_REGISTER_MEM(cmd, mem->bo, destOffset + 4, GEN6_REG_TIMESTAMP + 4);
        break;
    case XGL_TIMESTAMP_BOTTOM:
        cmd_batch_timestamp(cmd, mem->bo, destOffset);
        break;
    default:
        cmd->result = XGL_ERROR_INVALID_VALUE;
        break;
    }
}
