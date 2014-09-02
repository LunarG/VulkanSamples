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
#include "dset.h"
#include "img.h"
#include "mem.h"
#include "pipeline.h"
#include "shader.h"
#include "state.h"
#include "view.h"
#include "cmd_priv.h"

static void gen6_3DPRIMITIVE(struct intel_cmd *cmd,
                             int prim_type, bool indexed,
                             uint32_t vertex_count,
                             uint32_t vertex_start,
                             uint32_t instance_count,
                             uint32_t instance_start,
                             uint32_t vertex_base)
{
    const uint8_t cmd_len = 6;
    uint32_t dw0;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DPRIMITIVE) |
          prim_type << GEN6_3DPRIM_DW0_TYPE__SHIFT |
          (cmd_len - 2);

    if (indexed)
        dw0 |= GEN6_3DPRIM_DW0_ACCESS_RANDOM;

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, vertex_count);
    cmd_batch_write(cmd, vertex_start);
    cmd_batch_write(cmd, instance_count);
    cmd_batch_write(cmd, instance_start);
    cmd_batch_write(cmd, vertex_base);
}

static void gen7_3DPRIMITIVE(struct intel_cmd *cmd,
                             int prim_type, bool indexed,
                             uint32_t vertex_count,
                             uint32_t vertex_start,
                             uint32_t instance_count,
                             uint32_t instance_start,
                             uint32_t vertex_base)
{
    const uint8_t cmd_len = 7;
    uint32_t dw0, dw1;

    CMD_ASSERT(cmd, 7, 7.5);

    dw0 = GEN6_RENDER_CMD(3D, 3DPRIMITIVE) | (cmd_len - 2);
    dw1 = prim_type << GEN7_3DPRIM_DW1_TYPE__SHIFT;

    if (indexed)
        dw1 |= GEN7_3DPRIM_DW1_ACCESS_RANDOM;

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, dw1);
    cmd_batch_write(cmd, vertex_count);
    cmd_batch_write(cmd, vertex_start);
    cmd_batch_write(cmd, instance_count);
    cmd_batch_write(cmd, instance_start);
    cmd_batch_write(cmd, vertex_base);
}

static void gen6_PIPE_CONTROL(struct intel_cmd *cmd, uint32_t dw1,
                              struct intel_bo *bo, uint32_t bo_offset,
                              uint64_t imm)
{
   const uint8_t cmd_len = 5;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, PIPE_CONTROL) |
                        (cmd_len - 2);
   uint32_t reloc_flags = INTEL_RELOC_WRITE;

   CMD_ASSERT(cmd, 6, 7.5);

   assert(bo_offset % 8 == 0);

   if (dw1 & GEN6_PIPE_CONTROL_CS_STALL) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 73:
       *
       *     "1 of the following must also be set (when CS stall is set):
       *
       *       * Depth Cache Flush Enable ([0] of DW1)
       *       * Stall at Pixel Scoreboard ([1] of DW1)
       *       * Depth Stall ([13] of DW1)
       *       * Post-Sync Operation ([13] of DW1)
       *       * Render Target Cache Flush Enable ([12] of DW1)
       *       * Notify Enable ([8] of DW1)"
       *
       * From the Ivy Bridge PRM, volume 2 part 1, page 61:
       *
       *     "One of the following must also be set (when CS stall is set):
       *
       *       * Render Target Cache Flush Enable ([12] of DW1)
       *       * Depth Cache Flush Enable ([0] of DW1)
       *       * Stall at Pixel Scoreboard ([1] of DW1)
       *       * Depth Stall ([13] of DW1)
       *       * Post-Sync Operation ([13] of DW1)"
       */
      uint32_t bit_test = GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH |
                          GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                          GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL |
                          GEN6_PIPE_CONTROL_DEPTH_STALL;

      /* post-sync op */
      bit_test |= GEN6_PIPE_CONTROL_WRITE_IMM |
                  GEN6_PIPE_CONTROL_WRITE_PS_DEPTH_COUNT |
                  GEN6_PIPE_CONTROL_WRITE_TIMESTAMP;

      if (cmd_gen(cmd) == INTEL_GEN(6))
         bit_test |= GEN6_PIPE_CONTROL_NOTIFY_ENABLE;

      assert(dw1 & bit_test);
   }

   if (dw1 & GEN6_PIPE_CONTROL_DEPTH_STALL) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 73:
       *
       *     "Following bits must be clear (when Depth Stall is set):
       *
       *       * Render Target Cache Flush Enable ([12] of DW1)
       *       * Depth Cache Flush Enable ([0] of DW1)"
       */
      assert(!(dw1 & (GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH |
                      GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH)));
   }

   /*
    * From the Sandy Bridge PRM, volume 1 part 3, page 19:
    *
    *     "[DevSNB] PPGTT memory writes by MI_* (such as MI_STORE_DATA_IMM)
    *      and PIPE_CONTROL are not supported."
    *
    * The kernel will add the mapping automatically (when write domain is
    * INTEL_DOMAIN_INSTRUCTION).
    */
   if (cmd_gen(cmd) == INTEL_GEN(6) && bo) {
      bo_offset |= GEN6_PIPE_CONTROL_DW2_USE_GGTT;
      reloc_flags |= INTEL_RELOC_GGTT;
   }

   cmd_batch_reserve_reloc(cmd, cmd_len, (bool) bo);
   cmd_batch_write(cmd, dw0);
   cmd_batch_write(cmd, dw1);
   if (bo)
       cmd_batch_reloc(cmd, bo_offset, bo, reloc_flags);
   else
       cmd_batch_write(cmd, 0);
   cmd_batch_write(cmd, (uint32_t) imm);
   cmd_batch_write(cmd, (uint32_t) (imm >> 32));
}

static bool gen6_can_primitive_restart(const struct intel_cmd *cmd)
{
    const struct intel_pipeline *p = cmd->bind.pipeline.graphics;
    bool supported;

    CMD_ASSERT(cmd, 6, 7.5);

    if (cmd_gen(cmd) >= INTEL_GEN(7.5))
        return (p->prim_type != GEN6_3DPRIM_RECTLIST);

    switch (p->prim_type) {
    case GEN6_3DPRIM_POINTLIST:
    case GEN6_3DPRIM_LINELIST:
    case GEN6_3DPRIM_LINESTRIP:
    case GEN6_3DPRIM_TRILIST:
    case GEN6_3DPRIM_TRISTRIP:
        supported = true;
        break;
    default:
        supported = false;
        break;
    }

    if (!supported)
        return false;

    switch (cmd->bind.index.type) {
    case XGL_INDEX_8:
        supported = (p->primitive_restart_index != 0xffu);
        break;
    case XGL_INDEX_16:
        supported = (p->primitive_restart_index != 0xffffu);
        break;
    case XGL_INDEX_32:
        supported = (p->primitive_restart_index != 0xffffffffu);
        break;
    default:
        supported = false;
        break;
    }

    return supported;
}

static void gen6_3DSTATE_INDEX_BUFFER(struct intel_cmd *cmd,
                                      const struct intel_mem *mem,
                                      XGL_GPU_SIZE offset,
                                      XGL_INDEX_TYPE type,
                                      bool enable_cut_index)
{
    const uint8_t cmd_len = 3;
    uint32_t dw0, end_offset;
    unsigned offset_align;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_INDEX_BUFFER) | (cmd_len - 2);

    /* the bit is moved to 3DSTATE_VF */
    if (cmd_gen(cmd) >= INTEL_GEN(7.5))
        assert(!enable_cut_index);
    if (enable_cut_index)
        dw0 |= GEN6_IB_DW0_CUT_INDEX_ENABLE;

    switch (type) {
    case XGL_INDEX_8:
        dw0 |= GEN6_IB_DW0_FORMAT_BYTE;
        offset_align = 1;
        break;
    case XGL_INDEX_16:
        dw0 |= GEN6_IB_DW0_FORMAT_WORD;
        offset_align = 2;
        break;
    case XGL_INDEX_32:
        dw0 |= GEN6_IB_DW0_FORMAT_DWORD;
        offset_align = 4;
        break;
    default:
        cmd->result = XGL_ERROR_INVALID_VALUE;
        return;
        break;
    }

    if (offset % offset_align) {
        cmd->result = XGL_ERROR_INVALID_VALUE;
        return;
    }

    /* aligned and inclusive */
    end_offset = mem->size - (mem->size % offset_align) - 1;

    cmd_batch_reserve_reloc(cmd, cmd_len, 2);
    cmd_batch_write(cmd, dw0);
    cmd_batch_reloc(cmd, offset, mem->bo, 0);
    cmd_batch_reloc(cmd, end_offset, mem->bo, 0);
}

static void gen75_3DSTATE_VF(struct intel_cmd *cmd,
                             bool enable_cut_index,
                             uint32_t cut_index)
{
    const uint8_t cmd_len = 2;
    uint32_t dw0;

    CMD_ASSERT(cmd, 7.5, 7.5);

    dw0 = GEN75_RENDER_CMD(3D, 3DSTATE_VF) | (cmd_len - 2);
    if (enable_cut_index)
        dw0 |=  GEN75_VF_DW0_CUT_INDEX_ENABLE;

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, cut_index);
}


