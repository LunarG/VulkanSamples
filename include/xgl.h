//
// File: xgl.h
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

#ifndef __XGL_H__
#define __XGL_H__

#define XGL_MAKE_VERSION(major, minor, patch) \
    ((major << 22) | (minor << 12) | patch)

#include "xglPlatform.h"

// XGL API version supported by this file
#define XGL_API_VERSION XGL_MAKE_VERSION(0, 57, 1)

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
***************************************************************************************************
*   Core XGL API
***************************************************************************************************
*/

#ifdef __cplusplus
    #define XGL_DEFINE_HANDLE(_obj) struct _obj##_T {char _dummy;}; typedef _obj##_T* _obj;
    #define XGL_DEFINE_SUBCLASS_HANDLE(_obj, _base) struct _obj##_T : public _base##_T {}; typedef _obj##_T* _obj;
#else // __cplusplus
    #define XGL_DEFINE_HANDLE(_obj) typedef void* _obj;
    #define XGL_DEFINE_SUBCLASS_HANDLE(_obj, _base) typedef void* _obj;
#endif // __cplusplus

XGL_DEFINE_HANDLE(XGL_INSTANCE)
XGL_DEFINE_HANDLE(XGL_PHYSICAL_GPU)
XGL_DEFINE_HANDLE(XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DEVICE, XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_QUEUE, XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_GPU_MEMORY, XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_OBJECT, XGL_BASE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_BUFFER, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_BUFFER_VIEW, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_IMAGE, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_IMAGE_VIEW, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_COLOR_ATTACHMENT_VIEW, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DEPTH_STENCIL_VIEW, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_SHADER, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_PIPELINE, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_PIPELINE_DELTA, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_SAMPLER, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DESCRIPTOR_SET, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DESCRIPTOR_SET_LAYOUT, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DESCRIPTOR_REGION, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DYNAMIC_STATE_OBJECT, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DYNAMIC_VP_STATE_OBJECT, XGL_DYNAMIC_STATE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DYNAMIC_RS_STATE_OBJECT, XGL_DYNAMIC_STATE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DYNAMIC_CB_STATE_OBJECT, XGL_DYNAMIC_STATE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_DYNAMIC_DS_STATE_OBJECT, XGL_DYNAMIC_STATE_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_CMD_BUFFER, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_FENCE, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_QUEUE_SEMAPHORE, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_EVENT, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_QUERY_POOL, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_FRAMEBUFFER, XGL_OBJECT)
XGL_DEFINE_SUBCLASS_HANDLE(XGL_RENDER_PASS, XGL_OBJECT)

#define XGL_MAX_PHYSICAL_GPUS       16
#define XGL_MAX_PHYSICAL_GPU_NAME   256

#define XGL_LOD_CLAMP_NONE       MAX_FLOAT
#define XGL_LAST_MIP_OR_SLICE    0xffffffff

#define XGL_TRUE  1
#define XGL_FALSE 0

#define XGL_NULL_HANDLE 0

// This macro defines INT_MAX in enumerations to force compilers to use 32 bits
// to represent them. This may or may not be necessary on some compilers. The
// option to compile it out may allow compilers that warn about missing enumerants
// in switch statements to be silenced.
#define XGL_MAX_ENUM(T) T##_MAX_ENUM = 0x7FFFFFFF

// ------------------------------------------------------------------------------------------------
// Enumerations


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
    XGL_MAX_ENUM(_XGL_MEMORY_PRIORITY)
} XGL_MEMORY_PRIORITY;

typedef enum _XGL_IMAGE_LAYOUT
{
    XGL_IMAGE_LAYOUT_GENERAL                                = 0x00000000,   // General layout when image can be used for any kind of access
    XGL_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL               = 0x00000001,   // Optimal layout when image is only used for color attachment read/write
    XGL_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL       = 0x00000002,   // Optimal layout when image is only used for depth/stencil attachment read/write
    XGL_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL        = 0x00000003,   // Optimal layout when image is used for read only depth/stencil attachment and shader access
    XGL_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL               = 0x00000004,   // Optimal layout when image is used for read only shader access
    XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL                          = 0x00000005,   // Optimal layout when image is used only for clear operations
    XGL_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL                = 0x00000006,   // Optimal layout when image is used only as source of transfer operations
    XGL_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL           = 0x00000007,   // Optimal layout when image is used only as destination of transfer operations

    XGL_IMAGE_LAYOUT_BEGIN_RANGE                            = XGL_IMAGE_LAYOUT_GENERAL,
    XGL_IMAGE_LAYOUT_END_RANGE                              = XGL_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,
    XGL_NUM_IMAGE_LAYOUT                                    = (XGL_IMAGE_LAYOUT_END_RANGE - XGL_IMAGE_LAYOUT_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_IMAGE_LAYOUT)
} XGL_IMAGE_LAYOUT;

typedef enum _XGL_SET_EVENT
{
    XGL_SET_EVENT_TOP_OF_PIPE                               = 0x00000001,   // Set event before the GPU starts processing subsequent command
    XGL_SET_EVENT_VERTEX_PROCESSING_COMPLETE                = 0x00000002,   // Set event when all pending vertex processing is complete
    XGL_SET_EVENT_FRAGMENT_PROCESSING_COMPLETE              = 0x00000003,   // Set event when all pending fragment shader executions are complete
    XGL_SET_EVENT_GRAPHICS_PIPELINE_COMPLETE                = 0x00000004,   // Set event when all pending graphics operations are complete
    XGL_SET_EVENT_COMPUTE_PIPELINE_COMPLETE                 = 0x00000005,   // Set event when all pending compute operations are complete
    XGL_SET_EVENT_TRANSFER_COMPLETE                         = 0x00000006,   // Set event when all pending transfer operations are complete
    XGL_SET_EVENT_GPU_COMMANDS_COMPLETE                     = 0x00000007,   // Set event when all pending GPU work is complete

    XGL_SET_EVENT_BEGIN_RANGE                               = XGL_SET_EVENT_TOP_OF_PIPE,
    XGL_SET_EVENT_END_RANGE                                 = XGL_SET_EVENT_GPU_COMMANDS_COMPLETE,
    XGL_NUM_SET_EVENT                                       = (XGL_SET_EVENT_END_RANGE - XGL_SET_EVENT_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_SET_EVENT)
} XGL_SET_EVENT;

typedef enum _XGL_WAIT_EVENT
{
    XGL_WAIT_EVENT_TOP_OF_PIPE                              = 0x00000001,   // Wait event before the GPU starts processing subsequent commands
    XGL_WAIT_EVENT_BEFORE_RASTERIZATION                     = 0x00000002,   // Wait event before rasterizing subsequent primitives

    XGL_WAIT_EVENT_BEGIN_RANGE                              = XGL_WAIT_EVENT_TOP_OF_PIPE,
    XGL_WAIT_EVENT_END_RANGE                                = XGL_WAIT_EVENT_BEFORE_RASTERIZATION,
    XGL_NUM_WAIT_EVENT                                      = (XGL_WAIT_EVENT_END_RANGE - XGL_WAIT_EVENT_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_WAIT_EVENT)
} XGL_WAIT_EVENT;

typedef enum _XGL_MEMORY_OUTPUT_FLAGS
{
    XGL_MEMORY_OUTPUT_CPU_WRITE_BIT                         = 0x00000001,   // Controls output coherency of CPU writes
    XGL_MEMORY_OUTPUT_SHADER_WRITE_BIT                      = 0x00000002,   // Controls output coherency of generic shader writes
    XGL_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT                  = 0x00000004,   // Controls output coherency of color attachment writes
    XGL_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT          = 0x00000008,   // Controls output coherency of depth/stencil attachment writes
    XGL_MEMORY_OUTPUT_COPY_BIT                              = 0x00000010,   // Controls output coherency of copy operations
    XGL_MAX_ENUM(_XGL_MEMORY_OUTPUT_FLAGS)
} XGL_MEMORY_OUTPUT_FLAGS;

typedef enum _XGL_MEMORY_INPUT_FLAGS
{
    XGL_MEMORY_INPUT_CPU_READ_BIT                           = 0x00000001,   // Controls input coherency of CPU reads
    XGL_MEMORY_INPUT_INDIRECT_COMMAND_BIT                   = 0x00000002,   // Controls input coherency of indirect command reads
    XGL_MEMORY_INPUT_INDEX_FETCH_BIT                        = 0x00000004,   // Controls input coherency of index fetches
    XGL_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT             = 0x00000008,   // Controls input coherency of vertex attribute fetches
    XGL_MEMORY_INPUT_UNIFORM_READ_BIT                       = 0x00000010,   // Controls input coherency of uniform buffer reads
    XGL_MEMORY_INPUT_SHADER_READ_BIT                        = 0x00000020,   // Controls input coherency of generic shader reads
    XGL_MEMORY_INPUT_COLOR_ATTACHMENT_BIT                   = 0x00000040,   // Controls input coherency of color attachment reads
    XGL_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT           = 0x00000080,   // Controls input coherency of depth/stencil attachment reads
    XGL_MEMORY_INPUT_COPY_BIT                               = 0x00000100,   // Controls input coherency of copy operations
    XGL_MAX_ENUM(_XGL_MEMORY_INPUT_FLAGS)
} XGL_MEMORY_INPUT_FLAGS;

typedef enum _XGL_ATTACHMENT_LOAD_OP
{
    XGL_ATTACHMENT_LOAD_OP_LOAD                             = 0x00000000,
    XGL_ATTACHMENT_LOAD_OP_CLEAR                            = 0x00000001,
    XGL_ATTACHMENT_LOAD_OP_DONT_CARE                        = 0x00000002,
    
    XGL_ATTACHMENT_LOAD_OP_BEGIN_RANGE                      = XGL_ATTACHMENT_LOAD_OP_LOAD,
    XGL_ATTACHMENT_LOAD_OP_END_RANGE                        = XGL_ATTACHMENT_LOAD_OP_DONT_CARE,
    XGL_NUM_ATTACHMENT_LOAD_OP                              = (XGL_ATTACHMENT_LOAD_OP_END_RANGE - XGL_ATTACHMENT_LOAD_OP_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_ATTACHMENT_LOAD_OP)
} XGL_ATTACHMENT_LOAD_OP;

typedef enum _XGL_ATTACHMENT_STORE_OP
{
    XGL_ATTACHMENT_STORE_OP_STORE                           = 0x00000000,
    XGL_ATTACHMENT_STORE_OP_RESOLVE_MSAA                    = 0x00000001,
    XGL_ATTACHMENT_STORE_OP_DONT_CARE                       = 0x00000002,
    
    XGL_ATTACHMENT_STORE_OP_BEGIN_RANGE                     = XGL_ATTACHMENT_STORE_OP_STORE,
    XGL_ATTACHMENT_STORE_OP_END_RANGE                       = XGL_ATTACHMENT_STORE_OP_DONT_CARE,
    XGL_NUM_ATTACHMENT_STORE_OP                             = (XGL_ATTACHMENT_STORE_OP_END_RANGE - XGL_ATTACHMENT_STORE_OP_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_ATTACHMENT_STORE_OP)
} XGL_ATTACHMENT_STORE_OP;

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

typedef enum _XGL_DESCRIPTOR_TYPE
{
    XGL_DESCRIPTOR_TYPE_SAMPLER                             = 0x00000000,
    XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE                     = 0x00000001,
    XGL_DESCRIPTOR_TYPE_TEXTURE                             = 0x00000002,
    XGL_DESCRIPTOR_TYPE_TEXTURE_BUFFER                      = 0x00000003,
    XGL_DESCRIPTOR_TYPE_IMAGE                               = 0x00000004,
    XGL_DESCRIPTOR_TYPE_IMAGE_BUFFER                        = 0x00000005,
    XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER                      = 0x00000006,
    XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER               = 0x00000007,
    XGL_DESCRIPTOR_TYPE_RAW_BUFFER                          = 0x00000008,
    XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC              = 0x00000009,
    XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER_DYNAMIC       = 0x0000000a,
    XGL_DESCRIPTOR_TYPE_RAW_BUFFER_DYNAMIC                  = 0x0000000b,

    XGL_DESCRIPTOR_TYPE_BEGIN_RANGE                         = XGL_DESCRIPTOR_TYPE_SAMPLER,
    XGL_DESCRIPTOR_TYPE_END_RANGE                           = XGL_DESCRIPTOR_TYPE_RAW_BUFFER_DYNAMIC,
    XGL_NUM_DESCRIPTOR_TYPE                                 = (XGL_DESCRIPTOR_TYPE_END_RANGE - XGL_DESCRIPTOR_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_DESCRIPTOR_TYPE)
} XGL_DESCRIPTOR_TYPE;

typedef enum _XGL_DESCRIPTOR_REGION_USAGE
{
    XGL_DESCRIPTOR_REGION_USAGE_ONE_SHOT                    = 0x00000000,
    XGL_DESCRIPTOR_REGION_USAGE_DYNAMIC                     = 0x00000001,

    XGL_DESCRIPTOR_REGION_USAGE_BEGIN_RANGE                 = XGL_DESCRIPTOR_REGION_USAGE_ONE_SHOT,
    XGL_DESCRIPTOR_REGION_USAGE_END_RANGE                   = XGL_DESCRIPTOR_REGION_USAGE_DYNAMIC,
    XGL_NUM_DESCRIPTOR_REGION_USAGE                         = (XGL_DESCRIPTOR_REGION_USAGE_END_RANGE - XGL_DESCRIPTOR_REGION_USAGE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_DESCRIPTOR_REGION_USAGE)
} XGL_DESCRIPTOR_REGION_USAGE;

typedef enum _XGL_DESCRIPTOR_UPDATE_MODE
{
    XGL_DESCRIPTOR_UDPATE_MODE_COPY                         = 0x00000000,
    XGL_DESCRIPTOR_UPDATE_MODE_FASTEST                      = 0x00000001,

    XGL_DESCRIPTOR_UPDATE_MODE_BEGIN_RANGE                  = XGL_DESCRIPTOR_UDPATE_MODE_COPY,
    XGL_DESCRIPTOR_UPDATE_MODE_END_RANGE                    = XGL_DESCRIPTOR_UPDATE_MODE_FASTEST,
    XGL_NUM_DESCRIPTOR_UPDATE_MODE                          = (XGL_DESCRIPTOR_UPDATE_MODE_END_RANGE - XGL_DESCRIPTOR_UPDATE_MODE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_DESCRIPTOR_UPDATE_MODE)
} XGL_DESCRIPTOR_UPDATE_MODE;

typedef enum _XGL_DESCRIPTOR_SET_USAGE
{
    XGL_DESCRIPTOR_SET_USAGE_ONE_SHOT                       = 0x00000000,
    XGL_DESCRIPTOR_SET_USAGE_STATIC                         = 0x00000001,

    XGL_DESCRIPTOR_SET_USAGE_BEGIN_RANGE                    = XGL_DESCRIPTOR_SET_USAGE_ONE_SHOT,
    XGL_DESCRIPTOR_SET_USAGE_END_RANGE                      = XGL_DESCRIPTOR_SET_USAGE_STATIC,
    XGL_NUM_DESCRIPTOR_SET_USAGE                            = (XGL_DESCRIPTOR_SET_USAGE_END_RANGE - XGL_DESCRIPTOR_SET_USAGE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_DESCRIPTOR_SET_USAGE)
} XGL_DESCRIPTOR_SET_USAGE;

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
    XGL_STATE_BIND_COLOR_BLEND                              = 0x00000002,
    XGL_STATE_BIND_DEPTH_STENCIL                            = 0x00000003,

    XGL_STATE_BIND_POINT_BEGIN_RANGE                        = XGL_STATE_BIND_VIEWPORT,
    XGL_STATE_BIND_POINT_END_RANGE                          = XGL_STATE_BIND_DEPTH_STENCIL,
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
    XGL_TOPOLOGY_TRIANGLE_FAN                               = 0x00000005,
    XGL_TOPOLOGY_LINE_LIST_ADJ                              = 0x00000006,
    XGL_TOPOLOGY_LINE_STRIP_ADJ                             = 0x00000007,
    XGL_TOPOLOGY_TRIANGLE_LIST_ADJ                          = 0x00000008,
    XGL_TOPOLOGY_TRIANGLE_STRIP_ADJ                         = 0x00000009,
    XGL_TOPOLOGY_PATCH                                      = 0x0000000a,

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

    XGL_TEX_FILTER_BEGIN_RANGE                              = XGL_TEX_FILTER_NEAREST,
    XGL_TEX_FILTER_END_RANGE                                = XGL_TEX_FILTER_LINEAR,
    XGL_NUM_TEX_FILTER                                      = (XGL_TEX_FILTER_END_RANGE - XGL_TEX_FILTER_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_TEX_FILTER)
} XGL_TEX_FILTER;

