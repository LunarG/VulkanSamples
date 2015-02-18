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
#include "buf.h"
#include "desc.h"
#include "img.h"
#include "mem.h"
#include "pipeline.h"
#include "sampler.h"
#include "shader.h"
#include "state.h"
#include "view.h"
#include "cmd_priv.h"
#include "fb.h"

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
   uint32_t pos;

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
                                      const struct intel_buf *buf,
                                      XGL_GPU_SIZE offset,
                                      XGL_INDEX_TYPE type,
                                      bool enable_cut_index)
{
    const uint8_t cmd_len = 3;
    uint32_t dw0, end_offset, *dw;
    unsigned offset_align;
    uint32_t pos;

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
        cmd_fail(cmd, XGL_ERROR_INVALID_VALUE);
        return;
        break;
    }

    if (offset % offset_align) {
        cmd_fail(cmd, XGL_ERROR_INVALID_VALUE);
        return;
    }

    /* aligned and inclusive */
    end_offset = buf->size - (buf->size % offset_align) - 1;

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;

    cmd_reserve_reloc(cmd, 2);
    cmd_batch_reloc(cmd, pos + 1, buf->obj.mem->bo, offset, 0);
    cmd_batch_reloc(cmd, pos + 2, buf->obj.mem->bo, end_offset, 0);
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
                                           uint32_t width, uint32_t height)
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
    const struct intel_dynamic_rs *raster = cmd->bind.state.raster;
    uint32_t dw1, dw2, dw3;
    int point_width;

    CMD_ASSERT(cmd, 6, 7.5);

    dw1 = GEN7_SF_DW1_STATISTICS |
          GEN7_SF_DW1_DEPTH_OFFSET_SOLID |
          GEN7_SF_DW1_DEPTH_OFFSET_WIREFRAME |
          GEN7_SF_DW1_DEPTH_OFFSET_POINT |
          GEN7_SF_DW1_VIEWPORT_ENABLE |
          pipeline->cmd_sf_fill;

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        int format;

        switch (pipeline->db_format) {
        case XGL_FMT_D16_UNORM:
            format = GEN6_ZFORMAT_D16_UNORM;
            break;
        case XGL_FMT_D32_SFLOAT:
        case XGL_FMT_D32_SFLOAT_S8_UINT:
            format = GEN6_ZFORMAT_D32_FLOAT;
            break;
        default:
            assert(!cmd->bind.render_pass->fb->ds); // Must have valid format if ds attached
            format = 0;
            break;
        }

        dw1 |= format << GEN7_SF_DW1_DEPTH_FORMAT__SHIFT;
    }

    dw2 = pipeline->cmd_sf_cull;

    /* Scissor is always enabled */
    dw2 |= GEN7_SF_DW2_SCISSOR_ENABLE;

    if (pipeline->sample_count > 1) {
          dw2 |= 128 << GEN7_SF_DW2_LINE_WIDTH__SHIFT |
                 GEN7_SF_DW2_MSRASTMODE_ON_PATTERN;
    } else {
          dw2 |= 0 << GEN7_SF_DW2_LINE_WIDTH__SHIFT |
                 GEN7_SF_DW2_MSRASTMODE_OFF_PIXEL;
    }

    /* in U8.3 */
    point_width = (int) (raster->rs_info.pointSize * 8.0f + 0.5f);
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
    body[3] = u_fui((float) raster->rs_info.depthBias * 2.0f);
    body[4] = u_fui(raster->rs_info.slopeScaledDepthBias);
    body[5] = u_fui(raster->rs_info.depthBiasClamp);
}

static void gen6_3DSTATE_SF(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 20;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_SF) |
                         (cmd_len - 2);
    const uint32_t *sbe = cmd->bind.pipeline.graphics->cmd_3dstate_sbe;
    uint32_t sf[6];
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 6);

    gen7_fill_3DSTATE_SF_body(cmd, sf);

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = sbe[1];
    memcpy(&dw[2], sf, sizeof(sf));
    memcpy(&dw[8], &sbe[2], 12);
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

static void gen6_3DSTATE_CLIP(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 4;
    const uint32_t dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_CLIP) |
                         (cmd_len - 2);
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_pipeline_shader *vs = &pipeline->vs;
    const struct intel_pipeline_shader *fs = &pipeline->fs;
    const struct intel_dynamic_vp *viewport = cmd->bind.state.viewport;
    uint32_t dw1, dw2, dw3, *dw;

    CMD_ASSERT(cmd, 6, 7.5);

    dw1 = GEN6_CLIP_DW1_STATISTICS;
    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        dw1 |= GEN7_CLIP_DW1_SUBPIXEL_8BITS |
               GEN7_CLIP_DW1_EARLY_CULL_ENABLE |
               pipeline->cmd_clip_cull;
    }

    dw2 = GEN6_CLIP_DW2_CLIP_ENABLE |
          GEN6_CLIP_DW2_XY_TEST_ENABLE |
          GEN6_CLIP_DW2_APIMODE_OGL |
          (vs->enable_user_clip ? 1 : 0) << GEN6_CLIP_DW2_UCP_CLIP_ENABLES__SHIFT |
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

    /* TODO: framebuffer requests layer_count > 1 */
    if (cmd->bind.render_pass->fb->array_size == 1) {
        dw3 |= GEN6_CLIP_DW3_RTAINDEX_FORCED_ZERO;
    }

    cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = dw1;
    dw[2] = dw2;
    dw[3] = dw3;
}

static void gen6_add_scratch_space(struct intel_cmd *cmd,
                                   uint32_t batch_pos,
                                   const struct intel_pipeline *pipeline,
                                   const struct intel_pipeline_shader *sh)
{
    int scratch_space;

    CMD_ASSERT(cmd, 6, 7.5);

    assert(sh->per_thread_scratch_size &&
           sh->per_thread_scratch_size % 1024 == 0 &&
           u_is_pow2(sh->per_thread_scratch_size) &&
           sh->scratch_offset % 1024 == 0);
    scratch_space = u_ffs(sh->per_thread_scratch_size) - 11;

    cmd_reserve_reloc(cmd, 1);
    cmd_batch_reloc(cmd, batch_pos, pipeline->obj.mem->bo,
            sh->scratch_offset | scratch_space, INTEL_RELOC_WRITE);
}

