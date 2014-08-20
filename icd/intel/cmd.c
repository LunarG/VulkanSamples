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
#include "dev.h"
#include "mem.h"
#include "obj.h"
#include "cmd_priv.h"

/* reserved space used for intel_cmd_end() */
static const XGL_UINT intel_cmd_reserved = 2;

static XGL_RESULT cmd_alloc_and_map(struct intel_cmd *cmd, XGL_SIZE bo_size)
{
    struct intel_winsys *winsys = cmd->dev->winsys;
    struct intel_bo *bo;
    void *ptr;

    bo = intel_winsys_alloc_buffer(winsys,
            "batch buffer", bo_size, INTEL_DOMAIN_CPU);
    if (!bo)
        return XGL_ERROR_OUT_OF_GPU_MEMORY;

    ptr = intel_bo_map(bo, true);
    if (!bo) {
        intel_bo_unreference(bo);
        return XGL_ERROR_MEMORY_MAP_FAILED;
    }

    cmd->bo_size = bo_size;
    cmd->bo = bo;
    cmd->ptr_opaque = ptr;
    cmd->size = bo_size / sizeof(uint32_t) - intel_cmd_reserved;

    return XGL_SUCCESS;
}

static void cmd_unmap(struct intel_cmd *cmd)
{
    intel_bo_unmap(cmd->bo);
    cmd->ptr_opaque = NULL;
}

static void cmd_free(struct intel_cmd *cmd)
{
    intel_bo_unreference(cmd->bo);
    cmd->bo = NULL;
}

static void cmd_reset(struct intel_cmd *cmd)
{
    if (cmd->ptr_opaque)
        cmd_unmap(cmd);
    if (cmd->bo)
        cmd_free(cmd);

    cmd->used = 0;
    cmd->size = 0;
    cmd->reloc_used = 0;
    cmd->result = XGL_SUCCESS;
}

static void cmd_destroy(struct intel_obj *obj)
{
    struct intel_cmd *cmd = intel_cmd_from_obj(obj);

    intel_cmd_destroy(cmd);
}

void cmd_grow(struct intel_cmd *cmd)
{
    const XGL_SIZE bo_size = cmd->bo_size << 1;
    struct intel_bo *old_bo = cmd->bo;
    void *old_ptr = cmd->ptr_opaque;

    if (bo_size >= cmd->bo_size &&
        cmd_alloc_and_map(cmd, bo_size) == XGL_SUCCESS) {
        memcpy(cmd->ptr_opaque, old_ptr, cmd->used * sizeof(uint32_t));

        intel_bo_unmap(old_bo);
        intel_bo_unreference(old_bo);
    } else {
        intel_dev_log(cmd->dev, XGL_DBG_MSG_ERROR,
                XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0, 0,
                "failed to grow command buffer of size %u", cmd->bo_size);

        /* wrap it and fail silently */
        cmd->used = 0;
        cmd->result = XGL_ERROR_OUT_OF_GPU_MEMORY;
    }
}

XGL_RESULT intel_cmd_create(struct intel_dev *dev,
                            const XGL_CMD_BUFFER_CREATE_INFO *info,
                            struct intel_cmd **cmd_ret)
{
    struct intel_cmd *cmd;

    cmd = (struct intel_cmd *) intel_base_create(dev, sizeof(*cmd),
            dev->base.dbg, XGL_DBG_OBJECT_CMD_BUFFER, info, 0);
    if (!cmd)
        return XGL_ERROR_OUT_OF_MEMORY;

    cmd->obj.destroy = cmd_destroy;

    cmd->dev = dev;
    cmd->reloc_count = dev->gpu->batch_buffer_reloc_count;
    cmd->relocs = icd_alloc(sizeof(cmd->relocs[0]) * cmd->reloc_count,
            4096, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!cmd->relocs) {
        intel_cmd_destroy(cmd);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    *cmd_ret = cmd;

    return XGL_SUCCESS;
}

void intel_cmd_destroy(struct intel_cmd *cmd)
{
    cmd_reset(cmd);
    intel_base_destroy(&cmd->obj.base);
}

XGL_RESULT intel_cmd_begin(struct intel_cmd *cmd, XGL_FLAGS flags)
{
    XGL_SIZE bo_size = cmd->bo_size;

    cmd_reset(cmd);

    if (cmd->flags != flags || !bo_size) {
        cmd->flags = flags;

        bo_size = cmd->dev->gpu->max_batch_buffer_size;
        if (flags & XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT)
            bo_size /= 2;

        bo_size &= ~(sizeof(uint32_t) - 1);
    }

    return cmd_alloc_and_map(cmd, bo_size);
}

XGL_RESULT intel_cmd_end(struct intel_cmd *cmd)
{
    struct intel_winsys *winsys = cmd->dev->winsys;
    XGL_UINT i;

    /* reclaim the reserved space */
    cmd->size += intel_cmd_reserved;

    cmd_reserve(cmd, 2);
    cmd_write(cmd, GEN_MI_CMD(MI_BATCH_BUFFER_END));
    /* pad to even dwords */
    if (cmd->used & 1)
        cmd_write(cmd, GEN_MI_CMD(MI_NOOP));

    /* TODO we need a more "explicit" winsys */
    for (i = 0; i < cmd->reloc_count; i++) {
        const struct intel_cmd_reloc *reloc = &cmd->relocs[i];
        uint64_t presumed_offset;
        int err;

        err = intel_bo_add_reloc(cmd->bo, sizeof(uint32_t) * reloc->pos,
                reloc->mem->bo, reloc->val, reloc->read_domains,
                reloc->write_domain, &presumed_offset);
        if (err) {
            cmd->result = XGL_ERROR_UNKNOWN;
            break;
        }

        assert(presumed_offset == (uint64_t) (uint32_t) presumed_offset);
        cmd_patch(cmd, reloc->pos, (uint32_t) presumed_offset);
    }

    cmd_unmap(cmd);

    if (cmd->result != XGL_SUCCESS)
        return cmd->result;
    else if (intel_winsys_can_submit_bo(winsys, &cmd->bo, 1))
        return XGL_SUCCESS;
    else
        return XGL_ERROR_TOO_MANY_MEMORY_REFERENCES;
}

XGL_RESULT XGLAPI intelCreateCommandBuffer(
    XGL_DEVICE                                  device,
    const XGL_CMD_BUFFER_CREATE_INFO*           pCreateInfo,
    XGL_CMD_BUFFER*                             pCmdBuffer)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_cmd_create(dev, pCreateInfo,
            (struct intel_cmd **) pCmdBuffer);
}

