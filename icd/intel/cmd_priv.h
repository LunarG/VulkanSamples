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

struct intel_cmd_reloc {
    enum intel_cmd_writer_type which;
    XGL_UINT pos;

    uint32_t val;
    struct intel_bo *bo;

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
                     enum intel_cmd_writer_type which);

/**
 * Add a reloc at \p pos.  No error checking.
 */
static inline void cmd_writer_add_reloc(struct intel_cmd *cmd,
                                        enum intel_cmd_writer_type which,
                                        XGL_UINT pos, uint32_t val,
                                        struct intel_bo *bo,
                                        uint32_t flags)
{
    struct intel_cmd_reloc *reloc = &cmd->relocs[cmd->reloc_used];

    assert(cmd->reloc_used < cmd->reloc_count);

    reloc->which = which;
    reloc->pos = pos;
    reloc->val = val;
    reloc->bo = bo;
    reloc->flags = flags;

    cmd->reloc_used++;
}

/**
 * Reserve \p len DWords in the batch buffer for building a hardware command.
 */
static inline void cmd_batch_reserve(struct intel_cmd *cmd, XGL_UINT len)
{
    struct intel_cmd_writer *writer = &cmd->writers[INTEL_CMD_WRITER_BATCH];

    if (writer->used + len > writer->size)
        cmd_writer_grow(cmd, INTEL_CMD_WRITER_BATCH);
    assert(writer->used + len <= writer->size);
}

/**
 * Reserve \p len DWords in the batch buffer and \p reloc_len relocs for
 * building a hardware command.
 */
static inline void cmd_batch_reserve_reloc(struct intel_cmd *cmd,
                                           XGL_UINT len, XGL_UINT reloc_len)
{
    cmd_reserve_reloc(cmd, reloc_len);
    cmd_batch_reserve(cmd, len);
}

/**
 * Add a DWord to the hardware command being built.  No error checking.
 */
static inline void cmd_batch_write(struct intel_cmd *cmd, uint32_t val)
{
    struct intel_cmd_writer *writer = &cmd->writers[INTEL_CMD_WRITER_BATCH];

    assert(writer->used < writer->size);
    ((uint32_t *) writer->ptr_opaque)[writer->used++] = val;
}

/**
 * Add \p len DWords to the hardware command being built.  No error checking.
 */
static inline void cmd_batch_write_n(struct intel_cmd *cmd,
                                     const uint32_t *vals, XGL_UINT len)
{
    struct intel_cmd_writer *writer = &cmd->writers[INTEL_CMD_WRITER_BATCH];

    assert(writer->used + len <= writer->size);

    memcpy((uint32_t *) writer->ptr_opaque + writer->used,
            vals, sizeof(uint32_t) * len);
    writer->used += len;
}

/**
 * Add a reloc to the hardware command being built.  No error checking.
 */
