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

#include <math.h>
#include "genhw/genhw.h"
#include "dev.h"
#include "state.h"

static int translate_compare_func(XGL_COMPARE_FUNC func)
{
    switch (func) {
    case XGL_COMPARE_NEVER:         return GEN6_COMPAREFUNCTION_NEVER;
    case XGL_COMPARE_LESS:          return GEN6_COMPAREFUNCTION_LESS;
    case XGL_COMPARE_EQUAL:         return GEN6_COMPAREFUNCTION_EQUAL;
    case XGL_COMPARE_LESS_EQUAL:    return GEN6_COMPAREFUNCTION_LEQUAL;
    case XGL_COMPARE_GREATER:       return GEN6_COMPAREFUNCTION_GREATER;
    case XGL_COMPARE_NOT_EQUAL:     return GEN6_COMPAREFUNCTION_NOTEQUAL;
    case XGL_COMPARE_GREATER_EQUAL: return GEN6_COMPAREFUNCTION_GEQUAL;
    case XGL_COMPARE_ALWAYS:        return GEN6_COMPAREFUNCTION_ALWAYS;
    default:
      assert(!"unknown compare_func");
      return GEN6_COMPAREFUNCTION_NEVER;
    }
}

static int translate_stencil_op(XGL_STENCIL_OP op)
{
    switch (op) {
    case XGL_STENCIL_OP_KEEP:       return GEN6_STENCILOP_KEEP;
    case XGL_STENCIL_OP_ZERO:       return GEN6_STENCILOP_ZERO;
    case XGL_STENCIL_OP_REPLACE:    return GEN6_STENCILOP_REPLACE;
    case XGL_STENCIL_OP_INC_CLAMP:  return GEN6_STENCILOP_INCRSAT;
    case XGL_STENCIL_OP_DEC_CLAMP:  return GEN6_STENCILOP_DECRSAT;
    case XGL_STENCIL_OP_INVERT:     return GEN6_STENCILOP_INVERT;
    case XGL_STENCIL_OP_INC_WRAP:   return GEN6_STENCILOP_INCR;
    case XGL_STENCIL_OP_DEC_WRAP:   return GEN6_STENCILOP_DECR;
    default:
      assert(!"unknown stencil op");
      return GEN6_STENCILOP_KEEP;
    }
}

static int translate_blend_func(XGL_BLEND_FUNC func)
{
   switch (func) {
   case XGL_BLEND_FUNC_ADD:                return GEN6_BLENDFUNCTION_ADD;
   case XGL_BLEND_FUNC_SUBTRACT:           return GEN6_BLENDFUNCTION_SUBTRACT;
   case XGL_BLEND_FUNC_REVERSE_SUBTRACT:   return GEN6_BLENDFUNCTION_REVERSE_SUBTRACT;
   case XGL_BLEND_FUNC_MIN:                return GEN6_BLENDFUNCTION_MIN;
   case XGL_BLEND_FUNC_MAX:                return GEN6_BLENDFUNCTION_MAX;
   default:
      assert(!"unknown blend func");
      return GEN6_BLENDFUNCTION_ADD;
   };
}

