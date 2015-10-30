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
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 *   Chia-I Wu <olv@lunarg.com>
 */

#ifndef SHADER_H
#define SHADER_H

#include "intel.h"
#include "obj.h"

struct intel_ir;

struct intel_shader_module {
    struct intel_obj obj;

    const struct intel_gpu *gpu;
    /* content is just a copy of the SPIRV image */
    uint32_t code_size;
    void *code;

    /* simple cache */
    struct intel_ir *vs;
    struct intel_ir *tcs;
    struct intel_ir *tes;
    struct intel_ir *gs;
    struct intel_ir *fs;
    struct intel_ir *cs;
};

static inline struct intel_shader_module *intel_shader_module(VkShaderModule shaderModule)
{
    return *(struct intel_shader_module **) &shaderModule;
}

static inline struct intel_shader_module *intel_shader_module_from_obj(struct intel_obj *obj)
{
    return (struct intel_shader_module *)obj;
}

const struct intel_ir *intel_shader_module_get_ir(struct intel_shader_module *sm,
                                                  VkShaderStageFlagBits stage);

#endif /* SHADER_H */
