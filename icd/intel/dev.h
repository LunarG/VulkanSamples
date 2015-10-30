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

#ifndef DEV_H
#define DEV_H

#include "intel.h"
#include "gpu.h"
#include "obj.h"

struct intel_desc_region;
struct intel_pipeline_shader;
struct intel_queue;
struct intel_winsys;

enum intel_dev_meta_shader {
    /*
     * This expects an ivec2 to be pushed:
     *
     *  .x is memory offset
     *  .y is fill value
     *
     * as well as GEN6_VFCOMP_STORE_VID.
     */
    INTEL_DEV_META_VS_FILL_MEM,

    /*
     * These expect an ivec2 to be pushed:
     *
     *  .x is dst memory offset
     *  .y is src memory offset
     *
     * as well as GEN6_VFCOMP_STORE_VID.
     */
    INTEL_DEV_META_VS_COPY_MEM,
    INTEL_DEV_META_VS_COPY_MEM_UNALIGNED,

    /*
     * This expects an ivec4 to be pushed:
     *
     *  .xy is added to fargment coord to form (u, v)
     *  .z is extent width
     *  .w is dst memory offset
     *
     * as well as GEN6_VFCOMP_STORE_VID.
     */
    INTEL_DEV_META_VS_COPY_R8_TO_MEM,
    INTEL_DEV_META_VS_COPY_R16_TO_MEM,
    INTEL_DEV_META_VS_COPY_R32_TO_MEM,
    INTEL_DEV_META_VS_COPY_R32G32_TO_MEM,
    INTEL_DEV_META_VS_COPY_R32G32B32A32_TO_MEM,

    /*
     * These expect an ivec4 to be pushed:
     *
     *  .xy is added to fragment coord to form (u, v)
     *  .z is ai
     *  .w is lod
     */
    INTEL_DEV_META_FS_COPY_MEM,             /* ld_lz(u)             */
    INTEL_DEV_META_FS_COPY_1D,              /* ld(u, lod)           */
    INTEL_DEV_META_FS_COPY_1D_ARRAY,        /* ld(u, lod, ai)       */
    INTEL_DEV_META_FS_COPY_2D,              /* ld(u, lod, v)        */
    INTEL_DEV_META_FS_COPY_2D_ARRAY,        /* ld(u, lod, v, ai)    */
    INTEL_DEV_META_FS_COPY_2D_MS,           /* ld_mcs() + ld2dms()  */

    /*
     * These expect a second ivec4 to be pushed:
     *
     *  .x is memory offset
     *  .y is extent width
     *
     * The second ivec4 is to convert linear fragment coord to (u, v).
     */
    INTEL_DEV_META_FS_COPY_1D_TO_MEM,       /* ld(u, lod)           */
    INTEL_DEV_META_FS_COPY_1D_ARRAY_TO_MEM, /* ld(u, lod, ai)       */
    INTEL_DEV_META_FS_COPY_2D_TO_MEM,       /* ld(u, lod, v)        */
    INTEL_DEV_META_FS_COPY_2D_ARRAY_TO_MEM, /* ld(u, lod, v, ai)    */
    INTEL_DEV_META_FS_COPY_2D_MS_TO_MEM,    /* ld_mcs() + ld2dms()  */

    /*
     * This expects an ivec4 to be pushed:
     *
     *  .xy is added to fargment coord to form (u, v)
     *  .z is extent width
     *
     * .z is used to linearize (u, v).
     */
    INTEL_DEV_META_FS_COPY_MEM_TO_IMG,      /* ld_lz(u)             */

    /*
     * These expect the clear value to be pushed, and set fragment color or
     * depth to the clear value.
     */
    INTEL_DEV_META_FS_CLEAR_COLOR,
    INTEL_DEV_META_FS_CLEAR_DEPTH,

    /*
     * These expect an ivec4 to be pushed:
     *
     *  .xy is added to fragment coord to form (u, v)
     *
     * All samples are fetched and averaged.  The fragment color is set to the
     * averaged value.
     */
    INTEL_DEV_META_FS_RESOLVE_2X,
    INTEL_DEV_META_FS_RESOLVE_4X,
    INTEL_DEV_META_FS_RESOLVE_8X,
    INTEL_DEV_META_FS_RESOLVE_16X,

    INTEL_DEV_META_SHADER_COUNT,
};

struct intel_dev_dbg {
    struct intel_base_dbg base;
};

struct intel_dev {
    struct intel_base base;

    bool phy_dev_exts[INTEL_PHY_DEV_EXT_COUNT];

    struct intel_gpu *gpu;
    struct intel_winsys *winsys;

    struct intel_bo *cmd_scratch_bo;
    struct intel_pipeline_shader *cmd_meta_shaders[INTEL_DEV_META_SHADER_COUNT];

    struct intel_desc_region *desc_region;

    uint32_t sample_pattern_1x;
    uint32_t sample_pattern_2x;
    uint32_t sample_pattern_4x;
    uint32_t sample_pattern_8x[2];
    uint32_t sample_pattern_16x[4];

    struct intel_queue *queues[INTEL_GPU_ENGINE_COUNT];
};

static inline struct intel_dev *intel_dev(VkDevice dev)
{
    return (struct intel_dev *) dev;
}

static inline struct intel_dev_dbg *intel_dev_dbg(struct intel_dev *dev)
{
    return (struct intel_dev_dbg *) dev->base.dbg;
}

VkResult intel_dev_create(struct intel_gpu *gpu,
                            const VkDeviceCreateInfo *info,
                            struct intel_dev **dev_ret);
void intel_dev_destroy(struct intel_dev *dev);

void intel_dev_log(struct intel_dev *dev,
                   VkFlags msg_flags,
                   struct intel_base *src_object,
                   size_t location,
                   int32_t msg_code,
                   const char *format, ...);

static inline const struct intel_pipeline_shader *intel_dev_get_meta_shader(const struct intel_dev *dev,
                                                                            enum intel_dev_meta_shader id)
{
    assert(id < INTEL_DEV_META_SHADER_COUNT);
    return dev->cmd_meta_shaders[id];
}

#endif /* DEV_H */
