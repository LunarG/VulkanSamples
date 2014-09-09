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

#ifndef CMD_PRIV_H
#define CMD_PRIV_H

#include "genhw/genhw.h"
#include "dev.h"
#include "gpu.h"
#include "cmd.h"

#define CMD_ASSERT(cmd, min_gen, max_gen) \
    INTEL_GPU_ASSERT((cmd)->dev->gpu, (min_gen), (max_gen))

enum intel_cmd_item_type {
    /* for state buffer */
    INTEL_CMD_ITEM_BLOB,
    INTEL_CMD_ITEM_CLIP_VIEWPORT,
    INTEL_CMD_ITEM_SF_VIEWPORT,
    INTEL_CMD_ITEM_SCISSOR_RECT,
    INTEL_CMD_ITEM_CC_VIEWPORT,
    INTEL_CMD_ITEM_COLOR_CALC,
    INTEL_CMD_ITEM_DEPTH_STENCIL,
    INTEL_CMD_ITEM_BLEND,
    INTEL_CMD_ITEM_SAMPLER,

    /* for surface buffer */
    INTEL_CMD_ITEM_SURFACE,
    INTEL_CMD_ITEM_BINDING_TABLE,

    /* for instruction buffer */
    INTEL_CMD_ITEM_KERNEL,

    INTEL_CMD_ITEM_COUNT,
};

struct intel_cmd_item {
    enum intel_cmd_item_type type;
    XGL_SIZE offset;
    XGL_SIZE size;
};

struct intel_cmd_reloc {
    enum intel_cmd_writer_type which;
    XGL_SIZE offset;

    struct intel_bo *bo;
    uint32_t bo_offset;

    uint32_t flags;
};

static inline int cmd_gen(const struct intel_cmd *cmd)
{
    return intel_gpu_gen(cmd->dev->gpu);
}

static inline void cmd_reserve_reloc(struct intel_cmd *cmd,
                                     XGL_UINT reloc_len)
{
    /* fail silently */
    if (cmd->reloc_used + reloc_len > cmd->reloc_count) {
        cmd->reloc_used = 0;
        cmd->result = XGL_ERROR_TOO_MANY_MEMORY_REFERENCES;
    }
    assert(cmd->reloc_used + reloc_len <= cmd->reloc_count);
}

void cmd_writer_grow(struct intel_cmd *cmd,
                     enum intel_cmd_writer_type which,
                     XGL_SIZE new_size);

void cmd_writer_record(struct intel_cmd *cmd,
                       enum intel_cmd_writer_type which,
                       enum intel_cmd_item_type type,
                       XGL_SIZE offset, XGL_SIZE size);

/**
 * Return an offset to a region that is aligned to \p alignment and has at
 * least \p size bytes.
 */
static inline XGL_SIZE cmd_writer_reserve(struct intel_cmd *cmd,
                                          enum intel_cmd_writer_type which,
                                          XGL_SIZE alignment, XGL_SIZE size)
{
    struct intel_cmd_writer *writer = &cmd->writers[which];
    XGL_SIZE offset;

    assert(alignment && u_is_pow2(alignment));
    offset = u_align(writer->used, alignment);

    if (offset + size > writer->size) {
        cmd_writer_grow(cmd, which, offset + size);
        /* align again in case of errors */
        offset = u_align(writer->used, alignment);

        assert(offset + size <= writer->size);
    }

    return offset;
}

/**
 * Add a reloc at \p pos.  No error checking.
 */
static inline void cmd_writer_reloc(struct intel_cmd *cmd,
                                    enum intel_cmd_writer_type which,
                                    XGL_SIZE offset, struct intel_bo *bo,
                                    uint32_t bo_offset, uint32_t flags)
{
    struct intel_cmd_reloc *reloc = &cmd->relocs[cmd->reloc_used];

    assert(cmd->reloc_used < cmd->reloc_count);

    reloc->which = which;
    reloc->offset = offset;
    reloc->bo = bo;
    reloc->bo_offset = bo_offset;
    reloc->flags = flags;

    cmd->reloc_used++;
}

/**
 * Reserve a region from the state buffer.  Both the offset, in bytes, and the
 * pointer to the reserved region are returned.
 *
 * Note that \p alignment is in bytes and \p len is in DWords.
 */
