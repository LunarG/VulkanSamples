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

#include "genhw/genhw.h"
#include "dev.h"
#include "gpu.h"
#include "format.h"

struct intel_sampler_cap {
   int sampling;
   int filtering;
   int shadow_map;
   int chroma_key;
};

struct intel_dp_cap {
   int rt_write;
   int rt_write_blending;
   int typed_write;
   int media_color_processing;
};

/*
 * This table is based on:
 *
 *  - the Sandy Bridge PRM, volume 4 part 1, page 88-97
 *  - the Ivy Bridge PRM, volume 4 part 1, page 84-87
 */
static const struct intel_sampler_cap intel_sampler_caps[] = {
#define CAP(sampling, filtering, shadow_map, chroma_key) \
   { INTEL_GEN(sampling), INTEL_GEN(filtering), INTEL_GEN(shadow_map), INTEL_GEN(chroma_key) }
   [GEN6_FORMAT_R32G32B32A32_FLOAT]       = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R32G32B32A32_SINT]        = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32G32B32A32_UINT]        = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32G32B32X32_FLOAT]       = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R32G32B32_FLOAT]          = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R32G32B32_SINT]           = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32G32B32_UINT]           = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_UNORM]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_SNORM]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_SINT]        = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_UINT]        = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16B16A16_FLOAT]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R32G32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R32G32_SINT]              = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32G32_UINT]              = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32_FLOAT_X8X24_TYPELESS] = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_X32_TYPELESS_G8X24_UINT]  = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_L32A32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_R16G16B16X16_UNORM]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16B16X16_FLOAT]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_A32X32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_L32X32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_I32X32_FLOAT]             = CAP(  1,   5,   0,   0),
   [GEN6_FORMAT_B8G8R8A8_UNORM]           = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B8G8R8A8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R10G10B10A2_UNORM]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R10G10B10A2_UNORM_SRGB]   = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R10G10B10A2_UINT]         = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R10G10B10_SNORM_A2_UNORM] = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_UNORM]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_SNORM]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_SINT]            = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R8G8B8A8_UINT]            = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16_UNORM]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16_SNORM]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16_SINT]              = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16_UINT]              = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16G16_FLOAT]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B10G10R10A2_UNORM]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B10G10R10A2_UNORM_SRGB]   = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R11G11B10_FLOAT]          = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R32_SINT]                 = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32_UINT]                 = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R32_FLOAT]                = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_R24_UNORM_X8_TYPELESS]    = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_X24_TYPELESS_G8_UINT]     = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_L16A16_UNORM]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_I24X8_UNORM]              = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_L24X8_UNORM]              = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_A24X8_UNORM]              = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_I32_FLOAT]                = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_L32_FLOAT]                = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_A32_FLOAT]                = CAP(  1,   5,   1,   0),
   [GEN6_FORMAT_B8G8R8X8_UNORM]           = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B8G8R8X8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8X8_UNORM]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8B8X8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R9G9B9E5_SHAREDEXP]       = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B10G10R10X2_UNORM]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_L16A16_FLOAT]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B5G6R5_UNORM]             = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B5G6R5_UNORM_SRGB]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B5G5R5A1_UNORM]           = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B5G5R5A1_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B4G4R4A4_UNORM]           = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_B4G4R4A4_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8_UNORM]               = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8_SNORM]               = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_R8G8_SINT]                = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R8G8_UINT]                = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16_UNORM]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_R16_SNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16_SINT]                 = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16_UINT]                 = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R16_FLOAT]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_A8P8_UNORM_PALETTE0]      = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_A8P8_UNORM_PALETTE1]      = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_I16_UNORM]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_L16_UNORM]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_A16_UNORM]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_L8A8_UNORM]               = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_I16_FLOAT]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_L16_FLOAT]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_A16_FLOAT]                = CAP(  1,   1,   1,   0),
   [GEN6_FORMAT_L8A8_UNORM_SRGB]          = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_R5G5_SNORM_B6_UNORM]      = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_P8A8_UNORM_PALETTE0]      = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_P8A8_UNORM_PALETTE1]      = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_R8_UNORM]                 = CAP(  1,   1,   0, 4.5),
   [GEN6_FORMAT_R8_SNORM]                 = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8_SINT]                  = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_R8_UINT]                  = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_A8_UNORM]                 = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_I8_UNORM]                 = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_L8_UNORM]                 = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_P4A4_UNORM_PALETTE0]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_A4P4_UNORM_PALETTE0]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_P8_UNORM_PALETTE0]        = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_L8_UNORM_SRGB]            = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_P8_UNORM_PALETTE1]        = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_P4A4_UNORM_PALETTE1]      = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_A4P4_UNORM_PALETTE1]      = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_DXT1_RGB_SRGB]            = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_R1_UNORM]                 = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_YCRCB_NORMAL]             = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_YCRCB_SWAPUVY]            = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_P2_UNORM_PALETTE0]        = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_P2_UNORM_PALETTE1]        = CAP(4.5, 4.5,   0,   0),
   [GEN6_FORMAT_BC1_UNORM]                = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_BC2_UNORM]                = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_BC3_UNORM]                = CAP(  1,   1,   0,   1),
   [GEN6_FORMAT_BC4_UNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC5_UNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC1_UNORM_SRGB]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC2_UNORM_SRGB]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC3_UNORM_SRGB]           = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_MONO8]                    = CAP(  1,   0,   0,   0),
   [GEN6_FORMAT_YCRCB_SWAPUV]             = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_YCRCB_SWAPY]              = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_DXT1_RGB]                 = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_FXT1]                     = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC4_SNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_BC5_SNORM]                = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R16G16B16_FLOAT]          = CAP(  5,   5,   0,   0),
   [GEN6_FORMAT_BC6H_SF16]                = CAP(  7,   7,   0,   0),
   [GEN6_FORMAT_BC7_UNORM]                = CAP(  7,   7,   0,   0),
   [GEN6_FORMAT_BC7_UNORM_SRGB]           = CAP(  7,   7,   0,   0),
   [GEN6_FORMAT_BC6H_UF16]                = CAP(  7,   7,   0,   0),