typedef enum _XGL_TEX_MIPMAP_MODE
{
    XGL_TEX_MIPMAP_BASE                                     = 0,        // Always choose base level
    XGL_TEX_MIPMAP_NEAREST                                  = 1,        // Choose nearest mip level
    XGL_TEX_MIPMAP_LINEAR                                   = 2,        // Linear filter between mip levels

    XGL_TEX_MIPMAP_BEGIN_RANGE                              = XGL_TEX_MIPMAP_BASE,
    XGL_TEX_MIPMAP_END_RANGE                                = XGL_TEX_MIPMAP_LINEAR,
    XGL_NUM_TEX_MIPMAP                                      = (XGL_TEX_MIPMAP_END_RANGE - XGL_TEX_MIPMAP_BEGIN_RANGE + 1),
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
    XGL_FILL_POINTS                                         = 0x00000000,
    XGL_FILL_WIREFRAME                                      = 0x00000001,
    XGL_FILL_SOLID                                          = 0x00000002,

    XGL_FILL_MODE_BEGIN_RANGE                               = XGL_FILL_POINTS,
    XGL_FILL_MODE_END_RANGE                                 = XGL_FILL_SOLID,
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

typedef enum _XGL_PROVOKING_VERTEX_CONVENTION
{
    XGL_PROVOKING_VERTEX_FIRST                              = 0x00000000,
    XGL_PROVOKING_VERTEX_LAST                               = 0x00000001,

    XGL_PROVOKING_VERTEX_BEGIN_RANGE                        = XGL_PROVOKING_VERTEX_FIRST,
    XGL_PROVOKING_VERTEX_END_RANGE                          = XGL_PROVOKING_VERTEX_LAST,
    XGL_NUM_PROVOKING_VERTEX_CONVENTION                     = (XGL_PROVOKING_VERTEX_END_RANGE - XGL_PROVOKING_VERTEX_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_PROVOKING_VERTEX_CONVENTION)
} XGL_PROVOKING_VERTEX_CONVENTION;

typedef enum _XGL_COORDINATE_ORIGIN
{
    XGL_COORDINATE_ORIGIN_UPPER_LEFT                        = 0x00000000,
    XGL_COORDINATE_ORIGIN_LOWER_LEFT                        = 0x00000001,

    XGL_COORDINATE_ORIGIN_BEGIN_RANGE                       = XGL_COORDINATE_ORIGIN_UPPER_LEFT,
    XGL_COORDINATE_ORIGIN_END_RANGE                         = XGL_COORDINATE_ORIGIN_LOWER_LEFT,
    XGL_NUM_COORDINATE_ORIGIN                               = (XGL_COORDINATE_ORIGIN_END_RANGE - XGL_COORDINATE_ORIGIN_END_RANGE + 1),
    XGL_MAX_ENUM(_XGL_COORDINATE_ORIGIN)
} XGL_COORDINATE_ORIGIN;

typedef enum _XGL_DEPTH_MODE
{
    XGL_DEPTH_MODE_ZERO_TO_ONE                              = 0x00000000,
    XGL_DEPTH_MODE_NEGATIVE_ONE_TO_ONE                      = 0x00000001,

    XGL_DEPTH_MODE_BEGIN_RANGE                              = XGL_DEPTH_MODE_ZERO_TO_ONE,
    XGL_DEPTH_MODE_END_RANGE                                = XGL_DEPTH_MODE_NEGATIVE_ONE_TO_ONE,
    XGL_NUM_DEPTH_MODE                                      = (XGL_DEPTH_MODE_END_RANGE - XGL_DEPTH_MODE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_DEPTH_MODE)
} XGL_DEPTH_MODE;

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

    XGL_INFO_TYPE_PHYSICAL_GPU_BEGIN_RANGE                  = XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
    XGL_INFO_TYPE_PHYSICAL_GPU_END_RANGE                    = XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES,
    XGL_NUM_INFO_TYPE_PHYSICAL_GPU                          = (XGL_INFO_TYPE_PHYSICAL_GPU_END_RANGE - XGL_INFO_TYPE_PHYSICAL_GPU_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_PHYSICAL_GPU_INFO_TYPE)
} XGL_PHYSICAL_GPU_INFO_TYPE;

typedef enum _XGL_FORMAT_INFO_TYPE
{
    // Info type for xglGetFormatInfo()
    XGL_INFO_TYPE_FORMAT_PROPERTIES                         = 0x00000000,

    XGL_INFO_TYPE_FORMAT_BEGIN_RANGE                        = XGL_INFO_TYPE_FORMAT_PROPERTIES,
    XGL_INFO_TYPE_FORMAT_END_RANGE                          = XGL_INFO_TYPE_FORMAT_PROPERTIES,
    XGL_NUM_INFO_TYPE_FORMAT                                 = (XGL_INFO_TYPE_FORMAT_END_RANGE - XGL_INFO_TYPE_FORMAT_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_FORMAT_INFO_TYPE)
} XGL_FORMAT_INFO_TYPE;

typedef enum _XGL_SUBRESOURCE_INFO_TYPE
{
    // Info type for xglGetImageSubresourceInfo()
    XGL_INFO_TYPE_SUBRESOURCE_LAYOUT                        = 0x00000000,

    XGL_INFO_TYPE_SUBRESOURCE_BEGIN_RANGE                   = XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
    XGL_INFO_TYPE_SUBRESOURCE_END_RANGE                     = XGL_INFO_TYPE_SUBRESOURCE_LAYOUT,
    XGL_NUM_INFO_TYPE_SUBRESOURCE                           = (XGL_INFO_TYPE_SUBRESOURCE_END_RANGE - XGL_INFO_TYPE_SUBRESOURCE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_SUBRESOURCE_INFO_TYPE)
} XGL_SUBRESOURCE_INFO_TYPE;

typedef enum _XGL_OBJECT_INFO_TYPE
{
    // Info type for xglGetObjectInfo()
    XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT                   = 0x00000000,
    XGL_INFO_TYPE_MEMORY_REQUIREMENTS                       = 0x00000001,
    XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS                = 0x00000002,
    XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS                 = 0x00000003,

    XGL_INFO_TYPE_BEGIN_RANGE                               = XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
    XGL_INFO_TYPE_END_RANGE                                 = XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
    XGL_NUM_INFO_TYPE                                       = (XGL_INFO_TYPE_END_RANGE - XGL_INFO_TYPE_BEGIN_RANGE + 1),
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

typedef enum _XGL_RESULT
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
    XGL_MAX_ENUM(_XGL_RESULT_CODE)
} XGL_RESULT;

// ------------------------------------------------------------------------------------------------
// XGL format definitions

typedef enum _XGL_VERTEX_INPUT_STEP_RATE
{
    XGL_VERTEX_INPUT_STEP_RATE_VERTEX                       = 0x0,
    XGL_VERTEX_INPUT_STEP_RATE_INSTANCE                     = 0x1,
    XGL_VERTEX_INPUT_STEP_RATE_DRAW                         = 0x2,  //Optional

    XGL_VERTEX_INPUT_STEP_RATE_BEGIN_RANGE                  = XGL_VERTEX_INPUT_STEP_RATE_VERTEX,
    XGL_VERTEX_INPUT_STEP_RATE_END_RANGE                    = XGL_VERTEX_INPUT_STEP_RATE_DRAW,
    XGL_NUM_VERTEX_INPUT_STEP_RATE                          = (XGL_VERTEX_INPUT_STEP_RATE_END_RANGE - XGL_VERTEX_INPUT_STEP_RATE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_VERTEX_INPUT_STEP_RATE)
} XGL_VERTEX_INPUT_STEP_RATE;

typedef enum _XGL_FORMAT
{
    XGL_FMT_UNDEFINED                                       = 0x00000000,
    XGL_FMT_R4G4_UNORM                                      = 0x00000001,
    XGL_FMT_R4G4_USCALED                                    = 0x00000002,
    XGL_FMT_R4G4B4A4_UNORM                                  = 0x00000003,
    XGL_FMT_R4G4B4A4_USCALED                                = 0x00000004,
    XGL_FMT_R5G6B5_UNORM                                    = 0x00000005,
    XGL_FMT_R5G6B5_USCALED                                  = 0x00000006,
    XGL_FMT_R5G5B5A1_UNORM                                  = 0x00000007,
    XGL_FMT_R5G5B5A1_USCALED                                = 0x00000008,
    XGL_FMT_R8_UNORM                                        = 0x00000009,
    XGL_FMT_R8_SNORM                                        = 0x0000000A,
    XGL_FMT_R8_USCALED                                      = 0x0000000B,
    XGL_FMT_R8_SSCALED                                      = 0x0000000C,
    XGL_FMT_R8_UINT                                         = 0x0000000D,
    XGL_FMT_R8_SINT                                         = 0x0000000E,
    XGL_FMT_R8_SRGB                                         = 0x0000000F,
    XGL_FMT_R8G8_UNORM                                      = 0x00000010,
    XGL_FMT_R8G8_SNORM                                      = 0x00000011,
    XGL_FMT_R8G8_USCALED                                    = 0x00000012,
    XGL_FMT_R8G8_SSCALED                                    = 0x00000013,
    XGL_FMT_R8G8_UINT                                       = 0x00000014,
    XGL_FMT_R8G8_SINT                                       = 0x00000015,
    XGL_FMT_R8G8_SRGB                                       = 0x00000016,
    XGL_FMT_R8G8B8_UNORM                                    = 0x00000017,
    XGL_FMT_R8G8B8_SNORM                                    = 0x00000018,
    XGL_FMT_R8G8B8_USCALED                                  = 0x00000019,
    XGL_FMT_R8G8B8_SSCALED                                  = 0x0000001A,
    XGL_FMT_R8G8B8_UINT                                     = 0x0000001B,
    XGL_FMT_R8G8B8_SINT                                     = 0x0000001C,
    XGL_FMT_R8G8B8_SRGB                                     = 0x0000001D,
    XGL_FMT_R8G8B8A8_UNORM                                  = 0x0000001E,
    XGL_FMT_R8G8B8A8_SNORM                                  = 0x0000001F,
    XGL_FMT_R8G8B8A8_USCALED                                = 0x00000020,
    XGL_FMT_R8G8B8A8_SSCALED                                = 0x00000021,
    XGL_FMT_R8G8B8A8_UINT                                   = 0x00000022,
    XGL_FMT_R8G8B8A8_SINT                                   = 0x00000023,
    XGL_FMT_R8G8B8A8_SRGB                                   = 0x00000024,
    XGL_FMT_R10G10B10A2_UNORM                               = 0x00000025,
    XGL_FMT_R10G10B10A2_SNORM                               = 0x00000026,
    XGL_FMT_R10G10B10A2_USCALED                             = 0x00000027,
    XGL_FMT_R10G10B10A2_SSCALED                             = 0x00000028,
    XGL_FMT_R10G10B10A2_UINT                                = 0x00000029,
    XGL_FMT_R10G10B10A2_SINT                                = 0x0000002A,
    XGL_FMT_R16_UNORM                                       = 0x0000002B,
    XGL_FMT_R16_SNORM                                       = 0x0000002C,
    XGL_FMT_R16_USCALED                                     = 0x0000002D,
    XGL_FMT_R16_SSCALED                                     = 0x0000002E,
    XGL_FMT_R16_UINT                                        = 0x0000002F,
    XGL_FMT_R16_SINT                                        = 0x00000030,
    XGL_FMT_R16_SFLOAT                                      = 0x00000031,
    XGL_FMT_R16G16_UNORM                                    = 0x00000032,
    XGL_FMT_R16G16_SNORM                                    = 0x00000033,
    XGL_FMT_R16G16_USCALED                                  = 0x00000034,
    XGL_FMT_R16G16_SSCALED                                  = 0x00000035,
    XGL_FMT_R16G16_UINT                                     = 0x00000036,
    XGL_FMT_R16G16_SINT                                     = 0x00000037,
    XGL_FMT_R16G16_SFLOAT                                   = 0x00000038,
    XGL_FMT_R16G16B16_UNORM                                 = 0x00000039,
    XGL_FMT_R16G16B16_SNORM                                 = 0x0000003A,
    XGL_FMT_R16G16B16_USCALED                               = 0x0000003B,
    XGL_FMT_R16G16B16_SSCALED                               = 0x0000003C,
    XGL_FMT_R16G16B16_UINT                                  = 0x0000003D,
    XGL_FMT_R16G16B16_SINT                                  = 0x0000003E,
    XGL_FMT_R16G16B16_SFLOAT                                = 0x0000003F,
    XGL_FMT_R16G16B16A16_UNORM                              = 0x00000040,
    XGL_FMT_R16G16B16A16_SNORM                              = 0x00000041,
    XGL_FMT_R16G16B16A16_USCALED                            = 0x00000042,
    XGL_FMT_R16G16B16A16_SSCALED                            = 0x00000043,
    XGL_FMT_R16G16B16A16_UINT                               = 0x00000044,
    XGL_FMT_R16G16B16A16_SINT                               = 0x00000045,
    XGL_FMT_R16G16B16A16_SFLOAT                             = 0x00000046,
    XGL_FMT_R32_UINT                                        = 0x00000047,
    XGL_FMT_R32_SINT                                        = 0x00000048,
    XGL_FMT_R32_SFLOAT                                      = 0x00000049,
    XGL_FMT_R32G32_UINT                                     = 0x0000004A,
    XGL_FMT_R32G32_SINT                                     = 0x0000004B,
    XGL_FMT_R32G32_SFLOAT                                   = 0x0000004C,
    XGL_FMT_R32G32B32_UINT                                  = 0x0000004D,
    XGL_FMT_R32G32B32_SINT                                  = 0x0000004E,
    XGL_FMT_R32G32B32_SFLOAT                                = 0x0000004F,
    XGL_FMT_R32G32B32A32_UINT                               = 0x00000050,
    XGL_FMT_R32G32B32A32_SINT                               = 0x00000051,
    XGL_FMT_R32G32B32A32_SFLOAT                             = 0x00000052,
    XGL_FMT_R64_SFLOAT                                      = 0x00000053,
    XGL_FMT_R64G64_SFLOAT                                   = 0x00000054,
    XGL_FMT_R64G64B64_SFLOAT                                = 0x00000055,
    XGL_FMT_R64G64B64A64_SFLOAT                             = 0x00000056,
    XGL_FMT_R11G11B10_UFLOAT                                = 0x00000057,
    XGL_FMT_R9G9B9E5_UFLOAT                                 = 0x00000058,
    XGL_FMT_D16_UNORM                                       = 0x00000059,
    XGL_FMT_D24_UNORM                                       = 0x0000005A,
    XGL_FMT_D32_SFLOAT                                      = 0x0000005B,
    XGL_FMT_S8_UINT                                         = 0x0000005C,
    XGL_FMT_D16_UNORM_S8_UINT                               = 0x0000005D,
    XGL_FMT_D24_UNORM_S8_UINT                               = 0x0000005E,
    XGL_FMT_D32_SFLOAT_S8_UINT                              = 0x0000005F,
    XGL_FMT_BC1_RGB_UNORM                                   = 0x00000060,
    XGL_FMT_BC1_RGB_SRGB                                    = 0x00000061,
    XGL_FMT_BC1_RGBA_UNORM                                  = 0x00000062,
    XGL_FMT_BC1_RGBA_SRGB                                   = 0x00000063,
    XGL_FMT_BC2_UNORM                                       = 0x00000064,
    XGL_FMT_BC2_SRGB                                        = 0x00000065,
    XGL_FMT_BC3_UNORM                                       = 0x00000066,
    XGL_FMT_BC3_SRGB                                        = 0x00000067,
    XGL_FMT_BC4_UNORM                                       = 0x00000068,
    XGL_FMT_BC4_SNORM                                       = 0x00000069,
    XGL_FMT_BC5_UNORM                                       = 0x0000006A,
    XGL_FMT_BC5_SNORM                                       = 0x0000006B,
    XGL_FMT_BC6H_UFLOAT                                     = 0x0000006C,
    XGL_FMT_BC6H_SFLOAT                                     = 0x0000006D,
    XGL_FMT_BC7_UNORM                                       = 0x0000006E,
    XGL_FMT_BC7_SRGB                                        = 0x0000006F,
    XGL_FMT_ETC2_R8G8B8_UNORM                               = 0x00000070,
    XGL_FMT_ETC2_R8G8B8_SRGB                                = 0x00000071,
    XGL_FMT_ETC2_R8G8B8A1_UNORM                             = 0x00000072,
    XGL_FMT_ETC2_R8G8B8A1_SRGB                              = 0x00000073,
    XGL_FMT_ETC2_R8G8B8A8_UNORM                             = 0x00000074,
    XGL_FMT_ETC2_R8G8B8A8_SRGB                              = 0x00000075,
    XGL_FMT_EAC_R11_UNORM                                   = 0x00000076,
    XGL_FMT_EAC_R11_SNORM                                   = 0x00000077,
    XGL_FMT_EAC_R11G11_UNORM                                = 0x00000078,
    XGL_FMT_EAC_R11G11_SNORM                                = 0x00000079,
    XGL_FMT_ASTC_4x4_UNORM                                  = 0x0000007A,
    XGL_FMT_ASTC_4x4_SRGB                                   = 0x0000007B,
    XGL_FMT_ASTC_5x4_UNORM                                  = 0x0000007C,
    XGL_FMT_ASTC_5x4_SRGB                                   = 0x0000007D,
    XGL_FMT_ASTC_5x5_UNORM                                  = 0x0000007E,
    XGL_FMT_ASTC_5x5_SRGB                                   = 0x0000007F,
    XGL_FMT_ASTC_6x5_UNORM                                  = 0x00000080,
    XGL_FMT_ASTC_6x5_SRGB                                   = 0x00000081,
    XGL_FMT_ASTC_6x6_UNORM                                  = 0x00000082,
    XGL_FMT_ASTC_6x6_SRGB                                   = 0x00000083,
    XGL_FMT_ASTC_8x5_UNORM                                  = 0x00000084,
    XGL_FMT_ASTC_8x5_SRGB                                   = 0x00000085,
    XGL_FMT_ASTC_8x6_UNORM                                  = 0x00000086,
    XGL_FMT_ASTC_8x6_SRGB                                   = 0x00000087,
    XGL_FMT_ASTC_8x8_UNORM                                  = 0x00000088,
    XGL_FMT_ASTC_8x8_SRGB                                   = 0x00000089,
    XGL_FMT_ASTC_10x5_UNORM                                 = 0x0000008A,
    XGL_FMT_ASTC_10x5_SRGB                                  = 0x0000008B,
    XGL_FMT_ASTC_10x6_UNORM                                 = 0x0000008C,
    XGL_FMT_ASTC_10x6_SRGB                                  = 0x0000008D,
    XGL_FMT_ASTC_10x8_UNORM                                 = 0x0000008E,
    XGL_FMT_ASTC_10x8_SRGB                                  = 0x0000008F,
    XGL_FMT_ASTC_10x10_UNORM                                = 0x00000090,
    XGL_FMT_ASTC_10x10_SRGB                                 = 0x00000091,
    XGL_FMT_ASTC_12x10_UNORM                                = 0x00000092,
    XGL_FMT_ASTC_12x10_SRGB                                 = 0x00000093,
    XGL_FMT_ASTC_12x12_UNORM                                = 0x00000094,
    XGL_FMT_ASTC_12x12_SRGB                                 = 0x00000095,
    XGL_FMT_B4G4R4A4_UNORM                                  = 0x00000096,
    XGL_FMT_B5G5R5A1_UNORM                                  = 0x00000097,
    XGL_FMT_B5G6R5_UNORM                                    = 0x00000098,
    XGL_FMT_B5G6R5_USCALED                                  = 0x00000099,
    XGL_FMT_B8G8R8_UNORM                                    = 0x0000009A,
    XGL_FMT_B8G8R8_SNORM                                    = 0x0000009B,
    XGL_FMT_B8G8R8_USCALED                                  = 0x0000009C,
    XGL_FMT_B8G8R8_SSCALED                                  = 0x0000009D,
    XGL_FMT_B8G8R8_UINT                                     = 0x0000009E,
    XGL_FMT_B8G8R8_SINT                                     = 0x0000009F,
    XGL_FMT_B8G8R8_SRGB                                     = 0x000000A0,
    XGL_FMT_B8G8R8A8_UNORM                                  = 0x000000A1,
    XGL_FMT_B8G8R8A8_SNORM                                  = 0x000000A2,
    XGL_FMT_B8G8R8A8_USCALED                                = 0x000000A3,
    XGL_FMT_B8G8R8A8_SSCALED                                = 0x000000A4,
    XGL_FMT_B8G8R8A8_UINT                                   = 0x000000A5,
    XGL_FMT_B8G8R8A8_SINT                                   = 0x000000A6,
    XGL_FMT_B8G8R8A8_SRGB                                   = 0x000000A7,
    XGL_FMT_B10G10R10A2_UNORM                               = 0x000000A8,
    XGL_FMT_B10G10R10A2_SNORM                               = 0x000000A9,
    XGL_FMT_B10G10R10A2_USCALED                             = 0x000000AA,
    XGL_FMT_B10G10R10A2_SSCALED                             = 0x000000AB,
    XGL_FMT_B10G10R10A2_UINT                                = 0x000000AC,
    XGL_FMT_B10G10R10A2_SINT                                = 0x000000AD,

    XGL_FMT_BEGIN_RANGE                                     = XGL_FMT_UNDEFINED,
    XGL_FMT_END_RANGE                                       = XGL_FMT_B10G10R10A2_SINT,
    XGL_NUM_FMT                                             = (XGL_FMT_END_RANGE - XGL_FMT_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_FORMAT)
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

    XGL_SHADER_STAGE_BEGIN_RANGE                            = XGL_SHADER_STAGE_VERTEX,
    XGL_SHADER_STAGE_END_RANGE                              = XGL_SHADER_STAGE_COMPUTE,
    XGL_NUM_SHADER_STAGE                                    = (XGL_SHADER_STAGE_END_RANGE - XGL_SHADER_STAGE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_PIPELINE_SHADER_STAGE)
} XGL_PIPELINE_SHADER_STAGE;

