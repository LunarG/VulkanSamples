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
 */

#ifndef LAYOUT_H
#define LAYOUT_H

#include "genhw/genhw.h"
#include "intel.h"

#define INTEL_LAYOUT_MAX_LEVELS 16

struct intel_dev;

enum intel_layout_walk_type {
   /*
    * Array layers of an LOD are packed together vertically.  This maps to
    * ARYSPC_LOD0 for non-mipmapped 2D textures, and is extended to support
    * mipmapped stencil textures and HiZ on GEN6.
    */
   INTEL_LAYOUT_WALK_LOD,

   /*
    * LODs of an array layer are packed together.  This maps to ARYSPC_FULL
    * and is used for mipmapped 2D textures.
    */
   INTEL_LAYOUT_WALK_LAYER,

   /*
    * 3D slices of an LOD are packed together, horizontally with wrapping.
    * Used for 3D textures.
    */
   INTEL_LAYOUT_WALK_3D,
};

enum intel_layout_aux_type {
   INTEL_LAYOUT_AUX_NONE,
   INTEL_LAYOUT_AUX_HIZ,
   INTEL_LAYOUT_AUX_MCS,
};

struct intel_layout_lod {
   /* physical position */
   unsigned x;
   unsigned y;

   /*
    * Physical size of an LOD slice.  There may be multiple slices when the
    * walk type is not INTEL_LAYOUT_WALK_LAYER.
    */
   unsigned slice_width;
   unsigned slice_height;
};

/**
 * Texture layout.
 */
struct intel_layout {
   enum intel_layout_aux_type aux;

   /* physical width0, height0, and format */
   unsigned width0;
   unsigned height0;
   VkFormat format;
   bool separate_stencil;

   /*
    * width, height, and size of pixel blocks, for conversion between 2D
    * coordinates and memory offsets
    */
   unsigned block_width;
   unsigned block_height;
   unsigned block_size;

   enum intel_layout_walk_type walk;
   bool interleaved_samples;

   /* bitmask of valid tiling modes */
   unsigned valid_tilings;
   enum gen_surface_tiling tiling;

   /* mipmap alignments */
   unsigned align_i;
   unsigned align_j;

   struct intel_layout_lod lods[INTEL_LAYOUT_MAX_LEVELS];

   /* physical height of layers for INTEL_LAYOUT_WALK_LAYER */
   unsigned layer_height;

   /* distance in bytes between two pixel block rows */
   unsigned bo_stride;
   /* number of pixel block rows */
   unsigned bo_height;

   /* bitmask of levels that can use aux */
   unsigned aux_enables;
   unsigned aux_offsets[INTEL_LAYOUT_MAX_LEVELS];
   unsigned aux_layer_height;
   unsigned aux_stride;
   unsigned aux_height;
};

void intel_layout_init(struct intel_layout *layout,
                       struct intel_dev *dev,
                       const VkImageCreateInfo *info,
                       bool scanout);

/**
 * Convert from pixel position to 2D memory offset.
 */
static inline void
intel_layout_pos_to_mem(const struct intel_layout *layout,
                        unsigned pos_x, unsigned pos_y,
                        unsigned *mem_x, unsigned *mem_y)
{
   assert(pos_x % layout->block_width == 0);
   assert(pos_y % layout->block_height == 0);

   *mem_x = pos_x / layout->block_width * layout->block_size;
   *mem_y = pos_y / layout->block_height;
}

/**
 * Convert from 2D memory offset to linear offset.
 */
static inline unsigned
intel_layout_mem_to_linear(const struct intel_layout *layout,
                           unsigned mem_x, unsigned mem_y)
{
   return mem_y * layout->bo_stride + mem_x;
}

/**
 * Convert from 2D memory offset to raw offset.
 */
static inline unsigned
intel_layout_mem_to_raw(const struct intel_layout *layout,
                        unsigned mem_x, unsigned mem_y)
{
   unsigned tile_w U_ASSERT_ONLY;
   unsigned tile_h;

   switch (layout->tiling) {
   case GEN6_TILING_NONE:
      tile_w = 1;
      tile_h = 1;
      break;
   case GEN6_TILING_X:
      tile_w = 512;
      tile_h = 8;
      break;
   case GEN6_TILING_Y:
      tile_w = 128;
      tile_h = 32;
      break;
   case GEN8_TILING_W:
      tile_w = 64;
      tile_h = 64;
      break;
   default:
      assert(!"unknown tiling");
      tile_w = 1;
      tile_h = 1;
      break;
   }

   assert(mem_x % tile_w == 0);
   assert(mem_y % tile_h == 0);

   return mem_y * layout->bo_stride + mem_x * tile_h;
}

/**
 * Return the stride, in bytes, between slices within a level.
 */
static inline unsigned
intel_layout_get_slice_stride(const struct intel_layout *layout, unsigned level)
{
   unsigned h;

   switch (layout->walk) {
   case INTEL_LAYOUT_WALK_LOD:
      h = layout->lods[level].slice_height;
      break;
   case INTEL_LAYOUT_WALK_LAYER:
      h = layout->layer_height;
      break;
   case INTEL_LAYOUT_WALK_3D:
      if (level == 0) {
         h = layout->lods[0].slice_height;
         break;
      }
      /* fall through */
   default:
      assert(!"no single stride to walk across slices");
      h = 0;
      break;
   }

   assert(h % layout->block_height == 0);

   return (h / layout->block_height) * layout->bo_stride;
}

/**
 * Return the physical size, in bytes, of a slice in a level.
 */
static inline unsigned
intel_layout_get_slice_size(const struct intel_layout *layout, unsigned level)
{
   const unsigned w = layout->lods[level].slice_width;
   const unsigned h = layout->lods[level].slice_height;

   assert(w % layout->block_width == 0);
   assert(h % layout->block_height == 0);

   return (w / layout->block_width * layout->block_size) *
      (h / layout->block_height);
}

/**
 * Return the pixel position of a slice.
 */
static inline void
intel_layout_get_slice_pos(const struct intel_layout *layout,
                           unsigned level, unsigned slice,
                           unsigned *x, unsigned *y)
{
   switch (layout->walk) {
   case INTEL_LAYOUT_WALK_LOD:
      *x = layout->lods[level].x;
      *y = layout->lods[level].y + layout->lods[level].slice_height * slice;
      break;
   case INTEL_LAYOUT_WALK_LAYER:
      *x = layout->lods[level].x;
      *y = layout->lods[level].y + layout->layer_height * slice;
      break;
   case INTEL_LAYOUT_WALK_3D:
      {
         /* slices are packed horizontally with wrapping */
         const unsigned sx = slice & ((1 << level) - 1);
         const unsigned sy = slice >> level;

         *x = layout->lods[level].x + layout->lods[level].slice_width * sx;
         *y = layout->lods[level].y + layout->lods[level].slice_height * sy;

         /* should not overlap with the next level */
         if (level + 1 < ARRAY_SIZE(layout->lods) &&
             layout->lods[level + 1].y) {
            assert(*y + layout->lods[level].slice_height <=
                  layout->lods[level + 1].y);
         }
         break;
      }
   default:
      assert(!"unknown layout walk type");
      *x = 0;
      *y = 0;
      break;
   }

   /* should not exceed the bo size */
   assert(*y + layout->lods[level].slice_height <=
         layout->bo_height * layout->block_height);
}

unsigned
intel_layout_get_slice_tile_offset(const struct intel_layout *layout,
                                   unsigned level, unsigned slice,
                                   unsigned *x_offset, unsigned *y_offset);

#endif /* LAYOUT_H */