#undef CAP
};

/*
 * This table is based on:
 *
 *  - the Sandy Bridge PRM, volume 4 part 1, page 88-97
 *  - the Ivy Bridge PRM, volume 4 part 1, page 172, 252-253, and 277-278
 *  - the Haswell PRM, volume 7, page 262-264
 */
static const struct intel_dp_cap intel_dp_caps[] = {
#define CAP(rt_write, rt_write_blending, typed_write, media_color_processing) \
   { INTEL_GEN(rt_write), INTEL_GEN(rt_write_blending), INTEL_GEN(typed_write), INTEL_GEN(media_color_processing) }
   [GEN6_FORMAT_R32G32B32A32_FLOAT]       = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R32G32B32A32_SINT]        = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R32G32B32A32_UINT]        = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16B16A16_UNORM]       = CAP(  1, 4.5,   7,   6),
   [GEN6_FORMAT_R16G16B16A16_SNORM]       = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R16G16B16A16_SINT]        = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16B16A16_UINT]        = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16B16A16_FLOAT]       = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R32G32_FLOAT]             = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R32G32_SINT]              = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R32G32_UINT]              = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_B8G8R8A8_UNORM]           = CAP(  1,   1,   7,   6),
   [GEN6_FORMAT_B8G8R8A8_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R10G10B10A2_UNORM]        = CAP(  1,   1,   7,   6),
   [GEN6_FORMAT_R10G10B10A2_UNORM_SRGB]   = CAP(  0,   0,   0,   6),
   [GEN6_FORMAT_R10G10B10A2_UINT]         = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R8G8B8A8_UNORM]           = CAP(  1,   1,   7,   6),
   [GEN6_FORMAT_R8G8B8A8_UNORM_SRGB]      = CAP(  1,   1,   0,   6),
   [GEN6_FORMAT_R8G8B8A8_SNORM]           = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R8G8B8A8_SINT]            = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R8G8B8A8_UINT]            = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16_UNORM]             = CAP(  1, 4.5,   7,   0),
   [GEN6_FORMAT_R16G16_SNORM]             = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R16G16_SINT]              = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16_UINT]              = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16G16_FLOAT]             = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B10G10R10A2_UNORM]        = CAP(  1,   1,   7,   6),
   [GEN6_FORMAT_B10G10R10A2_UNORM_SRGB]   = CAP(  1,   1,   0,   6),
   [GEN6_FORMAT_R11G11B10_FLOAT]          = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R32_SINT]                 = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R32_UINT]                 = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R32_FLOAT]                = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B8G8R8X8_UNORM]           = CAP(  0,   0,   0,   6),
   [GEN6_FORMAT_B5G6R5_UNORM]             = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B5G6R5_UNORM_SRGB]        = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B5G5R5A1_UNORM]           = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B5G5R5A1_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_B4G4R4A4_UNORM]           = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B4G4R4A4_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8G8_UNORM]               = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R8G8_SNORM]               = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R8G8_SINT]                = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R8G8_UINT]                = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16_UNORM]                = CAP(  1, 4.5,   7,   7),
   [GEN6_FORMAT_R16_SNORM]                = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R16_SINT]                 = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16_UINT]                 = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R16_FLOAT]                = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B5G5R5X1_UNORM]           = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_B5G5R5X1_UNORM_SRGB]      = CAP(  1,   1,   0,   0),
   [GEN6_FORMAT_R8_UNORM]                 = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_R8_SNORM]                 = CAP(  1,   6,   7,   0),
   [GEN6_FORMAT_R8_SINT]                  = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_R8_UINT]                  = CAP(  1,   0,   7,   0),
   [GEN6_FORMAT_A8_UNORM]                 = CAP(  1,   1,   7,   0),
   [GEN6_FORMAT_YCRCB_NORMAL]             = CAP(  1,   0,   0,   6),
   [GEN6_FORMAT_YCRCB_SWAPUVY]            = CAP(  1,   0,   0,   6),
   [GEN6_FORMAT_YCRCB_SWAPUV]             = CAP(  1,   0,   0,   6),
   [GEN6_FORMAT_YCRCB_SWAPY]              = CAP(  1,   0,   0,   6),
