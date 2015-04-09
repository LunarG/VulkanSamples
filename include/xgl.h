//
// File: vulkan.h
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

#ifndef __VULKAN_H__
#define __VULKAN_H__

#define VK_MAKE_VERSION(major, minor, patch) \
    ((major << 22) | (minor << 12) | patch)

#include "vkPlatform.h"

// Vulkan API version supported by this file
#define VK_API_VERSION VK_MAKE_VERSION(0, 69, 0)

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
***************************************************************************************************
*   Core Vulkan API
***************************************************************************************************
*/

#ifdef __cplusplus
    #define VK_DEFINE_HANDLE(_obj) struct _obj##_T {char _dummy;}; typedef _obj##_T* _obj;
    #define VK_DEFINE_SUBCLASS_HANDLE(_obj, _base) struct _obj##_T : public _base##_T {}; typedef _obj##_T* _obj;
#else // __cplusplus
    #define VK_DEFINE_HANDLE(_obj) typedef void* _obj;
    #define VK_DEFINE_SUBCLASS_HANDLE(_obj, _base) typedef void* _obj;
#endif // __cplusplus

VK_DEFINE_HANDLE(VK_INSTANCE)
VK_DEFINE_HANDLE(VK_PHYSICAL_GPU)
VK_DEFINE_HANDLE(VK_BASE_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DEVICE, VK_BASE_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_QUEUE, VK_BASE_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_GPU_MEMORY, VK_BASE_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_OBJECT, VK_BASE_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_BUFFER, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_BUFFER_VIEW, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_IMAGE, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_IMAGE_VIEW, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_COLOR_ATTACHMENT_VIEW, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DEPTH_STENCIL_VIEW, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_SHADER, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_PIPELINE, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_SAMPLER, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DESCRIPTOR_SET, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DESCRIPTOR_SET_LAYOUT, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DESCRIPTOR_SET_LAYOUT_CHAIN, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DESCRIPTOR_POOL, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DYNAMIC_STATE_OBJECT, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DYNAMIC_VP_STATE_OBJECT, VK_DYNAMIC_STATE_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DYNAMIC_RS_STATE_OBJECT, VK_DYNAMIC_STATE_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DYNAMIC_CB_STATE_OBJECT, VK_DYNAMIC_STATE_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_DYNAMIC_DS_STATE_OBJECT, VK_DYNAMIC_STATE_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_CMD_BUFFER, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_FENCE, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_SEMAPHORE, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_EVENT, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_QUERY_POOL, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_FRAMEBUFFER, VK_OBJECT)
VK_DEFINE_SUBCLASS_HANDLE(VK_RENDER_PASS, VK_OBJECT)

#define VK_MAX_PHYSICAL_GPUS       16
#define VK_MAX_PHYSICAL_GPU_NAME   256

#define VK_LOD_CLAMP_NONE       MAX_FLOAT
#define VK_LAST_MIP_OR_SLICE    0xffffffff

#define VK_TRUE  1
#define VK_FALSE 0

#define VK_NULL_HANDLE 0

// This macro defines INT_MAX in enumerations to force compilers to use 32 bits
// to represent them. This may or may not be necessary on some compilers. The
// option to compile it out may allow compilers that warn about missing enumerants
// in switch statements to be silenced.
#define VK_MAX_ENUM(T) T##_MAX_ENUM = 0x7FFFFFFF

// ------------------------------------------------------------------------------------------------
// Enumerations


typedef enum _VK_MEMORY_PRIORITY
{
    VK_MEMORY_PRIORITY_UNUSED                               = 0x0,
    VK_MEMORY_PRIORITY_VERY_LOW                             = 0x1,
    VK_MEMORY_PRIORITY_LOW                                  = 0x2,
    VK_MEMORY_PRIORITY_NORMAL                               = 0x3,
    VK_MEMORY_PRIORITY_HIGH                                 = 0x4,
    VK_MEMORY_PRIORITY_VERY_HIGH                            = 0x5,

    VK_MEMORY_PRIORITY_BEGIN_RANGE                          = VK_MEMORY_PRIORITY_UNUSED,
    VK_MEMORY_PRIORITY_END_RANGE                            = VK_MEMORY_PRIORITY_VERY_HIGH,
    VK_NUM_MEMORY_PRIORITY                                  = (VK_MEMORY_PRIORITY_END_RANGE - VK_MEMORY_PRIORITY_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_MEMORY_PRIORITY)
} VK_MEMORY_PRIORITY;

typedef enum _VK_IMAGE_LAYOUT
{
    VK_IMAGE_LAYOUT_UNDEFINED                               = 0x00000000,   // Implicit layout an image is when its contents are undefined due to various reasons (e.g. right after creation)
    VK_IMAGE_LAYOUT_GENERAL                                 = 0x00000001,   // General layout when image can be used for any kind of access
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL                = 0x00000002,   // Optimal layout when image is only used for color attachment read/write
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL        = 0x00000003,   // Optimal layout when image is only used for depth/stencil attachment read/write
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL         = 0x00000004,   // Optimal layout when image is used for read only depth/stencil attachment and shader access
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL                = 0x00000005,   // Optimal layout when image is used for read only shader access
    VK_IMAGE_LAYOUT_CLEAR_OPTIMAL                           = 0x00000006,   // Optimal layout when image is used only for clear operations
    VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL                 = 0x00000007,   // Optimal layout when image is used only as source of transfer operations
    VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL            = 0x00000008,   // Optimal layout when image is used only as destination of transfer operations

    VK_IMAGE_LAYOUT_BEGIN_RANGE                             = VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_END_RANGE                               = VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,
    VK_NUM_IMAGE_LAYOUT                                     = (VK_IMAGE_LAYOUT_END_RANGE - VK_IMAGE_LAYOUT_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_IMAGE_LAYOUT)
} VK_IMAGE_LAYOUT;

typedef enum _VK_PIPE_EVENT
{
    VK_PIPE_EVENT_TOP_OF_PIPE                               = 0x00000001,   // Set event before the GPU starts processing subsequent command
    VK_PIPE_EVENT_VERTEX_PROCESSING_COMPLETE                = 0x00000002,   // Set event when all pending vertex processing is complete
    VK_PIPE_EVENT_LOCAL_FRAGMENT_PROCESSING_COMPLETE        = 0x00000003,   // Set event when all pending fragment shader executions are complete, within each fragment location
    VK_PIPE_EVENT_FRAGMENT_PROCESSING_COMPLETE              = 0x00000004,   // Set event when all pending fragment shader executions are complete
    VK_PIPE_EVENT_GRAPHICS_PIPELINE_COMPLETE                = 0x00000005,   // Set event when all pending graphics operations are complete
    VK_PIPE_EVENT_COMPUTE_PIPELINE_COMPLETE                 = 0x00000006,   // Set event when all pending compute operations are complete
    VK_PIPE_EVENT_TRANSFER_COMPLETE                         = 0x00000007,   // Set event when all pending transfer operations are complete
    VK_PIPE_EVENT_GPU_COMMANDS_COMPLETE                     = 0x00000008,   // Set event when all pending GPU work is complete

    VK_PIPE_EVENT_BEGIN_RANGE                               = VK_PIPE_EVENT_TOP_OF_PIPE,
    VK_PIPE_EVENT_END_RANGE                                 = VK_PIPE_EVENT_GPU_COMMANDS_COMPLETE,
    VK_NUM_PIPE_EVENT                                       = (VK_PIPE_EVENT_END_RANGE - VK_PIPE_EVENT_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_PIPE_EVENT)
} VK_PIPE_EVENT;

typedef enum _VK_WAIT_EVENT
{
    VK_WAIT_EVENT_TOP_OF_PIPE                               = 0x00000001,   // Wait event before the GPU starts processing subsequent commands
    VK_WAIT_EVENT_BEFORE_RASTERIZATION                      = 0x00000002,   // Wait event before rasterizing subsequent primitives

    VK_WAIT_EVENT_BEGIN_RANGE                               = VK_WAIT_EVENT_TOP_OF_PIPE,
    VK_WAIT_EVENT_END_RANGE                                 = VK_WAIT_EVENT_BEFORE_RASTERIZATION,
    VK_NUM_WAIT_EVENT                                       = (VK_WAIT_EVENT_END_RANGE - VK_WAIT_EVENT_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_WAIT_EVENT)
} VK_WAIT_EVENT;

typedef enum _VK_MEMORY_OUTPUT_FLAGS
{
    VK_MEMORY_OUTPUT_CPU_WRITE_BIT                          = 0x00000001,   // Controls output coherency of CPU writes
    VK_MEMORY_OUTPUT_SHADER_WRITE_BIT                       = 0x00000002,   // Controls output coherency of generic shader writes
    VK_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT                   = 0x00000004,   // Controls output coherency of color attachment writes
    VK_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT           = 0x00000008,   // Controls output coherency of depth/stencil attachment writes
    VK_MEMORY_OUTPUT_COPY_BIT                               = 0x00000010,   // Controls output coherency of copy operations
    VK_MAX_ENUM(_VK_MEMORY_OUTPUT_FLAGS)
} VK_MEMORY_OUTPUT_FLAGS;

typedef enum _VK_MEMORY_INPUT_FLAGS
{
    VK_MEMORY_INPUT_CPU_READ_BIT                            = 0x00000001,   // Controls input coherency of CPU reads
    VK_MEMORY_INPUT_INDIRECT_COMMAND_BIT                    = 0x00000002,   // Controls input coherency of indirect command reads
    VK_MEMORY_INPUT_INDEX_FETCH_BIT                         = 0x00000004,   // Controls input coherency of index fetches
    VK_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT              = 0x00000008,   // Controls input coherency of vertex attribute fetches
    VK_MEMORY_INPUT_UNIFORM_READ_BIT                        = 0x00000010,   // Controls input coherency of uniform buffer reads
    VK_MEMORY_INPUT_SHADER_READ_BIT                         = 0x00000020,   // Controls input coherency of generic shader reads
    VK_MEMORY_INPUT_COLOR_ATTACHMENT_BIT                    = 0x00000040,   // Controls input coherency of color attachment reads
    VK_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT            = 0x00000080,   // Controls input coherency of depth/stencil attachment reads
    VK_MEMORY_INPUT_COPY_BIT                                = 0x00000100,   // Controls input coherency of copy operations
    VK_MAX_ENUM(_VK_MEMORY_INPUT_FLAGS)
} VK_MEMORY_INPUT_FLAGS;

typedef enum _VK_ATTACHMENT_LOAD_OP
{
    VK_ATTACHMENT_LOAD_OP_LOAD                              = 0x00000000,
    VK_ATTACHMENT_LOAD_OP_CLEAR                             = 0x00000001,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE                         = 0x00000002,

    VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE                       = VK_ATTACHMENT_LOAD_OP_LOAD,
    VK_ATTACHMENT_LOAD_OP_END_RANGE                         = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    VK_NUM_ATTACHMENT_LOAD_OP                               = (VK_ATTACHMENT_LOAD_OP_END_RANGE - VK_ATTACHMENT_LOAD_OP_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_ATTACHMENT_LOAD_OP)
} VK_ATTACHMENT_LOAD_OP;

typedef enum _VK_ATTACHMENT_STORE_OP
{
    VK_ATTACHMENT_STORE_OP_STORE                            = 0x00000000,
    VK_ATTACHMENT_STORE_OP_RESOLVE_MSAA                     = 0x00000001,
    VK_ATTACHMENT_STORE_OP_DONT_CARE                        = 0x00000002,

    VK_ATTACHMENT_STORE_OP_BEGIN_RANGE                      = VK_ATTACHMENT_STORE_OP_STORE,
    VK_ATTACHMENT_STORE_OP_END_RANGE                        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    VK_NUM_ATTACHMENT_STORE_OP                              = (VK_ATTACHMENT_STORE_OP_END_RANGE - VK_ATTACHMENT_STORE_OP_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_ATTACHMENT_STORE_OP)
} VK_ATTACHMENT_STORE_OP;

typedef enum _VK_IMAGE_TYPE
{
    VK_IMAGE_1D                                             = 0x00000000,
    VK_IMAGE_2D                                             = 0x00000001,
    VK_IMAGE_3D                                             = 0x00000002,

    VK_IMAGE_TYPE_BEGIN_RANGE                               = VK_IMAGE_1D,
    VK_IMAGE_TYPE_END_RANGE                                 = VK_IMAGE_3D,
    VK_NUM_IMAGE_TYPE                                       = (VK_IMAGE_TYPE_END_RANGE - VK_IMAGE_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_IMAGE_TYPE)
} VK_IMAGE_TYPE;

typedef enum _VK_IMAGE_TILING
{
    VK_LINEAR_TILING                                        = 0x00000000,
    VK_OPTIMAL_TILING                                       = 0x00000001,

    VK_IMAGE_TILING_BEGIN_RANGE                             = VK_LINEAR_TILING,
    VK_IMAGE_TILING_END_RANGE                               = VK_OPTIMAL_TILING,
    VK_NUM_IMAGE_TILING                                     = (VK_IMAGE_TILING_END_RANGE - VK_IMAGE_TILING_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_IMAGE_TILING)
} VK_IMAGE_TILING;

typedef enum _VK_IMAGE_VIEW_TYPE
{
    VK_IMAGE_VIEW_1D                                        = 0x00000000,
    VK_IMAGE_VIEW_2D                                        = 0x00000001,
    VK_IMAGE_VIEW_3D                                        = 0x00000002,
    VK_IMAGE_VIEW_CUBE                                      = 0x00000003,

    VK_IMAGE_VIEW_TYPE_BEGIN_RANGE                          = VK_IMAGE_VIEW_1D,
    VK_IMAGE_VIEW_TYPE_END_RANGE                            = VK_IMAGE_VIEW_CUBE,
    VK_NUM_IMAGE_VIEW_TYPE                                  = (VK_IMAGE_VIEW_TYPE_END_RANGE - VK_IMAGE_VIEW_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_IMAGE_VIEW_TYPE)
} VK_IMAGE_VIEW_TYPE;

typedef enum _VK_IMAGE_ASPECT
{
    VK_IMAGE_ASPECT_COLOR                                   = 0x00000000,
    VK_IMAGE_ASPECT_DEPTH                                   = 0x00000001,
    VK_IMAGE_ASPECT_STENCIL                                 = 0x00000002,

    VK_IMAGE_ASPECT_BEGIN_RANGE                             = VK_IMAGE_ASPECT_COLOR,
    VK_IMAGE_ASPECT_END_RANGE                               = VK_IMAGE_ASPECT_STENCIL,
    VK_NUM_IMAGE_ASPECT                                     = (VK_IMAGE_ASPECT_END_RANGE - VK_IMAGE_ASPECT_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_IMAGE_ASPECT)
} VK_IMAGE_ASPECT;

typedef enum _VK_CHANNEL_SWIZZLE
{
    VK_CHANNEL_SWIZZLE_ZERO                                 = 0x00000000,
    VK_CHANNEL_SWIZZLE_ONE                                  = 0x00000001,
    VK_CHANNEL_SWIZZLE_R                                    = 0x00000002,
    VK_CHANNEL_SWIZZLE_G                                    = 0x00000003,
    VK_CHANNEL_SWIZZLE_B                                    = 0x00000004,
    VK_CHANNEL_SWIZZLE_A                                    = 0x00000005,

    VK_CHANNEL_SWIZZLE_BEGIN_RANGE                          = VK_CHANNEL_SWIZZLE_ZERO,
    VK_CHANNEL_SWIZZLE_END_RANGE                            = VK_CHANNEL_SWIZZLE_A,
    VK_NUM_CHANNEL_SWIZZLE                                  = (VK_CHANNEL_SWIZZLE_END_RANGE - VK_CHANNEL_SWIZZLE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_CHANNEL_SWIZZLE)
} VK_CHANNEL_SWIZZLE;

typedef enum _VK_DESCRIPTOR_TYPE
{
    VK_DESCRIPTOR_TYPE_SAMPLER                              = 0x00000000,
    VK_DESCRIPTOR_TYPE_SAMPLER_TEXTURE                      = 0x00000001,
    VK_DESCRIPTOR_TYPE_TEXTURE                              = 0x00000002,
    VK_DESCRIPTOR_TYPE_TEXTURE_BUFFER                       = 0x00000003,
    VK_DESCRIPTOR_TYPE_IMAGE                                = 0x00000004,
    VK_DESCRIPTOR_TYPE_IMAGE_BUFFER                         = 0x00000005,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER                       = 0x00000006,
    VK_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER                = 0x00000007,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC               = 0x00000008,
    VK_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER_DYNAMIC        = 0x00000009,

    VK_DESCRIPTOR_TYPE_BEGIN_RANGE                          = VK_DESCRIPTOR_TYPE_SAMPLER,
    VK_DESCRIPTOR_TYPE_END_RANGE                            = VK_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER_DYNAMIC,
    VK_NUM_DESCRIPTOR_TYPE                                  = (VK_DESCRIPTOR_TYPE_END_RANGE - VK_DESCRIPTOR_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_DESCRIPTOR_TYPE)
} VK_DESCRIPTOR_TYPE;

typedef enum _VK_DESCRIPTOR_POOL_USAGE
{
    VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT                       = 0x00000000,
    VK_DESCRIPTOR_POOL_USAGE_DYNAMIC                        = 0x00000001,

    VK_DESCRIPTOR_POOL_USAGE_BEGIN_RANGE                    = VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT,
    VK_DESCRIPTOR_POOL_USAGE_END_RANGE                      = VK_DESCRIPTOR_POOL_USAGE_DYNAMIC,
    VK_NUM_DESCRIPTOR_POOL_USAGE                            = (VK_DESCRIPTOR_POOL_USAGE_END_RANGE - VK_DESCRIPTOR_POOL_USAGE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_DESCRIPTOR_POOL_USAGE)
} VK_DESCRIPTOR_POOL_USAGE;

typedef enum _VK_DESCRIPTOR_UPDATE_MODE
{
    VK_DESCRIPTOR_UDPATE_MODE_COPY                          = 0x00000000,
    VK_DESCRIPTOR_UPDATE_MODE_FASTEST                       = 0x00000001,

    VK_DESCRIPTOR_UPDATE_MODE_BEGIN_RANGE                   = VK_DESCRIPTOR_UDPATE_MODE_COPY,
    VK_DESCRIPTOR_UPDATE_MODE_END_RANGE                     = VK_DESCRIPTOR_UPDATE_MODE_FASTEST,
    VK_NUM_DESCRIPTOR_UPDATE_MODE                           = (VK_DESCRIPTOR_UPDATE_MODE_END_RANGE - VK_DESCRIPTOR_UPDATE_MODE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_DESCRIPTOR_UPDATE_MODE)
} VK_DESCRIPTOR_UPDATE_MODE;

typedef enum _VK_DESCRIPTOR_SET_USAGE
{
    VK_DESCRIPTOR_SET_USAGE_ONE_SHOT                        = 0x00000000,
    VK_DESCRIPTOR_SET_USAGE_STATIC                          = 0x00000001,

    VK_DESCRIPTOR_SET_USAGE_BEGIN_RANGE                     = VK_DESCRIPTOR_SET_USAGE_ONE_SHOT,
    VK_DESCRIPTOR_SET_USAGE_END_RANGE                       = VK_DESCRIPTOR_SET_USAGE_STATIC,
    VK_NUM_DESCRIPTOR_SET_USAGE                             = (VK_DESCRIPTOR_SET_USAGE_END_RANGE - VK_DESCRIPTOR_SET_USAGE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_DESCRIPTOR_SET_USAGE)
} VK_DESCRIPTOR_SET_USAGE;

typedef enum _VK_QUERY_TYPE
{
    VK_QUERY_OCCLUSION                                      = 0x00000000,
    VK_QUERY_PIPELINE_STATISTICS                            = 0x00000001,

    VK_QUERY_TYPE_BEGIN_RANGE                               = VK_QUERY_OCCLUSION,
    VK_QUERY_TYPE_END_RANGE                                 = VK_QUERY_PIPELINE_STATISTICS,
    VK_NUM_QUERY_TYPE                                       = (VK_QUERY_TYPE_END_RANGE - VK_QUERY_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_QUERY_TYPE)
} VK_QUERY_TYPE;

typedef enum _VK_TIMESTAMP_TYPE
{
    VK_TIMESTAMP_TOP                                        = 0x00000000,
    VK_TIMESTAMP_BOTTOM                                     = 0x00000001,

    VK_TIMESTAMP_TYPE_BEGIN_RANGE                           = VK_TIMESTAMP_TOP,
    VK_TIMESTAMP_TYPE_END_RANGE                             = VK_TIMESTAMP_BOTTOM,
    VK_NUM_TIMESTAMP_TYPE                                   = (VK_TIMESTAMP_TYPE_END_RANGE - VK_TIMESTAMP_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_TIMESTEAMP_TYPE)
} VK_TIMESTAMP_TYPE;

