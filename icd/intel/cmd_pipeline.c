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
#include "sampler.h"
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
    uint32_t dw0, *dw;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DPRIMITIVE) |
          prim_type << GEN6_3DPRIM_DW0_TYPE__SHIFT |
          (cmd_len - 2);

    if (indexed)
        dw0 |= GEN6_3DPRIM_DW0_ACCESS_RANDOM;

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = vertex_count;
    dw[2] = vertex_start;
    dw[3] = instance_count;
    dw[4] = instance_start;
    dw[5] = vertex_base;
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
    uint32_t dw0, dw1, *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    dw0 = GEN6_RENDER_CMD(3D, 3DPRIMITIVE) | (cmd_len - 2);
    dw1 = prim_type << GEN7_3DPRIM_DW1_TYPE__SHIFT;

    if (indexed)
        dw1 |= GEN7_3DPRIM_DW1_ACCESS_RANDOM;

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = dw1;
    dw[2] = vertex_count;
    dw[3] = vertex_start;
    dw[4] = instance_count;
    dw[5] = instance_start;
    dw[6] = vertex_base;
}

static void gen6_PIPE_CONTROL(struct intel_cmd *cmd, uint32_t dw1,
                              struct intel_bo *bo, uint32_t bo_offset,
                              uint64_t imm)
{
   const uint8_t cmd_len = 5;
   const uint32_t dw0 = GEN6_RENDER_CMD(3D, PIPE_CONTROL) |
                        (cmd_len - 2);
   uint32_t reloc_flags = INTEL_RELOC_WRITE;
   uint32_t *dw;
   XGL_UINT pos;

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

   pos = cmd_batch_pointer(cmd, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = 0;
   dw[3] = (uint32_t) imm;
   dw[4] = (uint32_t) (imm >> 32);

   if (bo) {
       cmd_reserve_reloc(cmd, 1);
       cmd_batch_reloc(cmd, pos + 2, bo, bo_offset, reloc_flags);
   }
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
    uint32_t dw0, end_offset, *dw;
    unsigned offset_align;
    XGL_UINT pos;

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

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;

    cmd_reserve_reloc(cmd, 2);
    cmd_batch_reloc(cmd, pos + 1, mem->bo, offset, 0);
    cmd_batch_reloc(cmd, pos + 2, mem->bo, end_offset, 0);
}

static void gen75_3DSTATE_VF(struct intel_cmd *cmd,
                             bool enable_cut_index,
                             uint32_t cut_index)
{
    const uint8_t cmd_len = 2;
    uint32_t dw0, *dw;

    CMD_ASSERT(cmd, 7.5, 7.5);

    dw0 = GEN75_RENDER_CMD(3D, 3DSTATE_VF) | (cmd_len - 2);
    if (enable_cut_index)
        dw0 |=  GEN75_VF_DW0_CUT_INDEX_ENABLE;

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = cut_index;
}


static void gen6_3DSTATE_GS(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 7;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_GS) | (cmd_len - 2);
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 6);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 1 << GEN6_GS_DW4_URB_READ_LEN__SHIFT;
    dw[5] = GEN6_GS_DW5_STATISTICS;
    dw[6] = 0;
}

static void gen7_3DSTATE_GS(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 7;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_GS) | (cmd_len - 2);
    uint32_t *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 0;
    dw[5] = GEN6_GS_DW5_STATISTICS;
    dw[6] = 0;
}

static void gen6_3DSTATE_DRAWING_RECTANGLE(struct intel_cmd *cmd,
                                           XGL_UINT width, XGL_UINT height)
{
    const uint8_t cmd_len = 4;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_DRAWING_RECTANGLE) |
                         (cmd_len - 2);
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 7.5);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;

    if (width && height) {
        dw[1] = 0;
        dw[2] = (height - 1) << 16 |
                (width - 1);
    } else {
        dw[1] = 1;
        dw[2] = 0;
    }

    dw[3] = 0;
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
    const struct intel_pipeline_shader *vs = &cmd->bind.pipeline.graphics->vs;
    const struct intel_pipeline_shader *fs = &cmd->bind.pipeline.graphics->fs;
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
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 6);

    gen7_fill_3DSTATE_SF_body(cmd, sf);
    gen7_fill_3DSTATE_SBE_body(cmd, sbe);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = sbe[0];
    memcpy(&dw[2], sf, sizeof(sf));
    memcpy(&dw[8], &sbe[1], sizeof(sbe) - sizeof(sbe[0]));
}

static void gen7_3DSTATE_SF(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 7;
    uint32_t *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SF) |
            (cmd_len - 2);
    gen7_fill_3DSTATE_SF_body(cmd, &dw[1]);
}

static void gen7_3DSTATE_SBE(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 14;
    uint32_t *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_SBE) |
            (cmd_len - 2);
    gen7_fill_3DSTATE_SBE_body(cmd, &dw[1]);
}

static void gen6_3DSTATE_CLIP(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 4;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CLIP) |
                         (cmd_len - 2);
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_pipeline_shader *fs = &pipeline->fs;
    const struct intel_viewport_state *viewport = cmd->bind.state.viewport;
    const struct intel_raster_state *raster = cmd->bind.state.raster;
    uint32_t dw1, dw2, dw3, *dw;

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

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = dw1;
    dw[2] = dw2;
    dw[3] = dw3;
}

static void gen6_3DSTATE_WM(struct intel_cmd *cmd)
{
    const int max_threads = (cmd->dev->gpu->gt == 2) ? 80 : 40;
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_pipeline_shader *fs = &pipeline->fs;
    const struct intel_msaa_state *msaa = cmd->bind.state.msaa;
    const uint8_t cmd_len = 9;
    uint32_t dw0, dw2, dw4, dw5, dw6, *dw;

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

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = cmd->bind.pipeline.fs_offset;
    dw[2] = dw2;
    dw[3] = 0; /* scratch */
    dw[4] = dw4;
    dw[5] = dw5;
    dw[6] = dw6;
    dw[7] = 0; /* kernel 1 */
    dw[8] = 0; /* kernel 2 */
}

static void gen7_3DSTATE_WM(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_pipeline_shader *fs = &pipeline->fs;
    const struct intel_msaa_state *msaa = cmd->bind.state.msaa;
    const uint8_t cmd_len = 3;
    uint32_t dw0, dw1, dw2, *dw;

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

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = dw1;
    dw[2] = dw2;
}

static void gen7_3DSTATE_PS(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_pipeline_shader *fs = &pipeline->fs;
    const struct intel_msaa_state *msaa = cmd->bind.state.msaa;
    const uint8_t cmd_len = 8;
    uint32_t dw0, dw2, dw4, dw5, *dw;

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

    if (fs->in_count)
        dw4 |= GEN7_PS_DW4_ATTR_ENABLE;

    if (pipeline->cb_state.dualSourceBlendEnable)
        dw4 |= GEN7_PS_DW4_DUAL_SOURCE_BLEND;

    dw5 = fs->urb_grf_start << GEN7_PS_DW5_URB_GRF_START0__SHIFT |
          0 << GEN7_PS_DW5_URB_GRF_START1__SHIFT |
          0 << GEN7_PS_DW5_URB_GRF_START2__SHIFT;

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = cmd->bind.pipeline.fs_offset;
    dw[2] = dw2;
    dw[3] = 0; /* scratch */
    dw[4] = dw4;
    dw[5] = dw5;
    dw[6] = 0; /* kernel 1 */
    dw[7] = 0; /* kernel 2 */
}

static void gen6_3DSTATE_DEPTH_BUFFER(struct intel_cmd *cmd,
                                      const struct intel_ds_view *view)
{
    const uint8_t cmd_len = 7;
    uint32_t dw0, *dw;
    XGL_UINT pos;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = (cmd_gen(cmd) >= INTEL_GEN(7)) ?
        GEN7_RENDER_CMD(3D, 3DSTATE_DEPTH_BUFFER) :
        GEN6_RENDER_CMD(3D, 3DSTATE_DEPTH_BUFFER);
    dw0 |= (cmd_len - 2);

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = view->cmd[0];
    dw[2] = 0;
    dw[3] = view->cmd[2];
    dw[4] = view->cmd[3];
    dw[5] = view->cmd[4];
    dw[6] = view->cmd[5];

    if (view->img) {
        cmd_reserve_reloc(cmd, 1);
        cmd_batch_reloc(cmd, pos + 2, view->img->obj.mem->bo,
                view->cmd[1], INTEL_RELOC_WRITE);
    }
}

