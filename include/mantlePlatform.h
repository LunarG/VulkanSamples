//
// File: mantlePlatform.h
//
// Copyright 2014 ADVANCED MICRO DEVICES, INC.  All Rights Reserved.
//
// AMD is granting you permission to use this software for reference
// purposes only and not for use in any software product.
//
// You agree that you will not reverse engineer or decompile the Materials,
// in whole or in part, except as allowed by applicable law.
//
// WARRANTY DISCLAIMER: THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND.  AMD DISCLAIMS ALL WARRANTIES, EXPRESS, IMPLIED, OR STATUTORY,
// INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE, NON-INFRINGEMENT, THAT THE SOFTWARE
// WILL RUN UNINTERRUPTED OR ERROR-FREE OR WARRANTIES ARISING FROM CUSTOM OF
// TRADE OR COURSE OF USAGE.  THE ENTIRE RISK ASSOCIATED WITH THE USE OF THE
// SOFTWARE IS ASSUMED BY YOU.
// Some jurisdictions do not allow the exclusion of implied warranties, so
// the above exclusion may not apply to You.
//
// LIMITATION OF LIABILITY AND INDEMNIFICATION:  AMD AND ITS LICENSORS WILL
// NOT, UNDER ANY CIRCUMSTANCES BE LIABLE TO YOU FOR ANY PUNITIVE, DIRECT,
// INCIDENTAL, INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES ARISING FROM USE OF
// THE SOFTWARE OR THIS AGREEMENT EVEN IF AMD AND ITS LICENSORS HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
// In no event shall AMD's total liability to You for all damages, losses,
// and causes of action (whether in contract, tort (including negligence) or
// otherwise) exceed the amount of $100 USD.  You agree to defend, indemnify
// and hold harmless AMD and its licensors, and any of their directors,
// officers, employees, affiliates or agents from and against any and all
// loss, damage, liability and other expenses (including reasonable attorneys'
// fees), resulting from Your use of the Software or violation of the terms and
// conditions of this Agreement.
//
// U.S. GOVERNMENT RESTRICTED RIGHTS: The Materials are provided with "RESTRICTED
// RIGHTS." Use, duplication, or disclosure by the Government is subject to the
// restrictions as set forth in FAR 52.227-14 and DFAR252.227-7013, et seq., or
// its successor.  Use of the Materials by the Government constitutes
// acknowledgement of AMD's proprietary rights in them.
//
// EXPORT RESTRICTIONS: The Materials may be subject to export restrictions as
// stated in the Software License Agreement.
//

#ifndef __MANTLEPLATFORM_H__
#define __MANTLEPLATFORM_H__

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
    // On Windows, GR_STDCALL should equate to the __stdcall convention
    #define GR_STDCALL   __stdcall
#elif defined(__GNUC__)
    // On other platforms using GCC, GR_STDCALL stays undefined
    #define GR_STDCALL
#else
    // Unsupported Platform!
    #error "Unsupported OS Platform detected!"
#endif

#if defined(__GNUC__)
    #include <stddef.h>
#endif

#if defined(_WIN32)
    // Windows platform
    typedef unsigned __int8  GR_UINT8;
    typedef signed   __int32 GR_INT32;
    typedef unsigned __int32 GR_UINT32;
    typedef signed   __int64 GR_INT64;
    typedef unsigned __int64 GR_UINT64;
#elif defined(__GNUC__)
    // Other platforms
    typedef uint8_t          GR_UINT8;
    typedef int32_t          GR_INT32;
    typedef uint32_t         GR_UINT32;
    typedef int64_t          GR_INT64;
    typedef uint64_t         GR_UINT64;
#endif

typedef size_t    GR_SIZE;
typedef GR_UINT64 GR_GPU_SIZE;
typedef GR_UINT8  GR_BYTE;
typedef GR_INT32  GR_INT;
typedef GR_UINT32 GR_UINT;
typedef char      GR_CHAR;
typedef float     GR_FLOAT;
typedef double    GR_DOUBLE;
typedef GR_UINT32 GR_BOOL;
typedef void      GR_VOID;

typedef GR_UINT32 GR_SAMPLE_MASK;
typedef GR_UINT32 GR_FLAGS;
typedef GR_INT32  GR_ENUM;
typedef GR_INT32  GR_RESULT;

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // __MANTLEPLATFORM_H__