static int translate_blend(XGL_BLEND blend)
{
   switch (blend) {
   case XGL_BLEND_ZERO:                     return GEN6_BLENDFACTOR_ZERO;
   case XGL_BLEND_ONE:                      return GEN6_BLENDFACTOR_ONE;
   case XGL_BLEND_SRC_COLOR:                return GEN6_BLENDFACTOR_SRC_COLOR;
   case XGL_BLEND_ONE_MINUS_SRC_COLOR:      return GEN6_BLENDFACTOR_INV_SRC_COLOR;
   case XGL_BLEND_DEST_COLOR:               return GEN6_BLENDFACTOR_DST_COLOR;
   case XGL_BLEND_ONE_MINUS_DEST_COLOR:     return GEN6_BLENDFACTOR_INV_DST_COLOR;
   case XGL_BLEND_SRC_ALPHA:                return GEN6_BLENDFACTOR_SRC_ALPHA;
   case XGL_BLEND_ONE_MINUS_SRC_ALPHA:      return GEN6_BLENDFACTOR_INV_SRC_ALPHA;
   case XGL_BLEND_DEST_ALPHA:               return GEN6_BLENDFACTOR_DST_ALPHA;
   case XGL_BLEND_ONE_MINUS_DEST_ALPHA:     return GEN6_BLENDFACTOR_INV_DST_ALPHA;
   case XGL_BLEND_CONSTANT_COLOR:           return GEN6_BLENDFACTOR_CONST_COLOR;
   case XGL_BLEND_ONE_MINUS_CONSTANT_COLOR: return GEN6_BLENDFACTOR_INV_CONST_COLOR;
   case XGL_BLEND_CONSTANT_ALPHA:           return GEN6_BLENDFACTOR_CONST_ALPHA;
   case XGL_BLEND_ONE_MINUS_CONSTANT_ALPHA: return GEN6_BLENDFACTOR_INV_CONST_ALPHA;
   case XGL_BLEND_SRC_ALPHA_SATURATE:       return GEN6_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case XGL_BLEND_SRC1_COLOR:               return GEN6_BLENDFACTOR_SRC1_COLOR;
   case XGL_BLEND_ONE_MINUS_SRC1_COLOR:     return GEN6_BLENDFACTOR_INV_SRC1_COLOR;
   case XGL_BLEND_SRC1_ALPHA:               return GEN6_BLENDFACTOR_SRC1_ALPHA;
   case XGL_BLEND_ONE_MINUS_SRC1_ALPHA:     return GEN6_BLENDFACTOR_INV_SRC1_ALPHA;
   default:
      assert(!"unknown blend factor");
      return GEN6_BLENDFACTOR_ONE;
   };
}

static void
raster_state_init(struct intel_raster_state *state,
                  const struct intel_gpu *gpu,
                  const XGL_RASTER_STATE_CREATE_INFO *info)
{
    switch (info->fillMode) {
    case XFL_FILL_POINTS:
        state->cmd_sf_fill |= GEN7_SF_DW1_FRONTFACE_POINT |
                              GEN7_SF_DW1_BACKFACE_POINT;
        break;
    case XGL_FILL_WIREFRAME:
        state->cmd_sf_fill |= GEN7_SF_DW1_FRONTFACE_WIREFRAME |
                              GEN7_SF_DW1_BACKFACE_WIREFRAME;
        break;
    case XGL_FILL_SOLID:
    default:
        state->cmd_sf_fill |= GEN7_SF_DW1_FRONTFACE_SOLID |
                              GEN7_SF_DW1_BACKFACE_SOLID;
        break;
    }

    if (info->frontFace == XGL_FRONT_FACE_CCW) {
        state->cmd_sf_fill |= GEN7_SF_DW1_FRONTWINDING_CCW;
        state->cmd_clip_cull |= GEN7_CLIP_DW1_FRONTWINDING_CCW;
    }

    switch (info->cullMode) {
    case XGL_CULL_NONE:
    default:
        state->cmd_sf_cull |= GEN7_SF_DW2_CULLMODE_NONE;
        state->cmd_clip_cull |= GEN7_CLIP_DW1_CULLMODE_NONE;
        break;
    case XGL_CULL_FRONT:
        state->cmd_sf_cull |= GEN7_SF_DW2_CULLMODE_FRONT;
        state->cmd_clip_cull |= GEN7_CLIP_DW1_CULLMODE_FRONT;
        break;
    case XGL_CULL_BACK:
        state->cmd_sf_cull |= GEN7_SF_DW2_CULLMODE_BACK;
        state->cmd_clip_cull |= GEN7_CLIP_DW1_CULLMODE_BACK;
        break;
    case XGL_CULL_FRONT_AND_BACK:
        state->cmd_sf_cull |= GEN7_SF_DW2_CULLMODE_BOTH;
        state->cmd_clip_cull |= GEN7_CLIP_DW1_CULLMODE_BOTH;
        break;
    }

    /* only GEN7+ needs cull mode in 3DSTATE_CLIP */
    if (intel_gpu_gen(gpu) == INTEL_GEN(6))
        state->cmd_clip_cull = 0;

    /* XXX scale info->depthBias back into NDC */
    state->cmd_depth_offset_const = u_fui((float) info->depthBias * 2.0f);
    state->cmd_depth_offset_clamp = u_fui(info->depthBiasClamp);
    state->cmd_depth_offset_scale = u_fui(info->slopeScaledDepthBias);
}