typedef enum _VK_BORDER_COLOR_TYPE
{
    VK_BORDER_COLOR_OPAQUE_WHITE                            = 0x00000000,
    VK_BORDER_COLOR_TRANSPARENT_BLACK                       = 0x00000001,
    VK_BORDER_COLOR_OPAQUE_BLACK                            = 0x00000002,

    VK_BORDER_COLOR_TYPE_BEGIN_RANGE                        = VK_BORDER_COLOR_OPAQUE_WHITE,
    VK_BORDER_COLOR_TYPE_END_RANGE                          = VK_BORDER_COLOR_OPAQUE_BLACK,
    VK_NUM_BORDER_COLOR_TYPE                                = (VK_BORDER_COLOR_TYPE_END_RANGE - VK_BORDER_COLOR_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_BORDER_COLOR_TYPE)
} VK_BORDER_COLOR_TYPE;

typedef enum _VK_PIPELINE_BIND_POINT
{
    VK_PIPELINE_BIND_POINT_COMPUTE                          = 0x00000000,
    VK_PIPELINE_BIND_POINT_GRAPHICS                         = 0x00000001,

    VK_PIPELINE_BIND_POINT_BEGIN_RANGE                      = VK_PIPELINE_BIND_POINT_COMPUTE,
    VK_PIPELINE_BIND_POINT_END_RANGE                        = VK_PIPELINE_BIND_POINT_GRAPHICS,
    VK_NUM_PIPELINE_BIND_POINT                              = (VK_PIPELINE_BIND_POINT_END_RANGE - VK_PIPELINE_BIND_POINT_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_PIPELINE_BIND_POINT)
} VK_PIPELINE_BIND_POINT;

typedef enum _VK_STATE_BIND_POINT
{
    VK_STATE_BIND_VIEWPORT                                  = 0x00000000,
    VK_STATE_BIND_RASTER                                    = 0x00000001,
    VK_STATE_BIND_COLOR_BLEND                               = 0x00000002,
    VK_STATE_BIND_DEPTH_STENCIL                             = 0x00000003,

    VK_STATE_BIND_POINT_BEGIN_RANGE                         = VK_STATE_BIND_VIEWPORT,
    VK_STATE_BIND_POINT_END_RANGE                           = VK_STATE_BIND_DEPTH_STENCIL,
    VK_NUM_STATE_BIND_POINT                                 = (VK_STATE_BIND_POINT_END_RANGE - VK_STATE_BIND_POINT_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_STATE_BIND_POINT)
} VK_STATE_BIND_POINT;

typedef enum _VK_PRIMITIVE_TOPOLOGY
{
    VK_TOPOLOGY_POINT_LIST                                  = 0x00000000,
    VK_TOPOLOGY_LINE_LIST                                   = 0x00000001,
    VK_TOPOLOGY_LINE_STRIP                                  = 0x00000002,
    VK_TOPOLOGY_TRIANGLE_LIST                               = 0x00000003,
    VK_TOPOLOGY_TRIANGLE_STRIP                              = 0x00000004,
    VK_TOPOLOGY_TRIANGLE_FAN                                = 0x00000005,
    VK_TOPOLOGY_LINE_LIST_ADJ                               = 0x00000006,
    VK_TOPOLOGY_LINE_STRIP_ADJ                              = 0x00000007,
    VK_TOPOLOGY_TRIANGLE_LIST_ADJ                           = 0x00000008,
    VK_TOPOLOGY_TRIANGLE_STRIP_ADJ                          = 0x00000009,
    VK_TOPOLOGY_PATCH                                       = 0x0000000a,

    VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE                       = VK_TOPOLOGY_POINT_LIST,
    VK_PRIMITIVE_TOPOLOGY_END_RANGE                         = VK_TOPOLOGY_PATCH,
    VK_NUM_PRIMITIVE_TOPOLOGY                               = (VK_PRIMITIVE_TOPOLOGY_END_RANGE - VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_PRIMITIVE_TOPOLOGY)
} VK_PRIMITIVE_TOPOLOGY;

typedef enum _VK_INDEX_TYPE
{
    VK_INDEX_8                                              = 0x00000000,
    VK_INDEX_16                                             = 0x00000001,
    VK_INDEX_32                                             = 0x00000002,

    VK_INDEX_TYPE_BEGIN_RANGE                               = VK_INDEX_8,
    VK_INDEX_TYPE_END_RANGE                                 = VK_INDEX_32,
    VK_NUM_INDEX_TYPE                                       = (VK_INDEX_TYPE_END_RANGE - VK_INDEX_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_INDEX_TYPE)
} VK_INDEX_TYPE;

typedef enum _VK_TEX_FILTER
{
    VK_TEX_FILTER_NEAREST                                   = 0,
    VK_TEX_FILTER_LINEAR                                    = 1,

    VK_TEX_FILTER_BEGIN_RANGE                               = VK_TEX_FILTER_NEAREST,
    VK_TEX_FILTER_END_RANGE                                 = VK_TEX_FILTER_LINEAR,
    VK_NUM_TEX_FILTER                                       = (VK_TEX_FILTER_END_RANGE - VK_TEX_FILTER_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_TEX_FILTER)
} VK_TEX_FILTER;

typedef enum _VK_TEX_MIPMAP_MODE
{
    VK_TEX_MIPMAP_BASE                                      = 0,        // Always choose base level
    VK_TEX_MIPMAP_NEAREST                                   = 1,        // Choose nearest mip level
    VK_TEX_MIPMAP_LINEAR                                    = 2,        // Linear filter between mip levels

    VK_TEX_MIPMAP_BEGIN_RANGE                               = VK_TEX_MIPMAP_BASE,
    VK_TEX_MIPMAP_END_RANGE                                 = VK_TEX_MIPMAP_LINEAR,
    VK_NUM_TEX_MIPMAP                                       = (VK_TEX_MIPMAP_END_RANGE - VK_TEX_MIPMAP_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_TEX_MIPMAP_MODE)
} VK_TEX_MIPMAP_MODE;

typedef enum _VK_TEX_ADDRESS
{
    VK_TEX_ADDRESS_WRAP                                     = 0x00000000,
    VK_TEX_ADDRESS_MIRROR                                   = 0x00000001,
    VK_TEX_ADDRESS_CLAMP                                    = 0x00000002,
    VK_TEX_ADDRESS_MIRROR_ONCE                              = 0x00000003,
    VK_TEX_ADDRESS_CLAMP_BORDER                             = 0x00000004,

    VK_TEX_ADDRESS_BEGIN_RANGE                              = VK_TEX_ADDRESS_WRAP,
    VK_TEX_ADDRESS_END_RANGE                                = VK_TEX_ADDRESS_CLAMP_BORDER,
    VK_NUM_TEX_ADDRESS                                      = (VK_TEX_ADDRESS_END_RANGE - VK_TEX_ADDRESS_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_TEX_ADDRESS)
} VK_TEX_ADDRESS;

typedef enum _VK_COMPARE_FUNC
{
    VK_COMPARE_NEVER                                        = 0x00000000,
    VK_COMPARE_LESS                                         = 0x00000001,
    VK_COMPARE_EQUAL                                        = 0x00000002,
    VK_COMPARE_LESS_EQUAL                                   = 0x00000003,
    VK_COMPARE_GREATER                                      = 0x00000004,
    VK_COMPARE_NOT_EQUAL                                    = 0x00000005,
    VK_COMPARE_GREATER_EQUAL                                = 0x00000006,
    VK_COMPARE_ALWAYS                                       = 0x00000007,

    VK_COMPARE_FUNC_BEGIN_RANGE                             = VK_COMPARE_NEVER,
    VK_COMPARE_FUNC_END_RANGE                               = VK_COMPARE_ALWAYS,
    VK_NUM_COMPARE_FUNC                                     = (VK_COMPARE_FUNC_END_RANGE - VK_COMPARE_FUNC_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_COMPARE_FUNC)
} VK_COMPARE_FUNC;

typedef enum _VK_FILL_MODE
{
    VK_FILL_POINTS                                          = 0x00000000,
    VK_FILL_WIREFRAME                                       = 0x00000001,
    VK_FILL_SOLID                                           = 0x00000002,

    VK_FILL_MODE_BEGIN_RANGE                                = VK_FILL_POINTS,
    VK_FILL_MODE_END_RANGE                                  = VK_FILL_SOLID,
    VK_NUM_FILL_MODE                                        = (VK_FILL_MODE_END_RANGE - VK_FILL_MODE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_FILL_MODE)
} VK_FILL_MODE;

typedef enum _VK_CULL_MODE
{
    VK_CULL_NONE                                            = 0x00000000,
    VK_CULL_FRONT                                           = 0x00000001,
    VK_CULL_BACK                                            = 0x00000002,
    VK_CULL_FRONT_AND_BACK                                  = 0x00000003,

    VK_CULL_MODE_BEGIN_RANGE                                = VK_CULL_NONE,
    VK_CULL_MODE_END_RANGE                                  = VK_CULL_FRONT_AND_BACK,
    VK_NUM_CULL_MODE                                        = (VK_CULL_MODE_END_RANGE - VK_CULL_MODE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_CULL_MODE)
} VK_CULL_MODE;

typedef enum _VK_FACE_ORIENTATION
{
    VK_FRONT_FACE_CCW                                       = 0x00000000,
    VK_FRONT_FACE_CW                                        = 0x00000001,

    VK_FACE_ORIENTATION_BEGIN_RANGE                         = VK_FRONT_FACE_CCW,
    VK_FACE_ORIENTATION_END_RANGE                           = VK_FRONT_FACE_CW,
    VK_NUM_FACE_ORIENTATION                                 = (VK_FACE_ORIENTATION_END_RANGE - VK_FACE_ORIENTATION_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_FACE_ORIENTATION)
} VK_FACE_ORIENTATION;

typedef enum _VK_PROVOKING_VERTEX_CONVENTION
{
    VK_PROVOKING_VERTEX_FIRST                               = 0x00000000,
    VK_PROVOKING_VERTEX_LAST                                = 0x00000001,

    VK_PROVOKING_VERTEX_BEGIN_RANGE                         = VK_PROVOKING_VERTEX_FIRST,
    VK_PROVOKING_VERTEX_END_RANGE                           = VK_PROVOKING_VERTEX_LAST,
    VK_NUM_PROVOKING_VERTEX_CONVENTION                      = (VK_PROVOKING_VERTEX_END_RANGE - VK_PROVOKING_VERTEX_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_PROVOKING_VERTEX_CONVENTION)
} VK_PROVOKING_VERTEX_CONVENTION;

typedef enum _VK_COORDINATE_ORIGIN
{
    VK_COORDINATE_ORIGIN_UPPER_LEFT                         = 0x00000000,
    VK_COORDINATE_ORIGIN_LOWER_LEFT                         = 0x00000001,

    VK_COORDINATE_ORIGIN_BEGIN_RANGE                        = VK_COORDINATE_ORIGIN_UPPER_LEFT,
    VK_COORDINATE_ORIGIN_END_RANGE                          = VK_COORDINATE_ORIGIN_LOWER_LEFT,
    VK_NUM_COORDINATE_ORIGIN                                = (VK_COORDINATE_ORIGIN_END_RANGE - VK_COORDINATE_ORIGIN_END_RANGE + 1),
    VK_MAX_ENUM(_VK_COORDINATE_ORIGIN)
} VK_COORDINATE_ORIGIN;

typedef enum _VK_DEPTH_MODE
{
    VK_DEPTH_MODE_ZERO_TO_ONE                               = 0x00000000,
    VK_DEPTH_MODE_NEGATIVE_ONE_TO_ONE                       = 0x00000001,

    VK_DEPTH_MODE_BEGIN_RANGE                               = VK_DEPTH_MODE_ZERO_TO_ONE,
    VK_DEPTH_MODE_END_RANGE                                 = VK_DEPTH_MODE_NEGATIVE_ONE_TO_ONE,
    VK_NUM_DEPTH_MODE                                       = (VK_DEPTH_MODE_END_RANGE - VK_DEPTH_MODE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_DEPTH_MODE)
} VK_DEPTH_MODE;

typedef enum _VK_BLEND
{
    VK_BLEND_ZERO                                           = 0x00000000,
    VK_BLEND_ONE                                            = 0x00000001,
    VK_BLEND_SRC_COLOR                                      = 0x00000002,
    VK_BLEND_ONE_MINUS_SRC_COLOR                            = 0x00000003,
    VK_BLEND_DEST_COLOR                                     = 0x00000004,
    VK_BLEND_ONE_MINUS_DEST_COLOR                           = 0x00000005,
    VK_BLEND_SRC_ALPHA                                      = 0x00000006,
    VK_BLEND_ONE_MINUS_SRC_ALPHA                            = 0x00000007,
    VK_BLEND_DEST_ALPHA                                     = 0x00000008,
    VK_BLEND_ONE_MINUS_DEST_ALPHA                           = 0x00000009,
    VK_BLEND_CONSTANT_COLOR                                 = 0x0000000a,
    VK_BLEND_ONE_MINUS_CONSTANT_COLOR                       = 0x0000000b,
    VK_BLEND_CONSTANT_ALPHA                                 = 0x0000000c,
    VK_BLEND_ONE_MINUS_CONSTANT_ALPHA                       = 0x0000000d,
    VK_BLEND_SRC_ALPHA_SATURATE                             = 0x0000000e,
    VK_BLEND_SRC1_COLOR                                     = 0x0000000f,
    VK_BLEND_ONE_MINUS_SRC1_COLOR                           = 0x00000010,
    VK_BLEND_SRC1_ALPHA                                     = 0x00000011,
    VK_BLEND_ONE_MINUS_SRC1_ALPHA                           = 0x00000012,

    VK_BLEND_BEGIN_RANGE                                    = VK_BLEND_ZERO,
    VK_BLEND_END_RANGE                                      = VK_BLEND_ONE_MINUS_SRC1_ALPHA,
    VK_NUM_BLEND                                            = (VK_BLEND_END_RANGE - VK_BLEND_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_BLEND)
} VK_BLEND;

typedef enum _VK_BLEND_FUNC
{
    VK_BLEND_FUNC_ADD                                       = 0x00000000,
    VK_BLEND_FUNC_SUBTRACT                                  = 0x00000001,
    VK_BLEND_FUNC_REVERSE_SUBTRACT                          = 0x00000002,
    VK_BLEND_FUNC_MIN                                       = 0x00000003,
    VK_BLEND_FUNC_MAX                                       = 0x00000004,

    VK_BLEND_FUNC_BEGIN_RANGE                               = VK_BLEND_FUNC_ADD,
    VK_BLEND_FUNC_END_RANGE                                 = VK_BLEND_FUNC_MAX,
    VK_NUM_BLEND_FUNC                                       = (VK_BLEND_FUNC_END_RANGE - VK_BLEND_FUNC_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_BLEND_FUNC)
} VK_BLEND_FUNC;

typedef enum _VK_STENCIL_OP
{
    VK_STENCIL_OP_KEEP                                      = 0x00000000,
    VK_STENCIL_OP_ZERO                                      = 0x00000001,
    VK_STENCIL_OP_REPLACE                                   = 0x00000002,
    VK_STENCIL_OP_INC_CLAMP                                 = 0x00000003,
    VK_STENCIL_OP_DEC_CLAMP                                 = 0x00000004,
    VK_STENCIL_OP_INVERT                                    = 0x00000005,
    VK_STENCIL_OP_INC_WRAP                                  = 0x00000006,
    VK_STENCIL_OP_DEC_WRAP                                  = 0x00000007,

    VK_STENCIL_OP_BEGIN_RANGE                               = VK_STENCIL_OP_KEEP,
    VK_STENCIL_OP_END_RANGE                                 = VK_STENCIL_OP_DEC_WRAP,
    VK_NUM_STENCIL_OP                                       = (VK_STENCIL_OP_END_RANGE - VK_STENCIL_OP_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_STENCIL_OP)
} VK_STENCIL_OP;

typedef enum _VK_LOGIC_OP
{
    VK_LOGIC_OP_COPY                                        = 0x00000000,
    VK_LOGIC_OP_CLEAR                                       = 0x00000001,
    VK_LOGIC_OP_AND                                         = 0x00000002,
    VK_LOGIC_OP_AND_REVERSE                                 = 0x00000003,
    VK_LOGIC_OP_AND_INVERTED                                = 0x00000004,
    VK_LOGIC_OP_NOOP                                        = 0x00000005,
    VK_LOGIC_OP_XOR                                         = 0x00000006,
    VK_LOGIC_OP_OR                                          = 0x00000007,
    VK_LOGIC_OP_NOR                                         = 0x00000008,
    VK_LOGIC_OP_EQUIV                                       = 0x00000009,
    VK_LOGIC_OP_INVERT                                      = 0x0000000a,
    VK_LOGIC_OP_OR_REVERSE                                  = 0x0000000b,
    VK_LOGIC_OP_COPY_INVERTED                               = 0x0000000c,
    VK_LOGIC_OP_OR_INVERTED                                 = 0x0000000d,
    VK_LOGIC_OP_NAND                                        = 0x0000000e,
    VK_LOGIC_OP_SET                                         = 0x0000000f,

    VK_LOGIC_OP_BEGIN_RANGE                                 = VK_LOGIC_OP_COPY,
    VK_LOGIC_OP_END_RANGE                                   = VK_LOGIC_OP_SET,
    VK_NUM_LOGIC_OP                                         = (VK_LOGIC_OP_END_RANGE - VK_LOGIC_OP_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_LOGIC_OP)
} VK_LOGIC_OP;

typedef enum _VK_SYSTEM_ALLOC_TYPE
{
    VK_SYSTEM_ALLOC_API_OBJECT                              = 0x00000000,
    VK_SYSTEM_ALLOC_INTERNAL                                = 0x00000001,
    VK_SYSTEM_ALLOC_INTERNAL_TEMP                           = 0x00000002,
    VK_SYSTEM_ALLOC_INTERNAL_SHADER                         = 0x00000003,
    VK_SYSTEM_ALLOC_DEBUG                                   = 0x00000004,

    VK_SYSTEM_ALLOC_BEGIN_RANGE                             = VK_SYSTEM_ALLOC_API_OBJECT,
    VK_SYSTEM_ALLOC_END_RANGE                               = VK_SYSTEM_ALLOC_DEBUG,
    VK_NUM_SYSTEM_ALLOC_TYPE                                = (VK_SYSTEM_ALLOC_END_RANGE - VK_SYSTEM_ALLOC_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_SYSTEM_ALLOC_TYPE)
} VK_SYSTEM_ALLOC_TYPE;

typedef enum _VK_PHYSICAL_GPU_TYPE
{
    VK_GPU_TYPE_OTHER                                       = 0x00000000,
    VK_GPU_TYPE_INTEGRATED                                  = 0x00000001,
    VK_GPU_TYPE_DISCRETE                                    = 0x00000002,
    VK_GPU_TYPE_VIRTUAL                                     = 0x00000003,

    VK_PHYSICAL_GPU_TYPE_BEGIN_RANGE                        = VK_GPU_TYPE_OTHER,
    VK_PHYSICAL_GPU_TYPE_END_RANGE                          = VK_GPU_TYPE_VIRTUAL,
    VK_NUM_PHYSICAL_GPU_TYPE                                = (VK_PHYSICAL_GPU_TYPE_END_RANGE - VK_PHYSICAL_GPU_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_PHYSICAL_GPU_TYPE)
} VK_PHYSICAL_GPU_TYPE;

typedef enum _VK_PHYSICAL_GPU_INFO_TYPE
{
    // Info type for vkGetGpuInfo()
    VK_INFO_TYPE_PHYSICAL_GPU_PROPERTIES                    = 0x00000000,
    VK_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE                   = 0x00000001,
    VK_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES              = 0x00000002,
    VK_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES             = 0x00000003,

    VK_INFO_TYPE_PHYSICAL_GPU_BEGIN_RANGE                   = VK_INFO_TYPE_PHYSICAL_GPU_PROPERTIES,
    VK_INFO_TYPE_PHYSICAL_GPU_END_RANGE                     = VK_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES,
    VK_NUM_INFO_TYPE_PHYSICAL_GPU                           = (VK_INFO_TYPE_PHYSICAL_GPU_END_RANGE - VK_INFO_TYPE_PHYSICAL_GPU_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_PHYSICAL_GPU_INFO_TYPE)
} VK_PHYSICAL_GPU_INFO_TYPE;

typedef enum _VK_FORMAT_INFO_TYPE
{
    // Info type for vkGetFormatInfo()
    VK_INFO_TYPE_FORMAT_PROPERTIES                          = 0x00000000,

    VK_INFO_TYPE_FORMAT_BEGIN_RANGE                         = VK_INFO_TYPE_FORMAT_PROPERTIES,
    VK_INFO_TYPE_FORMAT_END_RANGE                           = VK_INFO_TYPE_FORMAT_PROPERTIES,
    VK_NUM_INFO_TYPE_FORMAT                                  = (VK_INFO_TYPE_FORMAT_END_RANGE - VK_INFO_TYPE_FORMAT_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_FORMAT_INFO_TYPE)
} VK_FORMAT_INFO_TYPE;

