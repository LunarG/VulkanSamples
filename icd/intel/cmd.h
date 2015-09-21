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

#ifndef CMD_H
#define CMD_H

#include "intel.h"
#include "obj.h"
#include "view.h"
#include "state.h"

struct intel_pipeline;
struct intel_pipeline_shader;
struct intel_viewport_state;
struct intel_raster_state;
struct intel_msaa_state;
struct intel_blend_state;
struct intel_ds_state;
struct intel_desc_set;
struct intel_render_pass;

struct intel_cmd_item;
struct intel_cmd_reloc;
struct intel_cmd_meta;

/*
 * We know what workarounds are needed for intel_pipeline.  These are mostly
 * for pipeline derivatives.
 */
enum intel_cmd_wa_flags {
    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 60:
     *
     *     "Before any depth stall flush (including those produced by
     *      non-pipelined state commands), software needs to first send a
     *      PIPE_CONTROL with no bits set except Post-Sync Operation != 0."
     */
    INTEL_CMD_WA_GEN6_PRE_DEPTH_STALL_WRITE = 1 << 0,

    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 274:
     *
     *     "A PIPE_CONTROL command, with only the Stall At Pixel Scoreboard
     *      field set (DW1 Bit 1), must be issued prior to any change to the
     *      value in this field (Maximum Number of Threads in 3DSTATE_WM)"
     *
     * From the Ivy Bridge PRM, volume 2 part 1, page 286:
     *
     *     "If this field (Maximum Number of Threads in 3DSTATE_PS) is changed
     *      between 3DPRIMITIVE commands, a PIPE_CONTROL command with Stall at
     *      Pixel Scoreboard set is required to be issued."
     */
    INTEL_CMD_WA_GEN6_PRE_COMMAND_SCOREBOARD_STALL = 1 << 1,

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 106:
     *
     *     "A PIPE_CONTROL with Post-Sync Operation set to 1h and a depth
     *      stall needs to be sent just prior to any 3DSTATE_VS,
     *      3DSTATE_URB_VS, 3DSTATE_CONSTANT_VS,
     *      3DSTATE_BINDING_TABLE_POINTER_VS, 3DSTATE_SAMPLER_STATE_POINTER_VS
     *      command.  Only one PIPE_CONTROL needs to be sent before any
     *      combination of VS associated 3DSTATE."
     */
    INTEL_CMD_WA_GEN7_PRE_VS_DEPTH_STALL_WRITE = 1 << 2,

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 258:
     *
     *     "Due to an HW issue driver needs to send a pipe control with stall
     *      when ever there is state change in depth bias related state"
     */
    INTEL_CMD_WA_GEN7_POST_COMMAND_CS_STALL = 1 << 3,

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 276:
     *
     *     "The driver must make sure a PIPE_CONTROL with the Depth Stall
     *      Enable bit set after all the following states are programmed:
     *
     *       - 3DSTATE_PS
     *       - 3DSTATE_VIEWPORT_STATE_POINTERS_CC
     *       - 3DSTATE_CONSTANT_PS
     *       - 3DSTATE_BINDING_TABLE_POINTERS_PS
     *       - 3DSTATE_SAMPLER_STATE_POINTERS_PS
     *       - 3DSTATE_CC_STATE_POINTERS
     *       - 3DSTATE_BLEND_STATE_POINTERS
     *       - 3DSTATE_DEPTH_STENCIL_STATE_POINTERS"
     */
    INTEL_CMD_WA_GEN7_POST_COMMAND_DEPTH_STALL = 1 << 4,
};

enum intel_cmd_writer_type {
    INTEL_CMD_WRITER_BATCH,
    INTEL_CMD_WRITER_SURFACE,
    INTEL_CMD_WRITER_STATE,
    INTEL_CMD_WRITER_INSTRUCTION,

    INTEL_CMD_WRITER_COUNT,
};

enum intel_use_pipeline_dynamic_state {
    INTEL_USE_PIPELINE_DYNAMIC_VIEWPORT                  = (1 << 0),
    INTEL_USE_PIPELINE_DYNAMIC_SCISSOR                   = (1 << 1),
    INTEL_USE_PIPELINE_DYNAMIC_LINE_WIDTH                = (1 << 2),
    INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BIAS                = (1 << 3),
    INTEL_USE_PIPELINE_DYNAMIC_BLEND_CONSTANTS           = (1 << 4),
    INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BOUNDS              = (1 << 5),
    INTEL_USE_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK      = (1 << 6),
    INTEL_USE_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK        = (1 << 7),
    INTEL_USE_PIPELINE_DYNAMIC_STENCIL_REFERENCE         = (1 << 8)
};

struct intel_cmd_shader_cache {
    struct {
        const void *shader;
        uint32_t kernel_offset;
    } *entries;

