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

#ifndef CMD_PRIV_H
#define CMD_PRIV_H

#include "genhw/genhw.h"
#include "dev.h"
#include "gpu.h"
#include "cmd.h"

#define CMD_ASSERT(cmd, min_gen, max_gen) \
    INTEL_GPU_ASSERT((cmd)->dev->gpu, (min_gen), (max_gen))

struct intel_cmd_reloc {
    struct intel_cmd_writer *writer;
    XGL_UINT pos;

    uint32_t val;
    struct intel_bo *bo;

    /*
     * With application state tracking promised by XGL, we should be able to
     * set
     *
     *   I915_EXEC_NO_RELOC
     *   I915_EXEC_HANDLE_LUT
     *   I915_EXEC_IS_PINNED
     *
     * once we figure them out.
     */
    uint16_t read_domains;
    uint16_t write_domain;
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
                     struct intel_cmd_writer *writer);

/**
 * Add a reloc at \p offset, relative to the current writer position.  No
 * error checking.
 */
static inline void cmd_writer_add_reloc(struct intel_cmd *cmd,
                                        struct intel_cmd_writer *writer,
                                        XGL_INT offset, uint32_t val,
                                        struct intel_bo *bo,
                                        uint16_t read_domains,
                                        uint16_t write_domain)
{
    struct intel_cmd_reloc *reloc = &cmd->relocs[cmd->reloc_used];

    assert(cmd->reloc_used < cmd->reloc_count);

    reloc->writer = writer;
    reloc->pos = writer->used + offset;
    reloc->val = val;
    reloc->bo = bo;
    reloc->read_domains = read_domains;
    reloc->write_domain = write_domain;

    cmd->reloc_used++;
}

/**
 * Reserve \p len DWords in the batch buffer for building a hardware command.
 */
static inline void cmd_batch_reserve(struct intel_cmd *cmd, XGL_UINT len)
{
    struct intel_cmd_writer *writer = &cmd->batch;

    if (writer->used + len > writer->size)
        cmd_writer_grow(cmd, writer);
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
    struct intel_cmd_writer *writer = &cmd->batch;

    assert(writer->used < writer->size);
    ((uint32_t *) writer->ptr_opaque)[writer->used++] = val;
}

/**
 * Add \p len DWords to the hardware command being built.  No error checking.
 */
static inline void cmd_batch_write_n(struct intel_cmd *cmd,
                                     const uint32_t *vals, XGL_UINT len)
{
    struct intel_cmd_writer *writer = &cmd->batch;

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
                                   uint16_t read_domains,
                                   uint16_t write_domain)
{
    struct intel_cmd_writer *writer = &cmd->batch;

    cmd_writer_add_reloc(cmd, writer, 0, val,
            bo, read_domains, write_domain);

    writer->used++;
}

/**
 * End the batch buffer.
 */
static inline void cmd_batch_end(struct intel_cmd *cmd)
{
    if (cmd->batch.used & 1) {
        cmd_batch_reserve(cmd, 1);
        cmd_batch_write(cmd, GEN_MI_CMD(MI_BATCH_BUFFER_END));
    } else {
        cmd_batch_reserve(cmd, 2);
        cmd_batch_write(cmd, GEN_MI_CMD(MI_BATCH_BUFFER_END));
        cmd_batch_write(cmd, GEN_MI_CMD(MI_NOOP));
    }
}

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
    struct intel_cmd_writer *writer = &cmd->state;
    XGL_UINT aligned;

    assert(alignment && u_is_pow2(alignment));
    aligned = u_align(writer->used, alignment);

    if (aligned + len > writer->size)
        cmd_writer_grow(cmd, writer);
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
 * Advance the writer position of the state buffer.  No error checking.
 */
static inline void cmd_state_advance(struct intel_cmd *cmd, XGL_UINT len)
{
    struct intel_cmd_writer *writer = &cmd->state;

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
    struct intel_cmd_writer *writer = &cmd->kernel;
    XGL_UINT kernel_pos;

    kernel_pos = u_align(writer->used, kernel_align);
    if (kernel_pos + kernel_len + prefetch_len > writer->size)
        cmd_writer_grow(cmd, writer);
    assert(kernel_pos + kernel_len + prefetch_len <= writer->size);

    memcpy(&((uint32_t *) writer->ptr_opaque)[kernel_pos], kernel, size);
    writer->used = kernel_pos + kernel_len;

    return kernel_pos;
}

#endif /* CMD_PRIV_H */
