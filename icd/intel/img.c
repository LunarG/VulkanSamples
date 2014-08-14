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

#include "kmd/winsys.h"
#include "dev.h"
#include "gpu.h"
#include "img.h"

struct tex_layout {
   const struct intel_gpu *gpu;
   const XGL_IMAGE_CREATE_INFO *info;

   bool has_depth, has_stencil;
   bool hiz, separate_stencil;

   XGL_FORMAT format;
   unsigned block_width, block_height, block_size;
   bool compressed;

   enum intel_tiling_mode tiling;
   unsigned valid_tilings; /* bitmask of valid tiling modes */

   bool array_spacing_full;
   bool interleaved;

   struct {
      int w, h, d;
      struct intel_img_slice *slices;
   } levels[INTEL_IMG_MAX_LEVELS];

   int align_i, align_j;
   int qpitch;

   int width, height;

   int bo_stride, bo_height;
   int hiz_stride, hiz_height;
};

/*
 * From the Ivy Bridge PRM, volume 1 part 1, page 105:
 *
 *     "In addition to restrictions on maximum height, width, and depth,
 *      surfaces are also restricted to a maximum size in bytes. This
 *      maximum is 2 GB for all products and all surface types."
 */
static const size_t intel_max_resource_size = 1u << 31;

static void
tex_layout_init_qpitch(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;
   int h0, h1;

   if (info->arraySize <= 1)
      return;

   h0 = u_align(layout->levels[0].h, layout->align_j);

   if (!layout->array_spacing_full) {
      layout->qpitch = h0;
      return;
   }

   h1 = u_align(layout->levels[1].h, layout->align_j);

   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 115:
    *
    *     "The following equation is used for surface formats other than
    *      compressed textures:
    *
    *        QPitch = (h0 + h1 + 11j)"
    *
    *     "The equation for compressed textures (BC* and FXT1 surface formats)
    *      follows:
    *
    *        QPitch = (h0 + h1 + 11j) / 4"
    *
    *     "[DevSNB] Errata: Sampler MSAA Qpitch will be 4 greater than the
    *      value calculated in the equation above, for every other odd Surface
    *      Height starting from 1 i.e. 1,5,9,13"
    *
    * From the Ivy Bridge PRM, volume 1 part 1, page 111-112:
    *
    *     "If Surface Array Spacing is set to ARYSPC_FULL (note that the depth
    *      buffer and stencil buffer have an implied value of ARYSPC_FULL):
    *
    *        QPitch = (h0 + h1 + 12j)
    *        QPitch = (h0 + h1 + 12j) / 4 (compressed)
    *
    *      (There are many typos or missing words here...)"
    *
    * To access the N-th slice, an offset of (Stride * QPitch * N) is added to
    * the base address.  The PRM divides QPitch by 4 for compressed formats
    * because the block height for those formats are 4, and it wants QPitch to
    * mean the number of memory rows, as opposed to texel rows, between
    * slices.  Since we use texel rows in tex->slice_offsets, we do not need
    * to divide QPitch by 4.
    */
   layout->qpitch = h0 + h1 +
      ((intel_gpu_gen(layout->gpu) >= INTEL_GEN(7)) ? 12 : 11) * layout->align_j;

   if (intel_gpu_gen(layout->gpu) == INTEL_GEN(6) && info->samples > 1 &&
       info->extent.height % 4 == 1)
      layout->qpitch += 4;
}

