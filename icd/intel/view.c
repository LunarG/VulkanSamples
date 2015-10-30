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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chia-I Wu <olv@lunarg.com>
 *
 */

#include "genhw/genhw.h"
#include "kmd/winsys.h"
#include "buf.h"
#include "dev.h"
#include "format.h"
#include "gpu.h"
#include "img.h"
#include "mem.h"
#include "view.h"

static void surface_state_null_gen7(const struct intel_gpu *gpu,
                                    uint32_t dw[8])
{
   INTEL_GPU_ASSERT(gpu, 7, 7.5);

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 62:
    *
    *     "A null surface is used in instances where an actual surface is not
    *      bound. When a write message is generated to a null surface, no
    *      actual surface is written to. When a read message (including any
    *      sampling engine message) is generated to a null surface, the result
    *      is all zeros.  Note that a null surface type is allowed to be used
    *      with all messages, even if it is not specificially indicated as
    *      supported. All of the remaining fields in surface state are ignored
    *      for null surfaces, with the following exceptions:
    *
    *      * Width, Height, Depth, LOD, and Render Target View Extent fields
    *        must match the depth buffer's corresponding state for all render
    *        target surfaces, including null.
    *      * All sampling engine and data port messages support null surfaces
    *        with the above behavior, even if not mentioned as specifically
    *        supported, except for the following:
    *        * Data Port Media Block Read/Write messages.
    *      * The Surface Type of a surface used as a render target (accessed
    *        via the Data Port's Render Target Write message) must be the same
    *        as the Surface Type of all other render targets and of the depth
    *        buffer (defined in 3DSTATE_DEPTH_BUFFER), unless either the depth
    *        buffer or render targets are SURFTYPE_NULL."
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 65:
    *
    *     "If Surface Type is SURFTYPE_NULL, this field (Tiled Surface) must be
    *      true"
    */

   dw[0] = GEN6_SURFTYPE_NULL << GEN7_SURFACE_DW0_TYPE__SHIFT |
           GEN6_FORMAT_B8G8R8A8_UNORM << GEN7_SURFACE_DW0_FORMAT__SHIFT |
           GEN6_TILING_X << 13;

   dw[1] = 0;
   dw[2] = 0;
   dw[3] = 0;
   dw[4] = 0;
   dw[5] = 0;
   dw[6] = 0;
   dw[7] = 0;
}

static void surface_state_buf_gen7(const struct intel_gpu *gpu,
                                   unsigned offset, unsigned size,
                                   unsigned struct_size,
                                   VkFormat elem_format,
                                   bool is_rt, bool render_cache_rw,
                                   uint32_t dw[8])
{
   const bool typed = !icd_format_is_undef(elem_format);
   const bool structured = (!typed && struct_size > 1);
   const int elem_size = (typed) ?
      icd_format_get_size(elem_format) : 1;
   int width, height, depth, pitch;
   int surface_type, surface_format, num_entries;

   INTEL_GPU_ASSERT(gpu, 7, 7.5);

   surface_type = (structured) ? GEN7_SURFTYPE_STRBUF : GEN6_SURFTYPE_BUFFER;

   surface_format = (typed) ?
      intel_format_translate_color(gpu, elem_format) : GEN6_FORMAT_RAW;

   /*
    * It's possible that the buffer view being used is smaller than
    * the format element size (required to be 16 for non-fragment shaders)
    * Make certain that size is at least struct_size to keep HW happy.
    */
   if (size < struct_size) {
       size = struct_size;
   }

   num_entries = size / struct_size;
   /* see if there is enough space to fit another element */
   if (size % struct_size >= elem_size && !structured)
      num_entries++;

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 67:
    *
    *     "For SURFTYPE_BUFFER render targets, this field (Surface Base
    *      Address) specifies the base address of first element of the
    *      surface. The surface is interpreted as a simple array of that
    *      single element type. The address must be naturally-aligned to the
    *      element size (e.g., a buffer containing R32G32B32A32_FLOAT elements
    *      must be 16-byte aligned)
    *
    *      For SURFTYPE_BUFFER non-rendertarget surfaces, this field specifies
    *      the base address of the first element of the surface, computed in
    *      software by adding the surface base address to the byte offset of
    *      the element in the buffer."
    */
   if (is_rt)
      assert(offset % elem_size == 0);

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 68:
    *
    *     "For typed buffer and structured buffer surfaces, the number of
    *      entries in the buffer ranges from 1 to 2^27.  For raw buffer
    *      surfaces, the number of entries in the buffer is the number of
    *      bytes which can range from 1 to 2^30."
    */
   assert(num_entries >= 1 &&
          num_entries <= 1 << ((typed || structured) ? 27 : 30));

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 69:
    *
    *     "For SURFTYPE_BUFFER: The low two bits of this field (Width) must be
    *      11 if the Surface Format is RAW (the size of the buffer must be a
    *      multiple of 4 bytes)."
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 70:
    *
    *     "For surfaces of type SURFTYPE_BUFFER and SURFTYPE_STRBUF, this
    *      field (Surface Pitch) indicates the size of the structure."
    *
    *     "For linear surfaces with Surface Type of SURFTYPE_STRBUF, the pitch
    *      must be a multiple of 4 bytes."
    */
   if (structured)
      assert(struct_size % 4 == 0);
   else if (!typed)
      assert(num_entries % 4 == 0);

   pitch = struct_size;

   pitch--;
   num_entries--;
   /* bits [6:0] */
   width  = (num_entries & 0x0000007f);
   /* bits [20:7] */
   height = (num_entries & 0x001fff80) >> 7;
   /* bits [30:21] */
   depth  = (num_entries & 0x7fe00000) >> 21;
   /* limit to [26:21] */
   if (typed || structured)
      depth &= 0x3f;

   dw[0] = surface_type << GEN7_SURFACE_DW0_TYPE__SHIFT |
           surface_format << GEN7_SURFACE_DW0_FORMAT__SHIFT;
   if (render_cache_rw)
      dw[0] |= GEN7_SURFACE_DW0_RENDER_CACHE_RW;

   dw[1] = offset;

   dw[2] = height << GEN7_SURFACE_DW2_HEIGHT__SHIFT |
           width << GEN7_SURFACE_DW2_WIDTH__SHIFT;

   dw[3] = depth << GEN7_SURFACE_DW3_DEPTH__SHIFT |
           pitch;

   dw[4] = 0;
   dw[5] = GEN7_MOCS_L3_WB << GEN7_SURFACE_DW5_MOCS__SHIFT;

   dw[6] = 0;
   dw[7] = 0;

   if (intel_gpu_gen(gpu) >= INTEL_GEN(7.5)) {
      dw[7] |= GEN75_SCS_RED   << GEN75_SURFACE_DW7_SCS_R__SHIFT |
               GEN75_SCS_GREEN << GEN75_SURFACE_DW7_SCS_G__SHIFT |
               GEN75_SCS_BLUE  << GEN75_SURFACE_DW7_SCS_B__SHIFT |
               GEN75_SCS_ALPHA << GEN75_SURFACE_DW7_SCS_A__SHIFT;
   }
}