static void
viewport_get_guardband(const struct intel_gpu *gpu,
                       int center_x, int center_y,
                       int *min_gbx, int *max_gbx,
                       int *min_gby, int *max_gby)
{
   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 234:
    *
    *     "Per-Device Guardband Extents
    *
    *       - Supported X,Y ScreenSpace "Guardband" Extent: [-16K,16K-1]
    *       - Maximum Post-Clamp Delta (X or Y): 16K"
    *
    *     "In addition, in order to be correctly rendered, objects must have a
    *      screenspace bounding box not exceeding 8K in the X or Y direction.
    *      This additional restriction must also be comprehended by software,
    *      i.e., enforced by use of clipping."
    *
    * From the Ivy Bridge PRM, volume 2 part 1, page 248:
    *
    *     "Per-Device Guardband Extents
    *
    *       - Supported X,Y ScreenSpace "Guardband" Extent: [-32K,32K-1]
    *       - Maximum Post-Clamp Delta (X or Y): N/A"
    *
    *     "In addition, in order to be correctly rendered, objects must have a
    *      screenspace bounding box not exceeding 8K in the X or Y direction.
    *      This additional restriction must also be comprehended by software,
    *      i.e., enforced by use of clipping."
    *
    * Combined, the bounding box of any object can not exceed 8K in both
    * width and height.
    *
    * Below we set the guardband as a squre of length 8K, centered at where
    * the viewport is.  This makes sure all objects passing the GB test are
    * valid to the renderer, and those failing the XY clipping have a
    * better chance of passing the GB test.
    */
   const int max_extent = (intel_gpu_gen(gpu) >= INTEL_GEN(7)) ? 32768 : 16384;
   const int half_len = 8192 / 2;

   /* make sure the guardband is within the valid range */
   if (center_x - half_len < -max_extent)
      center_x = -max_extent + half_len;
   else if (center_x + half_len > max_extent - 1)
      center_x = max_extent - half_len;

   if (center_y - half_len < -max_extent)
      center_y = -max_extent + half_len;
   else if (center_y + half_len > max_extent - 1)
      center_y = max_extent - half_len;

   *min_gbx = (float) (center_x - half_len);
   *max_gbx = (float) (center_x + half_len);
   *min_gby = (float) (center_y - half_len);
   *max_gby = (float) (center_y + half_len);
}

static XGL_RESULT
viewport_state_alloc_cmd(struct intel_viewport_state *state,
                         const struct intel_gpu *gpu,
                         const XGL_VIEWPORT_STATE_CREATE_INFO *info)
{
    INTEL_GPU_ASSERT(gpu, 6, 7.5);

    state->viewport_count = info->viewportCount;
    state->scissor_enable = info->scissorEnable;

    if (intel_gpu_gen(gpu) >= INTEL_GEN(7)) {
        state->cmd_align = GEN7_ALIGNMENT_SF_CLIP_VIEWPORT;
        state->cmd_len = 16 * info->viewportCount;

        state->cmd_clip_offset = 8;
    } else {
        state->cmd_align = GEN6_ALIGNMENT_SF_VIEWPORT;
        state->cmd_len = 8 * info->viewportCount;

        state->cmd_clip_offset =
            u_align(state->cmd_len, GEN6_ALIGNMENT_CLIP_VIEWPORT);
        state->cmd_len = state->cmd_clip_offset + 4 * info->viewportCount;
    }

    state->cmd_cc_offset =
        u_align(state->cmd_len, GEN6_ALIGNMENT_CC_VIEWPORT);
    state->cmd_len = state->cmd_cc_offset + 2 * info->viewportCount;

    if (state->scissor_enable) {
        state->cmd_scissor_rect_offset =
            u_align(state->cmd_len, GEN6_ALIGNMENT_SCISSOR_RECT);
        state->cmd_len = state->cmd_scissor_rect_offset +
            2 * info->viewportCount;
    }

    state->cmd = icd_alloc(sizeof(uint32_t) * state->cmd_len,
            0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!state->cmd)
        return XGL_ERROR_OUT_OF_MEMORY;

    return XGL_SUCCESS;
}