static void gen6_3DSTATE_WM(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_pipeline_shader *fs = &pipeline->fs;
    const uint8_t cmd_len = 9;
    uint32_t pos;
    uint32_t dw0, dw2, dw4, dw5, dw6, *dw;

    CMD_ASSERT(cmd, 6, 6);

    dw0 = GEN6_RENDER_CMD(3D, 3DSTATE_WM) | (cmd_len - 2);

    dw2 = (fs->sampler_count + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT |
          fs->surface_count << GEN6_THREADDISP_BINDING_TABLE_SIZE__SHIFT;

    dw4 = GEN6_WM_DW4_STATISTICS |
          fs->urb_grf_start << GEN6_WM_DW4_URB_GRF_START0__SHIFT |
          0 << GEN6_WM_DW4_URB_GRF_START1__SHIFT |
          0 << GEN6_WM_DW4_URB_GRF_START2__SHIFT;

    dw5 = (fs->max_threads - 1) << GEN6_WM_DW5_MAX_THREADS__SHIFT |
          GEN6_WM_DW5_PS_ENABLE |
          GEN6_WM_DW5_8_PIXEL_DISPATCH;

    if (fs->uses & INTEL_SHADER_USE_KILL ||
        pipeline->cb_state.alphaToCoverageEnable)
        dw5 |= GEN6_WM_DW5_PS_KILL;

    if (fs->computed_depth_mode)
        dw5 |= GEN6_WM_DW5_PS_COMPUTE_DEPTH;
    if (fs->uses & INTEL_SHADER_USE_DEPTH)
        dw5 |= GEN6_WM_DW5_PS_USE_DEPTH;
    if (fs->uses & INTEL_SHADER_USE_W)
        dw5 |= GEN6_WM_DW5_PS_USE_W;

    if (pipeline->dual_source_blend_enable)
        dw5 |= GEN6_WM_DW5_DUAL_SOURCE_BLEND;

    dw6 = fs->in_count << GEN6_WM_DW6_SF_ATTR_COUNT__SHIFT |
          GEN6_WM_DW6_POSOFFSET_NONE |
          GEN6_WM_DW6_ZW_INTERP_PIXEL |
          fs->barycentric_interps << GEN6_WM_DW6_BARYCENTRIC_INTERP__SHIFT |
          GEN6_WM_DW6_POINT_RASTRULE_UPPER_RIGHT;

    if (pipeline->sample_count > 1) {
        dw6 |= GEN6_WM_DW6_MSRASTMODE_ON_PATTERN |
               GEN6_WM_DW6_MSDISPMODE_PERPIXEL;
    } else {
        dw6 |= GEN6_WM_DW6_MSRASTMODE_OFF_PIXEL |
               GEN6_WM_DW6_MSDISPMODE_PERSAMPLE;
    }

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = cmd->bind.pipeline.fs_offset;
    dw[2] = dw2;
    dw[3] = 0; /* scratch */
    dw[4] = dw4;
    dw[5] = dw5;
    dw[6] = dw6;
    dw[7] = 0; /* kernel 1 */
    dw[8] = 0; /* kernel 2 */

    if (fs->per_thread_scratch_size)
        gen6_add_scratch_space(cmd, pos + 3, pipeline, fs);
}

static void gen7_3DSTATE_WM(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const struct intel_pipeline_shader *fs = &pipeline->fs;
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

    dw1 |= fs->computed_depth_mode << GEN7_WM_DW1_PSCDEPTH__SHIFT;

    if (fs->uses & INTEL_SHADER_USE_DEPTH)
        dw1 |= GEN7_WM_DW1_PS_USE_DEPTH;
    if (fs->uses & INTEL_SHADER_USE_W)
        dw1 |= GEN7_WM_DW1_PS_USE_W;

    dw2 = 0;

    if (pipeline->sample_count > 1) {
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
    const uint8_t cmd_len = 8;
    uint32_t dw0, dw2, dw4, dw5, *dw;
    uint32_t pos;

    CMD_ASSERT(cmd, 7, 7.5);

    dw0 = GEN7_RENDER_CMD(3D, 3DSTATE_PS) | (cmd_len - 2);

    dw2 = (fs->sampler_count + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT |
          fs->surface_count << GEN6_THREADDISP_BINDING_TABLE_SIZE__SHIFT;

    dw4 = GEN7_PS_DW4_POSOFFSET_NONE |
          GEN7_PS_DW4_8_PIXEL_DISPATCH;

    if (cmd_gen(cmd) >= INTEL_GEN(7.5)) {
        dw4 |= (fs->max_threads - 1) << GEN75_PS_DW4_MAX_THREADS__SHIFT;
        dw4 |= pipeline->cmd_sample_mask << GEN75_PS_DW4_SAMPLE_MASK__SHIFT;
    } else {
        dw4 |= (fs->max_threads - 1) << GEN7_PS_DW4_MAX_THREADS__SHIFT;
    }

    if (fs->in_count)
        dw4 |= GEN7_PS_DW4_ATTR_ENABLE;

    if (pipeline->dual_source_blend_enable)
        dw4 |= GEN7_PS_DW4_DUAL_SOURCE_BLEND;

    dw5 = fs->urb_grf_start << GEN7_PS_DW5_URB_GRF_START0__SHIFT |
          0 << GEN7_PS_DW5_URB_GRF_START1__SHIFT |
          0 << GEN7_PS_DW5_URB_GRF_START2__SHIFT;

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = cmd->bind.pipeline.fs_offset;
    dw[2] = dw2;
    dw[3] = 0; /* scratch */
    dw[4] = dw4;
    dw[5] = dw5;
    dw[6] = 0; /* kernel 1 */
    dw[7] = 0; /* kernel 2 */

    if (fs->per_thread_scratch_size)
        gen6_add_scratch_space(cmd, pos + 3, pipeline, fs);
}

static void gen6_3DSTATE_DEPTH_BUFFER(struct intel_cmd *cmd,
                                      const struct intel_ds_view *view)
{
    const uint8_t cmd_len = 7;
    uint32_t dw0, *dw;
    uint32_t pos;

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
    uint32_t pos;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = (cmd_gen(cmd) >= INTEL_GEN(7)) ?
        GEN7_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER) :
        GEN6_RENDER_CMD(3D, 3DSTATE_STENCIL_BUFFER);
    dw0 |= (cmd_len - 2);

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;

    if (view->has_stencil) {
        dw[1] = view->cmd[6];

        cmd_reserve_reloc(cmd, 1);
        cmd_batch_reloc(cmd, pos + 2, view->img->obj.mem->bo,
                view->cmd[7], INTEL_RELOC_WRITE);
    } else {
        dw[1] = 0;
        dw[2] = 0;
    }
}

static void gen6_3DSTATE_HIER_DEPTH_BUFFER(struct intel_cmd *cmd,
                                           const struct intel_ds_view *view)
{
    const uint8_t cmd_len = 3;
    uint32_t dw0, *dw;
    uint32_t pos;

    CMD_ASSERT(cmd, 6, 7.5);

    dw0 = (cmd_gen(cmd) >= INTEL_GEN(7)) ?
        GEN7_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER) :
        GEN6_RENDER_CMD(3D, 3DSTATE_HIER_DEPTH_BUFFER);
    dw0 |= (cmd_len - 2);

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;

    if (view->has_hiz) {
        dw[1] = view->cmd[8];

        cmd_reserve_reloc(cmd, 1);
        cmd_batch_reloc(cmd, pos + 2, view->img->obj.mem->bo,
                view->cmd[9], INTEL_RELOC_WRITE);
    } else {
        dw[1] = 0;
        dw[2] = 0;
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

static uint32_t gen6_BLEND_STATE(struct intel_cmd *cmd)
{
    const uint8_t cmd_align = GEN6_ALIGNMENT_BLEND_STATE;
    const uint8_t cmd_len = INTEL_MAX_RENDER_TARGETS * 2;
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;

    CMD_ASSERT(cmd, 6, 7.5);
    STATIC_ASSERT(ARRAY_SIZE(pipeline->cmd_cb) >= INTEL_MAX_RENDER_TARGETS);

    return cmd_state_write(cmd, INTEL_CMD_ITEM_BLEND, cmd_align, cmd_len, pipeline->cmd_cb);
}

static uint32_t gen6_DEPTH_STENCIL_STATE(struct intel_cmd *cmd,
                                         const struct intel_dynamic_ds *state)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const uint8_t cmd_align = GEN6_ALIGNMENT_DEPTH_STENCIL_STATE;
    const uint8_t cmd_len = 3;
    uint32_t dw[3];

    dw[0] = pipeline->cmd_depth_stencil;
    /* same read and write masks for both front and back faces */
    dw[1] = (state->ds_info.stencilReadMask & 0xff) << 24 |
            (state->ds_info.stencilWriteMask & 0xff) << 16 |
            (state->ds_info.stencilReadMask & 0xff) << 8 |
            (state->ds_info.stencilWriteMask & 0xff);
    dw[2] = pipeline->cmd_depth_test;

    CMD_ASSERT(cmd, 6, 7.5);

    if (state->ds_info.stencilWriteMask && pipeline->stencilTestEnable)
       dw[0] |= 1 << 18;

    return cmd_state_write(cmd, INTEL_CMD_ITEM_DEPTH_STENCIL,
            cmd_align, cmd_len, dw);
}

static uint32_t gen6_COLOR_CALC_STATE(struct intel_cmd *cmd,
                                      uint32_t stencil_ref,
                                      const uint32_t blend_color[4])
{
    const uint8_t cmd_align = GEN6_ALIGNMENT_COLOR_CALC_STATE;
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

void cmd_batch_state_base_address(struct intel_cmd *cmd)
{
    const uint8_t cmd_len = 10;
    const uint32_t dw0 = GEN6_RENDER_CMD(COMMON, STATE_BASE_ADDRESS) |
                         (cmd_len - 2);
    uint32_t pos;
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 7.5);

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);

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

    cmd_reserve_reloc(cmd, 3);
    cmd_batch_reloc_writer(cmd, pos + 2, INTEL_CMD_WRITER_SURFACE,
            cmd->writers[INTEL_CMD_WRITER_SURFACE].sba_offset + 1);
    cmd_batch_reloc_writer(cmd, pos + 3, INTEL_CMD_WRITER_STATE,
            cmd->writers[INTEL_CMD_WRITER_STATE].sba_offset + 1);
    cmd_batch_reloc_writer(cmd, pos + 5, INTEL_CMD_WRITER_INSTRUCTION,
            cmd->writers[INTEL_CMD_WRITER_INSTRUCTION].sba_offset + 1);
}

void cmd_batch_flush(struct intel_cmd *cmd, uint32_t pipe_control_dw0)
{
    if (pipe_control_dw0 == 0)
        return;

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

void cmd_batch_flush_all(struct intel_cmd *cmd)
{
    cmd_batch_flush(cmd, GEN6_PIPE_CONTROL_INSTRUCTION_CACHE_INVALIDATE |
                         GEN6_PIPE_CONTROL_RENDER_CACHE_FLUSH |
                         GEN6_PIPE_CONTROL_DEPTH_CACHE_FLUSH |
                         GEN6_PIPE_CONTROL_VF_CACHE_INVALIDATE |
                         GEN6_PIPE_CONTROL_TEXTURE_CACHE_INVALIDATE |
                         GEN6_PIPE_CONTROL_CS_STALL);
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
                         uint32_t pipe_control_flags,
                         struct intel_bo *bo,
                         XGL_GPU_SIZE offset,
                         uint64_t val)
{
    /* need any WA or stall? */
    gen6_PIPE_CONTROL(cmd,
            GEN6_PIPE_CONTROL_WRITE_IMM | pipe_control_flags,
            bo, offset, val);
}

static void gen6_cc_states(struct intel_cmd *cmd)
{
    const struct intel_dynamic_cb *blend = cmd->bind.state.blend;
    const struct intel_dynamic_ds *ds = cmd->bind.state.ds;
    uint32_t blend_offset, ds_offset, cc_offset;
    uint32_t stencil_ref;
    uint32_t blend_color[4];

    CMD_ASSERT(cmd, 6, 6);

    blend_offset = gen6_BLEND_STATE(cmd);

    if (blend)
        memcpy(blend_color, blend->cb_info.blendConst, sizeof(blend_color));
    else
        memset(blend_color, 0, sizeof(blend_color));

    if (ds) {
        ds_offset = gen6_DEPTH_STENCIL_STATE(cmd, ds);
        stencil_ref = (ds->ds_info.stencilFrontRef & 0xff) << 24 |
                      (ds->ds_info.stencilBackRef & 0xff) << 16;
    } else {
        ds_offset = 0;
        stencil_ref = 0;
    }

    cc_offset = gen6_COLOR_CALC_STATE(cmd, stencil_ref, blend_color);

    gen6_3DSTATE_CC_STATE_POINTERS(cmd, blend_offset, ds_offset, cc_offset);
}

static void gen6_viewport_states(struct intel_cmd *cmd)
{
    const struct intel_dynamic_vp *viewport = cmd->bind.state.viewport;
    uint32_t sf_offset, clip_offset, cc_offset, scissor_offset;

    if (!viewport)
        return;

    assert(viewport->cmd_len == (8 + 4 + 2) *
            /* viewports */ viewport->viewport_count + (/* scissor */ viewport->viewport_count * 2));

    sf_offset = cmd_state_write(cmd, INTEL_CMD_ITEM_SF_VIEWPORT,
            GEN6_ALIGNMENT_SF_VIEWPORT, 8 * viewport->viewport_count,
            viewport->cmd);

    clip_offset = cmd_state_write(cmd, INTEL_CMD_ITEM_CLIP_VIEWPORT,
            GEN6_ALIGNMENT_CLIP_VIEWPORT, 4 * viewport->viewport_count,
            &viewport->cmd[viewport->cmd_clip_pos]);

    cc_offset = cmd_state_write(cmd, INTEL_CMD_ITEM_CC_VIEWPORT,
            GEN6_ALIGNMENT_SF_VIEWPORT, 2 * viewport->viewport_count,
            &viewport->cmd[viewport->cmd_cc_pos]);

    scissor_offset = cmd_state_write(cmd, INTEL_CMD_ITEM_SCISSOR_RECT,
            GEN6_ALIGNMENT_SCISSOR_RECT, 2 * viewport->viewport_count,
            &viewport->cmd[viewport->cmd_scissor_rect_pos]);

    gen6_3DSTATE_VIEWPORT_STATE_POINTERS(cmd,
            clip_offset, sf_offset, cc_offset);

    gen6_3DSTATE_SCISSOR_STATE_POINTERS(cmd, scissor_offset);
}

static void gen7_cc_states(struct intel_cmd *cmd)
{
    const struct intel_dynamic_cb *blend = cmd->bind.state.blend;
    const struct intel_dynamic_ds *ds = cmd->bind.state.ds;
    uint32_t stencil_ref;
    uint32_t blend_color[4];
    uint32_t offset;

    CMD_ASSERT(cmd, 7, 7.5);

    if (!blend && !ds)
        return;

    offset = gen6_BLEND_STATE(cmd);
    gen7_3dstate_pointer(cmd,
            GEN7_RENDER_OPCODE_3DSTATE_BLEND_STATE_POINTERS, offset);

    if (blend)
        memcpy(blend_color, blend->cb_info.blendConst, sizeof(blend_color));
    else
        memset(blend_color, 0, sizeof(blend_color));

    if (ds) {
        offset = gen6_DEPTH_STENCIL_STATE(cmd, ds);
        stencil_ref = (ds->ds_info.stencilFrontRef & 0xff) << 24 |
                      (ds->ds_info.stencilBackRef & 0xff) << 16;
        gen7_3dstate_pointer(cmd,
                GEN7_RENDER_OPCODE_3DSTATE_DEPTH_STENCIL_STATE_POINTERS,
                offset);
        stencil_ref = (ds->ds_info.stencilFrontRef & 0xff) << 24 |
                      (ds->ds_info.stencilBackRef & 0xff) << 16;
    } else {
        stencil_ref = 0;
    }

    offset = gen6_COLOR_CALC_STATE(cmd, stencil_ref, blend_color);
    gen7_3dstate_pointer(cmd,
            GEN6_RENDER_OPCODE_3DSTATE_CC_STATE_POINTERS, offset);
}

static void gen7_viewport_states(struct intel_cmd *cmd)
{
    const struct intel_dynamic_vp *viewport = cmd->bind.state.viewport;
    uint32_t offset;

    if (!viewport)
        return;

    assert(viewport->cmd_len == (16 + 2 + 2) * viewport->viewport_count);

    offset = cmd_state_write(cmd, INTEL_CMD_ITEM_SF_VIEWPORT,
            GEN7_ALIGNMENT_SF_CLIP_VIEWPORT, 16 * viewport->viewport_count,
            viewport->cmd);
    gen7_3dstate_pointer(cmd,
            GEN7_RENDER_OPCODE_3DSTATE_VIEWPORT_STATE_POINTERS_SF_CLIP,
            offset);

    offset = cmd_state_write(cmd, INTEL_CMD_ITEM_CC_VIEWPORT,
            GEN6_ALIGNMENT_CC_VIEWPORT, 2 * viewport->viewport_count,
            &viewport->cmd[viewport->cmd_cc_pos]);
    gen7_3dstate_pointer(cmd,
            GEN7_RENDER_OPCODE_3DSTATE_VIEWPORT_STATE_POINTERS_CC,
            offset);

    offset = cmd_state_write(cmd, INTEL_CMD_ITEM_SCISSOR_RECT,
                             GEN6_ALIGNMENT_SCISSOR_RECT, 2 * viewport->viewport_count,
                             &viewport->cmd[viewport->cmd_scissor_rect_pos]);
    gen7_3dstate_pointer(cmd,
                         GEN6_RENDER_OPCODE_3DSTATE_SCISSOR_STATE_POINTERS,
                         offset);
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
    const uint32_t border_len = (cmd_gen(cmd) >= INTEL_GEN(7)) ? 4 : 12;
    const uint32_t border_stride =
        u_align(border_len, GEN6_ALIGNMENT_SAMPLER_BORDER_COLOR / 4);
    const struct intel_desc_set *set = cmd->bind.dset.graphics;
    uint32_t border_offset, *border_dw, sampler_offset, *sampler_dw;
    uint32_t surface_count;
    uint32_t i;

    CMD_ASSERT(cmd, 6, 7.5);

    if (!rmap || !rmap->sampler_count)
        return 0;

    surface_count = rmap->rt_count + rmap->texture_resource_count + rmap->resource_count + rmap->uav_count;

    /*
     * note that we cannot call cmd_state_pointer() here as the following
     * cmd_state_pointer() would invalidate the pointer
     */
    border_offset = cmd_state_reserve(cmd, INTEL_CMD_ITEM_BLOB,
            GEN6_ALIGNMENT_SAMPLER_BORDER_COLOR,
            border_stride * rmap->sampler_count);

    sampler_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_SAMPLER,
            GEN6_ALIGNMENT_SAMPLER_STATE,
            4 * rmap->sampler_count, &sampler_dw);

    cmd_state_update(cmd, border_offset,
            border_stride * rmap->sampler_count, &border_dw);

    for (i = 0; i < rmap->sampler_count; i++) {
        const struct intel_pipeline_rmap_slot *slot =
            &rmap->slots[surface_count + i];
        const struct intel_sampler *sampler;

        switch (slot->type) {
        case INTEL_PIPELINE_RMAP_SAMPLER:
            intel_desc_set_read_sampler(set, &slot->u.sampler, &sampler);
            break;
        case INTEL_PIPELINE_RMAP_UNUSED:
            sampler = NULL;
            break;
        default:
            assert(!"unexpected rmap type");
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
                                   const struct intel_pipeline_rmap *rmap,
                                   const XGL_PIPELINE_SHADER_STAGE stage)
{
    const uint32_t sba_offset =
        cmd->writers[INTEL_CMD_WRITER_SURFACE].sba_offset;
    const struct intel_desc_set *set = cmd->bind.dset.graphics;
    uint32_t binding_table[256], offset;
    uint32_t surface_count, i;

    CMD_ASSERT(cmd, 6, 7.5);

    surface_count = (rmap) ?
        rmap->rt_count + rmap->texture_resource_count + rmap->resource_count + rmap->uav_count : 0;
    if (!surface_count)
        return 0;

    assert(surface_count <= ARRAY_SIZE(binding_table));

    for (i = 0; i < surface_count; i++) {
        const struct intel_pipeline_rmap_slot *slot = &rmap->slots[i];
        struct intel_null_view null_view;
        bool need_null_view = false;

        switch (slot->type) {
        case INTEL_PIPELINE_RMAP_RT:
            {
                const struct intel_rt_view *view =
                    (slot->u.rt < cmd->bind.render_pass->fb->rt_count) ?
                    cmd->bind.render_pass->fb->rt[slot->u.rt] : NULL;

                if (view) {
                    offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                            GEN6_ALIGNMENT_SURFACE_STATE,
                            view->cmd_len, view->cmd);

                    cmd_reserve_reloc(cmd, 1);
                    cmd_surface_reloc(cmd, offset, 1, view->img->obj.mem->bo,
                            view->cmd[1], INTEL_RELOC_WRITE);
                } else {
                    need_null_view = true;
                }
            }
            break;
        case INTEL_PIPELINE_RMAP_SURFACE:
            {
                const int32_t dyn_idx = slot->u.surface.dynamic_offset_index;
                const struct intel_mem *mem;
                bool read_only;
                const uint32_t *cmd_data;
                uint32_t cmd_len;

                assert(dyn_idx < 0 ||
                       dyn_idx < set->layout->dynamic_desc_count);

                intel_desc_set_read_surface(set, &slot->u.surface.offset,
                        stage, &mem, &read_only, &cmd_data, &cmd_len);
                if (mem) {
                    const uint32_t dynamic_offset = (dyn_idx >= 0) ?
                        cmd->bind.dset.graphics_dynamic_offsets[dyn_idx] : 0;
                    const uint32_t reloc_flags =
                        (read_only) ? 0 : INTEL_RELOC_WRITE;

                    offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                            GEN6_ALIGNMENT_SURFACE_STATE,
                            cmd_len, cmd_data);

                    cmd_reserve_reloc(cmd, 1);
                    cmd_surface_reloc(cmd, offset, 1, mem->bo,
                            cmd_data[1] + dynamic_offset, reloc_flags);
                } else {
                    need_null_view = true;
                }
            }
            break;
        case INTEL_PIPELINE_RMAP_UNUSED:
            need_null_view = true;
            break;
        default:
            assert(!"unexpected rmap type");
            need_null_view = true;
            break;
        }

        if (need_null_view) {
            intel_null_view_init(&null_view, cmd->dev);
            offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                    GEN6_ALIGNMENT_SURFACE_STATE,
                    null_view.cmd_len, null_view.cmd);
        }

        binding_table[i] = offset - sba_offset;
    }

    offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_BINDING_TABLE,
            GEN6_ALIGNMENT_BINDING_TABLE_STATE,
            surface_count, binding_table) - sba_offset;

    /* there is a 64KB limit on BINIDNG_TABLE_STATEs */
    assert(offset + sizeof(uint32_t) * surface_count <= 64 * 1024);

    return offset;
}