typedef enum _XGL_SHADER_STAGE_FLAGS
{
    XGL_SHADER_STAGE_FLAGS_VERTEX_BIT                        = 0x00000001,
    XGL_SHADER_STAGE_FLAGS_TESS_CONTROL_BIT                  = 0x00000002,
    XGL_SHADER_STAGE_FLAGS_TESS_EVALUATION_BIT               = 0x00000004,
    XGL_SHADER_STAGE_FLAGS_GEOMETRY_BIT                      = 0x00000008,
    XGL_SHADER_STAGE_FLAGS_FRAGMENT_BIT                      = 0x00000010,
    XGL_SHADER_STAGE_FLAGS_COMPUTE_BIT                       = 0x00000020,

    XGL_SHADER_STAGE_FLAGS_ALL                               = 0x7FFFFFFF,
    XGL_MAX_ENUM(_XGL_SHADER_STAGE_FLAGS)
} XGL_SHADER_STAGE_FLAGS;

// Structure type enumerant
typedef enum _XGL_STRUCTURE_TYPE
{
    XGL_STRUCTURE_TYPE_APPLICATION_INFO                     = 0,
    XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO                   = 1,
    XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO                    = 2,
    XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO                     = 4,
    XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO                = 5,
    XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO              = 6,
    XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO               = 7,
    XGL_STRUCTURE_TYPE_EVENT_WAIT_INFO                      = 8,
    XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO               = 9,
    XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO    = 10,
    XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO       = 11,
    XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO                   = 12,
    XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO         = 13,
    XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO                  = 14,
    XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO    = 15,
    XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO         = 16,
    XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO         = 17,
    XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO         = 18,
    XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO         = 19,
    XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO               = 20,
    XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO                    = 21,
    XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO                    = 22,
    XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_CREATE_INFO          = 23,
    XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_OPEN_INFO            = 24,
    XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO               = 25,
    XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO    = 26,
    XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO        = 27,
    XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO    = 28,
    XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO        = 29,
    XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO      = 30,
    XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO        = 31,
    XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO        = 32,
    XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO        = 33,
    XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO        = 34,
    XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO        = 35,
    XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO                    = 36,
    XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO                   = 37,
    XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO              = 38,
    XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO              = 39,
    XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO                = 40,
    XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO       = 41,
    XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO              = 42,
    XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO                    = 43,
    XGL_STRUCTURE_TYPE_PIPELINE_BARRIER                     = 44,
    XGL_STRUCTURE_TYPE_MEMORY_BARRIER                       = 45,
    XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER                = 46,
    XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER                 = 47,
    XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO        = 48,
    XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS                      = 49,
    XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES              = 50,
    XGL_STRUCTURE_TYPE_UPDATE_IMAGES                        = 51,
    XGL_STRUCTURE_TYPE_UPDATE_BUFFERS                       = 52,
    XGL_STRUCTURE_TYPE_UPDATE_AS_COPY                       = 53,
    XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO             = 54,
    XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO              = 55,

    XGL_STRUCTURE_TYPE_BEGIN_RANGE                          = XGL_STRUCTURE_TYPE_APPLICATION_INFO,
    XGL_STRUCTURE_TYPE_END_RANGE                            = XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO,
    XGL_NUM_STRUCTURE_TYPE                                  = (XGL_STRUCTURE_TYPE_END_RANGE - XGL_STRUCTURE_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_STRUCTURE_TYPE)
} XGL_STRUCTURE_TYPE;

// ------------------------------------------------------------------------------------------------
// Flags

// Device creation flags
typedef enum _XGL_DEVICE_CREATE_FLAGS
{
    XGL_DEVICE_CREATE_VALIDATION_BIT                        = 0x00000001,
    XGL_DEVICE_CREATE_MGPU_IQ_MATCH_BIT                     = 0x00000002,
    XGL_MAX_ENUM(_XGL_DEVICE_CREATE_FLAGS)
} XGL_DEVICE_CREATE_FLAGS;

// Queue capabilities
typedef enum _XGL_QUEUE_FLAGS
{
    XGL_QUEUE_GRAPHICS_BIT                                  = 0x00000001,   // Queue supports graphics operations
    XGL_QUEUE_COMPUTE_BIT                                   = 0x00000002,   // Queue supports compute operations
    XGL_QUEUE_DMA_BIT                                       = 0x00000004,   // Queue supports DMA operations
    XGL_QUEUE_EXTENDED_BIT                                  = 0x40000000,   // Extended queue
    XGL_MAX_ENUM(_XGL_QUEUE_FLAGS)
} XGL_QUEUE_FLAGS;

// memory properties passed into xglAllocMemory().
typedef enum _XGL_MEMORY_PROPERTY_FLAGS
{
    XGL_MEMORY_PROPERTY_GPU_ONLY                            = 0x00000000,   // If not set, then allocate memory on device (GPU)
    XGL_MEMORY_PROPERTY_CPU_VISIBLE_BIT                     = 0x00000001,
    XGL_MEMORY_PROPERTY_CPU_GPU_COHERENT_BIT                = 0x00000002,
    XGL_MEMORY_PROPERTY_CPU_UNCACHED_BIT                    = 0x00000004,
    XGL_MEMORY_PROPERTY_CPU_WRITE_COMBINED_BIT              = 0x00000008,
    XGL_MEMORY_PROPERTY_PREFER_CPU_LOCAL                    = 0x00000010,   // all else being equal, prefer CPU access
    XGL_MEMORY_PROPERTY_SHAREABLE_BIT                       = 0x00000020,
    XGL_MAX_ENUM(_XGL_MEMORY_PROPERTY_FLAGS)
} XGL_MEMORY_PROPERTY_FLAGS;

typedef enum _XGL_MEMORY_TYPE
{
    XGL_MEMORY_TYPE_OTHER                                   = 0x00000000,   // device memory that is not any of the others
    XGL_MEMORY_TYPE_BUFFER                                  = 0x00000001,   // memory for buffers and associated information
    XGL_MEMORY_TYPE_IMAGE                                   = 0x00000002,   // memory for images and associated information

    XGL_MEMORY_TYPE_BEGIN_RANGE                             = XGL_MEMORY_TYPE_OTHER,
    XGL_MEMORY_TYPE_END_RANGE                               = XGL_MEMORY_TYPE_IMAGE,
    XGL_NUM_MEMORY_TYPE                                     = (XGL_MEMORY_TYPE_END_RANGE - XGL_MEMORY_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_MEMORY_TYPE)
} XGL_MEMORY_TYPE;

// Buffer and buffer allocation usage flags
typedef enum _XGL_BUFFER_USAGE_FLAGS
{
    XGL_BUFFER_USAGE_GENERAL                                = 0x00000000,   // no special usage
    XGL_BUFFER_USAGE_SHADER_ACCESS_READ_BIT                 = 0x00000001,   // Shader read (e.g. TBO, image buffer, UBO, SSBO)
    XGL_BUFFER_USAGE_SHADER_ACCESS_WRITE_BIT                = 0x00000002,   // Shader write (e.g. image buffer, SSBO)
    XGL_BUFFER_USAGE_SHADER_ACCESS_ATOMIC_BIT               = 0x00000004,   // Shader atomic operations (e.g. image buffer, SSBO)
    XGL_BUFFER_USAGE_TRANSFER_SOURCE_BIT                    = 0x00000008,   // used as a source for copies
    XGL_BUFFER_USAGE_TRANSFER_DESTINATION_BIT               = 0x00000010,   // used as a destination for copies
    XGL_BUFFER_USAGE_UNIFORM_READ_BIT                       = 0x00000020,   // Uniform read (UBO)
    XGL_BUFFER_USAGE_INDEX_FETCH_BIT                        = 0x00000040,   // Fixed function index fetch (index buffer)
    XGL_BUFFER_USAGE_VERTEX_FETCH_BIT                       = 0x00000080,   // Fixed function vertex fetch (VBO)
    XGL_BUFFER_USAGE_SHADER_STORAGE_BIT                     = 0x00000100,   // Shader storage buffer (SSBO)
    XGL_BUFFER_USAGE_RAW_BUFFER_BIT                         = 0x00000200,   // used as a raw buffer
    XGL_BUFFER_USAGE_INDIRECT_PARAMETER_FETCH_BIT           = 0x00000400,   // Can be the source of indirect parameters (e.g. indirect buffer, parameter buffer)
    XGL_BUFFER_USAGE_TEXTURE_BUFFER_BIT                     = 0x00000800,   // texture buffer (TBO)
    XGL_BUFFER_USAGE_IMAGE_BUFFER_BIT                       = 0x00001000,   // image buffer (load/store)
    XGL_MAX_ENUM(_XGL_BUFFER_USAGE_FLAGS)
} XGL_BUFFER_USAGE_FLAGS;

// Buffer flags
typedef enum _XGL_BUFFER_CREATE_FLAGS
{
    XGL_BUFFER_CREATE_SHAREABLE_BIT                         = 0x00000001,
    XGL_BUFFER_CREATE_SPARSE_BIT                            = 0x00000002,
    XGL_MAX_ENUM(_XGL_BUFFER_CREATE_FLAGS)
} XGL_BUFFER_CREATE_FLAGS;

