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
 * Advance without writing.
 */
static inline void cmd_advance(struct intel_cmd *cmd, XGL_UINT len)
{
    assert(cmd->used + len <= cmd->size);
    cmd->used += len;
}

/**
 * Write \p len DWords and advance.
 */
static inline void cmd_writen(struct intel_cmd *cmd,
                              const uint32_t *vals, XGL_UINT len)
{
    assert(cmd->used + len <= cmd->size);
    memcpy((uint32_t *) cmd->ptr_opaque + cmd->used,
            vals, sizeof(uint32_t) * len);
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

#endif /* CMD_PRIV_H */
