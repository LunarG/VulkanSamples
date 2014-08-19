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
#include "dev.h"

typedef enum _SHADER_TYPE
{
    SHADER_TYPE_VERTEX                                    = 0x0,
    SHADER_TYPE_FRAGMENT                                  = 0x1,
    SHADER_TYPE_GEOMETRY                                  = 0x2,
    SHADER_TYPE_COMPUTE                                   = 0x3,
    SHADER_TYPE_TESS_CONTROL                              = 0x4,
    SHADER_TYPE_TESS_EVALUATION                           = 0x5
} INTEL_SHADER_TYPE;

struct intel_shader {
    struct intel_obj obj;
    struct intel_dev *dev;

    void *pCode;
    uint32_t codeSize;
};

XGL_RESULT XGLAPI intelCreateShader(
    XGL_DEVICE                                  device,
    const XGL_SHADER_CREATE_INFO*               pCreateInfo,
    XGL_SHADER*                                 pShader);

static inline struct intel_shader *intel_shader(XGL_SHADER shader)
{
    return (struct intel_shader *) shader;
}

static inline struct intel_shader *intel_shader_from_obj(struct intel_obj *obj)
{
    return (struct intel_shader *) obj;
}

#endif // SHADER_H
