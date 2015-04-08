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
 *   LunarG
 */

#ifndef PIPELINE_COMPILER_INTERFACE_H
#define PIPELINE_COMPILER_INTERFACE_H


#ifdef __cplusplus
extern "C" {
#endif

#include "dev.h"

struct brw_context;
struct intel_desc_layout_chain;
struct intel_gpu;
struct intel_ir;
struct intel_pipeline_shader;

struct brw_context *intel_create_brw_context(const struct intel_gpu *gpu);
void intel_destroy_brw_context(struct brw_context *brw);

VK_RESULT intel_pipeline_shader_compile(struct intel_pipeline_shader *ips,
                                         const struct intel_gpu *gpu,
                                         const struct intel_desc_layout_chain *chain,
                                         const VK_PIPELINE_SHADER *info);

void intel_pipeline_shader_cleanup(struct intel_pipeline_shader *sh,
                                   const struct intel_gpu *gpu);

VK_RESULT intel_pipeline_shader_compile_meta(struct intel_pipeline_shader *sh,
                                              const struct intel_gpu *gpu,
                                              enum intel_dev_meta_shader id);

void intel_disassemble_kernel(const struct intel_gpu *gpu,
                              const void *kernel, size_t size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* PIPELINE_COMPILER_INTERFACE_H */
