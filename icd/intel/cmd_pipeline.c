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
#include "dset.h"
#include "mem.h"
#include "state.h"
#include "view.h"
#include "cmd_priv.h"

XGL_VOID XGLAPI intelCmdBindPipeline(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE                                pipeline)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    switch (pipelineBindPoint) {
    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        cmd->bind.pipeline.compute = pipeline;
        break;
    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        cmd->bind.pipeline.graphics = pipeline;
        break;
    default:
        break;
    }
}

XGL_VOID XGLAPI intelCmdBindPipelineDelta(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE_DELTA                          delta)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    switch (pipelineBindPoint) {
    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        cmd->bind.pipeline.compute_delta = delta;
        break;
    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        cmd->bind.pipeline.graphics_delta = delta;
        break;
    default:
        break;
    }
}

XGL_VOID XGLAPI intelCmdBindStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_STATE_OBJECT                            state)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    switch (stateBindPoint) {
    case XGL_STATE_BIND_VIEWPORT:
        cmd->bind.state.viewport =
            intel_viewport_state((XGL_VIEWPORT_STATE_OBJECT) state);
        break;
    case XGL_STATE_BIND_RASTER:
        cmd->bind.state.raster =
            intel_raster_state((XGL_RASTER_STATE_OBJECT) state);
        break;
    case XGL_STATE_BIND_DEPTH_STENCIL:
        cmd->bind.state.ds =
            intel_ds_state((XGL_DEPTH_STENCIL_STATE_OBJECT) state);
        break;
    case XGL_STATE_BIND_COLOR_BLEND:
        cmd->bind.state.blend =
            intel_blend_state((XGL_COLOR_BLEND_STATE_OBJECT) state);
        break;
    case XGL_STATE_BIND_MSAA:
        cmd->bind.state.msaa =
            intel_msaa_state((XGL_MSAA_STATE_OBJECT) state);
        break;
    default:
        break;
    }
}

XGL_VOID XGLAPI intelCmdBindDescriptorSet(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    index,
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    slotOffset)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_dset *dset = intel_dset(descriptorSet);

    assert(!index);

    switch (pipelineBindPoint) {
    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        cmd->bind.dset.compute = dset;
        cmd->bind.dset.compute_offset = slotOffset;
        break;
    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        cmd->bind.dset.graphics = dset;
        cmd->bind.dset.graphics_offset = slotOffset;
        break;
    default:
        break;
    }
}

XGL_VOID XGLAPI intelCmdBindDynamicMemoryView(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    switch (pipelineBindPoint) {
    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        intel_mem_view_init(&cmd->bind.mem_view.compute, cmd->dev, pMemView);
        break;
    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        intel_mem_view_init(&cmd->bind.mem_view.graphics, cmd->dev, pMemView);
        break;
    default:
        break;
    }
}

static void gen6_3DSTATE_INDEX_BUFFER(struct intel_cmd *cmd,
                                      struct intel_mem *mem,
                                      XGL_GPU_SIZE offset,
                                      XGL_INDEX_TYPE type,
                                      bool enable_cut_index)
{
    const uint8_t cmd_len = 3;
    uint32_t dw0, end_offset;
    unsigned offset_align;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = GEN_RENDER_CMD(3D, 3DSTATE_INDEX_BUFFER) | (cmd_len - 2);

    /* the bit is moved to 3DSTATE_VF */
    if (cmd_gen(cmd) >= INTEL_GEN(7.5))
        assert(!enable_cut_index);
    if (enable_cut_index)
        dw0 |= GEN6_IB_DW0_CUT_INDEX_ENABLE;

    switch (type) {
    case XGL_INDEX_8:
        dw0 |= GEN6_IB_DW0_FORMAT_BYTE;
        offset_align = 1;
        break;
    case XGL_INDEX_16:
        dw0 |= GEN6_IB_DW0_FORMAT_WORD;
        offset_align = 2;
        break;
    case XGL_INDEX_32:
        dw0 |= GEN6_IB_DW0_FORMAT_DWORD;
        offset_align = 4;
        break;
    default:
        cmd->result = XGL_ERROR_INVALID_VALUE;
        return;
        break;
    }

    if (offset % offset_align) {
        cmd->result = XGL_ERROR_INVALID_VALUE;
        return;
    }

    /* aligned and inclusive */
    end_offset = mem->size - (mem->size % offset_align) - 1;

    cmd_reserve(cmd, cmd_len);
    cmd_write(cmd, dw0);
    cmd_write_r(cmd, offset, mem, INTEL_DOMAIN_VERTEX, 0);
    cmd_write_r(cmd, end_offset, mem, INTEL_DOMAIN_VERTEX, 0);
}

XGL_VOID XGLAPI intelCmdBindIndexData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem_,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_mem *mem = intel_mem(mem_);

    if (cmd_gen(cmd) >= INTEL_GEN(7.5)) {
        gen6_3DSTATE_INDEX_BUFFER(cmd, mem, offset, indexType, false);
    } else {
        cmd->bind.index.mem = mem;
        cmd->bind.index.offset = offset;
        cmd->bind.index.type = indexType;
    }
}

XGL_VOID XGLAPI intelCmdBindAttachments(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    colorAttachmentCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*       pColorAttachments,
    const XGL_DEPTH_STENCIL_BIND_INFO*          pDepthStencilAttachment)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    XGL_UINT i;

    for (i = 0; i < colorAttachmentCount; i++) {
        const XGL_COLOR_ATTACHMENT_BIND_INFO *att = &pColorAttachments[i];
        struct intel_rt_view *rt = intel_rt_view(att->view);

        cmd->bind.att.rt[i] = rt;
    }

    cmd->bind.att.rt_count = colorAttachmentCount;

    if (pDepthStencilAttachment) {
        struct intel_ds_view *ds = intel_ds_view(pDepthStencilAttachment->view);
        cmd->bind.att.ds = ds;
    } else {
        cmd->bind.att.ds = NULL;
    }
}

XGL_VOID XGLAPI intelCmdDraw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstVertex,
    XGL_UINT                                    vertexCount,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount)
{
}

XGL_VOID XGLAPI intelCmdDrawIndexed(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstIndex,
    XGL_UINT                                    indexCount,
    XGL_INT                                     vertexOffset,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount)
{
}

XGL_VOID XGLAPI intelCmdDrawIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride)
{
}

XGL_VOID XGLAPI intelCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride)
{
}

XGL_VOID XGLAPI intelCmdDispatch(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    x,
    XGL_UINT                                    y,
    XGL_UINT                                    z)
{
}

XGL_VOID XGLAPI intelCmdDispatchIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset)
{
}