typedef enum _XGL_BUFFER_VIEW_TYPE
{
    XGL_BUFFER_VIEW_RAW                                     = 0x00000000,   // Raw buffer without special structure (e.g. UBO, SSBO, indirect and parameter buffers)
    XGL_BUFFER_VIEW_TYPED                                   = 0x00000001,   // Typed buffer, format and channels are used (TBO, image buffer)
    XGL_BUFFER_VIEW_STRUCTURED                              = 0x00000002,   // Structured buffer, stride is used (VBO, DX-style structured buffer)

    XGL_BUFFER_VIEW_TYPE_BEGIN_RANGE                        = XGL_BUFFER_VIEW_RAW,
    XGL_BUFFER_VIEW_TYPE_END_RANGE                          = XGL_BUFFER_VIEW_STRUCTURED,
    XGL_NUM_BUFFER_VIEW_TYPE                                = (XGL_BUFFER_VIEW_TYPE_END_RANGE - XGL_BUFFER_VIEW_TYPE_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_BUFFER_VIEW_TYPE)
} XGL_BUFFER_VIEW_TYPE;


// Images memory allocations can be used for resources of a given format class.
typedef enum _XGL_IMAGE_FORMAT_CLASS
{
    XGL_IMAGE_FORMAT_CLASS_128_BITS                         = 1,  // color formats
    XGL_IMAGE_FORMAT_CLASS_96_BITS                          = 2,
    XGL_IMAGE_FORMAT_CLASS_64_BITS                          = 3,
    XGL_IMAGE_FORMAT_CLASS_48_BITS                          = 4,
    XGL_IMAGE_FORMAT_CLASS_32_BITS                          = 5,
    XGL_IMAGE_FORMAT_CLASS_24_BITS                          = 6,
    XGL_IMAGE_FORMAT_CLASS_16_BITS                          = 7,
    XGL_IMAGE_FORMAT_CLASS_8_BITS                           = 8,
    XGL_IMAGE_FORMAT_CLASS_128_BIT_BLOCK                    = 9,  // 128-bit block compressed formats
    XGL_IMAGE_FORMAT_CLASS_64_BIT_BLOCK                     = 10, // 64-bit block compressed formats
    XGL_IMAGE_FORMAT_CLASS_D32                              = 11, // D32_SFLOAT
    XGL_IMAGE_FORMAT_CLASS_D24                              = 12, // D24_UNORM
    XGL_IMAGE_FORMAT_CLASS_D16                              = 13, // D16_UNORM
    XGL_IMAGE_FORMAT_CLASS_S8                               = 14, // S8_UINT
    XGL_IMAGE_FORMAT_CLASS_D32S8                            = 15, // D32_SFLOAT_S8_UINT
    XGL_IMAGE_FORMAT_CLASS_D24S8                            = 16, // D24_UNORM_S8_UINT
    XGL_IMAGE_FORMAT_CLASS_D16S8                            = 17, // D16_UNORM_S8_UINT
    XGL_IMAGE_FORMAT_CLASS_LINEAR                           = 18, // used for pitch-linear (transparent) textures

    XGL_IMAGE_FORMAT_CLASS_BEGIN_RANGE                      = XGL_IMAGE_FORMAT_CLASS_128_BITS,
    XGL_IMAGE_FORMAT_CLASS_END_RANGE                        = XGL_IMAGE_FORMAT_CLASS_LINEAR,
    XGL_NUM_IMAGE_FORMAT_CLASS                              = (XGL_IMAGE_FORMAT_CLASS_END_RANGE - XGL_IMAGE_FORMAT_CLASS_BEGIN_RANGE + 1),
    XGL_MAX_ENUM(_XGL_IMAGE_FORMAT_CLASS)
} XGL_IMAGE_FORMAT_CLASS;

// Image and image allocation usage flags
typedef enum _XGL_IMAGE_USAGE_FLAGS
{
    XGL_IMAGE_USAGE_GENERAL                                 = 0x00000000,   // no special usage
    XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT                  = 0x00000001,   // shader read (e.g. texture, image)
    XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT                 = 0x00000002,   // shader write (e.g. image)
    XGL_IMAGE_USAGE_SHADER_ACCESS_ATOMIC_BIT                = 0x00000004,   // shader atomic operations (e.g. image)
    XGL_IMAGE_USAGE_TRANSFER_SOURCE_BIT                     = 0x00000008,   // used as a source for copies 
    XGL_IMAGE_USAGE_TRANSFER_DESTINATION_BIT                = 0x00000010,   // used as a destination for copies
    XGL_IMAGE_USAGE_TEXTURE_BIT                             = 0x00000020,   // opaque texture (2d, 3d, etc.)
    XGL_IMAGE_USAGE_IMAGE_BIT                               = 0x00000040,   // opaque image (2d, 3d, etc.)
    XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT                    = 0x00000080,   // framebuffer color attachment
    XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT                       = 0x00000100,   // framebuffer depth/stencil 
    XGL_MAX_ENUM(_XGL_IMAGE_USAGE_FLAGS)
} XGL_IMAGE_USAGE_FLAGS;

// Image flags
typedef enum _XGL_IMAGE_CREATE_FLAGS
{
    XGL_IMAGE_CREATE_INVARIANT_DATA_BIT                     = 0x00000001,
    XGL_IMAGE_CREATE_CLONEABLE_BIT                          = 0x00000002,
    XGL_IMAGE_CREATE_SHAREABLE_BIT                          = 0x00000004,
    XGL_IMAGE_CREATE_SPARSE_BIT                             = 0x00000008,
    XGL_IMAGE_CREATE_MUTABLE_FORMAT_BIT                     = 0x00000010,   // Allows image views to have different format than the base image
    XGL_MAX_ENUM(_XGL_IMAGE_CREATE_FLAGS)
} XGL_IMAGE_CREATE_FLAGS;

// Depth-stencil view creation flags
typedef enum _XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS
{
    XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT       = 0x00000001,
    XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT     = 0x00000002,
    XGL_MAX_ENUM(_XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS)
} XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS;

// Pipeline creation flags
typedef enum _XGL_PIPELINE_CREATE_FLAGS
{
    XGL_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT            = 0x00000001,
    XGL_MAX_ENUM(_XGL_PIPELINE_CREATE_FLAGS)
} XGL_PIPELINE_CREATE_FLAGS;

// Semaphore creation flags
typedef enum _XGL_SEMAPHORE_CREATE_FLAGS
{
    XGL_SEMAPHORE_CREATE_SHAREABLE_BIT                      = 0x00000001,
    XGL_MAX_ENUM(_XGL_SEMAPHORE_CREATE_FLAGS)
} XGL_SEMAPHORE_CREATE_FLAGS;

// Memory reference flags
typedef enum _XGL_MEMORY_REF_FLAGS
{
    XGL_MEMORY_REF_READ_ONLY_BIT                            = 0x00000001,
    XGL_MAX_ENUM(_XGL_MEMORY_REF_FLAGS)
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
    XGL_MAX_ENUM(_XGL_FORMAT_FEATURE_FLAGS)
} XGL_FORMAT_FEATURE_FLAGS;

// Query flags
typedef enum _XGL_QUERY_CONTROL_FLAGS
{
    XGL_QUERY_IMPRECISE_DATA_BIT                            = 0x00000001,
    XGL_MAX_ENUM(_XGL_QUERY_CONTROL_FLAGS)
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
    XGL_MAX_ENUM(_XGL_GPU_COMPATIBILITY_FLAGS)
} XGL_GPU_COMPATIBILITY_FLAGS;

// Command buffer building flags
typedef enum _XGL_CMD_BUFFER_BUILD_FLAGS
{
    XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT             = 0x00000001,
    XGL_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH_BIT             = 0x00000002,
    XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT             = 0x00000004,
    XGL_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT       = 0x00000008,
    XGL_MAX_ENUM(_XGL_CMD_BUFFER_BUILD_FLAGS)
} XGL_CMD_BUFFER_BUILD_FLAGS;

// ------------------------------------------------------------------------------------------------
// XGL structures

typedef struct _XGL_OFFSET2D
{
    int32_t                                 x;
    int32_t                                 y;
} XGL_OFFSET2D;

typedef struct _XGL_OFFSET3D
{
    int32_t                                 x;
    int32_t                                 y;
    int32_t                                 z;
} XGL_OFFSET3D;

typedef struct _XGL_EXTENT2D
{
    int32_t                                 width;
    int32_t                                 height;
} XGL_EXTENT2D;

typedef struct _XGL_EXTENT3D
{
    int32_t                                 width;
    int32_t                                 height;
    int32_t                                 depth;
} XGL_EXTENT3D;

typedef struct _XGL_VIEWPORT
{
    float                                   originX;
    float                                   originY;
    float                                   width;
    float                                   height;
    float                                   minDepth;
    float                                   maxDepth;
} XGL_VIEWPORT;

typedef struct _XGL_RECT
{
    XGL_OFFSET2D                            offset;
    XGL_EXTENT2D                            extent;
} XGL_RECT;

typedef struct _XGL_CHANNEL_MAPPING
{
    XGL_CHANNEL_SWIZZLE                     r;
    XGL_CHANNEL_SWIZZLE                     g;
    XGL_CHANNEL_SWIZZLE                     b;
    XGL_CHANNEL_SWIZZLE                     a;
} XGL_CHANNEL_MAPPING;

typedef struct _XGL_PHYSICAL_GPU_PROPERTIES
{
    uint32_t                                apiVersion;
    uint32_t                                driverVersion;
    uint32_t                                vendorId;
    uint32_t                                deviceId;
    XGL_PHYSICAL_GPU_TYPE                   gpuType;
    char                                    gpuName[XGL_MAX_PHYSICAL_GPU_NAME];
    uint32_t                                maxMemRefsPerSubmission;
    XGL_GPU_SIZE                            maxInlineMemoryUpdateSize;
    uint32_t                                maxBoundDescriptorSets;
    uint32_t                                maxThreadGroupSize;
    uint64_t                                timestampFrequency;
    bool32_t                                multiColorAttachmentClears;
    uint32_t                                maxDescriptorSets;              // at least 2?
    uint32_t                                maxViewports;                   // at least 16?
    uint32_t                                maxColorAttachments;            // at least 8?
} XGL_PHYSICAL_GPU_PROPERTIES;

typedef struct _XGL_PHYSICAL_GPU_PERFORMANCE
{
    float                                   maxGpuClock;
    float                                   aluPerClock;
    float                                   texPerClock;
    float                                   primsPerClock;
    float                                   pixelsPerClock;
} XGL_PHYSICAL_GPU_PERFORMANCE;

typedef struct _XGL_GPU_COMPATIBILITY_INFO
{
    XGL_FLAGS                               compatibilityFlags; // XGL_GPU_COMPATIBILITY_FLAGS
} XGL_GPU_COMPATIBILITY_INFO;

typedef struct _XGL_APPLICATION_INFO
{
    XGL_STRUCTURE_TYPE                      sType;              // Type of structure. Should be XGL_STRUCTURE_TYPE_APPLICATION_INFO
    const void*                             pNext;              // Next structure in chain
    const char*                             pAppName;
    uint32_t                                appVersion;
    const char*                             pEngineName;
    uint32_t                                engineVersion;
    uint32_t                                apiVersion;
} XGL_APPLICATION_INFO;

typedef void* (XGLAPI *XGL_ALLOC_FUNCTION)(
    void*                                   pUserData,
    size_t                                  size,
    size_t                                  alignment,
    XGL_SYSTEM_ALLOC_TYPE                   allocType);

typedef void (XGLAPI *XGL_FREE_FUNCTION)(
    void*                                   pUserData,
    void*                                   pMem);

typedef struct _XGL_ALLOC_CALLBACKS
{
    void*                                   pUserData;
    XGL_ALLOC_FUNCTION                      pfnAlloc;
    XGL_FREE_FUNCTION                       pfnFree;
} XGL_ALLOC_CALLBACKS;

typedef struct _XGL_DEVICE_QUEUE_CREATE_INFO
{
    uint32_t                                queueNodeIndex;
    uint32_t                                queueCount;
} XGL_DEVICE_QUEUE_CREATE_INFO;

typedef struct _XGL_DEVICE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Should be XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO
    const void*                             pNext;                      // Pointer to next structure
    uint32_t                                queueRecordCount;
    const XGL_DEVICE_QUEUE_CREATE_INFO*     pRequestedQueues;
    uint32_t                                extensionCount;
    const char*const*                       ppEnabledExtensionNames;
    XGL_VALIDATION_LEVEL                    maxValidationLevel;
    XGL_FLAGS                               flags;                      // XGL_DEVICE_CREATE_FLAGS
} XGL_DEVICE_CREATE_INFO;

typedef struct _XGL_LAYER_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Should be XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO
    const void*                             pNext;                      // Pointer to next structure
    uint32_t                                layerCount;
    const char *const*                      ppActiveLayerNames;         // layer name from the layer's xglEnumerateLayers())
} XGL_LAYER_CREATE_INFO;

typedef struct _XGL_PHYSICAL_GPU_QUEUE_PROPERTIES
{
    XGL_FLAGS                               queueFlags;                 // XGL_QUEUE_FLAGS
    uint32_t                                queueCount;
    uint32_t                                maxAtomicCounters;
    bool32_t                                supportsTimestamps;
} XGL_PHYSICAL_GPU_QUEUE_PROPERTIES;

typedef struct _XGL_PHYSICAL_GPU_MEMORY_PROPERTIES
{
    bool32_t                                supportsMigration;
    bool32_t                                supportsPinning;
} XGL_PHYSICAL_GPU_MEMORY_PROPERTIES;

typedef struct _XGL_MEMORY_ALLOC_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO
    const void*                             pNext;                      // Pointer to next structure
    XGL_GPU_SIZE                            allocationSize;             // Size of memory allocation
    XGL_FLAGS                               memProps;                   // XGL_MEMORY_PROPERTY_FLAGS
    XGL_MEMORY_TYPE                         memType;
    XGL_MEMORY_PRIORITY                     memPriority;
} XGL_MEMORY_ALLOC_INFO;

// This structure is included in the XGL_MEMORY_ALLOC_INFO chain
// for memory regions allocated for buffer usage.
typedef struct _XGL_MEMORY_ALLOC_BUFFER_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO
    const void*                             pNext;                      // Pointer to next structure
    XGL_FLAGS                               usage;                      // XGL_BUFFER_USAGE_FLAGS
} XGL_MEMORY_ALLOC_BUFFER_INFO;

// This structure is included in the XGL_MEMORY_ALLOC_INFO chain
// for memory regions allocated for image usage.
typedef struct _XGL_MEMORY_ALLOC_IMAGE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO
    const void*                             pNext;                      // Pointer to next structure
    XGL_FLAGS                               usage;                      // XGL_IMAGE_USAGE_FLAGS
    XGL_IMAGE_FORMAT_CLASS                  formatClass;
    uint32_t                                samples;
} XGL_MEMORY_ALLOC_IMAGE_INFO;

typedef struct _XGL_MEMORY_OPEN_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO
    const void*                             pNext;                      // Pointer to next structure
    XGL_GPU_MEMORY                          sharedMem;
} XGL_MEMORY_OPEN_INFO;

typedef struct _XGL_PEER_MEMORY_OPEN_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO
    const void*                             pNext;                      // Pointer to next structure
    XGL_GPU_MEMORY                          originalMem;
} XGL_PEER_MEMORY_OPEN_INFO;

typedef struct _XGL_MEMORY_REQUIREMENTS
{
    XGL_GPU_SIZE                            size;                       // Specified in bytes
    XGL_GPU_SIZE                            alignment;                  // Specified in bytes
    XGL_GPU_SIZE                            granularity;                // Granularity on which xglBindObjectMemoryRange can bind sub-ranges of memory specified in bytes (usually the page size)
    XGL_FLAGS                               memProps;                   // XGL_MEMORY_PROPERTY_FLAGS
    XGL_MEMORY_TYPE                         memType;
} XGL_MEMORY_REQUIREMENTS;

