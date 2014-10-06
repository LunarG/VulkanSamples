//
// File: xgl.h
//
// Copyright 2014 ADVANCED MICRO DEVICES, INC.  All Rights Reserved.
//
// AMD is granting you permission to use this software and documentation (if
// any) (collectively, the "Materials") pursuant to the terms and conditions
// of the Software License Agreement included with the Materials.  If you do
// not have a copy of the Software License Agreement, contact your AMD
// representative for a copy.
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

#ifndef __XGL_H__
#define __XGL_H__

#include <stdint.h>

#define XGL_MAKE_VERSION(major, minor, patch) \
    ((major << 22) | (minor << 12) | patch)

// XGL API version supported by this file
#define XGL_API_VERSION XGL_MAKE_VERSION(0, 22, 0)

#ifndef XGLAPI
   #define XGLAPI
#endif

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
 * Datatypes
 */
typedef unsigned char   XGL_BOOL;
typedef void            XGL_VOID;
typedef signed char     XGL_CHAR;       /* 1-byte signed */
typedef int32_t         XGL_INT;        /* 4-byte signed */
typedef int32_t         XGL_INT32;      /* 4-byte signed */
typedef uint32_t        XGL_UINT;       /* 4-byte unsigned */
typedef uint32_t        XGL_UINT32;     /* 4-byte unsigned */
typedef uint64_t        XGL_UINT64;     /* 8-byte unsigned */
typedef uint32_t        XGL_SIZE;       /* 4-byte unsigned */
typedef uint32_t        XGL_GPU_SIZE;   /* 4-byte unsigned */
typedef uint32_t        XGL_FLAGS;      /* 4-byte unsigned */
typedef uint32_t        XGL_SAMPLE_MASK; /* 4-byte unsigned */
typedef uint8_t         XGL_UINT8;      /* 1-byte unsigned */
typedef float           XGL_FLOAT;      /* single precision float */
typedef double          XGL_DOUBLE;       /* double precision float in [0,1] */

/*
***************************************************************************************************
*   Core XGL API
***************************************************************************************************
*/

#ifdef __cplusplus
    #define XGL_DEFINE_HANDLE(_obj) struct _obj##_T {}; typedef _obj##_T* _obj;
    #define XGL_DEFINE_SUBCLASS_HANDLE(_obj, _base) struct _obj##_T : public _base##_T {}; typedef _obj##_T* _obj;
#else // __cplusplus
    #define XGL_DEFINE_HANDLE(_obj) typedef void* _obj;
    #define XGL_DEFINE_SUBCLASS_HANDLE(_obj, _base) typedef void* _obj;
#endif // __cplusplus

XGL_DEFINE_HANDLE(XGL_PHYSICAL_GPU)
XGL_DEFINE_HANDLE(XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DEVICE, XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_QUEUE, XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_GPU_MEMORY, XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_OBJECT, XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_IMAGE, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_IMAGE_VIEW, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_COLOR_ATTACHMENT_VIEW, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DEPTH_STENCIL_VIEW, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_SHADER, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_PIPELINE, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_PIPELINE_DELTA, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_SAMPLER, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DESCRIPTOR_SET, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_STATE_OBJECT, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_VIEWPORT_STATE_OBJECT, XGL_STATE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_RASTER_STATE_OBJECT, XGL_STATE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_MSAA_STATE_OBJECT, XGL_STATE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_COLOR_BLEND_STATE_OBJECT, XGL_STATE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DEPTH_STENCIL_STATE_OBJECT, XGL_STATE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_CMD_BUFFER, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_FENCE, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_QUEUE_SEMAPHORE, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_EVENT, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_QUERY_POOL, XGL_OBJECT)

#define XGL_MAX_PHYSICAL_GPUS       16
#define XGL_MAX_PHYSICAL_GPU_NAME   256
#define XGL_MAX_MEMORY_HEAPS        8
#define XGL_MAX_DESCRIPTOR_SETS     2
#define XGL_MAX_VIEWPORTS           16
#define XGL_MAX_COLOR_ATTACHMENTS   8

#define XGL_LOD_CLAMP_NONE       MAX_FLOAT
#define XGL_LAST_MIP_OR_SLICE    0xffffffff

#define XGL_TRUE  1
#define XGL_FALSE 0

#define XGL_NULL_HANDLE 0

// This macro defines MAX_UINT in enumerations to force compilers to use 32 bits
// to represent them. This may or may not be necessary on some compilers. The
// option to compile it out may allow compilers that warn about missing enumerants
// in switch statements to be silenced.
#define XGL_MAX_ENUM(T) T##_MAX_ENUM = 0xFFFFFFFF

// ------------------------------------------------------------------------------------------------
// Enumerations


typedef enum _XGL_QUEUE_TYPE
{
    XGL_QUEUE_TYPE_GRAPHICS                                 = 0x1,
    XGL_QUEUE_TYPE_COMPUTE                                  = 0x2,
    XGL_QUEUE_TYPE_DMA                                      = 0x3,
    XGL_MAX_ENUM(_XGL_QUEUE_TYPE)
} XGL_QUEUE_TYPE;

typedef enum _XGL_MEMORY_PRIORITY
{
    XGL_MEMORY_PRIORITY_UNUSED                              = 0x0,
    XGL_MEMORY_PRIORITY_VERY_LOW                            = 0x1,
    XGL_MEMORY_PRIORITY_LOW                                 = 0x2,
    XGL_MEMORY_PRIORITY_NORMAL                              = 0x3,
    XGL_MEMORY_PRIORITY_HIGH                                = 0x4,
    XGL_MEMORY_PRIORITY_VERY_HIGH                           = 0x5,

    XGL_MEMORY_PRIORITY_BEGIN_RANGE                         = XGL_MEMORY_PRIORITY_UNUSED,
    XGL_MEMORY_PRIORITY_END_RANGE                           = XGL_MEMORY_PRIORITY_VERY_HIGH,
    XGL_NUM_MEMORY_PRIORITY                                 = (XGL_MEMORY_PRIORITY_END_RANGE - XGL_MEMORY_PRIORITY_BEGIN_RANGE + 1),
} XGL_MEMORY_PRIORITY;

typedef enum _XGL_MEMORY_STATE
{
    XGL_MEMORY_STATE_DATA_TRANSFER                          = 0x00000000,
    XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY              = 0x00000001,
    XGL_MEMORY_STATE_GRAPHICS_SHADER_WRITE_ONLY             = 0x00000002,
    XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_WRITE             = 0x00000003,
    XGL_MEMORY_STATE_COMPUTE_SHADER_READ_ONLY               = 0x00000004,
    XGL_MEMORY_STATE_COMPUTE_SHADER_WRITE_ONLY              = 0x00000005,
    XGL_MEMORY_STATE_COMPUTE_SHADER_READ_WRITE              = 0x00000006,
    XGL_MEMORY_STATE_MULTI_SHADER_READ_ONLY                 = 0x00000007,
    XGL_MEMORY_STATE_INDEX_DATA                             = 0x00000008,
    XGL_MEMORY_STATE_INDIRECT_ARG                           = 0x00000009,
    XGL_MEMORY_STATE_WRITE_TIMESTAMP                        = 0x0000000A,
    XGL_MEMORY_STATE_QUEUE_ATOMIC                           = 0x0000000B,

    XGL_MEMORY_STATE_BEGIN_RANGE                            = XGL_MEMORY_STATE_DATA_TRANSFER,
    XGL_MEMORY_STATE_END_RANGE                              = XGL_MEMORY_STATE_QUEUE_ATOMIC,
    XGL_NUM_MEMORY_STATE                                    = (XGL_MEMORY_STATE_END_RANGE - XGL_MEMORY_STATE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_MEMORY_STATE)
} XGL_MEMORY_STATE;

typedef enum _XGL_IMAGE_STATE
{
    XGL_IMAGE_STATE_DATA_TRANSFER                           = 0x00000000,
    XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY               = 0x00000001,
    XGL_IMAGE_STATE_GRAPHICS_SHADER_WRITE_ONLY              = 0x00000002,
    XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_WRITE              = 0x00000003,
    XGL_IMAGE_STATE_COMPUTE_SHADER_READ_ONLY                = 0x00000004,
    XGL_IMAGE_STATE_COMPUTE_SHADER_WRITE_ONLY               = 0x00000005,
    XGL_IMAGE_STATE_COMPUTE_SHADER_READ_WRITE               = 0x00000006,
    XGL_IMAGE_STATE_MULTI_SHADER_READ_ONLY                  = 0x00000007,
    XGL_IMAGE_STATE_TARGET_AND_SHADER_READ_ONLY             = 0x00000008,
    XGL_IMAGE_STATE_UNINITIALIZED_TARGET                    = 0x00000009,
    XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL            = 0x0000000A,
    XGL_IMAGE_STATE_TARGET_SHADER_ACCESS_OPTIMAL            = 0x0000000B,
    XGL_IMAGE_STATE_CLEAR                                   = 0x0000000C,
    XGL_IMAGE_STATE_RESOLVE_SOURCE                          = 0x0000000D,
    XGL_IMAGE_STATE_RESOLVE_DESTINATION                     = 0x0000000E,

    XGL_IMAGE_STATE_BEGIN_RANGE                             = XGL_IMAGE_STATE_DATA_TRANSFER,
    XGL_IMAGE_STATE_END_RANGE                               = XGL_IMAGE_STATE_RESOLVE_DESTINATION,
    XGL_NUM_IMAGE_STATE                                     = (XGL_IMAGE_STATE_END_RANGE - XGL_IMAGE_STATE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_IMAGE_STATE)
} XGL_IMAGE_STATE;

typedef enum _XGL_IMAGE_TYPE
{
    XGL_IMAGE_1D                                            = 0x00000000,
    XGL_IMAGE_2D                                            = 0x00000001,
    XGL_IMAGE_3D                                            = 0x00000002,

    XGL_IMAGE_TYPE_BEGIN_RANGE                              = XGL_IMAGE_1D,
    XGL_IMAGE_TYPE_END_RANGE                                = XGL_IMAGE_3D,
    XGL_NUM_IMAGE_TYPE                                      = (XGL_IMAGE_TYPE_END_RANGE - XGL_IMAGE_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_IMAGE_TYPE)
} XGL_IMAGE_TYPE;

typedef enum _XGL_IMAGE_TILING
{
    XGL_LINEAR_TILING                                       = 0x00000000,
    XGL_OPTIMAL_TILING                                      = 0x00000001,

    XGL_IMAGE_TILING_BEGIN_RANGE                            = XGL_LINEAR_TILING,
    XGL_IMAGE_TILING_END_RANGE                              = XGL_OPTIMAL_TILING,
    XGL_NUM_IMAGE_TILING                                    = (XGL_IMAGE_TILING_END_RANGE - XGL_IMAGE_TILING_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_IMAGE_TILING)
} XGL_IMAGE_TILING;

typedef enum _XGL_IMAGE_VIEW_TYPE
{
    XGL_IMAGE_VIEW_1D                                       = 0x00000000,
    XGL_IMAGE_VIEW_2D                                       = 0x00000001,
    XGL_IMAGE_VIEW_3D                                       = 0x00000002,
    XGL_IMAGE_VIEW_CUBE                                     = 0x00000003,

    XGL_IMAGE_VIEW_TYPE_BEGIN_RANGE                         = XGL_IMAGE_VIEW_1D,
    XGL_IMAGE_VIEW_TYPE_END_RANGE                           = XGL_IMAGE_VIEW_CUBE,
    XGL_NUM_IMAGE_VIEW_TYPE                                 = (XGL_IMAGE_VIEW_TYPE_END_RANGE - XGL_IMAGE_VIEW_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_IMAGE_VIEW_TYPE)
} XGL_IMAGE_VIEW_TYPE;

typedef enum _XGL_IMAGE_ASPECT
{
    XGL_IMAGE_ASPECT_COLOR                                  = 0x00000000,
    XGL_IMAGE_ASPECT_DEPTH                                  = 0x00000001,
    XGL_IMAGE_ASPECT_STENCIL                                = 0x00000002,

    XGL_IMAGE_ASPECT_BEGIN_RANGE                            = XGL_IMAGE_ASPECT_COLOR,
    XGL_IMAGE_ASPECT_END_RANGE                              = XGL_IMAGE_ASPECT_STENCIL,
    XGL_NUM_IMAGE_ASPECT                                    = (XGL_IMAGE_ASPECT_END_RANGE - XGL_IMAGE_ASPECT_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_IMAGE_ASPECT)
} XGL_IMAGE_ASPECT;

typedef enum _XGL_CHANNEL_SWIZZLE
{
    XGL_CHANNEL_SWIZZLE_ZERO                                = 0x00000000,
    XGL_CHANNEL_SWIZZLE_ONE                                 = 0x00000001,
    XGL_CHANNEL_SWIZZLE_R                                   = 0x00000002,
    XGL_CHANNEL_SWIZZLE_G                                   = 0x00000003,
    XGL_CHANNEL_SWIZZLE_B                                   = 0x00000004,
    XGL_CHANNEL_SWIZZLE_A                                   = 0x00000005,

    XGL_CHANNEL_SWIZZLE_BEGIN_RANGE                         = XGL_CHANNEL_SWIZZLE_ZERO,
    XGL_CHANNEL_SWIZZLE_END_RANGE                           = XGL_CHANNEL_SWIZZLE_A,
    XGL_NUM_CHANNEL_SWIZZLE                                 = (XGL_CHANNEL_SWIZZLE_END_RANGE - XGL_CHANNEL_SWIZZLE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_CHANNEL_SWIZZLE)
} XGL_CHANNEL_SWIZZLE;