static int img_type_to_view_type(VkImageType type, unsigned first_layer, unsigned num_layers)
{
    if (first_layer == 0 && num_layers == 1) {
        switch (type) {
        case VK_IMAGE_TYPE_1D:   return VK_IMAGE_VIEW_TYPE_1D;
        case VK_IMAGE_TYPE_2D:   return VK_IMAGE_VIEW_TYPE_2D;
        case VK_IMAGE_TYPE_3D:   return VK_IMAGE_VIEW_TYPE_3D;
        default: assert(!"unknown img type"); return VK_IMAGE_VIEW_TYPE_1D;
        }
    } else {
        switch (type) {
        case VK_IMAGE_TYPE_1D:   return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        case VK_IMAGE_TYPE_2D:   return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case VK_IMAGE_TYPE_3D:   return VK_IMAGE_VIEW_TYPE_3D;
        default: assert(!"unknown img type"); return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
        }
    }
}

static int view_type_to_surface_type(VkImageViewType type)
{
    switch (type) {
    case VK_IMAGE_VIEW_TYPE_1D:         return GEN6_SURFTYPE_1D;
    case VK_IMAGE_VIEW_TYPE_1D_ARRAY:   return GEN6_SURFTYPE_1D;
    case VK_IMAGE_VIEW_TYPE_2D:         return GEN6_SURFTYPE_2D;
    case VK_IMAGE_VIEW_TYPE_2D_ARRAY:   return GEN6_SURFTYPE_2D;
    case VK_IMAGE_VIEW_TYPE_3D:         return GEN6_SURFTYPE_3D;
    case VK_IMAGE_VIEW_TYPE_CUBE:       return GEN6_SURFTYPE_CUBE;
    case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY: return GEN6_SURFTYPE_CUBE;
    default: assert(!"unknown view type"); return GEN6_SURFTYPE_NULL;
    }
}

static int channel_swizzle_to_scs(VkComponentSwizzle swizzle)
{
    switch (swizzle) {
    case VK_COMPONENT_SWIZZLE_ZERO:  return GEN75_SCS_ZERO;
    case VK_COMPONENT_SWIZZLE_ONE:   return GEN75_SCS_ONE;
    case VK_COMPONENT_SWIZZLE_R:     return GEN75_SCS_RED;
    case VK_COMPONENT_SWIZZLE_G:     return GEN75_SCS_GREEN;
    case VK_COMPONENT_SWIZZLE_B:     return GEN75_SCS_BLUE;
    case VK_COMPONENT_SWIZZLE_A:     return GEN75_SCS_ALPHA;
    default: assert(!"unknown swizzle"); return GEN75_SCS_ZERO;
    }
}