    uint32_t count;
    uint32_t used;
};

struct intel_cmd_dset_data {
    struct intel_desc_offset *set_offsets;
    uint32_t set_offset_count;

    uint32_t *dynamic_offsets;
    uint32_t dynamic_offset_count;
};

/*
 * States bounded to the command buffer.  We want to write states directly to
 * the command buffer when possible, and reduce this struct.
 */
struct intel_cmd_bind {
    const struct intel_cmd_meta *meta;

    struct intel_cmd_shader_cache shader_cache;

    struct {
        const struct intel_pipeline *graphics;
        const struct intel_pipeline *compute;

        uint32_t vs_offset;
        uint32_t tcs_offset;
        uint32_t tes_offset;
        uint32_t gs_offset;
        uint32_t fs_offset;
        uint32_t cs_offset;
    } pipeline;

    struct {
        VkFlags use_pipeline_dynamic_state;
        struct intel_dynamic_viewport viewport;
        struct intel_dynamic_line_width line_width;
        struct intel_dynamic_depth_bias depth_bias;
        struct intel_dynamic_blend blend;
        struct intel_dynamic_depth_bounds depth_bounds;
        struct intel_dynamic_stencil stencil;
    } state;

    struct {
        struct intel_cmd_dset_data graphics_data;
        struct intel_cmd_dset_data compute_data;
    } dset;

    struct {
        const struct intel_buf *buf[INTEL_MAX_VERTEX_BINDING_COUNT];
        VkDeviceSize offset[INTEL_MAX_VERTEX_BINDING_COUNT];
    } vertex;

    struct {
        const struct intel_buf *buf;
        VkDeviceSize offset;
        VkIndexType type;
    } index;


    bool render_pass_changed;
    const struct intel_render_pass *render_pass;
    const struct intel_render_pass_subpass *render_pass_subpass;
    const struct intel_fb *fb;
    VkRenderPassContents render_pass_contents;

    uint32_t draw_count;
    uint32_t wa_flags;
};

struct intel_cmd_writer {
    size_t size;
    struct intel_bo *bo;
    void *ptr;

    size_t used;

    uint32_t sba_offset;

    /* for decoding */
    struct intel_cmd_item *items;
    uint32_t item_alloc;
    uint32_t item_used;
};

struct intel_cmd_pool {
    struct intel_obj obj;
    struct intel_dev *dev;

    uint32_t queue_family_index;
    uint32_t create_flags;
};

static inline struct intel_cmd_pool *intel_cmd_pool(VkCmdPool pool)
{
    return *(struct intel_cmd_pool **) &pool;
}

static inline struct intel_cmd_pool *intel_cmd_pool_from_base(struct intel_base *base)
{
    return (struct intel_cmd_pool *) base;
}

static inline struct intel_cmd_pool *intel_cmd_pool_from_obj(struct intel_obj *obj)
{
    return (struct intel_cmd_pool *) &obj->base;
}

VkResult intel_cmd_pool_create(struct intel_dev *dev,
                            const VkCmdPoolCreateInfo *info,
                            struct intel_cmd_pool **cmd_pool_ret);
void intel_cmd_pool_destroy(struct intel_cmd_pool *pool);

struct intel_cmd {
    struct intel_obj obj;

    struct intel_dev *dev;
    struct intel_bo *scratch_bo;
    bool primary;
    int pipeline_select;

    struct intel_cmd_reloc *relocs;
    uint32_t reloc_count;

    VkFlags flags;

    struct intel_cmd_writer writers[INTEL_CMD_WRITER_COUNT];

    uint32_t reloc_used;
    VkResult result;

    struct intel_cmd_bind bind;
};

static inline struct intel_cmd *intel_cmd(VkCmdBuffer cmd)
{
    return (struct intel_cmd *) cmd;
}

static inline struct intel_cmd *intel_cmd_from_obj(struct intel_obj *obj)
{
    return (struct intel_cmd *) obj;
}

VkResult intel_cmd_create(struct intel_dev *dev,
                            const VkCmdBufferCreateInfo *info,
                            struct intel_cmd **cmd_ret);
void intel_cmd_destroy(struct intel_cmd *cmd);

VkResult intel_cmd_begin(struct intel_cmd *cmd, const VkCmdBufferBeginInfo* pBeginInfo);
VkResult intel_cmd_end(struct intel_cmd *cmd);

void intel_cmd_decode(struct intel_cmd *cmd, bool decode_inst_writer);

static inline struct intel_bo *intel_cmd_get_batch(const struct intel_cmd *cmd,
                                                   VkDeviceSize *used)
{
    const struct intel_cmd_writer *writer =
        &cmd->writers[INTEL_CMD_WRITER_BATCH];

    if (used)
        *used = writer->used;

    return writer->bo;
}

#endif /* CMD_H */