static void gen6_3DSTATE_GS(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 7;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_GS) | (cmd_len - 2);

    CMD_ASSERT(cmd, 6, 6);

    assert(cmd->bind.gs.shader == NULL);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, 0);
    cmd_batch_write(cmd, 0);
    cmd_batch_write(cmd, 0);
    cmd_batch_write(cmd, 1 << GEN6_GS_DW4_URB_READ_LEN__SHIFT);
    cmd_batch_write(cmd, GEN6_GS_DW5_STATISTICS);
    cmd_batch_write(cmd, 0);
}

static void gen7_3DSTATE_GS(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 7;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_GS) | (cmd_len - 2);

    CMD_ASSERT(cmd, 7, 7.5);

    assert(cmd->bind.gs.shader == NULL);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, 0);
    cmd_batch_write(cmd, 0);
    cmd_batch_write(cmd, 0);
    cmd_batch_write(cmd, 0);
    cmd_batch_write(cmd, GEN6_GS_DW5_STATISTICS);
    cmd_batch_write(cmd, 0);
}

static void gen6_3DSTATE_DRAWING_RECTANGLE(struct intel_cmd *cmd,
                                           XGL_UINT width, XGL_UINT height)
{
    const uint8_t cmd_len = 4;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_DRAWING_RECTANGLE) |
                         (cmd_len - 2);

    CMD_ASSERT(cmd, 6, 7.5);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    if (width && height) {
        cmd_batch_write(cmd, 0);
        cmd_batch_write(cmd, (height - 1) << 16 |
                             (width - 1));
    } else {
        cmd_batch_write(cmd, 1);
        cmd_batch_write(cmd, 0);
    }
    cmd_batch_write(cmd, 0);
}

static void gen7_fill_3DSTATE_SF_body(const struct intel_cmd *cmd,
                                      uint32_t body[6])
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_viewport_state *viewport = cmd->bind.state.viewport;
    const struct intel_raster_state *raster = cmd->bind.state.raster;
    const struct intel_msaa_state *msaa = cmd->bind.state.msaa;
    uint32_t dw1, dw2, dw3;
    int point_width;

    CMD_ASSERT(cmd, 6, 7.5);

    dw1 = GEN7_SF_DW1_STATISTICS |
          GEN7_SF_DW1_DEPTH_OFFSET_SOLID |
          GEN7_SF_DW1_DEPTH_OFFSET_WIREFRAME |
          GEN7_SF_DW1_DEPTH_OFFSET_POINT |
          GEN7_SF_DW1_VIEWPORT_ENABLE |
          raster->cmd_sf_fill;

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        int format;

        switch (pipeline->db_format.channelFormat) {
        case XGL_CH_FMT_R16:
            format = GEN6_ZFORMAT_D16_UNORM;
            break;
        case XGL_CH_FMT_R32:
        case XGL_CH_FMT_R32G8:
            format = GEN6_ZFORMAT_D32_FLOAT;
            break;
        default:
            assert(!"unknown depth format");
            format = 0;
            break;
        }

        dw1 |= format << GEN7_SF_DW1_DEPTH_FORMAT__SHIFT;
    }

    dw2 = raster->cmd_sf_cull;

    if (msaa->sample_count > 1) {
          dw2 |= 128 << GEN7_SF_DW2_LINE_WIDTH__SHIFT |
                 GEN7_SF_DW2_MSRASTMODE_ON_PATTERN;
    } else {
          dw2 |= 0 << GEN7_SF_DW2_LINE_WIDTH__SHIFT |
                 GEN7_SF_DW2_MSRASTMODE_OFF_PIXEL;
    }

    if (viewport->scissor_enable)
        dw2 |= GEN7_SF_DW2_SCISSOR_ENABLE;

    /* in U8.3 */
    point_width = (int) (pipeline->pointSize * 8.0f + 0.5f);
    point_width = U_CLAMP(point_width, 1, 2047);

    dw3 = pipeline->provoking_vertex_tri << GEN7_SF_DW3_TRI_PROVOKE__SHIFT |
          pipeline->provoking_vertex_line << GEN7_SF_DW3_LINE_PROVOKE__SHIFT |
          pipeline->provoking_vertex_trifan << GEN7_SF_DW3_TRIFAN_PROVOKE__SHIFT |
          GEN7_SF_DW3_SUBPIXEL_8BITS |
          GEN7_SF_DW3_USE_POINT_WIDTH |
          point_width;

    body[0] = dw1;
    body[1] = dw2;
    body[2] = dw3;
    body[3] = raster->cmd_depth_offset_const;
    body[4] = raster->cmd_depth_offset_scale;
    body[5] = raster->cmd_depth_offset_clamp;
}

static void gen7_fill_3DSTATE_SBE_body(const struct intel_cmd *cmd,
                                       uint32_t body[13])
{
    const struct intel_shader *vs =
        intel_shader(cmd->bind.pipeline.graphics->vs.shader);
    const struct intel_shader *fs =
        intel_shader(cmd->bind.pipeline.graphics->fs.shader);
    XGL_UINT attr_skip, attr_count;
    XGL_UINT vue_offset, vue_len;
    XGL_UINT i;
    uint32_t dw1;

    CMD_ASSERT(cmd, 6, 7.5);

    /* VS outputs VUE header and position additionally */
    assert(vs->out_count >= 2);
    attr_skip = 2;
    attr_count = vs->out_count - attr_skip;
    assert(fs->in_count == attr_count);
    assert(fs->in_count <= 32);

    vue_offset = attr_skip / 2;
    vue_len = (attr_count + 1) / 2;
    if (!vue_len)
        vue_len = 1;

    dw1 = fs->in_count << GEN7_SBE_DW1_ATTR_COUNT__SHIFT |
          vue_len << GEN7_SBE_DW1_URB_READ_LEN__SHIFT |
          vue_offset << GEN7_SBE_DW1_URB_READ_OFFSET__SHIFT;

    body[0] = dw1;

    for (i = 0; i < 8; i++) {
        uint16_t hi, lo;

        /* no attr swizzles */
        if (i * 2 + 1 < fs->in_count) {
            hi = i * 2 + 1;
            lo = i * 2;
        } else if (i * 2 < fs->in_count) {
            hi = 0;
            lo = i * 2;
        } else {
            hi = 0;
            lo = 0;
        }

        body[1 + i] = hi << GEN7_SBE_ATTR_HIGH__SHIFT | lo;
    }

    body[9] = 0; /* point sprite enables */
    body[10] = 0; /* constant interpolation enables */
    body[11] = 0; /* WrapShortest enables */
    body[12] = 0;
}

static void gen6_3DSTATE_SF(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 20;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SF) |
                         (cmd_len - 2);
    uint32_t sf[6];
    uint32_t sbe[13];

    CMD_ASSERT(cmd, 6, 6);

    gen7_fill_3DSTATE_SF_body(cmd, sf);
    gen7_fill_3DSTATE_SBE_body(cmd, sbe);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, sbe[0]);
    cmd_batch_write_n(cmd, sf, 6);
    cmd_batch_write_n(cmd, &sbe[1], 12);
}

static void gen7_3DSTATE_SF(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 7;
    uint32_t dw[7];

    CMD_ASSERT(cmd, 7, 7.5);

    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SF) |
            (cmd_len - 2);
    gen7_fill_3DSTATE_SF_body(cmd, &dw[1]);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write_n(cmd, dw, cmd_len);
}

static void gen7_3DSTATE_SBE(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 14;
    uint32_t dw[14];

    CMD_ASSERT(cmd, 7, 7.5);

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_SBE) |
            (cmd_len - 2);
    gen7_fill_3DSTATE_SBE_body(cmd, &dw[1]);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write_n(cmd, dw, cmd_len);
}

static void gen6_3DSTATE_CLIP(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 4;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CLIP) |
                         (cmd_len - 2);
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_shader *fs = intel_shader(pipeline->fs.shader);
    const struct intel_viewport_state *viewport = cmd->bind.state.viewport;
    const struct intel_raster_state *raster = cmd->bind.state.raster;
    uint32_t dw1, dw2, dw3;

    CMD_ASSERT(cmd, 6, 7.5);

    dw1 = GEN6_CLIP_DW1_STATISTICS;
    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        dw1 |= GEN7_CLIP_DW1_SUBPIXEL_8BITS |
               GEN7_CLIP_DW1_EARLY_CULL_ENABLE |
               raster->cmd_clip_cull;
    }

    dw2 = GEN6_CLIP_DW2_CLIP_ENABLE |
          GEN6_CLIP_DW2_XY_TEST_ENABLE |
          GEN6_CLIP_DW2_APIMODE_OGL |
          pipeline->provoking_vertex_tri << GEN6_CLIP_DW2_TRI_PROVOKE__SHIFT |
          pipeline->provoking_vertex_line << GEN6_CLIP_DW2_LINE_PROVOKE__SHIFT |
          pipeline->provoking_vertex_trifan << GEN6_CLIP_DW2_TRIFAN_PROVOKE__SHIFT;

    if (pipeline->rasterizerDiscardEnable)
        dw2 |= GEN6_CLIP_DW2_CLIPMODE_REJECT_ALL;
    else
        dw2 |= GEN6_CLIP_DW2_CLIPMODE_NORMAL;

    if (pipeline->depthClipEnable)
        dw2 |= GEN6_CLIP_DW2_Z_TEST_ENABLE;

    if (fs->barycentric_interps & (GEN6_INTERP_NONPERSPECTIVE_PIXEL |
                                   GEN6_INTERP_NONPERSPECTIVE_CENTROID |
                                   GEN6_INTERP_NONPERSPECTIVE_SAMPLE))
        dw2 |= GEN6_CLIP_DW2_NONPERSPECTIVE_BARYCENTRIC_ENABLE;

    dw3 = 0x1 << GEN6_CLIP_DW3_MIN_POINT_WIDTH__SHIFT |
          0x7ff << GEN6_CLIP_DW3_MAX_POINT_WIDTH__SHIFT |
          (viewport->viewport_count - 1);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, dw1);
    cmd_batch_write(cmd, dw2);
    cmd_batch_write(cmd, dw3);
}