static void surface_state_tex_gen7(const struct intel_gpu *gpu,
                                   const struct intel_img *img,
                                   VkImageViewType type,
                                   VkFormat format,
                                   unsigned first_level,
                                   unsigned num_levels,
                                   unsigned first_layer,
                                   unsigned num_layers,
                                   VkComponentMapping swizzles,
                                   bool is_rt,
                                   uint32_t dw[8])
{
   int surface_type, surface_format;
   int width, height, depth, pitch, lod;

   INTEL_GPU_ASSERT(gpu, 7, 7.5);

   surface_type = view_type_to_surface_type(type);
   assert(surface_type != GEN6_SURFTYPE_BUFFER);

   surface_format = intel_format_translate_color(gpu, format);
   assert(surface_format >= 0);

   width = img->layout.width0;
   height = img->layout.height0;
   depth = (type == VK_IMAGE_VIEW_TYPE_3D) ?
      img->depth : num_layers;
   pitch = img->layout.bo_stride;

   if (surface_type == GEN6_SURFTYPE_CUBE) {
      /*
       * From the Ivy Bridge PRM, volume 4 part 1, page 70:
       *
       *     "For SURFTYPE_CUBE:For Sampling Engine Surfaces, the range of
       *      this field is [0,340], indicating the number of cube array
       *      elements (equal to the number of underlying 2D array elements
       *      divided by 6). For other surfaces, this field must be zero."
       *
       * When is_rt is true, we treat the texture as a 2D one to avoid the
       * restriction.
       */
      if (is_rt) {
         surface_type = GEN6_SURFTYPE_2D;
      }
      else {
         assert(num_layers % 6 == 0);
         depth = num_layers / 6;
      }
   }

   /* sanity check the size */
   assert(width >= 1 && height >= 1 && depth >= 1 && pitch >= 1);
   assert(first_layer < 2048 && num_layers <= 2048);
   switch (surface_type) {
   case GEN6_SURFTYPE_1D:
      assert(width <= 16384 && height == 1 && depth <= 2048);
      break;
   case GEN6_SURFTYPE_2D:
      assert(width <= 16384 && height <= 16384 && depth <= 2048);
      break;
   case GEN6_SURFTYPE_3D:
      assert(width <= 2048 && height <= 2048 && depth <= 2048);
      if (!is_rt)
         assert(first_layer == 0);
      break;
   case GEN6_SURFTYPE_CUBE:
      assert(width <= 16384 && height <= 16384 && depth <= 86);
      assert(width == height);
      if (is_rt)
         assert(first_layer == 0);
      break;
   default:
      assert(!"unexpected surface type");
      break;
   }

   if (is_rt) {
      assert(num_levels == 1);
      lod = first_level;
   }
   else {
      lod = num_levels - 1;
   }

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 68:
    *
    *     "The Base Address for linear render target surfaces and surfaces
    *      accessed with the typed surface read/write data port messages must
    *      be element-size aligned, for non-YUV surface formats, or a multiple
    *      of 2 element-sizes for YUV surface formats.  Other linear surfaces
    *      have no alignment requirements (byte alignment is sufficient)."
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 70:
    *
    *     "For linear render target surfaces and surfaces accessed with the
    *      typed data port messages, the pitch must be a multiple of the
    *      element size for non-YUV surface formats. Pitch must be a multiple
    *      of 2 * element size for YUV surface formats. For linear surfaces
    *      with Surface Type of SURFTYPE_STRBUF, the pitch must be a multiple
    *      of 4 bytes.For other linear surfaces, the pitch can be any multiple
    *      of bytes."
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 74:
    *
    *     "For linear surfaces, this field (X Offset) must be zero."
    */
   if (img->layout.tiling == GEN6_TILING_NONE) {
      if (is_rt) {
         const int elem_size U_ASSERT_ONLY = icd_format_get_size(format);
         assert(pitch % elem_size == 0);
      }
   }

   assert(img->layout.tiling != GEN8_TILING_W);
   dw[0] = surface_type << GEN7_SURFACE_DW0_TYPE__SHIFT |
           surface_format << GEN7_SURFACE_DW0_FORMAT__SHIFT |
           img->layout.tiling << 13;

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 63:
    *
    *     "If this field (Surface Array) is enabled, the Surface Type must be
    *      SURFTYPE_1D, SURFTYPE_2D, or SURFTYPE_CUBE. If this field is
    *      disabled and Surface Type is SURFTYPE_1D, SURFTYPE_2D, or
    *      SURFTYPE_CUBE, the Depth field must be set to zero."
    *
    * For non-3D sampler surfaces, resinfo (the sampler message) always
    * returns zero for the number of layers when this field is not set.
    */
   if (surface_type != GEN6_SURFTYPE_3D) {
      if (num_layers > 1)
         dw[0] |= GEN7_SURFACE_DW0_IS_ARRAY;
      else
         assert(depth == 1);
   }

   assert(img->layout.align_i == 4 || img->layout.align_i == 8);
   assert(img->layout.align_j == 2 || img->layout.align_j == 4);

   if (img->layout.align_j == 4)
      dw[0] |= GEN7_SURFACE_DW0_VALIGN_4;

   if (img->layout.align_i == 8)
      dw[0] |= GEN7_SURFACE_DW0_HALIGN_8;

   if (img->layout.walk == INTEL_LAYOUT_WALK_LOD)
      dw[0] |= GEN7_SURFACE_DW0_ARYSPC_LOD0;
   else
      dw[0] |= GEN7_SURFACE_DW0_ARYSPC_FULL;

   if (is_rt)
      dw[0] |= GEN7_SURFACE_DW0_RENDER_CACHE_RW;

   if (surface_type == GEN6_SURFTYPE_CUBE && !is_rt)
      dw[0] |= GEN7_SURFACE_DW0_CUBE_FACE_ENABLES__MASK;

   dw[1] = 0;

   dw[2] = (height - 1) << GEN7_SURFACE_DW2_HEIGHT__SHIFT |
           (width - 1) << GEN7_SURFACE_DW2_WIDTH__SHIFT;

   dw[3] = (depth - 1) << GEN7_SURFACE_DW3_DEPTH__SHIFT |
           (pitch - 1);

   dw[4] = first_layer << 18 |
           (num_layers - 1) << 7;

   /*
    * MSFMT_MSS means the samples are not interleaved and MSFMT_DEPTH_STENCIL
    * means the samples are interleaved.  The layouts are the same when the
    * number of samples is 1.
    */
   if (img->layout.interleaved_samples && img->sample_count > 1) {
      assert(!is_rt);
      dw[4] |= GEN7_SURFACE_DW4_MSFMT_DEPTH_STENCIL;
   }
   else {
      dw[4] |= GEN7_SURFACE_DW4_MSFMT_MSS;
   }

   if (img->sample_count > 4)
      dw[4] |= GEN7_SURFACE_DW4_MULTISAMPLECOUNT_8;
   else if (img->sample_count > 2)
      dw[4] |= GEN7_SURFACE_DW4_MULTISAMPLECOUNT_4;
   else
      dw[4] |= GEN7_SURFACE_DW4_MULTISAMPLECOUNT_1;

   dw[5] = GEN7_MOCS_L3_WB << GEN7_SURFACE_DW5_MOCS__SHIFT |
           (first_level) << GEN7_SURFACE_DW5_MIN_LOD__SHIFT |
           lod;

   dw[6] = 0;
   dw[7] = 0;

   if (intel_gpu_gen(gpu) >= INTEL_GEN(7.5)) {
      dw[7] |=
          channel_swizzle_to_scs(swizzles.r) << GEN75_SURFACE_DW7_SCS_R__SHIFT |
          channel_swizzle_to_scs(swizzles.g) << GEN75_SURFACE_DW7_SCS_G__SHIFT |
          channel_swizzle_to_scs(swizzles.b) << GEN75_SURFACE_DW7_SCS_B__SHIFT |
          channel_swizzle_to_scs(swizzles.a) << GEN75_SURFACE_DW7_SCS_A__SHIFT;
   } else {
        assert(swizzles.r == VK_COMPONENT_SWIZZLE_R &&
               swizzles.g == VK_COMPONENT_SWIZZLE_G &&
               swizzles.b == VK_COMPONENT_SWIZZLE_B &&
               swizzles.a == VK_COMPONENT_SWIZZLE_A);
   }
}

static void surface_state_null_gen6(const struct intel_gpu *gpu,
                                    uint32_t dw[6])
{
   INTEL_GPU_ASSERT(gpu, 6, 6);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 71:
    *
    *     "A null surface will be used in instances where an actual surface is
    *      not bound. When a write message is generated to a null surface, no
    *      actual surface is written to. When a read message (including any
    *      sampling engine message) is generated to a null surface, the result
    *      is all zeros. Note that a null surface type is allowed to be used
    *      with all messages, even if it is not specificially indicated as
    *      supported. All of the remaining fields in surface state are ignored
    *      for null surfaces, with the following exceptions:
    *
    *        * [DevSNB+]: Width, Height, Depth, and LOD fields must match the
    *          depth buffer's corresponding state for all render target
    *          surfaces, including null.
    *        * Surface Format must be R8G8B8A8_UNORM."
    *
    * From the Sandy Bridge PRM, volume 4 part 1, page 82:
    *
    *     "If Surface Type is SURFTYPE_NULL, this field (Tiled Surface) must be
    *      true"
    */

   dw[0] = GEN6_SURFTYPE_NULL << GEN6_SURFACE_DW0_TYPE__SHIFT |
           GEN6_FORMAT_B8G8R8A8_UNORM << GEN6_SURFACE_DW0_FORMAT__SHIFT;

   dw[1] = 0;
   dw[2] = 0;
   dw[3] = GEN6_TILING_X;
   dw[4] = 0;
   dw[5] = 0;
}