static XGL_RESULT
viewport_state_init(struct intel_viewport_state *state,
                    const struct intel_gpu *gpu,
                    const XGL_VIEWPORT_STATE_CREATE_INFO *info)
{
    const XGL_UINT sf_stride = (intel_gpu_gen(gpu) >= INTEL_GEN(7)) ? 16 : 8;
    const XGL_UINT clip_stride = (intel_gpu_gen(gpu) >= INTEL_GEN(7)) ? 16 : 4;
    uint32_t *sf_viewport, *clip_viewport, *cc_viewport, *scissor_rect;
    XGL_UINT i;
    XGL_RESULT ret;

    INTEL_GPU_ASSERT(gpu, 6, 7.5);

    ret = viewport_state_alloc_cmd(state, gpu, info);
    if (ret != XGL_SUCCESS)
        return ret;

    sf_viewport = state->cmd;
    clip_viewport = state->cmd + state->cmd_clip_offset;
    cc_viewport = state->cmd + state->cmd_cc_offset;
    scissor_rect = state->cmd + state->cmd_scissor_rect_offset;

    for (i = 0; i < info->viewportCount; i++) {
        const XGL_VIEWPORT *viewport = &info->viewports[i];
        const XGL_RECT *scissor = &info->scissors[i];
        uint32_t *dw = NULL;
        float translate[3], scale[3];
        int min_gbx, max_gbx, min_gby, max_gby;

        scale[0] = viewport->width / 2.0f;
        scale[1] = viewport->height / 2.0f;
        scale[2] = (viewport->maxDepth - viewport->minDepth) / 2.0;
        translate[0] = viewport->originX + scale[0];
        translate[1] = viewport->originY + scale[1];
        translate[2] = (viewport->minDepth + viewport->maxDepth) / 2.0f;

        viewport_get_guardband(gpu, (int) translate[0], (int) translate[1],
                &min_gbx, &max_gbx, &min_gby, &max_gby);

        /* SF_VIEWPORT */
        dw = sf_viewport;
        dw[0] = u_fui(scale[0]);
        dw[1] = u_fui(scale[1]);
        dw[2] = u_fui(scale[2]);
        dw[3] = u_fui(translate[0]);
        dw[4] = u_fui(translate[1]);
        dw[5] = u_fui(translate[2]);
        dw[6] = 0;
        dw[7] = 0;
        sf_viewport += sf_stride;

        /* CLIP_VIEWPORT */
        dw = clip_viewport;
        dw[0] = u_fui(((float) min_gbx - translate[0]) / fabsf(scale[0]));
        dw[1] = u_fui(((float) max_gbx - translate[0]) / fabsf(scale[0]));
        dw[2] = u_fui(((float) min_gby - translate[1]) / fabsf(scale[1]));
        dw[3] = u_fui(((float) max_gby - translate[1]) / fabsf(scale[1]));
        clip_viewport += clip_stride;

        /* CC_VIEWPORT */
        dw = cc_viewport;
        dw[0] = u_fui(viewport->minDepth);
        dw[1] = u_fui(viewport->maxDepth);
        cc_viewport += 2;

        /* SCISSOR_RECT */
        if (state->scissor_enable) {
            int16_t max_x, max_y;

            max_x = (scissor->offset.x + scissor->extent.width - 1) & 0xffff;
            max_y = (scissor->offset.y + scissor->extent.height - 1) & 0xffff;

            dw = scissor_rect;
            if (scissor->extent.width && scissor->extent.height) {
                dw[0] = (scissor->offset.y & 0xffff) << 16 |
                        (scissor->offset.x & 0xffff);
                dw[1] = max_y << 16 | max_x;
            } else {
                dw[0] = 1 << 16 | 1;
                dw[1] = 0;
            }
            scissor_rect += 2;
        }
    }

    return XGL_SUCCESS;
}