static void gen6_3DSTATE_VERTEX_BUFFERS(struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    const uint8_t cmd_len = 1 + 4 * pipeline->vb_count;
    uint32_t *dw;
    uint32_t pos, i;

    CMD_ASSERT(cmd, 6, 7.5);

    if (!pipeline->vb_count)
        return;

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);

    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_BUFFERS) | (cmd_len - 2);
    dw++;
    pos++;

    for (i = 0; i < pipeline->vb_count; i++) {
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

        if (cmd->bind.vertex.buf[i]) {
            const struct intel_buf *buf = cmd->bind.vertex.buf[i];
            const XGL_GPU_SIZE offset = cmd->bind.vertex.offset[i];

            cmd_reserve_reloc(cmd, 2);
            cmd_batch_reloc(cmd, pos + 1, buf->obj.mem->bo, offset, 0);
            cmd_batch_reloc(cmd, pos + 2, buf->obj.mem->bo, buf->size - 1, 0);
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
    uint32_t pos;
    int vue_read_len;

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

    if (cmd_gen(cmd) >= INTEL_GEN(7.5))
        dw5 |= (vs->max_threads - 1) << GEN75_VS_DW5_MAX_THREADS__SHIFT;
    else
        dw5 |= (vs->max_threads - 1) << GEN6_VS_DW5_MAX_THREADS__SHIFT;

    if (pipeline->disable_vs_cache)
        dw5 |= GEN6_VS_DW5_CACHE_DISABLE;

    pos = cmd_batch_pointer(cmd, cmd_len, &dw);
    dw[0] = dw0;
    dw[1] = cmd->bind.pipeline.vs_offset;
    dw[2] = dw2;
    dw[3] = 0; /* scratch */
    dw[4] = dw4;
    dw[5] = dw5;

    if (vs->per_thread_scratch_size)
        gen6_add_scratch_space(cmd, pos + 3, pipeline, vs);
}

static void emit_shader_resources(struct intel_cmd *cmd)
{
    /* five HW shader stages */
    uint32_t binding_tables[5], samplers[5];

    binding_tables[0] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->vs.rmap,
            XGL_SHADER_STAGE_VERTEX);
    binding_tables[1] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->tcs.rmap,
            XGL_SHADER_STAGE_TESS_CONTROL);
    binding_tables[2] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->tes.rmap,
            XGL_SHADER_STAGE_TESS_EVALUATION);
    binding_tables[3] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->gs.rmap,
            XGL_SHADER_STAGE_GEOMETRY);
    binding_tables[4] = emit_binding_table(cmd,
            cmd->bind.pipeline.graphics->fs.rmap,
            XGL_SHADER_STAGE_FRAGMENT);

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
    gen6_3DSTATE_DRAWING_RECTANGLE(cmd, cmd->bind.render_pass->fb->width,
            cmd->bind.render_pass->fb->height);
}