typedef struct _XGL_BUFFER_MEMORY_REQUIREMENTS
{
    XGL_FLAGS                               usage;                      // XGL_BUFFER_USAGE_FLAGS
} XGL_BUFFER_MEMORY_REQUIREMENTS;

typedef struct _XGL_IMAGE_MEMORY_REQUIREMENTS
{
    XGL_FLAGS                               usage;                      // XGL_IMAGE_USAGE_FLAGS
    XGL_IMAGE_FORMAT_CLASS                  formatClass;
    uint32_t                                samples;
} XGL_IMAGE_MEMORY_REQUIREMENTS;

typedef struct _XGL_FORMAT_PROPERTIES
{
    XGL_FLAGS                               linearTilingFeatures;      // XGL_FORMAT_FEATURE_FLAGS
    XGL_FLAGS                               optimalTilingFeatures;     // XGL_FORMAT_FEATURE_FLAGS
} XGL_FORMAT_PROPERTIES;

typedef struct _XGL_BUFFER_VIEW_ATTACH_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO
    const void*                             pNext;                      // Pointer to next structure
    XGL_BUFFER_VIEW                         view;
} XGL_BUFFER_VIEW_ATTACH_INFO;

typedef struct _XGL_IMAGE_VIEW_ATTACH_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO
    const void*                             pNext;                      // Pointer to next structure
    XGL_IMAGE_VIEW                          view;
    XGL_IMAGE_LAYOUT                        layout;
} XGL_IMAGE_VIEW_ATTACH_INFO;

typedef struct _XGL_UPDATE_SAMPLERS
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS
    const void*                             pNext;                      // Pointer to next structure
    uint32_t                                index;
    uint32_t                                count;
    const XGL_SAMPLER*                      pSamplers;
} XGL_UPDATE_SAMPLERS;

typedef struct _XGL_SAMPLER_IMAGE_VIEW_INFO
{
    XGL_SAMPLER                             pSampler;
    const XGL_IMAGE_VIEW_ATTACH_INFO*       pImageView;
} XGL_SAMPLER_IMAGE_VIEW_INFO;

typedef struct _XGL_UPDATE_SAMPLER_TEXTURES
{
    XGL_STRUCTURE_TYPE                       sType;                     // Must be XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES
    const void*                              pNext;                     // Pointer to next structure
    uint32_t                                 index;
    uint32_t                                 count;
    const XGL_SAMPLER_IMAGE_VIEW_INFO*       pSamplerImageViews;
} XGL_UPDATE_SAMPLER_TEXTURES;

typedef struct _XGL_UPDATE_IMAGES
{
    XGL_STRUCTURE_TYPE                       sType;                     // Must be XGL_STRUCTURE_TYPE_UPDATE_IMAGES
    const void*                              pNext;                     // Pointer to next structure
    XGL_DESCRIPTOR_TYPE                      descriptorType;
    uint32_t                                 index;
    uint32_t                                 count;
    const XGL_IMAGE_VIEW_ATTACH_INFO* const* pImageViews;
} XGL_UPDATE_IMAGES;

typedef struct _XGL_UPDATE_BUFFERS
{
    XGL_STRUCTURE_TYPE                        sType;                    // Must be XGL_STRUCTURE_TYPE_UPDATE_BUFFERS
    const void*                               pNext;                    // Pointer to next structure
    XGL_DESCRIPTOR_TYPE                       descriptorType;
    uint32_t                                  index;
    uint32_t                                  count;
    const XGL_BUFFER_VIEW_ATTACH_INFO* const* pBufferViews;
} XGL_UPDATE_BUFFERS;

typedef struct _XGL_UPDATE_AS_COPY
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_UPDATE_AS_COPY
    const void*                             pNext;                      // Pointer to next structure
    XGL_DESCRIPTOR_TYPE                     descriptorType;
    XGL_DESCRIPTOR_SET                      descriptorSet;
    uint32_t                                descriptorIndex;
    uint32_t                                count;
} XGL_UPDATE_AS_COPY;

typedef struct _XGL_BUFFER_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO
    const void*                             pNext;                      // Pointer to next structure.
    XGL_GPU_SIZE                            size;                       // Specified in bytes
    XGL_FLAGS                               usage;                      // XGL_BUFFER_USAGE_FLAGS
    XGL_FLAGS                               flags;                      // XGL_BUFFER_CREATE_FLAGS
} XGL_BUFFER_CREATE_INFO;

typedef struct _XGL_BUFFER_VIEW_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO
    const void*                             pNext;                      // Pointer to next structure.
    XGL_BUFFER                              buffer;
    XGL_BUFFER_VIEW_TYPE                    viewType;
    XGL_GPU_SIZE                            stride;                     // Optionally specifies stride between elements
    XGL_FORMAT                              format;                     // Optionally specifies format of elements
    XGL_GPU_SIZE                            offset;                     // Specified in bytes
    XGL_GPU_SIZE                            range;                      // View size specified in bytes
} XGL_BUFFER_VIEW_CREATE_INFO;

typedef struct _XGL_IMAGE_SUBRESOURCE
{
    XGL_IMAGE_ASPECT                        aspect;
    uint32_t                                mipLevel;
    uint32_t                                arraySlice;
} XGL_IMAGE_SUBRESOURCE;

typedef struct _XGL_IMAGE_SUBRESOURCE_RANGE
{
    XGL_IMAGE_ASPECT                        aspect;
    uint32_t                                baseMipLevel;
    uint32_t                                mipLevels;
    uint32_t                                baseArraySlice;
    uint32_t                                arraySize;
} XGL_IMAGE_SUBRESOURCE_RANGE;

typedef struct _XGL_EVENT_WAIT_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_EVENT_WAIT_INFO
    const void*                             pNext;                      // Pointer to next structure.

    uint32_t                                eventCount;                 // Number of events to wait on
    const XGL_EVENT*                        pEvents;                    // Array of event objects to wait on

    XGL_WAIT_EVENT                          waitEvent;                  // Pipeline event where the wait should happen

    uint32_t                                memBarrierCount;            // Number of memory barriers
    const void**                            ppMemBarriers;              // Array of pointers to memory barriers (any of them can be either XGL_MEMORY_BARRIER, XGL_BUFFER_MEMORY_BARRIER, or XGL_IMAGE_MEMORY_BARRIER)
} XGL_EVENT_WAIT_INFO;

typedef struct _XGL_PIPELINE_BARRIER
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_PIPELINE_BARRIER
    const void*                             pNext;                      // Pointer to next structure.

    uint32_t                                eventCount;                 // Number of events to wait on
    const XGL_SET_EVENT*                    pEvents;                    // Array of pipeline events to wait on

    XGL_WAIT_EVENT                          waitEvent;                  // Pipeline event where the wait should happen

    uint32_t                                memBarrierCount;            // Number of memory barriers
    const void**                            ppMemBarriers;              // Array of pointers to memory barriers (any of them can be either XGL_MEMORY_BARRIER, XGL_BUFFER_MEMORY_BARRIER, or XGL_IMAGE_MEMORY_BARRIER)
} XGL_PIPELINE_BARRIER;

typedef struct _XGL_MEMORY_BARRIER
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_MEMORY_BARRIER
    const void*                             pNext;                      // Pointer to next structure.

    XGL_FLAGS                               outputMask;                 // Outputs the barrier should sync (see XGL_MEMORY_OUTPUT_FLAGS)
    XGL_FLAGS                               inputMask;                  // Inputs the barrier should sync to (see XGL_MEMORY_INPUT_FLAGS)
} XGL_MEMORY_BARRIER;

typedef struct _XGL_BUFFER_MEMORY_BARRIER
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER
    const void*                             pNext;                      // Pointer to next structure.

    XGL_FLAGS                               outputMask;                 // Outputs the barrier should sync (see XGL_MEMORY_OUTPUT_FLAGS)
    XGL_FLAGS                               inputMask;                  // Inputs the barrier should sync to (see XGL_MEMORY_INPUT_FLAGS)

    XGL_BUFFER                              buffer;                     // Buffer to sync

    XGL_GPU_SIZE                            offset;                     // Offset within the buffer to sync
    XGL_GPU_SIZE                            size;                       // Amount of bytes to sync
} XGL_BUFFER_MEMORY_BARRIER;

typedef struct _XGL_IMAGE_MEMORY_BARRIER
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
    const void*                             pNext;                      // Pointer to next structure.

    XGL_FLAGS                               outputMask;                 // Outputs the barrier should sync (see XGL_MEMORY_OUTPUT_FLAGS)
    XGL_FLAGS                               inputMask;                  // Inputs the barrier should sync to (see XGL_MEMORY_INPUT_FLAGS)

    XGL_IMAGE_LAYOUT                        oldLayout;                  // Current layout of the image
    XGL_IMAGE_LAYOUT                        newLayout;                  // New layout to transition the image to

    XGL_IMAGE                               image;                      // Image to sync

    XGL_IMAGE_SUBRESOURCE_RANGE             subresourceRange;           // Subresource range to sync
} XGL_IMAGE_MEMORY_BARRIER;

typedef struct _XGL_IMAGE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                      // Must be XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO
    const void*                             pNext;                      // Pointer to next structure.
    XGL_IMAGE_TYPE                          imageType;
    XGL_FORMAT                              format;
    XGL_EXTENT3D                            extent;
    uint32_t                                mipLevels;
    uint32_t                                arraySize;
    uint32_t                                samples;
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

typedef struct _XGL_IMAGE_VIEW_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                  // Must be XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
    const void*                             pNext;                  // Pointer to next structure
    XGL_IMAGE                               image;
    XGL_IMAGE_VIEW_TYPE                     viewType;
    XGL_FORMAT                              format;
    XGL_CHANNEL_MAPPING                     channels;
    XGL_IMAGE_SUBRESOURCE_RANGE             subresourceRange;
    float                                   minLod;
} XGL_IMAGE_VIEW_CREATE_INFO;

typedef struct _XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                  // Must be XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO
    const void*                             pNext;                  // Pointer to next structure
    XGL_IMAGE                               image;
    XGL_FORMAT                              format;
    uint32_t                                mipLevel;
    uint32_t                                baseArraySlice;
    uint32_t                                arraySize;
    XGL_IMAGE                               msaaResolveImage;
    XGL_IMAGE_SUBRESOURCE_RANGE             msaaResolveSubResource;
} XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO;

typedef struct _XGL_DEPTH_STENCIL_VIEW_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;                  // Must be XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO
    const void*                             pNext;                  // Pointer to next structure
    XGL_IMAGE                               image;
    uint32_t                                mipLevel;
    uint32_t                                baseArraySlice;
    uint32_t                                arraySize;
    XGL_IMAGE                               msaaResolveImage;
    XGL_IMAGE_SUBRESOURCE_RANGE             msaaResolveSubResource;
    XGL_FLAGS                               flags;                  // XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS
} XGL_DEPTH_STENCIL_VIEW_CREATE_INFO;

typedef struct _XGL_COLOR_ATTACHMENT_BIND_INFO
{
    XGL_COLOR_ATTACHMENT_VIEW               view;
    XGL_IMAGE_LAYOUT                        layout;
} XGL_COLOR_ATTACHMENT_BIND_INFO;

typedef struct _XGL_DEPTH_STENCIL_BIND_INFO
{
    XGL_DEPTH_STENCIL_VIEW                  view;
    XGL_IMAGE_LAYOUT                        layout;
} XGL_DEPTH_STENCIL_BIND_INFO;

typedef struct _XGL_BUFFER_COPY
{
    XGL_GPU_SIZE                            srcOffset;              // Specified in bytes
    XGL_GPU_SIZE                            destOffset;             // Specified in bytes
    XGL_GPU_SIZE                            copySize;               // Specified in bytes
} XGL_BUFFER_COPY;

typedef struct _XGL_IMAGE_MEMORY_BIND_INFO
{
    XGL_IMAGE_SUBRESOURCE                   subresource;
    XGL_OFFSET3D                            offset;
    XGL_EXTENT3D                            extent;
} XGL_IMAGE_MEMORY_BIND_INFO;

typedef struct _XGL_IMAGE_COPY
{
    XGL_IMAGE_SUBRESOURCE                   srcSubresource;
    XGL_OFFSET3D                            srcOffset;
    XGL_IMAGE_SUBRESOURCE                   destSubresource;
    XGL_OFFSET3D                            destOffset;
    XGL_EXTENT3D                            extent;
} XGL_IMAGE_COPY;

typedef struct _XGL_BUFFER_IMAGE_COPY
{
    XGL_GPU_SIZE                            bufferOffset;           // Specified in bytes
    XGL_IMAGE_SUBRESOURCE                   imageSubresource;
    XGL_OFFSET3D                            imageOffset;
    XGL_EXTENT3D                            imageExtent;
} XGL_BUFFER_IMAGE_COPY;

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
    const void*                             pNext;              // Pointer to next structure
    size_t                                  codeSize;           // Specified in bytes
    const void*                             pCode;
    XGL_FLAGS                               flags;              // Reserved
} XGL_SHADER_CREATE_INFO;

typedef struct _XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;              // Must be XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    const void*                             pNext;              // Pointer to next structure
    XGL_DESCRIPTOR_TYPE                     descriptorType;
    uint32_t                                count;
    XGL_FLAGS                               stageFlags;         // XGL_SHADER_STAGE_FLAGS
    XGL_SAMPLER                             immutableSampler;
} XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

typedef struct _XGL_DESCRIPTOR_TYPE_COUNT
{
    XGL_DESCRIPTOR_TYPE                     type;
    uint32_t                                count;
} XGL_DESCRIPTOR_TYPE_COUNT;

typedef struct _XGL_DESCRIPTOR_REGION_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;              // Must be XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO
    const void*                             pNext;              // Pointer to next structure
    uint32_t                                count;
    const XGL_DESCRIPTOR_TYPE_COUNT*        pTypeCount;
} XGL_DESCRIPTOR_REGION_CREATE_INFO;

typedef struct _XGL_LINK_CONST_BUFFER
{
    uint32_t                                    bufferId;
    size_t                                      bufferSize;
    const void*                                 pBufferData;
} XGL_LINK_CONST_BUFFER;

typedef struct _XGL_SPECIALIZATION_MAP_ENTRY
{
    uint32_t                                constantId;         // The SpecConstant ID specified in the BIL
    uint32_t                                offset;             // Offset of the value in the data block
} XGL_SPECIALIZATION_MAP_ENTRY;

typedef struct _XGL_SPECIALIZATION_INFO
{
    uint32_t                                mapEntryCount;
    const XGL_SPECIALIZATION_MAP_ENTRY*     pMap;               // mapEntryCount entries
    const void*                             pData;
} XGL_SPECIALIZATION_INFO;

typedef struct _XGL_PIPELINE_SHADER
{
    XGL_PIPELINE_SHADER_STAGE               stage;
    XGL_SHADER                              shader;
    uint32_t                                linkConstBufferCount;
    const XGL_LINK_CONST_BUFFER*            pLinkConstBufferInfo;
    const XGL_SPECIALIZATION_INFO*          pSpecializationInfo;
} XGL_PIPELINE_SHADER;

typedef struct _XGL_COMPUTE_PIPELINE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    XGL_PIPELINE_SHADER                     cs;
    XGL_FLAGS                               flags;      // XGL_PIPELINE_CREATE_FLAGS
    XGL_DESCRIPTOR_SET_LAYOUT               lastSetLayout;
    // For local size fields zero is treated an invalid value
    uint32_t                                localSizeX;
    uint32_t                                localSizeY;
    uint32_t                                localSizeZ;

} XGL_COMPUTE_PIPELINE_CREATE_INFO;

typedef struct _XGL_VERTEX_INPUT_BINDING_DESCRIPTION
{
    uint32_t                                strideInBytes;  // Distance between vertices in bytes (0 = no advancement)

    XGL_VERTEX_INPUT_STEP_RATE              stepRate;       // Rate at which binding is incremented
} XGL_VERTEX_INPUT_BINDING_DESCRIPTION;

typedef struct _XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION
{
    uint32_t                                binding;        // index into vertexBindingDescriptions

    XGL_FORMAT                              format;         // format of source data

    uint32_t                                offsetInBytes;  // Offset of first element in bytes from base of vertex
} XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION;

