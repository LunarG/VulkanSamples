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

    // C99:
    #define STATIC_INLINE static
#elif defined(__GNUC__)
    // On other platforms using GCC, XGLAPI stays undefined
    #define XGLAPI

    // C99:
    #define STATIC_INLINE static inline
#else
    // Unsupported Platform!
    #error "Unsupported OS Platform detected!"
#endif

#include <stddef.h>

#if !defined(XGL_NO_STDINT_H)
    #if defined(_MSC_VER) && (_MSC_VER < 1600)
        typedef signed   __int8  int8_t;
        typedef unsigned __int8  uint8_t;
        typedef signed   __int16 int16_t;
        typedef unsigned __int16 uint16_t;
        typedef signed   __int32 int32_t;
        typedef unsigned __int32 uint32_t;
        typedef signed   __int64 int64_t;
        typedef unsigned __int64 uint64_t;
    #else
        #include <stdint.h>
    #endif
#endif // !defined(XGL_NO_STDINT_H)

typedef uint64_t   XGL_GPU_SIZE;
typedef uint32_t   bool32_t;

typedef uint32_t   XGL_SAMPLE_MASK;
typedef uint32_t   XGL_FLAGS;
typedef int32_t    XGL_ENUM;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __XGLPLATFORM_H__