typedef enum _XGL_DESCRIPTOR_SET_SLOT_TYPE
{
    XGL_SLOT_UNUSED                                         = 0x00000000,
    XGL_SLOT_SHADER_RESOURCE                                = 0x00000001,
    XGL_SLOT_SHADER_UAV                                     = 0x00000002,
    XGL_SLOT_SHADER_SAMPLER                                 = 0x00000003,
// IMG CHANGE BEGIN - support for vertex input description
    XGL_SLOT_VERTEX_INPUT                                   = 0x00000004,
    XGL_SLOT_NEXT_DESCRIPTOR_SET                            = 0x00000005,
// IMG CHANGE END

    XGL_DESCRIPTOR_SET_SLOT_TYPE_BEGIN_RANGE                = XGL_SLOT_UNUSED,
    XGL_DESCRIPTOR_SET_SLOT_TYPE_END_RANGE                  = XGL_SLOT_NEXT_DESCRIPTOR_SET,
    XGL_NUM_DESCRIPTOR_SET_SLOT_TYPE                        = (XGL_DESCRIPTOR_SET_SLOT_TYPE_END_RANGE - XGL_DESCRIPTOR_SET_SLOT_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_DESCRIPTOR_SET_SLOT_TYPE)
} XGL_DESCRIPTOR_SET_SLOT_TYPE;

typedef enum _XGL_QUERY_TYPE
{
    XGL_QUERY_OCCLUSION                                     = 0x00000000,
    XGL_QUERY_PIPELINE_STATISTICS                           = 0x00000001,

    XGL_QUERY_TYPE_BEGIN_RANGE                              = XGL_QUERY_OCCLUSION,
    XGL_QUERY_TYPE_END_RANGE                                = XGL_QUERY_PIPELINE_STATISTICS,
    XGL_NUM_QUERY_TYPE                                      = (XGL_QUERY_TYPE_END_RANGE - XGL_QUERY_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_QUERY_TYPE)
} XGL_QUERY_TYPE;

typedef enum _XGL_TIMESTAMP_TYPE
{
    XGL_TIMESTAMP_TOP                                       = 0x00000000,
    XGL_TIMESTAMP_BOTTOM                                    = 0x00000001,

    XGL_TIMESTAMP_TYPE_BEGIN_RANGE                          = XGL_TIMESTAMP_TOP,
    XGL_TIMESTAMP_TYPE_END_RANGE                            = XGL_TIMESTAMP_BOTTOM,
    XGL_NUM_TIMESTAMP_TYPE                                  = (XGL_TIMESTAMP_TYPE_END_RANGE - XGL_TIMESTAMP_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_TIMESTEAMP_TYPE)
} XGL_TIMESTAMP_TYPE;

typedef enum _XGL_BORDER_COLOR_TYPE
{
    XGL_BORDER_COLOR_OPAQUE_WHITE                           = 0x00000000,
    XGL_BORDER_COLOR_TRANSPARENT_BLACK                      = 0x00000001,
    XGL_BORDER_COLOR_OPAQUE_BLACK                           = 0x00000002,

    XGL_BORDER_COLOR_TYPE_BEGIN_RANGE                       = XGL_BORDER_COLOR_OPAQUE_WHITE,
    XGL_BORDER_COLOR_TYPE_END_RANGE                         = XGL_BORDER_COLOR_OPAQUE_BLACK,
    XGL_NUM_BORDER_COLOR_TYPE                               = (XGL_BORDER_COLOR_TYPE_END_RANGE - XGL_BORDER_COLOR_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_BORDER_COLOR_TYPE)
} XGL_BORDER_COLOR_TYPE;

typedef enum _XGL_PIPELINE_BIND_POINT
{
    XGL_PIPELINE_BIND_POINT_COMPUTE                         = 0x00000000,
    XGL_PIPELINE_BIND_POINT_GRAPHICS                        = 0x00000001,

    XGL_PIPELINE_BIND_POINT_BEGIN_RANGE                     = XGL_PIPELINE_BIND_POINT_COMPUTE,
    XGL_PIPELINE_BIND_POINT_END_RANGE                       = XGL_PIPELINE_BIND_POINT_GRAPHICS,
    XGL_NUM_PIPELINE_BIND_POINT                             = (XGL_PIPELINE_BIND_POINT_END_RANGE - XGL_PIPELINE_BIND_POINT_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_PIPELINE_BIND_POINT)
} XGL_PIPELINE_BIND_POINT;

typedef enum _XGL_STATE_BIND_POINT
{
    XGL_STATE_BIND_VIEWPORT                                 = 0x00000000,
    XGL_STATE_BIND_RASTER                                   = 0x00000001,
    XGL_STATE_BIND_DEPTH_STENCIL                            = 0x00000002,
    XGL_STATE_BIND_COLOR_BLEND                              = 0x00000003,
    XGL_STATE_BIND_MSAA                                     = 0x00000004,

    XGL_STATE_BIND_POINT_BEGIN_RANGE                        = XGL_STATE_BIND_VIEWPORT,
    XGL_STATE_BIND_POINT_END_RANGE                          = XGL_STATE_BIND_MSAA,
    XGL_NUM_STATE_BIND_POINT                                = (XGL_STATE_BIND_POINT_END_RANGE - XGL_STATE_BIND_POINT_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_STATE_BIND_POINT)
} XGL_STATE_BIND_POINT;

typedef enum _XGL_PRIMITIVE_TOPOLOGY
{
    XGL_TOPOLOGY_POINT_LIST                                 = 0x00000000,
    XGL_TOPOLOGY_LINE_LIST                                  = 0x00000001,
    XGL_TOPOLOGY_LINE_STRIP                                 = 0x00000002,
    XGL_TOPOLOGY_TRIANGLE_LIST                              = 0x00000003,
    XGL_TOPOLOGY_TRIANGLE_STRIP                             = 0x00000004,
    XGL_TOPOLOGY_RECT_LIST                                  = 0x00000005,
    XGL_TOPOLOGY_QUAD_LIST                                  = 0x00000006,
    XGL_TOPOLOGY_QUAD_STRIP                                 = 0x00000007,
    XGL_TOPOLOGY_LINE_LIST_ADJ                              = 0x00000008,
    XGL_TOPOLOGY_LINE_STRIP_ADJ                             = 0x00000009,
    XGL_TOPOLOGY_TRIANGLE_LIST_ADJ                          = 0x0000000a,
    XGL_TOPOLOGY_TRIANGLE_STRIP_ADJ                         = 0x0000000b,
    XGL_TOPOLOGY_PATCH                                      = 0x0000000c,

    XGL_PRIMITIVE_TOPOLOGY_BEGIN_RANGE                      = XGL_TOPOLOGY_POINT_LIST,
    XGL_PRIMITIVE_TOPOLOGY_END_RANGE                        = XGL_TOPOLOGY_PATCH,
    XGL_NUM_PRIMITIVE_TOPOLOGY                              = (XGL_PRIMITIVE_TOPOLOGY_END_RANGE - XGL_PRIMITIVE_TOPOLOGY_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_PRIMITIVE_TOPOLOGY)
} XGL_PRIMITIVE_TOPOLOGY;

typedef enum _XGL_INDEX_TYPE
{
    XGL_INDEX_8                                             = 0x00000000,
    XGL_INDEX_16                                            = 0x00000001,
    XGL_INDEX_32                                            = 0x00000002,

    XGL_INDEX_TYPE_BEGIN_RANGE                              = XGL_INDEX_8,
    XGL_INDEX_TYPE_END_RANGE                                = XGL_INDEX_32,
    XGL_NUM_INDEX_TYPE                                      = (XGL_INDEX_TYPE_END_RANGE - XGL_INDEX_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_INDEX_TYPE)
} XGL_INDEX_TYPE;

typedef enum _XGL_TEX_FILTER
{
    XGL_TEX_FILTER_NEAREST                                  = 0,
    XGL_TEX_FILTER_LINEAR                                   = 1,
    XGL_MAX_ENUM(_XGL_TEX_FILTER)
} XGL_TEX_FILTER;

typedef enum _XGL_TEX_MIPMAP_MODE
{
    XGL_TEX_MIPMAP_BASE                                     = 0,        // Always choose base level
    XGL_TEX_MIPMAP_NEAREST                                  = 1,        // Choose nearest mip level
    XGL_TEX_MIPMAP_LINEAR                                   = 2,        // Linear filter between mip levels
    XGL_MAX_ENUM(_XGL_TEX_MIPMAP_MODE)
} XGL_TEX_MIPMAP_MODE;

typedef enum _XGL_TEX_ADDRESS
{
    XGL_TEX_ADDRESS_WRAP                                    = 0x00000000,
    XGL_TEX_ADDRESS_MIRROR                                  = 0x00000001,
    XGL_TEX_ADDRESS_CLAMP                                   = 0x00000002,
    XGL_TEX_ADDRESS_MIRROR_ONCE                             = 0x00000003,
    XGL_TEX_ADDRESS_CLAMP_BORDER                            = 0x00000004,

    XGL_TEX_ADDRESS_BEGIN_RANGE                             = XGL_TEX_ADDRESS_WRAP,
    XGL_TEX_ADDRESS_END_RANGE                               = XGL_TEX_ADDRESS_CLAMP_BORDER,
    XGL_NUM_TEX_ADDRESS                                     = (XGL_TEX_ADDRESS_END_RANGE - XGL_TEX_ADDRESS_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_TEX_ADDRESS)
} XGL_TEX_ADDRESS;

typedef enum _XGL_COMPARE_FUNC
{
    XGL_COMPARE_NEVER                                       = 0x00000000,
    XGL_COMPARE_LESS                                        = 0x00000001,
    XGL_COMPARE_EQUAL                                       = 0x00000002,
    XGL_COMPARE_LESS_EQUAL                                  = 0x00000003,
    XGL_COMPARE_GREATER                                     = 0x00000004,
    XGL_COMPARE_NOT_EQUAL                                   = 0x00000005,
    XGL_COMPARE_GREATER_EQUAL                               = 0x00000006,
    XGL_COMPARE_ALWAYS                                      = 0x00000007,

    XGL_COMPARE_FUNC_BEGIN_RANGE                            = XGL_COMPARE_NEVER,
    XGL_COMPARE_FUNC_END_RANGE                              = XGL_COMPARE_ALWAYS,
    XGL_NUM_COMPARE_FUNC                                    = (XGL_COMPARE_FUNC_END_RANGE - XGL_COMPARE_FUNC_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_COMPARE_FUNC)
} XGL_COMPARE_FUNC;

typedef enum _XGL_FILL_MODE
{
    XFL_FILL_POINTS                                         = 0x00000000,
    XGL_FILL_WIREFRAME                                      = 0x00000001,
    XGL_FILL_SOLID                                          = 0x00000002,

    XGL_FILL_MODE_BEGIN_RANGE                               = XGL_FILL_SOLID,
    XGL_FILL_MODE_END_RANGE                                 = XFL_FILL_POINTS,
    XGL_NUM_FILL_MODE                                       = (XGL_FILL_MODE_END_RANGE - XGL_FILL_MODE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_FILL_MODE)
} XGL_FILL_MODE;

typedef enum _XGL_CULL_MODE
{
    XGL_CULL_NONE                                           = 0x00000000,
    XGL_CULL_FRONT                                          = 0x00000001,
    XGL_CULL_BACK                                           = 0x00000002,
    XGL_CULL_FRONT_AND_BACK                                 = 0x00000003,

    XGL_CULL_MODE_BEGIN_RANGE                               = XGL_CULL_NONE,
    XGL_CULL_MODE_END_RANGE                                 = XGL_CULL_FRONT_AND_BACK,
    XGL_NUM_CULL_MODE                                       = (XGL_CULL_MODE_END_RANGE - XGL_CULL_MODE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_CULL_MODE)
} XGL_CULL_MODE;

typedef enum _XGL_FACE_ORIENTATION
{
    XGL_FRONT_FACE_CCW                                      = 0x00000000,
    XGL_FRONT_FACE_CW                                       = 0x00000001,

    XGL_FACE_ORIENTATION_BEGIN_RANGE                        = XGL_FRONT_FACE_CCW,
    XGL_FACE_ORIENTATION_END_RANGE                          = XGL_FRONT_FACE_CW,
    XGL_NUM_FACE_ORIENTATION                                = (XGL_FACE_ORIENTATION_END_RANGE - XGL_FACE_ORIENTATION_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_FACE_ORIENTATION)
} XGL_FACE_ORIENTATION;

typedef enum _XGL_BLEND
{
    XGL_BLEND_ZERO                                          = 0x00000000,
    XGL_BLEND_ONE                                           = 0x00000001,
    XGL_BLEND_SRC_COLOR                                     = 0x00000002,
    XGL_BLEND_ONE_MINUS_SRC_COLOR                           = 0x00000003,
    XGL_BLEND_DEST_COLOR                                    = 0x00000004,
    XGL_BLEND_ONE_MINUS_DEST_COLOR                          = 0x00000005,
    XGL_BLEND_SRC_ALPHA                                     = 0x00000006,
    XGL_BLEND_ONE_MINUS_SRC_ALPHA                           = 0x00000007,
    XGL_BLEND_DEST_ALPHA                                    = 0x00000008,
    XGL_BLEND_ONE_MINUS_DEST_ALPHA                          = 0x00000009,
    XGL_BLEND_CONSTANT_COLOR                                = 0x0000000a,
    XGL_BLEND_ONE_MINUS_CONSTANT_COLOR                      = 0x0000000b,
    XGL_BLEND_CONSTANT_ALPHA                                = 0x0000000c,
    XGL_BLEND_ONE_MINUS_CONSTANT_ALPHA                      = 0x0000000d,
    XGL_BLEND_SRC_ALPHA_SATURATE                            = 0x0000000e,
    XGL_BLEND_SRC1_COLOR                                    = 0x0000000f,
    XGL_BLEND_ONE_MINUS_SRC1_COLOR                          = 0x00000010,
    XGL_BLEND_SRC1_ALPHA                                    = 0x00000011,
    XGL_BLEND_ONE_MINUS_SRC1_ALPHA                          = 0x00000012,

    XGL_BLEND_BEGIN_RANGE                                   = XGL_BLEND_ZERO,
    XGL_BLEND_END_RANGE                                     = XGL_BLEND_ONE_MINUS_SRC1_ALPHA,
    XGL_NUM_BLEND                                           = (XGL_BLEND_END_RANGE - XGL_BLEND_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_BLEND)
} XGL_BLEND;