static void
tex_layout_init_alignments(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;

   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 113:
    *
    *     "surface format           align_i     align_j
    *      YUV 4:2:2 formats        4           *see below
    *      BC1-5                    4           4
    *      FXT1                     8           4
    *      all other formats        4           *see below"
    *
    *     "- align_j = 4 for any depth buffer
    *      - align_j = 2 for separate stencil buffer
    *      - align_j = 4 for any render target surface is multisampled (4x)
    *      - align_j = 4 for any render target surface with Surface Vertical
    *        Alignment = VALIGN_4
    *      - align_j = 2 for any render target surface with Surface Vertical
    *        Alignment = VALIGN_2
    *      - align_j = 2 for all other render target surface
    *      - align_j = 2 for any sampling engine surface with Surface Vertical
    *        Alignment = VALIGN_2
    *      - align_j = 4 for any sampling engine surface with Surface Vertical
    *        Alignment = VALIGN_4"
    *
    * From the Sandy Bridge PRM, volume 4 part 1, page 86:
    *
    *     "This field (Surface Vertical Alignment) must be set to VALIGN_2 if
    *      the Surface Format is 96 bits per element (BPE)."
    *
    * They can be rephrased as
    *
    *                                  align_i        align_j
    *   compressed formats             block width    block height
    *   PIPE_FORMAT_S8_UINT            4              2
    *   other depth/stencil formats    4              4
    *   4x multisampled                4              4
    *   bpp 96                         4              2
    *   others                         4              2 or 4
    */

   /*
    * From the Ivy Bridge PRM, volume 1 part 1, page 110:
    *
    *     "surface defined by      surface format     align_i     align_j
    *      3DSTATE_DEPTH_BUFFER    D16_UNORM          8           4
    *                              not D16_UNORM      4           4
    *      3DSTATE_STENCIL_BUFFER  N/A                8           8
    *      SURFACE_STATE           BC*, ETC*, EAC*    4           4
    *                              FXT1               8           4
    *                              all others         (set by SURFACE_STATE)"
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 63:
    *
    *     "- This field (Surface Vertical Aligment) is intended to be set to
    *        VALIGN_4 if the surface was rendered as a depth buffer, for a
    *        multisampled (4x) render target, or for a multisampled (8x)
    *        render target, since these surfaces support only alignment of 4.
    *      - Use of VALIGN_4 for other surfaces is supported, but uses more
    *        memory.
    *      - This field must be set to VALIGN_4 for all tiled Y Render Target
    *        surfaces.
    *      - Value of 1 is not supported for format YCRCB_NORMAL (0x182),
    *        YCRCB_SWAPUVY (0x183), YCRCB_SWAPUV (0x18f), YCRCB_SWAPY (0x190)
    *      - If Number of Multisamples is not MULTISAMPLECOUNT_1, this field
    *        must be set to VALIGN_4."
    *      - VALIGN_4 is not supported for surface format R32G32B32_FLOAT."
    *
    *     "- This field (Surface Horizontal Aligment) is intended to be set to
    *        HALIGN_8 only if the surface was rendered as a depth buffer with
    *        Z16 format or a stencil buffer, since these surfaces support only
    *        alignment of 8.
    *      - Use of HALIGN_8 for other surfaces is supported, but uses more
    *        memory.
    *      - This field must be set to HALIGN_4 if the Surface Format is BC*.
    *      - This field must be set to HALIGN_8 if the Surface Format is
    *        FXT1."
    *
    * They can be rephrased as
    *
    *                                  align_i        align_j
    *  compressed formats              block width    block height
    *  PIPE_FORMAT_Z16_UNORM           8              4
    *  PIPE_FORMAT_S8_UINT             8              8
    *  other depth/stencil formats     4 or 8         4
    *  2x or 4x multisampled           4 or 8         4
    *  tiled Y                         4 or 8         4 (if rt)
    *  PIPE_FORMAT_R32G32B32_FLOAT     4 or 8         2
    *  others                          4 or 8         2 or 4
    */

   if (layout->compressed) {
      /* this happens to be the case */
      layout->align_i = layout->block_width;
      layout->align_j = layout->block_height;
   }
   else if (layout->format.numericFormat == XGL_NUM_FMT_DS) {
      if (intel_gpu_gen(layout->gpu) >= INTEL_GEN(7)) {
         switch (layout->format.channelFormat) {
         case XGL_CH_FMT_R16:
            layout->align_i = 8;
            layout->align_j = 4;
            break;
         case XGL_CH_FMT_R8:
            layout->align_i = 8;
            layout->align_j = 8;
            break;
         default:
            layout->align_i = 4;
            layout->align_j = 4;
            break;
         }
      }
      else {
         switch (layout->format.channelFormat) {
         case XGL_CH_FMT_R8:
            layout->align_i = 4;
            layout->align_j = 2;
            break;
         default:
            layout->align_i = 4;
            layout->align_j = 4;
            break;
         }
      }
   }
   else {
      const bool valign_4 = (info->samples > 1) ||
         (intel_gpu_gen(layout->gpu) >= INTEL_GEN(7) &&
          layout->tiling == INTEL_TILING_Y &&
          (info->usage & XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT));

      if (valign_4)
         assert(layout->block_size != 12);

      layout->align_i = 4;
      layout->align_j = (valign_4) ? 4 : 2;
   }

   /*
    * the fact that align i and j are multiples of block width and height
    * respectively is what makes the size of the bo a multiple of the block
    * size, slices start at block boundaries, and many of the computations
    * work.
    */
   assert(layout->align_i % layout->block_width == 0);
   assert(layout->align_j % layout->block_height == 0);

   /* make sure u_align() works */
   assert(u_is_pow2(layout->align_i) &&
          u_is_pow2(layout->align_j));
   assert(u_is_pow2(layout->block_width) &&
          u_is_pow2(layout->block_height));
}

