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
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#include "genhw/genhw.h"
#include "kmd/winsys.h"
#include "dev.h"
#include "mem.h"
#include "obj.h"
#include "cmd_priv.h"
#include "fb.h"
#include "vk_debug_marker_lunarg.h"

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

    intel_bo_unref(writer->bo);
    writer->bo = NULL;

    writer->used = 0;

    writer->sba_offset = 0;

    if (writer->items) {
        intel_free(cmd, writer->items);
        writer->items = NULL;
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
                                        size_t size)
{
    static const char *writer_names[INTEL_CMD_WRITER_COUNT] = {
        [INTEL_CMD_WRITER_BATCH] = "batch",
        [INTEL_CMD_WRITER_SURFACE] = "surface",
        [INTEL_CMD_WRITER_STATE] = "state",
        [INTEL_CMD_WRITER_INSTRUCTION] = "instruction",
    };

    return intel_winsys_alloc_bo(winsys, writer_names[which], size, true);
}

/**
 * Allocate and map the buffer for writing.
 */
static VkResult cmd_writer_alloc_and_map(struct intel_cmd *cmd,
                                           enum intel_cmd_writer_type which)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];
    struct intel_bo *bo;

    bo = alloc_writer_bo(cmd->dev->winsys, which, writer->size);
    if (bo) {
        intel_bo_unref(writer->bo);
        writer->bo = bo;
    } else if (writer->bo) {
        /* reuse the old bo */
        cmd_writer_discard(cmd, which);
    } else {
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    writer->used = 0;
    writer->item_used = 0;

    writer->ptr = intel_bo_map(writer->bo, true);
    if (!writer->ptr)
        return VK_ERROR_UNKNOWN;

    return VK_SUCCESS;
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
                     size_t new_size)
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
        cmd_fail(cmd, VK_ERROR_OUT_OF_DEVICE_MEMORY);
        return;
    }

    /* map and copy the data over */
    new_ptr = intel_bo_map(new_bo, true);
    if (!new_ptr) {
        intel_bo_unref(new_bo);
        cmd_writer_discard(cmd, which);
        cmd_fail(cmd, VK_ERROR_UNKNOWN);
        return;
    }

    memcpy(new_ptr, writer->ptr, writer->used);

    intel_bo_unmap(writer->bo);
    intel_bo_unref(writer->bo);

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
                       size_t offset, size_t size)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];
    struct intel_cmd_item *item;

    if (writer->item_used == writer->item_alloc) {
        const unsigned new_alloc = (writer->item_alloc) ?
            writer->item_alloc << 1 : 256;
        struct intel_cmd_item *items;

        items = intel_alloc(cmd, sizeof(writer->items[0]) * new_alloc,
                0, VK_SYSTEM_ALLOC_TYPE_DEBUG);
        if (!items) {
            writer->item_used = 0;
            cmd_fail(cmd, VK_ERROR_OUT_OF_HOST_MEMORY);
            return;
        }

        memcpy(items, writer->items,
                sizeof(writer->items[0]) * writer->item_alloc);

        intel_free(cmd, writer->items);

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
                             size_t offset, uint32_t val)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];

    assert(offset + sizeof(val) <= writer->used);
    *((uint32_t *) ((char *) writer->ptr + offset)) = val;
}

static void cmd_reset(struct intel_cmd *cmd)
{
    uint32_t i;

    for (i = 0; i < INTEL_CMD_WRITER_COUNT; i++)
        cmd_writer_reset(cmd, i);

    if (cmd->bind.shader_cache.entries)
        intel_free(cmd, cmd->bind.shader_cache.entries);

    if (cmd->bind.dset.graphics_data.set_offsets)
        intel_free(cmd, cmd->bind.dset.graphics_data.set_offsets);
    if (cmd->bind.dset.graphics_data.dynamic_offsets)
        intel_free(cmd, cmd->bind.dset.graphics_data.dynamic_offsets);
    if (cmd->bind.dset.compute_data.set_offsets)
        intel_free(cmd, cmd->bind.dset.compute_data.set_offsets);
    if (cmd->bind.dset.compute_data.dynamic_offsets)
        intel_free(cmd, cmd->bind.dset.compute_data.dynamic_offsets);

    memset(&cmd->bind, 0, sizeof(cmd->bind));

    cmd->reloc_used = 0;
    cmd->result = VK_SUCCESS;
}