static void gen6_3DSTATE_STENCIL_BUFFER(struct intel_cmd *cmd,
                                        const struct intel_ds_view *view)
{
    const uint8_t cmd_len = 3;
    uint32_t dw0, *dw;
    XGL_UINT pos;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = (cmd_gen(cmd) >= INTEL_GEN(7)) ?
        GEN7_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER) :
        GEN6_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER);
    dw0 |= (cmd_len - 2);

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = view->cmd[6];
    dw[2] = 0;

    if (view->img) {
        cmd_reserve_reloc(cmd, 1);
        cmd_batch_reloc(cmd, pos + 2, view->img->obj.mem->bo,
                view->cmd[7], INTEL_RELOC_WRITE);
    }
}

static void gen6_3DSTATE_HIER_DEPTH_BUFFER(struct intel_cmd *cmd,
                                           const struct intel_ds_view *view)
{
    const uint8_t cmd_len = 3;
    uint32_t dw0, *dw;
    XGL_UINT pos;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = (cmd_gen(cmd) >= INTEL_GEN(7)) ?
        GEN7_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER) :
        GEN6_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER);
    dw0 |= (cmd_len - 2);

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = view->cmd[8];
    dw[2] = 0;

    if (view->img) {
        cmd_reserve_reloc(cmd, 1);
        cmd_batch_reloc(cmd, pos + 2, view->img->obj.mem->bo,
                view->cmd[9], INTEL_RELOC_WRITE);
    }
}

static void gen6_3DSTATE_CLEAR_PARAMS(struct intel_cmd *cmd,
                                      uint32_t clear_val)
{
    const uint8_t cmd_len = 2;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CLEAR_PARAMS) |
                         GEN6_CLEAR_PARAMS_DW0_VALID |
                         (cmd_len - 2);
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 6);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = clear_val;
}

static void gen7_3DSTATE_CLEAR_PARAMS(struct intel_cmd *cmd,
                                      uint32_t clear_val)
{
    const uint8_t cmd_len = 3;
    const uint32_t dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_CLEAR_PARAMS) |
                         (cmd_len - 2);
    uint32_t *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = clear_val;
    dw[2] = 1;
}

static void gen6_3DSTATE_CC_STATE_POINTERS(struct intel_cmd *cmd,
                                           uint32_t blend_offset,
                                           uint32_t ds_offset,
                                           uint32_t cc_offset)
{
    const uint8_t cmd_len = 4;
    uint32_t dw0, *dw;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CC_STATE_POINTERS) |
          (cmd_len - 2);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = blend_offset | 1;
    dw[2] = ds_offset | 1;
    dw[3] = cc_offset | 1;
}

static void gen6_3DSTATE_VIEWPORT_STATE_POINTERS(struct intel_cmd *cmd,
                                                 uint32_t clip_offset,
                                                 uint32_t sf_offset,
                                                 uint32_t cc_offset)
{
    const uint8_t cmd_len = 4;
    uint32_t dw0, *dw;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_VIEWPORT_STATE_POINTERS) |
          GEN6_PTR_VP_DW0_CLIP_CHANGED |
          GEN6_PTR_VP_DW0_SF_CHANGED |
          GEN6_PTR_VP_DW0_CC_CHANGED |
          (cmd_len - 2);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = clip_offset;
    dw[2] = sf_offset;
    dw[3] = cc_offset;
}

static void gen6_3DSTATE_SCISSOR_STATE_POINTERS(struct intel_cmd *cmd,
                                                uint32_t scissor_offset)
{
    const uint8_t cmd_len = 2;
    uint32_t dw0, *dw;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SCISSOR_STATE_POINTERS) |
          (cmd_len - 2);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = scissor_offset;
}

static void gen6_3DSTATE_BINDING_TABLE_POINTERS(struct intel_cmd *cmd,
                                                uint32_t vs_offset,
                                                uint32_t gs_offset,
                                                uint32_t ps_offset)
{
    const uint8_t cmd_len = 4;
    uint32_t dw0, *dw;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_BINDING_TABLE_POINTERS) |
          GEN6_PTR_BINDING_TABLE_DW0_VS_CHANGED |
          GEN6_PTR_BINDING_TABLE_DW0_GS_CHANGED |
          GEN6_PTR_BINDING_TABLE_DW0_PS_CHANGED |
          (cmd_len - 2);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = vs_offset;
    dw[2] = gs_offset;
    dw[3] = ps_offset;
}

static void gen6_3DSTATE_SAMPLER_STATE_POINTERS(struct intel_cmd *cmd,
                                                uint32_t vs_offset,
                                                uint32_t gs_offset,
                                                uint32_t ps_offset)
{
    const uint8_t cmd_len = 4;
    uint32_t dw0, *dw;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLER_STATE_POINTERS) |
          GEN6_PTR_SAMPLER_DW0_VS_CHANGED |
          GEN6_PTR_SAMPLER_DW0_GS_CHANGED |
          GEN6_PTR_SAMPLER_DW0_PS_CHANGED |
          (cmd_len - 2);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = vs_offset;
    dw[2] = gs_offset;
    dw[3] = ps_offset;
}

static void gen7_3dstate_pointer(struct intel_cmd *cmd,
                                 int subop, uint32_t offset)
{
    const uint8_t cmd_len = 2;
    const uint32_t dw0 = GEN6_RENDER_TYPE_RENDER |
                         GEN6_RENDER_SUBTYPE_3D |
                         subop | (cmd_len - 2);
    uint32_t *dw;

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = offset;
}

static uint32_t gen6_BLEND_STATE(struct intel_cmd *cmd,
                                 const struct intel_blend_state *state)
{
    const uint8_t cmd_align = GEN6_ALIGNMENT_BLEND_STATE * 4;
    const uint8_t cmd_len = XGL_MAX_COLOR_ATTACHMENTS * 2;

    CMD_ASSERT(cmd, 6, 7.5);
    STATIC_ASSERT(ARRAY_SIZE(state->cmd) >= cmd_len);

    return cmd_state_write(cmd, INTEL_CMD_ITEM_BLEND,
            cmd_align, cmd_len, state->cmd);
}

static uint32_t gen6_DEPTH_STENCIL_STATE(struct intel_cmd *cmd,
                                         const struct intel_ds_state *state)
{
    const uint8_t cmd_align = GEN6_ALIGNMENT_DEPTH_STENCIL_STATE * 4;
    const uint8_t cmd_len = 3;

    CMD_ASSERT(cmd, 6, 7.5);
    STATIC_ASSERT(ARRAY_SIZE(state->cmd) >= cmd_len);

    return cmd_state_write(cmd, INTEL_CMD_ITEM_DEPTH_STENCIL,
            cmd_align, cmd_len, state->cmd);
}

static uint32_t gen6_COLOR_CALC_STATE(struct intel_cmd *cmd,
                                      uint32_t stencil_ref,
                                      const uint32_t blend_color[4])
{
    const uint8_t cmd_align = GEN6_ALIGNMENT_COLOR_CALC_STATE * 4;
    const uint8_t cmd_len = 6;
    uint32_t offset, *dw;

    CMD_ASSERT(cmd, 6, 7.5);

    offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_COLOR_CALC,
            cmd_align, cmd_len, &dw);
    dw[0] = stencil_ref;
    dw[1] = 0;
    dw[2] = blend_color[0];
    dw[3] = blend_color[1];
    dw[4] = blend_color[2];
    dw[5] = blend_color[3];

    return offset;
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
    uint32_t blend_offset, ds_offset, cc_offset;
    uint32_t stencil_ref;
    uint32_t blend_color[4];

    CMD_ASSERT(cmd, 6, 6);

    if (blend) {
        blend_offset = gen6_BLEND_STATE(cmd, blend);
        memcpy(blend_color, blend->cmd_blend_color, sizeof(blend_color));
    } else {
        blend_offset = 0;
        memset(blend_color, 0, sizeof(blend_color));
    }

    if (ds) {
        ds_offset = gen6_DEPTH_STENCIL_STATE(cmd, ds);
        stencil_ref = ds->cmd_stencil_ref;
    } else {
        ds_offset = 0;
        stencil_ref = 0;
    }

    cc_offset = gen6_COLOR_CALC_STATE(cmd, stencil_ref, blend_color);

    gen6_3DSTATE_CC_STATE_POINTERS(cmd, blend_offset, ds_offset, cc_offset);
}