static void
tex_layout_init_levels(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;
   int last_level, lv;

   last_level = info->mipLevels - 1;

   /* need at least 2 levels to compute full qpitch */
   if (last_level == 0 && info->arraySize > 1 && layout->array_spacing_full)
      last_level++;

   /* compute mip level sizes */
   for (lv = 0; lv <= last_level; lv++) {
      int w, h, d;

      w = u_minify(info->extent.width, lv);
      h = u_minify(info->extent.height, lv);
      d = u_minify(info->extent.depth, lv);

      /*
       * From the Sandy Bridge PRM, volume 1 part 1, page 114:
       *
       *     "The dimensions of the mip maps are first determined by applying
       *      the sizing algorithm presented in Non-Power-of-Two Mipmaps
       *      above. Then, if necessary, they are padded out to compression
       *      block boundaries."
       */
      w = u_align(w, layout->block_width);
      h = u_align(h, layout->block_height);

      /*
       * From the Sandy Bridge PRM, volume 1 part 1, page 111:
       *
       *     "If the surface is multisampled (4x), these values must be
       *      adjusted as follows before proceeding:
       *
       *        W_L = ceiling(W_L / 2) * 4
       *        H_L = ceiling(H_L / 2) * 4"
       *
       * From the Ivy Bridge PRM, volume 1 part 1, page 108:
       *
       *     "If the surface is multisampled and it is a depth or stencil
       *      surface or Multisampled Surface StorageFormat in SURFACE_STATE
       *      is MSFMT_DEPTH_STENCIL, W_L and H_L must be adjusted as follows
       *      before proceeding:
       *
       *        #samples  W_L =                    H_L =
       *        2         ceiling(W_L / 2) * 4     HL [no adjustment]
       *        4         ceiling(W_L / 2) * 4     ceiling(H_L / 2) * 4
       *        8         ceiling(W_L / 2) * 8     ceiling(H_L / 2) * 4
       *        16        ceiling(W_L / 2) * 8     ceiling(H_L / 2) * 8"
       *
       * For interleaved samples (4x), where pixels
       *
       *   (x, y  ) (x+1, y  )
       *   (x, y+1) (x+1, y+1)
       *
       * would be is occupied by
       *
       *   (x, y  , si0) (x+1, y  , si0) (x, y  , si1) (x+1, y  , si1)
       *   (x, y+1, si0) (x+1, y+1, si0) (x, y+1, si1) (x+1, y+1, si1)
       *   (x, y  , si2) (x+1, y  , si2) (x, y  , si3) (x+1, y  , si3)
       *   (x, y+1, si2) (x+1, y+1, si2) (x, y+1, si3) (x+1, y+1, si3)
       *
       * Thus the need to
       *
       *   w = u_align(w, 2) * 2;
       *   y = u_align(y, 2) * 2;
       */
      if (layout->interleaved) {
         switch (info->samples) {
         case 0:
         case 1:
            break;
         case 2:
            w = u_align(w, 2) * 2;
            break;
         case 4:
            w = u_align(w, 2) * 2;
            h = u_align(h, 2) * 2;
            break;
         case 8:
            w = u_align(w, 2) * 4;
            h = u_align(h, 2) * 2;
            break;
         case 16:
            w = u_align(w, 2) * 4;
            h = u_align(h, 2) * 4;
            break;
         default:
            assert(!"unsupported sample count");
            break;
         }
      }

      layout->levels[lv].w = w;
      layout->levels[lv].h = h;
      layout->levels[lv].d = d;
   }
}

static void
tex_layout_init_spacing(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;

   if (intel_gpu_gen(layout->gpu) >= INTEL_GEN(7)) {
      /*
       * It is not explicitly states, but render targets are expected to be
       * UMS/CMS (samples non-interleaved) and depth/stencil buffers are
       * expected to be IMS (samples interleaved).
       *
       * See "Multisampled Surface Storage Format" field of SURFACE_STATE.
       */
      if (layout->has_depth || layout->has_stencil) {
         layout->interleaved = true;

         /*
          * From the Ivy Bridge PRM, volume 1 part 1, page 111:
          *
          *     "note that the depth buffer and stencil buffer have an implied
          *      value of ARYSPC_FULL"
          */
         layout->array_spacing_full = true;
      }
      else {
         layout->interleaved = false;

         /*
          * From the Ivy Bridge PRM, volume 4 part 1, page 66:
          *
          *     "If Multisampled Surface Storage Format is MSFMT_MSS and
          *      Number of Multisamples is not MULTISAMPLECOUNT_1, this field
          *      (Surface Array Spacing) must be set to ARYSPC_LOD0."
          *
          * As multisampled resources are not mipmapped, we never use
          * ARYSPC_FULL for them.
          */
         if (info->samples > 1)
            assert(info->mipLevels == 1);
         layout->array_spacing_full = (info->mipLevels > 1);
      }
   }
   else {
      /* GEN6 supports only interleaved samples */
      layout->interleaved = true;

      /*
       * From the Sandy Bridge PRM, volume 1 part 1, page 115:
       *
       *     "The separate stencil buffer does not support mip mapping, thus
       *      the storage for LODs other than LOD 0 is not needed. The
       *      following QPitch equation applies only to the separate stencil
       *      buffer:
       *
       *        QPitch = h_0"
       *
       * GEN6 does not support compact spacing otherwise.
       */
      layout->array_spacing_full =
          !(layout->format.channelFormat == XGL_CH_FMT_R8 &&
            layout->format.numericFormat == XGL_NUM_FMT_DS);
   }
}

