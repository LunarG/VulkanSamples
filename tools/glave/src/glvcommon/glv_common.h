/**************************************************************************
 *
 * Copyright 2014 Valve Software
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/

#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "glv_platform.h"

#include "glv_memory.h"
#include "glv_tracelog.h"

#if defined(WIN32)

#define GLVTRACER_EXPORT __declspec(dllexport)
#define GLVTRACER_STDCALL __stdcall
#define GLVTRACER_CDECL __cdecl
#define GLVTRACER_EXIT void __cdecl
#define GLVTRACER_ENTRY void
#define GLVTRACER_LEAVE void

#elif defined(PLATFORM_LINUX)

#define GLVTRACER_EXPORT __attribute__ ((visibility ("default")))
#define GLVTRACER_STDCALL
#define GLVTRACER_CDECL
#define GLVTRACER_EXIT void
#define GLVTRACER_ENTRY void __attribute__ ((constructor))
#define GLVTRACER_LEAVE void __attribute__ ((destructor))

#endif