static void emit_ds(struct intel_cmd *cmd)
{
    const struct intel_ds_view *ds = cmd->bind.render_pass->fb->ds;

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
    uint32_t i;

    /* see if the shader is already in the cache */
    for (i = 0; i < cache->used; i++) {
        if (cache->entries[i].shader == (const void *) shader)
            return cache->entries[i].kernel_offset;
    }

    offset = cmd_instruction_write(cmd, shader->codeSize, shader->pCode);

    /* grow the cache if full */
    if (cache->used >= cache->count) {
        const uint32_t count = cache->count + 16;
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

    gen6_3DSTATE_VERTEX_BUFFERS(cmd);
    gen6_3DSTATE_VS(cmd);
}

static uint32_t gen6_meta_DEPTH_STENCIL_STATE(struct intel_cmd *cmd,
                                         const struct intel_cmd_meta *meta)
{
    const uint8_t cmd_align = GEN6_ALIGNMENT_DEPTH_STENCIL_STATE;
    const uint8_t cmd_len = 3;
    uint32_t dw[3];
    uint32_t cmd_depth_stencil;
    uint32_t cmd_depth_test;

    CMD_ASSERT(cmd, 6, 7.5);

    cmd_depth_stencil = 0;
    cmd_depth_test = 0;
    if (meta->ds.aspect == XGL_IMAGE_ASPECT_DEPTH) {
        cmd_depth_test |= GEN6_ZS_DW2_DEPTH_WRITE_ENABLE |
                          GEN6_COMPAREFUNCTION_ALWAYS << 27;
    }
    else if (meta->ds.aspect == XGL_IMAGE_ASPECT_STENCIL) {
        cmd_depth_stencil = 1 << 31 |
                (GEN6_COMPAREFUNCTION_ALWAYS) << 28 |
                (GEN6_STENCILOP_KEEP) << 25 |
                (GEN6_STENCILOP_KEEP) << 22 |
                (GEN6_STENCILOP_REPLACE) << 19 |
                1 << 15 |
                (GEN6_COMPAREFUNCTION_ALWAYS) << 12 |
                (GEN6_STENCILOP_KEEP) << 9 |
                (GEN6_STENCILOP_KEEP) << 6 |
                (GEN6_STENCILOP_REPLACE) << 3;
    }

    cmd_depth_test |= GEN6_COMPAREFUNCTION_ALWAYS << 27;
    dw[0] = cmd_depth_stencil | 1 << 18;
    dw[1] = (0xff) << 24 | (0xff) << 16;
    dw[2] = cmd_depth_test;

    return cmd_state_write(cmd, INTEL_CMD_ITEM_DEPTH_STENCIL,
            cmd_align, cmd_len, dw);
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

    if (meta->mode == INTEL_CMD_META_FS_RECT) {
        /* BLEND_STATE */
        blend_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_BLEND,
                GEN6_ALIGNMENT_BLEND_STATE, 2, &dw);
        dw[0] = 0;
        dw[1] = GEN6_BLEND_DW1_COLORCLAMP_RTFORMAT | 0x3;
    }

    if (meta->mode != INTEL_CMD_META_VS_POINTS) {
        if (meta->ds.aspect != XGL_IMAGE_ASPECT_COLOR) {
            const uint32_t blend_color[4] = { 0, 0, 0, 0 };
            uint32_t stencil_ref = (meta->ds.stencil_ref & 0xff) << 24 |
                                   (meta->ds.stencil_ref & 0xff) << 16;

            /* DEPTH_STENCIL_STATE */
            ds_offset = gen6_meta_DEPTH_STENCIL_STATE(cmd, meta);

            /* COLOR_CALC_STATE */
            cc_offset = gen6_COLOR_CALC_STATE(cmd,
                    stencil_ref, blend_color);

            /* CC_VIEWPORT */
            cc_vp_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_CC_VIEWPORT,
                    GEN6_ALIGNMENT_CC_VIEWPORT, 2, &dw);
            dw[0] = u_fui(0.0f);
            dw[1] = u_fui(1.0f);
        } else {
            /* DEPTH_STENCIL_STATE */
            ds_offset = cmd_state_pointer(cmd, INTEL_CMD_ITEM_DEPTH_STENCIL,
                    GEN6_ALIGNMENT_DEPTH_STENCIL_STATE,
                    GEN6_DEPTH_STENCIL_STATE__SIZE, &dw);
            memset(dw, 0, sizeof(*dw) * GEN6_DEPTH_STENCIL_STATE__SIZE);
        }
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
        gen6_3DSTATE_CC_STATE_POINTERS(cmd, blend_offset, ds_offset, cc_offset);

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
    uint32_t binding_table[2] = { 0, 0 };
    uint32_t offset;
    const uint32_t sba_offset =
        cmd->writers[INTEL_CMD_WRITER_SURFACE].sba_offset;

    CMD_ASSERT(cmd, 6, 7.5);

    if (meta->mode == INTEL_CMD_META_DEPTH_STENCIL_RECT)
        return;

    /* SURFACE_STATEs */
    if (meta->src.valid) {
        offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                GEN6_ALIGNMENT_SURFACE_STATE,
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

        binding_table[0] = offset - sba_offset;
    }
    if (meta->dst.valid) {
        offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_SURFACE,
                GEN6_ALIGNMENT_SURFACE_STATE,
                meta->dst.surface_len, meta->dst.surface);

        cmd_reserve_reloc(cmd, 1);
        cmd_surface_reloc(cmd, offset, 1,
                (struct intel_bo *) meta->dst.reloc_target,
                meta->dst.reloc_offset, meta->dst.reloc_flags);

        binding_table[1] = offset - sba_offset;
    }

    /* BINDING_TABLE */
    offset = cmd_surface_write(cmd, INTEL_CMD_ITEM_BINDING_TABLE,
            GEN6_ALIGNMENT_BINDING_TABLE_STATE,
            2, binding_table);

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        const int subop = (meta->mode == INTEL_CMD_META_VS_POINTS) ?
            GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_VS :
            GEN7_RENDER_OPCODE_3DSTATE_BINDING_TABLE_POINTERS_PS;
        gen7_3dstate_pointer(cmd, subop, offset - sba_offset);
    } else {
        /* 3DSTATE_BINDING_TABLE_POINTERS */
        if (meta->mode == INTEL_CMD_META_VS_POINTS)
            gen6_3DSTATE_BINDING_TABLE_POINTERS(cmd, offset - sba_offset, 0, 0);
        else
            gen6_3DSTATE_BINDING_TABLE_POINTERS(cmd, 0, 0, offset - sba_offset);
    }
}

