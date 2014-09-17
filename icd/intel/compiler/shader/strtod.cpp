/*
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <stdlib.h>

#ifdef _GNU_SOURCE
#include <locale.h>
#ifdef __APPLE__
#include <xlocale.h>
#endif
#endif

#include "strtod.h"

#if defined(_GNU_SOURCE) && !defined(__CYGWIN__) && !defined(__FreeBSD__) && \
   !defined(__HAIKU__) && !defined(__UCLIBC__)
#define GLSL_HAVE_LOCALE_T
#endif

#ifdef GLSL_HAVE_LOCALE_T
static struct locale_initializer {
   locale_initializer() { loc = newlocale(LC_CTYPE_MASK, "C", NULL); }
   locale_t loc;
} loc_init;
#endif


/**
 * Wrapper around strtod which uses the "C" locale so the decimal
 * point is always '.'
 */
double
glsl_strtod(const char *s, char **end)
{
#ifdef GLSL_HAVE_LOCALE_T
   return strtod_l(s, end, loc_init.loc);
#else
   return strtod(s, end);
#endif
}


/**
 * Wrapper around strtof which uses the "C" locale so the decimal
 * point is always '.'
 */
float
glsl_strtof(const char *s, char **end)
{
#ifdef GLSL_HAVE_LOCALE_T
   return strtof_l(s, end, loc_init.loc);
#elif _XOPEN_SOURCE >= 600 || _ISOC99_SOURCE
   return strtof(s, end);
#else
   return (float) strtod(s, end);
#endif
}