typedef enum _VK_SUBRESOURCE_INFO_TYPE
{
    // Info type for vkGetImageSubresourceInfo()
    VK_INFO_TYPE_SUBRESOURCE_LAYOUT                         = 0x00000000,

    VK_INFO_TYPE_SUBRESOURCE_BEGIN_RANGE                    = VK_INFO_TYPE_SUBRESOURCE_LAYOUT,
    VK_INFO_TYPE_SUBRESOURCE_END_RANGE                      = VK_INFO_TYPE_SUBRESOURCE_LAYOUT,
    VK_NUM_INFO_TYPE_SUBRESOURCE                            = (VK_INFO_TYPE_SUBRESOURCE_END_RANGE - VK_INFO_TYPE_SUBRESOURCE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_SUBRESOURCE_INFO_TYPE)
} VK_SUBRESOURCE_INFO_TYPE;

typedef enum _VK_OBJECT_INFO_TYPE
{
    // Info type for vkGetObjectInfo()
    VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT                    = 0x00000000,
    VK_INFO_TYPE_MEMORY_REQUIREMENTS                        = 0x00000001,
    VK_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS                 = 0x00000002,
    VK_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS                  = 0x00000003,

    VK_INFO_TYPE_BEGIN_RANGE                                = VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
    VK_INFO_TYPE_END_RANGE                                  = VK_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS,
    VK_NUM_INFO_TYPE                                        = (VK_INFO_TYPE_END_RANGE - VK_INFO_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_OBJECT_INFO_TYPE)
} VK_OBJECT_INFO_TYPE;

typedef enum _VK_VALIDATION_LEVEL
{
    VK_VALIDATION_LEVEL_0                                   = 0x00000000,
    VK_VALIDATION_LEVEL_1                                   = 0x00000001,
    VK_VALIDATION_LEVEL_2                                   = 0x00000002,
    VK_VALIDATION_LEVEL_3                                   = 0x00000003,
    VK_VALIDATION_LEVEL_4                                   = 0x00000004,

    VK_VALIDATION_LEVEL_BEGIN_RANGE                         = VK_VALIDATION_LEVEL_0,
    VK_VALIDATION_LEVEL_END_RANGE                           = VK_VALIDATION_LEVEL_4,
    VK_NUM_VALIDATION_LEVEL                                 = (VK_VALIDATION_LEVEL_END_RANGE - VK_VALIDATION_LEVEL_BEGIN_RANGE + 1),

    VK_MAX_ENUM(_VK_VALIDATION_LEVEL)
} VK_VALIDATION_LEVEL;

// ------------------------------------------------------------------------------------------------
// Error and return codes

typedef enum _VK_RESULT
{
    // Return codes for successful operation execution (> = 0)
    VK_SUCCESS                                              = 0x0000000,
    VK_UNSUPPORTED                                          = 0x0000001,
    VK_NOT_READY                                            = 0x0000002,
    VK_TIMEOUT                                              = 0x0000003,
    VK_EVENT_SET                                            = 0x0000004,
    VK_EVENT_RESET                                          = 0x0000005,

    // Error codes (negative values)
    VK_ERROR_UNKNOWN                                        = -(0x00000001),
    VK_ERROR_UNAVAILABLE                                    = -(0x00000002),
    VK_ERROR_INITIALIZATION_FAILED                          = -(0x00000003),
    VK_ERROR_OUT_OF_MEMORY                                  = -(0x00000004),
    VK_ERROR_OUT_OF_GPU_MEMORY                              = -(0x00000005),
    VK_ERROR_DEVICE_ALREADY_CREATED                         = -(0x00000006),
    VK_ERROR_DEVICE_LOST                                    = -(0x00000007),
    VK_ERROR_INVALID_POINTER                                = -(0x00000008),
    VK_ERROR_INVALID_VALUE                                  = -(0x00000009),
    VK_ERROR_INVALID_HANDLE                                 = -(0x0000000A),
    VK_ERROR_INVALID_ORDINAL                                = -(0x0000000B),
    VK_ERROR_INVALID_MEMORY_SIZE                            = -(0x0000000C),
    VK_ERROR_INVALID_EXTENSION                              = -(0x0000000D),
    VK_ERROR_INVALID_FLAGS                                  = -(0x0000000E),
    VK_ERROR_INVALID_ALIGNMENT                              = -(0x0000000F),
    VK_ERROR_INVALID_FORMAT                                 = -(0x00000010),
    VK_ERROR_INVALID_IMAGE                                  = -(0x00000011),
    VK_ERROR_INVALID_DESCRIPTOR_SET_DATA                    = -(0x00000012),
    VK_ERROR_INVALID_QUEUE_TYPE                             = -(0x00000013),
    VK_ERROR_INVALID_OBJECT_TYPE                            = -(0x00000014),
    VK_ERROR_UNSUPPORTED_SHADER_IL_VERSION                  = -(0x00000015),
    VK_ERROR_BAD_SHADER_CODE                                = -(0x00000016),
    VK_ERROR_BAD_PIPELINE_DATA                              = -(0x00000017),
    VK_ERROR_TOO_MANY_MEMORY_REFERENCES                     = -(0x00000018),
    VK_ERROR_NOT_MAPPABLE                                   = -(0x00000019),
    VK_ERROR_MEMORY_MAP_FAILED                              = -(0x0000001A),
    VK_ERROR_MEMORY_UNMAP_FAILED                            = -(0x0000001B),
    VK_ERROR_INCOMPATIBLE_DEVICE                            = -(0x0000001C),
    VK_ERROR_INCOMPATIBLE_DRIVER                            = -(0x0000001D),
    VK_ERROR_INCOMPLETE_COMMAND_BUFFER                      = -(0x0000001E),
    VK_ERROR_BUILDING_COMMAND_BUFFER                        = -(0x0000001F),
    VK_ERROR_MEMORY_NOT_BOUND                               = -(0x00000020),
    VK_ERROR_INCOMPATIBLE_QUEUE                             = -(0x00000021),
    VK_ERROR_NOT_SHAREABLE                                  = -(0x00000022),
    VK_MAX_ENUM(_VK_RESULT_CODE)
} VK_RESULT;

// ------------------------------------------------------------------------------------------------
// Vulkan format definitions

typedef enum _VK_VERTEX_INPUT_STEP_RATE
{
    VK_VERTEX_INPUT_STEP_RATE_VERTEX                        = 0x0,
    VK_VERTEX_INPUT_STEP_RATE_INSTANCE                      = 0x1,
    VK_VERTEX_INPUT_STEP_RATE_DRAW                          = 0x2,  //Optional

    VK_VERTEX_INPUT_STEP_RATE_BEGIN_RANGE                   = VK_VERTEX_INPUT_STEP_RATE_VERTEX,
    VK_VERTEX_INPUT_STEP_RATE_END_RANGE                     = VK_VERTEX_INPUT_STEP_RATE_DRAW,
    VK_NUM_VERTEX_INPUT_STEP_RATE                           = (VK_VERTEX_INPUT_STEP_RATE_END_RANGE - VK_VERTEX_INPUT_STEP_RATE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_VERTEX_INPUT_STEP_RATE)
} VK_VERTEX_INPUT_STEP_RATE;

typedef enum _VK_FORMAT
{
    VK_FMT_UNDEFINED                                        = 0x00000000,
    VK_FMT_R4G4_UNORM                                       = 0x00000001,
    VK_FMT_R4G4_USCALED                                     = 0x00000002,
    VK_FMT_R4G4B4A4_UNORM                                   = 0x00000003,
    VK_FMT_R4G4B4A4_USCALED                                 = 0x00000004,
    VK_FMT_R5G6B5_UNORM                                     = 0x00000005,
    VK_FMT_R5G6B5_USCALED                                   = 0x00000006,
    VK_FMT_R5G5B5A1_UNORM                                   = 0x00000007,
    VK_FMT_R5G5B5A1_USCALED                                 = 0x00000008,
    VK_FMT_R8_UNORM                                         = 0x00000009,
    VK_FMT_R8_SNORM                                         = 0x0000000A,
    VK_FMT_R8_USCALED                                       = 0x0000000B,
    VK_FMT_R8_SSCALED                                       = 0x0000000C,
    VK_FMT_R8_UINT                                          = 0x0000000D,
    VK_FMT_R8_SINT                                          = 0x0000000E,
    VK_FMT_R8_SRGB                                          = 0x0000000F,
    VK_FMT_R8G8_UNORM                                       = 0x00000010,
    VK_FMT_R8G8_SNORM                                       = 0x00000011,
    VK_FMT_R8G8_USCALED                                     = 0x00000012,
    VK_FMT_R8G8_SSCALED                                     = 0x00000013,
    VK_FMT_R8G8_UINT                                        = 0x00000014,
    VK_FMT_R8G8_SINT                                        = 0x00000015,
    VK_FMT_R8G8_SRGB                                        = 0x00000016,
    VK_FMT_R8G8B8_UNORM                                     = 0x00000017,
    VK_FMT_R8G8B8_SNORM                                     = 0x00000018,
    VK_FMT_R8G8B8_USCALED                                   = 0x00000019,
    VK_FMT_R8G8B8_SSCALED                                   = 0x0000001A,
    VK_FMT_R8G8B8_UINT                                      = 0x0000001B,
    VK_FMT_R8G8B8_SINT                                      = 0x0000001C,
    VK_FMT_R8G8B8_SRGB                                      = 0x0000001D,
    VK_FMT_R8G8B8A8_UNORM                                   = 0x0000001E,
    VK_FMT_R8G8B8A8_SNORM                                   = 0x0000001F,
    VK_FMT_R8G8B8A8_USCALED                                 = 0x00000020,
    VK_FMT_R8G8B8A8_SSCALED                                 = 0x00000021,
    VK_FMT_R8G8B8A8_UINT                                    = 0x00000022,
    VK_FMT_R8G8B8A8_SINT                                    = 0x00000023,
    VK_FMT_R8G8B8A8_SRGB                                    = 0x00000024,
    VK_FMT_R10G10B10A2_UNORM                                = 0x00000025,
    VK_FMT_R10G10B10A2_SNORM                                = 0x00000026,
    VK_FMT_R10G10B10A2_USCALED                              = 0x00000027,
    VK_FMT_R10G10B10A2_SSCALED                              = 0x00000028,
    VK_FMT_R10G10B10A2_UINT                                 = 0x00000029,
    VK_FMT_R10G10B10A2_SINT                                 = 0x0000002A,
    VK_FMT_R16_UNORM                                        = 0x0000002B,
    VK_FMT_R16_SNORM                                        = 0x0000002C,
    VK_FMT_R16_USCALED                                      = 0x0000002D,
    VK_FMT_R16_SSCALED                                      = 0x0000002E,
    VK_FMT_R16_UINT                                         = 0x0000002F,
    VK_FMT_R16_SINT                                         = 0x00000030,
    VK_FMT_R16_SFLOAT                                       = 0x00000031,
    VK_FMT_R16G16_UNORM                                     = 0x00000032,
    VK_FMT_R16G16_SNORM                                     = 0x00000033,
    VK_FMT_R16G16_USCALED                                   = 0x00000034,
    VK_FMT_R16G16_SSCALED                                   = 0x00000035,
    VK_FMT_R16G16_UINT                                      = 0x00000036,
    VK_FMT_R16G16_SINT                                      = 0x00000037,
    VK_FMT_R16G16_SFLOAT                                    = 0x00000038,
    VK_FMT_R16G16B16_UNORM                                  = 0x00000039,
    VK_FMT_R16G16B16_SNORM                                  = 0x0000003A,
    VK_FMT_R16G16B16_USCALED                                = 0x0000003B,
    VK_FMT_R16G16B16_SSCALED                                = 0x0000003C,
    VK_FMT_R16G16B16_UINT                                   = 0x0000003D,
    VK_FMT_R16G16B16_SINT                                   = 0x0000003E,
    VK_FMT_R16G16B16_SFLOAT                                 = 0x0000003F,
    VK_FMT_R16G16B16A16_UNORM                               = 0x00000040,
    VK_FMT_R16G16B16A16_SNORM                               = 0x00000041,
    VK_FMT_R16G16B16A16_USCALED                             = 0x00000042,
    VK_FMT_R16G16B16A16_SSCALED                             = 0x00000043,
    VK_FMT_R16G16B16A16_UINT                                = 0x00000044,
    VK_FMT_R16G16B16A16_SINT                                = 0x00000045,
    VK_FMT_R16G16B16A16_SFLOAT                              = 0x00000046,
    VK_FMT_R32_UINT                                         = 0x00000047,
    VK_FMT_R32_SINT                                         = 0x00000048,
    VK_FMT_R32_SFLOAT                                       = 0x00000049,
    VK_FMT_R32G32_UINT                                      = 0x0000004A,
    VK_FMT_R32G32_SINT                                      = 0x0000004B,
    VK_FMT_R32G32_SFLOAT                                    = 0x0000004C,
    VK_FMT_R32G32B32_UINT                                   = 0x0000004D,
    VK_FMT_R32G32B32_SINT                                   = 0x0000004E,
    VK_FMT_R32G32B32_SFLOAT                                 = 0x0000004F,
    VK_FMT_R32G32B32A32_UINT                                = 0x00000050,
    VK_FMT_R32G32B32A32_SINT                                = 0x00000051,
    VK_FMT_R32G32B32A32_SFLOAT                              = 0x00000052,
    VK_FMT_R64_SFLOAT                                       = 0x00000053,
    VK_FMT_R64G64_SFLOAT                                    = 0x00000054,
    VK_FMT_R64G64B64_SFLOAT                                 = 0x00000055,
    VK_FMT_R64G64B64A64_SFLOAT                              = 0x00000056,
    VK_FMT_R11G11B10_UFLOAT                                 = 0x00000057,
    VK_FMT_R9G9B9E5_UFLOAT                                  = 0x00000058,
    VK_FMT_D16_UNORM                                        = 0x00000059,
    VK_FMT_D24_UNORM                                        = 0x0000005A,
    VK_FMT_D32_SFLOAT                                       = 0x0000005B,
    VK_FMT_S8_UINT                                          = 0x0000005C,
    VK_FMT_D16_UNORM_S8_UINT                                = 0x0000005D,
    VK_FMT_D24_UNORM_S8_UINT                                = 0x0000005E,
    VK_FMT_D32_SFLOAT_S8_UINT                               = 0x0000005F,
    VK_FMT_BC1_RGB_UNORM                                    = 0x00000060,
    VK_FMT_BC1_RGB_SRGB                                     = 0x00000061,
    VK_FMT_BC1_RGBA_UNORM                                   = 0x00000062,
    VK_FMT_BC1_RGBA_SRGB                                    = 0x00000063,
    VK_FMT_BC2_UNORM                                        = 0x00000064,
    VK_FMT_BC2_SRGB                                         = 0x00000065,
    VK_FMT_BC3_UNORM                                        = 0x00000066,
    VK_FMT_BC3_SRGB                                         = 0x00000067,
    VK_FMT_BC4_UNORM                                        = 0x00000068,
    VK_FMT_BC4_SNORM                                        = 0x00000069,
    VK_FMT_BC5_UNORM                                        = 0x0000006A,
    VK_FMT_BC5_SNORM                                        = 0x0000006B,
    VK_FMT_BC6H_UFLOAT                                      = 0x0000006C,
    VK_FMT_BC6H_SFLOAT                                      = 0x0000006D,
    VK_FMT_BC7_UNORM                                        = 0x0000006E,
    VK_FMT_BC7_SRGB                                         = 0x0000006F,
    VK_FMT_ETC2_R8G8B8_UNORM                                = 0x00000070,
    VK_FMT_ETC2_R8G8B8_SRGB                                 = 0x00000071,
    VK_FMT_ETC2_R8G8B8A1_UNORM                              = 0x00000072,
    VK_FMT_ETC2_R8G8B8A1_SRGB                               = 0x00000073,
    VK_FMT_ETC2_R8G8B8A8_UNORM                              = 0x00000074,
    VK_FMT_ETC2_R8G8B8A8_SRGB                               = 0x00000075,
    VK_FMT_EAC_R11_UNORM                                    = 0x00000076,
    VK_FMT_EAC_R11_SNORM                                    = 0x00000077,
    VK_FMT_EAC_R11G11_UNORM                                 = 0x00000078,
    VK_FMT_EAC_R11G11_SNORM                                 = 0x00000079,
    VK_FMT_ASTC_4x4_UNORM                                   = 0x0000007A,
    VK_FMT_ASTC_4x4_SRGB                                    = 0x0000007B,
    VK_FMT_ASTC_5x4_UNORM                                   = 0x0000007C,
    VK_FMT_ASTC_5x4_SRGB                                    = 0x0000007D,
    VK_FMT_ASTC_5x5_UNORM                                   = 0x0000007E,
    VK_FMT_ASTC_5x5_SRGB                                    = 0x0000007F,
    VK_FMT_ASTC_6x5_UNORM                                   = 0x00000080,
    VK_FMT_ASTC_6x5_SRGB                                    = 0x00000081,
    VK_FMT_ASTC_6x6_UNORM                                   = 0x00000082,
    VK_FMT_ASTC_6x6_SRGB                                    = 0x00000083,
    VK_FMT_ASTC_8x5_UNORM                                   = 0x00000084,
    VK_FMT_ASTC_8x5_SRGB                                    = 0x00000085,
    VK_FMT_ASTC_8x6_UNORM                                   = 0x00000086,
    VK_FMT_ASTC_8x6_SRGB                                    = 0x00000087,
    VK_FMT_ASTC_8x8_UNORM                                   = 0x00000088,
    VK_FMT_ASTC_8x8_SRGB                                    = 0x00000089,
    VK_FMT_ASTC_10x5_UNORM                                  = 0x0000008A,
    VK_FMT_ASTC_10x5_SRGB                                   = 0x0000008B,
    VK_FMT_ASTC_10x6_UNORM                                  = 0x0000008C,
    VK_FMT_ASTC_10x6_SRGB                                   = 0x0000008D,
    VK_FMT_ASTC_10x8_UNORM                                  = 0x0000008E,
    VK_FMT_ASTC_10x8_SRGB                                   = 0x0000008F,
    VK_FMT_ASTC_10x10_UNORM                                 = 0x00000090,
    VK_FMT_ASTC_10x10_SRGB                                  = 0x00000091,
    VK_FMT_ASTC_12x10_UNORM                                 = 0x00000092,
    VK_FMT_ASTC_12x10_SRGB                                  = 0x00000093,
    VK_FMT_ASTC_12x12_UNORM                                 = 0x00000094,
    VK_FMT_ASTC_12x12_SRGB                                  = 0x00000095,
    VK_FMT_B4G4R4A4_UNORM                                   = 0x00000096,
    VK_FMT_B5G5R5A1_UNORM                                   = 0x00000097,
    VK_FMT_B5G6R5_UNORM                                     = 0x00000098,
    VK_FMT_B5G6R5_USCALED                                   = 0x00000099,
    VK_FMT_B8G8R8_UNORM                                     = 0x0000009A,
    VK_FMT_B8G8R8_SNORM                                     = 0x0000009B,
    VK_FMT_B8G8R8_USCALED                                   = 0x0000009C,
    VK_FMT_B8G8R8_SSCALED                                   = 0x0000009D,
    VK_FMT_B8G8R8_UINT                                      = 0x0000009E,
    VK_FMT_B8G8R8_SINT                                      = 0x0000009F,
    VK_FMT_B8G8R8_SRGB                                      = 0x000000A0,
    VK_FMT_B8G8R8A8_UNORM                                   = 0x000000A1,
    VK_FMT_B8G8R8A8_SNORM                                   = 0x000000A2,
    VK_FMT_B8G8R8A8_USCALED                                 = 0x000000A3,
    VK_FMT_B8G8R8A8_SSCALED                                 = 0x000000A4,
    VK_FMT_B8G8R8A8_UINT                                    = 0x000000A5,
    VK_FMT_B8G8R8A8_SINT                                    = 0x000000A6,
    VK_FMT_B8G8R8A8_SRGB                                    = 0x000000A7,
    VK_FMT_B10G10R10A2_UNORM                                = 0x000000A8,
    VK_FMT_B10G10R10A2_SNORM                                = 0x000000A9,
    VK_FMT_B10G10R10A2_USCALED                              = 0x000000AA,
    VK_FMT_B10G10R10A2_SSCALED                              = 0x000000AB,
    VK_FMT_B10G10R10A2_UINT                                 = 0x000000AC,
    VK_FMT_B10G10R10A2_SINT                                 = 0x000000AD,

    VK_FMT_BEGIN_RANGE                                      = VK_FMT_UNDEFINED,
    VK_FMT_END_RANGE                                        = VK_FMT_B10G10R10A2_SINT,
    VK_NUM_FMT                                              = (VK_FMT_END_RANGE - VK_FMT_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_FORMAT)
} VK_FORMAT;