static void gen6_meta_urb(struct intel_cmd *cmd)
{
    const int vs_entry_count = (cmd->dev->gpu->gt == 2) ? 256 : 128;
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 6);

    /* 3DSTATE_URB */
    cmd_batch_pointer(cmd, 3, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_URB) | (3 - 2);
    dw[1] = vs_entry_count << GEN6_URB_DW1_VS_ENTRY_COUNT__SHIFT;
    dw[2] = 0;
}

static void gen7_meta_urb(struct intel_cmd *cmd)
{
    const int pcb_alloc = (cmd->dev->gpu->gt == 3) ? 16 : 8;
    const int urb_offset = pcb_alloc / 8;
    int vs_entry_count;
    uint32_t *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    /* 3DSTATE_PUSH_CONSTANT_ALLOC_x */
    cmd_batch_pointer(cmd, 10, &dw);

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_VS) | (2 - 2);
    dw[1] = pcb_alloc << GEN7_PCB_ALLOC_ANY_DW1_SIZE__SHIFT;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_PS) | (2 - 2);
    dw[1] = pcb_alloc << GEN7_PCB_ALLOC_ANY_DW1_OFFSET__SHIFT |
            pcb_alloc << GEN7_PCB_ALLOC_ANY_DW1_SIZE__SHIFT;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_HS) | (2 - 2);
    dw[1] = 0;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_DS) | (2 - 2);
    dw[1] = 0;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PUSH_CONSTANT_ALLOC_GS) | (2 - 2);
    dw[1] = 0;

    cmd_wa_gen7_post_command_cs_stall(cmd);

    cmd_wa_gen7_pre_vs_depth_stall_write(cmd);

    switch (cmd_gen(cmd)) {
    case INTEL_GEN(7.5):
        vs_entry_count = (cmd->dev->gpu->gt >= 2) ? 1664 : 640;
        break;
    case INTEL_GEN(7):
    default:
        vs_entry_count = (cmd->dev->gpu->gt == 2) ? 704 : 512;
        break;
    }

    /* 3DSTATE_URB_x */
    cmd_batch_pointer(cmd, 8, &dw);

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_VS) | (2 - 2);
    dw[1] = urb_offset << GEN7_URB_ANY_DW1_OFFSET__SHIFT |
            vs_entry_count;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_HS) | (2 - 2);
    dw[1] = urb_offset << GEN7_URB_ANY_DW1_OFFSET__SHIFT;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_DS) | (2 - 2);
    dw[1] = urb_offset << GEN7_URB_ANY_DW1_OFFSET__SHIFT;
    dw += 2;

    dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_URB_GS) | (2 - 2);
    dw[1] = urb_offset << GEN7_URB_ANY_DW1_OFFSET__SHIFT;
    dw += 2;
}