static void gen6_viewport_states(struct intel_cmd *cmd)
{
    const struct intel_viewport_state *viewport = cmd->bind.state.viewport;
    uint32_t sf_offset, clip_offset, cc_offset, scissor_offset;

    if (!viewport)
        return;

    assert(viewport->cmd_len == (8 + 4 + 2 + 2 * viewport->scissor_enable) *
            viewport->viewport_count);

    sf_offset = cmd_state_write(cmd, INTEL_CMD_ITEM_SF_VIEWPORT,
            GEN6_ALIGNMENT_SF_VIEWPORT * 4, 8 * viewport->viewport_count,
            viewport->cmd);

    clip_offset = cmd_state_write(cmd, INTEL_CMD_ITEM_CLIP_VIEWPORT,
            GEN6_ALIGNMENT_CLIP_VIEWPORT * 4, 4 * viewport->viewport_count,
            &viewport->cmd[viewport->cmd_clip_pos]);

    cc_offset = cmd_state_write(cmd, INTEL_CMD_ITEM_CC_VIEWPORT,
            GEN6_ALIGNMENT_SF_VIEWPORT * 4, 2 * viewport->viewport_count,
            &viewport->cmd[viewport->cmd_cc_pos]);

    if (viewport->scissor_enable) {
        scissor_offset = cmd_state_write(cmd, INTEL_CMD_ITEM_SCISSOR_RECT,
                GEN6_ALIGNMENT_SCISSOR_RECT * 4, 2 * viewport->viewport_count,
                &viewport->cmd[viewport->cmd_scissor_rect_pos]);
    } else {
        scissor_offset = 0;
    }

    gen6_3DSTATE_VIEWPORT_STATE_POINTERS(cmd,
            clip_offset, sf_offset, cc_offset);

    gen6_3DSTATE_SCISSOR_STATE_POINTERS(cmd, scissor_offset);
}

static void gen7_cc_states(struct intel_cmd *cmd)
{
    const struct intel_blend_state *blend = cmd->bind.state.blend;
    const struct intel_ds_state *ds = cmd->bind.state.ds;
    uint32_t stencil_ref;
    uint32_t blend_color[4];
    uint32_t offset;

    CMD_ASSERT(cmd, 7, 7.5);

    if (!blend && !ds)
        return;

    if (blend) {
        offset = gen6_BLEND_STATE(cmd, blend);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BLEND_STATE_POINTERS, offset);

        memcpy(blend_color, blend->cmd_blend_color, sizeof(blend_color));
    } else {
        memset(blend_color, 0, sizeof(blend_color));
    }

    if (ds) {
        offset = gen6_DEPTH_STENCIL_STATE(cmd, ds);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_DEPTH_STENCIL_STATE_POINTERS,
                offset);
    } else {
        stencil_ref = 0;
    }

    offset = gen6_COLOR_CALC_STATE(cmd, stencil_ref, blend_color);
    gen7_3dstate_pointer(cmd,
            GEN6_RENDER_OPCODE_3DSTATE_CC_STATE_POINTERS, offset);
}

static void gen7_viewport_states(struct intel_cmd *cmd)
{
    const struct intel_viewport_state *viewport = cmd->bind.state.viewport;
    uint32_t offset;

    if (!viewport)
        return;

    assert(viewport->cmd_len == (16 + 2 + 2 * viewport->scissor_enable) *
            viewport->viewport_count);

    offset = cmd_state_write(cmd, INTEL_CMD_ITEM_SF_VIEWPORT,
            GEN7_ALIGNMENT_SF_CLIP_VIEWPORT * 4, 16 * viewport->viewport_count,
            viewport->cmd);
    gen7_3dstate_pointer(cmd,
            GEN7_RENDER_OPCODE_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP,
            offset);

    offset = cmd_state_write(cmd, INTEL_CMD_ITEM_CC_VIEWPORT,
            GEN6_ALIGNMENT_CC_VIEWPORT * 4, 2 * viewport->viewport_count,
            &viewport->cmd[viewport->cmd_cc_pos]);
    gen7_3dstate_pointer(cmd,
            GEN7_RENDER_OPCODE_3DSTATE_VIEWPORT_STATE_POINTERS_CC,
            offset);

    if (viewport->scissor_enable) {
        offset = cmd_state_write(cmd, INTEL_CMD_ITEM_SCISSOR_RECT,
                GEN6_ALIGNMENT_SCISSOR_RECT * 4, 2 * viewport->viewport_count,
                &viewport->cmd[viewport->cmd_scissor_rect_pos]);
        gen7_3dstate_pointer(cmd,
                GEN6_RENDER_OPCODE_3DSTATE_SCISSOR_STATE_POINTERS,
                offset);
    }
}

static void gen6_pcb(struct intel_cmd *cmd, int subop,
                     const struct intel_pipeline_shader *sh)
{
    const uint8_t cmd_len = 5;
    uint32_t *dw;

    cmd_batch_pointer(cmd, cmd_len, &dw);

    dw[0] = GEN6_RENDER_TYPE_RENDER |
            GEN6_RENDER_SUBTYPE_3D |
            subop | (cmd_len - 2);
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 0;
}

static void gen7_pcb(struct intel_cmd *cmd, int subop,
                     const struct intel_pipeline_shader *sh)
{
    const uint8_t cmd_len = 7;
    uint32_t *dw;

    cmd_batch_pointer(cmd, cmd_len, &dw);

    dw[0] = GEN6_RENDER_TYPE_RENDER |
            GEN6_RENDER_SUBTYPE_3D |
            subop | (cmd_len - 2);
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 0;
    dw[5] = 0;
    dw[6] = 0;
}

static uint32_t emit_samplers(struct intel_cmd *cmd,
                              const struct intel_pipeline_rmap *rmap)
{
    const XGL_UINT border_len = (cmd_gen(cmd) >= INTEL_GEN(7)) ? 4 : 12;
    const XGL_UINT border_stride =
        u_align(border_len, GEN6_ALIGNMENT_SAMPLER_BORDER_COLOR);
    uint32_t border_offset, *border_dw, sampler_offset, *sampler_dw;
    XGL_UINT surface_count;
    XGL_UINT i;

    CMD_ASSERT(cmd, 6, 7.5);

    if (!rmap || !rmap->sampler_count)
        return 0;

    surface_count = rmap->rt_count + rmap->resource_count + rmap->uav_count;

    border_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_BLOB,
            GEN6_ALIGNMENT_SAMPLER_BORDER_COLOR * 4,
            border_stride * rmap->sampler_count, &border_dw);

    sampler_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_SAMPLER,
            GEN6_ALIGNMENT_SAMPLER_STATE * 4,
            4 * rmap->sampler_count, &sampler_dw);

    for (i = 0; i < rmap->sampler_count; i++) {
        const struct intel_pipeline_rmap_slot *slot =
            &rmap->slots[surface_count + i];
        const struct intel_sampler *sampler;

        switch (slot->path_len) {
        case 0:
            sampler = NULL;
            break;
        case INTEL_PIPELINE_RMAP_SLOT_RT:
        case INTEL_PIPELINE_RMAP_SLOT_DYN:
            assert(!"unexpected rmap slot type");
            sampler = NULL;
            break;
        case 1:
            {
                const struct intel_dset *dset = cmd->bind.dset.graphics;
                const XGL_UINT slot_offset = cmd->bind.dset.graphics_offset;
                const struct intel_dset_slot *dset_slot =
                    &dset->slots[slot_offset + slot->u.index];

                switch (dset_slot->type) {
                case INTEL_DSET_SLOT_SAMPLER:
                    sampler = dset_slot->u.sampler;
                    break;
                default:
                    assert(!"unexpected dset slot type");
                    sampler = NULL;
                    break;
                }
            }
            break;
        default:
            assert(!"nested descriptor set unsupported");
            sampler = NULL;
            break;
        }

        if (sampler) {
            memcpy(border_dw, &sampler->cmd[3], border_len * 4);

            sampler_dw[0] = sampler->cmd[0];
            sampler_dw[1] = sampler->cmd[1];
            sampler_dw[2] = border_offset;
            sampler_dw[3] = sampler->cmd[2];
        } else {
            sampler_dw[0] = GEN6_SAMPLER_DW0_DISABLE;
            sampler_dw[1] = 0;
            sampler_dw[2] = 0;
            sampler_dw[3] = 0;
        }

        border_offset += border_stride * 4;
        border_dw += border_stride;
        sampler_dw += 4;
    }

    return sampler_offset;
}