typedef enum _XGL_BLEND_FUNC
{
    XGL_BLEND_FUNC_ADD                                      = 0x00000000,
    XGL_BLEND_FUNC_SUBTRACT                                 = 0x00000001,
    XGL_BLEND_FUNC_REVERSE_SUBTRACT                         = 0x00000002,
    XGL_BLEND_FUNC_MIN                                      = 0x00000003,
    XGL_BLEND_FUNC_MAX                                      = 0x00000004,

    XGL_BLEND_FUNC_BEGIN_RANGE                              = XGL_BLEND_FUNC_ADD,
    XGL_BLEND_FUNC_END_RANGE                                = XGL_BLEND_FUNC_MAX,
    XGL_NUM_BLEND_FUNC                                      = (XGL_BLEND_FUNC_END_RANGE - XGL_BLEND_FUNC_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_BLEND_FUNC)
} XGL_BLEND_FUNC;

typedef enum _XGL_STENCIL_OP
{
    XGL_STENCIL_OP_KEEP                                     = 0x00000000,
    XGL_STENCIL_OP_ZERO                                     = 0x00000001,
    XGL_STENCIL_OP_REPLACE                                  = 0x00000002,
    XGL_STENCIL_OP_INC_CLAMP                                = 0x00000003,
    XGL_STENCIL_OP_DEC_CLAMP                                = 0x00000004,
    XGL_STENCIL_OP_INVERT                                   = 0x00000005,
    XGL_STENCIL_OP_INC_WRAP                                 = 0x00000006,
    XGL_STENCIL_OP_DEC_WRAP                                 = 0x00000007,

    XGL_STENCIL_OP_BEGIN_RANGE                              = XGL_STENCIL_OP_KEEP,
    XGL_STENCIL_OP_END_RANGE                                = XGL_STENCIL_OP_DEC_WRAP,
    XGL_NUM_STENCIL_OP                                      = (XGL_STENCIL_OP_END_RANGE - XGL_STENCIL_OP_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_STENCIL_OP)
} XGL_STENCIL_OP;

typedef enum _XGL_LOGIC_OP
{
    XGL_LOGIC_OP_COPY                                       = 0x00000000,
    XGL_LOGIC_OP_CLEAR                                      = 0x00000001,
    XGL_LOGIC_OP_AND                                        = 0x00000002,
    XGL_LOGIC_OP_AND_REVERSE                                = 0x00000003,
    XGL_LOGIC_OP_AND_INVERTED                               = 0x00000004,
    XGL_LOGIC_OP_NOOP                                       = 0x00000005,
    XGL_LOGIC_OP_XOR                                        = 0x00000006,
    XGL_LOGIC_OP_OR                                         = 0x00000007,
    XGL_LOGIC_OP_NOR                                        = 0x00000008,
    XGL_LOGIC_OP_EQUIV                                      = 0x00000009,
    XGL_LOGIC_OP_INVERT                                     = 0x0000000a,
    XGL_LOGIC_OP_OR_REVERSE                                 = 0x0000000b,
    XGL_LOGIC_OP_COPY_INVERTED                              = 0x0000000c,
    XGL_LOGIC_OP_OR_INVERTED                                = 0x0000000d,
    XGL_LOGIC_OP_NAND                                       = 0x0000000e,
    XGL_LOGIC_OP_SET                                        = 0x0000000f,

    XGL_LOGIC_OP_BEGIN_RANGE                                = XGL_LOGIC_OP_COPY,
    XGL_LOGIC_OP_END_RANGE                                  = XGL_LOGIC_OP_SET,
    XGL_NUM_LOGIC_OP                                        = (XGL_LOGIC_OP_END_RANGE - XGL_LOGIC_OP_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_LOGIC_OP)
} XGL_LOGIC_OP;

typedef enum _XGL_ATOMIC_OP
{
    XGL_ATOMIC_ADD_INT32                                    = 0x00000000,
    XGL_ATOMIC_SUB_INT32                                    = 0x00000001,
    XGL_ATOMIC_MIN_UINT32                                   = 0x00000002,
    XGL_ATOMIC_MAX_UINT32                                   = 0x00000003,
    XGL_ATOMIC_MIN_SINT32                                   = 0x00000004,
    XGL_ATOMIC_MAX_SINT32                                   = 0x00000005,
    XGL_ATOMIC_AND_INT32                                    = 0x00000006,
    XGL_ATOMIC_OR_INT32                                     = 0x00000007,
    XGL_ATOMIC_XOR_INT32                                    = 0x00000008,
    XGL_ATOMIC_INC_UINT32                                   = 0x00000009,
    XGL_ATOMIC_DEC_UINT32                                   = 0x0000000a,
    XGL_ATOMIC_ADD_INT64                                    = 0x0000000b,
    XGL_ATOMIC_SUB_INT64                                    = 0x0000000c,
    XGL_ATOMIC_MIN_UINT64                                   = 0x0000000d,
    XGL_ATOMIC_MAX_UINT64                                   = 0x0000000e,
    XGL_ATOMIC_MIN_SINT64                                   = 0x0000000f,
    XGL_ATOMIC_MAX_SINT64                                   = 0x00000010,
    XGL_ATOMIC_AND_INT64                                    = 0x00000011,
    XGL_ATOMIC_OR_INT64                                     = 0x00000012,
    XGL_ATOMIC_XOR_INT64                                    = 0x00000013,
    XGL_ATOMIC_INC_UINT64                                   = 0x00000014,
    XGL_ATOMIC_DEC_UINT64                                   = 0x00000015,

    XGL_ATOMIC_OP_BEGIN_RANGE                               = XGL_ATOMIC_ADD_INT32,
    XGL_ATOMIC_OP_END_RANGE                                 = XGL_ATOMIC_DEC_UINT64,
    XGL_NUM_ATOMIC_OP                                       = (XGL_ATOMIC_OP_END_RANGE - XGL_ATOMIC_OP_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_ATOMIC_OP)
} XGL_ATOMIC_OP;

typedef enum _XGL_SYSTEM_ALLOC_TYPE
{
    XGL_SYSTEM_ALLOC_API_OBJECT                             = 0x00000000,
    XGL_SYSTEM_ALLOC_INTERNAL                               = 0x00000001,
    XGL_SYSTEM_ALLOC_INTERNAL_TEMP                          = 0x00000002,
    XGL_SYSTEM_ALLOC_INTERNAL_SHADER                        = 0x00000003,
    XGL_SYSTEM_ALLOC_DEBUG                                  = 0x00000004,

    XGL_SYSTEM_ALLOC_BEGIN_RANGE                            = XGL_SYSTEM_ALLOC_API_OBJECT,
    XGL_SYSTEM_ALLOC_END_RANGE                              = XGL_SYSTEM_ALLOC_DEBUG,
    XGL_NUM_SYSTEM_ALLOC_TYPE                               = (XGL_SYSTEM_ALLOC_END_RANGE - XGL_SYSTEM_ALLOC_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_SYSTEM_ALLOC_TYPE)
} XGL_SYSTEM_ALLOC_TYPE;

typedef enum _XGL_HEAP_MEMORY_TYPE
{
    XGL_HEAP_MEMORY_OTHER                                   = 0x00000000,
    XGL_HEAP_MEMORY_LOCAL                                   = 0x00000001,
    XGL_HEAP_MEMORY_REMOTE                                  = 0x00000002,
    XGL_HEAP_MEMORY_EMBEDDED                                = 0x00000003,

    XGL_HEAP_MEMORY_BEGIN_RANGE                             = XGL_HEAP_MEMORY_OTHER,
    XGL_HEAP_MEMORY_END_RANGE                               = XGL_HEAP_MEMORY_EMBEDDED,
    XGL_NUM_HEAP_MEMORY_TYPE                                = (XGL_HEAP_MEMORY_END_RANGE - XGL_HEAP_MEMORY_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_HEAP_MEMORY_TYPE)
} XGL_HEAP_MEMORY_TYPE;

typedef enum _XGL_PHYSICAL_GPU_TYPE
{
    XGL_GPU_TYPE_OTHER                                      = 0x00000000,
    XGL_GPU_TYPE_INTEGRATED                                 = 0x00000001,
    XGL_GPU_TYPE_DISCRETE                                   = 0x00000002,
    XGL_GPU_TYPE_VIRTUAL                                    = 0x00000003,

    XGL_PHYSICAL_GPU_TYPE_BEGIN_RANGE                       = XGL_GPU_TYPE_OTHER,
    XGL_PHYSICAL_GPU_TYPE_END_RANGE                         = XGL_GPU_TYPE_VIRTUAL,
    XGL_NUM_PHYSICAL_GPU_TYPE                               = (XGL_PHYSICAL_GPU_TYPE_END_RANGE - XGL_PHYSICAL_GPU_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_PHYSICAL_GPU_TYPE)
} XGL_PHYSICAL_GPU_TYPE;

typedef enum _XGL_PHYSICAL_GPU_INFO_TYPE
{
    // Info type for xglGetGpuInfo()
    XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES                   = 0x00000000,
    XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE                  = 0x00000001,
    XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES             = 0x00000002,
    XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES            = 0x00000003,

    XGL_MAX_ENUM(_XGL_PHYSICAL_GPU_INFO_TYPE)
} XGL_PHYSICAL_GPU_INFO_TYPE;

typedef enum _XGL_MEMORY_HEAP_INFO_TYPE
{
    // Info type for xglGetMemoryHeapInfo()
    XGL_INFO_TYPE_MEMORY_HEAP_PROPERTIES                    = 0x00000000,

    XGL_MAX_ENUM(_XGL_MEMORY_HEAP_INFO_TYPE)
} XGL_MEMORY_HEAP_INFO_TYPE;

typedef enum _XGL_FORMAT_INFO_TYPE
{
    // Info type for xlgGetFormatInfo()
    XGL_INFO_TYPE_FORMAT_PROPERTIES                         = 0x00000000,

    XGL_MAX_ENUM(_XGL_FORMAT_INFO_TYPE)
} XGL_FORMAT_INFO_TYPE;

typedef enum _XGL_SUBRESOURCE_INFO_TYPE
{
    // Info type for xglGetImageSubresourceInfo()
    XGL_INFO_TYPE_SUBRESOURCE_LAYOUT                        = 0x00000000,

    XGL_MAX_ENUM(_XGL_SUBRESOURCE_INFO_TYPE)
} XGL_SUBRESOURCE_INFO_TYPE;

typedef enum _XGL_OBJECT_INFO_TYPE
{
    // Info type for xglGetObjectInfo()
    XGL_INFO_TYPE_MEMORY_REQUIREMENTS                       = 0x00000000,

    XGL_MAX_ENUM(_XGL_OBJECT_INFO_TYPE)
} XGL_OBJECT_INFO_TYPE;

typedef enum _XGL_VALIDATION_LEVEL
{
    XGL_VALIDATION_LEVEL_0                                  = 0x00000000,
    XGL_VALIDATION_LEVEL_1                                  = 0x00000001,
    XGL_VALIDATION_LEVEL_2                                  = 0x00000002,
    XGL_VALIDATION_LEVEL_3                                  = 0x00000003,
    XGL_VALIDATION_LEVEL_4                                  = 0x00000004,

    XGL_VALIDATION_LEVEL_BEGIN_RANGE                        = XGL_VALIDATION_LEVEL_0,
    XGL_VALIDATION_LEVEL_END_RANGE                          = XGL_VALIDATION_LEVEL_4,
    XGL_NUM_VALIDATION_LEVEL                                = (XGL_VALIDATION_LEVEL_END_RANGE - XGL_VALIDATION_LEVEL_BEGIN_RANGE + 1),

    XGL_MAX_ENUM(_XGL_VALIDATION_LEVEL)
} XGL_VALIDATION_LEVEL;

// ------------------------------------------------------------------------------------------------
// Error and return codes