static void
tex_layout_init_tiling(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;
   const unsigned tile_none = 1 << INTEL_TILING_NONE;
   const unsigned tile_x = 1 << INTEL_TILING_X;
   const unsigned tile_y = 1 << INTEL_TILING_Y;
   unsigned valid_tilings = tile_none | tile_x | tile_y;

   if (info->tiling == XGL_LINEAR_TILING)
      valid_tilings &= tile_none;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 318:
    *
    *     "[DevSNB+]: This field (Tiled Surface) must be set to TRUE. Linear
    *      Depth Buffer is not supported."
    *
    *     "The Depth Buffer, if tiled, must use Y-Major tiling."
    *
    * From the Sandy Bridge PRM, volume 1 part 2, page 22:
    *
    *     "W-Major Tile Format is used for separate stencil."
    *
    * Since the HW does not support W-tiled fencing, we have to do it in the
    * driver.
    */
   if (info->usage & XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT) {
       if (layout->format.channelFormat == XGL_CH_FMT_R8 &&
           layout->format.numericFormat == XGL_NUM_FMT_DS)
           valid_tilings &= tile_none;
       else
           valid_tilings &= tile_y;
   }

   if (info->usage & XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
      /*
       * From the Sandy Bridge PRM, volume 1 part 2, page 32:
       *
       *     "NOTE: 128BPE Format Color buffer ( render target ) MUST be
       *      either TileX or Linear."
       */
      if (layout->block_size == 16)
         valid_tilings &= ~tile_y;

      /*
       * From the Ivy Bridge PRM, volume 4 part 1, page 63:
       *
       *     "This field (Surface Vertical Aligment) must be set to VALIGN_4
       *      for all tiled Y Render Target surfaces."
       *
       *     "VALIGN_4 is not supported for surface format R32G32B32_FLOAT."
       */
      if (intel_gpu_gen(layout->gpu) >= INTEL_GEN(7) && layout->block_size == 12)
         valid_tilings &= ~tile_y;
   }

   /* no conflicting binding flags */
   assert(valid_tilings);

   layout->valid_tilings = valid_tilings;

   if (info->usage & (XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT)) {
      /*
       * heuristically set a minimum width/height for enabling tiling
       */
      if (info->extent.width < 64 && (valid_tilings & ~tile_x))
         valid_tilings &= ~tile_x;

      if ((info->extent.width < 32 || info->extent.height < 16) &&
          (info->extent.width < 16 || info->extent.height < 32) &&
          (valid_tilings & ~tile_y))
         valid_tilings &= ~tile_y;
   }
   else {
      /* force linear if we are not sure where the texture is bound to */
      if (valid_tilings & tile_none)
         valid_tilings &= tile_none;
   }

   /* prefer tiled over linear */
   if (valid_tilings & tile_y)
      layout->tiling = INTEL_TILING_Y;
   else if (valid_tilings & tile_x)
      layout->tiling = INTEL_TILING_X;
   else
      layout->tiling = INTEL_TILING_NONE;
}

static void
tex_layout_init_format(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;
   XGL_FORMAT format = info->format;

   if (format.numericFormat == XGL_NUM_FMT_DS) {
       switch (format.channelFormat) {
       case XGL_CH_FMT_R32G8:
           if (layout->separate_stencil)
               format.channelFormat = XGL_CH_FMT_R32;
           break;
       default:
           break;
       }
   }

   layout->format = format;

   switch (format.channelFormat) {
   case XGL_CH_FMT_BC1:
   case XGL_CH_FMT_BC2:
   case XGL_CH_FMT_BC3:
   case XGL_CH_FMT_BC4:
   case XGL_CH_FMT_BC5:
   case XGL_CH_FMT_BC6U:
   case XGL_CH_FMT_BC6S:
   case XGL_CH_FMT_BC7:
       layout->block_width = 4;
       layout->block_height = 4;
       layout->block_size =
           (format.channelFormat == XGL_CH_FMT_BC1 ||
            format.channelFormat == XGL_CH_FMT_BC4) ? 8 : 16;
       layout->compressed = true;
       break;
   default:
       layout->block_width = 1;
       layout->block_height = 1;
       layout->block_size = 1;
       layout->compressed = false;
       break;
   }
}