static uint32_t emit_binding_table(struct intel_cmd *cmd,
                                   const struct intel_pipeline_rmap *rmap)
{
    uint32_t binding_table[256], offset;
    XGL_UINT surface_count, i;

    CMD_ASSERT(cmd, 6, 7.5);

    surface_count = (rmap) ?
        rmap->rt_count + rmap->resource_count + rmap->uav_count : 0;
    if (!surface_count)
        return 0;

    assert(surface_count <= ARRAY_SIZE(binding_table));

    for (i = 0; i < surface_count; i++) {
        const struct intel_pipeline_rmap_slot *slot = &rmap->slots[i];

        switch (slot->path_len) {
        case 0:
            offset = 0;
            break;
        case INTEL_PIPELINE_RMAP_SLOT_RT:
            {
                const struct intel_rt_view *view = cmd->bind.att.rt[i];

                offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                        GEN6_ALIGNMENT_SURFACE_STATE * 4,
                        view->cmd_len, view->cmd);

                cmd_reserve_reloc(cmd, 1);
                cmd_surface_reloc(cmd, offset, 1, view->img->obj.mem->bo,
                        view->cmd[1], INTEL_RELOC_WRITE);
            }
            break;
        case INTEL_PIPELINE_RMAP_SLOT_DYN:
            {
                const struct intel_mem_view *view =
                    &cmd->bind.dyn_view.graphics;

                offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                        GEN6_ALIGNMENT_SURFACE_STATE * 4,
                        view->cmd_len, view->cmd);

                cmd_reserve_reloc(cmd, 1);
                cmd_surface_reloc(cmd, offset, 1, view->mem->bo,
                        view->cmd[1], INTEL_RELOC_WRITE);
            }
            break;
        case 1:
            {
                const struct intel_dset *dset = cmd->bind.dset.graphics;
                const XGL_UINT slot_offset = cmd->bind.dset.graphics_offset;
                const struct intel_dset_slot *dset_slot =
                    &dset->slots[slot_offset + slot->u.index];

                switch (dset_slot->type) {
                case INTEL_DSET_SLOT_IMG_VIEW:
                    offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                            GEN6_ALIGNMENT_SURFACE_STATE * 4,
                            dset_slot->u.img_view->cmd_len,
                            dset_slot->u.img_view->cmd);

                    cmd_reserve_reloc(cmd, 1);
                    cmd_surface_reloc(cmd, offset, 1,
                            dset_slot->u.img_view->img->obj.mem->bo,
                            dset_slot->u.img_view->cmd[1], 0);
                    break;
                case INTEL_DSET_SLOT_MEM_VIEW:
                    offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                            GEN6_ALIGNMENT_SURFACE_STATE * 4,
                            dset_slot->u.mem_view.cmd_len,
                            dset_slot->u.mem_view.cmd);

                    cmd_reserve_reloc(cmd, 1);
                    cmd_surface_reloc(cmd, offset, 1,
                            dset_slot->u.mem_view.mem->bo,
                            dset_slot->u.mem_view.cmd[1], 0);
                    break;
                default:
                    assert(!"unexpected dset slot type");
                    break;
                }
            }
            break;
        default:
            assert(!"nested descriptor set unsupported");
            break;
        }

        binding_table[i] = offset;
    }

    return cmd_state_write(cmd, INTEL_CMD_ITEM_BINDING_TABLE,
            GEN6_ALIGNMENT_BINDING_TABLE_STATE * 4,
            surface_count, binding_table);
}

static void gen6_3DSTATE_VERTEX_BUFFERS(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_pipeline_rmap *rmap = pipeline->vs.rmap;
    const struct intel_dset *dset = cmd->bind.dset.graphics;
    const uint8_t cmd_len = 1 + 4 * pipeline->vb_count;
    uint32_t *dw;
    XGL_UINT pos, i;

    CMD_ASSERT(cmd, 6, 7.5);

    if (!pipeline->vb_count)
        return;

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);

    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_BUFFERS) | (cmd_len - 2);
    dw++;
    pos++;

    for (i = 0; i < pipeline->vb_count; i++) {
        const XGL_UINT vb_offset = rmap->rt_count + rmap->resource_count +
            rmap->uav_count + rmap->sampler_count;
        const struct intel_pipeline_rmap_slot *slot = (i < rmap->vb_count) ?
            &rmap->slots[vb_offset + i] : NULL;
        struct intel_mem_view *view = NULL;

        if (slot) {
            switch (slot->path_len) {
            case 1:
                view = (dset->slots[slot->u.index].type ==
                        INTEL_DSET_SLOT_MEM_VIEW) ?
                    &dset->slots[slot->u.index].u.mem_view : NULL;
                break;
            default:
                break;
            }
        }

        assert(pipeline->vb[i].strideInBytes <= 2048);

        dw[0] = i << GEN6_VB_STATE_DW0_INDEX__SHIFT |
                pipeline->vb[i].strideInBytes;

        if (cmd_gen(cmd) >= INTEL_GEN(7))
            dw[0] |= GEN7_VB_STATE_DW0_ADDR_MODIFIED;

        switch (pipeline->vb[i].stepRate) {
        case XGL_VERTEX_INPUT_STEP_RATE_VERTEX:
            dw[0] |= GEN6_VB_STATE_DW0_ACCESS_VERTEXDATA;
            dw[3] = 0;
            break;
        case XGL_VERTEX_INPUT_STEP_RATE_INSTANCE:
            dw[0] |= GEN6_VB_STATE_DW0_ACCESS_INSTANCEDATA;
            dw[3] = 1;
            break;
        case XGL_VERTEX_INPUT_STEP_RATE_DRAW:
            dw[0] |= GEN6_VB_STATE_DW0_ACCESS_INSTANCEDATA;
            dw[3] = 0;
            break;
        default:
            assert(!"unknown step rate");
            dw[0] |= GEN6_VB_STATE_DW0_ACCESS_VERTEXDATA;
            dw[3] = 0;
            break;
        }

        if (view) {
            const uint32_t begin = view->cmd[1];
            const uint32_t end = view->mem->size - 1;

            cmd_reserve_reloc(cmd, 2);
            cmd_batch_reloc(cmd, pos + 1, view->mem->bo, begin, 0);
            cmd_batch_reloc(cmd, pos + 2, view->mem->bo, end, 0);
        } else {
            dw[0] |= GEN6_VB_STATE_DW0_IS_NULL;
            dw[1] = 0;
            dw[2] = 0;
        }

        dw += 4;
        pos += 4;
    }
}

