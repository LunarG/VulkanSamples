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
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ICD_UTILS_H
#define ICD_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#if defined(PLATFORM_LINUX)
#include <strings.h> /* for ffs() */
#endif
#include "icd.h"

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#if defined(__GNUC__)
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (x)
#define unlikely(x) (x)
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define u_popcount(u) __builtin_popcount(u)
#define u_popcountll(u) __builtin_popcountll(u)

#define STATIC_ASSERT(expr) do {            \
    (void) sizeof(char[1 - 2 * !(expr)]);   \
} while (0)

#define U_CLAMP(val, min, max) \
    ((val) < (min) ? (min) : (val) > (max)? (max) : (val))

/**
 * Return true if val is power of two, or zero.
 */
static inline bool u_is_pow2(unsigned int val)
{
    return ((val & (val - 1)) == 0);
}

static inline int u_ffs(int val)
{
#if defined(PLATFORM_LINUX)
	return ffs(val);
#else
	return __lzcnt(val) + 1;
#endif
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

static inline uint32_t u_fui(float f)
{
    union {
        float f;
        uint32_t ui;
    } u = { .f = f };

    return u.ui;
}

static inline float u_uif(uint32_t ui)
{
    union {
        float f;
        uint32_t ui;
    } u = { .ui = ui };

    return u.f;
}

static inline int u_iround(float f)
{
    if (f >= 0.0f)
        return (int) (f + 0.5f);
    else
        return (int) (f - 0.5f);
}

uint16_t u_float_to_half(float f);

#endif /* ICD_UTILS_H */