typedef struct _XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                            sType;          // Should be XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO
    const void*                                   pNext;          // Pointer to next structure

    uint32_t                                      bindingCount;   // number of bindings
    const XGL_VERTEX_INPUT_BINDING_DESCRIPTION*   pVertexBindingDescriptions;

    uint32_t                                      attributeCount; // number of attributes
    const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pVertexAttributeDescriptions;
} XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO;

typedef struct _XGL_PIPELINE_IA_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    XGL_PRIMITIVE_TOPOLOGY                  topology;
    bool32_t                                disableVertexReuse;         // optional
    bool32_t                                primitiveRestartEnable;
    uint32_t                                primitiveRestartIndex;      // optional (GL45)
} XGL_PIPELINE_IA_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_TESS_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    uint32_t                                patchControlPoints;
} XGL_PIPELINE_TESS_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_VP_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    uint32_t                                numViewports;
    XGL_COORDINATE_ORIGIN                   clipOrigin;                 // optional (GL45)
    XGL_DEPTH_MODE                          depthMode;                  // optional (GL45)
} XGL_PIPELINE_VP_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_RS_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    bool32_t                                depthClipEnable;
    bool32_t                                rasterizerDiscardEnable;
    bool32_t                                programPointSize;           // optional (GL45)
    XGL_COORDINATE_ORIGIN                   pointOrigin;                // optional (GL45)
    XGL_PROVOKING_VERTEX_CONVENTION         provokingVertex;            // optional (GL45)
    XGL_FILL_MODE                           fillMode;                   // optional (GL45)
    XGL_CULL_MODE                           cullMode;
    XGL_FACE_ORIENTATION                    frontFace;
} XGL_PIPELINE_RS_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_MS_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    uint32_t                                samples;
    bool32_t                                multisampleEnable;          // optional (GL45)
    bool32_t                                sampleShadingEnable;        // optional (GL45)
    float                                   minSampleShading;           // optional (GL45)
    XGL_SAMPLE_MASK                         sampleMask;
} XGL_PIPELINE_MS_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_CB_ATTACHMENT_STATE
{
    bool32_t                                blendEnable;
    XGL_FORMAT                              format;
    XGL_BLEND                               srcBlendColor;
    XGL_BLEND                               destBlendColor;
    XGL_BLEND_FUNC                          blendFuncColor;
    XGL_BLEND                               srcBlendAlpha;
    XGL_BLEND                               destBlendAlpha;
    XGL_BLEND_FUNC                          blendFuncAlpha;
    uint8_t                                 channelWriteMask;
} XGL_PIPELINE_CB_ATTACHMENT_STATE;

typedef struct _XGL_PIPELINE_CB_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    bool32_t                                alphaToCoverageEnable;
    bool32_t                                logicOpEnable;
    XGL_LOGIC_OP                            logicOp;
    uint32_t                                attachmentCount;    // # of pAttachments
    const XGL_PIPELINE_CB_ATTACHMENT_STATE* pAttachments;
} XGL_PIPELINE_CB_STATE_CREATE_INFO;

typedef struct _XGL_STENCIL_OP_STATE
{
    XGL_STENCIL_OP                          stencilFailOp;
    XGL_STENCIL_OP                          stencilPassOp;
    XGL_STENCIL_OP                          stencilDepthFailOp;
    XGL_COMPARE_FUNC                        stencilFunc;
} XGL_STENCIL_OP_STATE;

typedef struct _XGL_PIPELINE_DS_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    XGL_FORMAT                              format;
    bool32_t                                depthTestEnable;
    bool32_t                                depthWriteEnable;
    XGL_COMPARE_FUNC                        depthFunc;
    bool32_t                                depthBoundsEnable;          // optional (depth_bounds_test)
    bool32_t                                stencilTestEnable;
    XGL_STENCIL_OP_STATE                    front;
    XGL_STENCIL_OP_STATE                    back;
} XGL_PIPELINE_DS_STATE_CREATE_INFO;

typedef struct _XGL_PIPELINE_SHADER_STAGE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    XGL_PIPELINE_SHADER                     shader;
} XGL_PIPELINE_SHADER_STAGE_CREATE_INFO;

typedef struct _XGL_GRAPHICS_PIPELINE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    XGL_FLAGS                               flags;      // XGL_PIPELINE_CREATE_FLAGS
    XGL_DESCRIPTOR_SET_LAYOUT               lastSetLayout;
} XGL_GRAPHICS_PIPELINE_CREATE_INFO;

typedef struct _XGL_SAMPLER_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;          // Must be XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
    const void*                             pNext;          // Pointer to next structure
    XGL_TEX_FILTER                          magFilter;      // Filter mode for magnification
    XGL_TEX_FILTER                          minFilter;      // Filter mode for minifiation
    XGL_TEX_MIPMAP_MODE                     mipMode;        // Mipmap selection mode
    XGL_TEX_ADDRESS                         addressU;
    XGL_TEX_ADDRESS                         addressV;
    XGL_TEX_ADDRESS                         addressW;
    float                                   mipLodBias;
    uint32_t                                maxAnisotropy;
    XGL_COMPARE_FUNC                        compareFunc;
    float                                   minLod;
    float                                   maxLod;
    XGL_BORDER_COLOR_TYPE                   borderColorType;
} XGL_SAMPLER_CREATE_INFO;

typedef struct _XGL_DYNAMIC_VP_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    uint32_t                                viewportAndScissorCount;  // number of entries in pViewports and pScissors
    const XGL_VIEWPORT*                     pViewports;
    const XGL_RECT*                         pScissors;
} XGL_DYNAMIC_VP_STATE_CREATE_INFO;

typedef struct _XGL_DYNAMIC_RS_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    float                                   depthBias;
    float                                   depthBiasClamp;
    float                                   slopeScaledDepthBias;
    float                                   pointSize;          // optional (GL45) - Size of points
    float                                   pointFadeThreshold; // optional (GL45) - Size of point fade threshold
    float                                   lineWidth;          // optional (GL45) - Width of lines
} XGL_DYNAMIC_RS_STATE_CREATE_INFO;

typedef struct _XGL_DYNAMIC_CB_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    float                                   blendConst[4];
} XGL_DYNAMIC_CB_STATE_CREATE_INFO;

typedef struct _XGL_DYNAMIC_DS_STATE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    float                                   minDepth;               // optional (depth_bounds_test)
    float                                   maxDepth;               // optional (depth_bounds_test)
    uint32_t                                stencilReadMask;
    uint32_t                                stencilWriteMask;
    uint32_t                                stencilFrontRef;
    uint32_t                                stencilBackRef;
} XGL_DYNAMIC_DS_STATE_CREATE_INFO;

typedef struct _XGL_CMD_BUFFER_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    uint32_t                                queueNodeIndex;
    XGL_FLAGS                               flags;
} XGL_CMD_BUFFER_CREATE_INFO;

typedef struct _XGL_CMD_BUFFER_BEGIN_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO
    const void*                             pNext;      // Pointer to next structure

    XGL_FLAGS                               flags;      // XGL_CMD_BUFFER_BUILD_FLAGS
} XGL_CMD_BUFFER_BEGIN_INFO;

typedef struct _XGL_RENDER_PASS_BEGIN
{
    XGL_RENDER_PASS                         renderPass;
    XGL_FRAMEBUFFER                         framebuffer;
} XGL_RENDER_PASS_BEGIN;

typedef struct _XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO
    const void*                             pNext;      // Pointer to next structure

    XGL_RENDER_PASS_BEGIN                   renderPassContinue;  // Only needed when a render pass is split across two command buffers
} XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO;

// Union allowing specification of floating point or raw color data. Actual value selected is based on image being cleared.
typedef union _XGL_CLEAR_COLOR_VALUE
{
    float                                   floatColor[4];
    uint32_t                                rawColor[4];
} XGL_CLEAR_COLOR_VALUE;

typedef struct _XGL_CLEAR_COLOR
{
    XGL_CLEAR_COLOR_VALUE                   color;
    bool32_t                                useRawValue;
} XGL_CLEAR_COLOR;

typedef struct _XGL_RENDER_PASS_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure

    XGL_RECT                                renderArea;
    uint32_t                                colorAttachmentCount;
    XGL_EXTENT2D                            extent;
    uint32_t                                sampleCount;
    uint32_t                                layers;
    const XGL_FORMAT*                       pColorFormats;
    const XGL_IMAGE_LAYOUT*                 pColorLayouts;
    const XGL_ATTACHMENT_LOAD_OP*           pColorLoadOps;
    const XGL_ATTACHMENT_STORE_OP*          pColorStoreOps;
    const XGL_CLEAR_COLOR*                  pColorLoadClearValues;
    XGL_FORMAT                              depthStencilFormat;
    XGL_IMAGE_LAYOUT                        depthStencilLayout;
    XGL_ATTACHMENT_LOAD_OP                  depthLoadOp;
    float                                   depthLoadClearValue;
    XGL_ATTACHMENT_STORE_OP                 depthStoreOp;
    XGL_ATTACHMENT_LOAD_OP                  stencilLoadOp;
    uint32_t                                stencilLoadClearValue;
    XGL_ATTACHMENT_STORE_OP                 stencilStoreOp;
} XGL_RENDER_PASS_CREATE_INFO;

typedef struct _XGL_MEMORY_REF
{
    XGL_GPU_MEMORY                          mem;
    XGL_FLAGS                               flags;      // XGL_MEMORY_REF_FLAGS
} XGL_MEMORY_REF;

typedef struct _XGL_EVENT_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    XGL_FLAGS                               flags;      // Reserved
} XGL_EVENT_CREATE_INFO;

typedef struct _XGL_FENCE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    XGL_FLAGS                               flags;      // Reserved
} XGL_FENCE_CREATE_INFO;

typedef struct _XGL_QUEUE_SEMAPHORE_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    uint32_t                                initialCount;
    XGL_FLAGS                               flags;      // XGL_SEMAPHORE_CREATE_FLAGS
} XGL_QUEUE_SEMAPHORE_CREATE_INFO;

typedef struct _XGL_QUEUE_SEMAPHORE_OPEN_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_OPEN_INFO
    const void*                             pNext;      // Pointer to next structure
    XGL_QUEUE_SEMAPHORE                     sharedSemaphore;
} XGL_QUEUE_SEMAPHORE_OPEN_INFO;

typedef struct _XGL_PIPELINE_STATISTICS_DATA
{
    uint64_t                                fsInvocations;            // Fragment shader invocations
    uint64_t                                cPrimitives;              // Clipper primitives
    uint64_t                                cInvocations;             // Clipper invocations
    uint64_t                                vsInvocations;            // Vertex shader invocations
    uint64_t                                gsInvocations;            // Geometry shader invocations
    uint64_t                                gsPrimitives;             // Geometry shader primitives
    uint64_t                                iaPrimitives;             // Input primitives
    uint64_t                                iaVertices;               // Input vertices
    uint64_t                                tcsInvocations;           // Tessellation control shader invocations
    uint64_t                                tesInvocations;           // Tessellation evaluation shader invocations
    uint64_t                                csInvocations;            // Compute shader invocations
} XGL_PIPELINE_STATISTICS_DATA;

typedef struct _XGL_QUERY_POOL_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;      // Must be XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO
    const void*                             pNext;      // Pointer to next structure
    XGL_QUERY_TYPE                          queryType;
    uint32_t                                slots;
} XGL_QUERY_POOL_CREATE_INFO;

typedef struct _XGL_FRAMEBUFFER_CREATE_INFO
{
    XGL_STRUCTURE_TYPE                      sType;  // Must be XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
    const void*                             pNext;  // Pointer to next structure

    uint32_t                                colorAttachmentCount;
    const XGL_COLOR_ATTACHMENT_BIND_INFO*   pColorAttachments;
    const XGL_DEPTH_STENCIL_BIND_INFO*      pDepthStencilAttachment;

    uint32_t                                sampleCount;
    uint32_t                                width;
    uint32_t                                height;
    uint32_t                                layers;
} XGL_FRAMEBUFFER_CREATE_INFO;

typedef struct _XGL_DRAW_INDIRECT_CMD
{
    uint32_t                                vertexCount;
    uint32_t                                instanceCount;
    uint32_t                                firstVertex;
    uint32_t                                firstInstance;
} XGL_DRAW_INDIRECT_CMD;

typedef struct _XGL_DRAW_INDEXED_INDIRECT_CMD
{
    uint32_t                                indexCount;
    uint32_t                                instanceCount;
    uint32_t                                firstIndex;
    int32_t                                 vertexOffset;
    uint32_t                                firstInstance;
} XGL_DRAW_INDEXED_INDIRECT_CMD;

typedef struct _XGL_DISPATCH_INDIRECT_CMD
{
    uint32_t   x;
    uint32_t   y;
    uint32_t   z;
} XGL_DISPATCH_INDIRECT_CMD;