static void gen6_3DSTATE_WM(struct intel_cmd *cmd)
{
    const int max_threads = (cmd->dev->gpu->gt == 2) ? 80 : 40;
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_shader *fs = intel_shader(pipeline->fs.shader);
    const struct intel_msaa_state *msaa = cmd->bind.state.msaa;
    const uint8_t cmd_len = 9;
    uint32_t dw0, dw2, dw4, dw5, dw6;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_WM) | (cmd_len - 2);

    dw2 = (fs->sampler_count + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT |
          fs->surface_count << GEN6_THREADDISP_BINDING_TABLE_SIZE__SHIFT;

    dw4 = GEN6_WM_DW4_STATISTICS |
          fs->urb_grf_start << GEN6_WM_DW4_URB_GRF_START0__SHIFT |
          0 << GEN6_WM_DW4_URB_GRF_START1__SHIFT |
          0 << GEN6_WM_DW4_URB_GRF_START2__SHIFT;

    dw5 = (max_threads - 1) << GEN6_WM_DW5_MAX_THREADS__SHIFT |
          GEN6_WM_DW5_PS_ENABLE |
          GEN6_WM_DW5_8_PIXEL_DISPATCH;

    if (fs->uses & INTEL_SHADER_USE_KILL ||
        pipeline->cb_state.alphaToCoverageEnable)
        dw5 |= GEN6_WM_DW5_PS_KILL;

    if (fs->uses & INTEL_SHADER_USE_COMPUTED_DEPTH)
        dw5 |= GEN6_WM_DW5_PS_COMPUTE_DEPTH;
    if (fs->uses & INTEL_SHADER_USE_DEPTH)
        dw5 |= GEN6_WM_DW5_PS_USE_DEPTH;
    if (fs->uses & INTEL_SHADER_USE_W)
        dw5 |= GEN6_WM_DW5_PS_USE_W;

    if (pipeline->cb_state.dualSourceBlendEnable)
        dw5 |= GEN6_WM_DW5_DUAL_SOURCE_BLEND;

    dw6 = fs->in_count << GEN6_WM_DW6_SF_ATTR_COUNT__SHIFT |
          GEN6_WM_DW6_POSOFFSET_NONE |
          GEN6_WM_DW6_ZW_INTERP_PIXEL |
          fs->barycentric_interps << GEN6_WM_DW6_BARYCENTRIC_INTERP__SHIFT |
          GEN6_WM_DW6_POINT_RASTRULE_UPPER_RIGHT;

    if (msaa->sample_count > 1) {
        dw6 |= GEN6_WM_DW6_MSRASTMODE_ON_PATTERN |
               GEN6_WM_DW6_MSDISPMODE_PERPIXEL;
    } else {
        dw6 |= GEN6_WM_DW6_MSRASTMODE_OFF_PIXEL |
               GEN6_WM_DW6_MSDISPMODE_PERSAMPLE;
    }

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, cmd->bind.fs.kernel_pos << 2);
    cmd_batch_write(cmd, dw2);
    cmd_batch_write(cmd, 0); /* scratch */
    cmd_batch_write(cmd, dw4);
    cmd_batch_write(cmd, dw5);
    cmd_batch_write(cmd, dw6);
    cmd_batch_write(cmd, 0); /* kernel 1 */
    cmd_batch_write(cmd, 0); /* kernel 2 */
}

static void gen7_3DSTATE_WM(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_shader *fs = intel_shader(pipeline->fs.shader);
    const struct intel_msaa_state *msaa = cmd->bind.state.msaa;
    const uint8_t cmd_len = 3;
    uint32_t dw0, dw1, dw2;

    CMD_ASSERT(cmd, 7, 7.5);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_WM) | (cmd_len - 2);

    dw1 = GEN7_WM_DW1_STATISTICS |
          GEN7_WM_DW1_PS_ENABLE |
          GEN7_WM_DW1_ZW_INTERP_PIXEL |
          fs->barycentric_interps << GEN7_WM_DW1_BARYCENTRIC_INTERP__SHIFT |
          GEN7_WM_DW1_POINT_RASTRULE_UPPER_RIGHT;

    if (fs->uses & INTEL_SHADER_USE_KILL ||
        pipeline->cb_state.alphaToCoverageEnable)
        dw1 |= GEN7_WM_DW1_PS_KILL;

    if (fs->uses & INTEL_SHADER_USE_COMPUTED_DEPTH)
        dw1 |= GEN7_WM_DW1_PSCDEPTH_ON;
    if (fs->uses & INTEL_SHADER_USE_DEPTH)
        dw1 |= GEN7_WM_DW1_PS_USE_DEPTH;
    if (fs->uses & INTEL_SHADER_USE_W)
        dw1 |= GEN7_WM_DW1_PS_USE_W;

    dw2 = 0;

    if (msaa->sample_count > 1) {
        dw1 |= GEN7_WM_DW1_MSRASTMODE_ON_PATTERN;
        dw2 |= GEN7_WM_DW2_MSDISPMODE_PERPIXEL;
    } else {
        dw1 |= GEN7_WM_DW1_MSRASTMODE_OFF_PIXEL;
        dw2 |= GEN7_WM_DW2_MSDISPMODE_PERSAMPLE;
    }

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, dw1);
    cmd_batch_write(cmd, dw2);
}

static void gen7_3DSTATE_PS(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_shader *fs = intel_shader(pipeline->fs.shader);
    const struct intel_msaa_state *msaa = cmd->bind.state.msaa;
    const uint8_t cmd_len = 8;
    uint32_t dw0, dw2, dw4, dw5;

    CMD_ASSERT(cmd, 7, 7.5);

    dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_PS) | (cmd_len - 2);

    dw2 = (fs->sampler_count + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT |
          fs->surface_count << GEN6_THREADDISP_BINDING_TABLE_SIZE__SHIFT;

    dw4 = GEN7_PS_DW4_POSOFFSET_NONE |
          GEN7_PS_DW4_8_PIXEL_DISPATCH;

    if (cmd_gen(cmd) >= INTEL_GEN(7.5)) {
        const int max_threads =
            (cmd->dev->gpu->gt == 3) ? 408 :
            (cmd->dev->gpu->gt == 2) ? 204 : 102;
        dw4 |= (max_threads - 1) << GEN75_PS_DW4_MAX_THREADS__SHIFT;
        dw4 |= msaa->cmd[msaa->cmd_len - 1] << GEN75_PS_DW4_SAMPLE_MASK__SHIFT;
    } else {
        const int max_threads = (cmd->dev->gpu->gt == 2) ? 172 : 48;
        dw4 |= (max_threads - 1) << GEN7_PS_DW4_MAX_THREADS__SHIFT;
    }

    if (pipeline->fs.linkConstBufferCount)
        dw4 |= GEN7_PS_DW4_PUSH_CONSTANT_ENABLE;

    if (fs->in_count)
        dw4 |= GEN7_PS_DW4_ATTR_ENABLE;

    if (pipeline->cb_state.dualSourceBlendEnable)
        dw4 |= GEN7_PS_DW4_DUAL_SOURCE_BLEND;

    dw5 = fs->urb_grf_start << GEN7_PS_DW5_URB_GRF_START0__SHIFT |
          0 << GEN7_PS_DW5_URB_GRF_START1__SHIFT |
          0 << GEN7_PS_DW5_URB_GRF_START2__SHIFT;

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, cmd->bind.fs.kernel_pos << 2);
    cmd_batch_write(cmd, dw2);
    cmd_batch_write(cmd, 0); /* scratch */
    cmd_batch_write(cmd, dw4);
    cmd_batch_write(cmd, dw5);
    cmd_batch_write(cmd, 0); /* kernel 1 */
    cmd_batch_write(cmd, 0); /* kernel 2 */
}

static void gen6_3DSTATE_DEPTH_BUFFER(struct intel_cmd *cmd,
                                      const struct intel_ds_view *view)
{
    const uint8_t cmd_len = 7;
    uint32_t dw0;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = (cmd_gen(cmd) >= INTEL_GEN(7)) ?
        GEN7_RENDER_CMD(3D, 3DSTATE_DEPTH_BUFFER) :
        GEN6_RENDER_CMD(3D, 3DSTATE_DEPTH_BUFFER);
    dw0 |= (cmd_len - 2);

    cmd_batch_reserve_reloc(cmd, cmd_len, (bool) view->img);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, view->cmd[0]);
    if (view->img) {
        cmd_batch_reloc(cmd, view->cmd[1], view->img->obj.mem->bo,
                        INTEL_RELOC_WRITE);
    } else {
        cmd_batch_write(cmd, 0);
    }
    cmd_batch_write(cmd, view->cmd[2]);
    cmd_batch_write(cmd, view->cmd[3]);
    cmd_batch_write(cmd, view->cmd[4]);
    cmd_batch_write(cmd, view->cmd[5]);
}