// Shader stage enumerant
typedef enum _VK_PIPELINE_SHADER_STAGE
{
    VK_SHADER_STAGE_VERTEX                                  = 0,
    VK_SHADER_STAGE_TESS_CONTROL                            = 1,
    VK_SHADER_STAGE_TESS_EVALUATION                         = 2,
    VK_SHADER_STAGE_GEOMETRY                                = 3,
    VK_SHADER_STAGE_FRAGMENT                                = 4,
    VK_SHADER_STAGE_COMPUTE                                 = 5,

    VK_SHADER_STAGE_BEGIN_RANGE                             = VK_SHADER_STAGE_VERTEX,
    VK_SHADER_STAGE_END_RANGE                               = VK_SHADER_STAGE_COMPUTE,
    VK_NUM_SHADER_STAGE                                     = (VK_SHADER_STAGE_END_RANGE - VK_SHADER_STAGE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_PIPELINE_SHADER_STAGE)
} VK_PIPELINE_SHADER_STAGE;

typedef enum _VK_SHADER_STAGE_FLAGS
{
    VK_SHADER_STAGE_FLAGS_VERTEX_BIT                         = 0x00000001,
    VK_SHADER_STAGE_FLAGS_TESS_CONTROL_BIT                   = 0x00000002,
    VK_SHADER_STAGE_FLAGS_TESS_EVALUATION_BIT                = 0x00000004,
    VK_SHADER_STAGE_FLAGS_GEOMETRY_BIT                       = 0x00000008,
    VK_SHADER_STAGE_FLAGS_FRAGMENT_BIT                       = 0x00000010,
    VK_SHADER_STAGE_FLAGS_COMPUTE_BIT                        = 0x00000020,

    VK_SHADER_STAGE_FLAGS_ALL                                = 0x7FFFFFFF,
    VK_MAX_ENUM(_VK_SHADER_STAGE_FLAGS)
} VK_SHADER_STAGE_FLAGS;

// Structure type enumerant
typedef enum _VK_STRUCTURE_TYPE
{
    VK_STRUCTURE_TYPE_APPLICATION_INFO                      = 0,
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO                    = 1,
    VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO                     = 2,
    VK_STRUCTURE_TYPE_MEMORY_OPEN_INFO                      = 4,
    VK_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO                 = 5,
    VK_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO               = 6,
    VK_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO                = 7,
    VK_STRUCTURE_TYPE_EVENT_WAIT_INFO                       = 8,
    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO                = 9,
    VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO     = 10,
    VK_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO        = 11,
    VK_STRUCTURE_TYPE_SHADER_CREATE_INFO                    = 12,
    VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO          = 13,
    VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO                   = 14,
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO     = 15,
    VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO          = 16,
    VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO          = 17,
    VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO          = 18,
    VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO          = 19,
    VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO                = 20,
    VK_STRUCTURE_TYPE_EVENT_CREATE_INFO                     = 21,
    VK_STRUCTURE_TYPE_FENCE_CREATE_INFO                     = 22,
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO                 = 23,
    VK_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO                   = 24,
    VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO                = 25,
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO     = 26,
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO         = 27,
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO     = 28,
    VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO         = 29,
    VK_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO       = 30,
    VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO         = 31,
    VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO         = 32,
    VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO         = 33,
    VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO         = 34,
    VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO         = 35,
    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO                     = 36,
    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO                    = 37,
    VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO               = 38,
    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO               = 39,
    VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO                 = 40,
    VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO        = 41,
    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO               = 42,
    VK_STRUCTURE_TYPE_LAYER_CREATE_INFO                     = 43,
    VK_STRUCTURE_TYPE_PIPELINE_BARRIER                      = 44,
    VK_STRUCTURE_TYPE_MEMORY_BARRIER                        = 45,
    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER                 = 46,
    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER                  = 47,
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO           = 48,
    VK_STRUCTURE_TYPE_UPDATE_SAMPLERS                       = 49,
    VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES               = 50,
    VK_STRUCTURE_TYPE_UPDATE_IMAGES                         = 51,
    VK_STRUCTURE_TYPE_UPDATE_BUFFERS                        = 52,
    VK_STRUCTURE_TYPE_UPDATE_AS_COPY                        = 53,
    VK_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO              = 54,
    VK_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO               = 55,
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO                  = 56,

    VK_STRUCTURE_TYPE_BEGIN_RANGE                           = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    VK_STRUCTURE_TYPE_END_RANGE                             = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,

    VK_NUM_STRUCTURE_TYPE                                   = (VK_STRUCTURE_TYPE_END_RANGE - VK_STRUCTURE_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_STRUCTURE_TYPE)
} VK_STRUCTURE_TYPE;

// ------------------------------------------------------------------------------------------------
// Flags

// Device creation flags
typedef enum _VK_DEVICE_CREATE_FLAGS
{
    VK_DEVICE_CREATE_VALIDATION_BIT                         = 0x00000001,
    VK_DEVICE_CREATE_MGPU_IQ_MATCH_BIT                      = 0x00000002,
    VK_MAX_ENUM(_VK_DEVICE_CREATE_FLAGS)
} VK_DEVICE_CREATE_FLAGS;

// Queue capabilities
typedef enum _VK_QUEUE_FLAGS
{
    VK_QUEUE_GRAPHICS_BIT                                   = 0x00000001,   // Queue supports graphics operations
    VK_QUEUE_COMPUTE_BIT                                    = 0x00000002,   // Queue supports compute operations
    VK_QUEUE_DMA_BIT                                        = 0x00000004,   // Queue supports DMA operations
    VK_QUEUE_EXTENDED_BIT                                   = 0x40000000,   // Extended queue
    VK_MAX_ENUM(_VK_QUEUE_FLAGS)
} VK_QUEUE_FLAGS;

// memory properties passed into vkAllocMemory().
typedef enum _VK_MEMORY_PROPERTY_FLAGS
{
    VK_MEMORY_PROPERTY_GPU_ONLY                             = 0x00000000,   // If not set, then allocate memory on device (GPU)
    VK_MEMORY_PROPERTY_CPU_VISIBLE_BIT                      = 0x00000001,
    VK_MEMORY_PROPERTY_CPU_GPU_COHERENT_BIT                 = 0x00000002,
    VK_MEMORY_PROPERTY_CPU_UNCACHED_BIT                     = 0x00000004,
    VK_MEMORY_PROPERTY_CPU_WRITE_COMBINED_BIT               = 0x00000008,
    VK_MEMORY_PROPERTY_PREFER_CPU_LOCAL                     = 0x00000010,   // all else being equal, prefer CPU access
    VK_MEMORY_PROPERTY_SHAREABLE_BIT                        = 0x00000020,
    VK_MAX_ENUM(_VK_MEMORY_PROPERTY_FLAGS)
} VK_MEMORY_PROPERTY_FLAGS;

typedef enum _VK_MEMORY_TYPE
{
    VK_MEMORY_TYPE_OTHER                                    = 0x00000000,   // device memory that is not any of the others
    VK_MEMORY_TYPE_BUFFER                                   = 0x00000001,   // memory for buffers and associated information
    VK_MEMORY_TYPE_IMAGE                                    = 0x00000002,   // memory for images and associated information

    VK_MEMORY_TYPE_BEGIN_RANGE                              = VK_MEMORY_TYPE_OTHER,
    VK_MEMORY_TYPE_END_RANGE                                = VK_MEMORY_TYPE_IMAGE,
    VK_NUM_MEMORY_TYPE                                      = (VK_MEMORY_TYPE_END_RANGE - VK_MEMORY_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_MEMORY_TYPE)
} VK_MEMORY_TYPE;

// Buffer and buffer allocation usage flags
typedef enum _VK_BUFFER_USAGE_FLAGS
{
    VK_BUFFER_USAGE_GENERAL                                 = 0x00000000,   // no special usage
    VK_BUFFER_USAGE_SHADER_ACCESS_READ_BIT                  = 0x00000001,   // Shader read (e.g. TBO, image buffer, UBO, SSBO)
    VK_BUFFER_USAGE_SHADER_ACCESS_WRITE_BIT                 = 0x00000002,   // Shader write (e.g. image buffer, SSBO)
    VK_BUFFER_USAGE_SHADER_ACCESS_ATOMIC_BIT                = 0x00000004,   // Shader atomic operations (e.g. image buffer, SSBO)
    VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT                     = 0x00000008,   // used as a source for copies
    VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT                = 0x00000010,   // used as a destination for copies
    VK_BUFFER_USAGE_UNIFORM_READ_BIT                        = 0x00000020,   // Uniform read (UBO)
    VK_BUFFER_USAGE_INDEX_FETCH_BIT                         = 0x00000040,   // Fixed function index fetch (index buffer)
    VK_BUFFER_USAGE_VERTEX_FETCH_BIT                        = 0x00000080,   // Fixed function vertex fetch (VBO)
    VK_BUFFER_USAGE_SHADER_STORAGE_BIT                      = 0x00000100,   // Shader storage buffer (SSBO)
    VK_BUFFER_USAGE_INDIRECT_PARAMETER_FETCH_BIT            = 0x00000200,   // Can be the source of indirect parameters (e.g. indirect buffer, parameter buffer)
    VK_BUFFER_USAGE_TEXTURE_BUFFER_BIT                      = 0x00000400,   // texture buffer (TBO)
    VK_BUFFER_USAGE_IMAGE_BUFFER_BIT                        = 0x00000800,   // image buffer (load/store)
    VK_MAX_ENUM(_VK_BUFFER_USAGE_FLAGS)
} VK_BUFFER_USAGE_FLAGS;

// Buffer flags
typedef enum _VK_BUFFER_CREATE_FLAGS
{
    VK_BUFFER_CREATE_SHAREABLE_BIT                          = 0x00000001,
    VK_BUFFER_CREATE_SPARSE_BIT                             = 0x00000002,
    VK_MAX_ENUM(_VK_BUFFER_CREATE_FLAGS)
} VK_BUFFER_CREATE_FLAGS;

typedef enum _VK_BUFFER_VIEW_TYPE
{
    VK_BUFFER_VIEW_RAW                                      = 0x00000000,   // Raw buffer without special structure (e.g. UBO, SSBO, indirect and parameter buffers)
    VK_BUFFER_VIEW_TYPED                                    = 0x00000001,   // Typed buffer, format and channels are used (TBO, image buffer)

    VK_BUFFER_VIEW_TYPE_BEGIN_RANGE                         = VK_BUFFER_VIEW_RAW,
    VK_BUFFER_VIEW_TYPE_END_RANGE                           = VK_BUFFER_VIEW_TYPED,
    VK_NUM_BUFFER_VIEW_TYPE                                 = (VK_BUFFER_VIEW_TYPE_END_RANGE - VK_BUFFER_VIEW_TYPE_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_BUFFER_VIEW_TYPE)
} VK_BUFFER_VIEW_TYPE;


// Images memory allocations can be used for resources of a given format class.
typedef enum _VK_IMAGE_FORMAT_CLASS
{
    VK_IMAGE_FORMAT_CLASS_128_BITS                          = 1,  // color formats
    VK_IMAGE_FORMAT_CLASS_96_BITS                           = 2,
    VK_IMAGE_FORMAT_CLASS_64_BITS                           = 3,
    VK_IMAGE_FORMAT_CLASS_48_BITS                           = 4,
    VK_IMAGE_FORMAT_CLASS_32_BITS                           = 5,
    VK_IMAGE_FORMAT_CLASS_24_BITS                           = 6,
    VK_IMAGE_FORMAT_CLASS_16_BITS                           = 7,
    VK_IMAGE_FORMAT_CLASS_8_BITS                            = 8,
    VK_IMAGE_FORMAT_CLASS_128_BIT_BLOCK                     = 9,  // 128-bit block compressed formats
    VK_IMAGE_FORMAT_CLASS_64_BIT_BLOCK                      = 10, // 64-bit block compressed formats
    VK_IMAGE_FORMAT_CLASS_D32                               = 11, // D32_SFLOAT
    VK_IMAGE_FORMAT_CLASS_D24                               = 12, // D24_UNORM
    VK_IMAGE_FORMAT_CLASS_D16                               = 13, // D16_UNORM
    VK_IMAGE_FORMAT_CLASS_S8                                = 14, // S8_UINT
    VK_IMAGE_FORMAT_CLASS_D32S8                             = 15, // D32_SFLOAT_S8_UINT
    VK_IMAGE_FORMAT_CLASS_D24S8                             = 16, // D24_UNORM_S8_UINT
    VK_IMAGE_FORMAT_CLASS_D16S8                             = 17, // D16_UNORM_S8_UINT
    VK_IMAGE_FORMAT_CLASS_LINEAR                            = 18, // used for pitch-linear (transparent) textures

    VK_IMAGE_FORMAT_CLASS_BEGIN_RANGE                       = VK_IMAGE_FORMAT_CLASS_128_BITS,
    VK_IMAGE_FORMAT_CLASS_END_RANGE                         = VK_IMAGE_FORMAT_CLASS_LINEAR,
    VK_NUM_IMAGE_FORMAT_CLASS                               = (VK_IMAGE_FORMAT_CLASS_END_RANGE - VK_IMAGE_FORMAT_CLASS_BEGIN_RANGE + 1),
    VK_MAX_ENUM(_VK_IMAGE_FORMAT_CLASS)
} VK_IMAGE_FORMAT_CLASS;

// Image and image allocation usage flags
typedef enum _VK_IMAGE_USAGE_FLAGS
{
    VK_IMAGE_USAGE_GENERAL                                  = 0x00000000,   // no special usage
    VK_IMAGE_USAGE_SHADER_ACCESS_READ_BIT                   = 0x00000001,   // shader read (e.g. texture, image)
    VK_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT                  = 0x00000002,   // shader write (e.g. image)
    VK_IMAGE_USAGE_SHADER_ACCESS_ATOMIC_BIT                 = 0x00000004,   // shader atomic operations (e.g. image)
    VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT                      = 0x00000008,   // used as a source for copies
    VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT                 = 0x00000010,   // used as a destination for copies
    VK_IMAGE_USAGE_TEXTURE_BIT                              = 0x00000020,   // opaque texture (2d, 3d, etc.)
    VK_IMAGE_USAGE_IMAGE_BIT                                = 0x00000040,   // opaque image (2d, 3d, etc.)
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT                     = 0x00000080,   // framebuffer color attachment
    VK_IMAGE_USAGE_DEPTH_STENCIL_BIT                        = 0x00000100,   // framebuffer depth/stencil
    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT                 = 0x00000200,   // image data not needed outside of rendering.
    VK_MAX_ENUM(_VK_IMAGE_USAGE_FLAGS)
} VK_IMAGE_USAGE_FLAGS;

// Image flags
typedef enum _VK_IMAGE_CREATE_FLAGS
{
    VK_IMAGE_CREATE_INVARIANT_DATA_BIT                      = 0x00000001,
    VK_IMAGE_CREATE_CLONEABLE_BIT                           = 0x00000002,
    VK_IMAGE_CREATE_SHAREABLE_BIT                           = 0x00000004,
    VK_IMAGE_CREATE_SPARSE_BIT                              = 0x00000008,
    VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT                      = 0x00000010,   // Allows image views to have different format than the base image
    VK_MAX_ENUM(_VK_IMAGE_CREATE_FLAGS)
} VK_IMAGE_CREATE_FLAGS;

// Depth-stencil view creation flags
typedef enum _VK_DEPTH_STENCIL_VIEW_CREATE_FLAGS
{
    VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT        = 0x00000001,
    VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT      = 0x00000002,
    VK_MAX_ENUM(_VK_DEPTH_STENCIL_VIEW_CREATE_FLAGS)
} VK_DEPTH_STENCIL_VIEW_CREATE_FLAGS;

// Pipeline creation flags
typedef enum _VK_PIPELINE_CREATE_FLAGS
{
    VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT             = 0x00000001,
    VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT                = 0x00000002,
    VK_MAX_ENUM(_VK_PIPELINE_CREATE_FLAGS)
} VK_PIPELINE_CREATE_FLAGS;

// Fence creation flags
typedef enum _VK_FENCE_CREATE_FLAGS
{
    VK_FENCE_CREATE_SIGNALED_BIT                            = 0x00000001,
    VK_MAX_ENUM(_VK_FENCE_CREATE_FLAGS)
} VK_FENCE_CREATE_FLAGS;

// Semaphore creation flags
typedef enum _VK_SEMAPHORE_CREATE_FLAGS
{
    VK_SEMAPHORE_CREATE_SHAREABLE_BIT                       = 0x00000001,
    VK_MAX_ENUM(_VK_SEMAPHORE_CREATE_FLAGS)
} VK_SEMAPHORE_CREATE_FLAGS;

// Format capability flags
typedef enum _VK_FORMAT_FEATURE_FLAGS
{
    VK_FORMAT_IMAGE_SHADER_READ_BIT                         = 0x00000001,
    VK_FORMAT_IMAGE_SHADER_WRITE_BIT                        = 0x00000002,
    VK_FORMAT_IMAGE_COPY_BIT                                = 0x00000004,
    VK_FORMAT_MEMORY_SHADER_ACCESS_BIT                      = 0x00000008,
    VK_FORMAT_COLOR_ATTACHMENT_WRITE_BIT                    = 0x00000010,
    VK_FORMAT_COLOR_ATTACHMENT_BLEND_BIT                    = 0x00000020,
    VK_FORMAT_DEPTH_ATTACHMENT_BIT                          = 0x00000040,
    VK_FORMAT_STENCIL_ATTACHMENT_BIT                        = 0x00000080,
    VK_FORMAT_MSAA_ATTACHMENT_BIT                           = 0x00000100,
    VK_FORMAT_CONVERSION_BIT                                = 0x00000200,
    VK_MAX_ENUM(_VK_FORMAT_FEATURE_FLAGS)
} VK_FORMAT_FEATURE_FLAGS;

// Query flags
typedef enum _VK_QUERY_CONTROL_FLAGS
{
    VK_QUERY_IMPRECISE_DATA_BIT                             = 0x00000001,
    VK_MAX_ENUM(_VK_QUERY_CONTROL_FLAGS)
} VK_QUERY_CONTROL_FLAGS;

// GPU compatibility flags
typedef enum _VK_GPU_COMPATIBILITY_FLAGS
{
    VK_GPU_COMPAT_ASIC_FEATURES_BIT                         = 0x00000001,
    VK_GPU_COMPAT_IQ_MATCH_BIT                              = 0x00000002,
    VK_GPU_COMPAT_PEER_TRANSFER_BIT                         = 0x00000004,
    VK_GPU_COMPAT_SHARED_MEMORY_BIT                         = 0x00000008,
    VK_GPU_COMPAT_SHARED_SYNC_BIT                           = 0x00000010,
    VK_GPU_COMPAT_SHARED_GPU0_DISPLAY_BIT                   = 0x00000020,
    VK_GPU_COMPAT_SHARED_GPU1_DISPLAY_BIT                   = 0x00000040,
    VK_MAX_ENUM(_VK_GPU_COMPATIBILITY_FLAGS)
} VK_GPU_COMPATIBILITY_FLAGS;

// Command buffer building flags
typedef enum _VK_CMD_BUFFER_BUILD_FLAGS
{
    VK_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT              = 0x00000001,
    VK_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH_BIT              = 0x00000002,
    VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT              = 0x00000004,
    VK_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT        = 0x00000008,
    VK_MAX_ENUM(_VK_CMD_BUFFER_BUILD_FLAGS)
} VK_CMD_BUFFER_BUILD_FLAGS;

// ------------------------------------------------------------------------------------------------
// Vulkan structures

typedef struct _VK_OFFSET2D
{
    int32_t                                     x;
    int32_t                                     y;
} VK_OFFSET2D;

typedef struct _VK_OFFSET3D
{
    int32_t                                     x;
    int32_t                                     y;
    int32_t                                     z;
} VK_OFFSET3D;

typedef struct _VK_EXTENT2D
{
    int32_t                                     width;
    int32_t                                     height;
} VK_EXTENT2D;

typedef struct _VK_EXTENT3D
{
    int32_t                                     width;
    int32_t                                     height;
    int32_t                                     depth;
} VK_EXTENT3D;

typedef struct _VK_VIEWPORT
{
    float                                       originX;
    float                                       originY;
    float                                       width;
    float                                       height;
    float                                       minDepth;
    float                                       maxDepth;
} VK_VIEWPORT;

typedef struct _VK_RECT
{
    VK_OFFSET2D                                 offset;
    VK_EXTENT2D                                 extent;
} VK_RECT;