typedef enum _XGL_RESULT_CODE
{
    // Return codes for successful operation execution (>= 0)
    XGL_SUCCESS                                             = 0x0000000,
    XGL_UNSUPPORTED                                         = 0x0000001,
    XGL_NOT_READY                                           = 0x0000002,
    XGL_TIMEOUT                                             = 0x0000003,
    XGL_EVENT_SET                                           = 0x0000004,
    XGL_EVENT_RESET                                         = 0x0000005,

    // Error codes (negative values)
    XGL_ERROR_UNKNOWN                                       = -(0x00000001),
    XGL_ERROR_UNAVAILABLE                                   = -(0x00000002),
    XGL_ERROR_INITIALIZATION_FAILED                         = -(0x00000003),
    XGL_ERROR_OUT_OF_MEMORY                                 = -(0x00000004),
    XGL_ERROR_OUT_OF_GPU_MEMORY                             = -(0x00000005),
    XGL_ERROR_DEVICE_ALREADY_CREATED                        = -(0x00000006),
    XGL_ERROR_DEVICE_LOST                                   = -(0x00000007),
    XGL_ERROR_INVALID_POINTER                               = -(0x00000008),
    XGL_ERROR_INVALID_VALUE                                 = -(0x00000009),
    XGL_ERROR_INVALID_HANDLE                                = -(0x0000000A),
    XGL_ERROR_INVALID_ORDINAL                               = -(0x0000000B),
    XGL_ERROR_INVALID_MEMORY_SIZE                           = -(0x0000000C),
    XGL_ERROR_INVALID_EXTENSION                             = -(0x0000000D),
    XGL_ERROR_INVALID_FLAGS                                 = -(0x0000000E),
    XGL_ERROR_INVALID_ALIGNMENT                             = -(0x0000000F),
    XGL_ERROR_INVALID_FORMAT                                = -(0x00000010),
    XGL_ERROR_INVALID_IMAGE                                 = -(0x00000011),
    XGL_ERROR_INVALID_DESCRIPTOR_SET_DATA                   = -(0x00000012),
    XGL_ERROR_INVALID_QUEUE_TYPE                            = -(0x00000013),
    XGL_ERROR_INVALID_OBJECT_TYPE                           = -(0x00000014),
    XGL_ERROR_UNSUPPORTED_SHADER_IL_VERSION                 = -(0x00000015),
    XGL_ERROR_BAD_SHADER_CODE                               = -(0x00000016),
    XGL_ERROR_BAD_PIPELINE_DATA                             = -(0x00000017),
    XGL_ERROR_TOO_MANY_MEMORY_REFERENCES                    = -(0x00000018),
    XGL_ERROR_NOT_MAPPABLE                                  = -(0x00000019),
    XGL_ERROR_MEMORY_MAP_FAILED                             = -(0x0000001A),
    XGL_ERROR_MEMORY_UNMAP_FAILED                           = -(0x0000001B),
    XGL_ERROR_INCOMPATIBLE_DEVICE                           = -(0x0000001C),
    XGL_ERROR_INCOMPATIBLE_DRIVER                           = -(0x0000001D),
    XGL_ERROR_INCOMPLETE_COMMAND_BUFFER                     = -(0x0000001E),
    XGL_ERROR_BUILDING_COMMAND_BUFFER                       = -(0x0000001F),
    XGL_ERROR_MEMORY_NOT_BOUND                              = -(0x00000020),
    XGL_ERROR_INCOMPATIBLE_QUEUE                            = -(0x00000021),
    XGL_ERROR_NOT_SHAREABLE                                 = -(0x00000022),
} XGL_RESULT;

// ------------------------------------------------------------------------------------------------
// XGL format definitions

typedef enum _XGL_CHANNEL_FORMAT
{
    XGL_CH_FMT_UNDEFINED                                    = 0,
    XGL_CH_FMT_R4G4                                         = 1,
    XGL_CH_FMT_R4G4B4A4                                     = 2,
    XGL_CH_FMT_R5G6B5                                       = 3,
    XGL_CH_FMT_B5G6R5                                       = 4,
    XGL_CH_FMT_R5G5B5A1                                     = 5,
    XGL_CH_FMT_R8                                           = 6,
    XGL_CH_FMT_R8G8                                         = 7,
    XGL_CH_FMT_R8G8B8A8                                     = 8,
    XGL_CH_FMT_B8G8R8A8                                     = 9,
    XGL_CH_FMT_R10G11B11                                    = 10,
    XGL_CH_FMT_R11G11B10                                    = 11,
    XGL_CH_FMT_R10G10B10A2                                  = 12,
    XGL_CH_FMT_R16                                          = 13,
    XGL_CH_FMT_R16G16                                       = 14,
    XGL_CH_FMT_R16G16B16A16                                 = 15,
    XGL_CH_FMT_R32                                          = 16,
    XGL_CH_FMT_R32G32                                       = 17,
    XGL_CH_FMT_R32G32B32                                    = 18,
    XGL_CH_FMT_R32G32B32A32                                 = 19,
    XGL_CH_FMT_R16G8                                        = 20,
    XGL_CH_FMT_R32G8                                        = 21,
    XGL_CH_FMT_R9G9B9E5                                     = 22,
    XGL_CH_FMT_BC1                                          = 23,
    XGL_CH_FMT_BC2                                          = 24,
    XGL_CH_FMT_BC3                                          = 25,
    XGL_CH_FMT_BC4                                          = 26,
    XGL_CH_FMT_BC5                                          = 27,
    XGL_CH_FMT_BC6U                                         = 28,
    XGL_CH_FMT_BC6S                                         = 29,
    XGL_CH_FMT_BC7                                          = 30,
// IMG CHANGE BEGIN - support for vertex input description
    XGL_CH_FMT_R8G8B8                                       = 31,
    XGL_CH_FMT_R16G16B16                                    = 32,
    XGL_MAX_CH_FMT                                          = XGL_CH_FMT_R16G16B16,
// IMG CHANGE END
    XGL_MAX_ENUM(_XGL_CHANNEL_FORMAT)
} XGL_CHANNEL_FORMAT;

typedef enum _XGL_NUM_FORMAT
{
    XGL_NUM_FMT_UNDEFINED                                   = 0,
    XGL_NUM_FMT_UNORM                                       = 1,
    XGL_NUM_FMT_SNORM                                       = 2,
    XGL_NUM_FMT_UINT                                        = 3,
    XGL_NUM_FMT_SINT                                        = 4,
    XGL_NUM_FMT_FLOAT                                       = 5,
    XGL_NUM_FMT_SRGB                                        = 6,
    XGL_NUM_FMT_DS                                          = 7,
    XGL_MAX_NUM_FMT                                         = XGL_NUM_FMT_DS,
    XGL_MAX_ENUM(_XGL_NUM_FORMAT)
} XGL_NUM_FORMAT;

typedef enum _XGL_PROVOKING_VERTEX_CONVENTION
{
    XGL_PROVOKING_VERTEX_FIRST                              = 0,
    XGL_PROVOKING_VERTEX_LAST                               = 1,

    XGL_PROVOKING_VERTEX_BEGIN_RANGE                        = XGL_PROVOKING_VERTEX_FIRST,
    XGL_PROVOKING_VERTEX_END_RANGE                          = XGL_PROVOKING_VERTEX_LAST,
    XGL_NUM_PROVOKING_VERTEX_CONVENTIONS                    = (XGL_PROVOKING_VERTEX_END_RANGE - XGL_PROVOKING_VERTEX_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_PROVOKING_VERTEX_CONVENTION)
} XGL_PROVOKING_VERTEX_CONVENTION;

// IMG CHANGE BEGIN - support for vertex input description
typedef enum _XGL_VERTEX_INPUT_STEP_RATE
{
    XGL_VERTEX_INPUT_STEP_RATE_VERTEX     = 0x0,
    XGL_VERTEX_INPUT_STEP_RATE_INSTANCE   = 0x1,
    XGL_VERTEX_INPUT_STEP_RATE_DRAW       = 0x2,
    XGL_MAX_ENUM(_XGL_VERTEX_INPUT_STEP_RATE)
} XGL_VERTEX_INPUT_STEP_RATE;
// IMG CHANGE END

typedef struct _XGL_FORMAT
{
    XGL_CHANNEL_FORMAT  channelFormat;
    XGL_NUM_FORMAT      numericFormat;
} XGL_FORMAT;

// Shader stage enumerant
typedef enum _XGL_PIPELINE_SHADER_STAGE
{
    XGL_SHADER_STAGE_VERTEX                                 = 0,
    XGL_SHADER_STAGE_TESS_CONTROL                           = 1,
    XGL_SHADER_STAGE_TESS_EVALUATION                        = 2,
    XGL_SHADER_STAGE_GEOMETRY                               = 3,
    XGL_SHADER_STAGE_FRAGMENT                               = 4,
    XGL_SHADER_STAGE_COMPUTE                                = 5,
    XGL_MAX_ENUM(_XGL_PIPELINE_SHADER_STAGE)
} XGL_PIPELINE_SHADER_STAGE;

// Structure type enumerant
typedef enum _XGL_STRUCTURE_TYPE
{
    XGL_STRUCTURE_TYPE_APPLICATION_INFO                     = 0,
    XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO                   = 1,
    XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO                    = 2,
    XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO                     = 4,
    XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO                = 5,
    XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO              = 6,
    XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO               = 7,
    XGL_STRUCTURE_TYPE_MEMORY_STATE_TRANSITION              = 8,
    XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO               = 9,
    XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO    = 10,
    XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO       = 11,
    XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO                   = 12,
    XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO         = 13,
    XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO                  = 14,
    XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO           = 15,
    XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO             = 16,
    XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO               = 17,
    XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO        = 18,
    XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO      = 19,
    XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO               = 20,
    XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO                    = 21,
    XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO                    = 22,
    XGL_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO                = 23,
    XGL_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO                  = 24,
    XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO               = 25,
    XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO    = 26,
    XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO        = 27,
    XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO        = 28,
    XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO        = 29,
    XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO        = 30,
    XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO        = 31,
    XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO      = 32,
    XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO                    = 33,
// IMG CHANGE BEGIN - support for vertex input description
    XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO    = 34,
// IMG CHANGE END
    XGL_MAX_ENUM(_XGL_STRUCTURE_TYPE)
} XGL_STRUCTURE_TYPE;

// ------------------------------------------------------------------------------------------------
// Flags

// Device creation flags
typedef enum _XGL_DEVICE_CREATE_FLAGS
{
    XGL_DEVICE_CREATE_VALIDATION_BIT                        = 0x00000001,
    XGL_DEVICE_CREATE_MGPU_IQ_MATCH_BIT                     = 0x00000002,
} XGL_DEVICE_CREATE_FLAGS;

// Queue capabilities
typedef enum _XGL_QUEUE_FLAGS
{
    XGL_QUEUE_GRAPHICS_BIT                                  = 0x00000001,   // Queue supports graphics operations
    XGL_QUEUE_COMPUTE_BIT                                   = 0x00000002,   // Queue supports compute operations
    XGL_QUEUE_DMA_BIT                                       = 0x00000004,   // Queue supports DMA operations
    XGL_QUEUE_EXTENDED_BIT                                  = 0x80000000    // Extended queue
} XGL_QUEUE_FLAGS;

// Memory heap properties
typedef enum _XGL_MEMORY_HEAP_FLAGS
{
    XGL_MEMORY_HEAP_CPU_VISIBLE_BIT                         = 0x00000001,
    XGL_MEMORY_HEAP_CPU_GPU_COHERENT_BIT                    = 0x00000002,
    XGL_MEMORY_HEAP_CPU_UNCACHED_BIT                        = 0x00000004,
    XGL_MEMORY_HEAP_CPU_WRITE_COMBINED_BIT                  = 0x00000008,
    XGL_MEMORY_HEAP_HOLDS_PINNED_BIT                        = 0x00000010,
    XGL_MEMORY_HEAP_SHAREABLE_BIT                           = 0x00000020,
} XGL_MEMORY_HEAP_FLAGS;

// Memory allocation flags
typedef enum _XGL_MEMORY_ALLOC_FLAGS
{
    XGL_MEMORY_ALLOC_VIRTUAL_BIT                            = 0x00000001,
    XGL_MEMORY_ALLOC_SHAREABLE_BIT                          = 0x00000002,
} XGL_MEMORY_ALLOC_FLAGS;

// Image usage flags
typedef enum _XGL_IMAGE_USAGE_FLAGS
{
    XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT                  = 0x00000001,
    XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT                 = 0x00000002,
    XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT                    = 0x00000004,
    XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT                       = 0x00000008,
} XGL_IMAGE_USAGE_FLAGS;

// Image flags
typedef enum _XGL_IMAGE_CREATE_FLAGS
{
    XGL_IMAGE_CREATE_INVARIANT_DATA_BIT                     = 0x00000001,
    XGL_IMAGE_CREATE_CLONEABLE_BIT                          = 0x00000002,
    XGL_IMAGE_CREATE_SHAREABLE_BIT                          = 0x00000004,
} XGL_IMAGE_CREATE_FLAGS;

// Depth-stencil view creation flags
typedef enum _XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS
{
    XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT       = 0x00000001,
    XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT     = 0x00000002,
} XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS;

// Pipeline creation flags
typedef enum _XGL_PIPELINE_CREATE_FLAGS
{
    XGL_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT            = 0x00000001,
} XGL_PIPELINE_CREATE_FLAGS;

// Semaphore creation flags
typedef enum _XGL_SEMAPHORE_CREATE_FLAGS
{
    XGL_SEMAPHORE_CREATE_SHAREABLE_BIT                      = 0x00000001,
} XGL_SEMAPHORE_CREATE_FLAGS;

// Memory reference flags
typedef enum _XGL_MEMORY_REF_FLAGS
{
    XGL_MEMORY_REF_READ_ONLY_BIT                            = 0x00000001,
} XGL_MEMORY_REF_FLAGS;

// Format capability flags
typedef enum _XGL_FORMAT_FEATURE_FLAGS
{
    XGL_FORMAT_IMAGE_SHADER_READ_BIT                        = 0x00000001,
    XGL_FORMAT_IMAGE_SHADER_WRITE_BIT                       = 0x00000002,
    XGL_FORMAT_IMAGE_COPY_BIT                               = 0x00000004,
    XGL_FORMAT_MEMORY_SHADER_ACCESS_BIT                     = 0x00000008,
    XGL_FORMAT_COLOR_ATTACHMENT_WRITE_BIT                   = 0x00000010,
    XGL_FORMAT_COLOR_ATTACHMENT_BLEND_BIT                   = 0x00000020,
    XGL_FORMAT_DEPTH_ATTACHMENT_BIT                         = 0x00000040,
    XGL_FORMAT_STENCIL_ATTACHMENT_BIT                       = 0x00000080,
    XGL_FORMAT_MSAA_ATTACHMENT_BIT                          = 0x00000100,
    XGL_FORMAT_CONVERSION_BIT                               = 0x00000200,
} XGL_FORMAT_FEATURE_FLAGS;