static void
tex_layout_init_hiz(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;

   if (info->format.numericFormat == XGL_NUM_FMT_DS) {
       switch (info->format.channelFormat) {
       case XGL_CH_FMT_R32G8:
           layout->has_depth = true;
           layout->has_stencil = true;
           break;
       case XGL_CH_FMT_R32:
           layout->has_depth = true;
           break;
       case XGL_CH_FMT_R8:
           layout->has_stencil = true;
           break;
       default:
           assert(!"unsupported DS format");
           break;
       }
   }

   if (!layout->has_depth)
      return;

   layout->hiz = true;

   if (intel_gpu_gen(layout->gpu) == INTEL_GEN(6)) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 312:
       *
       *     "The hierarchical depth buffer does not support the LOD field, it
       *      is assumed by hardware to be zero. A separate hierarachical
       *      depth buffer is required for each LOD used, and the
       *      corresponding buffer's state delivered to hardware each time a
       *      new depth buffer state with modified LOD is delivered."
       *
       * But we have a stronger requirement.  Because of layer offsetting
       * (check out the callers of ilo_texture_get_slice_offset()), we already
       * have to require the texture to be non-mipmapped and non-array.
       */
      if (info->mipLevels > 1 || info->arraySize > 1 || info->extent.depth > 1)
         layout->hiz = false;
   }

   if (layout->has_stencil) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 317:
       *
       *     "This field (Separate Stencil Buffer Enable) must be set to the
       *      same value (enabled or disabled) as Hierarchical Depth Buffer
       *      Enable."
       *
       * GEN7+ requires separate stencil buffers.
       */
      if (intel_gpu_gen(layout->gpu) >= INTEL_GEN(7))
         layout->separate_stencil = true;
      else
         layout->separate_stencil = layout->hiz;

      if (layout->separate_stencil)
         layout->has_stencil = false;
   }
}

static XGL_RESULT
tex_layout_init(struct tex_layout *layout,
                const struct intel_gpu *gpu,
                const XGL_IMAGE_CREATE_INFO *info,
                struct intel_img_slice **slices)
{
   memset(layout, 0, sizeof(*layout));

   layout->gpu = gpu;
   layout->info = info;

   /* note that there are dependencies between these functions */
   tex_layout_init_hiz(layout);
   tex_layout_init_format(layout);
   tex_layout_init_tiling(layout);
   tex_layout_init_spacing(layout);
   tex_layout_init_levels(layout);
   tex_layout_init_alignments(layout);
   tex_layout_init_qpitch(layout);

   if (slices) {
      int lv;

      for (lv = 0; lv < info->mipLevels; lv++)
         layout->levels[lv].slices = slices[lv];
   }

   return XGL_SUCCESS;
}

static void
tex_layout_align(struct tex_layout *layout)
{
   int align_w = 1, align_h = 1, pad_h = 0;

   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 118:
    *
    *     "To determine the necessary padding on the bottom and right side of
    *      the surface, refer to the table in Section 7.18.3.4 for the i and j
    *      parameters for the surface format in use. The surface must then be
    *      extended to the next multiple of the alignment unit size in each
    *      dimension, and all texels contained in this extended surface must
    *      have valid GTT entries."
    *
    *     "For cube surfaces, an additional two rows of padding are required
    *      at the bottom of the surface. This must be ensured regardless of
    *      whether the surface is stored tiled or linear.  This is due to the
    *      potential rotation of cache line orientation from memory to cache."
    *
    *     "For compressed textures (BC* and FXT1 surface formats), padding at
    *      the bottom of the surface is to an even compressed row, which is
    *      equal to a multiple of 8 uncompressed texel rows. Thus, for padding
    *      purposes, these surfaces behave as if j = 8 only for surface
    *      padding purposes. The value of 4 for j still applies for mip level
    *      alignment and QPitch calculation."
    */
   if (layout->info->usage & XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT) {
       if (align_w < layout->align_i)
           align_w = layout->align_i;
       if (align_h < layout->align_j)
           align_h = layout->align_j;

       /* in case it is used as a cube */
      if (layout->info->imageType == XGL_IMAGE_2D)
         pad_h += 2;

      if (layout->compressed && align_h < 8)
         align_h = 8;
   }

   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 118:
    *
    *     "If the surface contains an odd number of rows of data, a final row
    *      below the surface must be allocated."
    */
   if (layout->info->usage & XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
       if (align_h < 2)
           align_h = 2;
   }

   /*
    * Depth Buffer Clear/Resolve works in 8x4 sample blocks.  In
    * ilo_texture_can_enable_hiz(), we always return true for the first slice.
    * To avoid out-of-bound access, we have to pad.
    */
   if (layout->hiz) {
       if (align_w < 8)
           align_w = 8;
       if (align_h < 4)
           align_h = 4;
   }

   layout->width = u_align(layout->width, align_w);
   layout->height = u_align(layout->height + pad_h, align_h);
}

