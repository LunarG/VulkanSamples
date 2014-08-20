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

#include "dev.h"
#include "gpu.h"
#include "cmd.h"

#define CMD_ASSERT(cmd, min_gen, max_gen) \
    INTEL_GPU_ASSERT((cmd)->dev->gpu, (min_gen), (max_gen))

static inline int cmd_gen(const struct intel_cmd *cmd)
{
    return intel_gpu_gen(cmd->dev->gpu);
}

void cmd_grow(struct intel_cmd *cmd);

/**
 * Reserve \p len DWords for writing.
 */
static inline void cmd_reserve(struct intel_cmd *cmd, XGL_UINT len)
{
    if (cmd->used + len > cmd->size)
        cmd_grow(cmd);
    assert(cmd->used + len <= cmd->size);
}

/**
 * Return a pointer of \p len DWords for writing.
 */
static inline uint32_t *cmd_ptr(struct intel_cmd *cmd, XGL_UINT len)
{
    cmd_reserve(cmd, len);
    return &((uint32_t *) cmd->ptr_opaque)[cmd->used];
}

/**
 * Add a reloc for the value at \p offset.
 */
static inline void cmd_add_reloc(struct intel_cmd *cmd, XGL_INT offset,
                                 uint32_t val, struct intel_mem *mem,
                                 uint16_t read_domains, uint16_t write_domain)
{
    struct intel_cmd_reloc *reloc = &cmd->relocs[cmd->reloc_used];

    reloc->pos = cmd->used + offset;
    reloc->val = val;
    reloc->mem = mem;
    reloc->read_domains = read_domains;
    reloc->write_domain = write_domain;

    cmd->reloc_used++;
}

/**
 * Advance without writing.
 */
static inline void cmd_advance(struct intel_cmd *cmd, XGL_UINT len)
{
    assert(cmd->used + len <= cmd->size);
    cmd->used += len;
}

/**
 * Write a DWord and advance.
 */
static inline void cmd_write(struct intel_cmd *cmd, uint32_t val)
{
    assert(cmd->used < cmd->size);
    ((uint32_t *) cmd->ptr_opaque)[cmd->used++] = val;
}

/**
 * Write \p len DWords and advance.
 */
static inline void cmd_write_n(struct intel_cmd *cmd,
                               const uint32_t *vals, XGL_UINT len)
{
    assert(cmd->used + len <= cmd->size);
    memcpy((uint32_t *) cmd->ptr_opaque + cmd->used,
            vals, sizeof(uint32_t) * len);
    cmd->used += len;
}

/**
 * Write a reloc and advance.
 */
static inline void cmd_write_r(struct intel_cmd *cmd, uint32_t val,
                               struct intel_mem *mem,
                               uint16_t read_domains, uint16_t write_domain)
{
    cmd_add_reloc(cmd, 0, val, mem, read_domains, write_domain);
    cmd->used++;
}

/**
 * Patch the given \p pos.
 */
static inline void cmd_patch(struct intel_cmd *cmd,
                             XGL_UINT pos, uint32_t val)
{
    assert(pos < cmd->used);
    ((uint32_t *) cmd->ptr_opaque)[pos] = val;
}

#endif /* CMD_PRIV_H */