static void gen6_3DSTATE_STENCIL_BUFFER(struct intel_cmd *cmd,
                                        const struct intel_ds_view *view)
{
    const uint8_t cmd_len = 3;
    uint32_t dw0;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = (cmd_gen(cmd) >= INTEL_GEN(7)) ?
        GEN7_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER) :
        GEN6_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER);
    dw0 |= (cmd_len - 2);

    cmd_batch_reserve_reloc(cmd, cmd_len, (bool) view->img);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, view->cmd[6]);
    if (view->img) {
        cmd_batch_reloc(cmd, view->cmd[7], view->img->obj.mem->bo,
                        INTEL_RELOC_WRITE);
    } else {
        cmd_batch_write(cmd, 0);
    }
}

static void gen6_3DSTATE_HIER_DEPTH_BUFFER(struct intel_cmd *cmd,
                                           const struct intel_ds_view *view)
{
    const uint8_t cmd_len = 3;
    uint32_t dw0;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = (cmd_gen(cmd) >= INTEL_GEN(7)) ?
        GEN7_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER) :
        GEN6_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER);
    dw0 |= (cmd_len - 2);

    cmd_batch_reserve_reloc(cmd, cmd_len, (bool) view->img);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, view->cmd[8]);
    if (view->img) {
        cmd_batch_reloc(cmd, view->cmd[9], view->img->obj.mem->bo,
                        INTEL_RELOC_WRITE);
    } else {
        cmd_batch_write(cmd, 0);
    }
}

static void gen6_3DSTATE_CLEAR_PARAMS(struct intel_cmd *cmd,
                                      uint32_t clear_val)
{
    const uint8_t cmd_len = 2;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CLEAR_PARAMS) |
                         GEN6_CLEAR_PARAMS_DW0_VALID |
                         (cmd_len - 2);

    CMD_ASSERT(cmd, 6, 6);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, clear_val);
}

static void gen7_3DSTATE_CLEAR_PARAMS(struct intel_cmd *cmd,
                                      uint32_t clear_val)
{
    const uint8_t cmd_len = 3;
    const uint32_t dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_CLEAR_PARAMS) |
                         (cmd_len - 2);

    CMD_ASSERT(cmd, 7, 7.5);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, clear_val);
    cmd_batch_write(cmd, 1);
}

static void gen6_3DSTATE_CC_STATE_POINTERS(struct intel_cmd *cmd,
                                           XGL_UINT blend_pos,
                                           XGL_UINT ds_pos,
                                           XGL_UINT cc_pos)
{
    const uint8_t cmd_len = 4;
    uint32_t dw0;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CC_STATE_POINTERS) |
          (cmd_len - 2);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, (blend_pos << 2) | 1);
    cmd_batch_write(cmd, (ds_pos << 2) | 1);
    cmd_batch_write(cmd, (cc_pos << 2) | 1);
}

static void gen6_3DSTATE_VIEWPORT_STATE_POINTERS(struct intel_cmd *cmd,
                                                 XGL_UINT clip_pos,
                                                 XGL_UINT sf_pos,
                                                 XGL_UINT cc_pos)
{
    const uint8_t cmd_len = 4;
    uint32_t dw0;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_VIEWPORT_STATE_POINTERS) |
          GEN6_PTR_VP_DW0_CLIP_CHANGED |
          GEN6_PTR_VP_DW0_SF_CHANGED |
          GEN6_PTR_VP_DW0_CC_CHANGED |
          (cmd_len - 2);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, clip_pos << 2);
    cmd_batch_write(cmd, sf_pos << 2);
    cmd_batch_write(cmd, cc_pos << 2);
}

static void gen6_3DSTATE_SCISSOR_STATE_POINTERS(struct intel_cmd *cmd,
                                                XGL_UINT scissor_pos)
{
    const uint8_t cmd_len = 2;
    uint32_t dw0;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SCISSOR_STATE_POINTERS) |
          (cmd_len - 2);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, scissor_pos << 2);
}

static void gen6_3DSTATE_BINDING_TABLE_POINTERS(struct intel_cmd *cmd,
                                                XGL_UINT vs_pos,
                                                XGL_UINT gs_pos,
                                                XGL_UINT ps_pos)
{
    const uint8_t cmd_len = 4;
    uint32_t dw0;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_BINDING_TABLE_POINTERS) |
          GEN6_PTR_BINDING_TABLE_DW0_VS_CHANGED |
          GEN6_PTR_BINDING_TABLE_DW0_GS_CHANGED |
          GEN6_PTR_BINDING_TABLE_DW0_PS_CHANGED |
          (cmd_len - 2);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, vs_pos << 2);
    cmd_batch_write(cmd, gs_pos << 2);
    cmd_batch_write(cmd, ps_pos << 2);
}

static void gen6_3DSTATE_SAMPLER_STATE_POINTERS(struct intel_cmd *cmd,
                                                XGL_UINT vs_pos,
                                                XGL_UINT gs_pos,
                                                XGL_UINT ps_pos)
{
    const uint8_t cmd_len = 4;
    uint32_t dw0;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLER_STATE_POINTERS) |
          GEN6_PTR_SAMPLER_DW0_VS_CHANGED |
          GEN6_PTR_SAMPLER_DW0_GS_CHANGED |
          GEN6_PTR_SAMPLER_DW0_PS_CHANGED |
          (cmd_len - 2);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, vs_pos << 2);
    cmd_batch_write(cmd, gs_pos << 2);
    cmd_batch_write(cmd, ps_pos << 2);
}

static void gen7_3dstate_pointer(struct intel_cmd *cmd,
                                 int subop, XGL_UINT pos)
{
    const uint8_t cmd_len = 2;
    const uint32_t dw0 = GEN6_RENDER_TYPE_RENDER |
                         GEN6_RENDER_SUBTYPE_3D |
                         subop | (cmd_len - 2);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, pos << 2);
}

static XGL_UINT gen6_BLEND_STATE(struct intel_cmd *cmd,
                                 const struct intel_blend_state *state)
{
    const uint8_t cmd_align = GEN6_ALIGNMENT_BLEND_STATE;
    const uint8_t cmd_len = XGL_MAX_COLOR_ATTACHMENTS * 2;

    CMD_ASSERT(cmd, 6, 7.5);
    STATIC_ASSERT(ARRAY_SIZE(state->cmd) >= cmd_len);

    return cmd_state_copy(cmd, state->cmd, cmd_len, cmd_align);
}

static XGL_UINT gen6_DEPTH_STENCIL_STATE(struct intel_cmd *cmd,
                                         const struct intel_ds_state *state)
{
    const uint8_t cmd_align = GEN6_ALIGNMENT_DEPTH_STENCIL_STATE;
    const uint8_t cmd_len = 3;

    CMD_ASSERT(cmd, 6, 7.5);
    STATIC_ASSERT(ARRAY_SIZE(state->cmd) >= cmd_len);

    return cmd_state_copy(cmd, state->cmd, cmd_len, cmd_align);
}

static XGL_UINT gen6_COLOR_CALC_STATE(struct intel_cmd *cmd,
                                      uint32_t stencil_ref,
                                      const uint32_t blend_color[4])
{
    const uint8_t cmd_align = GEN6_ALIGNMENT_COLOR_CALC_STATE;
    const uint8_t cmd_len = 6;
    XGL_UINT pos;
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 7.5);

    dw = cmd_state_reserve(cmd, cmd_len, cmd_align, &pos);
    dw[0] = stencil_ref;
    dw[1] = 0;
    dw[2] = blend_color[0];
    dw[3] = blend_color[1];
    dw[4] = blend_color[2];
    dw[5] = blend_color[3];
    cmd_state_advance(cmd, cmd_len);

    return pos;
}

static void cmd_wa_gen6_pre_depth_stall_write(struct intel_cmd *cmd)
{
    CMD_ASSERT(cmd, 6, 7.5);

    if (!cmd->bind.draw_count)
        return;

    if (cmd->bind.wa_flags & INTEL_CMD_WA_GEN6_PRE_DEPTH_STALL_WRITE)
        return;

    cmd->bind.wa_flags |= INTEL_CMD_WA_GEN6_PRE_DEPTH_STALL_WRITE;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 60:
    *
    *     "Pipe-control with CS-stall bit set must be sent BEFORE the
    *      pipe-control with a post-sync op and no write-cache flushes."
    *
    * The workaround below necessitates this workaround.
    */
    gen6_PIPE_CONTROL(cmd,
            GEN6_PIPE_CONTROL_CS_STALL |
            GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL,
            NULL, 0, 0);

    gen6_PIPE_CONTROL(cmd, GEN6_PIPE_CONTROL_WRITE_IMM,
            cmd->scratch_bo, 0, 0);
}

static void cmd_wa_gen6_pre_command_scoreboard_stall(struct intel_cmd *cmd)
{
    CMD_ASSERT(cmd, 6, 7.5);

    if (!cmd->bind.draw_count)
        return;

    gen6_PIPE_CONTROL(cmd, GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL,
            NULL, 0, 0);
}

static void cmd_wa_gen7_pre_vs_depth_stall_write(struct intel_cmd *cmd)
{
    CMD_ASSERT(cmd, 7, 7.5);

    if (!cmd->bind.draw_count)
        return;

    cmd_wa_gen6_pre_depth_stall_write(cmd);

    gen6_PIPE_CONTROL(cmd,
            GEN6_PIPE_CONTROL_DEPTH_STALL | GEN6_PIPE_CONTROL_WRITE_IMM,
            cmd->scratch_bo, 0, 0);
}