typedef struct _VK_CHANNEL_MAPPING
{
    VK_CHANNEL_SWIZZLE                          r;
    VK_CHANNEL_SWIZZLE                          g;
    VK_CHANNEL_SWIZZLE                          b;
    VK_CHANNEL_SWIZZLE                          a;
} VK_CHANNEL_MAPPING;

typedef struct _VK_PHYSICAL_GPU_PROPERTIES
{
    uint32_t                                    apiVersion;
    uint32_t                                    driverVersion;
    uint32_t                                    vendorId;
    uint32_t                                    deviceId;
    VK_PHYSICAL_GPU_TYPE                        gpuType;
    char                                        gpuName[VK_MAX_PHYSICAL_GPU_NAME];
    VK_GPU_SIZE                                 maxInlineMemoryUpdateSize;
    uint32_t                                    maxBoundDescriptorSets;
    uint32_t                                    maxThreadGroupSize;
    uint64_t                                    timestampFrequency;
    bool32_t                                    multiColorAttachmentClears;
    uint32_t                                    maxDescriptorSets;              // at least 2?
    uint32_t                                    maxViewports;                   // at least 16?
    uint32_t                                    maxColorAttachments;            // at least 8?
} VK_PHYSICAL_GPU_PROPERTIES;

typedef struct _VK_PHYSICAL_GPU_PERFORMANCE
{
    float                                       maxGpuClock;
    float                                       aluPerClock;
    float                                       texPerClock;
    float                                       primsPerClock;
    float                                       pixelsPerClock;
} VK_PHYSICAL_GPU_PERFORMANCE;

typedef struct _VK_GPU_COMPATIBILITY_INFO
{
    VK_FLAGS                                    compatibilityFlags; // VK_GPU_COMPATIBILITY_FLAGS
} VK_GPU_COMPATIBILITY_INFO;

typedef struct _VK_APPLICATION_INFO
{
    VK_STRUCTURE_TYPE                           sType;              // Type of structure. Should be VK_STRUCTURE_TYPE_APPLICATION_INFO
    const void*                                 pNext;              // Next structure in chain
    const char*                                 pAppName;
    uint32_t                                    appVersion;
    const char*                                 pEngineName;
    uint32_t                                    engineVersion;
    uint32_t                                    apiVersion;
} VK_APPLICATION_INFO;

typedef void* (VKAPI *VK_ALLOC_FUNCTION)(
    void*                                       pUserData,
    size_t                                      size,
    size_t                                      alignment,
    VK_SYSTEM_ALLOC_TYPE                        allocType);

typedef void (VKAPI *VK_FREE_FUNCTION)(
    void*                                       pUserData,
    void*                                       pMem);

typedef struct _VK_ALLOC_CALLBACKS
{
    void*                                       pUserData;
    VK_ALLOC_FUNCTION                           pfnAlloc;
    VK_FREE_FUNCTION                            pfnFree;
} VK_ALLOC_CALLBACKS;

typedef struct _VK_DEVICE_QUEUE_CREATE_INFO
{
    uint32_t                                    queueNodeIndex;
    uint32_t                                    queueCount;
} VK_DEVICE_QUEUE_CREATE_INFO;

typedef struct _VK_DEVICE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Should be VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure
    uint32_t                                    queueRecordCount;
    const VK_DEVICE_QUEUE_CREATE_INFO*          pRequestedQueues;
    uint32_t                                    extensionCount;
    const char*const*                           ppEnabledExtensionNames;
    VK_VALIDATION_LEVEL                         maxValidationLevel;
    VK_FLAGS                                    flags;                      // VK_DEVICE_CREATE_FLAGS
} VK_DEVICE_CREATE_INFO;

typedef struct _VK_INSTANCE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Should be VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure
    const VK_APPLICATION_INFO*                  pAppInfo;
    const VK_ALLOC_CALLBACKS*                   pAllocCb;
    uint32_t                                    extensionCount;
    const char*const*                           ppEnabledExtensionNames;    // layer or extension name to be enabled
} VK_INSTANCE_CREATE_INFO;

// can be added to VK_DEVICE_CREATE_INFO or VK_INSTANCE_CREATE_INFO via pNext
typedef struct _VK_LAYER_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Should be VK_STRUCTURE_TYPE_LAYER_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure
    uint32_t                                    layerCount;
    const char *const*                          ppActiveLayerNames;         // layer name from the layer's vkEnumerateLayers())
} VK_LAYER_CREATE_INFO;

typedef struct _VK_PHYSICAL_GPU_QUEUE_PROPERTIES
{
    VK_FLAGS                                    queueFlags;                 // VK_QUEUE_FLAGS
    uint32_t                                    queueCount;
    uint32_t                                    maxAtomicCounters;
    bool32_t                                    supportsTimestamps;
    uint32_t                                    maxMemReferences;           // Tells how many memory references can be active for the given queue
} VK_PHYSICAL_GPU_QUEUE_PROPERTIES;

typedef struct _VK_PHYSICAL_GPU_MEMORY_PROPERTIES
{
    bool32_t                                    supportsMigration;
    bool32_t                                    supportsPinning;
} VK_PHYSICAL_GPU_MEMORY_PROPERTIES;

typedef struct _VK_MEMORY_ALLOC_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VK_GPU_SIZE                                 allocationSize;             // Size of memory allocation
    VK_FLAGS                                    memProps;                   // VK_MEMORY_PROPERTY_FLAGS
    VK_MEMORY_TYPE                              memType;
    VK_MEMORY_PRIORITY                          memPriority;
} VK_MEMORY_ALLOC_INFO;

// This structure is included in the VK_MEMORY_ALLOC_INFO chain
// for memory regions allocated for buffer usage.
typedef struct _VK_MEMORY_ALLOC_BUFFER_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VK_FLAGS                                    usage;                      // VK_BUFFER_USAGE_FLAGS
} VK_MEMORY_ALLOC_BUFFER_INFO;

// This structure is included in the VK_MEMORY_ALLOC_INFO chain
// for memory regions allocated for image usage.
typedef struct _VK_MEMORY_ALLOC_IMAGE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VK_FLAGS                                    usage;                      // VK_IMAGE_USAGE_FLAGS
    VK_IMAGE_FORMAT_CLASS                       formatClass;
    uint32_t                                    samples;
} VK_MEMORY_ALLOC_IMAGE_INFO;

typedef struct _VK_MEMORY_OPEN_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_MEMORY_OPEN_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VK_GPU_MEMORY                               sharedMem;
} VK_MEMORY_OPEN_INFO;

typedef struct _VK_PEER_MEMORY_OPEN_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VK_GPU_MEMORY                               originalMem;
} VK_PEER_MEMORY_OPEN_INFO;

typedef struct _VK_MEMORY_REQUIREMENTS
{
    VK_GPU_SIZE                                 size;                       // Specified in bytes
    VK_GPU_SIZE                                 alignment;                  // Specified in bytes
    VK_GPU_SIZE                                 granularity;                // Granularity on which vkBindObjectMemoryRange can bind sub-ranges of memory specified in bytes (usually the page size)
    VK_FLAGS                                    memProps;                   // VK_MEMORY_PROPERTY_FLAGS
    VK_MEMORY_TYPE                              memType;
} VK_MEMORY_REQUIREMENTS;

typedef struct _VK_BUFFER_MEMORY_REQUIREMENTS
{
    VK_FLAGS                                    usage;                      // VK_BUFFER_USAGE_FLAGS
} VK_BUFFER_MEMORY_REQUIREMENTS;

typedef struct _VK_IMAGE_MEMORY_REQUIREMENTS
{
    VK_FLAGS                                    usage;                      // VK_IMAGE_USAGE_FLAGS
    VK_IMAGE_FORMAT_CLASS                       formatClass;
    uint32_t                                    samples;
} VK_IMAGE_MEMORY_REQUIREMENTS;

typedef struct _VK_FORMAT_PROPERTIES
{
    VK_FLAGS                                    linearTilingFeatures;      // VK_FORMAT_FEATURE_FLAGS
    VK_FLAGS                                    optimalTilingFeatures;     // VK_FORMAT_FEATURE_FLAGS
} VK_FORMAT_PROPERTIES;

typedef struct _VK_BUFFER_VIEW_ATTACH_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VK_BUFFER_VIEW                              view;
} VK_BUFFER_VIEW_ATTACH_INFO;

typedef struct _VK_IMAGE_VIEW_ATTACH_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VK_IMAGE_VIEW                               view;
    VK_IMAGE_LAYOUT                             layout;
} VK_IMAGE_VIEW_ATTACH_INFO;

typedef struct _VK_UPDATE_SAMPLERS
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_UPDATE_SAMPLERS
    const void*                                 pNext;                      // Pointer to next structure
    uint32_t                                    binding;                    // Binding of the sampler (array)
    uint32_t                                    arrayIndex;                 // First element of the array to update or zero otherwise
    uint32_t                                    count;                      // Number of elements to update
    const VK_SAMPLER*                           pSamplers;
} VK_UPDATE_SAMPLERS;

typedef struct _VK_SAMPLER_IMAGE_VIEW_INFO
{
    VK_SAMPLER                                  pSampler;
    const VK_IMAGE_VIEW_ATTACH_INFO*            pImageView;
} VK_SAMPLER_IMAGE_VIEW_INFO;

typedef struct _VK_UPDATE_SAMPLER_TEXTURES
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES
    const void*                                 pNext;                      // Pointer to next structure
    uint32_t                                    binding;                    // Binding of the combined texture sampler (array)
    uint32_t                                    arrayIndex;                 // First element of the array to update or zero otherwise
    uint32_t                                    count;                      // Number of elements to update
    const VK_SAMPLER_IMAGE_VIEW_INFO*           pSamplerImageViews;
} VK_UPDATE_SAMPLER_TEXTURES;

typedef struct _VK_UPDATE_IMAGES
{
    VK_STRUCTURE_TYPE                           sType;                     // Must be VK_STRUCTURE_TYPE_UPDATE_IMAGES
    const void*                                 pNext;                     // Pointer to next structure
    VK_DESCRIPTOR_TYPE                          descriptorType;
    uint32_t                                    binding;                   // Binding of the image (array)
    uint32_t                                    arrayIndex;                // First element of the array to update or zero otherwise
    uint32_t                                    count;                     // Number of elements to update
    const VK_IMAGE_VIEW_ATTACH_INFO* const*     pImageViews;
} VK_UPDATE_IMAGES;

typedef struct _VK_UPDATE_BUFFERS
{
    VK_STRUCTURE_TYPE                           sType;                    // Must be VK_STRUCTURE_TYPE_UPDATE_BUFFERS
    const void*                                 pNext;                    // Pointer to next structure
    VK_DESCRIPTOR_TYPE                          descriptorType;
    uint32_t                                    binding;                  // Binding of the buffer (array)
    uint32_t                                    arrayIndex;               // First element of the array to update or zero otherwise
    uint32_t                                    count;                    // Number of elements to update
    const VK_BUFFER_VIEW_ATTACH_INFO* const*    pBufferViews;
} VK_UPDATE_BUFFERS;

typedef struct _VK_UPDATE_AS_COPY
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_UPDATE_AS_COPY
    const void*                                 pNext;                      // Pointer to next structure
    VK_DESCRIPTOR_TYPE                          descriptorType;
    VK_DESCRIPTOR_SET                           descriptorSet;
    uint32_t                                    binding;
    uint32_t                                    arrayElement;
    uint32_t                                    count;
} VK_UPDATE_AS_COPY;

typedef struct _VK_BUFFER_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure.
    VK_GPU_SIZE                                 size;                       // Specified in bytes
    VK_FLAGS                                    usage;                      // VK_BUFFER_USAGE_FLAGS
    VK_FLAGS                                    flags;                      // VK_BUFFER_CREATE_FLAGS
} VK_BUFFER_CREATE_INFO;

typedef struct _VK_BUFFER_VIEW_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure.
    VK_BUFFER                                   buffer;
    VK_BUFFER_VIEW_TYPE                         viewType;
    VK_FORMAT                                   format;                     // Optionally specifies format of elements
    VK_GPU_SIZE                                 offset;                     // Specified in bytes
    VK_GPU_SIZE                                 range;                      // View size specified in bytes
} VK_BUFFER_VIEW_CREATE_INFO;

typedef struct _VK_IMAGE_SUBRESOURCE
{
    VK_IMAGE_ASPECT                             aspect;
    uint32_t                                    mipLevel;
    uint32_t                                    arraySlice;
} VK_IMAGE_SUBRESOURCE;

typedef struct _VK_IMAGE_SUBRESOURCE_RANGE
{
    VK_IMAGE_ASPECT                             aspect;
    uint32_t                                    baseMipLevel;
    uint32_t                                    mipLevels;
    uint32_t                                    baseArraySlice;
    uint32_t                                    arraySize;
} VK_IMAGE_SUBRESOURCE_RANGE;

typedef struct _VK_EVENT_WAIT_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_EVENT_WAIT_INFO
    const void*                                 pNext;                      // Pointer to next structure.

    uint32_t                                    eventCount;                 // Number of events to wait on
    const VK_EVENT*                             pEvents;                    // Array of event objects to wait on

    VK_WAIT_EVENT                               waitEvent;                  // Pipeline event where the wait should happen

    uint32_t                                    memBarrierCount;            // Number of memory barriers
    const void**                                ppMemBarriers;              // Array of pointers to memory barriers (any of them can be either VK_MEMORY_BARRIER, VK_BUFFER_MEMORY_BARRIER, or VK_IMAGE_MEMORY_BARRIER)
} VK_EVENT_WAIT_INFO;

typedef struct _VK_PIPELINE_BARRIER
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_PIPELINE_BARRIER
    const void*                                 pNext;                      // Pointer to next structure.

    uint32_t                                    eventCount;                 // Number of events to wait on
    const VK_PIPE_EVENT*                        pEvents;                    // Array of pipeline events to wait on

    VK_WAIT_EVENT                               waitEvent;                  // Pipeline event where the wait should happen

    uint32_t                                    memBarrierCount;            // Number of memory barriers
    const void**                                ppMemBarriers;              // Array of pointers to memory barriers (any of them can be either VK_MEMORY_BARRIER, VK_BUFFER_MEMORY_BARRIER, or VK_IMAGE_MEMORY_BARRIER)
} VK_PIPELINE_BARRIER;

typedef struct _VK_MEMORY_BARRIER
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_MEMORY_BARRIER
    const void*                                 pNext;                      // Pointer to next structure.

    VK_FLAGS                                    outputMask;                 // Outputs the barrier should sync (see VK_MEMORY_OUTPUT_FLAGS)
    VK_FLAGS                                    inputMask;                  // Inputs the barrier should sync to (see VK_MEMORY_INPUT_FLAGS)
} VK_MEMORY_BARRIER;

typedef struct _VK_BUFFER_MEMORY_BARRIER
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER
    const void*                                 pNext;                      // Pointer to next structure.

    VK_FLAGS                                    outputMask;                 // Outputs the barrier should sync (see VK_MEMORY_OUTPUT_FLAGS)
    VK_FLAGS                                    inputMask;                  // Inputs the barrier should sync to (see VK_MEMORY_INPUT_FLAGS)

    VK_BUFFER                                   buffer;                     // Buffer to sync

    VK_GPU_SIZE                                 offset;                     // Offset within the buffer to sync
    VK_GPU_SIZE                                 size;                       // Amount of bytes to sync
} VK_BUFFER_MEMORY_BARRIER;

typedef struct _VK_IMAGE_MEMORY_BARRIER
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
    const void*                                 pNext;                      // Pointer to next structure.

    VK_FLAGS                                    outputMask;                 // Outputs the barrier should sync (see VK_MEMORY_OUTPUT_FLAGS)
    VK_FLAGS                                    inputMask;                  // Inputs the barrier should sync to (see VK_MEMORY_INPUT_FLAGS)

    VK_IMAGE_LAYOUT                             oldLayout;                  // Current layout of the image
    VK_IMAGE_LAYOUT                             newLayout;                  // New layout to transition the image to

    VK_IMAGE                                    image;                      // Image to sync

    VK_IMAGE_SUBRESOURCE_RANGE                  subresourceRange;           // Subresource range to sync
} VK_IMAGE_MEMORY_BARRIER;

typedef struct _VK_IMAGE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                      // Must be VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure.
    VK_IMAGE_TYPE                               imageType;
    VK_FORMAT                                   format;
    VK_EXTENT3D                                 extent;
    uint32_t                                    mipLevels;
    uint32_t                                    arraySize;
    uint32_t                                    samples;
    VK_IMAGE_TILING                             tiling;
    VK_FLAGS                                    usage;                      // VK_IMAGE_USAGE_FLAGS
    VK_FLAGS                                    flags;                      // VK_IMAGE_CREATE_FLAGS
} VK_IMAGE_CREATE_INFO;

typedef struct _VK_PEER_IMAGE_OPEN_INFO
{
    VK_IMAGE                                    originalImage;
} VK_PEER_IMAGE_OPEN_INFO;

typedef struct _VK_SUBRESOURCE_LAYOUT
{
    VK_GPU_SIZE                                 offset;                 // Specified in bytes
    VK_GPU_SIZE                                 size;                   // Specified in bytes
    VK_GPU_SIZE                                 rowPitch;               // Specified in bytes
    VK_GPU_SIZE                                 depthPitch;             // Specified in bytes
} VK_SUBRESOURCE_LAYOUT;

typedef struct _VK_IMAGE_VIEW_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                  // Must be VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
    const void*                                 pNext;                  // Pointer to next structure
    VK_IMAGE                                    image;
    VK_IMAGE_VIEW_TYPE                          viewType;
    VK_FORMAT                                   format;
    VK_CHANNEL_MAPPING                          channels;
    VK_IMAGE_SUBRESOURCE_RANGE                  subresourceRange;
    float                                       minLod;
} VK_IMAGE_VIEW_CREATE_INFO;

typedef struct _VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                  // Must be VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO
    const void*                                 pNext;                  // Pointer to next structure
    VK_IMAGE                                    image;
    VK_FORMAT                                   format;
    uint32_t                                    mipLevel;
    uint32_t                                    baseArraySlice;
    uint32_t                                    arraySize;
    VK_IMAGE                                    msaaResolveImage;
    VK_IMAGE_SUBRESOURCE_RANGE                  msaaResolveSubResource;
} VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO;

typedef struct _VK_DEPTH_STENCIL_VIEW_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;                  // Must be VK_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO
    const void*                                 pNext;                  // Pointer to next structure
    VK_IMAGE                                    image;
    uint32_t                                    mipLevel;
    uint32_t                                    baseArraySlice;
    uint32_t                                    arraySize;
    VK_IMAGE                                    msaaResolveImage;
    VK_IMAGE_SUBRESOURCE_RANGE                  msaaResolveSubResource;
    VK_FLAGS                                    flags;                  // VK_DEPTH_STENCIL_VIEW_CREATE_FLAGS
} VK_DEPTH_STENCIL_VIEW_CREATE_INFO;

typedef struct _VK_COLOR_ATTACHMENT_BIND_INFO
{
    VK_COLOR_ATTACHMENT_VIEW                    view;
    VK_IMAGE_LAYOUT                             layout;
} VK_COLOR_ATTACHMENT_BIND_INFO;

typedef struct _VK_DEPTH_STENCIL_BIND_INFO
{
    VK_DEPTH_STENCIL_VIEW                       view;
    VK_IMAGE_LAYOUT                             layout;
} VK_DEPTH_STENCIL_BIND_INFO;

typedef struct _VK_BUFFER_COPY
{
    VK_GPU_SIZE                                 srcOffset;              // Specified in bytes
    VK_GPU_SIZE                                 destOffset;             // Specified in bytes
    VK_GPU_SIZE                                 copySize;               // Specified in bytes
} VK_BUFFER_COPY;

typedef struct _VK_IMAGE_MEMORY_BIND_INFO
{
    VK_IMAGE_SUBRESOURCE                        subresource;
    VK_OFFSET3D                                 offset;
    VK_EXTENT3D                                 extent;
} VK_IMAGE_MEMORY_BIND_INFO;

typedef struct _VK_IMAGE_COPY
{
    VK_IMAGE_SUBRESOURCE                        srcSubresource;
    VK_OFFSET3D                                 srcOffset;
    VK_IMAGE_SUBRESOURCE                        destSubresource;
    VK_OFFSET3D                                 destOffset;
    VK_EXTENT3D                                 extent;
} VK_IMAGE_COPY;

typedef struct _VK_IMAGE_BLIT
{
    VK_IMAGE_SUBRESOURCE                        srcSubresource;
    VK_OFFSET3D                                 srcOffset;
    VK_EXTENT3D                                 srcExtent;
    VK_IMAGE_SUBRESOURCE                        destSubresource;
    VK_OFFSET3D                                 destOffset;
    VK_EXTENT3D                                 destExtent;
} VK_IMAGE_BLIT;