static void gen6_3DSTATE_VS(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_pipeline_shader *vs = &pipeline->vs;
    const uint8_t cmd_len = 6;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_VS) | (cmd_len - 2);
    uint32_t dw2, dw4, dw5, *dw;
    int vue_read_len, max_threads;

    CMD_ASSERT(cmd, 6, 7.5);

    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 135:
     *
     *     "(Vertex URB Entry Read Length) Specifies the number of pairs of
     *      128-bit vertex elements to be passed into the payload for each
     *      vertex."
     *
     *     "It is UNDEFINED to set this field to 0 indicating no Vertex URB
     *      data to be read and passed to the thread."
     */
    vue_read_len = (vs->in_count + 1) / 2;
    if (!vue_read_len)
        vue_read_len = 1;

    dw2 = (vs->sampler_count + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT |
          vs->surface_count << GEN6_THREADDISP_BINDING_TABLE_SIZE__SHIFT;

    dw4 = vs->urb_grf_start << GEN6_VS_DW4_URB_GRF_START__SHIFT |
          vue_read_len << GEN6_VS_DW4_URB_READ_LEN__SHIFT |
          0 << GEN6_VS_DW4_URB_READ_OFFSET__SHIFT;

    dw5 = GEN6_VS_DW5_STATISTICS |
          GEN6_VS_DW5_VS_ENABLE;

    switch (cmd_gen(cmd)) {
    case INTEL_GEN(7.5):
        max_threads = (cmd->dev->gpu->gt >= 2) ? 280 : 70;
        break;
    case INTEL_GEN(7):
        max_threads = (cmd->dev->gpu->gt == 2) ? 128 : 36;
        break;
    case INTEL_GEN(6):
        max_threads = (cmd->dev->gpu->gt == 2) ? 60 : 24;
        break;
    default:
        max_threads = 1;
        break;
    }

    if (cmd_gen(cmd) >= INTEL_GEN(7.5))
        dw5 |= (max_threads - 1) << GEN75_VS_DW5_MAX_THREADS__SHIFT;
    else
        dw5 |= (max_threads - 1) << GEN6_VS_DW5_MAX_THREADS__SHIFT;

    if (pipeline->disable_vs_cache)
        dw5 |= GEN6_VS_DW5_CACHE_DISABLE;

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = cmd->bind.pipeline.vs_offset;
    dw[2] = dw2;
    dw[3] = 0; /* scratch */
    dw[4] = dw4;
    dw[5] = dw5;
}

static void emit_shader_resources(struct intel_cmd *cmd)
{
    /* five HW shader stages */
    uint32_t binding_tables[5], samplers[5];

    binding_tables[0] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->vs.rmap);
    binding_tables[1] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->tcs.rmap);
    binding_tables[2] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->tes.rmap);
    binding_tables[3] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->gs.rmap);
    binding_tables[4] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->fs.rmap);

    samplers[0] = emit_samplers(cmd, cmd->bind.pipeline.graphics->vs.rmap);
    samplers[1] = emit_samplers(cmd, cmd->bind.pipeline.graphics->tcs.rmap);
    samplers[2] = emit_samplers(cmd, cmd->bind.pipeline.graphics->tes.rmap);
    samplers[3] = emit_samplers(cmd, cmd->bind.pipeline.graphics->gs.rmap);
    samplers[4] = emit_samplers(cmd, cmd->bind.pipeline.graphics->fs.rmap);

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_VS,
                binding_tables[0]);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_HS,
                binding_tables[1]);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_DS,
                binding_tables[2]);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_GS,
                binding_tables[3]);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_PS,
                binding_tables[4]);

        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_VS,
                samplers[0]);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_HS,
                samplers[1]);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_DS,
                samplers[2]);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_GS,
                samplers[3]);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_SAMPLER_STATE_POINTERS_PS,
                samplers[4]);
    } else {
        assert(!binding_tables[1] && !binding_tables[2]);
        gen6_3DSTATE_BINDING_TABLE_POINTERS(cmd,
                binding_tables[0], binding_tables[3], binding_tables[4]);

        assert(!samplers[1] && !samplers[2]);
        gen6_3DSTATE_SAMPLER_STATE_POINTERS(cmd,
                samplers[0], samplers[3], samplers[4]);
    }
}

static void emit_rt(struct intel_cmd *cmd)
{
    cmd_wa_gen6_pre_depth_stall_write(cmd);
    gen6_3DSTATE_DRAWING_RECTANGLE(cmd, cmd->bind.att.width,
            cmd->bind.att.height);
}

static void emit_ds(struct intel_cmd *cmd)
{
    const struct intel_ds_view *ds = cmd->bind.att.ds;

    if (!ds) {
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

static uint32_t emit_shader(struct intel_cmd *cmd,
                            const struct intel_pipeline_shader *shader)
{
    struct intel_cmd_shader_cache *cache = &cmd->bind.shader_cache;
    uint32_t offset;
    XGL_UINT i;

    /* see if the shader is already in the cache */
    for (i = 0; i < cache->used; i++) {
        if (cache->entries[i].shader == (const void *) shader)
            return cache->entries[i].kernel_offset;
    }

    offset = cmd_instruction_write(cmd, shader->codeSize, shader->pCode);

    /* grow the cache if full */
    if (cache->used >= cache->count) {
        const XGL_UINT count = cache->count + 16;
        void *entries;

        entries = icd_alloc(sizeof(cache->entries[0]) * count, 0,
                XGL_SYSTEM_ALLOC_INTERNAL);
        if (entries) {
            if (cache->entries) {
                memcpy(entries, cache->entries,
                        sizeof(cache->entries[0]) * cache->used);
                icd_free(cache->entries);
            }

            cache->entries = entries;
            cache->count = count;
        }
    }

    /* add the shader to the cache */
    if (cache->used < cache->count) {
        cache->entries[cache->used].shader = (const void *) shader;
        cache->entries[cache->used].kernel_offset = offset;
        cache->used++;
    }

    return offset;
}

static void emit_graphics_pipeline(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;

    if (pipeline->wa_flags & INTEL_CMD_WA_GEN6_PRE_DEPTH_STALL_WRITE)
        cmd_wa_gen6_pre_depth_stall_write(cmd);
    if (pipeline->wa_flags & INTEL_CMD_WA_GEN6_PRE_COMMAND_SCOREBOARD_STALL)
        cmd_wa_gen6_pre_command_scoreboard_stall(cmd);
    if (pipeline->wa_flags & INTEL_CMD_WA_GEN7_PRE_VS_DEPTH_STALL_WRITE)
        cmd_wa_gen7_pre_vs_depth_stall_write(cmd);

    /* 3DSTATE_URB_VS and etc. */
    assert(pipeline->cmd_len);
    cmd_batch_write(cmd, pipeline->cmd_len, pipeline->cmds);

    if (pipeline->active_shaders & SHADER_VERTEX_FLAG) {
        cmd->bind.pipeline.vs_offset = emit_shader(cmd, &pipeline->vs);
    }
    if (pipeline->active_shaders & SHADER_TESS_CONTROL_FLAG) {
        cmd->bind.pipeline.tcs_offset = emit_shader(cmd, &pipeline->tcs);
    }
    if (pipeline->active_shaders & SHADER_TESS_EVAL_FLAG) {
        cmd->bind.pipeline.tes_offset = emit_shader(cmd, &pipeline->tes);
    }
    if (pipeline->active_shaders & SHADER_GEOMETRY_FLAG) {
        cmd->bind.pipeline.gs_offset = emit_shader(cmd, &pipeline->gs);
    }
    if (pipeline->active_shaders & SHADER_FRAGMENT_FLAG) {
        cmd->bind.pipeline.fs_offset = emit_shader(cmd, &pipeline->fs);
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

static void emit_bounded_states(struct intel_cmd *cmd)
{
    const struct intel_msaa_state *msaa = cmd->bind.state.msaa;

    emit_graphics_pipeline(cmd);

    emit_rt(cmd);
    emit_ds(cmd);

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

    emit_shader_resources(cmd);

    cmd_wa_gen6_pre_depth_stall_write(cmd);
    cmd_wa_gen6_pre_multisample_depth_flush(cmd);

    /* 3DSTATE_MULTISAMPLE and 3DSTATE_SAMPLE_MASK */
    cmd_batch_write(cmd, msaa->cmd_len, msaa->cmd);

    gen6_3DSTATE_VERTEX_BUFFERS(cmd);
    gen6_3DSTATE_VS(cmd);
}

static void gen6_meta_dynamic_states(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    uint32_t blend_offset, ds_offset, cc_offset, cc_vp_offset, *dw;

    CMD_ASSERT(cmd, 6, 7.5);

    blend_offset = 0;
    ds_offset = 0;
    cc_offset = 0;
    cc_vp_offset = 0;

    if (meta->dst.valid) {
        /* BLEND_STATE */
        blend_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_BLEND,
                GEN6_ALIGNMENT_BLEND_STATE * 4, 2, &dw);
        dw[0] = 0;
        dw[1] = GEN6_BLEND_DW1_COLORCLAMP_RTFORMAT | 0x3;
    }

    if (meta->ds) {
        assert(!"depth/stencil clear unsupported");

        /* DEPTH_STENCIL_STATE */
        ds_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_DEPTH_STENCIL,
                GEN6_ALIGNMENT_DEPTH_STENCIL_STATE * 4, 3, &dw);
        dw[0] = 0;
        dw[1] = 0;
        dw[2] = 0;

        /* COLOR_CALC_STATE */
        cc_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_COLOR_CALC,
                GEN6_ALIGNMENT_COLOR_CALC_STATE * 4, 6, &dw);
        dw[0] = 0;
        dw[1] = 0;
        dw[2] = 0;
        dw[3] = 0;
        dw[4] = 0;
        dw[5] = 0;

        /* CC_VIEWPORT */
        cc_vp_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_CC_VIEWPORT,
                GEN6_ALIGNMENT_CC_VIEWPORT * 4, 2, &dw);
        dw[0] = 0;
        dw[1] = 0;
    }

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BLEND_STATE_POINTERS,
                blend_offset);
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_DEPTH_STENCIL_STATE_POINTERS,
                ds_offset);
        gen7_3dstate_pointer(cmd,
                GEN6_RENDER_OPCODE_3DSTATE_CC_STATE_POINTERS, cc_offset);

        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_VIEWPORT_STATE_POINTERS_CC,
                cc_vp_offset);
    } else {
        /* 3DSTATE_CC_STATE_POINTERS */
        cmd_batch_pointer(cmd, 4, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CC_STATE_POINTERS) | (4 - 2);
        dw[1] = blend_offset | GEN6_PTR_CC_DW1_BLEND_CHANGED;
        dw[2] = ds_offset | GEN6_PTR_CC_DW2_ZS_CHANGED;
        dw[3] = cc_offset | GEN6_PTR_CC_DW3_CC_CHANGED;

        /* 3DSTATE_VIEWPORT_STATE_POINTERS */
        cmd_batch_pointer(cmd, 4, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VIEWPORT_STATE_POINTERS) | (4 - 2) |
                GEN6_PTR_VP_DW0_CC_CHANGED;
        dw[1] = 0;
        dw[2] = 0;
        dw[3] = cc_vp_offset;
    }
}