static void cmd_wa_gen7_post_command_cs_stall(struct intel_cmd *cmd)
{
    CMD_ASSERT(cmd, 7, 7.5);

    if (!cmd->bind.draw_count)
        return;

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 61:
     *
     *     "One of the following must also be set (when CS stall is set):
     *
     *       * Render Target Cache Flush Enable ([12] of DW1)
     *       * Depth Cache Flush Enable ([0] of DW1)
     *       * Stall at Pixel Scoreboard ([1] of DW1)
     *       * Depth Stall ([13] of DW1)
     *       * Post-Sync Operation ([13] of DW1)"
     */
    gen6_PIPE_CONTROL(cmd,
            GEN6_PIPE_CONTROL_CS_STALL |
            GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL,
            NULL, 0, 0);
}

static void cmd_wa_gen7_post_command_depth_stall(struct intel_cmd *cmd)
{
    CMD_ASSERT(cmd, 7, 7.5);

    if (!cmd->bind.draw_count)
        return;

    cmd_wa_gen6_pre_depth_stall_write(cmd);

    gen6_PIPE_CONTROL(cmd, GEN6_PIPE_CONTROL_DEPTH_STALL, NULL, 0, 0);
}

static void cmd_wa_gen6_pre_multisample_depth_flush(struct intel_cmd *cmd)
{
    CMD_ASSERT(cmd, 6, 7.5);

    if (!cmd->bind.draw_count)
        return;

    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 305:
     *
     *     "Driver must guarentee that all the caches in the depth pipe are
     *      flushed before this command (3DSTATE_MULTISAMPLE) is parsed. This
     *      requires driver to send a PIPE_CONTROL with a CS stall along with
     *      a Depth Flush prior to this command."
     *
     * From the Ivy Bridge PRM, volume 2 part 1, page 304:
     *
     *     "Driver must ierarchi that all the caches in the depth pipe are
     *      flushed before this command (3DSTATE_MULTISAMPLE) is parsed. This
     *      requires driver to send a PIPE_CONTROL with a CS stall along with
     *      a Depth Flush prior to this command.
     */
    gen6_PIPE_CONTROL(cmd,
            GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
            GEN6_PIPE_CONTROL_CS_STALL,
            NULL, 0, 0);
}

static void cmd_wa_gen6_pre_ds_flush(struct intel_cmd *cmd)
{
    CMD_ASSERT(cmd, 6, 7.5);

    if (!cmd->bind.draw_count)
        return;

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 315:
     *
     *     "Driver must send a least one PIPE_CONTROL command with CS Stall
     *      and a post sync operation prior to the group of depth
     *      commands(3DSTATE_DEPTH_BUFFER, 3DSTATE_CLEAR_PARAMS,
     *      3DSTATE_STENCIL_BUFFER, and 3DSTATE_HIER_DEPTH_BUFFER)."
     *
     * This workaround satifies all the conditions.
     */
    cmd_wa_gen6_pre_depth_stall_write(cmd);

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 315:
     *
     *     "Restriction: Prior to changing Depth/Stencil Buffer state (i.e.,
     *      any combination of 3DSTATE_DEPTH_BUFFER, 3DSTATE_CLEAR_PARAMS,
     *      3DSTATE_STENCIL_BUFFER, 3DSTATE_HIER_DEPTH_BUFFER) SW must first
     *      issue a pipelined depth stall (PIPE_CONTROL with Depth Stall bit
     *      set), followed by a pipelined depth cache flush (PIPE_CONTROL with
     *      Depth Flush Bit set, followed by another pipelined depth stall
     *      (PIPE_CONTROL with Depth Stall Bit set), unless SW can otherwise
     *      guarantee that the pipeline from WM onwards is already flushed
     *      (e.g., via a preceding MI_FLUSH)."
     */
    gen6_PIPE_CONTROL(cmd, GEN6_PIPE_CONTROL_DEPTH_STALL, NULL, 0, 0);
    gen6_PIPE_CONTROL(cmd, GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH, NULL, 0, 0);
    gen6_PIPE_CONTROL(cmd, GEN6_PIPE_CONTROL_DEPTH_STALL, NULL, 0, 0);
}

void cmd_batch_flush(struct intel_cmd *cmd, uint32_t pipe_control_dw0)
{
    if (!cmd->bind.draw_count)
        return;

    assert(!(pipe_control_dw0 & GEN6_PIPE_CONTROL_WRITE__MASK));

    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 60:
     *
     *     "Before a PIPE_CONTROL with Write Cache Flush Enable =1, a
     *      PIPE_CONTROL with any non-zero post-sync-op is required."
     */
    if (pipe_control_dw0 & GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH)
        cmd_wa_gen6_pre_depth_stall_write(cmd);

    /*
     * From the Ivy Bridge PRM, volume 2 part 1, page 61:
     *
     *     "One of the following must also be set (when CS stall is set):
     *
     *       * Render Target Cache Flush Enable ([12] of DW1)
     *       * Depth Cache Flush Enable ([0] of DW1)
     *       * Stall at Pixel Scoreboard ([1] of DW1)
     *       * Depth Stall ([13] of DW1)
     *       * Post-Sync Operation ([13] of DW1)"
     */
    if ((pipe_control_dw0 & GEN6_PIPE_CONTROL_CS_STALL) &&
        !(pipe_control_dw0 & (GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH |
                              GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                              GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL |
                              GEN6_PIPE_CONTROL_DEPTH_STALL)))
        pipe_control_dw0 |= GEN6_PIPE_CONTROL_PIXEL_SCOREBOARD_STALL;

    gen6_PIPE_CONTROL(cmd, pipe_control_dw0, NULL, 0, 0);
}

void cmd_batch_depth_count(struct intel_cmd *cmd,
                           struct intel_bo *bo,
                           XGL_GPU_SIZE offset)
{
    cmd_wa_gen6_pre_depth_stall_write(cmd);

    gen6_PIPE_CONTROL(cmd,
            GEN6_PIPE_CONTROL_DEPTH_STALL |
            GEN6_PIPE_CONTROL_WRITE_PS_DEPTH_COUNT,
            bo, offset, 0);
}

void cmd_batch_timestamp(struct intel_cmd *cmd,
                         struct intel_bo *bo,
                         XGL_GPU_SIZE offset)
{
    /* need any WA or stall? */
    gen6_PIPE_CONTROL(cmd, GEN6_PIPE_CONTROL_WRITE_TIMESTAMP, bo, offset, 0);
}

void cmd_batch_immediate(struct intel_cmd *cmd,
                         struct intel_bo *bo,
                         XGL_GPU_SIZE offset,
                         uint64_t val)
{
    /* need any WA or stall? */
    gen6_PIPE_CONTROL(cmd, GEN6_PIPE_CONTROL_WRITE_IMM, bo, offset, val);
}

static void gen6_cc_states(struct intel_cmd *cmd)
{
    const struct intel_blend_state *blend = cmd->bind.state.blend;
    const struct intel_ds_state *ds = cmd->bind.state.ds;
    XGL_UINT blend_pos, ds_pos, cc_pos;
    uint32_t stencil_ref;
    uint32_t blend_color[4];

    CMD_ASSERT(cmd, 6, 6);

    if (blend) {
        blend_pos = gen6_BLEND_STATE(cmd, blend);
        memcpy(blend_color, blend->cmd_blend_color, sizeof(blend_color));
    } else {
        blend_pos = 0;
        memset(blend_color, 0, sizeof(blend_color));
    }

    if (ds) {
        ds_pos = gen6_DEPTH_STENCIL_STATE(cmd, ds);
        stencil_ref = ds->cmd_stencil_ref;
    } else {
        ds_pos = 0;
        stencil_ref = 0;
    }

    cc_pos = gen6_COLOR_CALC_STATE(cmd, stencil_ref, blend_color);

    gen6_3DSTATE_CC_STATE_POINTERS(cmd, blend_pos, ds_pos, cc_pos);
}

static void gen6_viewport_states(struct intel_cmd *cmd)
{
    const struct intel_viewport_state *viewport = cmd->bind.state.viewport;
    XGL_UINT pos;

    if (!viewport)
        return;

    pos = cmd_state_copy(cmd, viewport->cmd, viewport->cmd_len,
            viewport->cmd_align);

    gen6_3DSTATE_VIEWPORT_STATE_POINTERS(cmd,
            pos + viewport->cmd_clip_offset,
            pos,
            pos + viewport->cmd_cc_offset);

    pos = (viewport->scissor_enable) ?
        pos + viewport->cmd_scissor_rect_offset : 0;

    gen6_3DSTATE_SCISSOR_STATE_POINTERS(cmd, pos);
}

static void gen7_cc_states(struct intel_cmd *cmd)
{
    const struct intel_blend_state *blend = cmd->bind.state.blend;
    const struct intel_ds_state *ds = cmd->bind.state.ds;
    uint32_t stencil_ref;
    uint32_t blend_color[4];
    XGL_UINT pos;

    CMD_ASSERT(cmd, 7, 7.5);

    if (!blend && !ds)
        return;

    if (blend) {
        pos = gen6_BLEND_STATE(cmd, blend);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BLEND_STATE_POINTERS, pos);

        memcpy(blend_color, blend->cmd_blend_color, sizeof(blend_color));
    } else {
        memset(blend_color, 0, sizeof(blend_color));
    }

    if (ds) {
        pos = gen6_DEPTH_STENCIL_STATE(cmd, ds);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_DEPTH_STENCIL_STATE_POINTERS, pos);
    } else {
        stencil_ref = 0;
    }

    pos = gen6_COLOR_CALC_STATE(cmd, stencil_ref, blend_color);
    gen7_3dstate_pointer(cmd,
            GEN6_RENDER_OPCODE_3DSTATE_CC_STATE_POINTERS, pos);
}