static void
msaa_state_init(struct intel_msaa_state *state,
                const struct intel_gpu *gpu,
                const XGL_MSAA_STATE_CREATE_INFO *info)
{
    /* taken from Mesa */
    static const uint32_t brw_multisample_positions_4x = 0xae2ae662;
    static const uint32_t brw_multisample_positions_8x[] = { 0xdbb39d79, 0x3ff55117 };
    uint32_t cmd, cmd_len;
    uint32_t *dw = state->cmd;

    INTEL_GPU_ASSERT(gpu, 6, 7.5);
    STATIC_ASSERT(ARRAY_SIZE(state->cmd) >= 6);

    state->sample_count = info->samples;
    if (!state->sample_count)
        state->sample_count = 1;

    /* 3DSTATE_MULTISAMPLE */
    cmd = GEN6_RENDER_CMD(3D, 3DSTATE_MULTISAMPLE);
    cmd_len = (intel_gpu_gen(gpu) >= INTEL_GEN(7)) ? 4 : 3;

    dw[0] = cmd | (cmd_len - 2);
    if (info->samples <= 1) {
        dw[1] = GEN6_MULTISAMPLE_DW1_NUMSAMPLES_1;
        dw[2] = 0;
    } else if (info->samples <= 4 || intel_gpu_gen(gpu) == INTEL_GEN(6)) {
        dw[1] = GEN6_MULTISAMPLE_DW1_NUMSAMPLES_4;
        dw[2] = brw_multisample_positions_4x;
    } else {
        dw[1] = GEN7_MULTISAMPLE_DW1_NUMSAMPLES_8;
        dw[2] = brw_multisample_positions_8x[0];
        dw[3] = brw_multisample_positions_8x[1];
    }

    dw += cmd_len;

    state->cmd_len = cmd_len + 2;

    /* 3DSTATE_SAMPLE_MASK */
    cmd = GEN6_RENDER_CMD(3D, 3DSTATE_SAMPLE_MASK);
    cmd_len = 2;

    dw[0] = cmd | (cmd_len - 2);
    dw[1] = info->sampleMask & ((1 << info->samples) - 1);
}

static void
blend_state_init(struct intel_blend_state *state,
                 const struct intel_gpu *gpu,
                 const XGL_COLOR_BLEND_STATE_CREATE_INFO *info)
{
   XGL_UINT i;

   INTEL_GPU_ASSERT(gpu, 6, 7.5);

   for (i = 0; i < ARRAY_SIZE(info->attachment); i++) {
      const XGL_COLOR_ATTACHMENT_BLEND_STATE *att = &info->attachment[i];
      uint32_t *dw = &state->cmd[2 * i];

      if (att->blendEnable) {
         dw[0] = 1 << 31 |
                 translate_blend_func(att->blendFuncAlpha) << 26 |
                 translate_blend(att->srcBlendAlpha) << 20 |
                 translate_blend(att->destBlendAlpha) << 15 |
                 translate_blend_func(att->blendFuncColor) << 11 |
                 translate_blend(att->srcBlendColor) << 5 |
                 translate_blend(att->destBlendColor);

         if (att->blendFuncAlpha != att->blendFuncColor ||
             att->srcBlendAlpha != att->srcBlendColor ||
             att->destBlendAlpha != att->destBlendColor)
             dw[0] |= 1 << 30;
      }

      dw[1] = GEN6_BLEND_DW1_COLORCLAMP_RTFORMAT |
              0x3;
   }

   memcpy(state->cmd_blend_color, info->blendConst, sizeof(info->blendConst));
}