// ------------------------------------------------------------------------------------------------
// API functions
typedef XGL_RESULT (XGLAPI *xglCreateInstanceType)(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, XGL_INSTANCE* pInstance);
typedef XGL_RESULT (XGLAPI *xglDestroyInstanceType)(XGL_INSTANCE instance);
typedef XGL_RESULT (XGLAPI *xglEnumerateGpusType)(XGL_INSTANCE instance, uint32_t maxGpus, uint32_t* pGpuCount, XGL_PHYSICAL_GPU* pGpus);
typedef XGL_RESULT (XGLAPI *xglGetGpuInfoType)(XGL_PHYSICAL_GPU gpu, XGL_PHYSICAL_GPU_INFO_TYPE infoType, size_t* pDataSize, void* pData);
typedef void *     (XGLAPI *xglGetProcAddrType)(XGL_PHYSICAL_GPU gpu, const char * pName);
typedef XGL_RESULT (XGLAPI *xglCreateDeviceType)(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_DEVICE* pDevice);
typedef XGL_RESULT (XGLAPI *xglDestroyDeviceType)(XGL_DEVICE device);
typedef XGL_RESULT (XGLAPI *xglGetExtensionSupportType)(XGL_PHYSICAL_GPU gpu, const char* pExtName);
typedef XGL_RESULT (XGLAPI *xglEnumerateLayersType)(XGL_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved);
typedef XGL_RESULT (XGLAPI *xglGetDeviceQueueType)(XGL_DEVICE device, uint32_t queueNodeIndex, uint32_t queueIndex, XGL_QUEUE* pQueue);
typedef XGL_RESULT (XGLAPI *xglQueueSubmitType)(XGL_QUEUE queue, uint32_t cmdBufferCount, const XGL_CMD_BUFFER* pCmdBuffers, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs, XGL_FENCE fence);
typedef XGL_RESULT (XGLAPI *xglQueueSetGlobalMemReferencesType)(XGL_QUEUE queue, uint32_t memRefCount, const XGL_MEMORY_REF* pMemRefs);
typedef XGL_RESULT (XGLAPI *xglQueueWaitIdleType)(XGL_QUEUE queue);
typedef XGL_RESULT (XGLAPI *xglDeviceWaitIdleType)(XGL_DEVICE device);
typedef XGL_RESULT (XGLAPI *xglAllocMemoryType)(XGL_DEVICE device, const XGL_MEMORY_ALLOC_INFO* pAllocInfo, XGL_GPU_MEMORY* pMem);
typedef XGL_RESULT (XGLAPI *xglFreeMemoryType)(XGL_GPU_MEMORY mem);
typedef XGL_RESULT (XGLAPI *xglSetMemoryPriorityType)(XGL_GPU_MEMORY mem, XGL_MEMORY_PRIORITY priority);
typedef XGL_RESULT (XGLAPI *xglMapMemoryType)(XGL_GPU_MEMORY mem, XGL_FLAGS flags, void** ppData);
typedef XGL_RESULT (XGLAPI *xglUnmapMemoryType)(XGL_GPU_MEMORY mem);
typedef XGL_RESULT (XGLAPI *xglPinSystemMemoryType)(XGL_DEVICE device, const void* pSysMem, size_t memSize, XGL_GPU_MEMORY* pMem);
typedef XGL_RESULT (XGLAPI *xglGetMultiGpuCompatibilityType)(XGL_PHYSICAL_GPU gpu0, XGL_PHYSICAL_GPU gpu1, XGL_GPU_COMPATIBILITY_INFO* pInfo);
typedef XGL_RESULT (XGLAPI *xglOpenSharedMemoryType)(XGL_DEVICE device, const XGL_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem);
typedef XGL_RESULT (XGLAPI *xglOpenSharedQueueSemaphoreType)(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo, XGL_QUEUE_SEMAPHORE* pSemaphore);
typedef XGL_RESULT (XGLAPI *xglOpenPeerMemoryType)(XGL_DEVICE device, const XGL_PEER_MEMORY_OPEN_INFO* pOpenInfo, XGL_GPU_MEMORY* pMem);
typedef XGL_RESULT (XGLAPI *xglOpenPeerImageType)(XGL_DEVICE device, const XGL_PEER_IMAGE_OPEN_INFO* pOpenInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem);
typedef XGL_RESULT (XGLAPI *xglDestroyObjectType)(XGL_OBJECT object);
typedef XGL_RESULT (XGLAPI *xglGetObjectInfoType)(XGL_BASE_OBJECT object, XGL_OBJECT_INFO_TYPE infoType, size_t* pDataSize, void* pData);
typedef XGL_RESULT (XGLAPI *xglBindObjectMemoryType)(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_MEMORY mem, XGL_GPU_SIZE offset);
typedef XGL_RESULT (XGLAPI *xglBindObjectMemoryRangeType)(XGL_OBJECT object, uint32_t allocationIdx, XGL_GPU_SIZE rangeOffset,XGL_GPU_SIZE rangeSize, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset);
typedef XGL_RESULT (XGLAPI *xglBindImageMemoryRangeType)(XGL_IMAGE image, uint32_t allocationIdx, const XGL_IMAGE_MEMORY_BIND_INFO* bindInfo, XGL_GPU_MEMORY mem, XGL_GPU_SIZE memOffset);
typedef XGL_RESULT (XGLAPI *xglCreateFenceType)(XGL_DEVICE device, const XGL_FENCE_CREATE_INFO* pCreateInfo, XGL_FENCE* pFence);
typedef XGL_RESULT (XGLAPI *xglGetFenceStatusType)(XGL_FENCE fence);
typedef XGL_RESULT (XGLAPI *xglWaitForFencesType)(XGL_DEVICE device, uint32_t fenceCount, const XGL_FENCE* pFences, bool32_t waitAll, uint64_t timeout);
typedef XGL_RESULT (XGLAPI *xglCreateQueueSemaphoreType)(XGL_DEVICE device, const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo, XGL_QUEUE_SEMAPHORE* pSemaphore);
typedef XGL_RESULT (XGLAPI *xglSignalQueueSemaphoreType)(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore);
typedef XGL_RESULT (XGLAPI *xglWaitQueueSemaphoreType)(XGL_QUEUE queue, XGL_QUEUE_SEMAPHORE semaphore);
typedef XGL_RESULT (XGLAPI *xglCreateEventType)(XGL_DEVICE device, const XGL_EVENT_CREATE_INFO* pCreateInfo, XGL_EVENT* pEvent);
typedef XGL_RESULT (XGLAPI *xglGetEventStatusType)(XGL_EVENT event);
typedef XGL_RESULT (XGLAPI *xglSetEventType)(XGL_EVENT event);
typedef XGL_RESULT (XGLAPI *xglResetEventType)(XGL_EVENT event);
typedef XGL_RESULT (XGLAPI *xglCreateQueryPoolType)(XGL_DEVICE device, const XGL_QUERY_POOL_CREATE_INFO* pCreateInfo, XGL_QUERY_POOL* pQueryPool);
typedef XGL_RESULT (XGLAPI *xglGetQueryPoolResultsType)(XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData);
typedef XGL_RESULT (XGLAPI *xglGetFormatInfoType)(XGL_DEVICE device, XGL_FORMAT format, XGL_FORMAT_INFO_TYPE infoType, size_t* pDataSize, void* pData);
typedef XGL_RESULT (XGLAPI *xglCreateBufferType)(XGL_DEVICE device, const XGL_BUFFER_CREATE_INFO* pCreateInfo, XGL_BUFFER* pBuffer);
typedef XGL_RESULT (XGLAPI *xglCreateBufferViewType)(XGL_DEVICE device, const XGL_BUFFER_VIEW_CREATE_INFO* pCreateInfo, XGL_BUFFER_VIEW* pView);
typedef XGL_RESULT (XGLAPI *xglCreateImageType)(XGL_DEVICE device, const XGL_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage);
typedef XGL_RESULT (XGLAPI *xglSetFastClearColorType)(XGL_IMAGE image, const float color[4]);
typedef XGL_RESULT (XGLAPI *xglSetFastClearDepthType)(XGL_IMAGE image, float depth);
typedef XGL_RESULT (XGLAPI *xglGetImageSubresourceInfoType)(XGL_IMAGE image, const XGL_IMAGE_SUBRESOURCE* pSubresource, XGL_SUBRESOURCE_INFO_TYPE infoType, size_t* pDataSize, void* pData);
typedef XGL_RESULT (XGLAPI *xglCreateImageViewType)(XGL_DEVICE device, const XGL_IMAGE_VIEW_CREATE_INFO* pCreateInfo, XGL_IMAGE_VIEW* pView);
typedef XGL_RESULT (XGLAPI *xglCreateColorAttachmentViewType)(XGL_DEVICE device, const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, XGL_COLOR_ATTACHMENT_VIEW* pView);
typedef XGL_RESULT (XGLAPI *xglCreateDepthStencilViewType)(XGL_DEVICE device, const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, XGL_DEPTH_STENCIL_VIEW* pView);
typedef XGL_RESULT (XGLAPI *xglCreateShaderType)(XGL_DEVICE device, const XGL_SHADER_CREATE_INFO* pCreateInfo, XGL_SHADER* pShader);
typedef XGL_RESULT (XGLAPI *xglCreateGraphicsPipelineType)(XGL_DEVICE device, const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline);
typedef XGL_RESULT (XGLAPI *xglCreateComputePipelineType)(XGL_DEVICE device, const XGL_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, XGL_PIPELINE* pPipeline);
typedef XGL_RESULT (XGLAPI *xglStorePipelineType)(XGL_PIPELINE pipeline, size_t* pDataSize, void* pData);
typedef XGL_RESULT (XGLAPI *xglLoadPipelineType)(XGL_DEVICE device, size_t dataSize, const void* pData, XGL_PIPELINE* pPipeline);
typedef XGL_RESULT (XGLAPI *xglCreatePipelineDeltaType)(XGL_DEVICE device, XGL_PIPELINE p1, XGL_PIPELINE p2, XGL_PIPELINE_DELTA* delta);
typedef XGL_RESULT (XGLAPI *xglCreateSamplerType)(XGL_DEVICE device, const XGL_SAMPLER_CREATE_INFO* pCreateInfo, XGL_SAMPLER* pSampler);
typedef XGL_RESULT (XGLAPI *xglCreateDescriptorSetLayoutType)(XGL_DEVICE device, XGL_FLAGS stageFlags, const uint32_t* pSetBindPoints, XGL_DESCRIPTOR_SET_LAYOUT priorSetLayout, const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList, XGL_DESCRIPTOR_SET_LAYOUT* pSetLayout);
typedef XGL_RESULT (XGLAPI *xglBeginDescriptorRegionUpdateType)(XGL_DEVICE device, XGL_DESCRIPTOR_UPDATE_MODE updateMode);
typedef XGL_RESULT (XGLAPI *xglEndDescriptorRegionUpdateType)(XGL_DEVICE device, XGL_CMD_BUFFER cmd);
typedef XGL_RESULT (XGLAPI *xglCreateDescriptorRegionType)(XGL_DEVICE device, XGL_DESCRIPTOR_REGION_USAGE regionUsage, uint32_t maxSets, const XGL_DESCRIPTOR_REGION_CREATE_INFO* pCreateInfo, XGL_DESCRIPTOR_REGION* pDescriptorRegion);
typedef XGL_RESULT (XGLAPI *xglClearDescriptorRegionType)(XGL_DESCRIPTOR_REGION descriptorRegion);
typedef XGL_RESULT (XGLAPI *xglAllocDescriptorSetsType)(XGL_DESCRIPTOR_REGION descriptorRegion, XGL_DESCRIPTOR_SET_USAGE setUsage, uint32_t count, const XGL_DESCRIPTOR_SET_LAYOUT* pSetLayouts, XGL_DESCRIPTOR_SET* pDescriptorSets, uint32_t* pCount);
typedef void       (XGLAPI *xglClearDescriptorSetsType)(XGL_DESCRIPTOR_REGION descriptorRegion, uint32_t count, const XGL_DESCRIPTOR_SET* pDescriptorSets);
typedef void       (XGLAPI *xglUpdateDescriptorsType)(XGL_DESCRIPTOR_SET descriptorSet, const void* pUpdateChain);
typedef XGL_RESULT (XGLAPI *xglCreateDynamicViewportStateType)(XGL_DEVICE device, const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_VP_STATE_OBJECT* pState);
typedef XGL_RESULT (XGLAPI *xglCreateDynamicRasterStateType)(XGL_DEVICE device, const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_RS_STATE_OBJECT* pState);
typedef XGL_RESULT (XGLAPI *xglCreateDynamicColorBlendStateType)(XGL_DEVICE device, const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_CB_STATE_OBJECT* pState);
typedef XGL_RESULT (XGLAPI *xglCreateDynamicDepthStencilStateType)(XGL_DEVICE device, const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo, XGL_DYNAMIC_DS_STATE_OBJECT* pState);
typedef XGL_RESULT (XGLAPI *xglCreateCommandBufferType)(XGL_DEVICE device, const XGL_CMD_BUFFER_CREATE_INFO* pCreateInfo, XGL_CMD_BUFFER* pCmdBuffer);
typedef XGL_RESULT (XGLAPI *xglBeginCommandBufferType)(XGL_CMD_BUFFER cmdBuffer, const XGL_CMD_BUFFER_BEGIN_INFO* pBeginInfo);
typedef XGL_RESULT (XGLAPI *xglEndCommandBufferType)(XGL_CMD_BUFFER cmdBuffer);
typedef XGL_RESULT (XGLAPI *xglResetCommandBufferType)(XGL_CMD_BUFFER cmdBuffer);
typedef void       (XGLAPI *xglCmdBindPipelineType)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE pipeline);
typedef void       (XGLAPI *xglCmdBindPipelineDeltaType)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_PIPELINE_DELTA delta);
typedef void       (XGLAPI *xglCmdBindDynamicStateObjectType)(XGL_CMD_BUFFER cmdBuffer, XGL_STATE_BIND_POINT stateBindPoint, XGL_DYNAMIC_STATE_OBJECT state);
typedef void       (XGLAPI *xglCmdBindDescriptorSetType)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, XGL_DESCRIPTOR_SET descriptorSet, const uint32_t* pUserData);
typedef void       (XGLAPI *xglCmdBindIndexBufferType)(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, XGL_INDEX_TYPE indexType);
typedef void       (XGLAPI *xglCmdBindVertexBufferType)(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t binding);
typedef void       (XGLAPI *xglCmdDrawType)(XGL_CMD_BUFFER cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount);
typedef void       (XGLAPI *xglCmdDrawIndexedType)(XGL_CMD_BUFFER cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount);
typedef void       (XGLAPI *xglCmdDrawIndirectType)(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride);
typedef void       (XGLAPI *xglCmdDrawIndexedIndirectType)(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset, uint32_t count, uint32_t stride);
typedef void       (XGLAPI *xglCmdDispatchType)(XGL_CMD_BUFFER cmdBuffer, uint32_t x, uint32_t y, uint32_t z);
typedef void       (XGLAPI *xglCmdDispatchIndirectType)(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER buffer, XGL_GPU_SIZE offset);
typedef void       (XGLAPI *xglCmdCopyBufferType)(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_COPY* pRegions);
typedef void       (XGLAPI *xglCmdCopyImageType)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, uint32_t regionCount, const XGL_IMAGE_COPY* pRegions);
typedef void       (XGLAPI *xglCmdCopyBufferToImageType)(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER srcBuffer, XGL_IMAGE destImage, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions);
typedef void       (XGLAPI *xglCmdCopyImageToBufferType)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_BUFFER destBuffer, uint32_t regionCount, const XGL_BUFFER_IMAGE_COPY* pRegions);
typedef void       (XGLAPI *xglCmdCloneImageDataType)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE_LAYOUT srcImageLayout, XGL_IMAGE destImage, XGL_IMAGE_LAYOUT destImageLayout);
typedef void       (XGLAPI *xglCmdUpdateBufferType)(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE dataSize, const uint32_t* pData);
typedef void       (XGLAPI *xglCmdFillBufferType)(XGL_CMD_BUFFER cmdBuffer, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset, XGL_GPU_SIZE fillSize, uint32_t data);
typedef void       (XGLAPI *xglCmdClearColorImageType)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, XGL_CLEAR_COLOR color, uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
typedef void       (XGLAPI *xglCmdClearDepthStencilType)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE image, float depth, uint32_t stencil, uint32_t rangeCount, const XGL_IMAGE_SUBRESOURCE_RANGE* pRanges);
typedef void       (XGLAPI *xglCmdResolveImageType)(XGL_CMD_BUFFER cmdBuffer, XGL_IMAGE srcImage, XGL_IMAGE destImage, uint32_t rectCount, const XGL_IMAGE_RESOLVE* pRects);
typedef void       (XGLAPI *xglCmdSetEventType)(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event, XGL_SET_EVENT pipeEvent);
typedef void       (XGLAPI *xglCmdResetEventType)(XGL_CMD_BUFFER cmdBuffer, XGL_EVENT event);
typedef void       (XGLAPI *xglCmdWaitEventsType)(XGL_CMD_BUFFER cmdBuffer, const XGL_EVENT_WAIT_INFO* pWaitInfo);
typedef void       (XGLAPI *xglCmdPipelineBarrierType)(XGL_CMD_BUFFER cmdBuffer, const XGL_PIPELINE_BARRIER* pBarrier);
typedef void       (XGLAPI *xglCmdBeginQueryType)(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot, XGL_FLAGS flags);
typedef void       (XGLAPI *xglCmdEndQueryType)(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t slot);
typedef void       (XGLAPI *xglCmdResetQueryPoolType)(XGL_CMD_BUFFER cmdBuffer, XGL_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount);
typedef void       (XGLAPI *xglCmdWriteTimestampType)(XGL_CMD_BUFFER cmdBuffer, XGL_TIMESTAMP_TYPE timestampType, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset);
typedef void       (XGLAPI *xglCmdInitAtomicCountersType)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData);
typedef void       (XGLAPI *xglCmdLoadAtomicCountersType)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER srcBuffer, XGL_GPU_SIZE srcOffset);
typedef void       (XGLAPI *xglCmdSaveAtomicCountersType)(XGL_CMD_BUFFER cmdBuffer, XGL_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, XGL_BUFFER destBuffer, XGL_GPU_SIZE destOffset);
typedef XGL_RESULT (XGLAPI *xglCreateFramebufferType)(XGL_DEVICE device, const XGL_FRAMEBUFFER_CREATE_INFO* pCreateInfo, XGL_FRAMEBUFFER* pFramebuffer);
typedef XGL_RESULT (XGLAPI *xglCreateRenderPassType)(XGL_DEVICE device, const XGL_RENDER_PASS_CREATE_INFO* pCreateInfo, XGL_RENDER_PASS* pRenderPass);
typedef void       (XGLAPI *xglCmdBeginRenderPassType)(XGL_CMD_BUFFER cmdBuffer, const XGL_RENDER_PASS_BEGIN* pRenderPassBegin);
typedef void       (XGLAPI *xglCmdEndRenderPassType)(XGL_CMD_BUFFER cmdBuffer, XGL_RENDER_PASS renderPass);