/**
 * Layout a 2D texture.
 */
static void
tex_layout_2d(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;
   unsigned int level_x, level_y, num_slices;
   int lv;

   level_x = 0;
   level_y = 0;
   for (lv = 0; lv < info->mipLevels; lv++) {
      const unsigned int level_w = layout->levels[lv].w;
      const unsigned int level_h = layout->levels[lv].h;
      int slice;

      /* set slice offsets */
      if (layout->levels[lv].slices) {
         for (slice = 0; slice < info->arraySize; slice++) {
            layout->levels[lv].slices[slice].x = level_x;
            /* slices are qpitch apart in Y-direction */
            layout->levels[lv].slices[slice].y =
               level_y + layout->qpitch * slice;
         }
      }

      /* extend the size of the monolithic bo to cover this mip level */
      if (layout->width < level_x + level_w)
         layout->width = level_x + level_w;
      if (layout->height < level_y + level_h)
         layout->height = level_y + level_h;

      /* MIPLAYOUT_BELOW */
      if (lv == 1)
         level_x += u_align(level_w, layout->align_i);
      else
         level_y += u_align(level_h, layout->align_j);
   }

   num_slices = info->arraySize;
   /* samples of the same index are stored in a slice */
   if (info->samples > 1 && !layout->interleaved)
      num_slices *= info->samples;

   /* we did not take slices into consideration in the computation above */
   layout->height += layout->qpitch * (num_slices - 1);

   tex_layout_align(layout);
}

/**
 * Layout a 3D texture.
 */
static void
tex_layout_3d(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;
   unsigned int level_y;
   int lv;

   level_y = 0;
   for (lv = 0; lv < info->mipLevels; lv++) {
      const unsigned int level_w = layout->levels[lv].w;
      const unsigned int level_h = layout->levels[lv].h;
      const unsigned int level_d = layout->levels[lv].d;
      const unsigned int slice_pitch = u_align(level_w, layout->align_i);
      const unsigned int slice_qpitch = u_align(level_h, layout->align_j);
      const unsigned int num_slices_per_row = 1 << lv;
      int slice;

      for (slice = 0; slice < level_d; slice += num_slices_per_row) {
         int i;

         /* set slice offsets */
         if (layout->levels[lv].slices) {
            for (i = 0; i < num_slices_per_row && slice + i < level_d; i++) {
               layout->levels[lv].slices[slice + i].x = slice_pitch * i;
               layout->levels[lv].slices[slice + i].y = level_y;
            }
         }

         /* move on to the next slice row */
         level_y += slice_qpitch;
      }

      /* rightmost slice */
      if (num_slices_per_row < level_d)
          slice = num_slices_per_row - 1;
      else
          slice = level_d - 1;

      /* extend the size of the monolithic bo to cover this slice */
      if (layout->width < slice_pitch * slice + level_w)
         layout->width = slice_pitch * slice + level_w;
      if (lv == info->mipLevels - 1)
         layout->height = (level_y - slice_qpitch) + level_h;
   }

   tex_layout_align(layout);
}

