/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Chia-I Wu <olv@lunarg.com>
 *
 */

#include "genhw/genhw.h"
#include "img.h"
#include "buf.h"
#include "cmd_priv.h"

enum {
    READ_OP          = 1 << 0,
    WRITE_OP         = 1 << 1,
    HIZ_OP           = 1 << 2,
};

enum {
    MEM_CACHE        = 1 << 0,
    DATA_READ_CACHE  = 1 << 1,
    DATA_WRITE_CACHE = 1 << 2,
    RENDER_CACHE     = 1 << 3,
    SAMPLER_CACHE    = 1 << 4,
};

static uint32_t img_get_layout_ops(const struct intel_img *img,
                                   VkImageLayout layout)
{
    uint32_t ops;

    switch ((int) layout) {
    case VK_IMAGE_LAYOUT_GENERAL:
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        ops = READ_OP | WRITE_OP;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        ops = READ_OP | WRITE_OP;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        ops = READ_OP | WRITE_OP | HIZ_OP;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        ops = READ_OP | HIZ_OP;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        ops = READ_OP;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        ops = READ_OP;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        ops = WRITE_OP;
        break;
    case VK_IMAGE_LAYOUT_UNDEFINED:
    default:
        ops = 0;
        break;
    }

    return ops;
}

static uint32_t img_get_layout_caches(const struct intel_img *img,
                                     VkImageLayout layout)
{
    uint32_t caches;

    switch ((int) layout) {
    case VK_IMAGE_LAYOUT_GENERAL:
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        // General layout when image can be used for any kind of access
        caches = MEM_CACHE | DATA_READ_CACHE | DATA_WRITE_CACHE | RENDER_CACHE | SAMPLER_CACHE;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        // Optimal layout when image is only used for color attachment read/write
        caches = DATA_WRITE_CACHE | RENDER_CACHE;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        // Optimal layout when image is only used for depth/stencil attachment read/write
        caches = DATA_WRITE_CACHE | RENDER_CACHE;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
        // Optimal layout when image is used for read only depth/stencil attachment and shader access
        caches = RENDER_CACHE;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        // Optimal layout when image is used for read only shader access
        caches = DATA_READ_CACHE | SAMPLER_CACHE;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        // Optimal layout when image is used only as source of transfer operations
        caches = MEM_CACHE | DATA_READ_CACHE | RENDER_CACHE | SAMPLER_CACHE;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        // Optimal layout when image is used only as destination of transfer operations
        caches = MEM_CACHE | DATA_WRITE_CACHE | RENDER_CACHE;
        break;
    default:
        caches = 0;
        break;
    }

    return caches;
}

static void cmd_resolve_depth(struct intel_cmd *cmd,
                              struct intel_img *img,
                              VkImageLayout old_layout,
                              VkImageLayout new_layout,
                              const VkImageSubresourceRange *range)
{
    const uint32_t old_ops = img_get_layout_ops(img, old_layout);
    const uint32_t new_ops = img_get_layout_ops(img, new_layout);

    if (old_ops & WRITE_OP) {
        if ((old_ops & HIZ_OP) && !(new_ops & HIZ_OP))
            cmd_meta_ds_op(cmd, INTEL_CMD_META_DS_RESOLVE, img, range);
        else if (!(old_ops & HIZ_OP) && (new_ops & HIZ_OP))
            cmd_meta_ds_op(cmd, INTEL_CMD_META_DS_HIZ_RESOLVE, img, range);
    }
}