// Query flags
typedef enum _XGL_QUERY_CONTROL_FLAGS
{
    XGL_QUERY_IMPRECISE_DATA_BIT                            = 0x00000001,
} XGL_QUERY_CONTROL_FLAGS;

// GPU compatibility flags
typedef enum _XGL_GPU_COMPATIBILITY_FLAGS
{
    XGL_GPU_COMPAT_ASIC_FEATURES_BIT                        = 0x00000001,
    XGL_GPU_COMPAT_IQ_MATCH_BIT                             = 0x00000002,
    XGL_GPU_COMPAT_PEER_TRANSFER_BIT                        = 0x00000004,
    XGL_GPU_COMPAT_SHARED_MEMORY_BIT                        = 0x00000008,
    XGL_GPU_COMPAT_SHARED_SYNC_BIT                          = 0x00000010,
    XGL_GPU_COMPAT_SHARED_GPU0_DISPLAY_BIT                  = 0x00000020,
    XGL_GPU_COMPAT_SHARED_GPU1_DISPLAY_BIT                  = 0x00000040,
} XGL_GPU_COMPATIBILITY_FLAGS;

// Command buffer building flags
typedef enum _XGL_CMD_BUFFER_BUILD_FLAGS
{
    XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT             = 0x00000001,
    XGL_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH_BIT             = 0x00000002,
    XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT             = 0x00000004,
    XGL_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT       = 0x00000008,
} XGL_CMD_BUFFER_BUILD_FLAGS;

// ------------------------------------------------------------------------------------------------
// XGL structures

typedef struct _XGL_OFFSET2D
{
    XGL_INT                                 x;
    XGL_INT                                 y;
} XGL_OFFSET2D;

typedef struct _XGL_OFFSET3D
{
    XGL_INT                                 x;
    XGL_INT                                 y;
    XGL_INT                                 z;
} XGL_OFFSET3D;

typedef struct _XGL_EXTENT2D
{
    XGL_INT                                 width;
    XGL_INT                                 height;
} XGL_EXTENT2D;

typedef struct _XGL_EXTENT3D
{
    XGL_INT                                 width;
    XGL_INT                                 height;
    XGL_INT                                 depth;
} XGL_EXTENT3D;

typedef struct _XGL_VIEWPORT
{
    XGL_FLOAT                               originX;
    XGL_FLOAT                               originY;
    XGL_FLOAT                               width;
    XGL_FLOAT                               height;
    XGL_FLOAT                               minDepth;
    XGL_FLOAT                               maxDepth;
} XGL_VIEWPORT;

typedef struct _XGL_RECT
{
    XGL_OFFSET2D                            offset;
    XGL_EXTENT2D                            extent;
} XGL_RECT;

typedef struct _XGL_PHYSICAL_GPU_PROPERTIES
{
    XGL_SIZE                                structSize;
    XGL_UINT32                              apiVersion;
    XGL_UINT32                              driverVersion;
    XGL_UINT32                              vendorId;
    XGL_UINT32                              deviceId;
    XGL_PHYSICAL_GPU_TYPE                   gpuType;
    XGL_CHAR                                gpuName[XGL_MAX_PHYSICAL_GPU_NAME];
    XGL_UINT                                maxMemRefsPerSubmission;
    XGL_GPU_SIZE                            virtualMemPageSize;
    XGL_GPU_SIZE                            maxInlineMemoryUpdateSize;
    XGL_UINT                                maxBoundDescriptorSets;
    XGL_UINT                                maxThreadGroupSize;
    XGL_UINT64                              timestampFrequency;
    XGL_BOOL                                multiColorAttachmentClears;
} XGL_PHYSICAL_GPU_PROPERTIES;

typedef struct _XGL_PHYSICAL_GPU_PERFORMANCE
{
    XGL_FLOAT                               maxGpuClock;
    XGL_FLOAT                               aluPerClock;
    XGL_FLOAT                               texPerClock;
    XGL_FLOAT                               primsPerClock;
    XGL_FLOAT                               pixelsPerClock;
} XGL_PHYSICAL_GPU_PERFORMANCE;

typedef struct _XGL_GPU_COMPATIBILITY_INFO
{
    XGL_FLAGS                               compatibilityFlags; // XGL_GPU_COMPATIBILITY_FLAGS
} XGL_GPU_COMPATIBILITY_INFO;

typedef struct _XGL_APPLICATION_INFO
{
    XGL_STRUCTURE_TYPE                      sType;              // Type of structure. Should be XGL_STRUCTURE_TYPE_APPLICATION_INFO
    XGL_VOID*                               pNext;              // Next structure in chain
    const XGL_CHAR*                         pAppName;
    XGL_UINT32                              appVersion;
    const XGL_CHAR*                         pEngineName;
    XGL_UINT32                              engineVersion;
    XGL_UINT32                              apiVersion;
} XGL_APPLICATION_INFO;

typedef XGL_VOID* (XGLAPI *XGL_ALLOC_FUNCTION)(
    XGL_VOID*                               pUserData,
    XGL_SIZE                                size,
    XGL_SIZE                                alignment,
    XGL_SYSTEM_ALLOC_TYPE                   allocType);

typedef XGL_VOID (XGLAPI *XGL_FREE_FUNCTION)(
    XGL_VOID*                               pUserData,
    XGL_VOID*                               pMem);

typedef struct _XGL_ALLOC_CALLBACKS
{
    XGL_VOID*                               pUserData;
    XGL_ALLOC_FUNCTION                      pfnAlloc;
    XGL_FREE_FUNCTION                       pfnFree;
} XGL_ALLOC_CALLBACKS;

typedef struct _XGL_DEVICE_QUEUE_CREATE_INFO
{
    XGL_UINT                                queueNodeIndex;
    XGL_UINT                                queueCount;
} XGL_DEVICE_QUEUE_CREATE_INFO;

typedef struct _XGL_DEVICE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Should be XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO
    XGL_VOID*                               pNext;                      // Pointer to next structure
    XGL_UINT                                queueRecordCount;
    const XGL_DEVICE_QUEUE_CREATE_INFO*     pRequestedQueues;
    XGL_UINT                                extensionCount;
    const XGL_CHAR*const*                   ppEnabledExtensionNames;
    XGL_VALIDATION_LEVEL                    maxValidationLevel;
    XGL_FLAGS                               flags;                      // XGL_DEVICE_CREATE_FLAGS
} XGL_DEVICE_CREATE_INFO;

typedef struct _XGL_PHYSICAL_GPU_QUEUE_PROPERTIES
{
    XGL_SIZE                                structSize;                 // Size of structure in bytes
    XGL_FLAGS                               queueFlags;                 // XGL_QUEUE_FLAGS
    XGL_UINT                                queueCount;
    XGL_UINT                                maxAtomicCounters;
    XGL_BOOL                                supportsTimestamps;
} XGL_PHYSICAL_GPU_QUEUE_PROPERTIES;

typedef struct _XGL_PHYSICAL_GPU_MEMORY_PROPERTIES
{
    XGL_SIZE                                structSize;                 // Size of structure in bytes
    XGL_BOOL                                supportsMigration;
    XGL_BOOL                                supportsVirtualMemoryRemapping;
    XGL_BOOL                                supportsPinning;
} XGL_PHYSICAL_GPU_MEMORY_PROPERTIES;

typedef struct _XGL_MEMORY_HEAP_PROPERTIES
{
    XGL_SIZE                                structSize;                 // Size of structure in bytes
    XGL_HEAP_MEMORY_TYPE                    heapMemoryType;             // XGL_HEAP_MEMORY_TYPE
    XGL_GPU_SIZE                            heapSize;                   // Specified in bytes
    XGL_GPU_SIZE                            pageSize;                   // Specified in bytes
    XGL_FLAGS                               flags;                      // XGL_MEMORY_HEAP_FLAGS
    XGL_FLOAT                               gpuReadPerfRating;
    XGL_FLOAT                               gpuWritePerfRating;
    XGL_FLOAT                               cpuReadPerfRating;
    XGL_FLOAT                               cpuWritePerfRating;
} XGL_MEMORY_HEAP_PROPERTIES;

typedef struct _XGL_MEMORY_ALLOC_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO
    XGL_VOID*                               pNext;                      // Pointer to next structure
    XGL_GPU_SIZE                            allocationSize;             // Size of memory allocation
    XGL_GPU_SIZE                            alignment;
    XGL_FLAGS                               flags;                      // XGL_MEMORY_ALLOC_FLAGS
    XGL_UINT                                heapCount;
    XGL_UINT                                heaps[XGL_MAX_MEMORY_HEAPS];
    XGL_MEMORY_PRIORITY                     memPriority;
} XGL_MEMORY_ALLOC_INFO;

typedef struct _XGL_MEMORY_OPEN_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO
    XGL_VOID*                               pNext;                      // Pointer to next structure
    XGL_GPU_MEMORY                          sharedMem;
} XGL_MEMORY_OPEN_INFO;

typedef struct _XGL_PEER_MEMORY_OPEN_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO
    XGL_VOID*                               pNext;                      // Pointer to next structure
    XGL_GPU_MEMORY                          originalMem;
} XGL_PEER_MEMORY_OPEN_INFO;

typedef struct _XGL_VIRTUAL_MEMORY_REMAP_RANGE
{
    XGL_GPU_MEMORY                          virtualMem;
    XGL_GPU_SIZE                            virtualStartPage;
    XGL_GPU_MEMORY                          realMem;
    XGL_GPU_SIZE                            realStartPage;
    XGL_GPU_SIZE                            pageCount;
} XGL_VIRTUAL_MEMORY_REMAP_RANGE;

typedef struct _XGL_MEMORY_REQUIREMENTS
{
    XGL_GPU_SIZE                            size;                       // Specified in bytes
    XGL_GPU_SIZE                            alignment;                  // Specified in bytes
    XGL_UINT                                heapCount;
    XGL_UINT                                heaps[XGL_MAX_MEMORY_HEAPS];
} XGL_MEMORY_REQUIREMENTS;

typedef struct _XGL_FORMAT_PROPERTIES
{
    XGL_FLAGS                               linearTilingFeatures;      // XGL_FORMAT_FEATURE_FLAGS
    XGL_FLAGS                               optimalTilingFeatures;     // XGL_FORMAT_FEATURE_FLAGS
} XGL_FORMAT_PROPERTIES;

typedef struct _XGL_MEMORY_VIEW_ATTACH_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO
    XGL_VOID*                               pNext;                      // Pointer to next structure
    XGL_GPU_MEMORY                          mem;
    XGL_GPU_SIZE                            offset;
    XGL_GPU_SIZE                            range;
    XGL_GPU_SIZE                            stride;
    XGL_FORMAT                              format;
    XGL_MEMORY_STATE                        state;
} XGL_MEMORY_VIEW_ATTACH_INFO;

typedef struct _XGL_IMAGE_VIEW_ATTACH_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO
    XGL_VOID*                               pNext;                      // Pointer to next structure
    XGL_IMAGE_VIEW                          view;
    XGL_IMAGE_STATE                         state;
} XGL_IMAGE_VIEW_ATTACH_INFO;

typedef struct _XGL_MEMORY_STATE_TRANSITION
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_STATE_TRANSITION
    XGL_VOID*                               pNext;                      // Pointer to next structure
    XGL_GPU_MEMORY                          mem;
    XGL_MEMORY_STATE                        oldState;
    XGL_MEMORY_STATE                        newState;
    XGL_GPU_SIZE                            offset;
    XGL_GPU_SIZE                            regionSize;
} XGL_MEMORY_STATE_TRANSITION;

typedef struct _XGL_IMAGE_SUBRESOURCE
{
    XGL_IMAGE_ASPECT                        aspect;
    XGL_UINT                                mipLevel;
    XGL_UINT                                arraySlice;
} XGL_IMAGE_SUBRESOURCE;

typedef struct _XGL_IMAGE_SUBRESOURCE_RANGE
{
    XGL_IMAGE_ASPECT                        aspect;
    XGL_UINT                                baseMipLevel;
    XGL_UINT                                mipLevels;
    XGL_UINT                                baseArraySlice;
    XGL_UINT                                arraySize;
} XGL_IMAGE_SUBRESOURCE_RANGE;

typedef struct _XGL_IMAGE_STATE_TRANSITION
{
    XGL_IMAGE                               image;
    XGL_IMAGE_STATE                         oldState;
    XGL_IMAGE_STATE                         newState;
    XGL_IMAGE_SUBRESOURCE_RANGE             subresourceRange;
} XGL_IMAGE_STATE_TRANSITION;

typedef struct _XGL_IMAGE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO
    const XGL_VOID*                         pNext;                      // Pointer to next structure.
    XGL_IMAGE_TYPE                          imageType;
    XGL_FORMAT                              format;
    XGL_EXTENT3D                            extent;
    XGL_UINT                                mipLevels;
    XGL_UINT                                arraySize;
    XGL_UINT                                samples;
    XGL_IMAGE_TILING                        tiling;
    XGL_FLAGS                               usage;                      // XGL_IMAGE_USAGE_FLAGS
    XGL_FLAGS                               flags;                      // XGL_IMAGE_CREATE_FLAGS
} XGL_IMAGE_CREATE_INFO;

typedef struct _XGL_PEER_IMAGE_OPEN_INFO
{
    XGL_IMAGE                               originalImage;
} XGL_PEER_IMAGE_OPEN_INFO;