typedef struct _VK_BUFFER_IMAGE_COPY
{
    VK_GPU_SIZE                                 bufferOffset;           // Specified in bytes
    VK_IMAGE_SUBRESOURCE                        imageSubresource;
    VK_OFFSET3D                                 imageOffset;
    VK_EXTENT3D                                 imageExtent;
} VK_BUFFER_IMAGE_COPY;

typedef struct _VK_IMAGE_RESOLVE
{
    VK_IMAGE_SUBRESOURCE                        srcSubresource;
    VK_OFFSET2D                                 srcOffset;
    VK_IMAGE_SUBRESOURCE                        destSubresource;
    VK_OFFSET2D                                 destOffset;
    VK_EXTENT2D                                 extent;
} VK_IMAGE_RESOLVE;

typedef struct _VK_SHADER_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;              // Must be VK_STRUCTURE_TYPE_SHADER_CREATE_INFO
    const void*                                 pNext;              // Pointer to next structure
    size_t                                      codeSize;           // Specified in bytes
    const void*                                 pCode;
    VK_FLAGS                                    flags;              // Reserved
} VK_SHADER_CREATE_INFO;

typedef struct _VK_DESCRIPTOR_SET_LAYOUT_BINDING
{
    VK_DESCRIPTOR_TYPE                          descriptorType;
    uint32_t                                    count;
    VK_FLAGS                                    stageFlags;         // VK_SHADER_STAGE_FLAGS
    const VK_SAMPLER*                           pImmutableSamplers;
} VK_DESCRIPTOR_SET_LAYOUT_BINDING;

typedef struct _VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;             // Must be VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    const void*                                 pNext;             // Pointer to next structure
    uint32_t                                    count;             // Number of bindings in the descriptor set layout
    const VK_DESCRIPTOR_SET_LAYOUT_BINDING*     pBinding;          // Array of descriptor set layout bindings
} VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

typedef struct _VK_DESCRIPTOR_TYPE_COUNT
{
    VK_DESCRIPTOR_TYPE                          type;
    uint32_t                                    count;
} VK_DESCRIPTOR_TYPE_COUNT;

typedef struct _VK_DESCRIPTOR_POOL_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;              // Must be VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
    const void*                                 pNext;              // Pointer to next structure
    uint32_t                                    count;
    const VK_DESCRIPTOR_TYPE_COUNT*             pTypeCount;
} VK_DESCRIPTOR_POOL_CREATE_INFO;

typedef struct _VK_LINK_CONST_BUFFER
{
    uint32_t                                    bufferId;
    size_t                                      bufferSize;
    const void*                                 pBufferData;
} VK_LINK_CONST_BUFFER;

typedef struct _VK_SPECIALIZATION_MAP_ENTRY
{
    uint32_t                                    constantId;         // The SpecConstant ID specified in the BIL
    uint32_t                                    offset;             // Offset of the value in the data block
} VK_SPECIALIZATION_MAP_ENTRY;

typedef struct _VK_SPECIALIZATION_INFO
{
    uint32_t                                    mapEntryCount;
    const VK_SPECIALIZATION_MAP_ENTRY*          pMap;               // mapEntryCount entries
    const void*                                 pData;
} VK_SPECIALIZATION_INFO;

typedef struct _VK_PIPELINE_SHADER
{
    VK_PIPELINE_SHADER_STAGE                    stage;
    VK_SHADER                                   shader;
    uint32_t                                    linkConstBufferCount;
    const VK_LINK_CONST_BUFFER*                 pLinkConstBufferInfo;
    const VK_SPECIALIZATION_INFO*               pSpecializationInfo;
} VK_PIPELINE_SHADER;

typedef struct _VK_COMPUTE_PIPELINE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VK_PIPELINE_SHADER                          cs;
    VK_FLAGS                                    flags;      // VK_PIPELINE_CREATE_FLAGS
    VK_DESCRIPTOR_SET_LAYOUT_CHAIN              setLayoutChain;
    uint32_t                                    localSizeX;
    uint32_t                                    localSizeY;
    uint32_t                                    localSizeZ;

} VK_COMPUTE_PIPELINE_CREATE_INFO;

typedef struct _VK_VERTEX_INPUT_BINDING_DESCRIPTION
{
    uint32_t                                    binding;        // Vertex buffer binding id
    uint32_t                                    strideInBytes;  // Distance between vertices in bytes (0  = no advancement)

    VK_VERTEX_INPUT_STEP_RATE                   stepRate;       // Rate at which binding is incremented
} VK_VERTEX_INPUT_BINDING_DESCRIPTION;

typedef struct _VK_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION
{
    uint32_t                                    location;       // location of the shader vertex attrib
    uint32_t                                    binding;        // Vertex buffer binding id

    VK_FORMAT                                   format;         // format of source data

    uint32_t                                    offsetInBytes;  // Offset of first element in bytes from base of vertex
} VK_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION;

typedef struct _VK_PIPELINE_VERTEX_INPUT_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;          // Should be VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO
    const void*                                 pNext;          // Pointer to next structure

    uint32_t                                    bindingCount;   // number of bindings
    const VK_VERTEX_INPUT_BINDING_DESCRIPTION*  pVertexBindingDescriptions;

    uint32_t                                    attributeCount; // number of attributes
    const VK_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pVertexAttributeDescriptions;
} VK_PIPELINE_VERTEX_INPUT_CREATE_INFO;

typedef struct _VK_PIPELINE_IA_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VK_PRIMITIVE_TOPOLOGY                       topology;
    bool32_t                                    disableVertexReuse;         // optional
    bool32_t                                    primitiveRestartEnable;
    uint32_t                                    primitiveRestartIndex;      // optional (GL45)
} VK_PIPELINE_IA_STATE_CREATE_INFO;

typedef struct _VK_PIPELINE_TESS_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    patchControlPoints;
} VK_PIPELINE_TESS_STATE_CREATE_INFO;

typedef struct _VK_PIPELINE_VP_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    numViewports;
    VK_COORDINATE_ORIGIN                        clipOrigin;                 // optional (GL45)
    VK_DEPTH_MODE                               depthMode;                  // optional (GL45)
} VK_PIPELINE_VP_STATE_CREATE_INFO;

typedef struct _VK_PIPELINE_RS_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    bool32_t                                    depthClipEnable;
    bool32_t                                    rasterizerDiscardEnable;
    bool32_t                                    programPointSize;           // optional (GL45)
    VK_COORDINATE_ORIGIN                        pointOrigin;                // optional (GL45)
    VK_PROVOKING_VERTEX_CONVENTION              provokingVertex;            // optional (GL45)
    VK_FILL_MODE                                fillMode;                   // optional (GL45)
    VK_CULL_MODE                                cullMode;
    VK_FACE_ORIENTATION                         frontFace;
} VK_PIPELINE_RS_STATE_CREATE_INFO;

typedef struct _VK_PIPELINE_MS_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    samples;
    bool32_t                                    multisampleEnable;          // optional (GL45)
    bool32_t                                    sampleShadingEnable;        // optional (GL45)
    float                                       minSampleShading;           // optional (GL45)
    VK_SAMPLE_MASK                              sampleMask;
} VK_PIPELINE_MS_STATE_CREATE_INFO;

typedef struct _VK_PIPELINE_CB_ATTACHMENT_STATE
{
    bool32_t                                    blendEnable;
    VK_FORMAT                                   format;
    VK_BLEND                                    srcBlendColor;
    VK_BLEND                                    destBlendColor;
    VK_BLEND_FUNC                               blendFuncColor;
    VK_BLEND                                    srcBlendAlpha;
    VK_BLEND                                    destBlendAlpha;
    VK_BLEND_FUNC                               blendFuncAlpha;
    uint8_t                                     channelWriteMask;
} VK_PIPELINE_CB_ATTACHMENT_STATE;

typedef struct _VK_PIPELINE_CB_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    bool32_t                                    alphaToCoverageEnable;
    bool32_t                                    logicOpEnable;
    VK_LOGIC_OP                                 logicOp;
    uint32_t                                    attachmentCount;    // # of pAttachments
    const VK_PIPELINE_CB_ATTACHMENT_STATE*      pAttachments;
} VK_PIPELINE_CB_STATE_CREATE_INFO;

typedef struct _VK_STENCIL_OP_STATE
{
    VK_STENCIL_OP                               stencilFailOp;
    VK_STENCIL_OP                               stencilPassOp;
    VK_STENCIL_OP                               stencilDepthFailOp;
    VK_COMPARE_FUNC                             stencilFunc;
} VK_STENCIL_OP_STATE;

typedef struct _VK_PIPELINE_DS_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VK_FORMAT                                   format;
    bool32_t                                    depthTestEnable;
    bool32_t                                    depthWriteEnable;
    VK_COMPARE_FUNC                             depthFunc;
    bool32_t                                    depthBoundsEnable;          // optional (depth_bounds_test)
    bool32_t                                    stencilTestEnable;
    VK_STENCIL_OP_STATE                         front;
    VK_STENCIL_OP_STATE                         back;
} VK_PIPELINE_DS_STATE_CREATE_INFO;

typedef struct _VK_PIPELINE_SHADER_STAGE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VK_PIPELINE_SHADER                          shader;
} VK_PIPELINE_SHADER_STAGE_CREATE_INFO;

typedef struct _VK_GRAPHICS_PIPELINE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VK_FLAGS                                    flags;      // VK_PIPELINE_CREATE_FLAGS
    VK_DESCRIPTOR_SET_LAYOUT_CHAIN              pSetLayoutChain;
} VK_GRAPHICS_PIPELINE_CREATE_INFO;

typedef struct _VK_SAMPLER_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;          // Must be VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
    const void*                                 pNext;          // Pointer to next structure
    VK_TEX_FILTER                               magFilter;      // Filter mode for magnification
    VK_TEX_FILTER                               minFilter;      // Filter mode for minifiation
    VK_TEX_MIPMAP_MODE                          mipMode;        // Mipmap selection mode
    VK_TEX_ADDRESS                              addressU;
    VK_TEX_ADDRESS                              addressV;
    VK_TEX_ADDRESS                              addressW;
    float                                       mipLodBias;
    uint32_t                                    maxAnisotropy;
    VK_COMPARE_FUNC                             compareFunc;
    float                                       minLod;
    float                                       maxLod;
    VK_BORDER_COLOR_TYPE                        borderColorType;
} VK_SAMPLER_CREATE_INFO;

typedef struct _VK_DYNAMIC_VP_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    viewportAndScissorCount;  // number of entries in pViewports and pScissors
    const VK_VIEWPORT*                          pViewports;
    const VK_RECT*                              pScissors;
} VK_DYNAMIC_VP_STATE_CREATE_INFO;

typedef struct _VK_DYNAMIC_RS_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    float                                       depthBias;
    float                                       depthBiasClamp;
    float                                       slopeScaledDepthBias;
    float                                       pointSize;          // optional (GL45) - Size of points
    float                                       pointFadeThreshold; // optional (GL45) - Size of point fade threshold
    float                                       lineWidth;          // optional (GL45) - Width of lines
} VK_DYNAMIC_RS_STATE_CREATE_INFO;

typedef struct _VK_DYNAMIC_CB_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    float                                       blendConst[4];
} VK_DYNAMIC_CB_STATE_CREATE_INFO;

typedef struct _VK_DYNAMIC_DS_STATE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    float                                       minDepth;               // optional (depth_bounds_test)
    float                                       maxDepth;               // optional (depth_bounds_test)
    uint32_t                                    stencilReadMask;
    uint32_t                                    stencilWriteMask;
    uint32_t                                    stencilFrontRef;
    uint32_t                                    stencilBackRef;
} VK_DYNAMIC_DS_STATE_CREATE_INFO;

typedef struct _VK_CMD_BUFFER_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    queueNodeIndex;
    VK_FLAGS                                    flags;
} VK_CMD_BUFFER_CREATE_INFO;

typedef struct _VK_CMD_BUFFER_BEGIN_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO
    const void*                                 pNext;      // Pointer to next structure

    VK_FLAGS                                    flags;      // VK_CMD_BUFFER_BUILD_FLAGS
} VK_CMD_BUFFER_BEGIN_INFO;

typedef struct _VK_RENDER_PASS_BEGIN
{
    VK_RENDER_PASS                              renderPass;
    VK_FRAMEBUFFER                              framebuffer;
} VK_RENDER_PASS_BEGIN;

typedef struct _VK_CMD_BUFFER_GRAPHICS_BEGIN_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO
    const void*                                 pNext;      // Pointer to next structure

    VK_RENDER_PASS_BEGIN                        renderPassContinue;  // Only needed when a render pass is split across two command buffers
} VK_CMD_BUFFER_GRAPHICS_BEGIN_INFO;

// Union allowing specification of floating point or raw color data. Actual value selected is based on image being cleared.
typedef union _VK_CLEAR_COLOR_VALUE
{
    float                                       floatColor[4];
    uint32_t                                    rawColor[4];
} VK_CLEAR_COLOR_VALUE;

typedef struct _VK_CLEAR_COLOR
{
    VK_CLEAR_COLOR_VALUE                        color;
    bool32_t                                    useRawValue;
} VK_CLEAR_COLOR;

typedef struct _VK_RENDER_PASS_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure

    VK_RECT                                     renderArea;
    uint32_t                                    colorAttachmentCount;
    VK_EXTENT2D                                 extent;
    uint32_t                                    sampleCount;
    uint32_t                                    layers;
    const VK_FORMAT*                            pColorFormats;
    const VK_IMAGE_LAYOUT*                      pColorLayouts;
    const VK_ATTACHMENT_LOAD_OP*                pColorLoadOps;
    const VK_ATTACHMENT_STORE_OP*               pColorStoreOps;
    const VK_CLEAR_COLOR*                       pColorLoadClearValues;
    VK_FORMAT                                   depthStencilFormat;
    VK_IMAGE_LAYOUT                             depthStencilLayout;
    VK_ATTACHMENT_LOAD_OP                       depthLoadOp;
    float                                       depthLoadClearValue;
    VK_ATTACHMENT_STORE_OP                      depthStoreOp;
    VK_ATTACHMENT_LOAD_OP                       stencilLoadOp;
    uint32_t                                    stencilLoadClearValue;
    VK_ATTACHMENT_STORE_OP                      stencilStoreOp;
} VK_RENDER_PASS_CREATE_INFO;

typedef struct _VK_EVENT_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_EVENT_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VK_FLAGS                                    flags;      // Reserved
} VK_EVENT_CREATE_INFO;

typedef struct _VK_FENCE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VK_FENCE_CREATE_FLAGS                       flags;      // VK_FENCE_CREATE_FLAGS
} VK_FENCE_CREATE_INFO;

typedef struct _VK_SEMAPHORE_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    initialCount;
    VK_FLAGS                                    flags;      // VK_SEMAPHORE_CREATE_FLAGS
} VK_SEMAPHORE_CREATE_INFO;

typedef struct _VK_SEMAPHORE_OPEN_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO
    const void*                                 pNext;      // Pointer to next structure
    VK_SEMAPHORE                                sharedSemaphore;
} VK_SEMAPHORE_OPEN_INFO;

typedef struct _VK_PIPELINE_STATISTICS_DATA
{
    uint64_t                                    fsInvocations;            // Fragment shader invocations
    uint64_t                                    cPrimitives;              // Clipper primitives
    uint64_t                                    cInvocations;             // Clipper invocations
    uint64_t                                    vsInvocations;            // Vertex shader invocations
    uint64_t                                    gsInvocations;            // Geometry shader invocations
    uint64_t                                    gsPrimitives;             // Geometry shader primitives
    uint64_t                                    iaPrimitives;             // Input primitives
    uint64_t                                    iaVertices;               // Input vertices
    uint64_t                                    tcsInvocations;           // Tessellation control shader invocations
    uint64_t                                    tesInvocations;           // Tessellation evaluation shader invocations
    uint64_t                                    csInvocations;            // Compute shader invocations
} VK_PIPELINE_STATISTICS_DATA;

typedef struct _VK_QUERY_POOL_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;      // Must be VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VK_QUERY_TYPE                               queryType;
    uint32_t                                    slots;
} VK_QUERY_POOL_CREATE_INFO;

typedef struct _VK_FRAMEBUFFER_CREATE_INFO
{
    VK_STRUCTURE_TYPE                           sType;  // Must be VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
    const void*                                 pNext;  // Pointer to next structure

    uint32_t                                    colorAttachmentCount;
    const VK_COLOR_ATTACHMENT_BIND_INFO*        pColorAttachments;
    const VK_DEPTH_STENCIL_BIND_INFO*           pDepthStencilAttachment;

    uint32_t                                    sampleCount;
    uint32_t                                    width;
    uint32_t                                    height;
    uint32_t                                    layers;
} VK_FRAMEBUFFER_CREATE_INFO;

typedef struct _VK_DRAW_INDIRECT_CMD
{
    uint32_t                                    vertexCount;
    uint32_t                                    instanceCount;
    uint32_t                                    firstVertex;
    uint32_t                                    firstInstance;
} VK_DRAW_INDIRECT_CMD;

typedef struct _VK_DRAW_INDEXED_INDIRECT_CMD
{
    uint32_t                                    indexCount;
    uint32_t                                    instanceCount;
    uint32_t                                    firstIndex;
    int32_t                                     vertexOffset;
    uint32_t                                    firstInstance;
} VK_DRAW_INDEXED_INDIRECT_CMD;

typedef struct _VK_DISPATCH_INDIRECT_CMD
{
    uint32_t                                    x;
    uint32_t                                    y;
    uint32_t                                    z;
} VK_DISPATCH_INDIRECT_CMD;

