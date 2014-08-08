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

#ifndef INTEL_H
#define INTEL_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <xgl.h>
#include <xglDbg.h>

#include "icd.h"

#define INTEL_API_VERSION XGL_MAKE_VERSION(0, 22, 0)
#define INTEL_DRIVER_VERSION 0

#define INTEL_GEN(gen) ((int) ((gen) * 100))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

/**
 * Return true if val is power of two, or zero.
 */
static inline bool u_is_pow2(unsigned int val)
{
    return ((val & (val - 1)) == 0);
}

static inline unsigned int u_align(unsigned int val, unsigned alignment)
{
    assert(alignment && u_is_pow2(alignment));
    return (val + alignment - 1) & ~(alignment - 1);
}

static inline unsigned int u_minify(unsigned int val, unsigned level)
{
    return (val >> level) ? val >> level : 1;
}

#endif /* INTEL_H */
