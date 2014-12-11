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

#include "shader.h"
#include "pipeline_priv.h"
#include "compiler/pipeline/pipeline_compiler_interface.h"

static XGL_RESULT pipeline_build_shader(struct intel_pipeline *pipeline,
                                        struct intel_pipeline_shader *sh,
                                        const XGL_PIPELINE_SHADER *sh_info)
{
    XGL_RESULT ret;

    ret = intel_pipeline_shader_compile(sh, pipeline->dev->gpu, sh_info);
    if (ret != XGL_SUCCESS)
        return ret;

    sh->max_threads =
        intel_gpu_get_max_threads(pipeline->dev->gpu, sh_info->stage);

    /* 1KB aligned */
    sh->scratch_offset = u_align(pipeline->scratch_size, 1024);
    pipeline->scratch_size = sh->scratch_offset +
        sh->per_thread_scratch_size * sh->max_threads;

    pipeline->active_shaders |= 1 << sh_info->stage;

    return XGL_SUCCESS;
}

XGL_RESULT pipeline_build_shaders(struct intel_pipeline *pipeline,
                                  const struct intel_pipeline_create_info *info)
{
    XGL_RESULT ret = XGL_SUCCESS;

    if (ret == XGL_SUCCESS && info->vs.shader)
        ret = pipeline_build_shader(pipeline, &pipeline->vs, &info->vs);
    if (ret == XGL_SUCCESS && info->tcs.shader)
        ret = pipeline_build_shader(pipeline, &pipeline->tcs, &info->tcs);
    if (ret == XGL_SUCCESS && info->tes.shader)
        ret = pipeline_build_shader(pipeline, &pipeline->tes, &info->tes);
    if (ret == XGL_SUCCESS && info->gs.shader)
        ret = pipeline_build_shader(pipeline, &pipeline->gs, &info->gs);
    if (ret == XGL_SUCCESS && info->fs.shader)
        ret = pipeline_build_shader(pipeline, &pipeline->fs, &info->fs);

    if (ret == XGL_SUCCESS && info->compute.cs.shader)
        ret = pipeline_build_shader(pipeline, &pipeline->cs, &info->compute.cs);

    return ret;
}

void pipeline_tear_shaders(struct intel_pipeline *pipeline)
{
    if (pipeline->active_shaders & SHADER_VERTEX_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->vs);
    }

    if (pipeline->active_shaders & SHADER_TESS_CONTROL_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->tcs);
    }

    if (pipeline->active_shaders & SHADER_TESS_EVAL_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->tes);
    }

    if (pipeline->active_shaders & SHADER_GEOMETRY_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->gs);
    }

    if (pipeline->active_shaders & SHADER_FRAGMENT_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->fs);
    }

    if (pipeline->active_shaders & SHADER_COMPUTE_FLAG) {
        intel_pipeline_shader_cleanup(&pipeline->cs);
    }
}

struct intel_pipeline_shader *intel_pipeline_shader_create_meta(struct intel_dev *dev,
                                                                enum intel_dev_meta_shader id)
{
    struct intel_pipeline_shader *sh;
    XGL_RESULT ret;

    sh = icd_alloc(sizeof(*sh), 0, XGL_SYSTEM_ALLOC_INTERNAL);
    if (!sh)
        return NULL;
    memset(sh, 0, sizeof(*sh));

    ret = intel_pipeline_shader_compile_meta(sh, dev->gpu, id);
    if (ret != XGL_SUCCESS) {
        icd_free(sh);
        return NULL;
    }

    switch (id) {
    case INTEL_DEV_META_VS_FILL_MEM:
    case INTEL_DEV_META_VS_COPY_MEM:
    case INTEL_DEV_META_VS_COPY_MEM_UNALIGNED:
        sh->max_threads = intel_gpu_get_max_threads(dev->gpu,
                XGL_SHADER_STAGE_VERTEX);
        break;
    default:
        sh->max_threads = intel_gpu_get_max_threads(dev->gpu,
                XGL_SHADER_STAGE_FRAGMENT);
        break;
    }

    return sh;
}

void intel_pipeline_shader_destroy(struct intel_pipeline_shader *sh)
{
    intel_pipeline_shader_cleanup(sh);
    icd_free(sh);
}