static void gen6_meta_surface_states(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    uint32_t binding_table[2];
    XGL_UINT surface_count = 0;
    uint32_t offset;

    CMD_ASSERT(cmd, 6, 7.5);

    /* SURFACE_STATE */
    if (meta->dst.valid) {
        offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                GEN6_ALIGNMENT_SURFACE_STATE * 4,
                meta->dst.surface_len, meta->dst.surface);

        cmd_reserve_reloc(cmd, 1);
        cmd_surface_reloc(cmd, offset, 1,
                (struct intel_bo *) meta->dst.reloc_target,
                meta->dst.reloc_offset, meta->dst.reloc_flags);

        binding_table[surface_count++] = offset;
    }
    if (meta->src.valid) {
        offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                GEN6_ALIGNMENT_SURFACE_STATE * 4,
                meta->src.surface_len, meta->src.surface);

        cmd_reserve_reloc(cmd, 1);
        if (meta->src.reloc_flags & INTEL_CMD_RELOC_TARGET_IS_WRITER) {
            cmd_surface_reloc_writer(cmd, offset, 1,
                    meta->src.reloc_target, meta->src.reloc_offset);
        } else {
            cmd_surface_reloc(cmd, offset, 1,
                    (struct intel_bo *) meta->src.reloc_target,
                    meta->src.reloc_offset, meta->src.reloc_flags);
        }

        binding_table[surface_count++] = offset;
    }

    /* BINDING_TABLE */
    offset = cmd_state_write(cmd, INTEL_CMD_ITEM_BINDING_TABLE,
            GEN6_ALIGNMENT_BINDING_TABLE_STATE * 4,
            surface_count, binding_table);

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_PS,
                offset);
    } else {
        /* 3DSTATE_BINDING_TABLE_POINTERS */
        gen6_3DSTATE_BINDING_TABLE_POINTERS(cmd, 0, 0, offset);
    }
}

static void gen6_meta_urb(struct intel_cmd *cmd)
{
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 6);

    /* 3DSTATE_URB */
    cmd_batch_pointer(cmd, 3, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_URB) | (3 - 2);
    dw[1] = 128 << GEN6_URB_DW1_VS_ENTRY_COUNT__SHIFT;
    dw[2] = 0;
}

static void gen7_meta_urb(struct intel_cmd *cmd)
{
    uint32_t *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    /* 3DSTATE_PUSH_CONSTANT_ALLOC_x */
    cmd_batch_pointer(cmd, 10, &dw);

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_VS) | (2 - 2);
    dw[1] = 0;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_HS) | (2 - 2);
    dw[1] = 0;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_DS) | (2 - 2);
    dw[1] = 0;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_GS) | (2 - 2);
    dw[1] = 0;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_PS) | (2 - 2);
    dw[1] = 1;

    cmd_wa_gen7_pre_vs_depth_stall_write(cmd);

    /* 3DSTATE_URB_x */
    cmd_batch_pointer(cmd, 8, &dw);

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_VS) | (2 - 2);
    dw[1] = 1 << GEN7_URB_ANY_DW1_OFFSET__SHIFT |
            512;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_HS) | (2 - 2);
    dw[1] = 0;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_DS) | (2 - 2);
    dw[1] = 0;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_GS) | (2 - 2);
    dw[1] = 0;
    dw += 2;
}

static void gen6_meta_vf(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    XGL_UINT vertices[3][2];
    uint32_t offset, *dw;
    XGL_UINT pos;

    CMD_ASSERT(cmd, 6, 7.5);

    /* write vertices */
    vertices[0][0] = meta->dst.x + meta->width;
    vertices[0][1] = meta->dst.y + meta->height;
    vertices[1][0] = meta->dst.x;
    vertices[1][1] = meta->dst.y + meta->height;
    vertices[2][0] = meta->dst.x;
    vertices[2][1] = meta->dst.y;
    offset = cmd_state_write(cmd, INTEL_CMD_ITEM_BLOB, 32,
            sizeof(vertices) / 4, (const uint32_t *) vertices);

    /* 3DSTATE_VERTEX_BUFFERS */
    pos = cmd_batch_pointer(cmd, 5, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_BUFFERS) | (5 - 2);
    dw[1] = sizeof(vertices[0]);
    if (cmd_gen(cmd) >= INTEL_GEN(7))
        dw[1] |= GEN7_VB_STATE_DW0_ADDR_MODIFIED;

    cmd_reserve_reloc(cmd, 2);
    cmd_batch_reloc_writer(cmd, pos + 2, INTEL_CMD_WRITER_STATE, offset);
    cmd_batch_reloc_writer(cmd, pos + 3, INTEL_CMD_WRITER_STATE,
            offset + sizeof(vertices) - 1);

    dw[4] = 0;

    /* 3DSTATE_VERTEX_ELEMENTS */
    cmd_batch_pointer(cmd, 5, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_ELEMENTS) | (5 - 2);
    dw[1] = GEN6_VE_STATE_DW0_VALID,
    dw[2] = GEN6_VFCOMP_STORE_0 << GEN6_VE_STATE_DW1_COMP0__SHIFT | /* Reserved */
            GEN6_VFCOMP_STORE_0 << GEN6_VE_STATE_DW1_COMP1__SHIFT | /* Render Target Array Index */
            GEN6_VFCOMP_STORE_0 << GEN6_VE_STATE_DW1_COMP2__SHIFT | /* Viewport Index */
            GEN6_VFCOMP_STORE_0 << GEN6_VE_STATE_DW1_COMP3__SHIFT;  /* Point Width */
    dw[3] = GEN6_VE_STATE_DW0_VALID |
            GEN6_FORMAT_R32G32_USCALED << GEN6_VE_STATE_DW0_FORMAT__SHIFT;
    dw[4] = GEN6_VFCOMP_STORE_SRC  << GEN6_VE_STATE_DW1_COMP0__SHIFT |
            GEN6_VFCOMP_STORE_SRC  << GEN6_VE_STATE_DW1_COMP1__SHIFT |
            GEN6_VFCOMP_STORE_0    << GEN6_VE_STATE_DW1_COMP2__SHIFT |
            GEN6_VFCOMP_STORE_1_FP << GEN6_VE_STATE_DW1_COMP3__SHIFT;
}