static void gen6_meta_vf(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    uint32_t vb_start, vb_end, vb_stride;
    int ve_format, ve_z_source;
    uint32_t *dw;
    uint32_t pos;

    CMD_ASSERT(cmd, 6, 7.5);

    switch (meta->mode) {
    case INTEL_CMD_META_VS_POINTS:
        cmd_batch_pointer(cmd, 3, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_ELEMENTS) | (3 - 2);
        dw[1] = GEN6_VE_STATE_DW0_VALID;
        dw[2] = GEN6_VFCOMP_STORE_VID << GEN6_VE_STATE_DW1_COMP0__SHIFT |
                GEN6_VFCOMP_NOSTORE << GEN6_VE_STATE_DW1_COMP1__SHIFT |
                GEN6_VFCOMP_NOSTORE << GEN6_VE_STATE_DW1_COMP2__SHIFT |
                GEN6_VFCOMP_NOSTORE << GEN6_VE_STATE_DW1_COMP3__SHIFT;
        return;
        break;
    case INTEL_CMD_META_FS_RECT:
        {
            uint32_t vertices[3][2];

            vertices[0][0] = meta->dst.x + meta->width;
            vertices[0][1] = meta->dst.y + meta->height;
            vertices[1][0] = meta->dst.x;
            vertices[1][1] = meta->dst.y + meta->height;
            vertices[2][0] = meta->dst.x;
            vertices[2][1] = meta->dst.y;

            vb_start = cmd_state_write(cmd, INTEL_CMD_ITEM_BLOB, 32,
                    sizeof(vertices) / 4, (const uint32_t *) vertices);

            vb_end = vb_start + sizeof(vertices) - 1;
            vb_stride = sizeof(vertices[0]);
            ve_z_source = GEN6_VFCOMP_STORE_0;
            ve_format = GEN6_FORMAT_R32G32_USCALED;
        }
        break;
    case INTEL_CMD_META_DEPTH_STENCIL_RECT:
        {
            float vertices[3][3];

            vertices[0][0] = (float) (meta->dst.x + meta->width);
            vertices[0][1] = (float) (meta->dst.y + meta->height);
            vertices[0][2] = u_uif(meta->clear_val[0]);
            vertices[1][0] = (float) meta->dst.x;
            vertices[1][1] = (float) (meta->dst.y + meta->height);
            vertices[1][2] = u_uif(meta->clear_val[0]);
            vertices[2][0] = (float) meta->dst.x;
            vertices[2][1] = (float) meta->dst.y;
            vertices[2][2] = u_uif(meta->clear_val[0]);

            vb_start = cmd_state_write(cmd, INTEL_CMD_ITEM_BLOB, 32,
                    sizeof(vertices) / 4, (const uint32_t *) vertices);

            vb_end = vb_start + sizeof(vertices) - 1;
            vb_stride = sizeof(vertices[0]);
            ve_z_source = GEN6_VFCOMP_STORE_SRC;
            ve_format = GEN6_FORMAT_R32G32B32_FLOAT;
        }
        break;
    default:
        assert(!"unknown meta mode");
        return;
        break;
    }

    /* 3DSTATE_VERTEX_BUFFERS */
    pos = cmd_batch_pointer(cmd, 5, &dw);

    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_BUFFERS) | (5 - 2);
    dw[1] = vb_stride;
    if (cmd_gen(cmd) >= INTEL_GEN(7))
        dw[1] |= GEN7_VB_STATE_DW0_ADDR_MODIFIED;

    cmd_reserve_reloc(cmd, 2);
    cmd_batch_reloc_writer(cmd, pos + 2, INTEL_CMD_WRITER_STATE, vb_start);
    cmd_batch_reloc_writer(cmd, pos + 3, INTEL_CMD_WRITER_STATE, vb_end);

    dw[4] = 0;

    /* 3DSTATE_VERTEX_ELEMENTS */
    cmd_batch_pointer(cmd, 5, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VERTEX_ELEMENTS) | (5 - 2);
    dw[1] = GEN6_VE_STATE_DW0_VALID;
    dw[2] = GEN6_VFCOMP_STORE_0 << GEN6_VE_STATE_DW1_COMP0__SHIFT | /* Reserved */
            GEN6_VFCOMP_STORE_0 << GEN6_VE_STATE_DW1_COMP1__SHIFT | /* Render Target Array Index */
            GEN6_VFCOMP_STORE_0 << GEN6_VE_STATE_DW1_COMP2__SHIFT | /* Viewport Index */
            GEN6_VFCOMP_STORE_0 << GEN6_VE_STATE_DW1_COMP3__SHIFT;  /* Point Width */
    dw[3] = GEN6_VE_STATE_DW0_VALID |
            ve_format << GEN6_VE_STATE_DW0_FORMAT__SHIFT;
    dw[4] = GEN6_VFCOMP_STORE_SRC  << GEN6_VE_STATE_DW1_COMP0__SHIFT |
            GEN6_VFCOMP_STORE_SRC  << GEN6_VE_STATE_DW1_COMP1__SHIFT |
            ve_z_source            << GEN6_VE_STATE_DW1_COMP2__SHIFT |
            GEN6_VFCOMP_STORE_1_FP << GEN6_VE_STATE_DW1_COMP3__SHIFT;
}