static XGL_RESULT
ds_state_init(struct intel_ds_state *state,
              const struct intel_gpu *gpu,
              const XGL_DEPTH_STENCIL_STATE_CREATE_INFO *info)
{
   uint32_t *dw = state->cmd;

   INTEL_GPU_ASSERT(gpu, 6, 7.5);

   STATIC_ASSERT(ARRAY_SIZE(state->cmd) >= 3);

   if (info->depthBoundsEnable)
       return XGL_ERROR_UNKNOWN;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 359:
    *
    *     "If the Depth Buffer is either undefined or does not have a surface
    *      format of D32_FLOAT_S8X24_UINT or D24_UNORM_S8_UINT and separate
    *      stencil buffer is disabled, Stencil Test Enable must be DISABLED"
    *
    * From the Sandy Bridge PRM, volume 2 part 1, page 370:
    *
    *     "This field (Stencil Test Enable) cannot be enabled if
    *      Surface Format in 3DSTATE_DEPTH_BUFFER is set to D16_UNORM."
    *
    * TODO We do not check these yet.
    */
   if (info->stencilTestEnable) {
      dw[0] = 1 << 31 |
              translate_compare_func(info->front.stencilFunc) << 28 |
              translate_stencil_op(info->front.stencilFailOp) << 25 |
              translate_stencil_op(info->front.stencilDepthFailOp) << 22 |
              translate_stencil_op(info->front.stencilPassOp) << 19 |
              1 << 15 |
              translate_compare_func(info->back.stencilFunc) << 12 |
              translate_stencil_op(info->back.stencilFailOp) << 9 |
              translate_stencil_op(info->back.stencilDepthFailOp) << 6 |
              translate_stencil_op(info->back.stencilPassOp) << 3;

      if (info->stencilWriteMask)
         dw[0] |= 1 << 18;

      dw[1] = (info->stencilReadMask & 0xff) << 24 |
              (info->stencilWriteMask & 0xff) << 16;

      state->cmd_stencil_ref = (info->front.stencilRef & 0xff) << 24 |
                               (info->back.stencilRef & 0xff) << 16;
   }

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 360:
    *
    *     "Enabling the Depth Test function without defining a Depth Buffer is
    *      UNDEFINED."
    *
    * From the Sandy Bridge PRM, volume 2 part 1, page 375:
    *
    *     "A Depth Buffer must be defined before enabling writes to it, or
    *      operation is UNDEFINED."
    *
    * TODO We do not check these yet.
    */
   if (info->depthTestEnable) {
      dw[2] = 1 << 31 |
              translate_compare_func(info->depthFunc) << 27 |
              (bool) info->depthWriteEnable << 26;
   } else {
      dw[2] = GEN6_COMPAREFUNCTION_ALWAYS << 27;
   }

   return XGL_SUCCESS;
}

static void viewport_state_destroy(struct intel_obj *obj)
{
    struct intel_viewport_state *state = intel_viewport_state_from_obj(obj);

    intel_viewport_state_destroy(state);
}

XGL_RESULT intel_viewport_state_create(struct intel_dev *dev,
                                       const XGL_VIEWPORT_STATE_CREATE_INFO *info,
                                       struct intel_viewport_state **state_ret)
{
    struct intel_viewport_state *state;
    XGL_RESULT ret;