#undef CAP
};

static const int intel_color_mapping[XGL_MAX_CH_FMT + 1][6] = {
    [XGL_CH_FMT_B5G6R5] =       { GEN6_FORMAT_B5G6R5_UNORM,
                                  0,
                                  0,
                                  0,
                                  0,
                                  GEN6_FORMAT_B5G6R5_UNORM_SRGB, },
    [XGL_CH_FMT_R8] =           { GEN6_FORMAT_R8_UNORM,
                                  GEN6_FORMAT_R8_SNORM,
                                  GEN6_FORMAT_R8_UINT,
                                  GEN6_FORMAT_R8_SINT,
                                  0,
                                  0, },
    [XGL_CH_FMT_R8G8] =         { GEN6_FORMAT_R8G8_UNORM,
                                  GEN6_FORMAT_R8G8_SNORM,
                                  GEN6_FORMAT_R8G8_UINT,
                                  GEN6_FORMAT_R8G8_SINT,
                                  0,
                                  0, },
    [XGL_CH_FMT_R8G8B8A8] =     { GEN6_FORMAT_R8G8B8A8_UNORM,
                                  GEN6_FORMAT_R8G8B8A8_SNORM,
                                  GEN6_FORMAT_R8G8B8A8_UINT,
                                  GEN6_FORMAT_R8G8B8A8_SINT,
                                  0,
                                  GEN6_FORMAT_R8G8B8A8_UNORM_SRGB, },
    [XGL_CH_FMT_B8G8R8A8] =     { GEN6_FORMAT_B8G8R8A8_UNORM,
                                  0,
                                  0,
                                  0,
                                  0,
                                  GEN6_FORMAT_B8G8R8A8_UNORM_SRGB, },
    [XGL_CH_FMT_R11G11B10] =    { 0,
                                  0,
                                  0,
                                  0,
                                  GEN6_FORMAT_R11G11B10_FLOAT,
                                  0, },
    [XGL_CH_FMT_R10G10B10A2] =  { GEN6_FORMAT_R10G10B10A2_UNORM,
                                  GEN6_FORMAT_R10G10B10A2_SNORM,
                                  GEN6_FORMAT_R10G10B10A2_UINT,
                                  GEN6_FORMAT_R10G10B10A2_SINT,
                                  0,
                                  0, },
    [XGL_CH_FMT_R16] =          { GEN6_FORMAT_R16_UNORM,
                                  GEN6_FORMAT_R16_SNORM,
                                  GEN6_FORMAT_R16_UINT,
                                  GEN6_FORMAT_R16_SINT,
                                  GEN6_FORMAT_R16_FLOAT,
                                  0, },
    [XGL_CH_FMT_R16G16] =       { GEN6_FORMAT_R16G16_UNORM,
                                  GEN6_FORMAT_R16G16_SNORM,
                                  GEN6_FORMAT_R16G16_UINT,
                                  GEN6_FORMAT_R16G16_SINT,
                                  GEN6_FORMAT_R16G16_FLOAT,
                                  0, },
    [XGL_CH_FMT_R16G16B16A16] = { GEN6_FORMAT_R16G16B16A16_UNORM,
                                  GEN6_FORMAT_R16G16B16A16_SNORM,
                                  GEN6_FORMAT_R16G16B16A16_UINT,
                                  GEN6_FORMAT_R16G16B16A16_SINT,
                                  GEN6_FORMAT_R16G16B16A16_FLOAT,
                                  0, },
    [XGL_CH_FMT_R32] =          { GEN6_FORMAT_R32_UNORM,
                                  GEN6_FORMAT_R32_SNORM,
                                  GEN6_FORMAT_R32_UINT,
                                  GEN6_FORMAT_R32_SINT,
                                  GEN6_FORMAT_R32_FLOAT,
                                  0, },
    [XGL_CH_FMT_R32G32] =       { GEN6_FORMAT_R32G32_UNORM,
                                  GEN6_FORMAT_R32G32_SNORM,
                                  GEN6_FORMAT_R32G32_UINT,
                                  GEN6_FORMAT_R32G32_SINT,
                                  GEN6_FORMAT_R32G32_FLOAT,
                                  0, },
    [XGL_CH_FMT_R32G32B32] =    { GEN6_FORMAT_R32G32B32_UNORM,
                                  GEN6_FORMAT_R32G32B32_SNORM,
                                  GEN6_FORMAT_R32G32B32_UINT,
                                  GEN6_FORMAT_R32G32B32_SINT,
                                  GEN6_FORMAT_R32G32B32_FLOAT,
                                  0, },
    [XGL_CH_FMT_R32G32B32A32] = { GEN6_FORMAT_R32G32B32A32_UNORM,
                                  GEN6_FORMAT_R32G32B32A32_SNORM,
                                  GEN6_FORMAT_R32G32B32A32_UINT,
                                  GEN6_FORMAT_R32G32B32A32_SINT,
                                  GEN6_FORMAT_R32G32B32A32_FLOAT,
                                  0, },
    [XGL_CH_FMT_R9G9B9E5] =     { 0,
                                  0,
                                  0,
                                  0,
                                  GEN6_FORMAT_R9G9B9E5_SHAREDEXP,
                                  0, },
    [XGL_CH_FMT_BC1] =          { GEN6_FORMAT_BC1_UNORM,
                                  0,
                                  0,
                                  0,
                                  0,
                                  GEN6_FORMAT_BC1_UNORM_SRGB, },
    [XGL_CH_FMT_BC2] =          { GEN6_FORMAT_BC2_UNORM,
                                  0,
                                  0,
                                  0,
                                  0,
                                  GEN6_FORMAT_BC2_UNORM_SRGB, },
    [XGL_CH_FMT_BC3] =          { GEN6_FORMAT_BC3_UNORM,
                                  0,
                                  0,
                                  0,
                                  0,
                                  GEN6_FORMAT_BC3_UNORM_SRGB, },
    [XGL_CH_FMT_BC4] =          { GEN6_FORMAT_BC4_UNORM,
                                  GEN6_FORMAT_BC4_SNORM,
                                  0,
                                  0,
                                  0,
                                  0, },
    [XGL_CH_FMT_BC5] =          { GEN6_FORMAT_BC5_UNORM,
                                  GEN6_FORMAT_BC5_SNORM,
                                  0,
                                  0,
                                  0,
                                  0, },
    [XGL_CH_FMT_BC6U] =         { GEN6_FORMAT_BC6H_UF16,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0, },
    [XGL_CH_FMT_BC6S] =         { GEN6_FORMAT_BC6H_SF16,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0, },
    [XGL_CH_FMT_BC7] =          { GEN6_FORMAT_BC7_UNORM,
                                  0,
                                  0,
                                  0,
                                  0,
                                  GEN6_FORMAT_BC7_UNORM_SRGB, },
};