#ifdef XGL_PROTOTYPES

// GPU initialization

XGL_RESULT XGLAPI xglCreateInstance(
    const XGL_APPLICATION_INFO*                 pAppInfo,
    const XGL_ALLOC_CALLBACKS*                  pAllocCb,
    XGL_INSTANCE*                               pInstance);

XGL_RESULT XGLAPI xglDestroyInstance(
    XGL_INSTANCE                                instance);

XGL_RESULT XGLAPI xglEnumerateGpus(
    XGL_INSTANCE                                instance,
    uint32_t                                    maxGpus,
    uint32_t*                                   pGpuCount,
    XGL_PHYSICAL_GPU*                           pGpus);

XGL_RESULT XGLAPI xglGetGpuInfo(
    XGL_PHYSICAL_GPU                            gpu,
    XGL_PHYSICAL_GPU_INFO_TYPE                  infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

void * XGLAPI xglGetProcAddr(
    XGL_PHYSICAL_GPU                            gpu,
    const char*                                 pName);

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
    const char*                                 pExtName);

// Layer discovery functions

XGL_RESULT XGLAPI xglEnumerateLayers(
    XGL_PHYSICAL_GPU                            gpu,
    size_t                                      maxLayerCount,
    size_t                                      maxStringSize,
    size_t*                                     pOutLayerCount,
    char* const*                                pOutLayers,
    void*                                       pReserved);

// Queue functions

XGL_RESULT XGLAPI xglGetDeviceQueue(
    XGL_DEVICE                                  device,
    uint32_t                                    queueNodeIndex,
    uint32_t                                    queueIndex,
    XGL_QUEUE*                                  pQueue);

XGL_RESULT XGLAPI xglQueueSubmit(
    XGL_QUEUE                                   queue,
    uint32_t                                    cmdBufferCount,
    const XGL_CMD_BUFFER*                       pCmdBuffers,
    uint32_t                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs,
    XGL_FENCE                                   fence);

XGL_RESULT XGLAPI xglQueueSetGlobalMemReferences(
    XGL_QUEUE                                   queue,
    uint32_t                                    memRefCount,
    const XGL_MEMORY_REF*                       pMemRefs);

XGL_RESULT XGLAPI xglQueueWaitIdle(
    XGL_QUEUE                                   queue);

XGL_RESULT XGLAPI xglDeviceWaitIdle(
    XGL_DEVICE                                  device);

// Memory functions

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
    void**                                      ppData);

XGL_RESULT XGLAPI xglUnmapMemory(
    XGL_GPU_MEMORY                              mem);

XGL_RESULT XGLAPI xglPinSystemMemory(
    XGL_DEVICE                                  device,
    const void*                                 pSysMem,
    size_t                                      memSize,
    XGL_GPU_MEMORY*                             pMem);

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
    size_t*                                     pDataSize,
    void*                                       pData);

XGL_RESULT XGLAPI xglBindObjectMemory(
    XGL_OBJECT                                  object,
    uint32_t                                    allocationIdx,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                memOffset);

XGL_RESULT XGLAPI xglBindObjectMemoryRange(
    XGL_OBJECT                                  object,
    uint32_t                                    allocationIdx,
    XGL_GPU_SIZE                                rangeOffset,
    XGL_GPU_SIZE                                rangeSize,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                memOffset);

XGL_RESULT XGLAPI xglBindImageMemoryRange(
    XGL_IMAGE                                   image,
    uint32_t                                    allocationIdx,
    const XGL_IMAGE_MEMORY_BIND_INFO*           bindInfo,
    XGL_GPU_MEMORY                              mem,
    XGL_GPU_SIZE                                memOffset);

// Fence functions

XGL_RESULT XGLAPI xglCreateFence(
    XGL_DEVICE                                  device,
    const XGL_FENCE_CREATE_INFO*                pCreateInfo,
    XGL_FENCE*                                  pFence);

XGL_RESULT XGLAPI xglGetFenceStatus(
    XGL_FENCE fence);

XGL_RESULT XGLAPI xglWaitForFences(
    XGL_DEVICE                                  device,
    uint32_t                                    fenceCount,
    const XGL_FENCE*                            pFences,
    bool32_t                                    waitAll,
    uint64_t                                    timeout);

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
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    size_t*                                     pDataSize,
    void*                                       pData);

// Format capabilities

XGL_RESULT XGLAPI xglGetFormatInfo(
    XGL_DEVICE                                  device,
    XGL_FORMAT                                  format,
    XGL_FORMAT_INFO_TYPE                        infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

// Buffer functions

XGL_RESULT XGLAPI xglCreateBuffer(
    XGL_DEVICE                                  device,
    const XGL_BUFFER_CREATE_INFO*               pCreateInfo,
    XGL_BUFFER*                                 pBuffer);

// Buffer view functions

XGL_RESULT XGLAPI xglCreateBufferView(
    XGL_DEVICE                                  device,
    const XGL_BUFFER_VIEW_CREATE_INFO*          pCreateInfo,
    XGL_BUFFER_VIEW*                            pView);

// Image functions

XGL_RESULT XGLAPI xglCreateImage(
    XGL_DEVICE                                  device,
    const XGL_IMAGE_CREATE_INFO*                pCreateInfo,
    XGL_IMAGE*                                  pImage);

XGL_RESULT XGLAPI xglSetFastClearColor(
    XGL_IMAGE                                   image,
    const float                                 color[4]);

XGL_RESULT XGLAPI xglSetFastClearDepth(
    XGL_IMAGE                                   image,
    float                                       depth);

XGL_RESULT XGLAPI xglGetImageSubresourceInfo(
    XGL_IMAGE                                   image,
    const XGL_IMAGE_SUBRESOURCE*                pSubresource,
    XGL_SUBRESOURCE_INFO_TYPE                   infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

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
    size_t*                                     pDataSize,
    void*                                       pData);

XGL_RESULT XGLAPI xglLoadPipeline(
    XGL_DEVICE                                  device,
    size_t                                      dataSize,
    const void*                                 pData,
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

XGL_RESULT XGLAPI xglCreateDescriptorSetLayout(
    XGL_DEVICE                                   device,
    XGL_FLAGS                                    stageFlags,            // XGL_SHADER_STAGE_FLAGS
    const uint32_t*                              pSetBindPoints,
    XGL_DESCRIPTOR_SET_LAYOUT                    priorSetLayout,
    const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pSetLayoutInfoList,
    XGL_DESCRIPTOR_SET_LAYOUT*                   pSetLayout);

XGL_RESULT XGLAPI xglBeginDescriptorRegionUpdate(
    XGL_DEVICE                                   device,
    XGL_DESCRIPTOR_UPDATE_MODE                   updateMode);

XGL_RESULT XGLAPI xglEndDescriptorRegionUpdate(
    XGL_DEVICE                                   device,
    XGL_CMD_BUFFER                               cmd);

XGL_RESULT XGLAPI xglCreateDescriptorRegion(
    XGL_DEVICE                                   device,
    XGL_DESCRIPTOR_REGION_USAGE                  regionUsage,
    uint32_t                                     maxSets,
    const XGL_DESCRIPTOR_REGION_CREATE_INFO*     pCreateInfo,
    XGL_DESCRIPTOR_REGION*                       pDescriptorRegion);

XGL_RESULT XGLAPI xglClearDescriptorRegion(
    XGL_DESCRIPTOR_REGION                        descriptorRegion);

XGL_RESULT XGLAPI xglAllocDescriptorSets(
    XGL_DESCRIPTOR_REGION                        descriptorRegion,
    XGL_DESCRIPTOR_SET_USAGE                     setUsage,
    uint32_t                                     count,
    const XGL_DESCRIPTOR_SET_LAYOUT*             pSetLayouts,
    XGL_DESCRIPTOR_SET*                          pDescriptorSets,
    uint32_t*                                    pCount);

void XGLAPI xglClearDescriptorSets(
    XGL_DESCRIPTOR_REGION                        descriptorRegion,
    uint32_t                                     count,
    const XGL_DESCRIPTOR_SET*                    pDescriptorSets);

void XGLAPI xglUpdateDescriptors(
    XGL_DESCRIPTOR_SET                           descriptorSet,
    const void*                                  pUpdateChain);

// State object functions

XGL_RESULT XGLAPI xglCreateDynamicViewportState(
    XGL_DEVICE                                  device,
    const XGL_DYNAMIC_VP_STATE_CREATE_INFO*     pCreateInfo,
    XGL_DYNAMIC_VP_STATE_OBJECT*                pState);

XGL_RESULT XGLAPI xglCreateDynamicRasterState(
    XGL_DEVICE                                  device,
    const XGL_DYNAMIC_RS_STATE_CREATE_INFO*     pCreateInfo,
    XGL_DYNAMIC_RS_STATE_OBJECT*                pState);

XGL_RESULT XGLAPI xglCreateDynamicColorBlendState(
    XGL_DEVICE                                  device,
    const XGL_DYNAMIC_CB_STATE_CREATE_INFO*     pCreateInfo,
    XGL_DYNAMIC_CB_STATE_OBJECT*                pState);

XGL_RESULT XGLAPI xglCreateDynamicDepthStencilState(
    XGL_DEVICE                                  device,
    const XGL_DYNAMIC_DS_STATE_CREATE_INFO*     pCreateInfo,
    XGL_DYNAMIC_DS_STATE_OBJECT*                pState);

// Command buffer functions

XGL_RESULT XGLAPI xglCreateCommandBuffer(
    XGL_DEVICE                                  device,
    const XGL_CMD_BUFFER_CREATE_INFO*           pCreateInfo,
    XGL_CMD_BUFFER*                             pCmdBuffer);

XGL_RESULT XGLAPI xglBeginCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_CMD_BUFFER_BEGIN_INFO*            pBeginInfo);

XGL_RESULT XGLAPI xglEndCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer);

XGL_RESULT XGLAPI xglResetCommandBuffer(
    XGL_CMD_BUFFER                              cmdBuffer);

// Command buffer building functions

void XGLAPI xglCmdBindPipeline(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE                                pipeline);

void XGLAPI xglCmdBindPipelineDelta(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_PIPELINE_DELTA                          delta);

void XGLAPI xglCmdBindDynamicStateObject(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_STATE_BIND_POINT                        stateBindPoint,
    XGL_DYNAMIC_STATE_OBJECT                    dynamicState);

void XGLAPI xglCmdBindDescriptorSet(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    XGL_DESCRIPTOR_SET                          descriptorSet,
    const uint32_t*                             pUserData);

void XGLAPI xglCmdBindIndexBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    XGL_INDEX_TYPE                              indexType);

void XGLAPI xglCmdBindVertexBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    uint32_t                                    binding);

void XGLAPI xglCmdDraw(
    XGL_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    firstVertex,
    uint32_t                                    vertexCount,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount);

void XGLAPI xglCmdDrawIndexed(
    XGL_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    firstIndex,
    uint32_t                                    indexCount,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount);

void XGLAPI xglCmdDrawIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    uint32_t                                    count,
    uint32_t                                    stride);

void XGLAPI xglCmdDrawIndexedIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset,
    uint32_t                                    count,
    uint32_t                                    stride);

void XGLAPI xglCmdDispatch(
    XGL_CMD_BUFFER                              cmdBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z);

void XGLAPI xglCmdDispatchIndirect(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  buffer,
    XGL_GPU_SIZE                                offset);

void XGLAPI xglCmdCopyBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  srcBuffer,
    XGL_BUFFER                                  destBuffer,
    uint32_t                                    regionCount,
    const XGL_BUFFER_COPY*                      pRegions);

void XGLAPI xglCmdCopyImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    uint32_t                                    regionCount,
    const XGL_IMAGE_COPY*                       pRegions);

void XGLAPI xglCmdCopyBufferToImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  srcBuffer,
    XGL_IMAGE                                   destImage,
    uint32_t                                    regionCount,
    const XGL_BUFFER_IMAGE_COPY*                pRegions);

void XGLAPI xglCmdCopyImageToBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_BUFFER                                  destBuffer,
    uint32_t                                    regionCount,
    const XGL_BUFFER_IMAGE_COPY*                pRegions);

void XGLAPI xglCmdCloneImageData(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE_LAYOUT                            srcImageLayout,
    XGL_IMAGE                                   destImage,
    XGL_IMAGE_LAYOUT                            destImageLayout);

void XGLAPI xglCmdUpdateBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                dataSize,
    const uint32_t*                             pData);

void XGLAPI xglCmdFillBuffer(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset,
    XGL_GPU_SIZE                                fillSize,
    uint32_t                                    data);

void XGLAPI xglCmdClearColorImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    XGL_CLEAR_COLOR                             color,
    uint32_t                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

void XGLAPI xglCmdClearDepthStencil(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   image,
    float                                       depth,
    uint32_t                                    stencil,
    uint32_t                                    rangeCount,
    const XGL_IMAGE_SUBRESOURCE_RANGE*          pRanges);

void XGLAPI xglCmdResolveImage(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_IMAGE                                   srcImage,
    XGL_IMAGE                                   destImage,
    uint32_t                                    rectCount,
    const XGL_IMAGE_RESOLVE*                    pRects);

void XGLAPI xglCmdSetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event,
    XGL_SET_EVENT                               pipeEvent);

void XGLAPI xglCmdResetEvent(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_EVENT                                   event);

void XGLAPI xglCmdWaitEvents(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_EVENT_WAIT_INFO*                  pWaitInfo);

void XGLAPI xglCmdPipelineBarrier(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_PIPELINE_BARRIER*                 pBarrier);

void XGLAPI xglCmdBeginQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    uint32_t                                    slot,
    XGL_FLAGS                                   flags);

void XGLAPI xglCmdEndQuery(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    uint32_t                                    slot);

void XGLAPI xglCmdResetQueryPool(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_QUERY_POOL                              queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount);

void XGLAPI xglCmdWriteTimestamp(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_TIMESTAMP_TYPE                          timestampType,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset);

void XGLAPI xglCmdInitAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    const uint32_t*                             pData);

void XGLAPI xglCmdLoadAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    XGL_BUFFER                                  srcBuffer,
    XGL_GPU_SIZE                                srcOffset);

void XGLAPI xglCmdSaveAtomicCounters(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_PIPELINE_BIND_POINT                     pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    XGL_BUFFER                                  destBuffer,
    XGL_GPU_SIZE                                destOffset);

XGL_RESULT XGLAPI xglCreateFramebuffer(
    XGL_DEVICE                                  device,
    const XGL_FRAMEBUFFER_CREATE_INFO*          pCreateInfo,
    XGL_FRAMEBUFFER*                            pFramebuffer);

XGL_RESULT XGLAPI xglCreateRenderPass(
    XGL_DEVICE                                  device,
    const XGL_RENDER_PASS_CREATE_INFO*          pCreateInfo,
    XGL_RENDER_PASS*                            pRenderPass);

void XGLAPI xglCmdBeginRenderPass(
    XGL_CMD_BUFFER                              cmdBuffer,
    const XGL_RENDER_PASS_BEGIN*                pRenderPassBegin);

void XGLAPI xglCmdEndRenderPass(
    XGL_CMD_BUFFER                              cmdBuffer,
    XGL_RENDER_PASS                             renderPass);

#endif // XGL_PROTOTYPES

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
        const void*           pNext;  // Pointer to next structure
        // More XFB state, if any goes here
    } XGL_DEPTH_STENCIL_VIEW_CREATE_INFO;

    We expect that only the shader-side configuration (via layout qualifiers or their IR
    equivalent) is used to configure the data written to each stream. When transform
    feedback is part of the pipeline, transform feedback binding would be available
    through a new API bind point:

        xglCmdBindTransformFeedbackMemoryView(
                XGL_CMD_BUFFER                              cmdBuffer,
                XGL_PIPELINE_BIND_POINT                     pipelineBindPoint, // = GRAPHICS
                uint32_t                                    index,
                const XGL_MEMORY_VIEW_ATTACH_INFO*          pMemView);

    2) "Bindless" + support for non-bindless hardware.

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

    3) Clean up some remaining Mantle'isms.

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