static uint32_t cmd_get_flush_flags(const struct intel_cmd *cmd,
                                    uint32_t old_caches,
                                    uint32_t new_caches,
                                    bool is_ds)
{
    uint32_t flags = 0;

    /* not dirty */
    if (!(old_caches & (MEM_CACHE | RENDER_CACHE | DATA_WRITE_CACHE)))
        return 0;

    if ((old_caches & RENDER_CACHE) && (new_caches & ~RENDER_CACHE)) {
        if (is_ds)
            flags |= GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH;
        else
            flags |= GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH;
    }

    if ((old_caches & DATA_WRITE_CACHE) &&
        (new_caches & ~(DATA_READ_CACHE | DATA_WRITE_CACHE))) {
        if (cmd_gen(cmd) >= INTEL_GEN(7))
            flags |= GEN7_PIPE_CONTROL_DC_FLUSH;
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

static void cmd_memory_barriers(struct intel_cmd *cmd,
                                uint32_t flush_flags,
                                uint32_t memory_barrier_count,
                                const void* const* memory_barriers)
{
    uint32_t i;
    VkFlags input_mask = 0;
    VkFlags output_mask = 0;

    for (i = 0; i < memory_barrier_count; i++) {

        const union {
            VkStructureType type;

            VkMemoryBarrier mem;
            VkBufferMemoryBarrier buf;
            VkImageMemoryBarrier img;
        } *u = memory_barriers[i];

        switch(u->type)
        {
        case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
            output_mask |= u->mem.srcAccessMask;
            input_mask  |= u->mem.dstAccessMask;
            break;
        case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
            output_mask |= u->buf.srcAccessMask;
            input_mask  |= u->buf.dstAccessMask;
            break;
        case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
            output_mask |= u->img.srcAccessMask;
            input_mask  |= u->img.dstAccessMask;
            {
                struct intel_img *img = intel_img(u->img.image);

                cmd_resolve_depth(cmd, img, u->img.oldLayout,
                        u->img.newLayout, &u->img.subresourceRange);

                flush_flags |= cmd_get_flush_flags(cmd,
                        img_get_layout_caches(img, u->img.oldLayout),
                        img_get_layout_caches(img, u->img.newLayout),
                        icd_format_is_ds(img->layout.format));
            }
            break;
        default:
            break;
        }
    }

    if (output_mask & VK_ACCESS_SHADER_WRITE_BIT) {
        flush_flags |= GEN7_PIPE_CONTROL_DC_FLUSH;
    }
    if (output_mask & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) {
        flush_flags |= GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH;
    }
    if (output_mask & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT) {
        flush_flags |= GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH;
    }

    /* CPU write is cache coherent, so VK_ACCESS_HOST_WRITE_BIT needs no flush. */
    /* Meta handles flushes, so VK_ACCESS_TRANSFER_WRITE_BIT needs no flush. */

    if (input_mask & (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT)) {
        flush_flags |= GEN6_PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE;
    }

    if (input_mask & VK_ACCESS_UNIFORM_READ_BIT) {
        flush_flags |= GEN6_PIPE_CONTROL_CONSTANT_CACHE_INVALIDATE;
    }

    if (input_mask & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT) {
        flush_flags |= GEN6_PIPE_CONTROL_VF_CACHE_INVALIDATE;
    }

    /* These bits have no corresponding cache invalidate operation.
     * VK_ACCESS_HOST_READ_BIT
     * VK_ACCESS_INDIRECT_COMMAND_READ_BIT
     * VK_ACCESS_INDEX_READ_BIT
     * VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
     * VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
     * VK_ACCESS_TRANSFER_READ_BIT
     */

    cmd_batch_flush(cmd, flush_flags);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents(
    VkCommandBuffer                                 commandBuffer,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    VkPipelineStageFlags                        sourceStageMask,
    VkPipelineStageFlags                        dstStageMask,
    uint32_t                                    memoryBarrierCount,
    const void* const*                          ppMemoryBarriers)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    /* This hardware will always wait at VK_PIPELINE_STAGE_TOP_OF_PIPE.
     * Passing a stageMask specifying other stages
     * does not change that.
     */

    /* Because the command buffer is serialized, reaching
     * a pipelined wait is always after completion of prior events.
     * pWaitInfo->pEvents need not be examined.
     * vkCmdWaitEvents is equivalent to memory barrier part of vkCmdPipelineBarrier.
     * cmd_memory_barriers will wait for GEN6_PIPE_CONTROL_CS_STALL and perform
     * appropriate cache control.
     */
    cmd_memory_barriers(cmd,
            GEN6_PIPE_CONTROL_CS_STALL,
            memoryBarrierCount, ppMemoryBarriers);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier(
        VkCommandBuffer                                 commandBuffer,
        VkPipelineStageFlags                        srcStageMask,
        VkPipelineStageFlags                        dstStageMask,
    VkDependencyFlags                           dependencyFlags,
        uint32_t                                    memoryBarrierCount,
        const void* const*                          ppMemoryBarriers)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);
    uint32_t pipe_control_flags = 0;

    /* This hardware will always wait at VK_WAIT_EVENT_TOP_OF_PIPE.
     * Passing a stageMask specifying other stages
     * does not change that.
     */

    /* Cache control is done with PIPE_CONTROL flags.
     * With no GEN6_PIPE_CONTROL_CS_STALL flag set, it behaves as VK_PIPE_EVENT_TOP_OF_PIPE.
     * All other pEvents values will behave as VK_PIPE_EVENT_COMMANDS_COMPLETE.
     */

    if ((srcStageMask & ~VK_PIPELINE_STAGE_HOST_BIT) ||
        (dstStageMask & ~VK_PIPELINE_STAGE_HOST_BIT)){
        pipe_control_flags = GEN6_PIPE_CONTROL_CS_STALL;
    }

    /* cmd_memory_barriers can wait for GEN6_PIPE_CONTROL_CS_STALL and perform
     * appropriate cache control.
     */
    cmd_memory_barriers(cmd,
            pipe_control_flags,
            memoryBarrierCount, ppMemoryBarriers);
}