static void surface_state_buf_gen6(const struct intel_gpu *gpu,
                                   unsigned offset, unsigned size,
                                   unsigned struct_size,
                                   VkFormat elem_format,
                                   bool is_rt, bool render_cache_rw,
                                   uint32_t dw[6])
{
   const bool typed = !icd_format_is_undef(elem_format);
   const int elem_size = icd_format_get_size(elem_format);
   int width, height, depth, pitch;
   int surface_format, num_entries;

   INTEL_GPU_ASSERT(gpu, 6, 6);

   /*
    * For SURFTYPE_BUFFER, a SURFACE_STATE specifies an element of a
    * structure in a buffer.
    */

   surface_format = (typed) ?
       intel_format_translate_color(gpu, elem_format) : GEN6_FORMAT_RAW;

   num_entries = size / struct_size;
   /* see if there is enough space to fit another element */
   if (size % struct_size >= elem_size)
      num_entries++;

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 76:
    *
    *     "For SURFTYPE_BUFFER render targets, this field (Surface Base
    *      Address) specifies the base address of first element of the
    *      surface. The surface is interpreted as a simple array of that
    *      single element type. The address must be naturally-aligned to the
    *      element size (e.g., a buffer containing R32G32B32A32_FLOAT elements
    *      must be 16-byte aligned).
    *
    *      For SURFTYPE_BUFFER non-rendertarget surfaces, this field specifies
    *      the base address of the first element of the surface, computed in
    *      software by adding the surface base address to the byte offset of
    *      the element in the buffer."
    */
   if (is_rt)
      assert(offset % elem_size == 0);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 77:
    *
    *     "For buffer surfaces, the number of entries in the buffer ranges
    *      from 1 to 2^27."
    */
   assert(num_entries >= 1 && num_entries <= 1 << 27);

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 81:
    *
    *     "For surfaces of type SURFTYPE_BUFFER, this field (Surface Pitch)
    *      indicates the size of the structure."
    */
   pitch = struct_size;

   pitch--;
   num_entries--;
   /* bits [6:0] */
   width  = (num_entries & 0x0000007f);
   /* bits [19:7] */
   height = (num_entries & 0x000fff80) >> 7;
   /* bits [26:20] */
   depth  = (num_entries & 0x07f00000) >> 20;

   dw[0] = GEN6_SURFTYPE_BUFFER << GEN6_SURFACE_DW0_TYPE__SHIFT |
           surface_format << GEN6_SURFACE_DW0_FORMAT__SHIFT;
   if (render_cache_rw)
      dw[0] |= GEN6_SURFACE_DW0_RENDER_CACHE_RW;

   dw[1] = offset;

   dw[2] = height << GEN6_SURFACE_DW2_HEIGHT__SHIFT |
           width << GEN6_SURFACE_DW2_WIDTH__SHIFT;

   dw[3] = depth << GEN6_SURFACE_DW3_DEPTH__SHIFT |
           pitch << GEN6_SURFACE_DW3_PITCH__SHIFT;

   dw[4] = 0;
   dw[5] = 0;
}

