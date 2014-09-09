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
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#include "genhw/genhw.h"
#include "kmd/winsys.h"
#include "dev.h"
#include "mem.h"
#include "obj.h"
#include "cmd_priv.h"

/**
 * Free all resources used by a writer.  Note that the initial size is not
 * reset.
 */
static void cmd_writer_reset(struct intel_cmd *cmd,
                             enum intel_cmd_writer_type which)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];

    if (writer->ptr) {
        intel_bo_unmap(writer->bo);
        writer->ptr = NULL;
    }

    if (writer->bo) {
        intel_bo_unreference(writer->bo);
        writer->bo = NULL;
    }

    writer->used = 0;

    if (writer->items) {
        icd_free(writer->items);
        writer->item_alloc = 0;
        writer->item_used = 0;
    }
}

/**
 * Discard everything written so far.
 */
static void cmd_writer_discard(struct intel_cmd *cmd,
                               enum intel_cmd_writer_type which)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];

    intel_bo_truncate_relocs(writer->bo, 0);
    writer->used = 0;
    writer->item_used = 0;
}

static struct intel_bo *alloc_writer_bo(struct intel_winsys *winsys,
                                        enum intel_cmd_writer_type which,
                                        XGL_SIZE size)
{
    static const char *writer_names[INTEL_CMD_WRITER_COUNT] = {
        [INTEL_CMD_WRITER_BATCH] = "batch",
        [INTEL_CMD_WRITER_INSTRUCTION] = "instruction",
    };

    return intel_winsys_alloc_buffer(winsys, writer_names[which], size, true);
}

/**
 * Allocate and map the buffer for writing.
 */
static XGL_RESULT cmd_writer_alloc_and_map(struct intel_cmd *cmd,
                                           enum intel_cmd_writer_type which)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];
    struct intel_bo *bo;

    bo = alloc_writer_bo(cmd->dev->winsys, which, writer->size);
    if (bo) {
        if (writer->bo)
            intel_bo_unreference(writer->bo);
        writer->bo = bo;
    } else if (writer->bo) {
        /* reuse the old bo */
        cmd_writer_discard(cmd, which);
    } else {
        return XGL_ERROR_OUT_OF_GPU_MEMORY;
    }

    writer->used = 0;
    writer->item_used = 0;

    writer->ptr = intel_bo_map(writer->bo, true);
    if (!writer->ptr)
        return XGL_ERROR_UNKNOWN;

    return XGL_SUCCESS;
}

/**
 * Unmap the buffer for submission.
 */
static void cmd_writer_unmap(struct intel_cmd *cmd,
                             enum intel_cmd_writer_type which)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];

    intel_bo_unmap(writer->bo);
    writer->ptr = NULL;
}

/**
 * Grow a mapped writer to at least \p new_size.  Failures are handled
 * silently.
 */
void cmd_writer_grow(struct intel_cmd *cmd,
                     enum intel_cmd_writer_type which,
                     XGL_SIZE new_size)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];
    struct intel_bo *new_bo;
    void *new_ptr;

    if (new_size < writer->size << 1)
        new_size = writer->size << 1;
    /* STATE_BASE_ADDRESS requires page-aligned buffers */
    new_size = u_align(new_size, 4096);

    new_bo = alloc_writer_bo(cmd->dev->winsys, which, new_size);
    if (!new_bo) {
        cmd_writer_discard(cmd, which);
        cmd->result = XGL_ERROR_OUT_OF_GPU_MEMORY;
        return;
    }

    /* map and copy the data over */
    new_ptr = intel_bo_map(new_bo, true);
    if (!new_ptr) {
        intel_bo_unreference(new_bo);
        cmd_writer_discard(cmd, which);
        cmd->result = XGL_ERROR_UNKNOWN;
        return;
    }

    memcpy(new_ptr, writer->ptr, writer->used);

    intel_bo_unmap(writer->bo);
    intel_bo_unreference(writer->bo);

    writer->size = new_size;
    writer->bo = new_bo;
    writer->ptr = new_ptr;
}

/**
 * Record an item for later decoding.
 */
