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
#include "img.h"
#include "mem.h"
#include "cmd_priv.h"

enum {
    MEM_CACHE        = 1 << 0,
    DATA_READ_CACHE  = 1 << 1,
    DATA_WRITE_CACHE = 1 << 2,
    RENDER_CACHE     = 1 << 3,
    SAMPLER_CACHE    = 1 << 4,
};

static uint32_t mem_get_state_caches(const struct intel_mem *mem,
                                     XGL_MEMORY_STATE state)
{
    uint32_t caches;

    switch (state) {
    case XGL_MEMORY_STATE_DATA_TRANSFER:
    case XGL_MEMORY_STATE_INDEX_DATA:
    case XGL_MEMORY_STATE_INDIRECT_ARG:
    case XGL_MEMORY_STATE_WRITE_TIMESTAMP:
    case XGL_MEMORY_STATE_QUEUE_ATOMIC:
        caches = MEM_CACHE;
        break;
    case XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY:
    case XGL_MEMORY_STATE_COMPUTE_SHADER_READ_ONLY:
    case XGL_MEMORY_STATE_MULTI_SHADER_READ_ONLY:
        caches = DATA_READ_CACHE;
        break;
    case XGL_MEMORY_STATE_GRAPHICS_SHADER_WRITE_ONLY:
    case XGL_MEMORY_STATE_COMPUTE_SHADER_WRITE_ONLY:
        caches = DATA_WRITE_CACHE;
        break;
    case XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_WRITE:
    case XGL_MEMORY_STATE_COMPUTE_SHADER_READ_WRITE:
        caches = DATA_READ_CACHE | DATA_WRITE_CACHE;
        break;
    default:
        caches = 0;
        break;
    }

    return caches;
}

static uint32_t cmd_get_mem_flush_flags(const struct intel_cmd *cmd,
                                        const struct intel_mem *mem,
                                        uint32_t old_caches,
                                        uint32_t new_caches)
{
    uint32_t flags = 0;

    /* not dirty */
    if (!(old_caches & (MEM_CACHE | DATA_WRITE_CACHE)))
        return 0;

    if ((old_caches & DATA_WRITE_CACHE) &&
        (new_caches & ~(DATA_READ_CACHE | DATA_WRITE_CACHE))) {
        if (cmd_gen(cmd) >= INTEL_GEN(7))
            flags |= GEN7_PIPE_CONTROL_DC_FLUSH_ENABLE;
    }

    if ((new_caches & DATA_READ_CACHE) && old_caches != DATA_WRITE_CACHE)
        flags |= GEN6_PIPE_CONTROL_CONSTANT_CACHE_INVALIDATE;

    if (!flags)
        return 0;

    flags |= GEN6_PIPE_CONTROL_CS_STALL;

    return flags;
}

static uint32_t img_get_state_caches(const struct intel_img *img,
                                     XGL_IMAGE_STATE state)
{
    uint32_t caches;

    switch (state) {
    case XGL_IMAGE_STATE_DATA_TRANSFER:
        caches = MEM_CACHE;
        break;
    case XGL_IMAGE_STATE_GRAPHICS_SHADER_WRITE_ONLY:
    case XGL_IMAGE_STATE_COMPUTE_SHADER_WRITE_ONLY:
        caches = DATA_WRITE_CACHE;
        break;
    case XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY:
    case XGL_IMAGE_STATE_MULTI_SHADER_READ_ONLY:
    case XGL_IMAGE_STATE_TARGET_AND_SHADER_READ_ONLY:
        caches = DATA_READ_CACHE | SAMPLER_CACHE;
        break;
    case XGL_IMAGE_STATE_COMPUTE_SHADER_READ_ONLY:
        caches = DATA_READ_CACHE;
        break;
    case XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_WRITE:
        caches = DATA_READ_CACHE | DATA_WRITE_CACHE | SAMPLER_CACHE;
        break;
    case XGL_IMAGE_STATE_COMPUTE_SHADER_READ_WRITE:
        caches = DATA_READ_CACHE | DATA_WRITE_CACHE;
        break;
    case XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL:
    case XGL_IMAGE_STATE_TARGET_SHADER_ACCESS_OPTIMAL:
        caches = RENDER_CACHE | DATA_WRITE_CACHE;
        break;
    case XGL_IMAGE_STATE_CLEAR:
    case XGL_IMAGE_STATE_RESOLVE_DESTINATION:
        caches = RENDER_CACHE;
        break;
    case XGL_IMAGE_STATE_RESOLVE_SOURCE:
        caches = SAMPLER_CACHE;
        break;
    default:
        caches = 0;
        break;
    }

    return caches;
}

static uint32_t cmd_get_img_flush_flags(const struct intel_cmd *cmd,
                                        const struct intel_img *img,
                                        uint32_t old_caches,
                                        uint32_t new_caches)
{
    uint32_t flags = 0;

    /* not dirty */
    if (!(old_caches & (MEM_CACHE | RENDER_CACHE | DATA_WRITE_CACHE)))
        return 0;

    if ((old_caches & RENDER_CACHE) && (new_caches & ~RENDER_CACHE)) {
        if (img->layout.format.numericFormat == XGL_NUM_FMT_DS)
            flags |= GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH;
        else
            flags |= GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH;
    }

    if ((old_caches & DATA_WRITE_CACHE) &&
        (new_caches & ~(DATA_READ_CACHE | DATA_WRITE_CACHE))) {
        if (cmd_gen(cmd) >= INTEL_GEN(7))
            flags |= GEN7_PIPE_CONTROL_DC_FLUSH_ENABLE;
    }

    if (new_caches & SAMPLER_CACHE)
        flags |= GEN6_PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE;

    if ((new_caches & DATA_READ_CACHE) && old_caches != DATA_WRITE_CACHE)
        flags |= GEN6_PIPE_CONTROL_CONSTANT_CACHE_INVALIDATE;

    if (!flags)
        return 0;

    flags |= GEN6_PIPE_CONTROL_CS_STALL;

    return flags;
}

XGL_VOID XGLAPI intelCmdPrepareMemoryRegions(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_MEMORY_STATE_TRANSITION*          pStateTransitions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    uint32_t flush_flags = 0;
    XGL_UINT i;

    for (i = 0; i < transitionCount; i++) {
        const XGL_MEMORY_STATE_TRANSITION *trans = &pStateTransitions[i];
        struct intel_mem *mem = intel_mem(trans->mem);

        flush_flags |= cmd_get_mem_flush_flags(cmd, mem,
                mem_get_state_caches(mem, trans->oldState),
                mem_get_state_caches(mem, trans->newState));
    }

    cmd_batch_flush(cmd, flush_flags);
}

XGL_VOID XGLAPI intelCmdPrepareImages(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_IMAGE_STATE_TRANSITION*           pStateTransitions)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    uint32_t flush_flags = 0;
    XGL_UINT i;

    for (i = 0; i < transitionCount; i++) {
        const XGL_IMAGE_STATE_TRANSITION *trans = &pStateTransitions[i];
        struct intel_img *img = intel_img(trans->image);

        flush_flags |= cmd_get_img_flush_flags(cmd, img,
                img_get_state_caches(img, trans->oldState),
                img_get_state_caches(img, trans->newState));
    }

    cmd_batch_flush(cmd, flush_flags);
}