static void surface_state_tex_gen6(const struct intel_gpu *gpu,
                                   const struct intel_img *img,
                                   VkImageViewType type,
                                   VkFormat format,
                                   unsigned first_level,
                                   unsigned num_levels,
                                   unsigned first_layer,
                                   unsigned num_layers,
                                   bool is_rt,
                                   uint32_t dw[6])
{
   int surface_type, surface_format;
   int width, height, depth, pitch, lod;

   INTEL_GPU_ASSERT(gpu, 6, 6);

   surface_type = view_type_to_surface_type(type);
   assert(surface_type != GEN6_SURFTYPE_BUFFER);

   surface_format = intel_format_translate_color(gpu, format);
   assert(surface_format >= 0);

   width = img->layout.width0;
   height = img->layout.height0;
   depth = (type == VK_IMAGE_VIEW_TYPE_3D) ?
      img->depth : num_layers;
   pitch = img->layout.bo_stride;

   if (surface_type == GEN6_SURFTYPE_CUBE) {
      /*
       * From the Sandy Bridge PRM, volume 4 part 1, page 81:
       *
       *     "For SURFTYPE_CUBE: [DevSNB+]: for Sampling Engine Surfaces, the
       *      range of this field (Depth) is [0,84], indicating the number of
       *      cube array elements (equal to the number of underlying 2D array
       *      elements divided by 6). For other surfaces, this field must be
       *      zero."
       *
       * When is_rt is true, we treat the texture as a 2D one to avoid the
       * restriction.
       */
      if (is_rt) {
         surface_type = GEN6_SURFTYPE_2D;
      }
      else {
         assert(num_layers % 6 == 0);
         depth = num_layers / 6;
      }
   }

   /* sanity check the size */
   assert(width >= 1 && height >= 1 && depth >= 1 && pitch >= 1);
   switch (surface_type) {
   case GEN6_SURFTYPE_1D:
      assert(width <= 8192 && height == 1 && depth <= 512);
      assert(first_layer < 512 && num_layers <= 512);
      break;
   case GEN6_SURFTYPE_2D:
      assert(width <= 8192 && height <= 8192 && depth <= 512);
      assert(first_layer < 512 && num_layers <= 512);
      break;
   case GEN6_SURFTYPE_3D:
      assert(width <= 2048 && height <= 2048 && depth <= 2048);
      assert(first_layer < 2048 && num_layers <= 512);
      if (!is_rt)
         assert(first_layer == 0);
      break;
   case GEN6_SURFTYPE_CUBE:
      assert(width <= 8192 && height <= 8192 && depth <= 85);
      assert(width == height);
      assert(first_layer < 512 && num_layers <= 512);
      if (is_rt)
         assert(first_layer == 0);
      break;
   default:
      assert(!"unexpected surface type");
      break;
   }

   /* non-full array spacing is supported only on GEN7+ */
   assert(img->layout.walk != INTEL_LAYOUT_WALK_LOD);
   /* non-interleaved samples are supported only on GEN7+ */
   if (img->sample_count > 1)
      assert(img->layout.interleaved_samples);

   if (is_rt) {
      assert(num_levels == 1);
      lod = first_level;
   }
   else {
      lod = num_levels - 1;
   }

   /*
    * From the Sandy Bridge PRM, volume 4 part 1, page 76:
    *
    *     "Linear render target surface base addresses must be element-size
    *      aligned, for non-YUV surface formats, or a multiple of 2
    *      element-sizes for YUV surface formats. Other linear surfaces have
    *      no alignment requirements (byte alignment is sufficient.)"
    *
    * From the Sandy Bridge PRM, volume 4 part 1, page 81:
    *
    *     "For linear render target surfaces, the pitch must be a multiple
    *      of the element size for non-YUV surface formats. Pitch must be a
    *      multiple of 2 * element size for YUV surface formats."
    *
    * From the Sandy Bridge PRM, volume 4 part 1, page 86:
    *
    *     "For linear surfaces, this field (X Offset) must be zero"
    */
   if (img->layout.tiling == GEN6_TILING_NONE) {
      if (is_rt) {
         const int elem_size U_ASSERT_ONLY = icd_format_get_size(format);
         assert(pitch % elem_size == 0);
      }
   }

   dw[0] = surface_type << GEN6_SURFACE_DW0_TYPE__SHIFT |
           surface_format << GEN6_SURFACE_DW0_FORMAT__SHIFT |
           GEN6_SURFACE_DW0_MIPLAYOUT_BELOW;

   if (surface_type == GEN6_SURFTYPE_CUBE && !is_rt) {
      dw[0] |= 1 << 9 |
               GEN6_SURFACE_DW0_CUBE_FACE_ENABLES__MASK;
   }

   if (is_rt)
      dw[0] |= GEN6_SURFACE_DW0_RENDER_CACHE_RW;

   dw[1] = 0;

   dw[2] = (height - 1) << GEN6_SURFACE_DW2_HEIGHT__SHIFT |
           (width - 1) << GEN6_SURFACE_DW2_WIDTH__SHIFT |
           lod << GEN6_SURFACE_DW2_MIP_COUNT_LOD__SHIFT;

   assert(img->layout.tiling != GEN8_TILING_W);
   dw[3] = (depth - 1) << GEN6_SURFACE_DW3_DEPTH__SHIFT |
           (pitch - 1) << GEN6_SURFACE_DW3_PITCH__SHIFT |
           img->layout.tiling;

   dw[4] = first_level << GEN6_SURFACE_DW4_MIN_LOD__SHIFT |
           first_layer << 17 |
           (num_layers - 1) << 8 |
           ((img->sample_count > 1) ? GEN6_SURFACE_DW4_MULTISAMPLECOUNT_4 :
                                      GEN6_SURFACE_DW4_MULTISAMPLECOUNT_1);

   dw[5] = 0;

   assert(img->layout.align_j == 2 || img->layout.align_j == 4);
   if (img->layout.align_j == 4)
      dw[5] |= GEN6_SURFACE_DW5_VALIGN_4;
}

struct ds_surface_info {
   int surface_type;
   int format;

   struct {
      unsigned stride;
      unsigned offset;
   } zs, stencil, hiz;

   unsigned width, height, depth;
   unsigned lod, first_layer, num_layers;
};

static void
ds_init_info_null(const struct intel_gpu *gpu,
                  struct ds_surface_info *info)
{
   INTEL_GPU_ASSERT(gpu, 6, 7.5);

   memset(info, 0, sizeof(*info));

   info->surface_type = GEN6_SURFTYPE_NULL;
   info->format = GEN6_ZFORMAT_D32_FLOAT;
   info->width = 1;
   info->height = 1;
   info->depth = 1;
   info->num_layers = 1;
}

static void
ds_init_info(const struct intel_gpu *gpu,
             const struct intel_img *img,
             VkImageViewType view_type,
             VkFormat format, unsigned level,
             unsigned first_layer, unsigned num_layers,
             struct ds_surface_info *info)
{
   bool separate_stencil;

   INTEL_GPU_ASSERT(gpu, 6, 7.5);

   memset(info, 0, sizeof(*info));

   info->surface_type = view_type_to_surface_type(view_type);