typedef struct _XGL_SUBRESOURCE_LAYOUT
{
    XGL_GPU_SIZE                            offset;                 // Specified in bytes
    XGL_GPU_SIZE                            size;                   // Specified in bytes
    XGL_GPU_SIZE                            rowPitch;               // Specified in bytes
    XGL_GPU_SIZE                            depthPitch;             // Specified in bytes
} XGL_SUBRESOURCE_LAYOUT;

typedef struct _XGL_CHANNEL_MAPPING
{
    XGL_CHANNEL_SWIZZLE                     r;
    XGL_CHANNEL_SWIZZLE                     g;
    XGL_CHANNEL_SWIZZLE                     b;
    XGL_CHANNEL_SWIZZLE                     a;
} XGL_CHANNEL_MAPPING;

typedef struct _XGL_IMAGE_VIEW_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                  // Must be XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
    const XGL_VOID*                         pNext;                  // Pointer to next structure
    XGL_IMAGE                               image;
    XGL_IMAGE_VIEW_TYPE                     viewType;
    XGL_FORMAT                              format;
    XGL_CHANNEL_MAPPING                     channels;
    XGL_IMAGE_SUBRESOURCE_RANGE             subresourceRange;
    XGL_FLOAT                               minLod;
} XGL_IMAGE_VIEW_CREATE_INFO;

typedef struct _XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                  // Must be XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO
    XGL_VOID*                               pNext;                  // Pointer to next structure
    XGL_IMAGE                               image;
    XGL_FORMAT                              format;
    XGL_UINT                                mipLevel;
    XGL_UINT                                baseArraySlice;
    XGL_UINT                                arraySize;
} XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO;

typedef struct _XGL_DEPTH_STENCIL_VIEW_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                  // Must be XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO
    const XGL_VOID*                         pNext;                  // Pointer to next structure
    XGL_IMAGE                               image;
    XGL_UINT                                mipLevel;
    XGL_UINT                                baseArraySlice;
    XGL_UINT                                arraySize;
    XGL_FLAGS                               flags;                  // XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS
} XGL_DEPTH_STENCIL_VIEW_CREATE_INFO;

typedef struct _XGL_COLOR_ATTACHMENT_BIND_INFO
{
    XGL_COLOR_ATTACHMENT_VIEW               view;
    XGL_IMAGE_STATE                         colorAttachmentState;   // XGL_IMAGE_STATE
} XGL_COLOR_ATTACHMENT_BIND_INFO;

typedef struct _XGL_DEPTH_STENCIL_BIND_INFO
{
    XGL_DEPTH_STENCIL_VIEW                  view;
    XGL_IMAGE_STATE                         depthState;             // XGL_IMAGE_STATE
    XGL_IMAGE_STATE                         stencilState;           // XGL_IMAGE_STATE
} XGL_DEPTH_STENCIL_BIND_INFO;

typedef struct _XGL_MEMORY_COPY
{
    XGL_GPU_SIZE                            srcOffset;              // Specified in bytes
    XGL_GPU_SIZE                            destOffset;             // Specified in bytes
    XGL_GPU_SIZE                            copySize;               // Specified in bytes
} XGL_MEMORY_COPY;

typedef struct _XGL_IMAGE_COPY
{
    XGL_IMAGE_SUBRESOURCE                   srcSubresource;
    XGL_OFFSET3D                            srcOffset;
    XGL_IMAGE_SUBRESOURCE                   destSubresource;
    XGL_OFFSET3D                            destOffset;
    XGL_EXTENT3D                            extent;
} XGL_IMAGE_COPY;

typedef struct _XGL_MEMORY_IMAGE_COPY
{
    XGL_GPU_SIZE                            memOffset;              // Specified in bytes
    XGL_IMAGE_SUBRESOURCE                   imageSubresource;
    XGL_OFFSET3D                            imageOffset;
    XGL_EXTENT3D                            imageExtent;
} XGL_MEMORY_IMAGE_COPY;

typedef struct _XGL_IMAGE_RESOLVE
{
    XGL_IMAGE_SUBRESOURCE                   srcSubresource;
    XGL_OFFSET2D                            srcOffset;
    XGL_IMAGE_SUBRESOURCE                   destSubresource;
    XGL_OFFSET2D                            destOffset;
    XGL_EXTENT2D                            extent;
} XGL_IMAGE_RESOLVE;

typedef struct _XGL_SHADER_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;              // Must be XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO
    const XGL_VOID*                         pNext;              // Pointer to next structure
    XGL_SIZE                                codeSize;           // Specified in bytes
    const XGL_VOID*                         pCode;
    XGL_FLAGS                               flags;              // Reserved
} XGL_SHADER_CREATE_INFO;

struct _XGL_DESCRIPTOR_SET_MAPPING;

typedef struct _XGL_DESCRIPTOR_SLOT_INFO
{
    XGL_DESCRIPTOR_SET_SLOT_TYPE slotObjectType;
    union
    {
        XGL_UINT                                  shaderEntityIndex;// Shader IL slot index for given entity type
        const struct _XGL_DESCRIPTOR_SET_MAPPING* pNextLevelSet;    // Pointer to next descriptor set level
    };
} XGL_DESCRIPTOR_SLOT_INFO;

typedef struct _XGL_DESCRIPTOR_SET_MAPPING
{
    XGL_UINT                                    descriptorCount;
    const XGL_DESCRIPTOR_SLOT_INFO*             pDescriptorInfo;
} XGL_DESCRIPTOR_SET_MAPPING;

typedef struct _XGL_LINK_CONST_BUFFER
{
    XGL_UINT                                    bufferId;
    XGL_SIZE                                    bufferSize;
    const XGL_VOID*                             pBufferData;
} XGL_LINK_CONST_BUFFER;

typedef struct _XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO
{
    XGL_DESCRIPTOR_SET_SLOT_TYPE            slotObjectType;
    XGL_UINT                                shaderEntityIndex;
} XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO;

typedef struct _XGL_PIPELINE_SHADER
{
    XGL_PIPELINE_SHADER_STAGE               stage;
    XGL_SHADER                              shader;
    XGL_DESCRIPTOR_SET_MAPPING              descriptorSetMapping[XGL_MAX_DESCRIPTOR_SETS];
    XGL_UINT                                linkConstBufferCount;
    const XGL_LINK_CONST_BUFFER*            pLinkConstBufferInfo;
    XGL_DYNAMIC_MEMORY_VIEW_SLOT_INFO       dynamicMemoryViewMapping;
} XGL_PIPELINE_SHADER;

typedef struct _XGL_COMPUTE_PIPELINE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_PIPELINE_SHADER                     cs;
    XGL_FLAGS                               flags;      // XGL_PIPELINE_CREATE_FLAGS
} XGL_COMPUTE_PIPELINE_CREATE_INFO;

// IMG CHANGE BEGIN - support for vertex input description

// Example descriptor set mapping:
//
// {
//     .descriptorCount = 4;
//     .pDescriptorInfo[0] =
//     {
//         .slotObjectType = XGL_SLOT_VERTEX_INPUT;
//         .shaderEntityIndex = 2;  // describes XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO.pVertexBindingDescriptions[2]
//     }
//     .pDescriptorInfo[1] =
//     {
//         .slotObjectType = XGL_SLOT_UNUSED;
//         .shaderEntityIndex = 0;
//     }
//     .pDescriptorInfo[2] =
//     {
//         .slotObjectType = XGL_SLOT_VERTEX_INPUT;
//         .shaderEntityIndex = 0;  // describes XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO.pVertexBindingDescriptions[0]
//     }
//     .pDescriptorInfo[3] =
//     {
//         .slotObjectType = XGL_SLOT_VERTEX_INPUT;
//         .shaderEntityIndex = 1;  // describes XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO.pVertexBindingDescriptions[1]
//     }
// }
//
// The shader inputs are mapped to pVertexAttributeDescriptions using a decoration in the BIL.
//
//
// Formats allowed for attributes (XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION.format) will be detailed in
// a table in the specification.
//
//
// Queryable limits:
//
// XGL_VERTEX_INPUT_BINDING_DESCRIPTION.strideInBytes
// XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION.offsetInBytes
//
//

typedef struct _XGL_VERTEX_INPUT_BINDING_DESCRIPTION
{
    XGL_UINT                                strideInBytes;  // Distance between vertices in bytes (0 = no advancement)

    XGL_VERTEX_INPUT_STEP_RATE              stepRate;       // Rate at which binding is incremented
} XGL_VERTEX_INPUT_BINDING_DESCRIPTION;

typedef struct _XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION
{
    XGL_UINT                                binding;        // index into vertexBindingDescriptions

    XGL_FORMAT                              format;	        // format of source data

    XGL_UINT                                offsetInBytes;  // Offset of first element in bytes from base of vertex
} XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION;

typedef struct _XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;          // Should be XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO
    XGL_VOID*                               pNext;          // Pointer to next structure

    XGL_UINT                                bindingCount;   // number of bindings
    XGL_VERTEX_INPUT_BINDING_DESCRIPTION*   pVertexBindingDescriptions;

    XGL_UINT                                attributeCount; // number of attributes
    XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pVertexAttributeDescriptions;
} XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO;
// IMG CHANGE END

typedef struct _XGL_PIPELINE_IA_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_PRIMITIVE_TOPOLOGY                  topology;
    XGL_BOOL                                disableVertexReuse;
    XGL_PROVOKING_VERTEX_CONVENTION         provokingVertex;
    XGL_BOOL                                primitiveRestartEnable;
    XGL_UINT32                              primitiveRestartIndex;
} XGL_PIPELINE_IA_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_TESS_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_UINT                                patchControlPoints;
    XGL_FLOAT                               optimalTessFactor;
    XGL_FLOAT                               fixedTessFactor;
} XGL_PIPELINE_TESS_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_RS_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_BOOL                                depthClipEnable;
    XGL_BOOL                                rasterizerDiscardEnable;
    XGL_FLOAT                               pointSize;  // Size of points
} XGL_PIPELINE_RS_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_CB_ATTACHMENT_STATE
{
    XGL_BOOL                                blendEnable;
    XGL_FORMAT                              format;
    XGL_UINT8                               channelWriteMask;
} XGL_PIPELINE_CB_ATTACHMENT_STATE;

typedef struct _XGL_PIPELINE_CB_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_BOOL                                alphaToCoverageEnable;
    XGL_BOOL                                dualSourceBlendEnable;
    XGL_LOGIC_OP                            logicOp;
    XGL_PIPELINE_CB_ATTACHMENT_STATE        attachment[XGL_MAX_COLOR_ATTACHMENTS];
} XGL_PIPELINE_CB_STATE;

typedef struct _XGL_PIPELINE_DB_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_FORMAT format;
} XGL_PIPELINE_DB_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_SHADER_STAGE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_PIPELINE_SHADER                     shader;
} XGL_PIPELINE_SHADER_STAGE_CREATE_INFO;

typedef struct _XGL_GRAPHICS_PIPELINE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_FLAGS                               flags;      // XGL_PIPELINE_CREATE_FLAGS
} XGL_GRAPHICS_PIPELINE_CREATE_INFO;

typedef struct _XGL_SAMPLER_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;          // Must be XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
    const XGL_VOID*                         pNext;          // Pointer to next structure
    XGL_TEX_FILTER                          magFilter;      // Filter mode for magnification
    XGL_TEX_FILTER                          minFilter;      // Filter mode for minifiation
    XGL_TEX_MIPMAP_MODE                     mipMode;        // Mipmap selection mode
    XGL_TEX_ADDRESS                         addressU;
    XGL_TEX_ADDRESS                         addressV;
    XGL_TEX_ADDRESS                         addressW;
    XGL_FLOAT                               mipLodBias;
    XGL_UINT                                maxAnisotropy;
    XGL_COMPARE_FUNC                        compareFunc;
    XGL_FLOAT                               minLod;
    XGL_FLOAT                               maxLod;
    XGL_BORDER_COLOR_TYPE                   borderColorType;
} XGL_SAMPLER_CREATE_INFO;

typedef struct _XGL_DESCRIPTOR_SET_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_UINT slots;
} XGL_DESCRIPTOR_SET_CREATE_INFO;

typedef struct _XGL_DESCRIPTOR_SET_ATTACH_INFO
{
    XGL_DESCRIPTOR_SET                      descriptorSet;
    XGL_UINT                                slotOffset;
} XGL_DESCRIPTOR_SET_ATTACH_INFO;

typedef struct _XGL_VIEWPORT_STATE_CREATE_INFO
{
    XGL_UINT                                viewportCount;
    XGL_BOOL                                scissorEnable;
    XGL_VIEWPORT                            viewports[XGL_MAX_VIEWPORTS];
    XGL_RECT                                scissors[XGL_MAX_VIEWPORTS];
} XGL_VIEWPORT_STATE_CREATE_INFO;

typedef struct _XGL_RASTER_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_FILL_MODE                           fillMode;
    XGL_CULL_MODE                           cullMode;
    XGL_FACE_ORIENTATION                    frontFace;
    XGL_INT                                 depthBias;
    XGL_FLOAT                               depthBiasClamp;
    XGL_FLOAT                               slopeScaledDepthBias;
} XGL_RASTER_STATE_CREATE_INFO;

typedef struct _XGL_MSAA_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_UINT                                samples;
    XGL_SAMPLE_MASK                         sampleMask;
} XGL_MSAA_STATE_CREATE_INFO;

typedef struct _XGL_COLOR_ATTACHMENT_BLEND_STATE
{
    XGL_BOOL                                blendEnable;
    XGL_BLEND                               srcBlendColor;
    XGL_BLEND                               destBlendColor;
    XGL_BLEND_FUNC                          blendFuncColor;
    XGL_BLEND                               srcBlendAlpha;
    XGL_BLEND                               destBlendAlpha;
    XGL_BLEND_FUNC                          blendFuncAlpha;
} XGL_COLOR_ATTACHMENT_BLEND_STATE;

