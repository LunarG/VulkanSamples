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

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__GNUC__) || defined(__SCO__) || defined(__USLC__)

/*
 * Using <stdint.h>
 */
#include <stdint.h>
typedef int8_t           XGL_INT8;
typedef uint8_t          XGL_UINT8;
typedef int16_t          XGL_INT16;
typedef uint16_t         XGL_UINT16;
typedef int32_t          XGL_INT32;
typedef uint32_t         XGL_UINT32;
typedef int64_t          XGL_INT64;
typedef uint64_t         XGL_UINT64;

#elif defined(_WIN32) && !defined(__SCITECH_SNAP__)

/*
 * Win32
 */
typedef signed   __int8  XGL_INT8;
typedef unsigned __int8  XGL_UINT8;
typedef signed   __int16 XGL_INT16;
typedef unsigned __int16 XGL_UINT16;
typedef signed   __int32 XGL_INT32;
typedef unsigned __int32 XGL_UINT32;
typedef signed   __int64 XGL_INT64;
typedef unsigned __int64 XGL_UINT64;

#endif

typedef size_t     XGL_SIZE;
typedef XGL_UINT64 XGL_GPU_SIZE;
typedef XGL_UINT8  XGL_BYTE;
typedef XGL_INT32  XGL_INT;
typedef XGL_UINT32 XGL_UINT;
typedef char       XGL_CHAR;
typedef float      XGL_FLOAT;
typedef double     XGL_DOUBLE;
typedef XGL_UINT32 XGL_BOOL;
typedef void       XGL_VOID;

typedef XGL_UINT32 XGL_SAMPLE_MASK;
typedef XGL_UINT32 XGL_FLAGS;
typedef XGL_INT32  XGL_ENUM;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __XGLPLATFORM_H__