   if (info->surface_type == GEN6_SURFTYPE_CUBE) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 325-326:
       *
       *     "For Other Surfaces (Cube Surfaces):
       *      This field (Minimum Array Element) is ignored."
       *
       *     "For Other Surfaces (Cube Surfaces):
       *      This field (Render Target View Extent) is ignored."
       *
       * As such, we cannot set first_layer and num_layers on cube surfaces.
       * To work around that, treat it as a 2D surface.
       */
      info->surface_type = GEN6_SURFTYPE_2D;
   }

   if (intel_gpu_gen(gpu) >= INTEL_GEN(7)) {
      separate_stencil = true;
   }
   else {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 317:
       *
       *     "This field (Separate Stencil Buffer Enable) must be set to the
       *      same value (enabled or disabled) as Hierarchical Depth Buffer
       *      Enable."
       */
      separate_stencil = intel_img_can_enable_hiz(img, level);
   }

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 317:
    *
    *     "If this field (Hierarchical Depth Buffer Enable) is enabled, the
    *      Surface Format of the depth buffer cannot be
    *      D32_FLOAT_S8X24_UINT or D24_UNORM_S8_UINT. Use of stencil
    *      requires the separate stencil buffer."
    *
    * From the Ironlake PRM, volume 2 part 1, page 330:
    *
    *     "If this field (Separate Stencil Buffer Enable) is disabled, the
    *      Surface Format of the depth buffer cannot be D24_UNORM_X8_UINT."
    *
    * There is no similar restriction for GEN6.  But when D24_UNORM_X8_UINT
    * is indeed used, the depth values output by the fragment shaders will
    * be different when read back.
    *
    * As for GEN7+, separate_stencil is always true.
    */
   switch (format) {
   case VK_FORMAT_D16_UNORM:
      info->format = GEN6_ZFORMAT_D16_UNORM;
      break;
   case VK_FORMAT_D32_SFLOAT:
      info->format = GEN6_ZFORMAT_D32_FLOAT;
      break;
   case VK_FORMAT_D32_SFLOAT_S8_UINT:
      info->format = (separate_stencil) ?
         GEN6_ZFORMAT_D32_FLOAT :
         GEN6_ZFORMAT_D32_FLOAT_S8X24_UINT;
      break;
   case VK_FORMAT_S8_UINT:
      if (separate_stencil) {
         info->format = GEN6_ZFORMAT_D32_FLOAT;
         break;
      }
      /* fall through */
   default:
      assert(!"unsupported depth/stencil format");
      ds_init_info_null(gpu, info);
      return;
      break;
   }

   if (format != VK_FORMAT_S8_UINT)
      info->zs.stride = img->layout.bo_stride;

   if (img->s8_layout) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 329:
       *
       *     "The pitch must be set to 2x the value computed based on width,
       *       as the stencil buffer is stored with two rows interleaved."
       *
       * According to the classic driver, we need to do the same for GEN7+
       * even though the Ivy Bridge PRM does not say anything about it.
       */
      info->stencil.stride = img->s8_layout->bo_stride * 2;

      if (intel_gpu_gen(gpu) == INTEL_GEN(6)) {
         unsigned x, y;

         assert(img->s8_layout->walk == INTEL_LAYOUT_WALK_LOD);

         /* offset to the level */
         intel_layout_get_slice_pos(img->s8_layout, level, 0, &x, &y);
         intel_layout_pos_to_mem(img->s8_layout, x, y, &x, &y);
         info->stencil.offset = intel_layout_mem_to_raw(img->s8_layout, x, y);
      }
   } else if (format == VK_FORMAT_S8_UINT) {
      info->stencil.stride = img->layout.bo_stride * 2;
   }

   if (intel_img_can_enable_hiz(img, level)) {
      info->hiz.stride = img->layout.aux_stride;

      /* offset to the level */
      if (intel_gpu_gen(gpu) == INTEL_GEN(6))
          info->hiz.offset = img->layout.aux_offsets[level];
   }

   info->width = img->layout.width0;
   info->height = img->layout.height0;
   info->depth = (img->type == VK_IMAGE_TYPE_3D) ?
      img->depth : num_layers;

   info->lod = level;
   info->first_layer = first_layer;
   info->num_layers = num_layers;
}

static void att_view_init_for_ds(struct intel_att_view *view,
                                 const struct intel_gpu *gpu,
                                 const struct intel_img *img,
                                 VkImageViewType view_type,
                                 VkFormat format, unsigned level,
                                 unsigned first_layer, unsigned num_layers)
{
   const int max_2d_size U_ASSERT_ONLY =
       (intel_gpu_gen(gpu) >= INTEL_GEN(7)) ? 16384 : 8192;
   const int max_array_size U_ASSERT_ONLY =
       (intel_gpu_gen(gpu) >= INTEL_GEN(7)) ? 2048 : 512;
   struct ds_surface_info info;
   uint32_t dw1, dw2, dw3, dw4, dw5, dw6;
   uint32_t *dw;

   INTEL_GPU_ASSERT(gpu, 6, 7.5);

   if (img) {
      ds_init_info(gpu, img, view_type, format, level,
              first_layer, num_layers, &info);
   }
   else {
      ds_init_info_null(gpu, &info);
   }

   switch (info.surface_type) {
   case GEN6_SURFTYPE_NULL:
      break;
   case GEN6_SURFTYPE_1D:
      assert(info.width <= max_2d_size && info.height == 1 &&
             info.depth <= max_array_size);
      assert(info.first_layer < max_array_size - 1 &&
             info.num_layers <= max_array_size);
      break;
   case GEN6_SURFTYPE_2D:
      assert(info.width <= max_2d_size && info.height <= max_2d_size &&
             info.depth <= max_array_size);
      assert(info.first_layer < max_array_size - 1 &&
             info.num_layers <= max_array_size);
      break;
   case GEN6_SURFTYPE_3D:
      assert(info.width <= 2048 && info.height <= 2048 && info.depth <= 2048);
      assert(info.first_layer < 2048 && info.num_layers <= max_array_size);
      break;
   case GEN6_SURFTYPE_CUBE:
      assert(info.width <= max_2d_size && info.height <= max_2d_size &&
             info.depth == 1);
      assert(info.first_layer == 0 && info.num_layers == 1);
      assert(info.width == info.height);
      break;
   default:
      assert(!"unexpected depth surface type");
      break;
   }

   dw1 = info.surface_type << 29 |
         info.format << 18;

   if (info.zs.stride) {
      /* required for GEN6+ */
      assert(info.zs.stride > 0 && info.zs.stride < 128 * 1024 &&
            info.zs.stride % 128 == 0);
      assert(info.width <= info.zs.stride);

      dw1 |= (info.zs.stride - 1);
   }

   dw2 = 0;

   if (intel_gpu_gen(gpu) >= INTEL_GEN(7)) {
      if (info.zs.stride)
         dw1 |= 1 << 28;

      if (info.stencil.stride)
         dw1 |= 1 << 27;

      if (info.hiz.stride)
         dw1 |= 1 << 22;

      dw3 = (info.height - 1) << 18 |
            (info.width - 1) << 4 |
            info.lod;

      dw4 = (info.depth - 1) << 21 |
            info.first_layer << 10 |
            GEN7_MOCS_L3_WB;

      dw5 = 0;

      dw6 = (info.num_layers - 1) << 21;
   }
   else {
      /* always Y-tiled */
      dw1 |= 1 << 27 |
             1 << 26;

      if (info.hiz.stride) {
         dw1 |= 1 << 22 |
                1 << 21;
      }

      dw3 = (info.height - 1) << 19 |
            (info.width - 1) << 6 |
            info.lod << 2 |
            GEN6_DEPTH_DW3_MIPLAYOUT_BELOW;

      dw4 = (info.depth - 1) << 21 |
            info.first_layer << 10 |
            (info.num_layers - 1) << 1;

      dw5 = 0;

      dw6 = 0;
   }

   STATIC_ASSERT(ARRAY_SIZE(view->att_cmd) >= 10);
   dw = view->att_cmd;

   dw[0] = dw1;
   dw[1] = dw2;
   dw[2] = dw3;
   dw[3] = dw4;
   dw[4] = dw5;
   dw[5] = dw6;

   /* separate stencil */
   if (info.stencil.stride) {
      assert(info.stencil.stride > 0 && info.stencil.stride < 128 * 1024 &&
             info.stencil.stride % 128 == 0);

      dw[6] = info.stencil.stride - 1;
      dw[7] = img->s8_offset;

      if (intel_gpu_gen(gpu) >= INTEL_GEN(7))
         dw[6] |= GEN7_MOCS_L3_WB << GEN6_STENCIL_DW1_MOCS__SHIFT;
      if (intel_gpu_gen(gpu) >= INTEL_GEN(7.5))
         dw[6] |= GEN75_STENCIL_DW1_STENCIL_BUFFER_ENABLE;
   }
   else {
      dw[6] = 0;
      dw[7] = 0;
   }

   /* hiz */
   if (info.hiz.stride) {
      dw[8] = info.hiz.stride - 1;
      dw[9] = img->aux_offset;

      if (intel_gpu_gen(gpu) >= INTEL_GEN(7))
         dw[8] |= GEN7_MOCS_L3_WB << GEN6_HIZ_DW1_MOCS__SHIFT;
   }
   else {
      dw[8] = 0;
      dw[9] = 0;
   }

   view->has_stencil = info.stencil.stride;
   view->has_hiz = info.hiz.stride;
}