typedef struct _XGL_COLOR_BLEND_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_COLOR_ATTACHMENT_BLEND_STATE        attachment[XGL_MAX_COLOR_ATTACHMENTS];
    XGL_FLOAT                               blendConst[4];
} XGL_COLOR_BLEND_STATE_CREATE_INFO;

typedef struct _XGL_STENCIL_OP_STATE
{
    XGL_STENCIL_OP                          stencilFailOp;
    XGL_STENCIL_OP                          stencilPassOp;
    XGL_STENCIL_OP                          stencilDepthFailOp;
    XGL_COMPARE_FUNC                        stencilFunc;
    XGL_UINT32                              stencilRef;
} XGL_STENCIL_OP_STATE;

typedef struct _XGL_DEPTH_STENCIL_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_BOOL                                depthTestEnable;
    XGL_BOOL                                depthWriteEnable;
    XGL_COMPARE_FUNC                        depthFunc;
    XGL_BOOL                                depthBoundsEnable;
    XGL_FLOAT                               minDepth;
    XGL_FLOAT                               maxDepth;
    XGL_BOOL                                stencilTestEnable;
    XGL_UINT32                              stencilReadMask;
    XGL_UINT32                              stencilWriteMask;
    XGL_STENCIL_OP_STATE                    front;
    XGL_STENCIL_OP_STATE                    back;
} XGL_DEPTH_STENCIL_STATE_CREATE_INFO;

typedef struct _XGL_CMD_BUFFER_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO
    const XGL_VOID*                         pNext;
    XGL_QUEUE_TYPE                          queueType;
    XGL_FLAGS                               flags;
} XGL_CMD_BUFFER_CREATE_INFO;

typedef struct _XGL_MEMORY_REF
{
    XGL_GPU_MEMORY                          mem;
    XGL_FLAGS                               flags;      // XGL_MEMORY_REF_FLAGS
} XGL_MEMORY_REF;

typedef struct _XGL_EVENT_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_FLAGS                               flags;      // Reserved
} XGL_EVENT_CREATE_INFO;

typedef struct _XGL_FENCE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_FLAGS                               flags;      // Reserved
} XGL_FENCE_CREATE_INFO;

typedef struct _XGL_QUEUE_SEMAPHORE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_UINT                                initialCount;
    XGL_FLAGS                               flags;      // XGL_SEMAPHORE_CREATE_FLAGS
} XGL_QUEUE_SEMAPHORE_CREATE_INFO;

typedef struct _XGL_QUEUE_SEMAPHORE_OPEN_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_QUEUE_SEMAPHORE                     sharedSemaphore;
} XGL_QUEUE_SEMAPHORE_OPEN_INFO;

typedef struct _XGL_PIPELINE_STATISTICS_DATA
{
    XGL_UINT64                              fsInvocations;            // Fragment shader invocations
    XGL_UINT64                              cPrimitives;              // Clipper primitives
    XGL_UINT64                              cInvocations;             // Clipper invocations
    XGL_UINT64                              vsInvocations;            // Vertex shader invocations
    XGL_UINT64                              gsInvocations;            // Geometry shader invocations
    XGL_UINT64                              gsPrimitives;             // Geometry shader primitives
    XGL_UINT64                              iaPrimitives;             // Input primitives
    XGL_UINT64                              iaVertices;               // Input vertices
    XGL_UINT64                              tcsInvocations;           // Tessellation control shader invocations
    XGL_UINT64                              tesInvocations;           // Tessellation evaluation shader invocations
    XGL_UINT64                              csInvocations;            // Compute shader invocations
} XGL_PIPELINE_STATISTICS_DATA;

typedef struct _XGL_QUERY_POOL_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO
    const XGL_VOID*                         pNext;      // Pointer to next structure
    XGL_QUERY_TYPE                          queryType;
    XGL_UINT                                slots;
} XGL_QUERY_POOL_CREATE_INFO;

typedef struct _XGL_DRAW_INDIRECT_CMD
{
    XGL_UINT32                              vertexCount;
    XGL_UINT32                              instanceCount;
    XGL_UINT32                              firstVertex;
    XGL_UINT32                              firstInstance;
} XGL_DRAW_INDIRECT_CMD;

typedef struct _XGL_DRAW_INDEXED_INDIRECT_CMD
{
    XGL_UINT32                              indexCount;
    XGL_UINT32                              instanceCount;
    XGL_UINT32                              firstIndex;
    XGL_INT32                               vertexOffset;
    XGL_UINT32                              firstInstance;
} XGL_DRAW_INDEXED_INDIRECT_CMD;

typedef struct _XGL_DISPATCH_INDIRECT_CMD
{
    XGL_UINT32 x;
    XGL_UINT32 y;
    XGL_UINT32 z;
} XGL_DISPATCH_INDIRECT_CMD;

// ------------------------------------------------------------------------------------------------
// API functions

// GPU initialization

XGL_RESULT XGLAPI xglInitAndEnumerateGpus(
    const XGL_APPLICATION_INFO*                 pAppInfo,
    const XGL_ALLOC_CALLBACKS*                  pAllocCb,
    XGL_UINT                                    maxGpus,
    XGL_UINT*                                   pGpuCount,
    XGL_PHYSICAL_GPU*                           pGpus);

XGL_RESULT XGLAPI xglGetGpuInfo(
    XGL_PHYSICAL_GPU                            gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE                  infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

// Device functions

XGL_RESULT XGLAPI xglCreateDevice(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_DEVICE_CREATE_INFO*               pCreateInfo,
    XGL_DEVICE*                                 pDevice);

XGL_RESULT XGLAPI xglDestroyDevice(
    XGL_DEVICE                                  device);

// Extension discovery functions

XGL_RESULT XGLAPI xglGetExtensionSupport(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_CHAR*                             pExtName);

// Queue functions

XGL_RESULT XGLAPI xglGetDeviceQueue(
    XGL_DEVICE                                  device,
    XGL_QUEUE_TYPE                              queueType,
    XGL_UINT                                    queueIndex,
    XGL_QUEUE*                                  pQueue);

XGL_RESULT XGLAPI xglQueueSubmit(
    XGL_QUEUE                                   queue,
    XGL_UINT                                    cmdBufferCount,
    const XGL_CMD_BUFFER*                       pCmdBuffers,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs,
    XGL_FENCE                                   fence);

XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(
    XGL_QUEUE                                   queue,
    XGL_UINT                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs);

XGL_RESULT XGLAPI xglQueueWaitIdle(
    XGL_QUEUE                                   queue);

XGL_RESULT XGLAPI xglDeviceWaitIdle(
    XGL_DEVICE                                  device);

// Memory functions

XGL_RESULT XGLAPI xglGetMemoryHeapCount(
    XGL_DEVICE                                  device,
    XGL_UINT*                                   pCount);

XGL_RESULT XGLAPI xglGetMemoryHeapInfo(
    XGL_DEVICE                                  device,
    XGL_UINT                                    heapId,
    XGL_MEMORY_HEAP_INFO_TYPE                   infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

XGL_RESULT XGLAPI xglAllocMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_ALLOC_INFO*                pAllocInfo,
    XGL_GPU_MEMORY*                             pMem);

XGL_RESULT XGLAPI xglFreeMemory(
    XGL_GPU_MEMORY                              mem);

XGL_RESULT XGLAPI xglSetMemoryPriority(
    XGL_GPU_MEMORY                              mem,
    XGL_MEMORY_PRIORITY                         priority);

XGL_RESULT XGLAPI xglMapMemory(
    XGL_GPU_MEMORY                              mem,
    XGL_FLAGS                                   flags,                // Reserved
    XGL_VOID**                                  ppData);

XGL_RESULT XGLAPI xglUnmapMemory(
    XGL_GPU_MEMORY                              mem);

XGL_RESULT XGLAPI xglPinSystemMemory(
    XGL_DEVICE                                  device,
    const XGL_VOID*                             pSysMem,
    XGL_SIZE                                    memSize,
    XGL_GPU_MEMORY*                             pMem);

XGL_RESULT XGLAPI xglRemapVirtualMemoryPages(
    XGL_DEVICE                                  device,
    XGL_UINT                                    rangeCount,
    const XGL_VIRTUAL_MEMORY_REMAP_RANGE*       pRanges,
    XGL_UINT                                    preWaitSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*                  pPreWaitSemaphores,
    XGL_UINT                                    postSignalSemaphoreCount,
    const XGL_QUEUE_SEMAPHORE*                  pPostSignalSemaphores);

// Multi-device functions

XGL_RESULT XGLAPI xglGetMultiGpuCompatibility(
    XGL_PHYSICAL_GPU                            gpu0,
    XGL_PHYSICAL_GPU                            gpu1,
    XGL_GPU_COMPATIBILITY_INFO*                 pInfo);

XGL_RESULT XGLAPI xglOpenSharedMemory(
    XGL_DEVICE                                  device,
    const XGL_MEMORY_OPEN_INFO*                 pOpenInfo,
    XGL_GPU_MEMORY*                             pMem);

XGL_RESULT XGLAPI xglOpenSharedQueueSemaphore(
    XGL_DEVICE                                  device,
    const XGL_QUEUE_SEMAPHORE_OPEN_INFO*        pOpenInfo,
    XGL_QUEUE_SEMAPHORE*                        pSemaphore);

XGL_RESULT XGLAPI xglOpenPeerMemory(
    XGL_DEVICE                                  device,
    const XGL_PEER_MEMORY_OPEN_INFO*            pOpenInfo,
    XGL_GPU_MEMORY*                             pMem);

XGL_RESULT XGLAPI xglOpenPeerImage(
    XGL_DEVICE                                  device,
    const XGL_PEER_IMAGE_OPEN_INFO*             pOpenInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem);

// Generic API object functions

XGL_RESULT XGLAPI xglDestroyObject(
    XGL_OBJECT                                  object);

XGL_RESULT XGLAPI xglGetObjectInfo(
    XGL_BASE_OBJECT                             object,
    XGL_OBJECT_INFO_TYPE                        infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

XGL_RESULT XGLAPI xglBindObjectMemory(
    XGL_OBJECT                                  object,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset);

// Fence functions

XGL_RESULT XGLAPI xglCreateFence(
    XGL_DEVICE                                  device,
    const XGL_FENCE_CREATE_INFO*                pCreateInfo,
    XGL_FENCE*                                  pFence);

XGL_RESULT XGLAPI xglGetFenceStatus(
    XGL_FENCE fence);

XGL_RESULT XGLAPI xglWaitForFences(
    XGL_DEVICE                                  device,
    XGL_UINT                                    fenceCount,
    const XGL_FENCE*                            pFences,
    XGL_BOOL                                    waitAll,
    XGL_UINT64                                  timeout);

// Queue semaphore functions

XGL_RESULT XGLAPI xglCreateQueueSemaphore(
    XGL_DEVICE                                  device,
    const XGL_QUEUE_SEMAPHORE_CREATE_INFO*      pCreateInfo,
    XGL_QUEUE_SEMAPHORE*                        pSemaphore);

XGL_RESULT XGLAPI xglSignalQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore);

XGL_RESULT XGLAPI xglWaitQueueSemaphore(
    XGL_QUEUE                                   queue,
    XGL_QUEUE_SEMAPHORE                         semaphore);

// Event functions

XGL_RESULT XGLAPI xglCreateEvent(
    XGL_DEVICE                                  device,
    const XGL_EVENT_CREATE_INFO*                pCreateInfo,
    XGL_EVENT*                                  pEvent);

XGL_RESULT XGLAPI xglGetEventStatus(
    XGL_EVENT                                   event);

XGL_RESULT XGLAPI xglSetEvent(
    XGL_EVENT                                   event);

XGL_RESULT XGLAPI xglResetEvent(
    XGL_EVENT                                   event);

// Query functions

XGL_RESULT XGLAPI xglCreateQueryPool(
    XGL_DEVICE                                  device,
    const XGL_QUERY_POOL_CREATE_INFO*           pCreateInfo,
    XGL_QUERY_POOL*                             pQueryPool);

XGL_RESULT XGLAPI xglGetQueryPoolResults(
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    startQuery,
    XGL_UINT                                    queryCount,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

// Format capabilities

XGL_RESULT XGLAPI xglGetFormatInfo(
    XGL_DEVICE                                  device,
    XGL_FORMAT                                  format,
    XGL_FORMAT_INFO_TYPE                        infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

// Image functions

XGL_RESULT XGLAPI xglCreateImage(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
    XGL_IMAGE*                                  pImage);

XGL_RESULT XGLAPI xglGetImageSubresourceInfo(
    XGL_IMAGE                                   image,
    const XGL_IMAGE_SUBRESOURCE*                pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE                   infoType,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

// Image view functions

XGL_RESULT XGLAPI xglCreateImageView(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_VIEW_CREATE_INFO*           pCreateInfo,
    XGL_IMAGE_VIEW*                             pView);

XGL_RESULT XGLAPI xglCreateColorAttachmentView(
    XGL_DEVICE                                  device,
    const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    XGL_COLOR_ATTACHMENT_VIEW*                  pView);

XGL_RESULT XGLAPI xglCreateDepthStencilView(
    XGL_DEVICE                                  device,
    const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*   pCreateInfo,
    XGL_DEPTH_STENCIL_VIEW*                     pView);

// Shader functions

XGL_RESULT XGLAPI xglCreateShader(
    XGL_DEVICE                                  device,
    const XGL_SHADER_CREATE_INFO*               pCreateInfo,
    XGL_SHADER*                                 pShader);

// Pipeline functions

XGL_RESULT XGLAPI xglCreateGraphicsPipeline(
    XGL_DEVICE                                  device,
    const XGL_GRAPHICS_PIPELINE_CREATE_INFO*    pCreateInfo,
    XGL_PIPELINE*                               pPipeline);

XGL_RESULT XGLAPI xglCreateComputePipeline(
    XGL_DEVICE                                  device,
    const XGL_COMPUTE_PIPELINE_CREATE_INFO*     pCreateInfo,
    XGL_PIPELINE*                               pPipeline);

XGL_RESULT XGLAPI xglStorePipeline(
    XGL_PIPELINE                                pipeline,
    XGL_SIZE*                                   pDataSize,
    XGL_VOID*                                   pData);

XGL_RESULT XGLAPI xglLoadPipeline(
    XGL_DEVICE                                  device,
    XGL_SIZE                                    dataSize,
    const XGL_VOID*                             pData,
    XGL_PIPELINE*                               pPipeline);

XGL_RESULT XGLAPI xglCreatePipelineDelta(
    XGL_DEVICE                                  device,
    XGL_PIPELINE                                p1,
    XGL_PIPELINE                                p2,
    XGL_PIPELINE_DELTA*                         delta);

// Sampler functions

XGL_RESULT XGLAPI xglCreateSampler(
    XGL_DEVICE                                  device,
    const XGL_SAMPLER_CREATE_INFO*              pCreateInfo,
    XGL_SAMPLER*                                pSampler);

// Descriptor set functions

XGL_RESULT XGLAPI xglCreateDescriptorSet(
    XGL_DEVICE                                  device,
    const XGL_DESCRIPTOR_SET_CREATE_INFO*       pCreateInfo,
    XGL_DESCRIPTOR_SET*                         pDescriptorSet);

XGL_VOID XGLAPI xglBeginDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET                          descriptorSet);

XGL_VOID XGLAPI xglEndDescriptorSetUpdate(
    XGL_DESCRIPTOR_SET                          descriptorSet);

XGL_VOID XGLAPI xglAttachSamplerDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_SAMPLER*                          pSamplers);

XGL_VOID XGLAPI xglAttachImageViewDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_IMAGE_VIEW_ATTACH_INFO*           pImageViews);

XGL_VOID XGLAPI xglAttachMemoryViewDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemViews);

