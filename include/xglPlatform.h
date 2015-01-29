//
// File: xglPlatform.h
//
/*
** Copyright (c) 2014 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/


#ifndef __XGLPLATFORM_H__
#define __XGLPLATFORM_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
***************************************************************************************************
*   Platform-specific directives and type declarations
***************************************************************************************************
*/

#if defined(_WIN32)
    // On Windows, XGLAPI should equate to the __stdcall convention
    #define XGLAPI   __stdcall
#elif defined(__GNUC__)
    // On other platforms using GCC, XGLAPI stays undefined
    #define XGLAPI
#else
    // Unsupported Platform!
    #error "Unsupported OS Platform detected!"
#endif

#include <stddef.h>
#include <stdint.h>

typedef uint64_t   XGL_GPU_SIZE;
typedef uint32_t   bool32_t;

typedef uint32_t   XGL_SAMPLE_MASK;
typedef uint32_t   XGL_FLAGS;
typedef int32_t    XGL_ENUM;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __XGLPLATFORM_H__