    state = (struct intel_viewport_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_VIEWPORT_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = viewport_state_destroy;

    ret = viewport_state_init(state, dev->gpu, info);
    if (ret != XGL_SUCCESS) {
        intel_viewport_state_destroy(state);
        return ret;
    }

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_viewport_state_destroy(struct intel_viewport_state *state)
{
    icd_free(state->cmd);
    intel_base_destroy(&state->obj.base);
}

static void raster_state_destroy(struct intel_obj *obj)
{
    struct intel_raster_state *state = intel_raster_state_from_obj(obj);

    intel_raster_state_destroy(state);
}

XGL_RESULT intel_raster_state_create(struct intel_dev *dev,
                                     const XGL_RASTER_STATE_CREATE_INFO *info,
                                     struct intel_raster_state **state_ret)
{
    struct intel_raster_state *state;

    state = (struct intel_raster_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_RASTER_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = raster_state_destroy;

    raster_state_init(state, dev->gpu, info);

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_raster_state_destroy(struct intel_raster_state *state)
{
    intel_base_destroy(&state->obj.base);
}

static void msaa_state_destroy(struct intel_obj *obj)
{
    struct intel_msaa_state *state = intel_msaa_state_from_obj(obj);

    intel_msaa_state_destroy(state);
}

XGL_RESULT intel_msaa_state_create(struct intel_dev *dev,
                                   const XGL_MSAA_STATE_CREATE_INFO *info,
                                   struct intel_msaa_state **state_ret)
{
    struct intel_msaa_state *state;

    state = (struct intel_msaa_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_MSAA_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = msaa_state_destroy;

    msaa_state_init(state, dev->gpu, info);

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_msaa_state_destroy(struct intel_msaa_state *state)
{
    intel_base_destroy(&state->obj.base);
}

static void blend_state_destroy(struct intel_obj *obj)
{
    struct intel_blend_state *state = intel_blend_state_from_obj(obj);

    intel_blend_state_destroy(state);
}

XGL_RESULT intel_blend_state_create(struct intel_dev *dev,
                                    const XGL_COLOR_BLEND_STATE_CREATE_INFO *info,
                                    struct intel_blend_state **state_ret)
{
    struct intel_blend_state *state;

    state = (struct intel_blend_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_COLOR_BLEND_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = blend_state_destroy;

    blend_state_init(state, dev->gpu, info);

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_blend_state_destroy(struct intel_blend_state *state)
{
    intel_base_destroy(&state->obj.base);
}

static void ds_state_destroy(struct intel_obj *obj)
{
    struct intel_ds_state *state = intel_ds_state_from_obj(obj);

    intel_ds_state_destroy(state);
}

XGL_RESULT intel_ds_state_create(struct intel_dev *dev,
                                 const XGL_DEPTH_STENCIL_STATE_CREATE_INFO *info,
                                 struct intel_ds_state **state_ret)
{
    struct intel_ds_state *state;
    XGL_RESULT ret;

    state = (struct intel_ds_state *) intel_base_create(dev,
            sizeof(*state), dev->base.dbg, XGL_DBG_OBJECT_DEPTH_STENCIL_STATE,
            info, 0);
    if (!state)
        return XGL_ERROR_OUT_OF_MEMORY;

    state->obj.destroy = ds_state_destroy;

    ret = ds_state_init(state, dev->gpu, info);
    if (ret != XGL_SUCCESS) {
        intel_ds_state_destroy(state);
        return ret;
    }

    *state_ret = state;

    return XGL_SUCCESS;
}

void intel_ds_state_destroy(struct intel_ds_state *state)
{
    intel_base_destroy(&state->obj.base);
}

XGL_RESULT XGLAPI intelCreateViewportState(
    XGL_DEVICE                                  device,
    const XGL_VIEWPORT_STATE_CREATE_INFO*       pCreateInfo,
    XGL_VIEWPORT_STATE_OBJECT*                  pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_viewport_state_create(dev, pCreateInfo,
            (struct intel_viewport_state **) pState);
}

XGL_RESULT XGLAPI intelCreateRasterState(
    XGL_DEVICE                                  device,
    const XGL_RASTER_STATE_CREATE_INFO*         pCreateInfo,
    XGL_RASTER_STATE_OBJECT*                    pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_raster_state_create(dev, pCreateInfo,
            (struct intel_raster_state **) pState);
}

XGL_RESULT XGLAPI intelCreateMsaaState(
    XGL_DEVICE                                  device,
    const XGL_MSAA_STATE_CREATE_INFO*           pCreateInfo,
    XGL_MSAA_STATE_OBJECT*                      pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_msaa_state_create(dev, pCreateInfo,
            (struct intel_msaa_state **) pState);
}

XGL_RESULT XGLAPI intelCreateColorBlendState(
    XGL_DEVICE                                  device,
    const XGL_COLOR_BLEND_STATE_CREATE_INFO*    pCreateInfo,
    XGL_COLOR_BLEND_STATE_OBJECT*               pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_blend_state_create(dev, pCreateInfo,
            (struct intel_blend_state **) pState);
}

XGL_RESULT XGLAPI intelCreateDepthStencilState(
    XGL_DEVICE                                  device,
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO*  pCreateInfo,
    XGL_DEPTH_STENCIL_STATE_OBJECT*             pState)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_ds_state_create(dev, pCreateInfo,
            (struct intel_ds_state **) pState);
}