XGL_VOID XGLAPI xglAttachNestedDescriptors(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount,
    const XGL_DESCRIPTOR_SET_ATTACH_INFO*       pNestedDescriptorSets);

XGL_VOID XGLAPI xglClearDescriptorSetSlots(
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    startSlot,
    XGL_UINT                                    slotCount);

// State object functions

XGL_RESULT XGLAPI xglCreateViewportState(
    XGL_DEVICE                                  device,
    const XGL_VIEWPORT_STATE_CREATE_INFO*       pCreateInfo,
    XGL_VIEWPORT_STATE_OBJECT*                  pState);

XGL_RESULT XGLAPI xglCreateRasterState(
    XGL_DEVICE                                  device,
    const XGL_RASTER_STATE_CREATE_INFO*         pCreateInfo,
    XGL_RASTER_STATE_OBJECT*                    pState);

XGL_RESULT XGLAPI xglCreateMsaaState(
    XGL_DEVICE                                  device,
    const XGL_MSAA_STATE_CREATE_INFO*           pCreateInfo,
    XGL_MSAA_STATE_OBJECT*                      pState);

XGL_RESULT XGLAPI xglCreateColorBlendState(
    XGL_DEVICE                                  device,
    const XGL_COLOR_BLEND_STATE_CREATE_INFO*    pCreateInfo,
    XGL_COLOR_BLEND_STATE_OBJECT*               pState);

XGL_RESULT XGLAPI xglCreateDepthStencilState(
    XGL_DEVICE                                  device,
    const XGL_DEPTH_STENCIL_STATE_CREATE_INFO*  pCreateInfo,
    XGL_DEPTH_STENCIL_STATE_OBJECT*             pState);

// Command buffer functions

XGL_RESULT XGLAPI xglCreateCommandBuffer(
    XGL_DEVICE                                  device,
    const XGL_CMD_BUFFER_CREATE_INFO*           pCreateInfo,
    XGL_CMD_BUFFER*                             pCmdBuffer);

XGL_RESULT XGLAPI xglBeginCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_FLAGS                                   flags);               // XGL_CMD_BUFFER_BUILD_FLAGS

XGL_RESULT XGLAPI xglEndCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer);

XGL_RESULT XGLAPI xglResetCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer);

// Command buffer building functions

XGL_VOID XGLAPI xglCmdBindPipeline(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE                                pipeline);

XGL_VOID XGLAPI xglCmdBindPipelineDelta(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE_DELTA                          delta);

XGL_VOID XGLAPI xglCmdBindStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_STATE_OBJECT                            state);

XGL_VOID XGLAPI xglCmdBindDescriptorSet(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    index,
    XGL_DESCRIPTOR_SET                          descriptorSet,
    XGL_UINT                                    slotOffset);

XGL_VOID XGLAPI xglCmdBindDynamicMemoryView(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView);

XGL_VOID XGLAPI xglCmdBindIndexData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType);

XGL_VOID XGLAPI xglCmdBindAttachments(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    colorAttachmentCount,
    const XGL_COLOR_ATTACHMENT_BIND_INFO*       pColorAttachments,
    const XGL_DEPTH_STENCIL_BIND_INFO*          pDepthStencilAttachment);

XGL_VOID XGLAPI xglCmdPrepareMemoryRegions(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_MEMORY_STATE_TRANSITION*          pStateTransitions);

XGL_VOID XGLAPI xglCmdPrepareImages(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    transitionCount,
    const XGL_IMAGE_STATE_TRANSITION*           pStateTransitions);

XGL_VOID XGLAPI xglCmdDraw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstVertex,
    XGL_UINT                                    vertexCount,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount);

XGL_VOID XGLAPI xglCmdDrawIndexed(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    firstIndex,
    XGL_UINT                                    indexCount,
    XGL_INT                                     vertexOffset,
    XGL_UINT                                    firstInstance,
    XGL_UINT                                    instanceCount);

XGL_VOID XGLAPI xglCmdDrawIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride);

XGL_VOID XGLAPI xglCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset,
    XGL_UINT32                                  count,
    XGL_UINT32                                  stride);

XGL_VOID XGLAPI xglCmdDispatch(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_UINT                                    x,
    XGL_UINT                                    y,
    XGL_UINT                                    z);

XGL_VOID XGLAPI xglCmdDispatchIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                offset);

XGL_VOID XGLAPI xglCmdCopyMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_COPY*                      pRegions);

XGL_VOID XGLAPI xglCmdCopyImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_IMAGE_COPY*                       pRegions);

XGL_VOID XGLAPI xglCmdCopyMemoryToImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              srcMem,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions);

XGL_VOID XGLAPI xglCmdCopyImageToMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_GPU_MEMORY                              destMem,
    XGL_UINT                                    regionCount,
    const XGL_MEMORY_IMAGE_COPY*                pRegions);

XGL_VOID XGLAPI xglCmdCloneImageData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_STATE                             srcImageState,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_STATE                             destImageState);

XGL_VOID XGLAPI xglCmdUpdateMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                dataSize,
    const XGL_UINT32*                           pData);

XGL_VOID XGLAPI xglCmdFillMemory(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                fillSize,
    XGL_UINT32                                  data);

XGL_VOID XGLAPI xglCmdClearColorImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_FLOAT                             color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

XGL_VOID XGLAPI xglCmdClearColorImageRaw(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    const XGL_UINT32                            color[4],
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

XGL_VOID XGLAPI xglCmdClearDepthStencil(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    XGL_FLOAT                                   depth,
    XGL_UINT32                                  stencil,
    XGL_UINT                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

XGL_VOID XGLAPI xglCmdResolveImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    XGL_UINT                                    rectCount,
    const XGL_IMAGE_RESOLVE*                    pRects);

XGL_VOID XGLAPI xglCmdSetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event);

XGL_VOID XGLAPI xglCmdResetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event);

XGL_VOID XGLAPI xglCmdMemoryAtomic(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset,
    XGL_UINT64                                  srcData,
    XGL_ATOMIC_OP                               atomicOp);

XGL_VOID XGLAPI xglCmdBeginQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot,
    XGL_FLAGS                                   flags);

XGL_VOID XGLAPI xglCmdEndQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    slot);

XGL_VOID XGLAPI xglCmdResetQueryPool(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    XGL_UINT                                    startQuery,
    XGL_UINT                                    queryCount);

XGL_VOID XGLAPI xglCmdWriteTimestamp(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_TIMESTAMP_TYPE                          timestampType,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset);

XGL_VOID XGLAPI xglCmdInitAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    const XGL_UINT32*                           pData);

XGL_VOID XGLAPI xglCmdLoadAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              srcMem,
    XGL_GPU_SIZE                                srcOffset);

XGL_VOID XGLAPI xglCmdSaveAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_UINT                                    startCounter,
    XGL_UINT                                    counterCount,
    XGL_GPU_MEMORY                              destMem,
    XGL_GPU_SIZE                                destOffset);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __XGL_H__

/******************************************************************************************

    Open Issues + Missing Features
    ------------------------------

    Here are a few higher level issues that we'd like to fix given time. A feature missing
    from this header (or the following list) isn't necessarily an indication that we want
    to drop that feature. Only that we either haven't thought of it or haven't had time
    to add it yet.

    1) Transform Feedback (XFB)

    OpenGL supports transform feedback (XFB). That is not included in this header, but
    we feel there is likely value in including it.

    To incorporate trasnform feedback, we could create a new pipeline stage. This would
    be injected into a PSO by including the following in the chain:

    typedef struct _XGL_XFB_CREATE_INFO
    {
        XGL_STRUCTURE_TYPE    sType;  // Must be XGL_STRUCTURE_TYPE_PIPELINE_XFB_CREATE_INFO
        const XGL_VOID*       pNext;  // Pointer to next structure
        // More XFB state, if any goes here
    } XGL_DEPTH_STENCIL_VIEW_CREATE_INFO;

    We expect that only the shader-side configuration (via layout qualifiers or their IR
    equivalent) is used to configure the data written to each stream. When transform
    feedback is part of the pipeline, transform feedback binding would be available
    through a new API bind point:

        xglCmdBindTransformFeedbackMemoryView(
                XGL_CMD_BUFFER                              cmdBuffer,
                XGL_PIPELINE_BIND_POINT                     pipelineBindPoint, // = GRAPHICS
                XGL_UINT                                    index,
                const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView);

    2) Framebuffer Objects

    The XGL API here doesn't have a direct equivalent for a framebuffer object. In GL,
    the framebuffer object owns attachments, and the indirection table for glDrawBuffers, etc.
    The indirection is gone - only identity is supported here.

    We may introduce an analog to the framebuffer object that packages all color
    attachments. You would create a framebuffer thus:

    typedef struct _XGL_FRAMEBUFFER_CREATE_INFO
    {
        XGL_STRUCTURE_TYPE    sType;  // Must be XGL_STRUCTURE_TYPE_PIPELINE_XFB_CREATE_INFO
        const XGL_VOID*       pNext;  // Pointer to next structure
        XGL_UINT32            colorAttachmentCount;
        XGL_COLOR_ATTACHMENT_BIND_INFO* pColorAttachments;
        XGL_DEPTH_STENCIL_BIND_INFO pDepthStencilAttachment;
    } XGL_FRAMEBUFFER_CREATE_INFO;

        xglCreateFramebuffer(
            XGL_DEVICE device,
            const XGL_FRAMEBUFFER_CREATE_INFO* pInfo,
            XGL_FRAMEBUFFER* pFramebuffer);

    We then replace the xglCmdBindAttachments API with:

        xglBindFramebuffer(
            XGL_CMD_BUFFER cmdBuffer,
            XGL_PIPELINE_BIND_POINT pipelineBindPoint, // = GRAPHICS
            XGL_FRAMEBUFFER framebuffer);

    3) "Bindless" + support for non-bindless hardware.

    XGL doesn't have bindless textures the way that GL does. It has resource descriptor
    sets, or resource tables. Resource tables can be nested and hold references to more
    resource tables. They are explicitly sized by the application and have no artificial
    upper size limit. An application can still attach as many textures as they want to
    a resource descriptor set, and can modify the set asynchronously to GPU work.
    Therefore, we can still have "unlimited textures". An application hoping to use
    bindless can use an index into a large table of textures and achieve the same effect.

    For non-bindless hardware, with fixed (but potentially large) register files for
    resource bindings, the table approach should still work if a limited size can be
    reported somehow.

    4) Clean up some remaining Mantle'isms.

    Queue types: It's a bit hand wavey. In Mantle, we have a "universal" queue type that
    supports compute and graphics and a "compute" queue that only supports compute. Devices
    must support at least one universal queue and DMA queues are an extension. I would like
    to do the following (and have attempted to do that here, but am only half done):

        a) Separate out the queue capabilities (compute, DMA, graphics) and allow support
           for any number of queues with any combination of capabilities each.

        b) Allow compute-only or even DMA-only (like video capture or SDI) devices to
           be supported.

        c) Allow new queue types to be supported by extensions without having to allocate
           bits in the bitfield until they're promoted to core.

    Terminology: There are still some references to "targets" (render targets) and other
    terminology that has been changed from Mantle. Need to do a clean-up pass.

    4) The window system interface is an extension in Mantle. We have not tried to fold
       any of it into core here. There is no mention of SwapBuffers, presentation, default
       framebuffers or anything like that. In the extension, presentation is queued up into
       the graphics queue just like any other command.

*******************************************************************************************/
