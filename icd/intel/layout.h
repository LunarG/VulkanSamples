/*
 * Mesa 3-D graphics library
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
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef LAYOUT_H
#define LAYOUT_H

#include "kmd/winsys.h"
#include "intel.h"

#define INTEL_IMG_MAX_LEVELS 16

enum intel_layout_aux_type {
   INTEL_LAYOUT_AUX_NONE,
   INTEL_LAYOUT_AUX_HIZ,
   INTEL_LAYOUT_AUX_MCS,
};

/**
 * Texture layout.
 */
struct intel_layout {
   enum intel_layout_aux_type aux_type;
   XGL_FORMAT format;
   bool separate_stencil;

   /*
    * width, height, and size of pixel blocks, for conversion between 2D
    * coordinates and memory offsets
    */
   unsigned block_width;
   unsigned block_height;
   unsigned block_size;

   /* arrangements of samples and array layers */
   bool interleaved_samples;
   bool full_layers;
   bool is_2d;

   /* bitmask of valid tiling modes */
   unsigned valid_tilings;
   enum intel_tiling_mode tiling;

   /* mipmap alignments */
   unsigned align_i;
   unsigned align_j;

   struct {
      /* physical position of a mipmap */
      unsigned x;
      unsigned y;

      /*
       * Physical size of a mipmap slice.  For 3D textures, there may be
       * multiple slices.
       */
      unsigned slice_width;
      unsigned slice_height;
   } levels[INTEL_IMG_MAX_LEVELS];

   /* physical height of array layers, cube faces, or sample layers */
   unsigned layer_height;

   /* distance in bytes between two pixel block rows */
   unsigned bo_stride;
   /* number of pixel block rows */
   unsigned bo_height;

   unsigned aux_stride;
   unsigned aux_height;
};

void intel_layout_init(struct intel_layout *layout,
                       const struct intel_dev *dev,
                       const XGL_IMAGE_CREATE_INFO *info);

bool
intel_layout_update_for_imported_bo(struct intel_layout *layout,
                                    enum intel_tiling_mode tiling,
                                    unsigned bo_stride);

/**
 * Convert from pixel position to memory position.
 */
static inline void
intel_layout_pos_to_mem(const struct intel_layout *layout,
                        unsigned x, unsigned y,
                        unsigned *mem_x, unsigned *mem_y)
{
   assert(x % layout->block_width == 0);
   assert(y % layout->block_height == 0);

   *mem_x = x / layout->block_width * layout->block_size;
   *mem_y = y / layout->block_height;
}

/**
 * Convert from memory position to memory offset.
 */
static inline unsigned
intel_layout_mem_to_off(const struct intel_layout *layout,
                        unsigned mem_x, unsigned mem_y)
{
   return mem_y * layout->bo_stride + mem_x;
}

/**
 * Return the stride, in bytes, between slices within a level.
 */
static inline unsigned
intel_layout_get_slice_stride(const struct intel_layout *layout,
                              unsigned level)
{
   unsigned y;

   if (layout->is_2d) {
      y = layout->layer_height;
   } else if (level == 0) {
      y = layout->levels[0].slice_height;
   } else {
      assert(!"no single slice stride for 3D texture with level > 0");
      y = 0;
   }

   assert(y % layout->block_height == 0);

   return (y / layout->block_height) * layout->bo_stride;
}

/**
 * Return the physical size, in bytes, of a slice in a level.
 */
static inline unsigned
intel_layout_get_slice_size(const struct intel_layout *layout, unsigned level)
{
   const unsigned w = layout->levels[level].slice_width;
   const unsigned h = layout->levels[level].slice_height;

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
   if (layout->is_2d) {
      *x = layout->levels[level].x;
      *y = layout->levels[level].y + layout->layer_height * slice;
   } else {
      const unsigned sx = slice & ((1 << level) - 1);
      const unsigned sy = slice >> level;

      *x = layout->levels[level].x + layout->levels[level].slice_width * sx;
      *y = layout->levels[level].y + layout->levels[level].slice_height * sy;

      /* should not overlap with the next level */
      if (level + 1 < ARRAY_SIZE(layout->levels) &&
          layout->levels[level + 1].y) {
         assert(*y + layout->levels[level].slice_height <=
               layout->levels[level + 1].y);
      }
   }

   /* should not exceed the bo size */
   assert(*y + layout->levels[level].slice_height <=
         layout->bo_height * layout->block_height);
}

#endif /* LAYOUT_H */