static inline uint32_t cmd_state_pointer(struct intel_cmd *cmd,
                                         enum intel_cmd_item_type item,
                                         XGL_SIZE alignment, XGL_UINT len,
                                         uint32_t **dw)
{
    const enum intel_cmd_writer_type which = INTEL_CMD_WRITER_STATE;
    const XGL_SIZE size = len << 2;
    const XGL_SIZE offset = cmd_writer_reserve(cmd, which, alignment, size);
    struct intel_cmd_writer *writer = &cmd->writers[which];

    /* all states are at least aligned to 32-bytes */
    assert(alignment % 32 == 0);

    *dw = (uint32_t *) ((char *) writer->ptr + offset);

    writer->used = offset + size;

    if (intel_debug & INTEL_DEBUG_BATCH)
        cmd_writer_record(cmd, which, item, offset, size);

    return offset;
}

/**
 * Write a dynamic state to the state buffer.
 */
static inline uint32_t cmd_state_write(struct intel_cmd *cmd,
                                       enum intel_cmd_item_type item,
                                       XGL_SIZE alignment, XGL_UINT len,
                                       const uint32_t *dw)
{
    uint32_t offset, *dst;

    offset = cmd_state_pointer(cmd, item, alignment, len, &dst);
    memcpy(dst, dw, len << 2);

    return offset;
}

/**
 * Write a surface state to the surface buffer.  The offset, in bytes, of the
 * state is returned.
 *
 * Note that \p alignment is in bytes and \p len is in DWords.
 */
static inline uint32_t cmd_surface_write(struct intel_cmd *cmd,
                                         enum intel_cmd_item_type item,
                                         XGL_SIZE alignment, XGL_UINT len,
                                         const uint32_t *dw)
{
    assert(item == INTEL_CMD_ITEM_SURFACE ||
           item == INTEL_CMD_ITEM_BINDING_TABLE);

    return cmd_state_write(cmd, item, alignment, len, dw);
}

/**
 * Add a relocation entry for a DWord of a surface state.
 */
static inline void cmd_surface_reloc(struct intel_cmd *cmd,
                                     uint32_t offset, XGL_UINT dw_index,
                                     struct intel_bo *bo,
                                     uint32_t bo_offset, uint32_t reloc_flags)
{
    const enum intel_cmd_writer_type which = INTEL_CMD_WRITER_STATE;

    cmd_writer_reloc(cmd, which, offset + (dw_index << 2),
            bo, bo_offset, reloc_flags);
}

/**
 * Write a kernel to the instruction buffer.  The offset, in bytes, of the
 * kernel is returned.
 */
static inline uint32_t cmd_instruction_write(struct intel_cmd *cmd,
                                             XGL_SIZE size,
                                             const void *kernel)
{
    const enum intel_cmd_writer_type which = INTEL_CMD_WRITER_INSTRUCTION;
    /*
     * From the Sandy Bridge PRM, volume 4 part 2, page 112:
     *
     *     "Due to prefetch of the instruction stream, the EUs may attempt to
     *      access up to 8 instructions (128 bytes) beyond the end of the
     *      kernel program - possibly into the next memory page.  Although
     *      these instructions will not be executed, software must account for
     *      the prefetch in order to avoid invalid page access faults."
     */
    const XGL_SIZE reserved_size = size + 128;
    /* kernels are aligned to 64 bytes */
    const XGL_SIZE alignment = 64;
    const XGL_SIZE offset = cmd_writer_reserve(cmd,
            which, alignment, reserved_size);
    struct intel_cmd_writer *writer = &cmd->writers[which];

    memcpy((char *) writer->ptr + offset, kernel, size);

    writer->used = offset + size;

    if (intel_debug & INTEL_DEBUG_BATCH)
        cmd_writer_record(cmd, which, INTEL_CMD_ITEM_KERNEL, offset, size);

    return offset;
}

/**
 * Reserve a region from the batch buffer.  Both the offset, in DWords, and
 * the pointer to the reserved region are returned.
 *
 * Note that \p len is in DWords.
 */