// ------------------------------------------------------------------------------------------------
// API functions
typedef VK_RESULT (VKAPI *vkCreateInstanceType)(const VK_INSTANCE_CREATE_INFO* pCreateInfo, VK_INSTANCE* pInstance);
typedef VK_RESULT (VKAPI *vkDestroyInstanceType)(VK_INSTANCE instance);
typedef VK_RESULT (VKAPI *vkEnumerateGpusType)(VK_INSTANCE instance, uint32_t maxGpus, uint32_t* pGpuCount, VK_PHYSICAL_GPU* pGpus);
typedef VK_RESULT (VKAPI *vkGetGpuInfoType)(VK_PHYSICAL_GPU gpu, VK_PHYSICAL_GPU_INFO_TYPE infoType, size_t* pDataSize, void* pData);
typedef void *    (VKAPI *vkGetProcAddrType)(VK_PHYSICAL_GPU gpu, const char * pName);
typedef VK_RESULT (VKAPI *vkCreateDeviceType)(VK_PHYSICAL_GPU gpu, const VK_DEVICE_CREATE_INFO* pCreateInfo, VK_DEVICE* pDevice);
typedef VK_RESULT (VKAPI *vkDestroyDeviceType)(VK_DEVICE device);
typedef VK_RESULT (VKAPI *vkGetExtensionSupportType)(VK_PHYSICAL_GPU gpu, const char* pExtName);
typedef VK_RESULT (VKAPI *vkEnumerateLayersType)(VK_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved);
typedef VK_RESULT (VKAPI *vkGetDeviceQueueType)(VK_DEVICE device, uint32_t queueNodeIndex, uint32_t queueIndex, VK_QUEUE* pQueue);
typedef VK_RESULT (VKAPI *vkQueueSubmitType)(VK_QUEUE queue, uint32_t cmdBufferCount, const VK_CMD_BUFFER* pCmdBuffers, VK_FENCE fence);
typedef VK_RESULT (VKAPI *vkQueueAddMemReferenceType)(VK_QUEUE queue, VK_GPU_MEMORY mem);
typedef VK_RESULT (VKAPI *vkQueueRemoveMemReferenceType)(VK_QUEUE queue, VK_GPU_MEMORY mem);
typedef VK_RESULT (VKAPI *vkQueueWaitIdleType)(VK_QUEUE queue);
typedef VK_RESULT (VKAPI *vkDeviceWaitIdleType)(VK_DEVICE device);
typedef VK_RESULT (VKAPI *vkAllocMemoryType)(VK_DEVICE device, const VK_MEMORY_ALLOC_INFO* pAllocInfo, VK_GPU_MEMORY* pMem);
typedef VK_RESULT (VKAPI *vkFreeMemoryType)(VK_GPU_MEMORY mem);
typedef VK_RESULT (VKAPI *vkSetMemoryPriorityType)(VK_GPU_MEMORY mem, VK_MEMORY_PRIORITY priority);
typedef VK_RESULT (VKAPI *vkMapMemoryType)(VK_GPU_MEMORY mem, VK_FLAGS flags, void** ppData);
typedef VK_RESULT (VKAPI *vkUnmapMemoryType)(VK_GPU_MEMORY mem);
typedef VK_RESULT (VKAPI *vkPinSystemMemoryType)(VK_DEVICE device, const void* pSysMem, size_t memSize, VK_GPU_MEMORY* pMem);
typedef VK_RESULT (VKAPI *vkGetMultiGpuCompatibilityType)(VK_PHYSICAL_GPU gpu0, VK_PHYSICAL_GPU gpu1, VK_GPU_COMPATIBILITY_INFO* pInfo);
typedef VK_RESULT (VKAPI *vkOpenSharedMemoryType)(VK_DEVICE device, const VK_MEMORY_OPEN_INFO* pOpenInfo, VK_GPU_MEMORY* pMem);
typedef VK_RESULT (VKAPI *vkOpenSharedSemaphoreType)(VK_DEVICE device, const VK_SEMAPHORE_OPEN_INFO* pOpenInfo, VK_SEMAPHORE* pSemaphore);
typedef VK_RESULT (VKAPI *vkOpenPeerMemoryType)(VK_DEVICE device, const VK_PEER_MEMORY_OPEN_INFO* pOpenInfo, VK_GPU_MEMORY* pMem);
typedef VK_RESULT (VKAPI *vkOpenPeerImageType)(VK_DEVICE device, const VK_PEER_IMAGE_OPEN_INFO* pOpenInfo, VK_IMAGE* pImage, VK_GPU_MEMORY* pMem);
typedef VK_RESULT (VKAPI *vkDestroyObjectType)(VK_OBJECT object);
typedef VK_RESULT (VKAPI *vkGetObjectInfoType)(VK_BASE_OBJECT object, VK_OBJECT_INFO_TYPE infoType, size_t* pDataSize, void* pData);
typedef VK_RESULT (VKAPI *vkBindObjectMemoryType)(VK_OBJECT object, uint32_t allocationIdx, VK_GPU_MEMORY mem, VK_GPU_SIZE offset);
typedef VK_RESULT (VKAPI *vkBindObjectMemoryRangeType)(VK_OBJECT object, uint32_t allocationIdx, VK_GPU_SIZE rangeOffset,VK_GPU_SIZE rangeSize, VK_GPU_MEMORY mem, VK_GPU_SIZE memOffset);
typedef VK_RESULT (VKAPI *vkBindImageMemoryRangeType)(VK_IMAGE image, uint32_t allocationIdx, const VK_IMAGE_MEMORY_BIND_INFO* bindInfo, VK_GPU_MEMORY mem, VK_GPU_SIZE memOffset);
typedef VK_RESULT (VKAPI *vkCreateFenceType)(VK_DEVICE device, const VK_FENCE_CREATE_INFO* pCreateInfo, VK_FENCE* pFence);
typedef VK_RESULT (VKAPI *vkResetFencesType)(VK_DEVICE device, uint32_t fenceCount, VK_FENCE* pFences);
typedef VK_RESULT (VKAPI *vkGetFenceStatusType)(VK_FENCE fence);
typedef VK_RESULT (VKAPI *vkWaitForFencesType)(VK_DEVICE device, uint32_t fenceCount, const VK_FENCE* pFences, bool32_t waitAll, uint64_t timeout);
typedef VK_RESULT (VKAPI *vkCreateSemaphoreType)(VK_DEVICE device, const VK_SEMAPHORE_CREATE_INFO* pCreateInfo, VK_SEMAPHORE* pSemaphore);
typedef VK_RESULT (VKAPI *vkQueueSignalSemaphoreType)(VK_QUEUE queue, VK_SEMAPHORE semaphore);
typedef VK_RESULT (VKAPI *vkQueueWaitSemaphoreType)(VK_QUEUE queue, VK_SEMAPHORE semaphore);
typedef VK_RESULT (VKAPI *vkCreateEventType)(VK_DEVICE device, const VK_EVENT_CREATE_INFO* pCreateInfo, VK_EVENT* pEvent);
typedef VK_RESULT (VKAPI *vkGetEventStatusType)(VK_EVENT event);
typedef VK_RESULT (VKAPI *vkSetEventType)(VK_EVENT event);
typedef VK_RESULT (VKAPI *vkResetEventType)(VK_EVENT event);
typedef VK_RESULT (VKAPI *vkCreateQueryPoolType)(VK_DEVICE device, const VK_QUERY_POOL_CREATE_INFO* pCreateInfo, VK_QUERY_POOL* pQueryPool);
typedef VK_RESULT (VKAPI *vkGetQueryPoolResultsType)(VK_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData);
typedef VK_RESULT (VKAPI *vkGetFormatInfoType)(VK_DEVICE device, VK_FORMAT format, VK_FORMAT_INFO_TYPE infoType, size_t* pDataSize, void* pData);
typedef VK_RESULT (VKAPI *vkCreateBufferType)(VK_DEVICE device, const VK_BUFFER_CREATE_INFO* pCreateInfo, VK_BUFFER* pBuffer);
typedef VK_RESULT (VKAPI *vkCreateBufferViewType)(VK_DEVICE device, const VK_BUFFER_VIEW_CREATE_INFO* pCreateInfo, VK_BUFFER_VIEW* pView);
typedef VK_RESULT (VKAPI *vkCreateImageType)(VK_DEVICE device, const VK_IMAGE_CREATE_INFO* pCreateInfo, VK_IMAGE* pImage);
typedef VK_RESULT (VKAPI *vkGetImageSubresourceInfoType)(VK_IMAGE image, const VK_IMAGE_SUBRESOURCE* pSubresource, VK_SUBRESOURCE_INFO_TYPE infoType, size_t* pDataSize, void* pData);
typedef VK_RESULT (VKAPI *vkCreateImageViewType)(VK_DEVICE device, const VK_IMAGE_VIEW_CREATE_INFO* pCreateInfo, VK_IMAGE_VIEW* pView);
typedef VK_RESULT (VKAPI *vkCreateColorAttachmentViewType)(VK_DEVICE device, const VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo, VK_COLOR_ATTACHMENT_VIEW* pView);
typedef VK_RESULT (VKAPI *vkCreateDepthStencilViewType)(VK_DEVICE device, const VK_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo, VK_DEPTH_STENCIL_VIEW* pView);
typedef VK_RESULT (VKAPI *vkCreateShaderType)(VK_DEVICE device, const VK_SHADER_CREATE_INFO* pCreateInfo, VK_SHADER* pShader);
typedef VK_RESULT (VKAPI *vkCreateGraphicsPipelineType)(VK_DEVICE device, const VK_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, VK_PIPELINE* pPipeline);
typedef VK_RESULT (VKAPI *vkCreateGraphicsPipelineDerivativeType)(VK_DEVICE device, const VK_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo, VK_PIPELINE basePipeline, VK_PIPELINE* pPipeline);
typedef VK_RESULT (VKAPI *vkCreateComputePipelineType)(VK_DEVICE device, const VK_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo, VK_PIPELINE* pPipeline);
typedef VK_RESULT (VKAPI *vkStorePipelineType)(VK_PIPELINE pipeline, size_t* pDataSize, void* pData);
typedef VK_RESULT (VKAPI *vkLoadPipelineType)(VK_DEVICE device, size_t dataSize, const void* pData, VK_PIPELINE* pPipeline);
typedef VK_RESULT (VKAPI *vkLoadPipelineDerivativeType)(VK_DEVICE device, size_t dataSize, const void* pData, VK_PIPELINE basePipeline, VK_PIPELINE* pPipeline);
typedef VK_RESULT (VKAPI *vkCreateSamplerType)(VK_DEVICE device, const VK_SAMPLER_CREATE_INFO* pCreateInfo, VK_SAMPLER* pSampler);
typedef VK_RESULT (VKAPI *vkCreateDescriptorSetLayoutType)(VK_DEVICE device, const VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pCreateInfo, VK_DESCRIPTOR_SET_LAYOUT* pSetLayout);
typedef VK_RESULT (VKAPI *vkCreateDescriptorSetLayoutChainType)(VK_DEVICE device, uint32_t setLayoutArrayCount, const VK_DESCRIPTOR_SET_LAYOUT* pSetLayoutArray, VK_DESCRIPTOR_SET_LAYOUT_CHAIN* pLayoutChain);
typedef VK_RESULT (VKAPI *vkBeginDescriptorPoolUpdateType)(VK_DEVICE device, VK_DESCRIPTOR_UPDATE_MODE updateMode);
typedef VK_RESULT (VKAPI *vkEndDescriptorPoolUpdateType)(VK_DEVICE device, VK_CMD_BUFFER cmd);
typedef VK_RESULT (VKAPI *vkCreateDescriptorPoolType)(VK_DEVICE device, VK_DESCRIPTOR_POOL_USAGE poolUsage, uint32_t maxSets, const VK_DESCRIPTOR_POOL_CREATE_INFO* pCreateInfo, VK_DESCRIPTOR_POOL* pDescriptorPool);
typedef VK_RESULT (VKAPI *vkResetDescriptorPoolType)(VK_DESCRIPTOR_POOL descriptorPool);
typedef VK_RESULT (VKAPI *vkAllocDescriptorSetsType)(VK_DESCRIPTOR_POOL descriptorPool, VK_DESCRIPTOR_SET_USAGE setUsage, uint32_t count, const VK_DESCRIPTOR_SET_LAYOUT* pSetLayouts, VK_DESCRIPTOR_SET* pDescriptorSets, uint32_t* pCount);
typedef void      (VKAPI *vkClearDescriptorSetsType)(VK_DESCRIPTOR_POOL descriptorPool, uint32_t count, const VK_DESCRIPTOR_SET* pDescriptorSets);
typedef void      (VKAPI *vkUpdateDescriptorsType)(VK_DESCRIPTOR_SET descriptorSet, uint32_t updateCount, const void** pUpdateArray);
typedef VK_RESULT (VKAPI *vkCreateDynamicViewportStateType)(VK_DEVICE device, const VK_DYNAMIC_VP_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_VP_STATE_OBJECT* pState);
typedef VK_RESULT (VKAPI *vkCreateDynamicRasterStateType)(VK_DEVICE device, const VK_DYNAMIC_RS_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_RS_STATE_OBJECT* pState);
typedef VK_RESULT (VKAPI *vkCreateDynamicColorBlendStateType)(VK_DEVICE device, const VK_DYNAMIC_CB_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_CB_STATE_OBJECT* pState);
typedef VK_RESULT (VKAPI *vkCreateDynamicDepthStencilStateType)(VK_DEVICE device, const VK_DYNAMIC_DS_STATE_CREATE_INFO* pCreateInfo, VK_DYNAMIC_DS_STATE_OBJECT* pState);
typedef VK_RESULT (VKAPI *vkCreateCommandBufferType)(VK_DEVICE device, const VK_CMD_BUFFER_CREATE_INFO* pCreateInfo, VK_CMD_BUFFER* pCmdBuffer);
typedef VK_RESULT (VKAPI *vkBeginCommandBufferType)(VK_CMD_BUFFER cmdBuffer, const VK_CMD_BUFFER_BEGIN_INFO* pBeginInfo);
typedef VK_RESULT (VKAPI *vkEndCommandBufferType)(VK_CMD_BUFFER cmdBuffer);
typedef VK_RESULT (VKAPI *vkResetCommandBufferType)(VK_CMD_BUFFER cmdBuffer);
typedef void      (VKAPI *vkCmdBindPipelineType)(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, VK_PIPELINE pipeline);
typedef void      (VKAPI *vkCmdBindDynamicStateObjectType)(VK_CMD_BUFFER cmdBuffer, VK_STATE_BIND_POINT stateBindPoint, VK_DYNAMIC_STATE_OBJECT state);
typedef void      (VKAPI *vkCmdBindDescriptorSetsType)(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, VK_DESCRIPTOR_SET_LAYOUT_CHAIN layoutChain, uint32_t layoutChainSlot, uint32_t count, const VK_DESCRIPTOR_SET* pDescriptorSets, const uint32_t* pUserData);
typedef void      (VKAPI *vkCmdBindIndexBufferType)(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, VK_INDEX_TYPE indexType);
typedef void      (VKAPI *vkCmdBindVertexBufferType)(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, uint32_t binding);
typedef void      (VKAPI *vkCmdDrawType)(VK_CMD_BUFFER cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount);
typedef void      (VKAPI *vkCmdDrawIndexedType)(VK_CMD_BUFFER cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount);
typedef void      (VKAPI *vkCmdDrawIndirectType)(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, uint32_t count, uint32_t stride);
typedef void      (VKAPI *vkCmdDrawIndexedIndirectType)(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset, uint32_t count, uint32_t stride);
typedef void      (VKAPI *vkCmdDispatchType)(VK_CMD_BUFFER cmdBuffer, uint32_t x, uint32_t y, uint32_t z);
typedef void      (VKAPI *vkCmdDispatchIndirectType)(VK_CMD_BUFFER cmdBuffer, VK_BUFFER buffer, VK_GPU_SIZE offset);
typedef void      (VKAPI *vkCmdCopyBufferType)(VK_CMD_BUFFER cmdBuffer, VK_BUFFER srcBuffer, VK_BUFFER destBuffer, uint32_t regionCount, const VK_BUFFER_COPY* pRegions);
typedef void      (VKAPI *vkCmdCopyImageType)(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout, uint32_t regionCount, const VK_IMAGE_COPY* pRegions);
typedef void      (VKAPI *vkCmdBlitImageType)(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout, uint32_t regionCount, const VK_IMAGE_BLIT* pRegions);
typedef void      (VKAPI *vkCmdCopyBufferToImageType)(VK_CMD_BUFFER cmdBuffer, VK_BUFFER srcBuffer, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout, uint32_t regionCount, const VK_BUFFER_IMAGE_COPY* pRegions);
typedef void      (VKAPI *vkCmdCopyImageToBufferType)(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_BUFFER destBuffer, uint32_t regionCount, const VK_BUFFER_IMAGE_COPY* pRegions);
typedef void      (VKAPI *vkCmdCloneImageDataType)(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout);
typedef void      (VKAPI *vkCmdUpdateBufferType)(VK_CMD_BUFFER cmdBuffer, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset, VK_GPU_SIZE dataSize, const uint32_t* pData);
typedef void      (VKAPI *vkCmdFillBufferType)(VK_CMD_BUFFER cmdBuffer, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset, VK_GPU_SIZE fillSize, uint32_t data);
typedef void      (VKAPI *vkCmdClearColorImageType)(VK_CMD_BUFFER cmdBuffer, VK_IMAGE image, VK_IMAGE_LAYOUT imageLayout, VK_CLEAR_COLOR color, uint32_t rangeCount, const VK_IMAGE_SUBRESOURCE_RANGE* pRanges);
typedef void      (VKAPI *vkCmdClearDepthStencilType)(VK_CMD_BUFFER cmdBuffer, VK_IMAGE image, VK_IMAGE_LAYOUT imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const VK_IMAGE_SUBRESOURCE_RANGE* pRanges);
typedef void      (VKAPI *vkCmdResolveImageType)(VK_CMD_BUFFER cmdBuffer, VK_IMAGE srcImage, VK_IMAGE_LAYOUT srcImageLayout, VK_IMAGE destImage, VK_IMAGE_LAYOUT destImageLayout, uint32_t rectCount, const VK_IMAGE_RESOLVE* pRects);
typedef void      (VKAPI *vkCmdSetEventType)(VK_CMD_BUFFER cmdBuffer, VK_EVENT event, VK_PIPE_EVENT pipeEvent);
typedef void      (VKAPI *vkCmdResetEventType)(VK_CMD_BUFFER cmdBuffer, VK_EVENT event, VK_PIPE_EVENT pipeEvent);
typedef void      (VKAPI *vkCmdWaitEventsType)(VK_CMD_BUFFER cmdBuffer, const VK_EVENT_WAIT_INFO* pWaitInfo);
typedef void      (VKAPI *vkCmdPipelineBarrierType)(VK_CMD_BUFFER cmdBuffer, const VK_PIPELINE_BARRIER* pBarrier);
typedef void      (VKAPI *vkCmdBeginQueryType)(VK_CMD_BUFFER cmdBuffer, VK_QUERY_POOL queryPool, uint32_t slot, VK_FLAGS flags);
typedef void      (VKAPI *vkCmdEndQueryType)(VK_CMD_BUFFER cmdBuffer, VK_QUERY_POOL queryPool, uint32_t slot);
typedef void      (VKAPI *vkCmdResetQueryPoolType)(VK_CMD_BUFFER cmdBuffer, VK_QUERY_POOL queryPool, uint32_t startQuery, uint32_t queryCount);
typedef void      (VKAPI *vkCmdWriteTimestampType)(VK_CMD_BUFFER cmdBuffer, VK_TIMESTAMP_TYPE timestampType, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset);
typedef void      (VKAPI *vkCmdInitAtomicCountersType)(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData);
typedef void      (VKAPI *vkCmdLoadAtomicCountersType)(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VK_BUFFER srcBuffer, VK_GPU_SIZE srcOffset);
typedef void      (VKAPI *vkCmdSaveAtomicCountersType)(VK_CMD_BUFFER cmdBuffer, VK_PIPELINE_BIND_POINT pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VK_BUFFER destBuffer, VK_GPU_SIZE destOffset);
typedef VK_RESULT (VKAPI *vkCreateFramebufferType)(VK_DEVICE device, const VK_FRAMEBUFFER_CREATE_INFO* pCreateInfo, VK_FRAMEBUFFER* pFramebuffer);
typedef VK_RESULT (VKAPI *vkCreateRenderPassType)(VK_DEVICE device, const VK_RENDER_PASS_CREATE_INFO* pCreateInfo, VK_RENDER_PASS* pRenderPass);
typedef void      (VKAPI *vkCmdBeginRenderPassType)(VK_CMD_BUFFER cmdBuffer, const VK_RENDER_PASS_BEGIN* pRenderPassBegin);
typedef void      (VKAPI *vkCmdEndRenderPassType)(VK_CMD_BUFFER cmdBuffer, VK_RENDER_PASS renderPass);

#ifdef VK_PROTOTYPES

// GPU initialization

VK_RESULT VKAPI vkCreateInstance(
    const VK_INSTANCE_CREATE_INFO*             pCreateInfo,
    VK_INSTANCE*                               pInstance);

VK_RESULT VKAPI vkDestroyInstance(
    VK_INSTANCE                                 instance);

VK_RESULT VKAPI vkEnumerateGpus(
    VK_INSTANCE                                 instance,
    uint32_t                                    maxGpus,
    uint32_t*                                   pGpuCount,
    VK_PHYSICAL_GPU*                            pGpus);

