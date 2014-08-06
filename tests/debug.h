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
 * Many function below borrowed from Mesa u_debug.h
 */

#ifndef DEBUG_H
#define DEBUG_H

static inline void
debug_printf(const char *format, ...)
{
#ifdef DEBUG
   va_list ap;
   va_start(ap, format);
   _debug_vprintf(format, ap);
   va_end(ap);
#else
   (void) format; /* silence warning */
#endif
}

/**
 * Assert macro
 *
 * Do not expect that the assert call terminates -- errors must be handled
 * regardless of assert behavior.
 *
 * For non debug builds the assert macro will expand to a no-op, so do not
 * call functions with side effects in the assert expression.
 */
#ifdef DEBUG
#define debug_assert(expr) ((expr) ? (void)0 : _debug_assert_fail(#expr, __FILE__, __LINE__, __FUNCTION__))
#else
#define debug_assert(expr) do { } while (0 && (expr))
#endif


/** Override standard assert macro */
#ifdef assert
#undef assert
#endif
#define assert(expr) debug_assert(expr)


/**
 * Output the current function name.
 */
#ifdef DEBUG
#define debug_checkpoint() \
   _debug_printf("%s\n", __FUNCTION__)
#else
#define debug_checkpoint() \
   ((void)0)
#endif


/**
 * Output the full source code position.
 */
#ifdef DEBUG
#define debug_checkpoint_full() \
   _debug_printf("%s:%u:%s\n", __FILE__, __LINE__, __FUNCTION__)
#else
#define debug_checkpoint_full() \
   ((void)0)
#endif


#endif // DEBUG_H
