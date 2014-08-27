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
#include "cmd_pipeline.h"

static XGL_RESULT cmd_writer_alloc_and_map(struct intel_cmd *cmd,
                                           struct intel_cmd_writer *writer,
                                           XGL_UINT size)
{
    struct intel_winsys *winsys = cmd->dev->winsys;
    const XGL_GPU_SIZE bo_size = sizeof(uint32_t) * size;
    struct intel_bo *bo;
    void *ptr;

    bo = intel_winsys_alloc_buffer(winsys,
            "batch buffer", bo_size, true);
    if (!bo)
        return XGL_ERROR_OUT_OF_GPU_MEMORY;

    ptr = intel_bo_map(bo, true);
    if (!bo) {
        intel_bo_unreference(bo);
        return XGL_ERROR_MEMORY_MAP_FAILED;
    }

    writer->bo = bo;
    writer->ptr_opaque = ptr;
    writer->size = size;
    writer->used = 0;

    return XGL_SUCCESS;
}

static void cmd_writer_copy(struct intel_cmd *cmd,
                            struct intel_cmd_writer *writer,
                            const uint32_t *vals, XGL_UINT len)
{
    assert(writer->used + len <= writer->size);
    memcpy((uint32_t *) writer->ptr_opaque + writer->used,
            vals, sizeof(uint32_t) * len);
    writer->used += len;
}

static void cmd_writer_patch(struct intel_cmd *cmd,
                             struct intel_cmd_writer *writer,
                             XGL_UINT pos, uint32_t val)
{
    assert(pos < writer->used);
    ((uint32_t *) writer->ptr_opaque)[pos] = val;
}

void cmd_writer_grow(struct intel_cmd *cmd,
                     struct intel_cmd_writer *writer)
{
    const XGL_UINT size = writer->size << 1;
    const XGL_UINT old_used = writer->used;
    struct intel_bo *old_bo = writer->bo;
    void *old_ptr = writer->ptr_opaque;

    if (size >= writer->size &&
        cmd_writer_alloc_and_map(cmd, writer, size) == XGL_SUCCESS) {
        cmd_writer_copy(cmd, writer, (const uint32_t *) old_ptr, old_used);

        intel_bo_unmap(old_bo);
        intel_bo_unreference(old_bo);
    } else {
        intel_dev_log(cmd->dev, XGL_DBG_MSG_ERROR,
                XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0, 0,
                "failed to grow command buffer of size %u", writer->size);

        /* wrap it and fail silently */
        writer->used = 0;
        cmd->result = XGL_ERROR_OUT_OF_GPU_MEMORY;
    }
}

static void cmd_writer_unmap(struct intel_cmd *cmd,
                             struct intel_cmd_writer *writer)
{
    intel_bo_unmap(writer->bo);
    writer->ptr_opaque = NULL;
}

static void cmd_writer_free(struct intel_cmd *cmd,
                            struct intel_cmd_writer *writer)
{
    intel_bo_unreference(writer->bo);
    writer->bo = NULL;
}

static void cmd_writer_reset(struct intel_cmd *cmd,
                             struct intel_cmd_writer *writer)
{
    /* do not reset writer->size as we want to know how big it has grown to */
    writer->used = 0;

    if (writer->ptr_opaque)
        cmd_writer_unmap(cmd, writer);
    if (writer->bo)
        cmd_writer_free(cmd, writer);
}

static void cmd_unmap(struct intel_cmd *cmd)
{
    cmd_writer_unmap(cmd, &cmd->batch);
    cmd_writer_unmap(cmd, &cmd->state);
    cmd_writer_unmap(cmd, &cmd->kernel);
}

static void cmd_reset(struct intel_cmd *cmd)
{
    // Release any shaders referenced by the command buffer.
    cmd_clear_shader_cache(cmd);

    cmd_writer_reset(cmd, &cmd->batch);
    cmd_writer_reset(cmd, &cmd->state);
    cmd_writer_reset(cmd, &cmd->kernel);

    memset(&cmd->bind, 0, sizeof(cmd->bind));

    cmd->reloc_used = 0;
    cmd->result = XGL_SUCCESS;
}

static void cmd_destroy(struct intel_obj *obj)
{
    struct intel_cmd *cmd = intel_cmd_from_obj(obj);

    intel_cmd_destroy(cmd);
}

XGL_RESULT intel_cmd_create(struct intel_dev *dev,
                            const XGL_CMD_BUFFER_CREATE_INFO *info,
                            struct intel_cmd **cmd_ret)
{
    int pipeline_select;
    struct intel_cmd *cmd;