static void gen6_meta_disabled(struct intel_cmd *cmd)
{
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 6);

    /* 3DSTATE_CONSTANT_VS */
    cmd_batch_pointer(cmd, 5, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_VS) | (5 - 2);
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 0;

    /* 3DSTATE_VS */
    cmd_batch_pointer(cmd, 6, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VS) | (6 - 2);
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 0;
    dw[5] = 0;

    /* 3DSTATE_CONSTANT_GS */
    cmd_batch_pointer(cmd, 5, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_GS) | (5 - 2);
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 0;

    /* 3DSTATE_GS */
    cmd_batch_pointer(cmd, 7, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_GS) | (7 - 2);
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 1 << GEN6_GS_DW4_URB_READ_LEN__SHIFT;
    dw[5] = GEN6_GS_DW5_STATISTICS;
    dw[6] = 0;

    /* 3DSTATE_CLIP */
    cmd_batch_pointer(cmd, 4, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CLIP) | (4 - 2);
    dw[1] = 0;
    dw[2] = 0;
    dw[3] = 0;

    /* 3DSTATE_SF */
    cmd_batch_pointer(cmd, 20, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SF) | (20 - 2);
    dw[1] = 1 << GEN7_SBE_DW1_URB_READ_LEN__SHIFT;
    memset(&dw[2], 0, 18 * sizeof(*dw));
}

static void gen7_meta_disabled(struct intel_cmd *cmd)
{
    uint32_t *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    /* 3DSTATE_CONSTANT_VS */
    cmd_batch_pointer(cmd, 7, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_VS) | (7 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (7 - 1));

    /* 3DSTATE_VS */
    cmd_batch_pointer(cmd, 6, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VS) | (6 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (6 - 1));

    /* 3DSTATE_CONSTANT_HS */
    cmd_batch_pointer(cmd, 7, &dw);
    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_CONSTANT_HS) | (7 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (7 - 1));

    /* 3DSTATE_HS */
    cmd_batch_pointer(cmd, 7, &dw);
    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_HS) | (7 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (7 - 1));

    /* 3DSTATE_TE */
    cmd_batch_pointer(cmd, 4, &dw);
    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_TE) | (4 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (4 - 1));

    /* 3DSTATE_CONSTANT_DS */
    cmd_batch_pointer(cmd, 7, &dw);
    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_CONSTANT_DS) | (7 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (7 - 1));

    /* 3DSTATE_DS */
    cmd_batch_pointer(cmd, 6, &dw);
    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_DS) | (6 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (6 - 1));

    /* 3DSTATE_CONSTANT_GS */
    cmd_batch_pointer(cmd, 7, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_GS) | (7 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (7 - 1));

    /* 3DSTATE_GS */
    cmd_batch_pointer(cmd, 7, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_GS) | (7 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (7 - 1));

    /* 3DSTATE_STREAMOUT */
    cmd_batch_pointer(cmd, 3, &dw);
    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_STREAMOUT) | (3 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (3 - 1));

    /* 3DSTATE_CLIP */
    cmd_batch_pointer(cmd, 4, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CLIP) | (4 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (4 - 1));

    /* 3DSTATE_SF */
    cmd_batch_pointer(cmd, 7, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SF) | (7 - 2);
    memset(&dw[1], 0, sizeof(*dw) * (7 - 1));

    /* 3DSTATE_SBE */
    cmd_batch_pointer(cmd, 14, &dw);
    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_SBE) | (14 - 2);
    dw[1] = 1 << GEN7_SBE_DW1_URB_READ_LEN__SHIFT;
    memset(&dw[2], 0, sizeof(*dw) * (14 - 2));
}

static void gen6_meta_wm(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 7.5);

    cmd_wa_gen6_pre_multisample_depth_flush(cmd);

    /* 3DSTATE_MULTISAMPLE */
    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        cmd_batch_pointer(cmd, 4, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_MULTISAMPLE) | (4 - 2);
        dw[1] = (meta->samples <= 1) ? GEN6_MULTISAMPLE_DW1_NUMSAMPLES_1 :
                (meta->samples <= 4) ? GEN6_MULTISAMPLE_DW1_NUMSAMPLES_4 :
                                       GEN7_MULTISAMPLE_DW1_NUMSAMPLES_8;
        dw[2] = 0;
        dw[3] = 0;
    } else {
        cmd_batch_pointer(cmd, 3, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_MULTISAMPLE) | (3 - 2);
        dw[1] = (meta->samples <= 1) ? GEN6_MULTISAMPLE_DW1_NUMSAMPLES_1 :
                                       GEN6_MULTISAMPLE_DW1_NUMSAMPLES_4;
        dw[2] = 0;
    }

    /* 3DSTATE_SAMPLE_MASK */
    cmd_batch_pointer(cmd, 2, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLE_MASK) | (2 - 2);
    dw[1] = (1 << meta->samples) - 1;

    /* 3DSTATE_DRAWING_RECTANGLE */
    cmd_batch_pointer(cmd, 4, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_DRAWING_RECTANGLE) | (4 - 2);
    dw[1] = meta->dst.y << 16 | meta->dst.x;
    dw[2] = (meta->dst.y + meta->height - 1) << 16 |
            (meta->dst.x + meta->width - 1);
    dw[3] = 0;
}

static uint32_t gen6_meta_ps_constants(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    XGL_UINT offset_x, offset_y;
    /* one GPR */
    XGL_UINT consts[8];
    XGL_UINT const_count;

    CMD_ASSERT(cmd, 6, 7.5);

    /* underflow is fine here */
    offset_x = meta->src.x - meta->dst.x;
    offset_y = meta->src.y - meta->dst.y;

    switch (meta->shader_id) {
    case INTEL_DEV_META_FS_COPY_MEM:
    case INTEL_DEV_META_FS_COPY_1D:
    case INTEL_DEV_META_FS_COPY_1D_ARRAY:
    case INTEL_DEV_META_FS_COPY_2D:
    case INTEL_DEV_META_FS_COPY_2D_ARRAY:
    case INTEL_DEV_META_FS_COPY_2D_MS:
        consts[0] = offset_x;
        consts[1] = offset_y;
        consts[2] = meta->src.layer;
        consts[3] = meta->src.lod;
        const_count = 4;
        break;
    case INTEL_DEV_META_FS_COPY_1D_TO_MEM:
    case INTEL_DEV_META_FS_COPY_1D_ARRAY_TO_MEM:
    case INTEL_DEV_META_FS_COPY_2D_TO_MEM:
    case INTEL_DEV_META_FS_COPY_2D_ARRAY_TO_MEM:
    case INTEL_DEV_META_FS_COPY_2D_MS_TO_MEM:
        consts[0] = offset_x;
        consts[1] = offset_y;
        consts[2] = meta->src.layer;
        consts[3] = meta->src.lod;
        consts[4] = meta->src.x;
        consts[5] = meta->width;
        const_count = 6;
        break;
    case INTEL_DEV_META_FS_COPY_MEM_TO_IMG:
        consts[0] = offset_x;
        consts[1] = offset_y;
        consts[2] = meta->width;
        const_count = 3;
        break;
    case INTEL_DEV_META_FS_CLEAR_COLOR:
        consts[0] = meta->clear_val[0];
        consts[1] = meta->clear_val[1];
        consts[2] = meta->clear_val[2];
        consts[3] = meta->clear_val[3];
        const_count = 4;
        break;
    case INTEL_DEV_META_FS_CLEAR_DEPTH:
        consts[0] = meta->clear_val[0];
        const_count = 1;
        break;
    case INTEL_DEV_META_FS_RESOLVE_2X:
    case INTEL_DEV_META_FS_RESOLVE_4X:
    case INTEL_DEV_META_FS_RESOLVE_8X:
    case INTEL_DEV_META_FS_RESOLVE_16X:
        consts[0] = offset_x;
        consts[1] = offset_y;
        const_count = 2;
        break;
    default:
        assert(!"unknown meta shader id");
        const_count = 0;
        break;
    }

    /* this can be skipped but it makes state dumping prettier */
    memset(&consts[const_count], 0, sizeof(consts[0]) * (8 - const_count));

    return cmd_state_write(cmd, INTEL_CMD_ITEM_BLOB, 32, 8, consts);
}