static uint32_t gen6_meta_vs_constants(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    /* one GPR */
    uint32_t consts[8];
    uint32_t const_count;

    CMD_ASSERT(cmd, 6, 7.5);

    switch (meta->shader_id) {
    case INTEL_DEV_META_VS_FILL_MEM:
        consts[0] = meta->dst.x;
        consts[1] = meta->clear_val[0];
        const_count = 2;
        break;
    case INTEL_DEV_META_VS_COPY_MEM:
    case INTEL_DEV_META_VS_COPY_MEM_UNALIGNED:
        consts[0] = meta->dst.x;
        consts[1] = meta->src.x;
        const_count = 2;
        break;
    case INTEL_DEV_META_VS_COPY_R8_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R16_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32_TO_MEM:
    case INTEL_DEV_META_VS_COPY_R32G32B32A32_TO_MEM:
        consts[0] = meta->src.x;
        consts[1] = meta->src.y;
        consts[2] = meta->width;
        consts[3] = meta->dst.x;
        const_count = 4;
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

static void gen6_meta_vs(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    const struct intel_pipeline_shader *sh =
        intel_dev_get_meta_shader(cmd->dev, meta->shader_id);
    uint32_t offset, *dw;

    CMD_ASSERT(cmd, 6, 7.5);

    if (meta->mode != INTEL_CMD_META_VS_POINTS) {
        uint32_t cmd_len;

        /* 3DSTATE_CONSTANT_VS */
        cmd_len = (cmd_gen(cmd) >= INTEL_GEN(7)) ? 7 : 5;
        cmd_batch_pointer(cmd, cmd_len, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_VS) | (cmd_len - 2);
        memset(&dw[1], 0, sizeof(*dw) * (cmd_len - 1));

        /* 3DSTATE_VS */
        cmd_batch_pointer(cmd, 6, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VS) | (6 - 2);
        memset(&dw[1], 0, sizeof(*dw) * (6 - 1));

        return;
    }

    assert(meta->dst.valid && sh->uses == INTEL_SHADER_USE_VID);

    /* 3DSTATE_CONSTANT_VS */
    offset = gen6_meta_vs_constants(cmd);
    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        cmd_batch_pointer(cmd, 7, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_VS) | (7 - 2);
        dw[1] = 1 << GEN7_PCB_ANY_DW1_PCB0_SIZE__SHIFT;
        dw[2] = 0;
        dw[3] = offset;
        dw[4] = 0;
        dw[5] = 0;
        dw[6] = 0;
    } else {
        cmd_batch_pointer(cmd, 5, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_VS) | (5 - 2) |
            GEN6_PCB_ANY_DW0_PCB0_VALID;
        dw[1] = offset;
        dw[2] = 0;
        dw[3] = 0;
        dw[4] = 0;
    }

    /* 3DSTATE_VS */
    offset = emit_shader(cmd, sh);
    cmd_batch_pointer(cmd, 6, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_VS) | (6 - 2);
    dw[1] = offset;
    dw[2] = GEN6_THREADDISP_SPF |
            (sh->sampler_count + 3) / 4 << GEN6_THREADDISP_SAMPLER_COUNT__SHIFT |
             sh->surface_count << GEN6_THREADDISP_BINDING_TABLE_SIZE__SHIFT;
    dw[3] = 0; /* scratch */
    dw[4] = sh->urb_grf_start << GEN6_VS_DW4_URB_GRF_START__SHIFT |
            1 << GEN6_VS_DW4_URB_READ_LEN__SHIFT;

    dw[5] = GEN6_VS_DW5_CACHE_DISABLE |
            GEN6_VS_DW5_VS_ENABLE;
    if (cmd_gen(cmd) >= INTEL_GEN(7.5))
        dw[5] |= (sh->max_threads - 1) << GEN75_VS_DW5_MAX_THREADS__SHIFT;
    else
        dw[5] |= (sh->max_threads - 1) << GEN6_VS_DW5_MAX_THREADS__SHIFT;

    assert(!sh->per_thread_scratch_size);
}

static void gen6_meta_disabled(struct intel_cmd *cmd)
{
    uint32_t *dw;

    CMD_ASSERT(cmd, 6, 6);

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

static void gen6_meta_clip(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    uint32_t *dw;

    /* 3DSTATE_CLIP */
    cmd_batch_pointer(cmd, 4, &dw);
    dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CLIP) | (4 - 2);
    dw[1] = 0;
    if (meta->mode == INTEL_CMD_META_VS_POINTS) {
        dw[2] = GEN6_CLIP_DW2_CLIP_ENABLE |
                GEN6_CLIP_DW2_CLIPMODE_REJECT_ALL;
    } else {
        dw[2] = 0;
    }
    dw[3] = 0;
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
    if (meta->mode == INTEL_CMD_META_VS_POINTS) {
        /* unused */
        dw[1] = 0;
        dw[2] = 0;
    } else {
        dw[1] = meta->dst.y << 16 | meta->dst.x;
        dw[2] = (meta->dst.y + meta->height - 1) << 16 |
                (meta->dst.x + meta->width - 1);
    }
    dw[3] = 0;
}

static uint32_t gen6_meta_ps_constants(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    uint32_t offset_x, offset_y;
    /* one GPR */
    uint32_t consts[8];
    uint32_t const_count;

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
        consts[1] = meta->clear_val[1];
        const_count = 2;
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

    if (meta->mode != INTEL_CMD_META_FS_RECT) {
        /* 3DSTATE_CONSTANT_PS */
        cmd_batch_pointer(cmd, 5, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_PS) | (5 - 2);
        dw[1] = 0;
        dw[2] = 0;
        dw[3] = 0;
        dw[4] = 0;

        /* 3DSTATE_WM */
        cmd_batch_pointer(cmd, 9, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_WM) | (9 - 2);
        dw[1] = 0;
        dw[2] = 0;
        dw[3] = 0;
        dw[4] = 0;
        dw[5] = (sh->max_threads - 1) << GEN6_WM_DW5_MAX_THREADS__SHIFT;
        dw[6] = 0;
        dw[7] = 0;
        dw[8] = 0;

        return;
    }

    /* a normal color write */
    assert(meta->dst.valid && !sh->uses);

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
    dw[3] = 0; /* scratch */
    dw[4] = sh->urb_grf_start << GEN6_WM_DW4_URB_GRF_START0__SHIFT;
    dw[5] = (sh->max_threads - 1) << GEN6_WM_DW5_MAX_THREADS__SHIFT |
            GEN6_WM_DW5_PS_ENABLE |
            GEN6_WM_DW5_16_PIXEL_DISPATCH;

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

    assert(!sh->per_thread_scratch_size);
}

static void gen7_meta_ps(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    const struct intel_pipeline_shader *sh =
        intel_dev_get_meta_shader(cmd->dev, meta->shader_id);
    uint32_t offset, *dw;

    CMD_ASSERT(cmd, 7, 7.5);

    if (meta->mode != INTEL_CMD_META_FS_RECT) {
        /* 3DSTATE_WM */
        cmd_batch_pointer(cmd, 3, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_WM) | (3 - 2);
        memset(&dw[1], 0, sizeof(*dw) * (3 - 1));

        /* 3DSTATE_CONSTANT_GS */
        cmd_batch_pointer(cmd, 7, &dw);
        dw[0] = GEN6_RENDER_CMD(3D, 3DSTATE_CONSTANT_PS) | (7 - 2);
        memset(&dw[1], 0, sizeof(*dw) * (7 - 1));

        /* 3DSTATE_PS */
        cmd_batch_pointer(cmd, 8, &dw);
        dw[0] = GEN7_RENDER_CMD(3D, 3DSTATE_PS) | (8 - 2);
        dw[1] = 0;
        dw[2] = 0;
        dw[3] = 0;
        dw[4] = GEN7_PS_DW4_8_PIXEL_DISPATCH | /* required to avoid hangs */
                (sh->max_threads - 1) << GEN7_PS_DW4_MAX_THREADS__SHIFT;
        dw[5] = 0;
        dw[6] = 0;
        dw[7] = 0;

        return;
    }

    /* a normal color write */
    assert(meta->dst.valid && !sh->uses);

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
    dw[3] = 0; /* scratch */

    dw[4] = GEN7_PS_DW4_PUSH_CONSTANT_ENABLE |
            GEN7_PS_DW4_POSOFFSET_NONE |
            GEN7_PS_DW4_16_PIXEL_DISPATCH;

    if (cmd_gen(cmd) >= INTEL_GEN(7.5)) {
        dw[4] |= (sh->max_threads - 1) << GEN75_PS_DW4_MAX_THREADS__SHIFT;
        dw[4] |= ((1 << meta->samples) - 1) << GEN75_PS_DW4_SAMPLE_MASK__SHIFT;
    } else {
        dw[4] |= (sh->max_threads - 1) << GEN7_PS_DW4_MAX_THREADS__SHIFT;
    }

    dw[5] = sh->urb_grf_start << GEN7_PS_DW5_URB_GRF_START0__SHIFT;
    dw[6] = 0;
    dw[7] = 0;

    assert(!sh->per_thread_scratch_size);
}

static void gen6_meta_depth_buffer(struct intel_cmd *cmd)
{
    const struct intel_cmd_meta *meta = cmd->bind.meta;
    const struct intel_ds_view *ds = meta->ds.view;

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
                                   const struct intel_desc_set *dset,
                                   const uint32_t *dynamic_offsets)
{
    const uint32_t size = sizeof(*dynamic_offsets) *
        dset->layout->dynamic_desc_count;

    if (size > cmd->bind.dset.graphics_dynamic_offset_size) {
        if (cmd->bind.dset.graphics_dynamic_offsets)
            icd_free(cmd->bind.dset.graphics_dynamic_offsets);

        cmd->bind.dset.graphics_dynamic_offsets = icd_alloc(size,
                4, XGL_SYSTEM_ALLOC_INTERNAL);
        if (!cmd->bind.dset.graphics_dynamic_offsets) {
            cmd_fail(cmd, XGL_ERROR_OUT_OF_MEMORY);
            return;
        }

        cmd->bind.dset.graphics_dynamic_offset_size = size;
    }

    cmd->bind.dset.graphics = dset;
    memcpy(cmd->bind.dset.graphics_dynamic_offsets, dynamic_offsets, size);
}

static void cmd_bind_compute_dset(struct intel_cmd *cmd,
                                  const struct intel_desc_set *dset,
                                  const uint32_t *dynamic_offsets)
{
    const uint32_t size = sizeof(*dynamic_offsets) *
        dset->layout->dynamic_desc_count;

    if (size > cmd->bind.dset.compute_dynamic_offset_size) {
        if (cmd->bind.dset.compute_dynamic_offsets)
            icd_free(cmd->bind.dset.compute_dynamic_offsets);

        cmd->bind.dset.compute_dynamic_offsets = icd_alloc(size,
                4, XGL_SYSTEM_ALLOC_INTERNAL);
        if (!cmd->bind.dset.compute_dynamic_offsets) {
            cmd_fail(cmd, XGL_ERROR_OUT_OF_MEMORY);
            return;
        }

        cmd->bind.dset.compute_dynamic_offset_size = size;
    }

    cmd->bind.dset.compute = dset;
    memcpy(cmd->bind.dset.compute_dynamic_offsets, dynamic_offsets, size);
}

static void cmd_bind_vertex_data(struct intel_cmd *cmd,
                                 const struct intel_buf *buf,
                                 XGL_GPU_SIZE offset, uint32_t binding)
{
    if (binding >= ARRAY_SIZE(cmd->bind.vertex.buf)) {
        cmd_fail(cmd, XGL_ERROR_UNKNOWN);
        return;
    }

    cmd->bind.vertex.buf[binding] = buf;
    cmd->bind.vertex.offset[binding] = offset;
}

static void cmd_bind_index_data(struct intel_cmd *cmd,
                                const struct intel_buf *buf,
                                XGL_GPU_SIZE offset, XGL_INDEX_TYPE type)
{
    cmd->bind.index.buf = buf;
    cmd->bind.index.offset = offset;
    cmd->bind.index.type = type;
}

static void cmd_bind_viewport_state(struct intel_cmd *cmd,
                                    const struct intel_dynamic_vp *state)
{
    cmd->bind.state.viewport = state;
}

static void cmd_bind_raster_state(struct intel_cmd *cmd,
                                  const struct intel_dynamic_rs *state)
{
    cmd->bind.state.raster = state;
}

static void cmd_bind_ds_state(struct intel_cmd *cmd,
                              const struct intel_dynamic_ds *state)
{
    cmd->bind.state.ds = state;
}

static void cmd_bind_blend_state(struct intel_cmd *cmd,
                                 const struct intel_dynamic_cb *state)
{
    cmd->bind.state.blend = state;
}

static uint32_t cmd_get_max_surface_write(const struct intel_cmd *cmd)
{
    const struct intel_pipeline *pipeline = cmd->bind.pipeline.graphics;
    struct intel_pipeline_rmap *rmaps[5] = {
        pipeline->vs.rmap,
        pipeline->tcs.rmap,
        pipeline->tes.rmap,
        pipeline->gs.rmap,
        pipeline->fs.rmap,
    };
    uint32_t max_write;
    int i;

    STATIC_ASSERT(GEN6_ALIGNMENT_SURFACE_STATE >= GEN6_SURFACE_STATE__SIZE);
    STATIC_ASSERT(GEN6_ALIGNMENT_SURFACE_STATE >=
            GEN6_ALIGNMENT_BINDING_TABLE_STATE);

    /* pad first */
    max_write = GEN6_ALIGNMENT_SURFACE_STATE;

    for (i = 0; i < ARRAY_SIZE(rmaps); i++) {
        const struct intel_pipeline_rmap *rmap = rmaps[i];
        const uint32_t surface_count = (rmap) ?
            rmap->rt_count + rmap->texture_resource_count +
            rmap->resource_count + rmap->uav_count : 0;

        if (surface_count) {
            /* SURFACE_STATEs */
            max_write += GEN6_ALIGNMENT_SURFACE_STATE * surface_count;

            /* BINDING_TABLE_STATE */
            max_write += u_align(sizeof(uint32_t) * surface_count,
                    GEN6_ALIGNMENT_SURFACE_STATE);
        }
    }

    return max_write;
}

static void cmd_adjust_state_base_address(struct intel_cmd *cmd)
{
    struct intel_cmd_writer *writer = &cmd->writers[INTEL_CMD_WRITER_SURFACE];
    const uint32_t cur_surface_offset = writer->used - writer->sba_offset;
    uint32_t max_surface_write;

    /* enough for src and dst SURFACE_STATEs plus BINDING_TABLE_STATE */
    if (cmd->bind.meta)
        max_surface_write = 64 * sizeof(uint32_t);
    else
        max_surface_write = cmd_get_max_surface_write(cmd);

    /* there is a 64KB limit on BINDING_TABLE_STATEs */
    if (cur_surface_offset + max_surface_write > 64 * 1024) {
        /* SBA expects page-aligned addresses */
        writer->sba_offset = writer->used & ~0xfff;

        assert((writer->used & 0xfff) + max_surface_write <= 64 * 1024);

        cmd_batch_state_base_address(cmd);
    }
}

static void cmd_draw(struct intel_cmd *cmd,
                     uint32_t vertex_start,
                     uint32_t vertex_count,
                     uint32_t instance_start,
                     uint32_t instance_count,
                     bool indexed,
                     uint32_t vertex_base)
{
    const struct intel_pipeline *p = cmd->bind.pipeline.graphics;
    const uint32_t surface_writer_used U_ASSERT_ONLY =
        cmd->writers[INTEL_CMD_WRITER_SURFACE].used;

    cmd_adjust_state_base_address(cmd);

    emit_bounded_states(cmd);

    /* sanity check on cmd_get_max_surface_write() */
    assert(cmd->writers[INTEL_CMD_WRITER_SURFACE].used -
            surface_writer_used <= cmd_get_max_surface_write(cmd));

    if (indexed) {
        if (p->primitive_restart && !gen6_can_primitive_restart(cmd))
            cmd_fail(cmd, XGL_ERROR_UNKNOWN);

        if (cmd_gen(cmd) >= INTEL_GEN(7.5)) {
            gen75_3DSTATE_VF(cmd, p->primitive_restart,
                    p->primitive_restart_index);
            gen6_3DSTATE_INDEX_BUFFER(cmd, cmd->bind.index.buf,
                    cmd->bind.index.offset, cmd->bind.index.type,
                    false);
        } else {
            gen6_3DSTATE_INDEX_BUFFER(cmd, cmd->bind.index.buf,
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

    if (intel_debug & INTEL_DEBUG_NOCACHE)
        cmd_batch_flush_all(cmd);
}

void cmd_draw_meta(struct intel_cmd *cmd, const struct intel_cmd_meta *meta)
{
    cmd->bind.meta = meta;

    cmd_adjust_state_base_address(cmd);

    cmd_wa_gen6_pre_depth_stall_write(cmd);
    cmd_wa_gen6_pre_command_scoreboard_stall(cmd);

    gen6_meta_dynamic_states(cmd);
    gen6_meta_surface_states(cmd);

    if (cmd_gen(cmd) >= INTEL_GEN(7)) {
        gen7_meta_urb(cmd);
        gen6_meta_vf(cmd);
        gen6_meta_vs(cmd);
        gen7_meta_disabled(cmd);
        gen6_meta_clip(cmd);
        gen6_meta_wm(cmd);
        gen7_meta_ps(cmd);
        gen6_meta_depth_buffer(cmd);

        cmd_wa_gen7_post_command_cs_stall(cmd);
        cmd_wa_gen7_post_command_depth_stall(cmd);

        if (meta->mode == INTEL_CMD_META_VS_POINTS) {
            gen7_3DPRIMITIVE(cmd, GEN6_3DPRIM_POINTLIST, false,
                    meta->width * meta->height, 0, 1, 0, 0);
        } else {
            gen7_3DPRIMITIVE(cmd, GEN6_3DPRIM_RECTLIST, false, 3, 0, 1, 0, 0);
        }
    } else {
        gen6_meta_urb(cmd);
        gen6_meta_vf(cmd);
        gen6_meta_vs(cmd);
        gen6_meta_disabled(cmd);
        gen6_meta_clip(cmd);
        gen6_meta_wm(cmd);
        gen6_meta_ps(cmd);
        gen6_meta_depth_buffer(cmd);

        if (meta->mode == INTEL_CMD_META_VS_POINTS) {
            gen6_3DPRIMITIVE(cmd, GEN6_3DPRIM_POINTLIST, false,
                    meta->width * meta->height, 0, 1, 0, 0);
        } else {
            gen6_3DPRIMITIVE(cmd, GEN6_3DPRIM_RECTLIST, false, 3, 0, 1, 0, 0);
        }
    }

    cmd->bind.draw_count++;
    /* need to re-emit all workarounds */
    cmd->bind.wa_flags = 0;

    cmd->bind.meta = NULL;

    if (intel_debug & INTEL_DEBUG_NOCACHE)
        cmd_batch_flush_all(cmd);
}

ICD_EXPORT void XGLAPI xglCmdBindPipeline(
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
        cmd_fail(cmd, XGL_ERROR_INVALID_VALUE);
        break;
    }
}

ICD_EXPORT void XGLAPI xglCmdBindPipelineDelta(
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
        cmd_fail(cmd, XGL_ERROR_INVALID_VALUE);
        break;
    }
}

ICD_EXPORT void XGLAPI xglCmdBindDynamicStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_DYNAMIC_STATE_OBJECT                    state)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    switch (stateBindPoint) {
    case XGL_STATE_BIND_VIEWPORT:
        cmd_bind_viewport_state(cmd,
                intel_dynamic_vp((XGL_DYNAMIC_VP_STATE_OBJECT) state));
        break;
    case XGL_STATE_BIND_RASTER:
        cmd_bind_raster_state(cmd,
                intel_dynamic_rs((XGL_DYNAMIC_RS_STATE_OBJECT) state));
        break;
    case XGL_STATE_BIND_DEPTH_STENCIL:
        cmd_bind_ds_state(cmd,
                intel_dynamic_ds((XGL_DYNAMIC_DS_STATE_OBJECT) state));
        break;
    case XGL_STATE_BIND_COLOR_BLEND:
        cmd_bind_blend_state(cmd,
                intel_dynamic_cb((XGL_DYNAMIC_CB_STATE_OBJECT) state));
        break;
    default:
        cmd_fail(cmd, XGL_ERROR_INVALID_VALUE);
        break;
    }
}