/* note that this may force the texture to be linear */
static bool
tex_layout_calculate_bo_size(struct tex_layout *layout)
{
   assert(layout->width % layout->block_width == 0);
   assert(layout->height % layout->block_height == 0);
   assert(layout->qpitch % layout->block_height == 0);

   layout->bo_stride =
      (layout->width / layout->block_width) * layout->block_size;
   layout->bo_height = layout->height / layout->block_height;

   while (true) {
      int w = layout->bo_stride, h = layout->bo_height;
      int align_w, align_h;

      /*
       * From the Haswell PRM, volume 5, page 163:
       *
       *     "For linear surfaces, additional padding of 64 bytes is required
       *      at the bottom of the surface. This is in addition to the padding
       *      required above."
       */
      if (intel_gpu_gen(layout->gpu) >= INTEL_GEN(7.5) &&
          (layout->info->usage & XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT) &&
          layout->tiling == INTEL_TILING_NONE) {
         layout->bo_height +=
            (64 + layout->bo_stride - 1) / layout->bo_stride;
      }

      /*
       * From the Sandy Bridge PRM, volume 4 part 1, page 81:
       *
       *     "- For linear render target surfaces, the pitch must be a
       *        multiple of the element size for non-YUV surface formats.
       *        Pitch must be a multiple of 2 * element size for YUV surface
       *        formats.
       *      - For other linear surfaces, the pitch can be any multiple of
       *        bytes.
       *      - For tiled surfaces, the pitch must be a multiple of the tile
       *        width."
       *
       * Different requirements may exist when the bo is used in different
       * places, but our alignments here should be good enough that we do not
       * need to check layout->info->usage.
       */
      switch (layout->tiling) {
      case INTEL_TILING_X:
         align_w = 512;
         align_h = 8;
         break;
      case INTEL_TILING_Y:
         align_w = 128;
         align_h = 32;
         break;
      default:
         if (layout->format.channelFormat == XGL_CH_FMT_R8 &&
             layout->format.numericFormat == XGL_NUM_FMT_DS) {
            /*
             * From the Sandy Bridge PRM, volume 1 part 2, page 22:
             *
             *     "A 4KB tile is subdivided into 8-high by 8-wide array of
             *      Blocks for W-Major Tiles (W Tiles). Each Block is 8 rows by 8
             *      bytes."
             *
             * Since we asked for INTEL_TILING_NONE instead of the non-existent
             * INTEL_TILING_W, we want to align to W tiles here.
             */
            align_w = 64;
            align_h = 64;
         }
         else {
            /* some good enough values */
            align_w = 64;
            align_h = 2;
         }
         break;
      }

      w = u_align(w, align_w);
      h = u_align(h, align_h);

      /* make sure the bo is mappable */
      if (layout->tiling != INTEL_TILING_NONE) {
         /*
          * Usually only the first 256MB of the GTT is mappable.
          *
          * See also how intel_context::max_gtt_map_object_size is calculated.
          */
         const size_t mappable_gtt_size = 256 * 1024 * 1024;

         /*
          * Be conservative.  We may be able to switch from VALIGN_4 to
          * VALIGN_2 if the layout was Y-tiled, but let's keep it simple.
          */
         if (mappable_gtt_size / w / 4 < h) {
            if (layout->valid_tilings & (1 << INTEL_TILING_NONE)) {
               layout->tiling = INTEL_TILING_NONE;
               continue;
            }
            else {
               /* send a warning? */
            }
         }
      }

      layout->bo_stride = w;
      layout->bo_height = h;
      break;
   }

   return (layout->bo_height <= intel_max_resource_size / layout->bo_stride);
}

static void
tex_layout_calculate_hiz_size(struct tex_layout *layout)
{
   const XGL_IMAGE_CREATE_INFO *info = layout->info;
   const int hz_align_j = 8;
   int hz_width, hz_height;

   if (!layout->hiz)
      return;

   /*
    * See the Sandy Bridge PRM, volume 2 part 1, page 312, and the Ivy Bridge
    * PRM, volume 2 part 1, page 312-313.
    *
    * It seems HiZ buffer is aligned to 8x8, with every two rows packed into a
    * memory row.
    */

   hz_width = u_align(layout->levels[0].w, 16);

   if (info->imageType == XGL_IMAGE_3D) {
      unsigned lv;

      hz_height = 0;

      for (lv = 0; lv < info->mipLevels; lv++) {
         const unsigned h = u_align(layout->levels[lv].h, hz_align_j);
         hz_height += h * layout->levels[lv].d;
      }

      hz_height /= 2;
   }
   else {
      const unsigned h0 = u_align(layout->levels[0].h, hz_align_j);
      unsigned hz_qpitch = h0;

      if (layout->array_spacing_full) {
         const unsigned h1 = u_align(layout->levels[1].h, hz_align_j);
         const unsigned htail =
            ((intel_gpu_gen(layout->gpu) >= INTEL_GEN(7)) ? 12 : 11) * hz_align_j;

         hz_qpitch += h1 + htail;
      }

      hz_height = hz_qpitch * info->arraySize / 2;

      if (intel_gpu_gen(layout->gpu) >= INTEL_GEN(7))
         hz_height = u_align(hz_height, 8);
   }

   /* align to Y-tile */
   layout->hiz_stride = u_align(hz_width, 128);
   layout->hiz_height = u_align(hz_height, 32);
}

static bool
img_alloc_slices(struct intel_img *img,
                 XGL_UINT levels, XGL_INT depth,
                 XGL_UINT array_size)
{
   struct intel_img_slice *slices;
   int total_depth, lv;

   /* sum the depths of all levels */
   total_depth = 0;
   for (lv = 0; lv < levels; lv++)
      total_depth += u_minify(depth, lv);

   /*
    * There are (depth * tex->base.array_size) slices in total.  Either depth
    * is one (non-3D) or templ->array_size is one (non-array), but it does
    * not matter.
    */
   slices = icd_alloc(sizeof(*slices) * total_depth * array_size,
           0, XGL_SYSTEM_ALLOC_INTERNAL);
   if (!slices)
      return false;

   img->slices[0] = slices;

   /* point to the respective positions in the buffer */
   for (lv = 1; lv < levels; lv++) {
      img->slices[lv] = img->slices[lv - 1] +
         u_minify(depth, lv - 1) * array_size;
   }

   return true;
}