static void gen7_viewport_states(struct intel_cmd *cmd)
{
    const struct intel_viewport_state *viewport = cmd->bind.state.viewport;
    XGL_UINT pos;

    if (!viewport)
        return;

    pos = cmd_state_copy(cmd, viewport->cmd, viewport->cmd_len,
            viewport->cmd_align);

    gen7_3dstate_pointer(cmd,
            GEN7_RENDER_OPCODE_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP, pos);
    gen7_3dstate_pointer(cmd,
            GEN7_RENDER_OPCODE_3DSTATE_VIEWPORT_STATE_POINTERS_CC,
            pos + viewport->cmd_cc_offset);
    if (viewport->scissor_enable) {
        gen7_3dstate_pointer(cmd,
                GEN6_RENDER_OPCODE_3DSTATE_SCISSOR_STATE_POINTERS,
                pos + viewport->cmd_scissor_rect_offset);
    }
}

static void gen6_pcb(struct intel_cmd *cmd, int subop,
                     const XGL_PIPELINE_SHADER *sh)
{
    const uint8_t cmd_len = 5;
    const XGL_UINT alignment = 32;
    const XGL_UINT max_size =
        (subop == GEN6_RENDER_OPCODE_3DSTATE_CONSTANT_VS) ? 1024 : 2048;
    const XGL_UINT max_pcb = 4;
    uint32_t pcb[4] = { 0, 0, 0, 0 };
    XGL_FLAGS pcb_enables = 0;
    XGL_SIZE total_size = 0;
    uint32_t dw0;
    XGL_UINT i;

    for (i = 0; i < sh->linkConstBufferCount; i++) {
        const XGL_LINK_CONST_BUFFER *info = &sh->pLinkConstBufferInfo[i];
        const XGL_SIZE size = u_align(info->bufferSize, alignment);
        void *ptr;

        if (info->bufferId >= max_pcb ||
            pcb_enables & ((1 << info->bufferId)) ||
            total_size + info->bufferSize > max_size) {
            cmd->result = XGL_ERROR_UNKNOWN;
            return;
        }
        if (!size)
            continue;

        pcb_enables |= 1 << info->bufferId;
        total_size += size;

        ptr = cmd_state_reserve(cmd, size / sizeof(uint32_t),
                alignment / sizeof(uint32_t), &pcb[info->bufferId]);
        memcpy(ptr, info->pBufferData, info->bufferSize);
        cmd_state_advance(cmd, size / sizeof(uint32_t));

        pcb[info->bufferId] |= size / alignment - 1;
    }

    dw0 = GEN6_RENDER_TYPE_RENDER |
          GEN6_RENDER_SUBTYPE_3D |
          subop |
          pcb_enables << 12 |
          (cmd_len - 2);

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, pcb[0]);
    cmd_batch_write(cmd, pcb[1]);
    cmd_batch_write(cmd, pcb[2]);
    cmd_batch_write(cmd, pcb[3]);
}

static void gen7_pcb(struct intel_cmd *cmd, int subop,
                     const XGL_PIPELINE_SHADER *sh)
{
    const uint8_t cmd_len = 7;
    const uint32_t dw0 = GEN6_RENDER_TYPE_RENDER |
                         GEN6_RENDER_SUBTYPE_3D |
                         subop |
                         (cmd_len - 2);
    const XGL_UINT alignment = 32;
    const XGL_UINT max_size = 2048;
    const XGL_UINT max_pcb = 4;
    uint16_t pcb_len[4] = { 0, 0, 0, 0 };
    uint32_t pcb[4] = { 0, 0, 0, 0 };
    XGL_FLAGS pcb_enables = 0;
    XGL_SIZE total_size = 0;
    XGL_UINT i;

    for (i = 0; i < sh->linkConstBufferCount; i++) {
        const XGL_LINK_CONST_BUFFER *info = &sh->pLinkConstBufferInfo[i];
        const XGL_SIZE size = u_align(info->bufferSize, alignment);
        void *ptr;

        if (info->bufferId >= max_pcb ||
            pcb_enables & ((1 << info->bufferId)) ||
            total_size + info->bufferSize > max_size) {
            cmd->result = XGL_ERROR_UNKNOWN;
            return;
        }
        if (!size)
            continue;

        pcb_enables |= 1 << info->bufferId;
        total_size += size;

        pcb_len[info->bufferId] = size / alignment;

        ptr = cmd_state_reserve(cmd, size / sizeof(uint32_t),
                alignment / sizeof(uint32_t), &pcb[info->bufferId]);
        memcpy(ptr, info->pBufferData, info->bufferSize);
        cmd_state_advance(cmd, size / sizeof(uint32_t));
    }

    /* no holes */
    if (!u_is_pow2(pcb_enables + 1)) {
        cmd->result = XGL_ERROR_UNKNOWN;
        return;
    }

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, pcb_len[1] << 16 | pcb_len[0]);
    cmd_batch_write(cmd, pcb_len[3] << 16 | pcb_len[2]);
    cmd_batch_write(cmd, pcb[0]);
    cmd_batch_write(cmd, pcb[1]);
    cmd_batch_write(cmd, pcb[2]);
    cmd_batch_write(cmd, pcb[3]);
}

static void emit_ps_resources(struct intel_cmd *cmd,
                              const struct intel_rmap *rmap)
{
    const XGL_UINT surface_count = rmap->rt_count +
        rmap->resource_count + rmap->uav_count;
    uint32_t binding_table[256];
    XGL_UINT pos, i;

    assert(surface_count <= ARRAY_SIZE(binding_table));

    for (i = 0; i < surface_count; i++) {
        const struct intel_rmap_slot *slot = &rmap->slots[i];
        uint32_t *dw;

        switch (slot->path_len) {
        case 0:
            pos = 0;
            break;
        case INTEL_RMAP_SLOT_RT:
            {
                const struct intel_rt_view *view = cmd->bind.att.rt[i];

                dw = cmd_state_reserve_reloc(cmd, view->cmd_len, 1,
                        GEN6_ALIGNMENT_SURFACE_STATE, &pos);

                memcpy(dw, view->cmd, sizeof(uint32_t) * view->cmd_len);
                cmd_state_reloc(cmd, 1, view->cmd[1], view->img->obj.mem->bo,
                        INTEL_RELOC_WRITE);
                cmd_state_advance(cmd, view->cmd_len);
            }
            break;
        case INTEL_RMAP_SLOT_DYN:
            {
                const struct intel_mem_view *view =
                    &cmd->bind.dyn_view.graphics;

                dw = cmd_state_reserve_reloc(cmd, view->cmd_len, 1,
                        GEN6_ALIGNMENT_SURFACE_STATE, &pos);

                memcpy(dw, view->cmd, sizeof(uint32_t) * view->cmd_len);
                cmd_state_reloc(cmd, 1, view->cmd[1], view->mem->bo,
                        INTEL_RELOC_WRITE);
                cmd_state_advance(cmd, view->cmd_len);
            }
            break;
        case 1:
        default:
            /* TODO */
            assert(!"no dset support");
            break;
        }

        binding_table[i] = pos << 2;
    }

    pos = cmd_state_copy(cmd, binding_table, surface_count,
            GEN6_ALIGNMENT_BINDING_TABLE_STATE);

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_PS, pos);

        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_VS, 0);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_HS, 0);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_DS, 0);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_GS, 0);

        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_VS, 0);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_HS, 0);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_DS, 0);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_GS, 0);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_PS, 0);
    } else {
        gen6_3DSTATE_BINDING_TABLE_POINTERS(cmd, 0, 0, pos);
        gen6_3DSTATE_SAMPLER_STATE_POINTERS(cmd, 0, 0, 0);
    }
}

// TODO: These should probably be generated
/* DW2 */
# define GEN6_VS_SPF_MODE				(1 << 31)
# define GEN6_VS_VECTOR_MASK_ENABLE			(1 << 30)
# define GEN6_VS_SAMPLER_COUNT_SHIFT			27
# define GEN6_VS_BINDING_TABLE_ENTRY_COUNT_SHIFT	18
# define GEN6_VS_FLOATING_POINT_MODE_IEEE_754		(0 << 16)
# define GEN6_VS_FLOATING_POINT_MODE_ALT		(1 << 16)