static const VkComponentMapping identity_channel_mapping = {
    .r = VK_COMPONENT_SWIZZLE_R,
    .g = VK_COMPONENT_SWIZZLE_G,
    .b = VK_COMPONENT_SWIZZLE_B,
    .a = VK_COMPONENT_SWIZZLE_A,
};

static void att_view_init_for_rt(struct intel_att_view *view,
                                 const struct intel_gpu *gpu,
                                 const struct intel_img *img,
                                 VkImageViewType view_type,
                                 VkFormat format, unsigned level,
                                 unsigned first_layer, unsigned num_layers)
{
    if (intel_gpu_gen(gpu) >= INTEL_GEN(7)) {
        surface_state_tex_gen7(gpu, img, view_type, format,
                level, 1, first_layer, num_layers,
                identity_channel_mapping, true, view->att_cmd);
    } else {
        surface_state_tex_gen6(gpu, img, view_type, format,
                level, 1, first_layer, num_layers,
                true, view->att_cmd);
    }

    view->is_rt = true;
}

static void att_view_init_for_input(struct intel_att_view *view,
                                    const struct intel_gpu *gpu,
                                    const struct intel_img *img,
                                    VkImageViewType view_type,
                                    VkFormat format, unsigned level,
                                    unsigned first_layer, unsigned num_layers)
{
    if (intel_gpu_gen(gpu) >= INTEL_GEN(7)) {
        if (false) {
            surface_state_tex_gen7(gpu, img, view_type, format,
                    level, 1, first_layer, num_layers,
                    identity_channel_mapping, false, view->cmd);
        } else {
            surface_state_null_gen7(gpu, view->cmd);
        }

        view->cmd_len = 8;
    } else {
        if (false) {
            surface_state_tex_gen6(gpu, img, view_type, format,
                    level, 1, first_layer, num_layers, false, view->cmd);
        } else {
            surface_state_null_gen6(gpu, view->cmd);
        }

        view->cmd_len = 6;
    }
}

void intel_null_view_init(struct intel_null_view *view,
                          struct intel_dev *dev)
{
    if (intel_gpu_gen(dev->gpu) >= INTEL_GEN(7)) {
        surface_state_null_gen7(dev->gpu, view->cmd);
        view->cmd_len = 8;
    } else {
        surface_state_null_gen6(dev->gpu, view->cmd);
        view->cmd_len = 6;
    }
}

static void buf_view_destroy(struct intel_obj *obj)
{
    struct intel_buf_view *view = intel_buf_view_from_obj(obj);

    intel_buf_view_destroy(view);
}