XGL_RESULT XGLAPI intelBeginCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_FLAGS                                   flags)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    return intel_cmd_begin(cmd, flags);
}

XGL_RESULT XGLAPI intelEndCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    return intel_cmd_end(cmd);
}

XGL_RESULT XGLAPI intelResetCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_reset(cmd);

    return XGL_SUCCESS;
}

XGL_VOID XGLAPI intelCmdBindPipeline(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE                                pipeline)
{
}

XGL_VOID XGLAPI intelCmdBindPipelineDelta(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE_DELTA                          delta)
{
}

XGL_VOID XGLAPI intelCmdBindStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_STATE_OBJECT                            state)
{
}

XGL_VOID XGLAPI intelCmdBindDescriptorSet(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    index,
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    slotOffset)
{
}

XGL_VOID XGLAPI intelCmdBindDynamicMemoryView(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView)
{
}

XGL_VOID XGLAPI intelCmdBindIndexData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType)
{
}

XGL_VOID XGLAPI intelCmdBindAttachments(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    colorAttachmentCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*       pColorAttachments,
    const XGL_DEPTH_STENCIL_BIND_INFO*          pDepthStencilAttachment)
{
}

XGL_VOID XGLAPI intelCmdPrepareMemoryRegions(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_MEMORY_STATE_TRANSITION*          pStateTransitions)
{
}

XGL_VOID XGLAPI intelCmdPrepareImages(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_IMAGE_STATE_TRANSITION*           pStateTransitions)
{
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

XGL_VOID XGLAPI intelCmdCopyMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_COPY*                      pRegions)
{
}

XGL_VOID XGLAPI intelCmdCopyImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_IMAGE_COPY*                       pRegions)
{
}

XGL_VOID XGLAPI intelCmdCopyMemoryToImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions)
{
}

XGL_VOID XGLAPI intelCmdCopyImageToMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions)
{
}

XGL_VOID XGLAPI intelCmdCloneImageData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_STATE                             srcImageState,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_STATE                             destImageState)
{
}

XGL_VOID XGLAPI intelCmdUpdateMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                dataSize,
    const XGL_UINT32*                           pData)
{
}

XGL_VOID XGLAPI intelCmdFillMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                fillSize,
    XGL_UINT32                                  data)
{
}

XGL_VOID XGLAPI intelCmdClearColorImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_FLOAT                             color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
}

XGL_VOID XGLAPI intelCmdClearColorImageRaw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_UINT32                            color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
}

XGL_VOID XGLAPI intelCmdClearDepthStencil(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    XGL_FLOAT                                   depth,
    XGL_UINT32                                  stencil,
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges)
{
}

XGL_VOID XGLAPI intelCmdResolveImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    rectCount,
    const XGL_IMAGE_RESOLVE*                    pRects)
{
}

XGL_VOID XGLAPI intelCmdSetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event)
{
}

XGL_VOID XGLAPI intelCmdResetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event)
{
}

XGL_VOID XGLAPI intelCmdMemoryAtomic(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_UINT64                                  srcData,
    XGL_ATOMIC_OP                               atomicOp)
{
}

XGL_VOID XGLAPI intelCmdBeginQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot,
    XGL_FLAGS                                   flags)
{
}

XGL_VOID XGLAPI intelCmdEndQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot)
{
}

XGL_VOID XGLAPI intelCmdResetQueryPool(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    startQuery,
    XGL_UINT                                    queryCount)
{
}

XGL_VOID XGLAPI intelCmdWriteTimestamp(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_TIMESTAMP_TYPE                          timestampType,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset)
{
}

XGL_VOID XGLAPI intelCmdInitAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    const XGL_UINT32*                           pData)
{
}

XGL_VOID XGLAPI intelCmdLoadAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_SIZE                                srcOffset)
{
}

XGL_VOID XGLAPI intelCmdSaveAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset)
{
}

XGL_VOID XGLAPI intelCmdDbgMarkerBegin(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_CHAR*                             pMarker)
{
}

XGL_VOID XGLAPI intelCmdDbgMarkerEnd(
    XGL_CMD_BUFFER                              cmdBuffer)
{
}