    switch (info->queueType) {
    case XGL_QUEUE_TYPE_GRAPHICS:
        pipeline_select = GEN6_PIPELINE_SELECT_DW0_SELECT_3D;
        break;
    case XGL_QUEUE_TYPE_COMPUTE:
        pipeline_select = GEN6_PIPELINE_SELECT_DW0_SELECT_MEDIA;
        break;
    case XGL_QUEUE_TYPE_DMA:
        pipeline_select = -1;
        break;
    default:
        return XGL_ERROR_INVALID_VALUE;
        break;
    }

    cmd = (struct intel_cmd *) intel_base_create(dev, sizeof(*cmd),
            dev->base.dbg, XGL_DBG_OBJECT_CMD_BUFFER, info, 0);
    if (!cmd)
        return XGL_ERROR_OUT_OF_MEMORY;

    cmd->obj.destroy = cmd_destroy;

    cmd->dev = dev;
    cmd->scratch_bo = dev->cmd_scratch_bo;
    cmd->pipeline_select = pipeline_select;

    /*
     * Allocate space to keep track of all the shaders used by this
     * command buffer. This will be grown dynamically if needed.
     * Align to pointer size since this is just a list of shader pointers.
     * TODO: Setting initial size of list to arbitrary 16, should this
     * be configurable?
     */
    cmd->bind.shaderCache.shaderList = icd_alloc(sizeof(struct intel_shader) * 16,
                                                 sizeof(struct intel_shader *),
                                                 XGL_SYSTEM_ALLOC_INTERNAL_SHADER);
    cmd->bind.shaderCache.size = 16;
    cmd->bind.shaderCache.used = 0;

    /*
     * XXX This is not quite right.  intel_gpu sets maxMemRefsPerSubmission to
     * batch_buffer_reloc_count, but we may emit up to two relocs, for start
     * and end offsets, for each referenced memories.
     */
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

    icd_free(cmd->bind.shaderCache.shaderList);

    icd_free(cmd->relocs);
    intel_base_destroy(&cmd->obj.base);
}

XGL_RESULT intel_cmd_begin(struct intel_cmd *cmd, XGL_FLAGS flags)
{
    XGL_RESULT ret;

    cmd_reset(cmd);

    if (cmd->flags != flags) {
        cmd->flags = flags;
        cmd->batch.size = 0;
    }

    if (!cmd->batch.size) {
        const XGL_UINT size =
            cmd->dev->gpu->max_batch_buffer_size / sizeof(uint32_t) / 2;
        XGL_UINT divider = 1;

        if (flags & XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT)
            divider *= 4;

        cmd->batch.size = size / divider;
        cmd->state.size = size / divider;
        cmd->kernel.size = 16384 / sizeof(uint32_t) / divider;
    }

    ret = cmd_writer_alloc_and_map(cmd, &cmd->batch, cmd->batch.size);
    if (ret == XGL_SUCCESS)
        ret = cmd_writer_alloc_and_map(cmd, &cmd->state, cmd->state.size);
    if (ret == XGL_SUCCESS)
        ret = cmd_writer_alloc_and_map(cmd, &cmd->kernel, cmd->kernel.size);
    if (ret != XGL_SUCCESS) {
        cmd_reset(cmd);
        return ret;
    }

    cmd_batch_begin(cmd);

    return XGL_SUCCESS;
}

XGL_RESULT intel_cmd_end(struct intel_cmd *cmd)
{
    struct intel_winsys *winsys = cmd->dev->winsys;
    XGL_UINT i;

    cmd_batch_end(cmd);

    /* TODO we need a more "explicit" winsys */
    for (i = 0; i < cmd->reloc_used; i++) {
        const struct intel_cmd_reloc *reloc = &cmd->relocs[i];
        uint64_t presumed_offset;
        int err;

        err = intel_bo_add_reloc(reloc->writer->bo,
                sizeof(uint32_t) * reloc->pos, reloc->bo, reloc->val,
                reloc->flags, &presumed_offset);
        if (err) {
            cmd->result = XGL_ERROR_UNKNOWN;
            break;
        }

        assert(presumed_offset == (uint64_t) (uint32_t) presumed_offset);
        cmd_writer_patch(cmd, reloc->writer, reloc->pos,
                (uint32_t) presumed_offset);
    }

    cmd_unmap(cmd);

    if (cmd->result != XGL_SUCCESS)
        return cmd->result;

    if (intel_winsys_can_submit_bo(winsys, &cmd->batch.bo, 1))
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