static void gen6_3DSTATE_VS(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = GEN6_3DSTATE_VS__SIZE;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_VS) | (cmd_len - 2);
    uint32_t dw2, dw4, dw5;

    CMD_ASSERT(cmd, 6, 7.5);

    /* From the BSpec, 3D Pipeline > Geometry > Vertex Shader > State,
     * 3DSTATE_VS, Dword 5.0 "VS Function Enable":
     *
     *   [DevSNB] A pipeline flush must be programmed prior to a 3DSTATE_VS
     *   command that causes the VS Function Enable to toggle. Pipeline
     *   flush can be executed by sending a PIPE_CONTROL command with CS
     *   stall bit set and a post sync operation.
     *
     * Although we don't disable the VS during normal drawing, BLORP sometimes
     * disables it.  To be safe, do the flush here just in case.
     */
    cmd_wa_gen6_pre_depth_stall_write(cmd);

    if (cmd->bind.vs.shader == NULL) {
        cmd_batch_reserve(cmd, cmd_len);
        cmd_batch_write(cmd, dw0);
        cmd_batch_write(cmd, 0);
        cmd_batch_write(cmd, 0);
        cmd_batch_write(cmd, 0);
        cmd_batch_write(cmd, 0);
        cmd_batch_write(cmd, 0);
        return;
    }

    /*
     * Most of this is know at pipeline create EXCEPT the kernel address,
     * so that's why this is in cmd_pipeline vs. pipeline.
     */
    dw2 = (u_align(cmd->bind.vs.shader->sampler_count, 4) / 4) << GEN6_VS_SAMPLER_COUNT_SHIFT;
    dw4 = (1 << GEN6_VS_DW4_URB_GRF_START__SHIFT) |
          (cmd->bind.vs.shader->urb_read_length << GEN6_VS_DW4_URB_READ_LEN__SHIFT) |
          (0 << GEN6_VS_DW4_URB_READ_OFFSET__SHIFT);

    dw5 = GEN6_VS_DW5_STATISTICS |
          GEN6_VS_DW5_VS_ENABLE;
    if (cmd_gen(cmd) == INTEL_GEN(7.5)) {
        dw5 |= ((cmd->dev->gpu->max_vs_threads-1) << GEN75_VS_DW5_MAX_THREADS__SHIFT);
    } else {
        dw5 |= ((cmd->dev->gpu->max_vs_threads-1) << GEN6_VS_DW5_MAX_THREADS__SHIFT);
    }

    cmd_batch_reserve(cmd, cmd_len);
    cmd_batch_write(cmd, dw0);
    cmd_batch_write(cmd, cmd->bind.vs.kernel_pos);
    cmd_batch_write(cmd, dw2);
    cmd_batch_write(cmd, 0); /* scratch */
    cmd_batch_write(cmd, dw4);
    cmd_batch_write(cmd, dw5);
}

static void emit_bounded_states(struct intel_cmd *cmd)
{
    const struct intel_msaa_state *msaa = cmd->bind.state.msaa;

    /* TODO more states */

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        gen7_cc_states(cmd);
        gen7_viewport_states(cmd);

        gen7_pcb(cmd, GEN6_RENDER_OPCODE_3DSTATE_CONSTANT_VS,
                &cmd->bind.pipeline.graphics->vs);
        gen7_pcb(cmd, GEN6_RENDER_OPCODE_3DSTATE_CONSTANT_PS,
                &cmd->bind.pipeline.graphics->fs);

        gen6_3DSTATE_CLIP(cmd);
        gen7_3DSTATE_SF(cmd);
        gen7_3DSTATE_SBE(cmd);
        gen7_3DSTATE_WM(cmd);
        gen7_3DSTATE_PS(cmd);
    } else {
        gen6_cc_states(cmd);
        gen6_viewport_states(cmd);

        gen6_pcb(cmd, GEN6_RENDER_OPCODE_3DSTATE_CONSTANT_VS,
                &cmd->bind.pipeline.graphics->vs);
        gen6_pcb(cmd, GEN6_RENDER_OPCODE_3DSTATE_CONSTANT_PS,
                &cmd->bind.pipeline.graphics->fs);

        gen6_3DSTATE_CLIP(cmd);
        gen6_3DSTATE_SF(cmd);
        gen6_3DSTATE_WM(cmd);
    }

    emit_ps_resources(cmd, cmd->bind.pipeline.graphics->intel_fs.rmap);

    cmd_wa_gen6_pre_depth_stall_write(cmd);
    cmd_wa_gen6_pre_multisample_depth_flush(cmd);
    /* 3DSTATE_MULTISAMPLE and 3DSTATE_SAMPLE_MASK */
    cmd_batch_reserve(cmd, msaa->cmd_len);
    cmd_batch_write_n(cmd, msaa->cmd, msaa->cmd_len);

    gen6_3DSTATE_VS(cmd);
}

static void emit_shader(struct intel_cmd *cmd,
                        const struct intel_pipe_shader *shader,
                        struct intel_cmd_shader *pCmdShader)
{
    uint32_t i;
    struct intel_cmd_shader *cmdShader;

    for (i=0; i<cmd->bind.shaderCache.used; i++) {
        if (cmd->bind.shaderCache.shaderArray[i].shader == shader) {
            /* shader is already part of pipeline */
            return;
        }
    }

    if (cmd->bind.shaderCache.used == cmd->bind.shaderCache.count) {
        const XGL_UINT new_count = cmd->bind.shaderCache.count + 16;

        cmdShader = cmd->bind.shaderCache.shaderArray;

        cmd->bind.shaderCache.shaderArray =
            icd_alloc(sizeof(*cmdShader) * new_count,
                    0, XGL_SYSTEM_ALLOC_INTERNAL);
        if (cmd->bind.shaderCache.shaderArray == NULL) {
            cmd->bind.shaderCache.shaderArray = cmdShader;
            cmd->result = XGL_ERROR_OUT_OF_MEMORY;
            return;
        }

        if (cmdShader) {
            memcpy(cmd->bind.shaderCache.shaderArray, cmdShader,
                    sizeof(*cmdShader) * cmd->bind.shaderCache.used);
            icd_free(cmdShader);
        }

        cmd->bind.shaderCache.count = new_count;
    }

    cmdShader = &cmd->bind.shaderCache.shaderArray[cmd->bind.shaderCache.used];
    cmdShader->shader = shader;
    cmdShader->kernel_pos = cmd_kernel_copy(cmd, shader->pCode, shader->codeSize);
    *pCmdShader = *cmdShader;
    cmd->bind.shaderCache.used++;
    return;
}

static void cmd_bind_graphics_pipeline(struct intel_cmd *cmd,
                                       const struct intel_pipeline *pipeline)
{
    cmd->bind.pipeline.graphics = pipeline;

    if (pipeline->wa_flags & INTEL_CMD_WA_GEN6_PRE_DEPTH_STALL_WRITE)
        cmd_wa_gen6_pre_depth_stall_write(cmd);
    if (pipeline->wa_flags & INTEL_CMD_WA_GEN6_PRE_COMMAND_SCOREBOARD_STALL)
        cmd_wa_gen6_pre_command_scoreboard_stall(cmd);
    if (pipeline->wa_flags & INTEL_CMD_WA_GEN7_PRE_VS_DEPTH_STALL_WRITE)
        cmd_wa_gen7_pre_vs_depth_stall_write(cmd);

    /* 3DSTATE_URB_VS and etc. */
    assert(pipeline->cmd_len);
    cmd_batch_reserve(cmd, pipeline->cmd_len);
    cmd_batch_write_n(cmd, pipeline->cmds, pipeline->cmd_len);

    if (pipeline->active_shaders & SHADER_VERTEX_FLAG) {
        emit_shader(cmd, &pipeline->intel_vs, &cmd->bind.vs);
    }
    if (pipeline->active_shaders & SHADER_GEOMETRY_FLAG) {
        emit_shader(cmd, &pipeline->gs, &cmd->bind.gs);
    }
    if (pipeline->active_shaders & SHADER_FRAGMENT_FLAG) {
        emit_shader(cmd, &pipeline->intel_fs, &cmd->bind.fs);
    }
    if (pipeline->active_shaders & SHADER_TESS_CONTROL_FLAG) {
        emit_shader(cmd, &pipeline->tess_control, &cmd->bind.tess_control);
    }
    if (pipeline->active_shaders & SHADER_TESS_EVAL_FLAG) {
        emit_shader(cmd, &pipeline->tess_eval, &cmd->bind.tess_eval);
    }

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        gen7_3DSTATE_GS(cmd);
    } else {
        gen6_3DSTATE_GS(cmd);
    }

    if (pipeline->wa_flags & INTEL_CMD_WA_GEN7_POST_COMMAND_CS_STALL)
        cmd_wa_gen7_post_command_cs_stall(cmd);
    if (pipeline->wa_flags & INTEL_CMD_WA_GEN7_POST_COMMAND_DEPTH_STALL)
        cmd_wa_gen7_post_command_depth_stall(cmd);
}

static void cmd_bind_compute_pipeline(struct intel_cmd *cmd,
                                      const struct intel_pipeline *pipeline)
{
    cmd->bind.pipeline.compute = pipeline;
}

static void cmd_bind_graphics_delta(struct intel_cmd *cmd,
                                    const struct intel_pipeline_delta *delta)
{
    cmd->bind.pipeline.graphics_delta = delta;
}

static void cmd_bind_compute_delta(struct intel_cmd *cmd,
                                   const struct intel_pipeline_delta *delta)
{
    cmd->bind.pipeline.compute_delta = delta;
}

static void cmd_bind_graphics_dset(struct intel_cmd *cmd,
                                   const struct intel_dset *dset,
                                   XGL_UINT slot_offset)
{
    cmd->bind.dset.graphics = dset;
    cmd->bind.dset.graphics_offset = slot_offset;
}

static void cmd_bind_compute_dset(struct intel_cmd *cmd,
                                  const struct intel_dset *dset,
                                  XGL_UINT slot_offset)
{
    cmd->bind.dset.compute = dset;
    cmd->bind.dset.compute_offset = slot_offset;
}

static void cmd_bind_graphics_dyn_view(struct intel_cmd *cmd,
                                       const XGL_MEMORY_VIEW_ATTACH_INFO *info)
{
    intel_mem_view_init(&cmd->bind.dyn_view.graphics, cmd->dev, info);
}