static void cmd_destroy(struct intel_obj *obj)
{
    struct intel_cmd *cmd = intel_cmd_from_obj(obj);

    intel_cmd_destroy(cmd);
}

VkResult intel_cmd_create(struct intel_dev *dev,
                            const VkCmdBufferCreateInfo *info,
                            struct intel_cmd **cmd_ret)
{
    int pipeline_select;
    struct intel_cmd *cmd;

    switch (info->queueNodeIndex) {
    case INTEL_GPU_ENGINE_3D:
        pipeline_select = GEN6_PIPELINE_SELECT_DW0_SELECT_3D;
        break;
    default:
        return VK_ERROR_INVALID_VALUE;
        break;
    }

    cmd = (struct intel_cmd *) intel_base_create(&dev->base.handle,
            sizeof(*cmd), dev->base.dbg, VK_OBJECT_TYPE_COMMAND_BUFFER, info, 0);
    if (!cmd)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    cmd->obj.destroy = cmd_destroy;

    cmd->dev = dev;
    cmd->scratch_bo = dev->cmd_scratch_bo;
    cmd->pipeline_select = pipeline_select;

    /*
     * XXX This is not quite right.  intel_gpu sets maxMemReferences to
     * batch_buffer_reloc_count, but we may emit up to two relocs, for start
     * and end offsets, for each referenced memories.
     */
    cmd->reloc_count = dev->gpu->batch_buffer_reloc_count;
    cmd->relocs = intel_alloc(cmd, sizeof(cmd->relocs[0]) * cmd->reloc_count,
            4096, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!cmd->relocs) {
        intel_cmd_destroy(cmd);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    *cmd_ret = cmd;

    return VK_SUCCESS;
}

void intel_cmd_destroy(struct intel_cmd *cmd)
{
    cmd_reset(cmd);

    intel_free(cmd, cmd->relocs);
    intel_base_destroy(&cmd->obj.base);
}

VkResult intel_cmd_begin(struct intel_cmd *cmd, const VkCmdBufferBeginInfo *info)
{
    const VkCmdBufferGraphicsBeginInfo *ginfo;
    VkResult ret;
    uint32_t i;
    VkFlags flags = 0;

    cmd_reset(cmd);

    while (info != NULL) {
        switch (info->sType) {
        case VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO:
            flags = info->flags;
            break;
        case VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
            ginfo = (const VkCmdBufferGraphicsBeginInfo *) info;
            cmd_begin_render_pass(cmd, intel_render_pass(ginfo->renderPassContinue.renderPass),
                                  intel_fb(ginfo->renderPassContinue.framebuffer));
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
            break;
        }

        info = (const VkCmdBufferBeginInfo*) info->pNext;
    }

    if (cmd->flags != flags) {
        cmd->flags = flags;
        cmd->writers[INTEL_CMD_WRITER_BATCH].size = 0;
    }

    if (!cmd->writers[INTEL_CMD_WRITER_BATCH].size) {
        const uint32_t size = cmd->dev->gpu->max_batch_buffer_size / 2;
        uint32_t divider = 1;

        if (flags & VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT)
            divider *= 4;

        cmd->writers[INTEL_CMD_WRITER_BATCH].size = size / divider;
        cmd->writers[INTEL_CMD_WRITER_SURFACE].size = size / divider / 2;
        cmd->writers[INTEL_CMD_WRITER_STATE].size = size / divider / 2;
        cmd->writers[INTEL_CMD_WRITER_INSTRUCTION].size = 16384 / divider;
    }

    for (i = 0; i < INTEL_CMD_WRITER_COUNT; i++) {
        ret = cmd_writer_alloc_and_map(cmd, i);
        if (ret != VK_SUCCESS) {
            cmd_reset(cmd);
            return  ret;
        }
    }

    cmd_batch_begin(cmd);

    return VK_SUCCESS;
}

VkResult intel_cmd_end(struct intel_cmd *cmd)
{
    struct intel_winsys *winsys = cmd->dev->winsys;
    uint32_t i;

    /* no matching intel_cmd_begin() */
    if (!cmd->writers[INTEL_CMD_WRITER_BATCH].ptr)
        return VK_ERROR_INCOMPLETE_COMMAND_BUFFER;

    cmd_batch_end(cmd);

    /* TODO we need a more "explicit" winsys */
    for (i = 0; i < cmd->reloc_used; i++) {
        const struct intel_cmd_reloc *reloc = &cmd->relocs[i];
        const struct intel_cmd_writer *writer = &cmd->writers[reloc->which];
        uint64_t presumed_offset;
        int err;

        /*
         * Once a bo is used as a reloc target, libdrm_intel disallows more
         * relocs to be added to it.  That may happen when
         * INTEL_CMD_RELOC_TARGET_IS_WRITER is set.  We have to process them
         * in another pass.
         */
        if (reloc->flags & INTEL_CMD_RELOC_TARGET_IS_WRITER)
            continue;

        err = intel_bo_add_reloc(writer->bo, reloc->offset,
                (struct intel_bo *) reloc->target, reloc->target_offset,
                reloc->flags, &presumed_offset);
        if (err) {
            cmd_fail(cmd, VK_ERROR_UNKNOWN);
            break;
        }

        assert(presumed_offset == (uint64_t) (uint32_t) presumed_offset);
        cmd_writer_patch(cmd, reloc->which, reloc->offset,
                (uint32_t) presumed_offset);
    }
    for (i = 0; i < cmd->reloc_used; i++) {
        const struct intel_cmd_reloc *reloc = &cmd->relocs[i];
        const struct intel_cmd_writer *writer = &cmd->writers[reloc->which];
        uint64_t presumed_offset;
        int err;

        if (!(reloc->flags & INTEL_CMD_RELOC_TARGET_IS_WRITER))
            continue;

        err = intel_bo_add_reloc(writer->bo, reloc->offset,
                cmd->writers[reloc->target].bo, reloc->target_offset,
                reloc->flags & ~INTEL_CMD_RELOC_TARGET_IS_WRITER,
                &presumed_offset);
        if (err) {
            cmd_fail(cmd, VK_ERROR_UNKNOWN);
            break;
        }

        assert(presumed_offset == (uint64_t) (uint32_t) presumed_offset);
        cmd_writer_patch(cmd, reloc->which, reloc->offset,
                (uint32_t) presumed_offset);
    }

    for (i = 0; i < INTEL_CMD_WRITER_COUNT; i++)
        cmd_writer_unmap(cmd, i);

    if (cmd->result != VK_SUCCESS)
        return cmd->result;

    if (intel_winsys_can_submit_bo(winsys,
                &cmd->writers[INTEL_CMD_WRITER_BATCH].bo, 1))
        return VK_SUCCESS;
    else
        return VK_ERROR_UNKNOWN;
}

ICD_EXPORT VkResult VKAPI vkCreateCommandBuffer(
    VkDevice                                  device,
    const VkCmdBufferCreateInfo*           pCreateInfo,
    VkCmdBuffer*                             pCmdBuffer)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_cmd_create(dev, pCreateInfo,
            (struct intel_cmd **) pCmdBuffer);
}

ICD_EXPORT VkResult VKAPI vkBeginCommandBuffer(
    VkCmdBuffer                              cmdBuffer,
    const VkCmdBufferBeginInfo            *info)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    return intel_cmd_begin(cmd, info);
}

ICD_EXPORT VkResult VKAPI vkEndCommandBuffer(
    VkCmdBuffer                              cmdBuffer)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    return intel_cmd_end(cmd);
}

ICD_EXPORT VkResult VKAPI vkResetCommandBuffer(
    VkCmdBuffer                              cmdBuffer)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_reset(cmd);

    return VK_SUCCESS;
}
