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

#ifndef SHADER_H
#define SHADER_H

#include "intel.h"
#include "obj.h"

enum intel_shader_use {
    INTEL_SHADER_USE_VID                = (1 << 0),
    INTEL_SHADER_USE_IID                = (1 << 1),

    INTEL_SHADER_USE_KILL               = (1 << 2),
    INTEL_SHADER_USE_COMPUTED_DEPTH     = (1 << 3),
    INTEL_SHADER_USE_DEPTH              = (1 << 4),
    INTEL_SHADER_USE_W                  = (1 << 5),
};

/* just the kernel now */
struct intel_ir {
    void *kernel;
    XGL_SIZE size;
};

struct intel_shader {
    struct intel_obj obj;

    struct intel_ir *ir;
    XGL_FLAGS uses;

    XGL_UINT in_count;
    XGL_UINT out_count;

    XGL_UINT sampler_count;
    XGL_UINT surface_count;

    XGL_UINT urb_grf_start;

    XGL_FLAGS barycentric_interps;
};

static inline struct intel_shader *intel_shader(XGL_SHADER shader)
{
    return (struct intel_shader *) shader;
}

static inline struct intel_shader *intel_shader_from_obj(struct intel_obj *obj)
{
    return (struct intel_shader *) obj;
}

XGL_RESULT XGLAPI intelCreateShader(
    XGL_DEVICE                                  device,
    const XGL_SHADER_CREATE_INFO*               pCreateInfo,
    XGL_SHADER*                                 pShader);

#endif /* SHADER_H */