static void cmd_bind_compute_dyn_view(struct intel_cmd *cmd,
                                      const XGL_MEMORY_VIEW_ATTACH_INFO *info)
{
    intel_mem_view_init(&cmd->bind.dyn_view.compute, cmd->dev, info);
}

static void cmd_bind_index_data(struct intel_cmd *cmd,
                                const struct intel_mem *mem,
                                XGL_GPU_SIZE offset, XGL_INDEX_TYPE type)
{
    if (cmd_gen(cmd) >= INTEL_GEN(7.5)) {
        gen6_3DSTATE_INDEX_BUFFER(cmd, mem, offset, type, false);
    } else {
        cmd->bind.index.mem = mem;
        cmd->bind.index.offset = offset;
        cmd->bind.index.type = type;
    }
}

static void cmd_bind_rt(struct intel_cmd *cmd,
                        const XGL_COLOR_ATTACHMENT_BIND_INFO *attachments,
                        XGL_UINT count)
{
    XGL_UINT width = 0, height = 0;
    XGL_UINT i;

    for (i = 0; i < count; i++) {
        const XGL_COLOR_ATTACHMENT_BIND_INFO *att = &attachments[i];
        const struct intel_rt_view *rt = intel_rt_view(att->view);
        const struct intel_layout *layout = &rt->img->layout;

        if (i == 0) {
            width = layout->width0;
            height = layout->height0;
        } else {
            if (width > layout->width0)
                width = layout->width0;
            if (height > layout->height0)
                height = layout->height0;
        }

        cmd->bind.att.rt[i] = rt;
    }

    cmd->bind.att.rt_count = count;

    cmd_wa_gen6_pre_depth_stall_write(cmd);
    gen6_3DSTATE_DRAWING_RECTANGLE(cmd, width, height);
}

static void cmd_bind_ds(struct intel_cmd *cmd,
                        const XGL_DEPTH_STENCIL_BIND_INFO *info)
{
    const struct intel_ds_view *ds;

    if (info) {
        cmd->bind.att.ds = intel_ds_view(info->view);
        ds = cmd->bind.att.ds;
    } else {
        /* all zeros */
        static const struct intel_ds_view null_ds;
        ds = &null_ds;
    }

    cmd_wa_gen6_pre_ds_flush(cmd);
    gen6_3DSTATE_DEPTH_BUFFER(cmd, ds);
    gen6_3DSTATE_STENCIL_BUFFER(cmd, ds);
    gen6_3DSTATE_HIER_DEPTH_BUFFER(cmd, ds);

    if (cmd_gen(cmd) >= INTEL_GEN(7))
        gen7_3DSTATE_CLEAR_PARAMS(cmd, 0);
    else
        gen6_3DSTATE_CLEAR_PARAMS(cmd, 0);
}

static void cmd_bind_viewport_state(struct intel_cmd *cmd,
                                    const struct intel_viewport_state *state)
{
    cmd->bind.state.viewport = state;
}

static void cmd_bind_raster_state(struct intel_cmd *cmd,
                                  const struct intel_raster_state *state)
{
    cmd->bind.state.raster = state;
}

static void cmd_bind_ds_state(struct intel_cmd *cmd,
                              const struct intel_ds_state *state)
{
    cmd->bind.state.ds = state;
}

static void cmd_bind_blend_state(struct intel_cmd *cmd,
                                 const struct intel_blend_state *state)
{
    cmd->bind.state.blend = state;
}

static void cmd_bind_msaa_state(struct intel_cmd *cmd,
                                const struct intel_msaa_state *state)
{
    cmd->bind.state.msaa = state;
}

static void cmd_draw(struct intel_cmd *cmd,
                     XGL_UINT vertex_start,
                     XGL_UINT vertex_count,
                     XGL_UINT instance_start,
                     XGL_UINT instance_count,
                     bool indexed,
                     XGL_UINT vertex_base)
{
    const struct intel_pipeline *p = cmd->bind.pipeline.graphics;

    emit_bounded_states(cmd);

    if (indexed) {
        if (p->primitive_restart && !gen6_can_primitive_restart(cmd))
            cmd->result = XGL_ERROR_UNKNOWN;

        if (cmd_gen(cmd) >= INTEL_GEN(7.5)) {
            gen75_3DSTATE_VF(cmd, p->primitive_restart,
                    p->primitive_restart_index);
        } else {
            gen6_3DSTATE_INDEX_BUFFER(cmd, cmd->bind.index.mem,
                    cmd->bind.index.offset, cmd->bind.index.type,
                    p->primitive_restart);
        }
    } else {
        assert(!vertex_base);
    }

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        gen7_3DPRIMITIVE(cmd, p->prim_type, indexed, vertex_count,
                vertex_start, instance_count, instance_start, vertex_base);
    } else {
        gen6_3DPRIMITIVE(cmd, p->prim_type, indexed, vertex_count,
                vertex_start, instance_count, instance_start, vertex_base);
    }

    cmd->bind.draw_count++;
    /* need to re-emit all workarounds */
    cmd->bind.wa_flags = 0;
}

XGL_VOID XGLAPI intelCmdBindPipeline(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE                                pipeline)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    switch (pipelineBindPoint) {
    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        cmd_bind_compute_pipeline(cmd, intel_pipeline(pipeline));
        break;
    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        cmd_bind_graphics_pipeline(cmd, intel_pipeline(pipeline));
        break;
    default:
        cmd->result = XGL_ERROR_INVALID_VALUE;
        break;
    }
}

XGL_VOID XGLAPI intelCmdBindPipelineDelta(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE_DELTA                          delta)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    switch (pipelineBindPoint) {
    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        cmd_bind_compute_delta(cmd, delta);
        break;
    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        cmd_bind_graphics_delta(cmd, delta);
        break;
    default:
        cmd->result = XGL_ERROR_INVALID_VALUE;
        break;
    }
}

XGL_VOID XGLAPI intelCmdBindStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_STATE_OBJECT                            state)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    switch (stateBindPoint) {
    case XGL_STATE_BIND_VIEWPORT:
        cmd_bind_viewport_state(cmd,
                intel_viewport_state((XGL_VIEWPORT_STATE_OBJECT) state));
        break;
    case XGL_STATE_BIND_RASTER:
        cmd_bind_raster_state(cmd,
                intel_raster_state((XGL_RASTER_STATE_OBJECT) state));
        break;
    case XGL_STATE_BIND_DEPTH_STENCIL:
        cmd_bind_ds_state(cmd,
                intel_ds_state((XGL_DEPTH_STENCIL_STATE_OBJECT) state));
        break;
    case XGL_STATE_BIND_COLOR_BLEND:
        cmd_bind_blend_state(cmd,
                intel_blend_state((XGL_COLOR_BLEND_STATE_OBJECT) state));
        break;
    case XGL_STATE_BIND_MSAA:
        cmd_bind_msaa_state(cmd,
                intel_msaa_state((XGL_MSAA_STATE_OBJECT) state));
        break;
    default:
        cmd->result = XGL_ERROR_INVALID_VALUE;
        break;
    }
}

XGL_VOID XGLAPI intelCmdBindDescriptorSet(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    index,
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    slotOffset)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_dset *dset = intel_dset(descriptorSet);

    assert(!index);

    switch (pipelineBindPoint) {
    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        cmd_bind_compute_dset(cmd, dset, slotOffset);
        break;
    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        cmd_bind_graphics_dset(cmd, dset, slotOffset);
        break;
    default:
        cmd->result = XGL_ERROR_INVALID_VALUE;
        break;
    }
}

XGL_VOID XGLAPI intelCmdBindDynamicMemoryView(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    switch (pipelineBindPoint) {
    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        cmd_bind_compute_dyn_view(cmd, pMemView);
        break;
    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        cmd_bind_graphics_dyn_view(cmd, pMemView);
        break;
    default:
        cmd->result = XGL_ERROR_INVALID_VALUE;
        break;
    }
}

XGL_VOID XGLAPI intelCmdBindIndexData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem_,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_mem *mem = intel_mem(mem_);

    cmd_bind_index_data(cmd, mem, offset, indexType);
}

XGL_VOID XGLAPI intelCmdBindAttachments(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    colorAttachmentCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*       pColorAttachments,
    const XGL_DEPTH_STENCIL_BIND_INFO*          pDepthStencilAttachment)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_bind_rt(cmd, pColorAttachments, colorAttachmentCount);
    cmd_bind_ds(cmd, pDepthStencilAttachment);
}

XGL_VOID XGLAPI intelCmdDraw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstVertex,
    XGL_UINT                                    vertexCount,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_draw(cmd, firstVertex, vertexCount,
            firstInstance, instanceCount, false, 0);
}

XGL_VOID XGLAPI intelCmdDrawIndexed(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstIndex,
    XGL_UINT                                    indexCount,
    XGL_INT                                     vertexOffset,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_draw(cmd, firstIndex, indexCount,
            firstInstance, instanceCount, true, vertexOffset);
}

XGL_VOID XGLAPI intelCmdDrawIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd->result = XGL_ERROR_UNKNOWN;
}

XGL_VOID XGLAPI intelCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd->result = XGL_ERROR_UNKNOWN;
}

XGL_VOID XGLAPI intelCmdDispatch(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    x,
    XGL_UINT                                    y,
    XGL_UINT                                    z)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd->result = XGL_ERROR_UNKNOWN;
}

XGL_VOID XGLAPI intelCmdDispatchIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd->result = XGL_ERROR_UNKNOWN;
}