static inline XGL_UINT cmd_batch_pointer(struct intel_cmd *cmd,
                                         XGL_UINT len, uint32_t **dw)
{
    const enum intel_cmd_writer_type which = INTEL_CMD_WRITER_BATCH;
    /*
     * We know the batch bo is always aligned.  Using 1 here should allow the
     * compiler to optimize away aligning.
     */
    const XGL_SIZE alignment = 1;
    const XGL_SIZE size = len << 2;
    const XGL_SIZE offset = cmd_writer_reserve(cmd, which, alignment, size);
    struct intel_cmd_writer *writer = &cmd->writers[which];

    assert(offset % 4 == 0);
    *dw = (uint32_t *) ((char *) writer->ptr + offset);

    writer->used = offset + size;

    return offset >> 2;
}

/**
 * Write a command to the batch buffer.
 */
static inline XGL_UINT cmd_batch_write(struct intel_cmd *cmd,
                                       XGL_UINT len, const uint32_t *dw)
{
    XGL_UINT pos;
    uint32_t *dst;

    pos = cmd_batch_pointer(cmd, len, &dst);
    memcpy(dst, dw, len << 2);

    return pos;
}

/**
 * Add a relocation entry for a DWord of a command.
 */
static inline void cmd_batch_reloc(struct intel_cmd *cmd, XGL_UINT pos,
                                   struct intel_bo *bo,
                                   uint32_t bo_offset, uint32_t reloc_flags)
{
    const enum intel_cmd_writer_type which = INTEL_CMD_WRITER_BATCH;

    cmd_writer_reloc(cmd, which, pos << 2, bo, bo_offset, reloc_flags);
}

/**
 * Begin the batch buffer.
 */
static inline void cmd_batch_begin(struct intel_cmd *cmd)
{
    /* STATE_BASE_ADDRESS */
    const uint8_t cmd_len = 10;
    const uint32_t dw0 = GEN6_RENDER_CMD(COMMON, STATE_BASE_ADDRESS) |
                         (cmd_len - 2);
    XGL_UINT pos;
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 7.5);

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);

    /* relocs are not added until cmd_batch_end() */
    assert(!pos);

    dw[0] = dw0;
    /* start offsets */
    dw[1] = 1;
    dw[2] = 1;
    dw[3] = 1;
    dw[4] = 1;
    dw[5] = 1;
    /* end offsets */
    dw[6] = 1;
    dw[7] = 1 + 0xfffff000;
    dw[8] = 1 + 0xfffff000;
    dw[9] = 1;
}

/**
 * End the batch buffer.
 */
static inline void cmd_batch_end(struct intel_cmd *cmd)
{
    struct intel_cmd_writer *writer = &cmd->writers[INTEL_CMD_WRITER_BATCH];
    const struct intel_cmd_writer *state =
        &cmd->writers[INTEL_CMD_WRITER_STATE];
    const struct intel_cmd_writer *inst =
        &cmd->writers[INTEL_CMD_WRITER_INSTRUCTION];
    uint32_t *dw;

    cmd_reserve_reloc(cmd, 5);
    cmd_batch_reloc(cmd, 2, state->bo, 1, 0);
    cmd_batch_reloc(cmd, 3, state->bo, 1, 0);
    cmd_batch_reloc(cmd, 5, inst->bo, 1, 0);
    cmd_batch_reloc(cmd, 7, state->bo, 1 + (state->size << 2), 0);
    cmd_batch_reloc(cmd, 9, inst->bo, 1 + (inst->size << 2), 0);

    if (writer->used & 0x7) {
        cmd_batch_pointer(cmd, 1, &dw);
        dw[0] = GEN6_MI_CMD(MI_BATCH_BUFFER_END);
    } else {
        cmd_batch_pointer(cmd, 2, &dw);
        dw[0] = GEN6_MI_CMD(MI_BATCH_BUFFER_END);
        dw[1] = GEN6_MI_CMD(MI_NOOP);
    }
}

void cmd_batch_flush(struct intel_cmd *cmd, uint32_t pipe_control_dw0);

void cmd_batch_depth_count(struct intel_cmd *cmd,
                           struct intel_bo *bo,
                           XGL_GPU_SIZE offset);

void cmd_batch_timestamp(struct intel_cmd *cmd,
                         struct intel_bo *bo,
                         XGL_GPU_SIZE offset);

void cmd_batch_immediate(struct intel_cmd *cmd,
                         struct intel_bo *bo,
                         XGL_GPU_SIZE offset,
                         uint64_t val);

#endif /* CMD_PRIV_H */