static void img_destroy(struct intel_obj *obj)
{
    struct intel_img *img = intel_img_from_obj(obj);

    intel_img_destroy(img);
}

static XGL_RESULT img_get_info(struct intel_base *base, int type,
                               XGL_SIZE *size, XGL_VOID *data)
{
    struct intel_img *img = intel_img_from_base(base);
    XGL_RESULT ret = XGL_SUCCESS;

    switch (type) {
    case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
        {
            XGL_MEMORY_REQUIREMENTS *mem_req = data;

            mem_req->size = img->bo_stride * img->bo_height;
            mem_req->alignment = 4096;
            mem_req->heapCount = 1;
            mem_req->heaps[0] = 0;

            *size = sizeof(*mem_req);
        }
        break;
    default:
        ret = intel_base_get_info(base, type, size, data);
        break;
    }

    return ret;
}

XGL_RESULT intel_img_create(struct intel_dev *dev,
                            const XGL_IMAGE_CREATE_INFO *info,
                            struct intel_img **img_ret)
{
    struct tex_layout layout;
    struct intel_img *img;
    XGL_RESULT ret;

    img = (struct intel_img *) intel_base_create(dev, sizeof(*img),
            dev->base.dbg, XGL_DBG_OBJECT_IMAGE, info, 0);
    if (!img)
        return XGL_ERROR_OUT_OF_MEMORY;

    if (!img_alloc_slices(img, info->mipLevels, info->extent.depth,
                info->arraySize)) {
        intel_img_destroy(img);
        return XGL_ERROR_OUT_OF_MEMORY;
    }

    ret = tex_layout_init(&layout, dev->gpu, info, img->slices);
    if (ret != XGL_SUCCESS) {
        intel_img_destroy(img);
        return ret;
    }

    if (info->imageType == XGL_IMAGE_3D)
        tex_layout_3d(&layout);
    else
        tex_layout_2d(&layout);

    if (!tex_layout_calculate_bo_size(&layout)) {
        intel_img_destroy(img);
        return XGL_ERROR_INVALID_MEMORY_SIZE;
    }

    tex_layout_calculate_hiz_size(&layout);

    /* TODO */
    if (layout.hiz || layout.separate_stencil) {
        intel_dev_log(dev, XGL_DBG_MSG_ERROR, XGL_VALIDATION_LEVEL_0,
                XGL_NULL_HANDLE, 0, 0, "HiZ or separate stencil enabled");
        intel_img_destroy(img);
        return XGL_ERROR_INVALID_MEMORY_SIZE;
    }

    img->bo_format = layout.format;
    img->tiling = layout.tiling;
    img->bo_stride = layout.bo_stride;
    img->bo_height = layout.bo_height;
    img->block_width = layout.block_width;
    img->block_height = layout.block_height;
    img->block_size = layout.block_size;
    img->halign_8 = (layout.align_i == 8);
    img->valign_4 = (layout.align_j == 4);
    img->array_spacing_full = layout.array_spacing_full;
    img->interleaved = layout.interleaved;

    img->obj.destroy = img_destroy;
    img->obj.base.get_info = img_get_info;

    *img_ret = img;

    return XGL_SUCCESS;
}

void intel_img_destroy(struct intel_img *img)
{
    if (img->slices[0])
        icd_free(img->slices[0]);

    intel_base_destroy(&img->obj.base);
}

XGL_RESULT XGLAPI intelCreateImage(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
    XGL_IMAGE*                                  pImage)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_img_create(dev, pCreateInfo, (struct intel_img **) pImage);
}

XGL_RESULT XGLAPI intelGetImageSubresourceInfo(
    XGL_IMAGE                                   image,
    const XGL_IMAGE_SUBRESOURCE*                pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE                   infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    const struct intel_img *img = intel_img(image);
    XGL_RESULT ret = XGL_SUCCESS;

    switch (infoType) {
    case XGL_INFO_TYPE_SUBRESOURCE_LAYOUT:
        {
            XGL_SUBRESOURCE_LAYOUT *layout = (XGL_SUBRESOURCE_LAYOUT *) pData;
            const struct intel_img_slice *slice =
                &img->slices[pSubresource->mipLevel][pSubresource->arraySlice];
            const unsigned int bx = slice->x / img->block_width;
            const unsigned int by = slice->y / img->block_height;

            *pDataSize = sizeof(XGL_SUBRESOURCE_LAYOUT);

            /*
             * size is not readily available and depthPitch might not be
             * available.  Leave them alone for now.
             */
            layout->offset = by * img->bo_stride + bx * img->block_size;
            layout->size = 0;
            layout->rowPitch = img->bo_stride;
            layout->depthPitch = 0;
        }
        break;
    default:
        ret = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}