VK_RESULT VKAPI vkGetGpuInfo(
    VK_PHYSICAL_GPU                             gpu,
    VK_PHYSICAL_GPU_INFO_TYPE                   infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

void * VKAPI vkGetProcAddr(
    VK_PHYSICAL_GPU                             gpu,
    const char*                                 pName);

// Device functions

VK_RESULT VKAPI vkCreateDevice(
    VK_PHYSICAL_GPU                             gpu,
    const VK_DEVICE_CREATE_INFO*                pCreateInfo,
    VK_DEVICE*                                  pDevice);

VK_RESULT VKAPI vkDestroyDevice(
    VK_DEVICE                                   device);

// Extension discovery functions

VK_RESULT VKAPI vkGetExtensionSupport(
    VK_PHYSICAL_GPU                             gpu,
    const char*                                 pExtName);

// Layer discovery functions

VK_RESULT VKAPI vkEnumerateLayers(
    VK_PHYSICAL_GPU                             gpu,
    size_t                                      maxLayerCount,
    size_t                                      maxStringSize,
    size_t*                                     pOutLayerCount,
    char* const*                                pOutLayers,
    void*                                       pReserved);

// Queue functions

VK_RESULT VKAPI vkGetDeviceQueue(
    VK_DEVICE                                   device,
    uint32_t                                    queueNodeIndex,
    uint32_t                                    queueIndex,
    VK_QUEUE*                                   pQueue);

VK_RESULT VKAPI vkQueueSubmit(
    VK_QUEUE                                    queue,
    uint32_t                                    cmdBufferCount,
    const VK_CMD_BUFFER*                        pCmdBuffers,
    VK_FENCE                                    fence);

VK_RESULT VKAPI vkQueueAddMemReference(
    VK_QUEUE                                    queue,
    VK_GPU_MEMORY                               mem);

VK_RESULT VKAPI vkQueueRemoveMemReference(
    VK_QUEUE                                    queue,
    VK_GPU_MEMORY                               mem);

VK_RESULT VKAPI vkQueueWaitIdle(
    VK_QUEUE                                    queue);

VK_RESULT VKAPI vkDeviceWaitIdle(
    VK_DEVICE                                   device);

// Memory functions

VK_RESULT VKAPI vkAllocMemory(
    VK_DEVICE                                   device,
    const VK_MEMORY_ALLOC_INFO*                 pAllocInfo,
    VK_GPU_MEMORY*                              pMem);

VK_RESULT VKAPI vkFreeMemory(
    VK_GPU_MEMORY                               mem);

VK_RESULT VKAPI vkSetMemoryPriority(
    VK_GPU_MEMORY                               mem,
    VK_MEMORY_PRIORITY                          priority);

VK_RESULT VKAPI vkMapMemory(
    VK_GPU_MEMORY                               mem,
    VK_FLAGS                                    flags,                // Reserved
    void**                                      ppData);

VK_RESULT VKAPI vkUnmapMemory(
    VK_GPU_MEMORY                               mem);

VK_RESULT VKAPI vkPinSystemMemory(
    VK_DEVICE                                   device,
    const void*                                 pSysMem,
    size_t                                      memSize,
    VK_GPU_MEMORY*                              pMem);

// Multi-device functions

VK_RESULT VKAPI vkGetMultiGpuCompatibility(
    VK_PHYSICAL_GPU                             gpu0,
    VK_PHYSICAL_GPU                             gpu1,
    VK_GPU_COMPATIBILITY_INFO*                  pInfo);

VK_RESULT VKAPI vkOpenSharedMemory(
    VK_DEVICE                                   device,
    const VK_MEMORY_OPEN_INFO*                  pOpenInfo,
    VK_GPU_MEMORY*                              pMem);

VK_RESULT VKAPI vkOpenSharedSemaphore(
    VK_DEVICE                                   device,
    const VK_SEMAPHORE_OPEN_INFO*               pOpenInfo,
    VK_SEMAPHORE*                               pSemaphore);

VK_RESULT VKAPI vkOpenPeerMemory(
    VK_DEVICE                                   device,
    const VK_PEER_MEMORY_OPEN_INFO*             pOpenInfo,
    VK_GPU_MEMORY*                              pMem);

VK_RESULT VKAPI vkOpenPeerImage(
    VK_DEVICE                                   device,
    const VK_PEER_IMAGE_OPEN_INFO*              pOpenInfo,
    VK_IMAGE*                                   pImage,
    VK_GPU_MEMORY*                              pMem);

// Generic API object functions

VK_RESULT VKAPI vkDestroyObject(
    VK_OBJECT                                   object);

VK_RESULT VKAPI vkGetObjectInfo(
    VK_BASE_OBJECT                              object,
    VK_OBJECT_INFO_TYPE                         infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

VK_RESULT VKAPI vkBindObjectMemory(
    VK_OBJECT                                   object,
    uint32_t                                    allocationIdx,
    VK_GPU_MEMORY                               mem,
    VK_GPU_SIZE                                 memOffset);

VK_RESULT VKAPI vkBindObjectMemoryRange(
    VK_OBJECT                                   object,
    uint32_t                                    allocationIdx,
    VK_GPU_SIZE                                 rangeOffset,
    VK_GPU_SIZE                                 rangeSize,
    VK_GPU_MEMORY                               mem,
    VK_GPU_SIZE                                 memOffset);

VK_RESULT VKAPI vkBindImageMemoryRange(
    VK_IMAGE                                    image,
    uint32_t                                    allocationIdx,
    const VK_IMAGE_MEMORY_BIND_INFO*            bindInfo,
    VK_GPU_MEMORY                               mem,
    VK_GPU_SIZE                                 memOffset);

// Fence functions

VK_RESULT VKAPI vkCreateFence(
    VK_DEVICE                                   device,
    const VK_FENCE_CREATE_INFO*                 pCreateInfo,
    VK_FENCE*                                   pFence);

VK_RESULT VKAPI vkResetFences(
    VK_DEVICE                                   device,
    uint32_t                                    fenceCount,
    VK_FENCE*                                   pFences);

VK_RESULT VKAPI vkGetFenceStatus(
    VK_FENCE fence);

VK_RESULT VKAPI vkWaitForFences(
    VK_DEVICE                                   device,
    uint32_t                                    fenceCount,
    const VK_FENCE*                             pFences,
    bool32_t                                    waitAll,
    uint64_t                                    timeout); // timeout in nanoseconds

// Queue semaphore functions

VK_RESULT VKAPI vkCreateSemaphore(
    VK_DEVICE                                   device,
    const VK_SEMAPHORE_CREATE_INFO*             pCreateInfo,
    VK_SEMAPHORE*                               pSemaphore);

VK_RESULT VKAPI vkQueueSignalSemaphore(
    VK_QUEUE                                    queue,
    VK_SEMAPHORE                                semaphore);

VK_RESULT VKAPI vkQueueWaitSemaphore(
    VK_QUEUE                                    queue,
    VK_SEMAPHORE                                semaphore);

// Event functions

VK_RESULT VKAPI vkCreateEvent(
    VK_DEVICE                                   device,
    const VK_EVENT_CREATE_INFO*                 pCreateInfo,
    VK_EVENT*                                   pEvent);

VK_RESULT VKAPI vkGetEventStatus(
    VK_EVENT                                    event);

VK_RESULT VKAPI vkSetEvent(
    VK_EVENT                                    event);

VK_RESULT VKAPI vkResetEvent(
    VK_EVENT                                    event);

// Query functions

VK_RESULT VKAPI vkCreateQueryPool(
    VK_DEVICE                                   device,
    const VK_QUERY_POOL_CREATE_INFO*            pCreateInfo,
    VK_QUERY_POOL*                              pQueryPool);

VK_RESULT VKAPI vkGetQueryPoolResults(
    VK_QUERY_POOL                               queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    size_t*                                     pDataSize,
    void*                                       pData);

// Format capabilities

VK_RESULT VKAPI vkGetFormatInfo(
    VK_DEVICE                                   device,
    VK_FORMAT                                   format,
    VK_FORMAT_INFO_TYPE                         infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

// Buffer functions

VK_RESULT VKAPI vkCreateBuffer(
    VK_DEVICE                                   device,
    const VK_BUFFER_CREATE_INFO*                pCreateInfo,
    VK_BUFFER*                                  pBuffer);

// Buffer view functions

VK_RESULT VKAPI vkCreateBufferView(
    VK_DEVICE                                   device,
    const VK_BUFFER_VIEW_CREATE_INFO*           pCreateInfo,
    VK_BUFFER_VIEW*                             pView);

// Image functions

VK_RESULT VKAPI vkCreateImage(
    VK_DEVICE                                   device,
    const VK_IMAGE_CREATE_INFO*                 pCreateInfo,
    VK_IMAGE*                                   pImage);

VK_RESULT VKAPI vkGetImageSubresourceInfo(
    VK_IMAGE                                    image,
    const VK_IMAGE_SUBRESOURCE*                 pSubresource,
    VK_SUBRESOURCE_INFO_TYPE                    infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

// Image view functions

VK_RESULT VKAPI vkCreateImageView(
    VK_DEVICE                                   device,
    const VK_IMAGE_VIEW_CREATE_INFO*            pCreateInfo,
    VK_IMAGE_VIEW*                              pView);

VK_RESULT VKAPI vkCreateColorAttachmentView(
    VK_DEVICE                                   device,
    const VK_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pCreateInfo,
    VK_COLOR_ATTACHMENT_VIEW*                   pView);

VK_RESULT VKAPI vkCreateDepthStencilView(
    VK_DEVICE                                   device,
    const VK_DEPTH_STENCIL_VIEW_CREATE_INFO*    pCreateInfo,
    VK_DEPTH_STENCIL_VIEW*                      pView);

// Shader functions

VK_RESULT VKAPI vkCreateShader(
    VK_DEVICE                                   device,
    const VK_SHADER_CREATE_INFO*                pCreateInfo,
    VK_SHADER*                                  pShader);

// Pipeline functions

VK_RESULT VKAPI vkCreateGraphicsPipeline(
    VK_DEVICE                                   device,
    const VK_GRAPHICS_PIPELINE_CREATE_INFO*     pCreateInfo,
    VK_PIPELINE*                                pPipeline);

VK_RESULT VKAPI vkCreateGraphicsPipelineDerivative(
    VK_DEVICE                                   device,
    const VK_GRAPHICS_PIPELINE_CREATE_INFO*     pCreateInfo,
    VK_PIPELINE                                 basePipeline,
    VK_PIPELINE*                                pPipeline);

VK_RESULT VKAPI vkCreateComputePipeline(
    VK_DEVICE                                   device,
    const VK_COMPUTE_PIPELINE_CREATE_INFO*      pCreateInfo,
    VK_PIPELINE*                                pPipeline);

VK_RESULT VKAPI vkStorePipeline(
    VK_PIPELINE                                 pipeline,
    size_t*                                     pDataSize,
    void*                                       pData);

VK_RESULT VKAPI vkLoadPipeline(
    VK_DEVICE                                   device,
    size_t                                      dataSize,
    const void*                                 pData,
    VK_PIPELINE*                                pPipeline);

VK_RESULT VKAPI vkLoadPipelineDerivative(
    VK_DEVICE                                   device,
    size_t                                      dataSize,
    const void*                                 pData,
    VK_PIPELINE                                 basePipeline,
    VK_PIPELINE*                                pPipeline);

// Sampler functions

VK_RESULT VKAPI vkCreateSampler(
    VK_DEVICE                                   device,
    const VK_SAMPLER_CREATE_INFO*               pCreateInfo,
    VK_SAMPLER*                                 pSampler);

// Descriptor set functions

VK_RESULT VKAPI vkCreateDescriptorSetLayout(
    VK_DEVICE                                    device,
    const VK_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*  pCreateInfo,
    VK_DESCRIPTOR_SET_LAYOUT*                    pSetLayout);

VK_RESULT VKAPI vkCreateDescriptorSetLayoutChain(
    VK_DEVICE                                    device,
    uint32_t                                     setLayoutArrayCount,
    const VK_DESCRIPTOR_SET_LAYOUT*              pSetLayoutArray,
    VK_DESCRIPTOR_SET_LAYOUT_CHAIN*              pLayoutChain);

VK_RESULT VKAPI vkBeginDescriptorPoolUpdate(
    VK_DEVICE                                    device,
    VK_DESCRIPTOR_UPDATE_MODE                    updateMode);

VK_RESULT VKAPI vkEndDescriptorPoolUpdate(
    VK_DEVICE                                    device,
    VK_CMD_BUFFER                                cmd);

VK_RESULT VKAPI vkCreateDescriptorPool(
    VK_DEVICE                                    device,
    VK_DESCRIPTOR_POOL_USAGE                     poolUsage,
    uint32_t                                     maxSets,
    const VK_DESCRIPTOR_POOL_CREATE_INFO*        pCreateInfo,
    VK_DESCRIPTOR_POOL*                          pDescriptorPool);

VK_RESULT VKAPI vkResetDescriptorPool(
    VK_DESCRIPTOR_POOL                           descriptorPool);

VK_RESULT VKAPI vkAllocDescriptorSets(
    VK_DESCRIPTOR_POOL                           descriptorPool,
    VK_DESCRIPTOR_SET_USAGE                      setUsage,
    uint32_t                                     count,
    const VK_DESCRIPTOR_SET_LAYOUT*              pSetLayouts,
    VK_DESCRIPTOR_SET*                           pDescriptorSets,
    uint32_t*                                    pCount);

void VKAPI vkClearDescriptorSets(
    VK_DESCRIPTOR_POOL                           descriptorPool,
    uint32_t                                     count,
    const VK_DESCRIPTOR_SET*                     pDescriptorSets);

void VKAPI vkUpdateDescriptors(
    VK_DESCRIPTOR_SET                            descriptorSet,
    uint32_t                                     updateCount,
    const void**                                 pUpdateArray);

// State object functions

VK_RESULT VKAPI vkCreateDynamicViewportState(
    VK_DEVICE                                   device,
    const VK_DYNAMIC_VP_STATE_CREATE_INFO*      pCreateInfo,
    VK_DYNAMIC_VP_STATE_OBJECT*                 pState);

VK_RESULT VKAPI vkCreateDynamicRasterState(
    VK_DEVICE                                   device,
    const VK_DYNAMIC_RS_STATE_CREATE_INFO*      pCreateInfo,
    VK_DYNAMIC_RS_STATE_OBJECT*                 pState);

VK_RESULT VKAPI vkCreateDynamicColorBlendState(
    VK_DEVICE                                   device,
    const VK_DYNAMIC_CB_STATE_CREATE_INFO*      pCreateInfo,
    VK_DYNAMIC_CB_STATE_OBJECT*                 pState);

VK_RESULT VKAPI vkCreateDynamicDepthStencilState(
    VK_DEVICE                                   device,
    const VK_DYNAMIC_DS_STATE_CREATE_INFO*      pCreateInfo,
    VK_DYNAMIC_DS_STATE_OBJECT*                 pState);

// Command buffer functions

VK_RESULT VKAPI vkCreateCommandBuffer(
    VK_DEVICE                                   device,
    const VK_CMD_BUFFER_CREATE_INFO*            pCreateInfo,
    VK_CMD_BUFFER*                              pCmdBuffer);

VK_RESULT VKAPI vkBeginCommandBuffer(
    VK_CMD_BUFFER                               cmdBuffer,
    const VK_CMD_BUFFER_BEGIN_INFO*             pBeginInfo);

VK_RESULT VKAPI vkEndCommandBuffer(
    VK_CMD_BUFFER                               cmdBuffer);

VK_RESULT VKAPI vkResetCommandBuffer(
    VK_CMD_BUFFER                               cmdBuffer);

// Command buffer building functions

void VKAPI vkCmdBindPipeline(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_PIPELINE_BIND_POINT                      pipelineBindPoint,
    VK_PIPELINE                                 pipeline);

void VKAPI vkCmdBindDynamicStateObject(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_STATE_BIND_POINT                         stateBindPoint,
    VK_DYNAMIC_STATE_OBJECT                     dynamicState);

void VKAPI vkCmdBindDescriptorSets(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_PIPELINE_BIND_POINT                      pipelineBindPoint,
    VK_DESCRIPTOR_SET_LAYOUT_CHAIN              layoutChain,
    uint32_t                                    layoutChainSlot,
    uint32_t                                    count,
    const VK_DESCRIPTOR_SET*                    pDescriptorSets,
    const uint32_t    *                         pUserData);

void VKAPI vkCmdBindIndexBuffer(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_BUFFER                                   buffer,
    VK_GPU_SIZE                                 offset,
    VK_INDEX_TYPE                               indexType);

void VKAPI vkCmdBindVertexBuffer(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_BUFFER                                   buffer,
    VK_GPU_SIZE                                 offset,
    uint32_t                                    binding);

void VKAPI vkCmdDraw(
    VK_CMD_BUFFER                               cmdBuffer,
    uint32_t                                    firstVertex,
    uint32_t                                    vertexCount,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount);

void VKAPI vkCmdDrawIndexed(
    VK_CMD_BUFFER                               cmdBuffer,
    uint32_t                                    firstIndex,
    uint32_t                                    indexCount,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount);

void VKAPI vkCmdDrawIndirect(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_BUFFER                                   buffer,
    VK_GPU_SIZE                                 offset,
    uint32_t                                    count,
    uint32_t                                    stride);

void VKAPI vkCmdDrawIndexedIndirect(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_BUFFER                                   buffer,
    VK_GPU_SIZE                                 offset,
    uint32_t                                    count,
    uint32_t                                    stride);

void VKAPI vkCmdDispatch(
    VK_CMD_BUFFER                               cmdBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z);
void VKAPI vkCmdDispatchIndirect(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_BUFFER                                   buffer,
    VK_GPU_SIZE                                 offset);

void VKAPI vkCmdCopyBuffer(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_BUFFER                                   srcBuffer,
    VK_BUFFER                                   destBuffer,
    uint32_t                                    regionCount,
    const VK_BUFFER_COPY*                       pRegions);

void VKAPI vkCmdCopyImage(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_IMAGE                                    srcImage,
    VK_IMAGE_LAYOUT                             srcImageLayout,
    VK_IMAGE                                    destImage,
    VK_IMAGE_LAYOUT                             destImageLayout,
    uint32_t                                    regionCount,
    const VK_IMAGE_COPY*                        pRegions);

void VKAPI vkCmdBlitImage(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_IMAGE                                    srcImage,
    VK_IMAGE_LAYOUT                             srcImageLayout,
    VK_IMAGE                                    destImage,
    VK_IMAGE_LAYOUT                             destImageLayout,
    uint32_t                                    regionCount,
    const VK_IMAGE_BLIT*                        pRegions);

void VKAPI vkCmdCopyBufferToImage(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_BUFFER                                   srcBuffer,
    VK_IMAGE                                    destImage,
    VK_IMAGE_LAYOUT                             destImageLayout,
    uint32_t                                    regionCount,
    const VK_BUFFER_IMAGE_COPY*                 pRegions);

void VKAPI vkCmdCopyImageToBuffer(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_IMAGE                                    srcImage,
    VK_IMAGE_LAYOUT                             srcImageLayout,
    VK_BUFFER                                   destBuffer,
    uint32_t                                    regionCount,
    const VK_BUFFER_IMAGE_COPY*                 pRegions);

void VKAPI vkCmdCloneImageData(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_IMAGE                                    srcImage,
    VK_IMAGE_LAYOUT                             srcImageLayout,
    VK_IMAGE                                    destImage,
    VK_IMAGE_LAYOUT                             destImageLayout);

void VKAPI vkCmdUpdateBuffer(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_BUFFER                                   destBuffer,
    VK_GPU_SIZE                                 destOffset,
    VK_GPU_SIZE                                 dataSize,
    const uint32_t*                             pData);

void VKAPI vkCmdFillBuffer(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_BUFFER                                   destBuffer,
    VK_GPU_SIZE                                 destOffset,
    VK_GPU_SIZE                                 fillSize,
    uint32_t                                    data);

void VKAPI vkCmdClearColorImage(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_IMAGE                                    image,
    VK_IMAGE_LAYOUT                             imageLayout,
    VK_CLEAR_COLOR                              color,
    uint32_t                                    rangeCount,
    const VK_IMAGE_SUBRESOURCE_RANGE*           pRanges);

void VKAPI vkCmdClearDepthStencil(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_IMAGE                                    image,
    VK_IMAGE_LAYOUT                             imageLayout,
    float                                       depth,
    uint32_t                                    stencil,
    uint32_t                                    rangeCount,
    const VK_IMAGE_SUBRESOURCE_RANGE*           pRanges);

void VKAPI vkCmdResolveImage(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_IMAGE                                    srcImage,
    VK_IMAGE_LAYOUT                             srcImageLayout,
    VK_IMAGE                                    destImage,
    VK_IMAGE_LAYOUT                             destImageLayout,
    uint32_t                                    rectCount,
    const VK_IMAGE_RESOLVE*                     pRects);

void VKAPI vkCmdSetEvent(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_EVENT                                    event,
    VK_PIPE_EVENT                               pipeEvent);

void VKAPI vkCmdResetEvent(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_EVENT                                    event,
    VK_PIPE_EVENT                               pipeEvent);

void VKAPI vkCmdWaitEvents(
    VK_CMD_BUFFER                               cmdBuffer,
    const VK_EVENT_WAIT_INFO*                   pWaitInfo);

void VKAPI vkCmdPipelineBarrier(
    VK_CMD_BUFFER                               cmdBuffer,
    const VK_PIPELINE_BARRIER*                  pBarrier);

void VKAPI vkCmdBeginQuery(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_QUERY_POOL                               queryPool,
    uint32_t                                    slot,
    VK_FLAGS                                    flags);

void VKAPI vkCmdEndQuery(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_QUERY_POOL                               queryPool,
    uint32_t                                    slot);

void VKAPI vkCmdResetQueryPool(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_QUERY_POOL                               queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount);

void VKAPI vkCmdWriteTimestamp(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_TIMESTAMP_TYPE                           timestampType,
    VK_BUFFER                                   destBuffer,
    VK_GPU_SIZE                                 destOffset);

void VKAPI vkCmdInitAtomicCounters(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_PIPELINE_BIND_POINT                      pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    const uint32_t*                             pData);

void VKAPI vkCmdLoadAtomicCounters(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_PIPELINE_BIND_POINT                      pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    VK_BUFFER                                   srcBuffer,
    VK_GPU_SIZE                                 srcOffset);

void VKAPI vkCmdSaveAtomicCounters(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_PIPELINE_BIND_POINT                      pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    VK_BUFFER                                   destBuffer,
    VK_GPU_SIZE                                 destOffset);

VK_RESULT VKAPI vkCreateFramebuffer(
    VK_DEVICE                                   device,
    const VK_FRAMEBUFFER_CREATE_INFO*           pCreateInfo,
    VK_FRAMEBUFFER*                             pFramebuffer);

VK_RESULT VKAPI vkCreateRenderPass(
    VK_DEVICE                                   device,
    const VK_RENDER_PASS_CREATE_INFO*           pCreateInfo,
    VK_RENDER_PASS*                             pRenderPass);

void VKAPI vkCmdBeginRenderPass(
    VK_CMD_BUFFER                               cmdBuffer,
    const VK_RENDER_PASS_BEGIN*                 pRenderPassBegin);

void VKAPI vkCmdEndRenderPass(
    VK_CMD_BUFFER                               cmdBuffer,
    VK_RENDER_PASS                              renderPass);

#endif // VK_PROTOTYPES

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __VULKAN_H__