void cmd_writer_record(struct intel_cmd *cmd,
                       enum intel_cmd_writer_type which,
                       enum intel_cmd_item_type type,
                       XGL_SIZE offset, XGL_SIZE size)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];
    struct intel_cmd_item *item;

    if (writer->item_used == writer->item_alloc) {
        const unsigned new_alloc = (writer->item_alloc) ?
            writer->item_alloc << 1 : 256;
        struct intel_cmd_item *items;

        items = icd_alloc(sizeof(writer->items[0]) * new_alloc,
                0, XGL_SYSTEM_ALLOC_DEBUG);
        if (!items) {
            writer->item_used = 0;
            cmd->result = XGL_ERROR_OUT_OF_MEMORY;
            return;
        }

        memcpy(items, writer->items,
                sizeof(writer->items[0]) * writer->item_alloc);

        icd_free(writer->items);

        writer->items = items;
        writer->item_alloc = new_alloc;
    }

    item = &writer->items[writer->item_used++];
    item->type = type;
    item->offset = offset;
    item->size = size;
}

static void cmd_writer_patch(struct intel_cmd *cmd,
                             enum intel_cmd_writer_type which,
                             XGL_SIZE offset, uint32_t val)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];

    assert(offset + sizeof(val) <= writer->used);
    *((uint32_t *) ((char *) writer->ptr + offset)) = val;
}

static void cmd_reset(struct intel_cmd *cmd)
{
    XGL_UINT i;

    for (i = 0; i < INTEL_CMD_WRITER_COUNT; i++)
        cmd_writer_reset(cmd, i);

    if (cmd->bind.shaderCache.shaderArray)
        icd_free(cmd->bind.shaderCache.shaderArray);
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

    icd_free(cmd->relocs);
    intel_base_destroy(&cmd->obj.base);
}

XGL_RESULT intel_cmd_begin(struct intel_cmd *cmd, XGL_FLAGS flags)
{
    XGL_RESULT ret;
    XGL_UINT i;

    cmd_reset(cmd);

    if (cmd->flags != flags) {
        cmd->flags = flags;
        cmd->writers[INTEL_CMD_WRITER_BATCH].size = 0;
    }

    if (!cmd->writers[INTEL_CMD_WRITER_BATCH].size) {
        const XGL_UINT size = cmd->dev->gpu->max_batch_buffer_size / 2;
        XGL_UINT divider = 1;

        if (flags & XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT)
            divider *= 4;

        cmd->writers[INTEL_CMD_WRITER_BATCH].size = size / divider;
        cmd->writers[INTEL_CMD_WRITER_STATE].size = size / divider;
        cmd->writers[INTEL_CMD_WRITER_INSTRUCTION].size = 16384 / divider;
    }

    for (i = 0; i < INTEL_CMD_WRITER_COUNT; i++) {
        ret = cmd_writer_alloc_and_map(cmd, i);
        if (ret != XGL_SUCCESS) {
            cmd_reset(cmd);
            return  ret;
        }
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
        const struct intel_cmd_writer *writer = &cmd->writers[reloc->which];
        uint64_t presumed_offset;
        int err;

        err = intel_bo_add_reloc(writer->bo, reloc->offset,
                reloc->bo, reloc->bo_offset,
                reloc->flags, &presumed_offset);
        if (err) {
            cmd->result = XGL_ERROR_UNKNOWN;
            break;
        }

        assert(presumed_offset == (uint64_t) (uint32_t) presumed_offset);
        cmd_writer_patch(cmd, reloc->which, reloc->offset,
                (uint32_t) presumed_offset);
    }

    for (i = 0; i < INTEL_CMD_WRITER_COUNT; i++)
        cmd_writer_unmap(cmd, i);

    if (cmd->result != XGL_SUCCESS)
        return cmd->result;

    if (intel_winsys_can_submit_bo(winsys,
                &cmd->writers[INTEL_CMD_WRITER_BATCH].bo, 1))
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

XGL_VOID XGLAPI intelCmdMemoryAtomic(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_UINT64                                  srcData,
    XGL_ATOMIC_OP                               atomicOp)
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