ICD_EXPORT void XGLAPI xglCmdBindDescriptorSet(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_DESCRIPTOR_SET                          descriptorSet,
    const uint32_t*                             pUserData)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_desc_set *dset = intel_desc_set(descriptorSet);

    switch (pipelineBindPoint) {
    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        cmd_bind_compute_dset(cmd, dset, pUserData);
        break;
    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        cmd_bind_graphics_dset(cmd, dset, pUserData);
        break;
    default:
        cmd_fail(cmd, XGL_ERROR_INVALID_VALUE);
        break;
    }
}

ICD_EXPORT void XGLAPI xglCmdBindVertexBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    uint32_t                                    binding)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_buf *buf = intel_buf(buffer);

    cmd_bind_vertex_data(cmd, buf, offset, binding);
}

ICD_EXPORT void XGLAPI xglCmdBindIndexBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);
    struct intel_buf *buf = intel_buf(buffer);

    cmd_bind_index_data(cmd, buf, offset, indexType);
}

ICD_EXPORT void XGLAPI xglCmdDraw(
    XGL_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    firstVertex,
    uint32_t                                    vertexCount,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_draw(cmd, firstVertex, vertexCount,
            firstInstance, instanceCount, false, 0);
}

ICD_EXPORT void XGLAPI xglCmdDrawIndexed(
    XGL_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    firstIndex,
    uint32_t                                    indexCount,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_draw(cmd, firstIndex, indexCount,
            firstInstance, instanceCount, true, vertexOffset);
}

ICD_EXPORT void XGLAPI xglCmdDrawIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    uint32_t                                    count,
    uint32_t                                    stride)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_fail(cmd, XGL_ERROR_UNKNOWN);
}

ICD_EXPORT void XGLAPI xglCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    uint32_t                                    count,
    uint32_t                                    stride)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_fail(cmd, XGL_ERROR_UNKNOWN);
}

ICD_EXPORT void XGLAPI xglCmdDispatch(
    XGL_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_fail(cmd, XGL_ERROR_UNKNOWN);
}

ICD_EXPORT void XGLAPI xglCmdDispatchIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd_fail(cmd, XGL_ERROR_UNKNOWN);
}

ICD_EXPORT void XGLAPI xglCmdBeginRenderPass(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_RENDER_PASS                             renderPass)
{
   struct intel_cmd *cmd = intel_cmd(cmdBuffer);

   cmd_begin_render_pass(cmd, (struct intel_render_pass *) renderPass);
}

ICD_EXPORT void XGLAPI xglCmdEndRenderPass(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_RENDER_PASS                             renderPass)
{
   struct intel_cmd *cmd = intel_cmd(cmdBuffer);

   cmd_end_render_pass(cmd, (struct intel_render_pass *) renderPass);
}