static inline void cmd_batch_reloc(struct intel_cmd *cmd,
                                   uint32_t val, struct intel_bo *bo,
                                   uint32_t flags)
{
    struct intel_cmd_writer *writer = &cmd->writers[INTEL_CMD_WRITER_BATCH];

    cmd_writer_add_reloc(cmd, INTEL_CMD_WRITER_BATCH,
            writer->used, val, bo, flags);

    writer->used++;
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

    CMD_ASSERT(cmd, 6, 7.5);

    cmd_batch_reserve(cmd, cmd_len);

    /* relocs are not added until cmd_batch_end() */
    assert(cmd->writers[INTEL_CMD_WRITER_BATCH].used == 0);

    cmd_batch_write(cmd, dw0);

    /* start offsets */
    cmd_batch_write(cmd, 1);
    cmd_batch_write(cmd, 1);
    cmd_batch_write(cmd, 1);
    cmd_batch_write(cmd, 1);
    cmd_batch_write(cmd, 1);
    /* end offsets */
    cmd_batch_write(cmd, 1);
    cmd_batch_write(cmd, 1 + 0xfffff000);
    cmd_batch_write(cmd, 1 + 0xfffff000);
    cmd_batch_write(cmd, 1);
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

    cmd_reserve_reloc(cmd, 5);
    cmd_writer_add_reloc(cmd, INTEL_CMD_WRITER_BATCH,
            2, 1, state->bo, 0);
    cmd_writer_add_reloc(cmd, INTEL_CMD_WRITER_BATCH,
            3, 1, state->bo, 0);
    cmd_writer_add_reloc(cmd, INTEL_CMD_WRITER_BATCH,
            5, 1, inst->bo, 0);
    cmd_writer_add_reloc(cmd, INTEL_CMD_WRITER_BATCH,
            7, 1 + (state->size << 2), state->bo, 0);
    cmd_writer_add_reloc(cmd, INTEL_CMD_WRITER_BATCH,
            9, 1 + (inst->size << 2), inst->bo, 0);

    if (writer->used & 1) {
        cmd_batch_reserve(cmd, 1);
        cmd_batch_write(cmd, GEN6_MI_CMD(MI_BATCH_BUFFER_END));
    } else {
        cmd_batch_reserve(cmd, 2);
        cmd_batch_write(cmd, GEN6_MI_CMD(MI_BATCH_BUFFER_END));
        cmd_batch_write(cmd, GEN6_MI_CMD(MI_NOOP));
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
/**
 * Reserve \p len DWords in the state buffer for building a hardware state.
 * The current writer position is aligned to \p alignment first.  Both the
 * pointer to the reserved region and the aligned position are returned.
 *
 * Note that the returned pointer is only valid until the next reserve call.
 */
static inline uint32_t *cmd_state_reserve(struct intel_cmd *cmd, XGL_UINT len,
                                          XGL_UINT alignment, XGL_UINT *pos)
{
    struct intel_cmd_writer *writer = &cmd->writers[INTEL_CMD_WRITER_STATE];
    XGL_UINT aligned;

    assert(alignment && u_is_pow2(alignment));
    aligned = u_align(writer->used, alignment);

    if (aligned + len > writer->size)
        cmd_writer_grow(cmd, INTEL_CMD_WRITER_STATE);
    assert(aligned + len <= writer->size);

    writer->used = aligned;
    *pos = aligned;

    return &((uint32_t *) writer->ptr_opaque)[writer->used];
}

/**
 * Similar to \p cmd_state_reserve, except that \p reloc_len relocs are also
 * reserved.
 */
static inline uint32_t *cmd_state_reserve_reloc(struct intel_cmd *cmd,
                                                XGL_UINT len,
                                                XGL_UINT reloc_len,
                                                XGL_UINT alignment,
                                                XGL_UINT *pos)
{
    cmd_reserve_reloc(cmd, reloc_len);
    return cmd_state_reserve(cmd, len, alignment, pos);
}

/**
 * Add a reloc at \p offset, relative to the current writer position.  No
 * error checking.
 */
static inline void cmd_state_reloc(struct intel_cmd *cmd,
                                   XGL_INT offset, uint32_t val,
                                   struct intel_bo *bo,
                                   uint32_t flags)
{
    struct intel_cmd_writer *writer = &cmd->writers[INTEL_CMD_WRITER_STATE];

    cmd_writer_add_reloc(cmd, INTEL_CMD_WRITER_STATE,
            writer->used + offset, val, bo, flags);
}

/**
 * Advance the writer position of the state buffer.  No error checking.
 */
static inline void cmd_state_advance(struct intel_cmd *cmd, XGL_UINT len)
{
    struct intel_cmd_writer *writer = &cmd->writers[INTEL_CMD_WRITER_STATE];

    assert(writer->used + len <= writer->size);
    writer->used += len;
}

/**
 * A convenient function to copy a hardware state of \p len DWords into the
 * state buffer.  The position of the state is returned.
 */
static inline XGL_UINT cmd_state_copy(struct intel_cmd *cmd,
                                      const uint32_t *vals, XGL_UINT len,
                                      XGL_UINT alignment)
{
    uint32_t *dst;
    XGL_UINT pos;

    dst = cmd_state_reserve(cmd, len, alignment, &pos);
    memcpy(dst, vals, sizeof(uint32_t) * len);
    cmd_state_advance(cmd, len);

    return pos;
}

static inline XGL_UINT cmd_kernel_copy(struct intel_cmd *cmd,
                                       const void *kernel, XGL_SIZE size)
{
    /*
     * From the Sandy Bridge PRM, volume 4 part 2, page 112:
     *
     *     "Due to prefetch of the instruction stream, the EUs may attempt to
     *      access up to 8 instructions (128 bytes) beyond the end of the
     *      kernel program - possibly into the next memory page.  Although
     *      these instructions will not be executed, software must account for
     *      the prefetch in order to avoid invalid page access faults."
     */
    const XGL_UINT prefetch_len = 128 / sizeof(uint32_t);
    /* kernels are aligned to 64-byte */
    const XGL_UINT kernel_align = 64 / sizeof(uint32_t);
    const XGL_UINT kernel_len = ((size + 3) & ~3) / sizeof(uint32_t);
    struct intel_cmd_writer *writer =
        &cmd->writers[INTEL_CMD_WRITER_INSTRUCTION];
    XGL_UINT kernel_pos;

    kernel_pos = u_align(writer->used, kernel_align);
    if (kernel_pos + kernel_len + prefetch_len > writer->size)
        cmd_writer_grow(cmd, INTEL_CMD_WRITER_INSTRUCTION);
    assert(kernel_pos + kernel_len + prefetch_len <= writer->size);

    memcpy(&((uint32_t *) writer->ptr_opaque)[kernel_pos], kernel, size);
    writer->used = kernel_pos + kernel_len;

    return kernel_pos;
}

#endif /* CMD_PRIV_H */