static int intel_format_translate_color(XGL_FORMAT format)
{
    int fmt;

    assert(format.numericFormat != XGL_NUM_FMT_UNDEFINED &&
           format.numericFormat != XGL_NUM_FMT_DS);

    fmt = intel_color_mapping[format.channelFormat][format.numericFormat - 1];

    /* GEN6_FORMAT_R32G32B32A32_FLOAT happens to be 0 */
    if (format.channelFormat == XGL_CH_FMT_R32G32B32A32 &&
        format.numericFormat == XGL_NUM_FMT_FLOAT)
        assert(fmt == 0);
    else if (!fmt)
        fmt = -1;

    return fmt;
}

static XGL_FLAGS intel_format_get_color_features(const struct intel_dev *dev,
                                                 XGL_FORMAT format)
{
    const int fmt = intel_format_translate_color(format);
    const struct intel_sampler_cap *sampler;
    const struct intel_dp_cap *dp;
    XGL_FLAGS features;

    if (fmt < 0)
        return 0;

    sampler = (fmt < ARRAY_SIZE(intel_sampler_caps)) ?
        &intel_sampler_caps[fmt] : NULL;
    dp = (fmt < ARRAY_SIZE(intel_dp_caps)) ?  &intel_dp_caps[fmt] : NULL;

    features = XGL_FORMAT_MEMORY_SHADER_ACCESS_BIT;

#define TEST(dev, func, cap) ((func) && (func)->cap && \
        intel_gpu_gen((dev)->gpu) >= (func)->cap)
    if (TEST(dev, sampler, sampling)) {
        if (format.numericFormat == XGL_NUM_FMT_UINT ||
            format.numericFormat == XGL_NUM_FMT_SINT ||
            TEST(dev, sampler, filtering))
            features |= XGL_FORMAT_IMAGE_SHADER_READ_BIT;
    }

    if (TEST(dev, dp, typed_write))
        features |= XGL_FORMAT_IMAGE_SHADER_WRITE_BIT;

    if (TEST(dev, dp, rt_write)) {
        features |= XGL_FORMAT_COLOR_ATTACHMENT_WRITE_BIT;

        if (TEST(dev, dp, rt_write_blending))
            features |= XGL_FORMAT_COLOR_ATTACHMENT_BLEND_BIT;

        if (features & XGL_FORMAT_IMAGE_SHADER_READ_BIT) {
            features |= XGL_FORMAT_IMAGE_COPY_BIT |
                        XGL_FORMAT_CONVERSION_BIT;
        }
    }