static void gen6_meta_ps(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    const struct intel_pipeline_shader *sh =
        intel_dev_get_meta_shader(cmd->dev, meta->shader_id);
    uint32_t offset, *dw;

    CMD_ASSERT(cmd, 6, 6);

    /* 3DSTATE_CONSTANT_PS */
    offset = gen6_meta_ps_constants(cmd);
    cmd_batch_pointer(cmd, 5, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_PS) | (5 - 2) |
            GEN6_PCB_ANY_DW0_PCB0_VALID;
    dw[1] = offset;
    dw[2] = 0;
    dw[3] = 0;
    dw[4] = 0;

    /* 3DSTATE_WM */
    offset = emit_shader(cmd, sh);
    cmd_batch_pointer(cmd, 9, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_WM) | (9 - 2);
    dw[1] = offset;
    dw[2] = (sh->sampler_count + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT |
             sh->surface_count << GEN6_THREADDISP_BINDING_TABLE_SIZE__SHIFT;
    dw[3] = 0;
    dw[4] = sh->urb_grf_start << GEN6_WM_DW4_URB_GRF_START0__SHIFT;
    dw[5] = (40 - 1) << GEN6_WM_DW5_MAX_THREADS__SHIFT |
            GEN6_WM_DW5_PS_ENABLE |
            GEN6_WM_DW5_8_PIXEL_DISPATCH;
    dw[6] = sh->in_count << GEN6_WM_DW6_SF_ATTR_COUNT__SHIFT |
            GEN6_WM_DW6_POSOFFSET_NONE |
            GEN6_WM_DW6_ZW_INTERP_PIXEL |
            sh->barycentric_interps << GEN6_WM_DW6_BARYCENTRIC_INTERP__SHIFT |
            GEN6_WM_DW6_POINT_RASTRULE_UPPER_RIGHT;
    if (meta->samples > 1) {
        dw[6] |= GEN6_WM_DW6_MSRASTMODE_ON_PATTERN |
                 GEN6_WM_DW6_MSDISPMODE_PERPIXEL;
    } else {
        dw[6] |= GEN6_WM_DW6_MSRASTMODE_OFF_PIXEL |
                 GEN6_WM_DW6_MSDISPMODE_PERSAMPLE;
    }
    dw[7] = 0;
    dw[8] = 0;
}

static void gen7_meta_ps(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    const struct intel_pipeline_shader *sh =
        intel_dev_get_meta_shader(cmd->dev, meta->shader_id);
    uint32_t offset, *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    /* 3DSTATE_WM */
    cmd_batch_pointer(cmd, 3, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_WM) | (3 - 2);
    dw[1] = GEN7_WM_DW1_PS_ENABLE |
            GEN7_WM_DW1_ZW_INTERP_PIXEL |
            sh->barycentric_interps << GEN7_WM_DW1_BARYCENTRIC_INTERP__SHIFT |
            GEN7_WM_DW1_POINT_RASTRULE_UPPER_RIGHT;
    dw[2] = 0;

    /* 3DSTATE_CONSTANT_PS */
    offset = gen6_meta_ps_constants(cmd);
    cmd_batch_pointer(cmd, 7, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_PS) | (7 - 2);
    dw[1] = 1 << GEN7_PCB_ANY_DW1_PCB0_SIZE__SHIFT;
    dw[2] = 0;
    dw[3] = offset;
    dw[4] = 0;
    dw[5] = 0;
    dw[6] = 0;

    /* 3DSTATE_PS */
    offset = emit_shader(cmd, sh);
    cmd_batch_pointer(cmd, 8, &dw);
    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PS) | (8 - 2);
    dw[1] = offset;
    dw[2] = (sh->sampler_count + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT |
             sh->surface_count << GEN6_THREADDISP_BINDING_TABLE_SIZE__SHIFT;
    dw[3] = 0;

    dw[4] = GEN7_PS_DW4_PUSH_CONSTANT_ENABLE |
            GEN7_PS_DW4_POSOFFSET_NONE |
            GEN7_PS_DW4_8_PIXEL_DISPATCH |
            (48 - 1) << GEN7_PS_DW4_MAX_THREADS__SHIFT;
    if (cmd_gen(cmd) >= INTEL_GEN(7.5))
        dw[4] |= ((1 << meta->samples) - 1) << GEN75_PS_DW4_SAMPLE_MASK__SHIFT;

    dw[5] = sh->urb_grf_start << GEN7_PS_DW5_URB_GRF_START0__SHIFT;
    dw[6] = 0;
    dw[7] = 0;
}

static void gen6_meta_depth_buffer(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    const struct intel_ds_view *ds = meta->ds;

    CMD_ASSERT(cmd, 6, 7.5);

    if (!ds) {
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

static void cmd_bind_graphics_pipeline(struct intel_cmd *cmd,
                                       const struct intel_pipeline *pipeline)
{
    cmd->bind.pipeline.graphics = pipeline;
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
    cmd->bind.index.mem = mem;
    cmd->bind.index.offset = offset;
    cmd->bind.index.type = type;
}

static void cmd_bind_attachments(struct intel_cmd *cmd,
                                 XGL_UINT rt_count,
                                 const XGL_COLOR_ATTACHMENT_BIND_INFO *rt_info,
                                 const XGL_DEPTH_STENCIL_BIND_INFO *ds_info)
{
    XGL_UINT width = 0, height = 0;
    XGL_UINT i;

    for (i = 0; i < rt_count; i++) {
        const XGL_COLOR_ATTACHMENT_BIND_INFO *att = &rt_info[i];
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

    cmd->bind.att.rt_count = rt_count;

    if (ds_info) {
        const struct intel_layout *layout;

        cmd->bind.att.ds = intel_ds_view(ds_info->view);
        layout = &cmd->bind.att.ds->img->layout;

        if (width > layout->width0)
            width = layout->width0;
        if (height > layout->height0)
            height = layout->height0;
    } else {
        cmd->bind.att.ds = NULL;
    }

    cmd->bind.att.width = width;
    cmd->bind.att.height = height;
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
            gen6_3DSTATE_INDEX_BUFFER(cmd, cmd->bind.index.mem,
                    cmd->bind.index.offset, cmd->bind.index.type,
                    false);
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

void cmd_draw_meta(struct intel_cmd *cmd, const struct intel_cmd_meta *meta)
{
    cmd->bind.meta = meta;

    cmd_wa_gen6_pre_depth_stall_write(cmd);

    gen6_meta_dynamic_states(cmd);
    gen6_meta_surface_states(cmd);

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        gen7_meta_urb(cmd);
        gen6_meta_vf(cmd);
        gen7_meta_disabled(cmd);
        gen6_meta_wm(cmd);
        gen7_meta_ps(cmd);
        gen6_meta_depth_buffer(cmd);

        cmd_wa_gen7_post_command_cs_stall(cmd);
        cmd_wa_gen7_post_command_depth_stall(cmd);

        gen7_3DPRIMITIVE(cmd, GEN6_3DPRIM_RECTLIST, false, 3, 0, 1, 0, 0);
    } else {
        gen6_meta_urb(cmd);
        gen6_meta_vf(cmd);
        gen6_meta_disabled(cmd);
        gen6_meta_wm(cmd);
        gen6_meta_ps(cmd);
        gen6_meta_depth_buffer(cmd);

        gen6_3DPRIMITIVE(cmd, GEN6_3DPRIM_RECTLIST, false, 3, 0, 1, 0, 0);
    }

    cmd->bind.draw_count++;
    /* need to re-emit all workarounds */
    cmd->bind.wa_flags = 0;

    cmd->bind.meta = NULL;
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

    cmd_bind_attachments(cmd, colorAttachmentCount, pColorAttachments,
            pDepthStencilAttachment);
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