void intel_buf_view_init(const struct intel_dev *dev,
                         const VkBufferViewCreateInfo *info,
                         struct intel_buf_view *view,
                         bool raw)
{
    struct intel_buf *buf = intel_buf(info->buffer);
    /* TODO: Is transfer destination the only shader write operation? */
    const bool will_write = (buf->usage & (VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
    VkFormat format;
    VkDeviceSize stride;
    uint32_t *cmd;
    int i;

    view->obj.destroy = buf_view_destroy;

    view->buf = buf;

    /*
     * The compiler expects uniform buffers to have pitch of
     * 4 for fragment shaders, but 16 for other stages.  The format
     * must be VK_FORMAT_R32G32B32A32_SFLOAT.
     */
    if (raw) {
        format = VK_FORMAT_R32G32B32A32_SFLOAT;
        stride = 16;
    } else {
        format = info->format;
        stride = icd_format_get_size(format);
    }
    cmd = view->cmd;

    for (i = 0; i < 2; i++) {
        if (intel_gpu_gen(dev->gpu) >= INTEL_GEN(7)) {
            surface_state_buf_gen7(dev->gpu, info->offset,
                    info->range, stride, format,
                    will_write, will_write, cmd);
            view->cmd_len = 8;
        } else {
            surface_state_buf_gen6(dev->gpu, info->offset,
                    info->range, stride, format,
                    will_write, will_write, cmd);
            view->cmd_len = 6;
        }

        /* switch to view->fs_cmd */
        if (raw) {
            cmd = view->fs_cmd;
            stride = 4;
        } else {
            memcpy(view->fs_cmd, view->cmd, sizeof(uint32_t) * view->cmd_len);
            break;
        }
    }
}

VkResult intel_buf_view_create(struct intel_dev *dev,
                               const VkBufferViewCreateInfo *info,
                               struct intel_buf_view **view_ret)
{
    struct intel_buf_view *view;

    view = (struct intel_buf_view *) intel_base_create(&dev->base.handle,
            sizeof(*view), dev->base.dbg, VK_OBJECT_TYPE_BUFFER_VIEW,
            info, 0);
    if (!view)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    intel_buf_view_init(dev, info, view, false);

    *view_ret = view;

    return VK_SUCCESS;
}

void intel_buf_view_destroy(struct intel_buf_view *view)
{
    intel_base_destroy(&view->obj.base);
}

static void img_view_destroy(struct intel_obj *obj)
{
    struct intel_img_view *view = intel_img_view_from_obj(obj);

    intel_img_view_destroy(view);
}

void intel_img_view_init(struct intel_dev *dev,
                         const VkImageViewCreateInfo *info,
                         struct intel_img_view *view)
{
    VkComponentMapping state_swizzles;
    uint32_t mip_levels, array_size;
    struct intel_img *img = intel_img(info->image);

    mip_levels = info->subresourceRange.levelCount;
    if (mip_levels > img->mip_levels - info->subresourceRange.baseMipLevel)
        mip_levels = img->mip_levels - info->subresourceRange.baseMipLevel;

    array_size = info->subresourceRange.layerCount;
    if (array_size > img->array_size - info->subresourceRange.baseArrayLayer)
        array_size = img->array_size - info->subresourceRange.baseArrayLayer;

    view->obj.destroy = img_view_destroy;

    view->img = img;

    if (!(img->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
        if (intel_gpu_gen(dev->gpu) >= INTEL_GEN(7.5)) {
            state_swizzles = info->components;
            view->shader_swizzles.r = VK_COMPONENT_SWIZZLE_R;
            view->shader_swizzles.g = VK_COMPONENT_SWIZZLE_G;
            view->shader_swizzles.b = VK_COMPONENT_SWIZZLE_B;
            view->shader_swizzles.a = VK_COMPONENT_SWIZZLE_A;
        } else {
            state_swizzles.r = VK_COMPONENT_SWIZZLE_R;
            state_swizzles.g = VK_COMPONENT_SWIZZLE_G;
            state_swizzles.b = VK_COMPONENT_SWIZZLE_B;
            state_swizzles.a = VK_COMPONENT_SWIZZLE_A;
            view->shader_swizzles = info->components;
        }

        /* shader_swizzles is ignored by the compiler */
        if (view->shader_swizzles.r != VK_COMPONENT_SWIZZLE_R ||
            view->shader_swizzles.g != VK_COMPONENT_SWIZZLE_G ||
            view->shader_swizzles.b != VK_COMPONENT_SWIZZLE_B ||
            view->shader_swizzles.a != VK_COMPONENT_SWIZZLE_A) {
            intel_dev_log(dev, VK_DBG_REPORT_WARN_BIT,
                          (struct intel_base*)view, 0, 0,
                          "image data swizzling is ignored");
        }

        if (intel_gpu_gen(dev->gpu) >= INTEL_GEN(7)) {
            surface_state_tex_gen7(dev->gpu, img, info->viewType, info->format,
                    info->subresourceRange.baseMipLevel, mip_levels,
                    info->subresourceRange.baseArrayLayer, array_size,
                    state_swizzles, false, view->cmd);
            view->cmd_len = 8;
        } else {
            surface_state_tex_gen6(dev->gpu, img, info->viewType, info->format,
                    info->subresourceRange.baseMipLevel, mip_levels,
                    info->subresourceRange.baseArrayLayer, array_size,
                    false, view->cmd);
            view->cmd_len = 6;
        }
    }
}

VkResult intel_img_view_create(struct intel_dev *dev,
                               const VkImageViewCreateInfo *info,
                               struct intel_img_view **view_ret)
{
    struct intel_img_view *view;

    view = (struct intel_img_view *) intel_base_create(&dev->base.handle,
            sizeof(*view), dev->base.dbg, VK_OBJECT_TYPE_IMAGE_VIEW, info, 0);
    if (!view)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    intel_img_view_init(dev, info, view);

    /* Initialize attachment view info in case it's needed */
    intel_att_view_init(dev, info, &view->att_view);

    *view_ret = view;

    return VK_SUCCESS;
}

void intel_img_view_destroy(struct intel_img_view *view)
{
    intel_base_destroy(&view->obj.base);
}

void intel_att_view_init(struct intel_dev *dev,
                           const VkImageViewCreateInfo *info,
                           struct intel_att_view *att_view)
{
    struct intel_img *img = intel_img(info->image);
    VkImageViewType view_type;

    att_view->img = img;

    att_view->mipLevel = info->subresourceRange.baseMipLevel;
    att_view->baseArrayLayer = info->subresourceRange.baseArrayLayer;
    att_view->array_size = info->subresourceRange.layerCount;

    view_type = img_type_to_view_type(img->type,
                                      info->subresourceRange.baseArrayLayer,
                                      info->subresourceRange.layerCount);

    att_view_init_for_input(att_view, dev->gpu, img, view_type, info->format,
                            info->subresourceRange.baseMipLevel,
                            info->subresourceRange.baseArrayLayer,
                            info->subresourceRange.layerCount);

    if (img->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        att_view_init_for_ds(att_view, dev->gpu, img, view_type, img->layout.format,
                             info->subresourceRange.baseMipLevel,
                             info->subresourceRange.baseArrayLayer,
                             info->subresourceRange.layerCount);
    } else {
        att_view_init_for_rt(att_view, dev->gpu, img, view_type, info->format,
                             info->subresourceRange.baseMipLevel,
                             info->subresourceRange.baseArrayLayer,
                             info->subresourceRange.layerCount);
    }
}

ICD_EXPORT VkResult VKAPI vkCreateBufferView(
    VkDevice                            device,
    const VkBufferViewCreateInfo*       pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkBufferView*                       pView)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_buf_view_create(dev, pCreateInfo,
            (struct intel_buf_view **) pView);
}

ICD_EXPORT void VKAPI vkDestroyBufferView(
    VkDevice                            device,
    VkBufferView                        bufferView,
    const VkAllocationCallbacks*                     pAllocator)

 {
    struct intel_obj *obj = intel_obj(bufferView);

    obj->destroy(obj);
 }

ICD_EXPORT VkResult VKAPI vkCreateImageView(
    VkDevice                            device,
    const VkImageViewCreateInfo*        pCreateInfo,
    const VkAllocationCallbacks*                     pAllocator,
    VkImageView*                        pView)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_img_view_create(dev, pCreateInfo,
            (struct intel_img_view **) pView);
}

ICD_EXPORT void VKAPI vkDestroyImageView(
    VkDevice                            device,
    VkImageView                         imageView,
    const VkAllocationCallbacks*                     pAllocator)

{
    struct intel_obj *obj = intel_obj(imageView);

    obj->destroy(obj);
}