#undef TEST

    return features;
}

static XGL_FLAGS intel_format_get_ds_features(const struct intel_dev *dev,
                                              XGL_FORMAT format)
{
    XGL_FLAGS features;

    assert(format.numericFormat == XGL_NUM_FMT_DS);

    switch (format.channelFormat) {
    case XGL_CH_FMT_R8:
        features = XGL_FORMAT_STENCIL_ATTACHMENT_BIT;;
        break;
    case XGL_CH_FMT_R16:
    case XGL_CH_FMT_R32:
        features = XGL_FORMAT_DEPTH_ATTACHMENT_BIT;
        break;
    case XGL_CH_FMT_R32G8:
        features = XGL_FORMAT_DEPTH_ATTACHMENT_BIT |
                   XGL_FORMAT_STENCIL_ATTACHMENT_BIT;
        break;
    default:
        features = 0;
        break;
    }

    return features;
}

static XGL_FLAGS intel_format_get_raw_features(const struct intel_dev *dev,
                                               XGL_FORMAT format)
{
    assert(format.numericFormat == XGL_NUM_FMT_UNDEFINED);

    return (format.channelFormat == XGL_CH_FMT_UNDEFINED) ?
        XGL_FORMAT_MEMORY_SHADER_ACCESS_BIT : 0;
}

static void intel_format_get_props(const struct intel_dev *dev,
                                   XGL_FORMAT format,
                                   XGL_FORMAT_PROPERTIES *props)
{
    switch (format.numericFormat) {
    case XGL_NUM_FMT_UNDEFINED:
        props->linearTilingFeatures =
            intel_format_get_raw_features(dev, format);
        props->optimalTilingFeatures = 0;
        break;
    case XGL_NUM_FMT_UNORM:
    case XGL_NUM_FMT_SNORM:
    case XGL_NUM_FMT_UINT:
    case XGL_NUM_FMT_SINT:
    case XGL_NUM_FMT_FLOAT:
    case XGL_NUM_FMT_SRGB:
        props->linearTilingFeatures =
            intel_format_get_color_features(dev, format);
        props->optimalTilingFeatures = props->linearTilingFeatures;
        break;
    case XGL_NUM_FMT_DS:
        props->linearTilingFeatures = 0;
        props->optimalTilingFeatures =
            intel_format_get_ds_features(dev, format);
        break;
    default:
        props->linearTilingFeatures = 0;
        props->optimalTilingFeatures = 0;
        break;
    }
}

XGL_RESULT XGLAPI intelGetFormatInfo(
    XGL_DEVICE                                  device,
    XGL_FORMAT                                  format,
    XGL_FORMAT_INFO_TYPE                        infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData)
{
    const struct intel_dev *dev = intel_dev(device);
    XGL_RESULT ret = XGL_SUCCESS;

    switch (infoType) {
    case XGL_INFO_TYPE_FORMAT_PROPERTIES:
        *pDataSize = sizeof(XGL_FORMAT_PROPERTIES);
        intel_format_get_props(dev, format, pData);
        break;
    default:
        ret = XGL_ERROR_INVALID_VALUE;
        break;
    }

    return ret;
}
