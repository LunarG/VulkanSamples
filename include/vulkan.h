//
// File: vulkan.h
//
/*
** Copyright (c) 2014-2015 The Khronos Group Inc.
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

#include "vk_platform.h"

// Vulkan API version supported by this file
#define VK_API_VERSION VK_MAKE_VERSION(0, 80, 0)

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

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkBaseObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDevice, VkBaseObject)
VK_DEFINE_SUBCLASS_HANDLE(VkQueue, VkBaseObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDeviceMemory, VkBaseObject)
VK_DEFINE_SUBCLASS_HANDLE(VkObject, VkBaseObject)
VK_DEFINE_SUBCLASS_HANDLE(VkBuffer, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkBufferView, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkImage, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkImageView, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkColorAttachmentView, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDepthStencilView, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkShader, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkPipeline, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkSampler, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDescriptorSet, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDescriptorSetLayout, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDescriptorSetLayoutChain, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDescriptorPool, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDynamicStateObject, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDynamicVpState, VkDynamicStateObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDynamicRsState, VkDynamicStateObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDynamicCbState, VkDynamicStateObject)
VK_DEFINE_SUBCLASS_HANDLE(VkDynamicDsState, VkDynamicStateObject)
VK_DEFINE_SUBCLASS_HANDLE(VkCmdBuffer, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkFence, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkSemaphore, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkEvent, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkQueryPool, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkFramebuffer, VkObject)
VK_DEFINE_SUBCLASS_HANDLE(VkRenderPass, VkObject)

#define VK_MAX_PHYSICAL_DEVICE_NAME 256
#define VK_MAX_EXTENSION_NAME      256

#define VK_LOD_CLAMP_NONE       MAX_FLOAT
#define VK_LAST_MIP_OR_SLICE    0xffffffff

#define VK_WHOLE_SIZE           UINT64_MAX

#define VK_TRUE  1
#define VK_FALSE 0

#define VK_NULL_HANDLE 0

// This macro defines INT_MAX in enumerations to force compilers to use 32 bits
// to represent them. This may or may not be necessary on some compilers. The
// option to compile it out may allow compilers that warn about missing enumerants
// in switch statements to be silenced.
// Using this macro is not needed for flag bit enums because those aren't used
// as storage type anywhere.
#define VK_MAX_ENUM(Prefix) VK_##Prefix##_MAX_ENUM = 0x7FFFFFFF

// This macro defines the BEGIN_RANGE, END_RANGE, NUM, and MAX_ENUM constants for
// the enumerations.
#define VK_ENUM_RANGE(Prefix, First, Last) \
    VK_##Prefix##_BEGIN_RANGE                               = VK_##Prefix##_##First, \
    VK_##Prefix##_END_RANGE                                 = VK_##Prefix##_##Last, \
    VK_NUM_##Prefix                                         = (VK_##Prefix##_END_RANGE - VK_##Prefix##_BEGIN_RANGE + 1), \
    VK_MAX_ENUM(Prefix)

// This is a helper macro to define the value of flag bit enum values.
#define VK_BIT(bit)     (1 << (bit))

// ------------------------------------------------------------------------------------------------
// Enumerations

typedef enum VkMemoryPriority_
{
    VK_MEMORY_PRIORITY_UNUSED                               = 0x00000000,
    VK_MEMORY_PRIORITY_VERY_LOW                             = 0x00000001,
    VK_MEMORY_PRIORITY_LOW                                  = 0x00000002,
    VK_MEMORY_PRIORITY_NORMAL                               = 0x00000003,
    VK_MEMORY_PRIORITY_HIGH                                 = 0x00000004,
    VK_MEMORY_PRIORITY_VERY_HIGH                            = 0x00000005,

    VK_ENUM_RANGE(MEMORY_PRIORITY, UNUSED, VERY_HIGH)
} VkMemoryPriority;

typedef enum VkImageLayout_
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

    VK_ENUM_RANGE(IMAGE_LAYOUT, UNDEFINED, TRANSFER_DESTINATION_OPTIMAL)
} VkImageLayout;

typedef enum VkPipeEvent_
{
    VK_PIPE_EVENT_TOP_OF_PIPE                               = 0x00000001,   // Set event before the device starts processing subsequent command
    VK_PIPE_EVENT_VERTEX_PROCESSING_COMPLETE                = 0x00000002,   // Set event when all pending vertex processing is complete
    VK_PIPE_EVENT_LOCAL_FRAGMENT_PROCESSING_COMPLETE        = 0x00000003,   // Set event when all pending fragment shader executions are complete, within each fragment location
    VK_PIPE_EVENT_FRAGMENT_PROCESSING_COMPLETE              = 0x00000004,   // Set event when all pending fragment shader executions are complete
    VK_PIPE_EVENT_GRAPHICS_PIPELINE_COMPLETE                = 0x00000005,   // Set event when all pending graphics operations are complete
    VK_PIPE_EVENT_COMPUTE_PIPELINE_COMPLETE                 = 0x00000006,   // Set event when all pending compute operations are complete
    VK_PIPE_EVENT_TRANSFER_COMPLETE                         = 0x00000007,   // Set event when all pending transfer operations are complete
    VK_PIPE_EVENT_COMMANDS_COMPLETE                         = 0x00000008,   // Set event when all pending work is complete

    VK_ENUM_RANGE(PIPE_EVENT, TOP_OF_PIPE, COMMANDS_COMPLETE)
} VkPipeEvent;

typedef enum VkWaitEvent_
{
    VK_WAIT_EVENT_TOP_OF_PIPE                               = 0x00000001,   // Wait event before the device starts processing subsequent commands
    VK_WAIT_EVENT_BEFORE_RASTERIZATION                      = 0x00000002,   // Wait event before rasterizing subsequent primitives

    VK_ENUM_RANGE(WAIT_EVENT, TOP_OF_PIPE, BEFORE_RASTERIZATION)
} VkWaitEvent;

typedef enum VkAttachmentLoadOp_
{
    VK_ATTACHMENT_LOAD_OP_LOAD                              = 0x00000000,
    VK_ATTACHMENT_LOAD_OP_CLEAR                             = 0x00000001,
    VK_ATTACHMENT_LOAD_OP_DONT_CARE                         = 0x00000002,

    VK_ENUM_RANGE(ATTACHMENT_LOAD_OP, LOAD, DONT_CARE)
} VkAttachmentLoadOp;

typedef enum VkAttachmentStoreOp_
{
    VK_ATTACHMENT_STORE_OP_STORE                            = 0x00000000,
    VK_ATTACHMENT_STORE_OP_RESOLVE_MSAA                     = 0x00000001,
    VK_ATTACHMENT_STORE_OP_DONT_CARE                        = 0x00000002,

    VK_ENUM_RANGE(ATTACHMENT_STORE_OP, STORE, DONT_CARE)
} VkAttachmentStoreOp;

typedef enum VkImageType_
{
    VK_IMAGE_TYPE_1D                                        = 0x00000000,
    VK_IMAGE_TYPE_2D                                        = 0x00000001,
    VK_IMAGE_TYPE_3D                                        = 0x00000002,

    VK_ENUM_RANGE(IMAGE_TYPE, 1D, 3D)
} VkImageType;

typedef enum VkImageTiling_
{
    VK_IMAGE_TILING_LINEAR                                  = 0x00000000,
    VK_IMAGE_TILING_OPTIMAL                                 = 0x00000001,

    VK_ENUM_RANGE(IMAGE_TILING, LINEAR, OPTIMAL)
} VkImageTiling;

typedef enum VkImageViewType_
{
    VK_IMAGE_VIEW_TYPE_1D                                   = 0x00000000,
    VK_IMAGE_VIEW_TYPE_2D                                   = 0x00000001,
    VK_IMAGE_VIEW_TYPE_3D                                   = 0x00000002,
    VK_IMAGE_VIEW_TYPE_CUBE                                 = 0x00000003,

    VK_ENUM_RANGE(IMAGE_VIEW_TYPE, 1D, CUBE)
} VkImageViewType;

typedef enum VkImageAspect_
{
    VK_IMAGE_ASPECT_COLOR                                   = 0x00000000,
    VK_IMAGE_ASPECT_DEPTH                                   = 0x00000001,
    VK_IMAGE_ASPECT_STENCIL                                 = 0x00000002,

    VK_ENUM_RANGE(IMAGE_ASPECT, COLOR, STENCIL)
} VkImageAspect;

typedef enum VkBufferViewType_
{
    VK_BUFFER_VIEW_TYPE_RAW                                 = 0x00000000,   // Raw buffer without special structure (UBO, SSBO)
    VK_BUFFER_VIEW_TYPE_FORMATTED                           = 0x00000001,   // Buffer with format (TBO, IBO)

    VK_ENUM_RANGE(BUFFER_VIEW_TYPE, RAW, FORMATTED)
} VkBufferViewType;

typedef enum VkChannelSwizzle_
{
    VK_CHANNEL_SWIZZLE_ZERO                                 = 0x00000000,
    VK_CHANNEL_SWIZZLE_ONE                                  = 0x00000001,
    VK_CHANNEL_SWIZZLE_R                                    = 0x00000002,
    VK_CHANNEL_SWIZZLE_G                                    = 0x00000003,
    VK_CHANNEL_SWIZZLE_B                                    = 0x00000004,
    VK_CHANNEL_SWIZZLE_A                                    = 0x00000005,

    VK_ENUM_RANGE(CHANNEL_SWIZZLE, ZERO, A)
} VkChannelSwizzle;

typedef enum VkDescriptorType_
{
    VK_DESCRIPTOR_TYPE_SAMPLER                              = 0x00000000,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER               = 0x00000001,
    VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE                        = 0x00000002,
    VK_DESCRIPTOR_TYPE_STORAGE_IMAGE                        = 0x00000003,
    VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER                 = 0x00000004,
    VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER                 = 0x00000005,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER                       = 0x00000006,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER                       = 0x00000007,
    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC               = 0x00000008,
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC               = 0x00000009,

    VK_ENUM_RANGE(DESCRIPTOR_TYPE, SAMPLER, STORAGE_BUFFER_DYNAMIC)
} VkDescriptorType;

typedef enum VkDescriptorPoolUsage_
{
    VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT                       = 0x00000000,
    VK_DESCRIPTOR_POOL_USAGE_DYNAMIC                        = 0x00000001,

    VK_ENUM_RANGE(DESCRIPTOR_POOL_USAGE, ONE_SHOT, DYNAMIC)
} VkDescriptorPoolUsage;

typedef enum VkDescriptorUpdateMode_
{
    VK_DESCRIPTOR_UPDATE_MODE_COPY                          = 0x00000000,
    VK_DESCRIPTOR_UPDATE_MODE_FASTEST                       = 0x00000001,

    VK_ENUM_RANGE(DESCRIPTOR_UPDATE_MODE, COPY, FASTEST)
} VkDescriptorUpdateMode;

typedef enum VkDescriptorSetUsage_
{
    VK_DESCRIPTOR_SET_USAGE_ONE_SHOT                        = 0x00000000,
    VK_DESCRIPTOR_SET_USAGE_STATIC                          = 0x00000001,

    VK_ENUM_RANGE(DESCRIPTOR_SET_USAGE, ONE_SHOT, STATIC)
} VkDescriptorSetUsage;

typedef enum VkQueryType_
{
    VK_QUERY_TYPE_OCCLUSION                                 = 0x00000000,
    VK_QUERY_TYPE_PIPELINE_STATISTICS                       = 0x00000001, // Optional

    VK_ENUM_RANGE(QUERY_TYPE, OCCLUSION, PIPELINE_STATISTICS)
} VkQueryType;

typedef enum VkTimestampType_
{
    VK_TIMESTAMP_TYPE_TOP                                   = 0x00000000,
    VK_TIMESTAMP_TYPE_BOTTOM                                = 0x00000001,

    VK_ENUM_RANGE(TIMESTAMP_TYPE, TOP, BOTTOM)
} VkTimestampType;

typedef enum VkBorderColor_
{
    VK_BORDER_COLOR_OPAQUE_WHITE                            = 0x00000000,
    VK_BORDER_COLOR_TRANSPARENT_BLACK                       = 0x00000001,
    VK_BORDER_COLOR_OPAQUE_BLACK                            = 0x00000002,

    VK_ENUM_RANGE(BORDER_COLOR, OPAQUE_WHITE, OPAQUE_BLACK)
} VkBorderColor;

typedef enum VkPipelineBindPoint_
{
    VK_PIPELINE_BIND_POINT_COMPUTE                          = 0x00000000,
    VK_PIPELINE_BIND_POINT_GRAPHICS                         = 0x00000001,

    VK_ENUM_RANGE(PIPELINE_BIND_POINT, COMPUTE, GRAPHICS)
} VkPipelineBindPoint;

typedef enum VkStateBindPoint_
{
    VK_STATE_BIND_POINT_VIEWPORT                            = 0x00000000,
    VK_STATE_BIND_POINT_RASTER                              = 0x00000001,
    VK_STATE_BIND_POINT_COLOR_BLEND                         = 0x00000002,
    VK_STATE_BIND_POINT_DEPTH_STENCIL                       = 0x00000003,

    VK_ENUM_RANGE(STATE_BIND_POINT, VIEWPORT, DEPTH_STENCIL)
} VkStateBindPoint;

typedef enum VkPrimitiveTopology_
{
    VK_PRIMITIVE_TOPOLOGY_POINT_LIST                        = 0x00000000,
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST                         = 0x00000001,
    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP                        = 0x00000002,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST                     = 0x00000003,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP                    = 0x00000004,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN                      = 0x00000005,
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST_ADJ                     = 0x00000006,
    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJ                    = 0x00000007,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_ADJ                 = 0x00000008,
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJ                = 0x00000009,
    VK_PRIMITIVE_TOPOLOGY_PATCH                             = 0x0000000a,

    VK_ENUM_RANGE(PRIMITIVE_TOPOLOGY, POINT_LIST, PATCH)
} VkPrimitiveTopology;

typedef enum VkIndexType_
{
    VK_INDEX_TYPE_UINT8                                     = 0x00000000,
    VK_INDEX_TYPE_UINT16                                    = 0x00000001,
    VK_INDEX_TYPE_UINT32                                    = 0x00000002,

    VK_ENUM_RANGE(INDEX_TYPE, UINT8, UINT32)
} VkIndexType;

typedef enum VkTexFilter_
{
    VK_TEX_FILTER_NEAREST                                   = 0x00000000,
    VK_TEX_FILTER_LINEAR                                    = 0x00000001,

    VK_ENUM_RANGE(TEX_FILTER, NEAREST, LINEAR)
} VkTexFilter;

typedef enum VkTexMipmapMode_
{
    VK_TEX_MIPMAP_MODE_BASE                                 = 0x00000000,   // Always choose base level
    VK_TEX_MIPMAP_MODE_NEAREST                              = 0x00000001,   // Choose nearest mip level
    VK_TEX_MIPMAP_MODE_LINEAR                               = 0x00000002,   // Linear filter between mip levels

    VK_ENUM_RANGE(TEX_MIPMAP_MODE, BASE, LINEAR)
} VkTexMipmapMode;

typedef enum VkTexAddress_
{
    VK_TEX_ADDRESS_WRAP                                     = 0x00000000,
    VK_TEX_ADDRESS_MIRROR                                   = 0x00000001,
    VK_TEX_ADDRESS_CLAMP                                    = 0x00000002,
    VK_TEX_ADDRESS_MIRROR_ONCE                              = 0x00000003,
    VK_TEX_ADDRESS_CLAMP_BORDER                             = 0x00000004,

    VK_ENUM_RANGE(TEX_ADDRESS, WRAP, CLAMP_BORDER)
} VkTexAddress;

typedef enum VkCompareOp_
{
    VK_COMPARE_OP_NEVER                                     = 0x00000000,
    VK_COMPARE_OP_LESS                                      = 0x00000001,
    VK_COMPARE_OP_EQUAL                                     = 0x00000002,
    VK_COMPARE_OP_LESS_EQUAL                                = 0x00000003,
    VK_COMPARE_OP_GREATER                                   = 0x00000004,
    VK_COMPARE_OP_NOT_EQUAL                                 = 0x00000005,
    VK_COMPARE_OP_GREATER_EQUAL                             = 0x00000006,
    VK_COMPARE_OP_ALWAYS                                    = 0x00000007,

    VK_ENUM_RANGE(COMPARE_OP, NEVER, ALWAYS)
} VkCompareOp;

typedef enum VkFillMode_
{
    VK_FILL_MODE_POINTS                                     = 0x00000000,
    VK_FILL_MODE_WIREFRAME                                  = 0x00000001,
    VK_FILL_MODE_SOLID                                      = 0x00000002,

    VK_ENUM_RANGE(FILL_MODE, POINTS, SOLID)
} VkFillMode;

typedef enum VkCullMode_
{
    VK_CULL_MODE_NONE                                       = 0x00000000,
    VK_CULL_MODE_FRONT                                      = 0x00000001,
    VK_CULL_MODE_BACK                                       = 0x00000002,
    VK_CULL_MODE_FRONT_AND_BACK                             = 0x00000003,

    VK_ENUM_RANGE(CULL_MODE, NONE, FRONT_AND_BACK)
} VkCullMode;

typedef enum VkFrontFace_
{
    VK_FRONT_FACE_CCW                                       = 0x00000000,
    VK_FRONT_FACE_CW                                        = 0x00000001,

    VK_ENUM_RANGE(FRONT_FACE, CCW, CW)
} VkFrontFace;

typedef enum VkProvokingVertex_
{
    VK_PROVOKING_VERTEX_FIRST                               = 0x00000000,
    VK_PROVOKING_VERTEX_LAST                                = 0x00000001,

    VK_ENUM_RANGE(PROVOKING_VERTEX, FIRST, LAST)
} VkProvokingVertex;

typedef enum VkCoordinateOrigin_
{
    VK_COORDINATE_ORIGIN_UPPER_LEFT                         = 0x00000000,
    VK_COORDINATE_ORIGIN_LOWER_LEFT                         = 0x00000001,

    VK_ENUM_RANGE(COORDINATE_ORIGIN, UPPER_LEFT, LOWER_LEFT)
} VkCoordinateOrigin;

typedef enum VkDepthMode_
{
    VK_DEPTH_MODE_ZERO_TO_ONE                               = 0x00000000,
    VK_DEPTH_MODE_NEGATIVE_ONE_TO_ONE                       = 0x00000001,

    VK_ENUM_RANGE(DEPTH_MODE, ZERO_TO_ONE, NEGATIVE_ONE_TO_ONE)
} VkDepthMode;

typedef enum VkBlend_
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

    VK_ENUM_RANGE(BLEND, ZERO, ONE_MINUS_SRC1_ALPHA)
} VkBlend;

typedef enum VkBlendOp_
{
    VK_BLEND_OP_ADD                                         = 0x00000000,
    VK_BLEND_OP_SUBTRACT                                    = 0x00000001,
    VK_BLEND_OP_REVERSE_SUBTRACT                            = 0x00000002,
    VK_BLEND_OP_MIN                                         = 0x00000003,
    VK_BLEND_OP_MAX                                         = 0x00000004,

    VK_ENUM_RANGE(BLEND_OP, ADD, MAX)
} VkBlendOp;

typedef enum VkStencilOp_
{
    VK_STENCIL_OP_KEEP                                      = 0x00000000,
    VK_STENCIL_OP_ZERO                                      = 0x00000001,
    VK_STENCIL_OP_REPLACE                                   = 0x00000002,
    VK_STENCIL_OP_INC_CLAMP                                 = 0x00000003,
    VK_STENCIL_OP_DEC_CLAMP                                 = 0x00000004,
    VK_STENCIL_OP_INVERT                                    = 0x00000005,
    VK_STENCIL_OP_INC_WRAP                                  = 0x00000006,
    VK_STENCIL_OP_DEC_WRAP                                  = 0x00000007,

    VK_ENUM_RANGE(STENCIL_OP, KEEP, DEC_WRAP)
} VkStencilOp;

typedef enum VkLogicOp_
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

    VK_ENUM_RANGE(LOGIC_OP, COPY, SET)
} VkLogicOp;

typedef enum VkSystemAllocType_
{
    VK_SYSTEM_ALLOC_TYPE_API_OBJECT                         = 0x00000000,
    VK_SYSTEM_ALLOC_TYPE_INTERNAL                           = 0x00000001,
    VK_SYSTEM_ALLOC_TYPE_INTERNAL_TEMP                      = 0x00000002,
    VK_SYSTEM_ALLOC_TYPE_INTERNAL_SHADER                    = 0x00000003,
    VK_SYSTEM_ALLOC_TYPE_DEBUG                              = 0x00000004,

    VK_ENUM_RANGE(SYSTEM_ALLOC_TYPE, API_OBJECT, DEBUG)
} VkSystemAllocType;

typedef enum VkPhysicalDeviceType_
{
    VK_PHYSICAL_DEVICE_TYPE_OTHER                           = 0x00000000,
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU                  = 0x00000001,
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU                    = 0x00000002,
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU                     = 0x00000003,
    VK_PHYSICAL_DEVICE_TYPE_CPU                             = 0x00000004,

    VK_ENUM_RANGE(PHYSICAL_DEVICE_TYPE, OTHER, CPU)
} VkPhysicalDeviceType;

typedef enum VkPhysicalDeviceInfoType_
{
    // Info type for vkGetPhysicalDeviceInfo()
    VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES                 = 0x00000000,
    VK_PHYSICAL_DEVICE_INFO_TYPE_PERFORMANCE                = 0x00000001,
    VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES           = 0x00000002,
    VK_PHYSICAL_DEVICE_INFO_TYPE_MEMORY_PROPERTIES          = 0x00000003,

    VK_ENUM_RANGE(PHYSICAL_DEVICE_INFO_TYPE, PROPERTIES, MEMORY_PROPERTIES)
} VkPhysicalDeviceInfoType;

typedef enum VkExtensionInfoType_
{
    // Info type for vkGetGlobalExtensionInfo() and vkGetPhysicalDeviceExtensionInfo()
    VK_EXTENSION_INFO_TYPE_COUNT                            = 0x00000000,
    VK_EXTENSION_INFO_TYPE_PROPERTIES                       = 0x00000001,

    //VK_ENUM_RANGE(EXTENSION_INFO_TYPE, COUNT, PROPERTIES)
} VkExtensionInfoType;

typedef enum VkFormatInfoType_
{
    // Info type for vkGetFormatInfo()
    VK_FORMAT_INFO_TYPE_PROPERTIES                          = 0x00000000,

    VK_ENUM_RANGE(FORMAT_INFO_TYPE, PROPERTIES, PROPERTIES)
} VkFormatInfoType;

typedef enum VkSubresourceInfoType_
{
    // Info type for vkGetImageSubresourceInfo()
    VK_SUBRESOURCE_INFO_TYPE_LAYOUT                         = 0x00000000,

    VK_ENUM_RANGE(SUBRESOURCE_INFO_TYPE, LAYOUT, LAYOUT)
} VkSubresourceInfoType;

typedef enum VkObjectInfoType_
{
    // Info type for vkGetObjectInfo()
    VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT             = 0x00000000,
    VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS                 = 0x00000001,

    VK_ENUM_RANGE(OBJECT_INFO_TYPE, MEMORY_ALLOCATION_COUNT, MEMORY_REQUIREMENTS)
} VkObjectInfoType;

typedef enum VkVertexInputStepRate_
{
    VK_VERTEX_INPUT_STEP_RATE_VERTEX                        = 0x0,
    VK_VERTEX_INPUT_STEP_RATE_INSTANCE                      = 0x1,
    VK_VERTEX_INPUT_STEP_RATE_DRAW                          = 0x2,  //Optional

    VK_ENUM_RANGE(VERTEX_INPUT_STEP_RATE, VERTEX, DRAW)
} VkVertexInputStepRate;

// Vulkan format definitions
typedef enum VkFormat_
{
    VK_FORMAT_UNDEFINED                                     = 0x00000000,
    VK_FORMAT_R4G4_UNORM                                    = 0x00000001,
    VK_FORMAT_R4G4_USCALED                                  = 0x00000002,
    VK_FORMAT_R4G4B4A4_UNORM                                = 0x00000003,
    VK_FORMAT_R4G4B4A4_USCALED                              = 0x00000004,
    VK_FORMAT_R5G6B5_UNORM                                  = 0x00000005,
    VK_FORMAT_R5G6B5_USCALED                                = 0x00000006,
    VK_FORMAT_R5G5B5A1_UNORM                                = 0x00000007,
    VK_FORMAT_R5G5B5A1_USCALED                              = 0x00000008,
    VK_FORMAT_R8_UNORM                                      = 0x00000009,
    VK_FORMAT_R8_SNORM                                      = 0x0000000A,
    VK_FORMAT_R8_USCALED                                    = 0x0000000B,
    VK_FORMAT_R8_SSCALED                                    = 0x0000000C,
    VK_FORMAT_R8_UINT                                       = 0x0000000D,
    VK_FORMAT_R8_SINT                                       = 0x0000000E,
    VK_FORMAT_R8_SRGB                                       = 0x0000000F,
    VK_FORMAT_R8G8_UNORM                                    = 0x00000010,
    VK_FORMAT_R8G8_SNORM                                    = 0x00000011,
    VK_FORMAT_R8G8_USCALED                                  = 0x00000012,
    VK_FORMAT_R8G8_SSCALED                                  = 0x00000013,
    VK_FORMAT_R8G8_UINT                                     = 0x00000014,
    VK_FORMAT_R8G8_SINT                                     = 0x00000015,
    VK_FORMAT_R8G8_SRGB                                     = 0x00000016,
    VK_FORMAT_R8G8B8_UNORM                                  = 0x00000017,
    VK_FORMAT_R8G8B8_SNORM                                  = 0x00000018,
    VK_FORMAT_R8G8B8_USCALED                                = 0x00000019,
    VK_FORMAT_R8G8B8_SSCALED                                = 0x0000001A,
    VK_FORMAT_R8G8B8_UINT                                   = 0x0000001B,
    VK_FORMAT_R8G8B8_SINT                                   = 0x0000001C,
    VK_FORMAT_R8G8B8_SRGB                                   = 0x0000001D,
    VK_FORMAT_R8G8B8A8_UNORM                                = 0x0000001E,
    VK_FORMAT_R8G8B8A8_SNORM                                = 0x0000001F,
    VK_FORMAT_R8G8B8A8_USCALED                              = 0x00000020,
    VK_FORMAT_R8G8B8A8_SSCALED                              = 0x00000021,
    VK_FORMAT_R8G8B8A8_UINT                                 = 0x00000022,
    VK_FORMAT_R8G8B8A8_SINT                                 = 0x00000023,
    VK_FORMAT_R8G8B8A8_SRGB                                 = 0x00000024,
    VK_FORMAT_R10G10B10A2_UNORM                             = 0x00000025,
    VK_FORMAT_R10G10B10A2_SNORM                             = 0x00000026,
    VK_FORMAT_R10G10B10A2_USCALED                           = 0x00000027,
    VK_FORMAT_R10G10B10A2_SSCALED                           = 0x00000028,
    VK_FORMAT_R10G10B10A2_UINT                              = 0x00000029,
    VK_FORMAT_R10G10B10A2_SINT                              = 0x0000002A,
    VK_FORMAT_R16_UNORM                                     = 0x0000002B,
    VK_FORMAT_R16_SNORM                                     = 0x0000002C,
    VK_FORMAT_R16_USCALED                                   = 0x0000002D,
    VK_FORMAT_R16_SSCALED                                   = 0x0000002E,
    VK_FORMAT_R16_UINT                                      = 0x0000002F,
    VK_FORMAT_R16_SINT                                      = 0x00000030,
    VK_FORMAT_R16_SFLOAT                                    = 0x00000031,
    VK_FORMAT_R16G16_UNORM                                  = 0x00000032,
    VK_FORMAT_R16G16_SNORM                                  = 0x00000033,
    VK_FORMAT_R16G16_USCALED                                = 0x00000034,
    VK_FORMAT_R16G16_SSCALED                                = 0x00000035,
    VK_FORMAT_R16G16_UINT                                   = 0x00000036,
    VK_FORMAT_R16G16_SINT                                   = 0x00000037,
    VK_FORMAT_R16G16_SFLOAT                                 = 0x00000038,
    VK_FORMAT_R16G16B16_UNORM                               = 0x00000039,
    VK_FORMAT_R16G16B16_SNORM                               = 0x0000003A,
    VK_FORMAT_R16G16B16_USCALED                             = 0x0000003B,
    VK_FORMAT_R16G16B16_SSCALED                             = 0x0000003C,
    VK_FORMAT_R16G16B16_UINT                                = 0x0000003D,
    VK_FORMAT_R16G16B16_SINT                                = 0x0000003E,
    VK_FORMAT_R16G16B16_SFLOAT                              = 0x0000003F,
    VK_FORMAT_R16G16B16A16_UNORM                            = 0x00000040,
    VK_FORMAT_R16G16B16A16_SNORM                            = 0x00000041,
    VK_FORMAT_R16G16B16A16_USCALED                          = 0x00000042,
    VK_FORMAT_R16G16B16A16_SSCALED                          = 0x00000043,
    VK_FORMAT_R16G16B16A16_UINT                             = 0x00000044,
    VK_FORMAT_R16G16B16A16_SINT                             = 0x00000045,
    VK_FORMAT_R16G16B16A16_SFLOAT                           = 0x00000046,
    VK_FORMAT_R32_UINT                                      = 0x00000047,
    VK_FORMAT_R32_SINT                                      = 0x00000048,
    VK_FORMAT_R32_SFLOAT                                    = 0x00000049,
    VK_FORMAT_R32G32_UINT                                   = 0x0000004A,
    VK_FORMAT_R32G32_SINT                                   = 0x0000004B,
    VK_FORMAT_R32G32_SFLOAT                                 = 0x0000004C,
    VK_FORMAT_R32G32B32_UINT                                = 0x0000004D,
    VK_FORMAT_R32G32B32_SINT                                = 0x0000004E,
    VK_FORMAT_R32G32B32_SFLOAT                              = 0x0000004F,
    VK_FORMAT_R32G32B32A32_UINT                             = 0x00000050,
    VK_FORMAT_R32G32B32A32_SINT                             = 0x00000051,
    VK_FORMAT_R32G32B32A32_SFLOAT                           = 0x00000052,
    VK_FORMAT_R64_SFLOAT                                    = 0x00000053,
    VK_FORMAT_R64G64_SFLOAT                                 = 0x00000054,
    VK_FORMAT_R64G64B64_SFLOAT                              = 0x00000055,
    VK_FORMAT_R64G64B64A64_SFLOAT                           = 0x00000056,
    VK_FORMAT_R11G11B10_UFLOAT                              = 0x00000057,
    VK_FORMAT_R9G9B9E5_UFLOAT                               = 0x00000058,
    VK_FORMAT_D16_UNORM                                     = 0x00000059,
    VK_FORMAT_D24_UNORM                                     = 0x0000005A,
    VK_FORMAT_D32_SFLOAT                                    = 0x0000005B,
    VK_FORMAT_S8_UINT                                       = 0x0000005C,
    VK_FORMAT_D16_UNORM_S8_UINT                             = 0x0000005D,
    VK_FORMAT_D24_UNORM_S8_UINT                             = 0x0000005E,
    VK_FORMAT_D32_SFLOAT_S8_UINT                            = 0x0000005F,
    VK_FORMAT_BC1_RGB_UNORM                                 = 0x00000060,
    VK_FORMAT_BC1_RGB_SRGB                                  = 0x00000061,
    VK_FORMAT_BC1_RGBA_UNORM                                = 0x00000062,
    VK_FORMAT_BC1_RGBA_SRGB                                 = 0x00000063,
    VK_FORMAT_BC2_UNORM                                     = 0x00000064,
    VK_FORMAT_BC2_SRGB                                      = 0x00000065,
    VK_FORMAT_BC3_UNORM                                     = 0x00000066,
    VK_FORMAT_BC3_SRGB                                      = 0x00000067,
    VK_FORMAT_BC4_UNORM                                     = 0x00000068,
    VK_FORMAT_BC4_SNORM                                     = 0x00000069,
    VK_FORMAT_BC5_UNORM                                     = 0x0000006A,
    VK_FORMAT_BC5_SNORM                                     = 0x0000006B,
    VK_FORMAT_BC6H_UFLOAT                                   = 0x0000006C,
    VK_FORMAT_BC6H_SFLOAT                                   = 0x0000006D,
    VK_FORMAT_BC7_UNORM                                     = 0x0000006E,
    VK_FORMAT_BC7_SRGB                                      = 0x0000006F,
    VK_FORMAT_ETC2_R8G8B8_UNORM                             = 0x00000070,
    VK_FORMAT_ETC2_R8G8B8_SRGB                              = 0x00000071,
    VK_FORMAT_ETC2_R8G8B8A1_UNORM                           = 0x00000072,
    VK_FORMAT_ETC2_R8G8B8A1_SRGB                            = 0x00000073,
    VK_FORMAT_ETC2_R8G8B8A8_UNORM                           = 0x00000074,
    VK_FORMAT_ETC2_R8G8B8A8_SRGB                            = 0x00000075,
    VK_FORMAT_EAC_R11_UNORM                                 = 0x00000076,
    VK_FORMAT_EAC_R11_SNORM                                 = 0x00000077,
    VK_FORMAT_EAC_R11G11_UNORM                              = 0x00000078,
    VK_FORMAT_EAC_R11G11_SNORM                              = 0x00000079,
    VK_FORMAT_ASTC_4x4_UNORM                                = 0x0000007A,
    VK_FORMAT_ASTC_4x4_SRGB                                 = 0x0000007B,
    VK_FORMAT_ASTC_5x4_UNORM                                = 0x0000007C,
    VK_FORMAT_ASTC_5x4_SRGB                                 = 0x0000007D,
    VK_FORMAT_ASTC_5x5_UNORM                                = 0x0000007E,
    VK_FORMAT_ASTC_5x5_SRGB                                 = 0x0000007F,
    VK_FORMAT_ASTC_6x5_UNORM                                = 0x00000080,
    VK_FORMAT_ASTC_6x5_SRGB                                 = 0x00000081,
    VK_FORMAT_ASTC_6x6_UNORM                                = 0x00000082,
    VK_FORMAT_ASTC_6x6_SRGB                                 = 0x00000083,
    VK_FORMAT_ASTC_8x5_UNORM                                = 0x00000084,
    VK_FORMAT_ASTC_8x5_SRGB                                 = 0x00000085,
    VK_FORMAT_ASTC_8x6_UNORM                                = 0x00000086,
    VK_FORMAT_ASTC_8x6_SRGB                                 = 0x00000087,
    VK_FORMAT_ASTC_8x8_UNORM                                = 0x00000088,
    VK_FORMAT_ASTC_8x8_SRGB                                 = 0x00000089,
    VK_FORMAT_ASTC_10x5_UNORM                               = 0x0000008A,
    VK_FORMAT_ASTC_10x5_SRGB                                = 0x0000008B,
    VK_FORMAT_ASTC_10x6_UNORM                               = 0x0000008C,
    VK_FORMAT_ASTC_10x6_SRGB                                = 0x0000008D,
    VK_FORMAT_ASTC_10x8_UNORM                               = 0x0000008E,
    VK_FORMAT_ASTC_10x8_SRGB                                = 0x0000008F,
    VK_FORMAT_ASTC_10x10_UNORM                              = 0x00000090,
    VK_FORMAT_ASTC_10x10_SRGB                               = 0x00000091,
    VK_FORMAT_ASTC_12x10_UNORM                              = 0x00000092,
    VK_FORMAT_ASTC_12x10_SRGB                               = 0x00000093,
    VK_FORMAT_ASTC_12x12_UNORM                              = 0x00000094,
    VK_FORMAT_ASTC_12x12_SRGB                               = 0x00000095,
    VK_FORMAT_B4G4R4A4_UNORM                                = 0x00000096,
    VK_FORMAT_B5G5R5A1_UNORM                                = 0x00000097,
    VK_FORMAT_B5G6R5_UNORM                                  = 0x00000098,
    VK_FORMAT_B5G6R5_USCALED                                = 0x00000099,
    VK_FORMAT_B8G8R8_UNORM                                  = 0x0000009A,
    VK_FORMAT_B8G8R8_SNORM                                  = 0x0000009B,
    VK_FORMAT_B8G8R8_USCALED                                = 0x0000009C,
    VK_FORMAT_B8G8R8_SSCALED                                = 0x0000009D,
    VK_FORMAT_B8G8R8_UINT                                   = 0x0000009E,
    VK_FORMAT_B8G8R8_SINT                                   = 0x0000009F,
    VK_FORMAT_B8G8R8_SRGB                                   = 0x000000A0,
    VK_FORMAT_B8G8R8A8_UNORM                                = 0x000000A1,
    VK_FORMAT_B8G8R8A8_SNORM                                = 0x000000A2,
    VK_FORMAT_B8G8R8A8_USCALED                              = 0x000000A3,
    VK_FORMAT_B8G8R8A8_SSCALED                              = 0x000000A4,
    VK_FORMAT_B8G8R8A8_UINT                                 = 0x000000A5,
    VK_FORMAT_B8G8R8A8_SINT                                 = 0x000000A6,
    VK_FORMAT_B8G8R8A8_SRGB                                 = 0x000000A7,
    VK_FORMAT_B10G10R10A2_UNORM                             = 0x000000A8,
    VK_FORMAT_B10G10R10A2_SNORM                             = 0x000000A9,
    VK_FORMAT_B10G10R10A2_USCALED                           = 0x000000AA,
    VK_FORMAT_B10G10R10A2_SSCALED                           = 0x000000AB,
    VK_FORMAT_B10G10R10A2_UINT                              = 0x000000AC,
    VK_FORMAT_B10G10R10A2_SINT                              = 0x000000AD,

    VK_ENUM_RANGE(FORMAT, UNDEFINED, B10G10R10A2_SINT)
} VkFormat;

// Shader stage enumerant
typedef enum VkShaderStage_
{
    VK_SHADER_STAGE_VERTEX                                  = 0,
    VK_SHADER_STAGE_TESS_CONTROL                            = 1,
    VK_SHADER_STAGE_TESS_EVALUATION                         = 2,
    VK_SHADER_STAGE_GEOMETRY                                = 3,
    VK_SHADER_STAGE_FRAGMENT                                = 4,
    VK_SHADER_STAGE_COMPUTE                                 = 5,

    VK_ENUM_RANGE(SHADER_STAGE, VERTEX, COMPUTE)
} VkShaderStage;

// Structure type enumerant
typedef enum VkStructureType_
{
    VK_STRUCTURE_TYPE_APPLICATION_INFO                      = 0,
    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO                    = 1,
    VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO                     = 2,
    VK_STRUCTURE_TYPE_MEMORY_OPEN_INFO                      = 3,
    VK_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO                 = 4,
    VK_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO               = 5,
    VK_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO                = 6,
    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO                = 7,
    VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO     = 8,
    VK_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO        = 9,
    VK_STRUCTURE_TYPE_SHADER_CREATE_INFO                    = 10,
    VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO          = 11,
    VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO                   = 12,
    VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO     = 13,
    VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO          = 14,
    VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO          = 15,
    VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO          = 16,
    VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO          = 17,
    VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO                = 18,
    VK_STRUCTURE_TYPE_EVENT_CREATE_INFO                     = 19,
    VK_STRUCTURE_TYPE_FENCE_CREATE_INFO                     = 20,
    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO                 = 21,
    VK_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO                   = 22,
    VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO                = 23,
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO     = 24,
    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO         = 25,
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO     = 26,
    VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO         = 27,
    VK_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO       = 28,
    VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO         = 29,
    VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO         = 30,
    VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO         = 31,
    VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO         = 32,
    VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO         = 33,
    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO                     = 34,
    VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO                    = 35,
    VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO               = 36,
    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO               = 37,
    VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO                 = 38,
    VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO        = 39,
    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO               = 40,
    VK_STRUCTURE_TYPE_LAYER_CREATE_INFO                     = 41,
    VK_STRUCTURE_TYPE_MEMORY_BARRIER                        = 42,
    VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER                 = 43,
    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER                  = 44,
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO           = 45,
    VK_STRUCTURE_TYPE_UPDATE_SAMPLERS                       = 46,
    VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES               = 47,
    VK_STRUCTURE_TYPE_UPDATE_IMAGES                         = 48,
    VK_STRUCTURE_TYPE_UPDATE_BUFFERS                        = 49,
    VK_STRUCTURE_TYPE_UPDATE_AS_COPY                        = 50,
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO                  = 51,

    VK_ENUM_RANGE(STRUCTURE_TYPE, APPLICATION_INFO, INSTANCE_CREATE_INFO)
} VkStructureType;

// ------------------------------------------------------------------------------------------------
// Error and return codes

typedef enum VkResult_
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
    VK_ERROR_OUT_OF_HOST_MEMORY                             = -(0x00000004),
    VK_ERROR_OUT_OF_DEVICE_MEMORY                           = -(0x00000005),
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

    VK_MAX_ENUM(RESULT)
} VkResult;

// ------------------------------------------------------------------------------------------------
// Flags

// Device creation flags
typedef VkFlags VkDeviceCreateFlags;
typedef enum VkDeviceCreateFlagBits_
{
    VK_DEVICE_CREATE_VALIDATION_BIT                         = VK_BIT(0),
    VK_DEVICE_CREATE_MULTI_DEVICE_IQ_MATCH_BIT              = VK_BIT(1),
} VkDeviceCreateFlagBits;

// Queue capabilities
typedef VkFlags VkQueueFlags;
typedef enum VkQueueFlagBits_
{
    VK_QUEUE_GRAPHICS_BIT                                   = VK_BIT(0),    // Queue supports graphics operations
    VK_QUEUE_COMPUTE_BIT                                    = VK_BIT(1),    // Queue supports compute operations
    VK_QUEUE_DMA_BIT                                        = VK_BIT(2),    // Queue supports DMA operations
    VK_QUEUE_MEMMGR_BIT                                     = VK_BIT(3),    // Queue supports memory management operations
    VK_QUEUE_EXTENDED_BIT                                   = VK_BIT(30),   // Extended queue
} VkQueueFlagBits;

// Memory properties passed into vkAllocMemory().
typedef VkFlags VkMemoryPropertyFlags;
typedef enum VkMemoryPropertyFlagBits_
{
    VK_MEMORY_PROPERTY_DEVICE_ONLY                          = 0,            // If otherwise stated, then allocate memory on device
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT                     = VK_BIT(0),    // Memory should be mappable by host
    VK_MEMORY_PROPERTY_HOST_DEVICE_COHERENT_BIT             = VK_BIT(1),    // Memory should be coherent between host and device accesses
    VK_MEMORY_PROPERTY_HOST_UNCACHED_BIT                    = VK_BIT(2),    // Memory should not be cached by the host
    VK_MEMORY_PROPERTY_HOST_WRITE_COMBINED_BIT              = VK_BIT(3),    // Memory should support host write combining
    VK_MEMORY_PROPERTY_PREFER_HOST_LOCAL                    = VK_BIT(4),    // If set, prefer host access
    VK_MEMORY_PROPERTY_SHAREABLE_BIT                        = VK_BIT(5),
} VkMemoryPropertyFlagBits;

// Memory output flags passed to resource transition commands
typedef VkFlags VkMemoryOutputFlags;
typedef enum VkMemoryOutputFlagBits_
{
    VK_MEMORY_OUTPUT_CPU_WRITE_BIT                          = VK_BIT(0),    // Controls output coherency of CPU writes
    VK_MEMORY_OUTPUT_SHADER_WRITE_BIT                       = VK_BIT(1),    // Controls output coherency of generic shader writes
    VK_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT                   = VK_BIT(2),    // Controls output coherency of color attachment writes
    VK_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT           = VK_BIT(3),    // Controls output coherency of depth/stencil attachment writes
    VK_MEMORY_OUTPUT_TRANSFER_BIT                           = VK_BIT(4),    // Controls output coherency of transfer operations
} VkMemoryOutputFlagBits;

// Memory input flags passed to resource transition commands
typedef VkFlags VkMemoryInputFlags;
typedef enum VkMemoryInputFlagBits_
{
    VK_MEMORY_INPUT_CPU_READ_BIT                            = VK_BIT(0),    // Controls input coherency of CPU reads
    VK_MEMORY_INPUT_INDIRECT_COMMAND_BIT                    = VK_BIT(1),    // Controls input coherency of indirect command reads
    VK_MEMORY_INPUT_INDEX_FETCH_BIT                         = VK_BIT(2),    // Controls input coherency of index fetches
    VK_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT              = VK_BIT(3),    // Controls input coherency of vertex attribute fetches
    VK_MEMORY_INPUT_UNIFORM_READ_BIT                        = VK_BIT(4),    // Controls input coherency of uniform buffer reads
    VK_MEMORY_INPUT_SHADER_READ_BIT                         = VK_BIT(5),    // Controls input coherency of generic shader reads
    VK_MEMORY_INPUT_COLOR_ATTACHMENT_BIT                    = VK_BIT(6),    // Controls input coherency of color attachment reads
    VK_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT            = VK_BIT(7),    // Controls input coherency of depth/stencil attachment reads
    VK_MEMORY_INPUT_TRANSFER_BIT                            = VK_BIT(8),    // Controls input coherency of transfer operations
} VkMemoryInputFlagBits;

// Buffer usage flags
typedef VkFlags VkBufferUsageFlags;
typedef enum VkBufferUsageFlagBits_
{
    VK_BUFFER_USAGE_GENERAL                                 = 0,            // No special usage
    VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT                     = VK_BIT(0),    // Can be used as a source of transfer operations
    VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT                = VK_BIT(1),    // Can be used as a destination of transfer operations
    VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT                = VK_BIT(2),    // Can be used as TBO
    VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT                = VK_BIT(3),    // Can be used as IBO
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT                      = VK_BIT(4),    // Can be used as UBO
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT                      = VK_BIT(5),    // Can be used as SSBO
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT                        = VK_BIT(6),    // Can be used as source of fixed function index fetch (index buffer)
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT                       = VK_BIT(7),    // Can be used as source of fixed function vertex fetch (VBO)
    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT                     = VK_BIT(8),    // Can be the source of indirect parameters (e.g. indirect buffer, parameter buffer)
} VkBufferUsageFlagBits;

// Buffer creation flags
typedef VkFlags VkBufferCreateFlags;
typedef enum VkBufferCreateFlagBits_
{
    VK_BUFFER_CREATE_SHAREABLE_BIT                          = VK_BIT(0),    // Buffer should be shareable
    VK_BUFFER_CREATE_SPARSE_BIT                             = VK_BIT(1),    // Buffer should support sparse backing
} VkBufferCreateFlagBits;

// Shader stage flags
typedef VkFlags VkShaderStageFlags;
typedef enum VkShaderStageFlagBits_
{
    VK_SHADER_STAGE_VERTEX_BIT                              = VK_BIT(0),
    VK_SHADER_STAGE_TESS_CONTROL_BIT                        = VK_BIT(1),
    VK_SHADER_STAGE_TESS_EVALUATION_BIT                     = VK_BIT(2),
    VK_SHADER_STAGE_GEOMETRY_BIT                            = VK_BIT(3),
    VK_SHADER_STAGE_FRAGMENT_BIT                            = VK_BIT(4),
    VK_SHADER_STAGE_COMPUTE_BIT                             = VK_BIT(5),

    VK_SHADER_STAGE_ALL                                     = 0x7FFFFFFF,
} VkShaderStageFlagBits;

// Image usage flags
typedef VkFlags VkImageUsageFlags;
typedef enum VkImageUsageFlagBits_
{
    VK_IMAGE_USAGE_GENERAL                                  = 0,            // No special usage
    VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT                      = VK_BIT(0),    // Can be used as a source of transfer operations
    VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT                 = VK_BIT(1),    // Can be used as a destination of transfer operations
    VK_IMAGE_USAGE_SAMPLED_BIT                              = VK_BIT(2),    // Can be sampled from (SAMPLED_IMAGE and COMBINED_IMAGE_SAMPLER descriptor types)
    VK_IMAGE_USAGE_STORAGE_BIT                              = VK_BIT(3),    // Can be used as storage image (STORAGE_IMAGE descriptor type)
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT                     = VK_BIT(4),    // Can be used as framebuffer color attachment
    VK_IMAGE_USAGE_DEPTH_STENCIL_BIT                        = VK_BIT(5),    // Can be used as framebuffer depth/stencil attachment
    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT                 = VK_BIT(6),    // Image data not needed outside of rendering
} VkImageUsageFlagBits;

// Image creation flags
typedef VkFlags VkImageCreateFlags;
typedef enum VkImageCreateFlagBits_
{
    VK_IMAGE_CREATE_INVARIANT_DATA_BIT                      = VK_BIT(0),
    VK_IMAGE_CREATE_CLONEABLE_BIT                           = VK_BIT(1),
    VK_IMAGE_CREATE_SHAREABLE_BIT                           = VK_BIT(2),    // Image should be shareable
    VK_IMAGE_CREATE_SPARSE_BIT                              = VK_BIT(3),    // Image should support sparse backing
    VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT                      = VK_BIT(4),    // Allows image views to have different format than the base image
    VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT                     = VK_BIT(5),    // Allows creating image views with cube type from the created image
} VkImageCreateFlagBits;

// Depth-stencil view creation flags
typedef VkFlags VkDepthStencilViewCreateFlags;
typedef enum VkDepthStencilViewCreateFlagBits_
{
    VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT        = VK_BIT(0),
    VK_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT      = VK_BIT(1),
} VkDepthStencilViewCreateFlagBits;

// Pipeline creation flags
typedef VkFlags VkPipelineCreateFlags;
typedef enum VkPipelineCreateFlagBits_
{
    VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT             = VK_BIT(0),
    VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT                = VK_BIT(1),
} VkPipelineCreateFlagBits;

// Channel flags
typedef VkFlags VkChannelFlags;
typedef enum VkChannelFlagBits_
{
    VK_CHANNEL_R_BIT                                        = VK_BIT(0),
    VK_CHANNEL_G_BIT                                        = VK_BIT(1),
    VK_CHANNEL_B_BIT                                        = VK_BIT(2),
    VK_CHANNEL_A_BIT                                        = VK_BIT(3),
} VkChannelFlagBits;

// Fence creation flags
typedef VkFlags VkFenceCreateFlags;
typedef enum VkFenceCreateFlagBits_
{
    VK_FENCE_CREATE_SIGNALED_BIT                            = VK_BIT(0),
} VkFenceCreateFlagBits;

// Semaphore creation flags
typedef VkFlags VkSemaphoreCreateFlags;
typedef enum VkSemaphoreCreateFlagBits_
{
    VK_SEMAPHORE_CREATE_SHAREABLE_BIT                       = VK_BIT(0),
} VkSemaphoreCreateFlagBits;

// Format capability flags
typedef VkFlags VkFormatFeatureFlags;
typedef enum VkFormatFeatureFlagBits_
{
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT                     = VK_BIT(0),    // Format can be used for sampled images (SAMPLED_IMAGE and COMBINED_IMAGE_SAMPLER descriptor types)
    VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT                     = VK_BIT(1),    // Format can be used for storage images (STORAGE_IMAGE descriptor type)
    VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT              = VK_BIT(2),    // Format supports atomic operations in case it's used for storage images
    VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT              = VK_BIT(3),    // Format can be used for uniform texel buffers (TBOs)
    VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT              = VK_BIT(4),    // Format can be used for storage texel buffers (IBOs)
    VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT       = VK_BIT(5),    // Format supports atomic operations in case it's used for storage texel buffers
    VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT                     = VK_BIT(6),    // Format can be used for vertex buffers (VBOs)
    VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT                  = VK_BIT(7),    // Format can be used for color attachment images
    VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT            = VK_BIT(8),    // Format supports blending in case it's used for color attachment images
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT          = VK_BIT(9),    // Format can be used for depth/stencil attachment images
    VK_FORMAT_FEATURE_CONVERSION_BIT                        = VK_BIT(10),   // Format can be used as the source or destination of format converting blits
} VkFormatFeatureFlagBits;

// Query control flags
typedef VkFlags VkQueryControlFlags;
typedef enum VkQueryControlFlagBits_
{
    VK_QUERY_CONTROL_CONSERVATIVE_BIT                       = VK_BIT(0),    // Allow conservative results to be collected by the query
} VkQueryControlFlagBits;

// Query result flags
typedef VkFlags VkQueryResultFlags;
typedef enum VkQueryResultFlagBits_
{
    VK_QUERY_RESULT_32_BIT                                  = 0,           // Results of the queries are written to the destination buffer as 32-bit values
    VK_QUERY_RESULT_64_BIT                                  = VK_BIT(0),   // Results of the queries are written to the destination buffer as 64-bit values
    // VK_QUERY_RESULT_NO_WAIT_BIT                             = 0,           // Results of the queries aren't waited on before proceeding with the result copy
    VK_QUERY_RESULT_WAIT_BIT                                = VK_BIT(1),   // Results of the queries are waited on before proceeding with the result copy
    VK_QUERY_RESULT_WITH_AVAILABILITY_BIT                   = VK_BIT(2),   // Besides the results of the query, the availability of the results is also written
    VK_QUERY_RESULT_PARTIAL_BIT                             = VK_BIT(3),   // Copy the partial results of the query even if the final results aren't available
} VkQueryResultFlagBits;

// Physical device compatibility flags
typedef VkFlags VkPhysicalDeviceCompatibilityFlags;
typedef enum VkPhysicalDeviceCompatibilityFlagBits_
{
    VK_PHYSICAL_DEVICE_COMPATIBILITY_FEATURES_BIT           = VK_BIT(0),
    VK_PHYSICAL_DEVICE_COMPATIBILITY_IQ_MATCH_BIT           = VK_BIT(1),
    VK_PHYSICAL_DEVICE_COMPATIBILITY_PEER_TRANSFER_BIT      = VK_BIT(2),
    VK_PHYSICAL_DEVICE_COMPATIBILITY_SHARED_MEMORY_BIT      = VK_BIT(3),
    VK_PHYSICAL_DEVICE_COMPATIBILITY_SHARED_SYNC_BIT        = VK_BIT(4),
    VK_PHYSICAL_DEVICE_COMPATIBILITY_SHARED_DEVICE0_DISPLAY_BIT = VK_BIT(5),
    VK_PHYSICAL_DEVICE_COMPATIBILITY_SHARED_DEVICE1_DISPLAY_BIT = VK_BIT(6),
} VkPhysicalDeviceCompatibilityFlagBits;

// Shader creation flags
typedef VkFlags VkShaderCreateFlags;

// Event creation flags
typedef VkFlags VkEventCreateFlags;

// Command buffer creation flags
typedef VkFlags VkCmdBufferCreateFlags;

// Command buffer optimization flags
typedef VkFlags VkCmdBufferOptimizeFlags;
typedef enum VkCmdBufferOptimizeFlagBits_
{
    VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT                  = VK_BIT(0),
    VK_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH_BIT              = VK_BIT(1),
    VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT              = VK_BIT(2),
    VK_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT        = VK_BIT(3),
} VkCmdBufferOptimizeFlagBits;

// Memory mapping flags
typedef VkFlags VkMemoryMapFlags;

// ------------------------------------------------------------------------------------------------
// Vulkan structures

typedef struct VkOffset2D_
{
    int32_t                                     x;
    int32_t                                     y;
} VkOffset2D;

typedef struct VkOffset3D_
{
    int32_t                                     x;
    int32_t                                     y;
    int32_t                                     z;
} VkOffset3D;

typedef struct VkExtent2D_
{
    int32_t                                     width;
    int32_t                                     height;
} VkExtent2D;

typedef struct VkExtent3D_
{
    int32_t                                     width;
    int32_t                                     height;
    int32_t                                     depth;
} VkExtent3D;

typedef struct VkViewport_
{
    float                                       originX;
    float                                       originY;
    float                                       width;
    float                                       height;
    float                                       minDepth;
    float                                       maxDepth;
} VkViewport;

typedef struct VkRect_
{
    VkOffset2D                                  offset;
    VkExtent2D                                  extent;
} VkRect;

typedef struct VkChannelMapping_
{
    VkChannelSwizzle                            r;
    VkChannelSwizzle                            g;
    VkChannelSwizzle                            b;
    VkChannelSwizzle                            a;
} VkChannelMapping;

typedef struct VkPhysicalDeviceProperties_
{
    uint32_t                                    apiVersion;
    uint32_t                                    driverVersion;
    uint32_t                                    vendorId;
    uint32_t                                    deviceId;
    VkPhysicalDeviceType                        deviceType;
    char                                        deviceName[VK_MAX_PHYSICAL_DEVICE_NAME];
    VkDeviceSize                                maxInlineMemoryUpdateSize;
    uint32_t                                    maxBoundDescriptorSets;
    uint32_t                                    maxThreadGroupSize;
    uint64_t                                    timestampFrequency;
    bool32_t                                    multiColorAttachmentClears;
    uint32_t                                    maxDescriptorSets;              // at least 2?
    uint32_t                                    maxViewports;                   // at least 16?
    uint32_t                                    maxColorAttachments;            // at least 8?
} VkPhysicalDeviceProperties;

typedef struct VkPhysicalDevicePerformance_
{
    float                                       maxDeviceClock;
    float                                       aluPerClock;
    float                                       texPerClock;
    float                                       primsPerClock;
    float                                       pixelsPerClock;
} VkPhysicalDevicePerformance;

typedef struct VkPhysicalDeviceCompatibilityInfo_
{
    VkPhysicalDeviceCompatibilityFlags          compatibilityFlags;
} VkPhysicalDeviceCompatibilityInfo;

typedef struct VkExtensionProperties_
{
    char                                        extName[VK_MAX_EXTENSION_NAME];     // extension name
    uint32_t                                    version;                            // version of the extension specification
} VkExtensionProperties;

typedef struct VkApplicationInfo_
{
    VkStructureType                             sType;              // Type of structure. Should be VK_STRUCTURE_TYPE_APPLICATION_INFO
    const void*                                 pNext;              // Next structure in chain
    const char*                                 pAppName;
    uint32_t                                    appVersion;
    const char*                                 pEngineName;
    uint32_t                                    engineVersion;
    uint32_t                                    apiVersion;
} VkApplicationInfo;

typedef void* (VKAPI *PFN_vkAllocFunction)(
    void*                                       pUserData,
    size_t                                      size,
    size_t                                      alignment,
    VkSystemAllocType                           allocType);

typedef void (VKAPI *PFN_vkFreeFunction)(
    void*                                       pUserData,
    void*                                       pMem);

typedef struct VkAllocCallbacks_
{
    void*                                       pUserData;
    PFN_vkAllocFunction                         pfnAlloc;
    PFN_vkFreeFunction                          pfnFree;
} VkAllocCallbacks;

typedef struct VkDeviceQueueCreateInfo_
{
    uint32_t                                    queueNodeIndex;
    uint32_t                                    queueCount;
} VkDeviceQueueCreateInfo;

typedef struct VkDeviceCreateInfo_
{
    VkStructureType                             sType;                      // Should be VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure
    uint32_t                                    queueRecordCount;
    const VkDeviceQueueCreateInfo*              pRequestedQueues;
    uint32_t                                    extensionCount;
    const char*const*                           ppEnabledExtensionNames;
    VkDeviceCreateFlags                         flags;                      // Device creation flags
} VkDeviceCreateInfo;

typedef struct VkInstanceCreateInfo_
{
    VkStructureType                             sType;                      // Should be VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure
    const VkApplicationInfo*                    pAppInfo;
    const VkAllocCallbacks*                     pAllocCb;
    uint32_t                                    extensionCount;
    const char*const*                           ppEnabledExtensionNames;    // layer or extension name to be enabled
} VkInstanceCreateInfo;

// can be added to VkDeviceCreateInfo via pNext
typedef struct _VkLayerCreateInfo
{
    VkStructureType                             sType;                      // Should be VK_STRUCTURE_TYPE_LAYER_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure
    uint32_t                                    layerCount;
    const char *const*                          ppActiveLayerNames;         // layer name from the layer's vkEnumerateLayers())
} VkLayerCreateInfo;

typedef struct VkPhysicalDeviceQueueProperties_
{
    VkQueueFlags                                queueFlags;                 // Queue flags
    uint32_t                                    queueCount;
    uint32_t                                    maxAtomicCounters;
    bool32_t                                    supportsTimestamps;
    uint32_t                                    maxMemReferences;           // Tells how many memory references can be active for the given queue
} VkPhysicalDeviceQueueProperties;

typedef struct VkPhysicalDeviceMemoryProperties_
{
    bool32_t                                    supportsMigration;
    bool32_t                                    supportsPinning;
} VkPhysicalDeviceMemoryProperties;

typedef struct VkMemoryAllocInfo_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VkDeviceSize                                allocationSize;             // Size of memory allocation
    VkMemoryPropertyFlags                       memProps;                   // Memory property flags
    VkMemoryPriority                            memPriority;
} VkMemoryAllocInfo;

typedef struct VkMemoryOpenInfo_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_MEMORY_OPEN_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VkDeviceMemory                              sharedMem;
} VkMemoryOpenInfo;

typedef struct VkPeerMemoryOpenInfo_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VkDeviceMemory                              originalMem;
} VkPeerMemoryOpenInfo;

typedef struct VkMemoryRequirements_
{
    VkDeviceSize                                size;                       // Specified in bytes
    VkDeviceSize                                alignment;                  // Specified in bytes
    VkDeviceSize                                granularity;                // Granularity on which vkQueueBindObjectMemoryRange can bind sub-ranges of memory specified in bytes (usually the page size)
    VkMemoryPropertyFlags                       memPropsAllowed;            // Allowed memory property flags
    VkMemoryPropertyFlags                       memPropsRequired;           // Required memory property flags
} VkMemoryRequirements;

typedef struct VkFormatProperties_
{
    VkFormatFeatureFlags                        linearTilingFeatures;       // Format features in case of linear tiling
    VkFormatFeatureFlags                        optimalTilingFeatures;      // Format features in case of optimal tiling
} VkFormatProperties;

typedef struct VkBufferViewAttachInfo_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VkBufferView                                view;
} VkBufferViewAttachInfo;

typedef struct VkImageViewAttachInfo_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO
    const void*                                 pNext;                      // Pointer to next structure
    VkImageView                                 view;
    VkImageLayout                               layout;
} VkImageViewAttachInfo;

typedef struct VkUpdateSamplers_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_UPDATE_SAMPLERS
    const void*                                 pNext;                      // Pointer to next structure
    uint32_t                                    binding;                    // Binding of the sampler (array)
    uint32_t                                    arrayIndex;                 // First element of the array to update or zero otherwise
    uint32_t                                    count;                      // Number of elements to update
    const VkSampler*                            pSamplers;
} VkUpdateSamplers;

typedef struct VkSamplerImageViewInfo_
{
    VkSampler                                   sampler;
    const VkImageViewAttachInfo*                pImageView;
} VkSamplerImageViewInfo;

typedef struct VkUpdateSamplerTextures_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES
    const void*                                 pNext;                      // Pointer to next structure
    uint32_t                                    binding;                    // Binding of the combined texture sampler (array)
    uint32_t                                    arrayIndex;                 // First element of the array to update or zero otherwise
    uint32_t                                    count;                      // Number of elements to update
    const VkSamplerImageViewInfo*               pSamplerImageViews;
} VkUpdateSamplerTextures;

typedef struct VkUpdateImages_
{
    VkStructureType                             sType;                     // Must be VK_STRUCTURE_TYPE_UPDATE_IMAGES
    const void*                                 pNext;                     // Pointer to next structure
    VkDescriptorType                            descriptorType;
    uint32_t                                    binding;                   // Binding of the image (array)
    uint32_t                                    arrayIndex;                // First element of the array to update or zero otherwise
    uint32_t                                    count;                     // Number of elements to update
    const VkImageViewAttachInfo*                pImageViews;
} VkUpdateImages;

typedef struct VkUpdateBuffers_
{
    VkStructureType                             sType;                    // Must be VK_STRUCTURE_TYPE_UPDATE_BUFFERS
    const void*                                 pNext;                    // Pointer to next structure
    VkDescriptorType                            descriptorType;
    uint32_t                                    binding;                  // Binding of the buffer (array)
    uint32_t                                    arrayIndex;               // First element of the array to update or zero otherwise
    uint32_t                                    count;                    // Number of elements to update
    const VkBufferViewAttachInfo*               pBufferViews;
} VkUpdateBuffers;

typedef struct VkUpdateAsCopy_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_UPDATE_AS_COPY
    const void*                                 pNext;                      // Pointer to next structure
    VkDescriptorType                            descriptorType;
    VkDescriptorSet                             descriptorSet;
    uint32_t                                    binding;
    uint32_t                                    arrayElement;
    uint32_t                                    count;
} VkUpdateAsCopy;

typedef struct VkBufferCreateInfo_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure.
    VkDeviceSize                                size;                       // Specified in bytes
    VkBufferUsageFlags                          usage;                      // Buffer usage flags
    VkBufferCreateFlags                         flags;                      // Buffer creation flags
} VkBufferCreateInfo;

typedef struct VkBufferViewCreateInfo_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure.
    VkBuffer                                    buffer;
    VkBufferViewType                            viewType;
    VkFormat                                    format;                     // Optionally specifies format of elements
    VkDeviceSize                                offset;                     // Specified in bytes
    VkDeviceSize                                range;                      // View size specified in bytes
} VkBufferViewCreateInfo;

typedef struct VkImageSubresource_
{
    VkImageAspect                               aspect;
    uint32_t                                    mipLevel;
    uint32_t                                    arraySlice;
} VkImageSubresource;

typedef struct VkImageSubresourceRange_
{
    VkImageAspect                               aspect;
    uint32_t                                    baseMipLevel;
    uint32_t                                    mipLevels;
    uint32_t                                    baseArraySlice;
    uint32_t                                    arraySize;
} VkImageSubresourceRange;

typedef struct VkMemoryBarrier_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_MEMORY_BARRIER
    const void*                                 pNext;                      // Pointer to next structure.

    VkMemoryOutputFlags                         outputMask;                 // Outputs the barrier should sync
    VkMemoryInputFlags                          inputMask;                  // Inputs the barrier should sync to
} VkMemoryBarrier;

typedef struct VkBufferMemoryBarrier_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER
    const void*                                 pNext;                      // Pointer to next structure.

    VkMemoryOutputFlags                         outputMask;                 // Outputs the barrier should sync
    VkMemoryInputFlags                          inputMask;                  // Inputs the barrier should sync to

    VkBuffer                                    buffer;                     // Buffer to sync

    VkDeviceSize                                offset;                     // Offset within the buffer to sync
    VkDeviceSize                                size;                       // Amount of bytes to sync
} VkBufferMemoryBarrier;

typedef struct VkImageMemoryBarrier_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
    const void*                                 pNext;                      // Pointer to next structure.

    VkMemoryOutputFlags                         outputMask;                 // Outputs the barrier should sync
    VkMemoryInputFlags                          inputMask;                  // Inputs the barrier should sync to

    VkImageLayout                               oldLayout;                  // Current layout of the image
    VkImageLayout                               newLayout;                  // New layout to transition the image to

    VkImage                                     image;                      // Image to sync

    VkImageSubresourceRange                     subresourceRange;           // Subresource range to sync
} VkImageMemoryBarrier;

typedef struct VkImageCreateInfo_
{
    VkStructureType                             sType;                      // Must be VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
    const void*                                 pNext;                      // Pointer to next structure.
    VkImageType                                 imageType;
    VkFormat                                    format;
    VkExtent3D                                  extent;
    uint32_t                                    mipLevels;
    uint32_t                                    arraySize;
    uint32_t                                    samples;
    VkImageTiling                               tiling;
    VkImageUsageFlags                           usage;                      // Image usage flags
    VkImageCreateFlags                          flags;                      // Image creation flags
} VkImageCreateInfo;

typedef struct VkPeerImageOpenInfo_
{
    VkImage                                     originalImage;
} VkPeerImageOpenInfo;

typedef struct VkSubresourceLayout_
{
    VkDeviceSize                                offset;                 // Specified in bytes
    VkDeviceSize                                size;                   // Specified in bytes
    VkDeviceSize                                rowPitch;               // Specified in bytes
    VkDeviceSize                                depthPitch;             // Specified in bytes
} VkSubresourceLayout;

typedef struct VkImageViewCreateInfo_
{
    VkStructureType                             sType;                  // Must be VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
    const void*                                 pNext;                  // Pointer to next structure
    VkImage                                     image;
    VkImageViewType                             viewType;
    VkFormat                                    format;
    VkChannelMapping                            channels;
    VkImageSubresourceRange                     subresourceRange;
    float                                       minLod;
} VkImageViewCreateInfo;

typedef struct VkColorAttachmentViewCreateInfo_
{
    VkStructureType                             sType;                  // Must be VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO
    const void*                                 pNext;                  // Pointer to next structure
    VkImage                                     image;
    VkFormat                                    format;
    uint32_t                                    mipLevel;
    uint32_t                                    baseArraySlice;
    uint32_t                                    arraySize;
    VkImage                                     msaaResolveImage;
    VkImageSubresourceRange                     msaaResolveSubResource;
} VkColorAttachmentViewCreateInfo;

typedef struct VkDepthStencilViewCreateInfo_
{
    VkStructureType                             sType;                  // Must be VK_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO
    const void*                                 pNext;                  // Pointer to next structure
    VkImage                                     image;
    uint32_t                                    mipLevel;
    uint32_t                                    baseArraySlice;
    uint32_t                                    arraySize;
    VkImage                                     msaaResolveImage;
    VkImageSubresourceRange                     msaaResolveSubResource;
    VkDepthStencilViewCreateFlags               flags;                  // Depth stencil attachment view flags
} VkDepthStencilViewCreateInfo;

typedef struct VkColorAttachmentBindInfo_
{
    VkColorAttachmentView                       view;
    VkImageLayout                               layout;
} VkColorAttachmentBindInfo;

typedef struct VkDepthStencilBindInfo_
{
    VkDepthStencilView                          view;
    VkImageLayout                               layout;
} VkDepthStencilBindInfo;

typedef struct VkBufferCopy_
{
    VkDeviceSize                                srcOffset;              // Specified in bytes
    VkDeviceSize                                destOffset;             // Specified in bytes
    VkDeviceSize                                copySize;               // Specified in bytes
} VkBufferCopy;

typedef struct VkImageMemoryBindInfo_
{
    VkImageSubresource                          subresource;
    VkOffset3D                                  offset;
    VkExtent3D                                  extent;
} VkImageMemoryBindInfo;

typedef struct VkImageCopy_
{
    VkImageSubresource                          srcSubresource;
    VkOffset3D                                  srcOffset;             // Specified in pixels for both compressed and uncompressed images
    VkImageSubresource                          destSubresource;
    VkOffset3D                                  destOffset;            // Specified in pixels for both compressed and uncompressed images
    VkExtent3D                                  extent;                // Specified in pixels for both compressed and uncompressed images
} VkImageCopy;

typedef struct VkImageBlit_
{
    VkImageSubresource                          srcSubresource;
    VkOffset3D                                  srcOffset;              // Specified in pixels for both compressed and uncompressed images
    VkExtent3D                                  srcExtent;              // Specified in pixels for both compressed and uncompressed images
    VkImageSubresource                          destSubresource;
    VkOffset3D                                  destOffset;             // Specified in pixels for both compressed and uncompressed images
    VkExtent3D                                  destExtent;             // Specified in pixels for both compressed and uncompressed images
} VkImageBlit;

typedef struct VkBufferImageCopy_
{
    VkDeviceSize                                bufferOffset;           // Specified in bytes
    VkImageSubresource                          imageSubresource;
    VkOffset3D                                  imageOffset;            // Specified in pixels for both compressed and uncompressed images
    VkExtent3D                                  imageExtent;            // Specified in pixels for both compressed and uncompressed images
} VkBufferImageCopy;

typedef struct VkImageResolve_
{
    VkImageSubresource                          srcSubresource;
    VkOffset3D                                  srcOffset;
    VkImageSubresource                          destSubresource;
    VkOffset3D                                  destOffset;
    VkExtent3D                                  extent;
} VkImageResolve;

typedef struct VkShaderCreateInfo_
{
    VkStructureType                             sType;              // Must be VK_STRUCTURE_TYPE_SHADER_CREATE_INFO
    const void*                                 pNext;              // Pointer to next structure
    size_t                                      codeSize;           // Specified in bytes
    const void*                                 pCode;
    VkShaderCreateFlags                         flags;              // Reserved
} VkShaderCreateInfo;

typedef struct VkDescriptorSetLayoutBinding_
{
    VkDescriptorType                            descriptorType;     // Type of the descriptors in this binding
    uint32_t                                    count;              // Number of descriptors in this binding
    VkShaderStageFlags                          stageFlags;         // Shader stages this binding is visible to
    const VkSampler*                            pImmutableSamplers; // Immutable samplers (used if descriptor type is SAMPLER or COMBINED_IMAGE_SAMPLER, is either NULL or contains <count> number of elements)
} VkDescriptorSetLayoutBinding;

typedef struct VkDescriptorSetLayoutCreateInfo_
{
    VkStructureType                             sType;              // Must be VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    const void*                                 pNext;              // Pointer to next structure
    uint32_t                                    count;              // Number of bindings in the descriptor set layout
    const VkDescriptorSetLayoutBinding*         pBinding;           // Array of descriptor set layout bindings
} VkDescriptorSetLayoutCreateInfo;

typedef struct VkDescriptorTypeCount_
{
    VkDescriptorType                            type;
    uint32_t                                    count;
} VkDescriptorTypeCount;

typedef struct VkDescriptorPoolCreateInfo_
{
    VkStructureType                             sType;              // Must be VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO
    const void*                                 pNext;              // Pointer to next structure
    uint32_t                                    count;
    const VkDescriptorTypeCount*                pTypeCount;
} VkDescriptorPoolCreateInfo;

typedef struct VkLinkConstBuffer_
{
    uint32_t                                    bufferId;
    size_t                                      bufferSize;
    const void*                                 pBufferData;
} VkLinkConstBuffer;

typedef struct VkSpecializationMapEntry_
{
    uint32_t                                    constantId;         // The SpecConstant ID specified in the BIL
    uint32_t                                    offset;             // Offset of the value in the data block
} VkSpecializationMapEntry;

typedef struct VkSpecializationInfo_
{
    uint32_t                                    mapEntryCount;
    const VkSpecializationMapEntry*             pMap;               // mapEntryCount entries
    const void*                                 pData;
} VkSpecializationInfo;

typedef struct VkPipelineShader_
{
    VkShaderStage                               stage;
    VkShader                                    shader;
    uint32_t                                    linkConstBufferCount;
    const VkLinkConstBuffer*                    pLinkConstBufferInfo;
    const VkSpecializationInfo*                 pSpecializationInfo;
} VkPipelineShader;

typedef struct VkComputePipelineCreateInfo_
{
    VkStructureType                             sType;          // Must be VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO
    const void*                                 pNext;          // Pointer to next structure
    VkPipelineShader                            cs;
    VkPipelineCreateFlags                       flags;          // Pipeline creation flags
    VkDescriptorSetLayoutChain                  setLayoutChain;
    uint32_t                                    localSizeX;
    uint32_t                                    localSizeY;
    uint32_t                                    localSizeZ;
} VkComputePipelineCreateInfo;

typedef struct VkVertexInputBindingDescription_
{
    uint32_t                                    binding;        // Vertex buffer binding id
    uint32_t                                    strideInBytes;  // Distance between vertices in bytes (0 = no advancement)

    VkVertexInputStepRate                       stepRate;       // Rate at which binding is incremented
} VkVertexInputBindingDescription;

typedef struct VkVertexInputAttributeDescription_
{
    uint32_t                                    location;       // location of the shader vertex attrib
    uint32_t                                    binding;        // Vertex buffer binding id

    VkFormat                                    format;         // format of source data

    uint32_t                                    offsetInBytes;  // Offset of first element in bytes from base of vertex
} VkVertexInputAttributeDescription;

typedef struct VkPipelineVertexInputCreateInfo_
{
    VkStructureType                             sType;          // Should be VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO
    const void*                                 pNext;          // Pointer to next structure

    uint32_t                                    bindingCount;   // number of bindings
    const VkVertexInputBindingDescription*      pVertexBindingDescriptions;

    uint32_t                                    attributeCount; // number of attributes
    const VkVertexInputAttributeDescription*    pVertexAttributeDescriptions;
} VkPipelineVertexInputCreateInfo;

typedef struct VkPipelineIaStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VkPrimitiveTopology                         topology;
    bool32_t                                    disableVertexReuse;         // optional
    bool32_t                                    primitiveRestartEnable;
    uint32_t                                    primitiveRestartIndex;      // optional (GL45)
} VkPipelineIaStateCreateInfo;

typedef struct VkPipelineTessStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    patchControlPoints;
} VkPipelineTessStateCreateInfo;

typedef struct VkPipelineVpStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    viewportCount;
    VkCoordinateOrigin                          clipOrigin;                 // optional (GL45)
    VkDepthMode                                 depthMode;                  // optional (GL45)
} VkPipelineVpStateCreateInfo;

typedef struct VkPipelineRsStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    bool32_t                                    depthClipEnable;
    bool32_t                                    rasterizerDiscardEnable;
    bool32_t                                    programPointSize;           // optional (GL45)
    VkCoordinateOrigin                          pointOrigin;                // optional (GL45)
    VkProvokingVertex                           provokingVertex;            // optional (GL45)
    VkFillMode                                  fillMode;                   // optional (GL45)
    VkCullMode                                  cullMode;
    VkFrontFace                                 frontFace;
} VkPipelineRsStateCreateInfo;

typedef struct VkPipelineMsStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    samples;
    bool32_t                                    multisampleEnable;          // optional (GL45)
    bool32_t                                    sampleShadingEnable;        // optional (GL45)
    float                                       minSampleShading;           // optional (GL45)
    VkSampleMask                                sampleMask;
} VkPipelineMsStateCreateInfo;

typedef struct VkPipelineCbAttachmentState_
{
    bool32_t                                    blendEnable;
    VkFormat                                    format;
    VkBlend                                     srcBlendColor;
    VkBlend                                     destBlendColor;
    VkBlendOp                                   blendOpColor;
    VkBlend                                     srcBlendAlpha;
    VkBlend                                     destBlendAlpha;
    VkBlendOp                                   blendOpAlpha;
    VkChannelFlags                              channelWriteMask;
} VkPipelineCbAttachmentState;

typedef struct VkPipelineCbStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    bool32_t                                    alphaToCoverageEnable;
    bool32_t                                    logicOpEnable;
    VkLogicOp                                   logicOp;
    uint32_t                                    attachmentCount;    // # of pAttachments
    const VkPipelineCbAttachmentState*          pAttachments;
} VkPipelineCbStateCreateInfo;

typedef struct VkStencilOpState_
{
    VkStencilOp                                 stencilFailOp;
    VkStencilOp                                 stencilPassOp;
    VkStencilOp                                 stencilDepthFailOp;
    VkCompareOp                                 stencilCompareOp;
} VkStencilOpState;

typedef struct VkPipelineDsStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VkFormat                                    format;
    bool32_t                                    depthTestEnable;
    bool32_t                                    depthWriteEnable;
    VkCompareOp                                 depthCompareOp;
    bool32_t                                    depthBoundsEnable;          // optional (depth_bounds_test)
    bool32_t                                    stencilTestEnable;
    VkStencilOpState                            front;
    VkStencilOpState                            back;
} VkPipelineDsStateCreateInfo;

typedef struct VkPipelineShaderStageCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VkPipelineShader                            shader;
} VkPipelineShaderStageCreateInfo;

typedef struct VkGraphicsPipelineCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VkPipelineCreateFlags                       flags;      // Pipeline creation flags
    VkDescriptorSetLayoutChain                  pSetLayoutChain;
} VkGraphicsPipelineCreateInfo;

typedef struct VkSamplerCreateInfo_
{
    VkStructureType                             sType;          // Must be VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
    const void*                                 pNext;          // Pointer to next structure
    VkTexFilter                                 magFilter;      // Filter mode for magnification
    VkTexFilter                                 minFilter;      // Filter mode for minifiation
    VkTexMipmapMode                             mipMode;        // Mipmap selection mode
    VkTexAddress                                addressU;
    VkTexAddress                                addressV;
    VkTexAddress                                addressW;
    float                                       mipLodBias;
    uint32_t                                    maxAnisotropy;
    VkCompareOp                                 compareOp;
    float                                       minLod;
    float                                       maxLod;
    VkBorderColor                               borderColor;
} VkSamplerCreateInfo;

typedef struct VkDynamicVpStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    viewportAndScissorCount;  // number of entries in pViewports and pScissors
    const VkViewport*                           pViewports;
    const VkRect*                               pScissors;
} VkDynamicVpStateCreateInfo;

typedef struct VkDynamicRsStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    float                                       depthBias;
    float                                       depthBiasClamp;
    float                                       slopeScaledDepthBias;
    float                                       pointSize;          // optional (GL45) - Size of points
    float                                       pointFadeThreshold; // optional (GL45) - Size of point fade threshold
    float                                       lineWidth;          // optional (GL45) - Width of lines
} VkDynamicRsStateCreateInfo;

typedef struct VkDynamicCbStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    float                                       blendConst[4];
} VkDynamicCbStateCreateInfo;

typedef struct VkDynamicDsStateCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    float                                       minDepth;               // optional (depth_bounds_test)
    float                                       maxDepth;               // optional (depth_bounds_test)
    uint32_t                                    stencilReadMask;
    uint32_t                                    stencilWriteMask;
    uint32_t                                    stencilFrontRef;
    uint32_t                                    stencilBackRef;
} VkDynamicDsStateCreateInfo;

typedef struct VkCmdBufferCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    queueNodeIndex;
    VkCmdBufferCreateFlags                      flags;      // Command buffer creation flags
} VkCmdBufferCreateInfo;

typedef struct VkCmdBufferBeginInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO
    const void*                                 pNext;      // Pointer to next structure

    VkCmdBufferOptimizeFlags                    flags;      // Command buffer optimization flags
} VkCmdBufferBeginInfo;

typedef struct VkRenderPassBegin_
{
    VkRenderPass                                renderPass;
    VkFramebuffer                               framebuffer;
} VkRenderPassBegin;

typedef struct VkCmdBufferGraphicsBeginInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO
    const void*                                 pNext;      // Pointer to next structure

    VkRenderPassBegin                           renderPassContinue;  // Only needed when a render pass is split across two command buffers
} VkCmdBufferGraphicsBeginInfo;

// Union allowing specification of floating point or raw color data. Actual value selected is based on image being cleared.
typedef union VkClearColorValue_
{
    float                                       floatColor[4];
    uint32_t                                    rawColor[4];
} VkClearColorValue;

typedef struct VkClearColor_
{
    VkClearColorValue                           color;
    bool32_t                                    useRawValue;
} VkClearColor;

typedef struct VkRenderPassCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure

    VkRect                                      renderArea;
    uint32_t                                    colorAttachmentCount;
    VkExtent2D                                  extent;
    uint32_t                                    sampleCount;
    uint32_t                                    layers;
    const VkFormat*                             pColorFormats;
    const VkImageLayout*                        pColorLayouts;
    const VkAttachmentLoadOp*                   pColorLoadOps;
    const VkAttachmentStoreOp*                  pColorStoreOps;
    const VkClearColor*                         pColorLoadClearValues;
    VkFormat                                    depthStencilFormat;
    VkImageLayout                               depthStencilLayout;
    VkAttachmentLoadOp                          depthLoadOp;
    float                                       depthLoadClearValue;
    VkAttachmentStoreOp                         depthStoreOp;
    VkAttachmentLoadOp                          stencilLoadOp;
    uint32_t                                    stencilLoadClearValue;
    VkAttachmentStoreOp                         stencilStoreOp;
} VkRenderPassCreateInfo;

typedef struct VkEventCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_EVENT_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VkEventCreateFlags                          flags;      // Event creation flags
} VkEventCreateInfo;

typedef struct VkFenceCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VkFenceCreateFlags                          flags;      // Fence creation flags
} VkFenceCreateInfo;

typedef struct VkSemaphoreCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    uint32_t                                    initialCount;
    VkSemaphoreCreateFlags                      flags;      // Semaphore creation flags
} VkSemaphoreCreateInfo;

typedef struct VkSemaphoreOpenInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO
    const void*                                 pNext;      // Pointer to next structure
    VkSemaphore                                 sharedSemaphore;
} VkSemaphoreOpenInfo;

typedef struct VkPipelineStatisticsData_
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
} VkPipelineStatisticsData;

typedef struct VkQueryPoolCreateInfo_
{
    VkStructureType                             sType;      // Must be VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO
    const void*                                 pNext;      // Pointer to next structure
    VkQueryType                                 queryType;
    uint32_t                                    slots;
} VkQueryPoolCreateInfo;

typedef struct VkFramebufferCreateInfo_
{
    VkStructureType                             sType;  // Must be VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
    const void*                                 pNext;  // Pointer to next structure

    uint32_t                                    colorAttachmentCount;
    const VkColorAttachmentBindInfo*            pColorAttachments;
    const VkDepthStencilBindInfo*               pDepthStencilAttachment;

    uint32_t                                    sampleCount;
    uint32_t                                    width;
    uint32_t                                    height;
    uint32_t                                    layers;
} VkFramebufferCreateInfo;

typedef struct VkDrawIndirectCmd_
{
    uint32_t                                    vertexCount;
    uint32_t                                    instanceCount;
    uint32_t                                    firstVertex;
    uint32_t                                    firstInstance;
} VkDrawIndirectCmd;

typedef struct VkDrawIndexedIndirectCmd_
{
    uint32_t                                    indexCount;
    uint32_t                                    instanceCount;
    uint32_t                                    firstIndex;
    int32_t                                     vertexOffset;
    uint32_t                                    firstInstance;
} VkDrawIndexedIndirectCmd;

typedef struct VkDispatchIndirectCmd_
{
    uint32_t                                    x;
    uint32_t                                    y;
    uint32_t                                    z;
} VkDispatchIndirectCmd;

// ------------------------------------------------------------------------------------------------
// API functions
typedef VkResult (VKAPI *PFN_vkCreateInstance)(const VkInstanceCreateInfo* pCreateInfo, VkInstance* pInstance);
typedef VkResult (VKAPI *PFN_vkDestroyInstance)(VkInstance instance);
typedef VkResult (VKAPI *PFN_vkEnumeratePhysicalDevices)(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices);
typedef VkResult (VKAPI *PFN_vkGetPhysicalDeviceInfo)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceInfoType infoType, size_t* pDataSize, void* pData);
typedef void *   (VKAPI *PFN_vkGetProcAddr)(VkPhysicalDevice physicalDevice, const char * pName);
typedef VkResult (VKAPI *PFN_vkCreateDevice)(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice);
typedef VkResult (VKAPI *PFN_vkDestroyDevice)(VkDevice device);
typedef VkResult (VKAPI *PFN_vkGetGlobalExtensionInfo)(VkExtensionInfoType infoType, uint32_t extensionIndex, size_t* pDataSize, void* pData);
typedef VkResult (VKAPI *PFN_vkGetPhysicalDeviceExtensionInfo)(VkPhysicalDevice gpu, VkExtensionInfoType infoType, uint32_t extensionIndex, size_t* pDataSize, void* pData);
typedef VkResult (VKAPI *PFN_vkGetExtensionSupport)(VkPhysicalDevice physicalDevice, const char* pExtName);
typedef VkResult (VKAPI *PFN_vkEnumerateLayers)(VkPhysicalDevice physicalDevice, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved);
typedef VkResult (VKAPI *PFN_vkGetDeviceQueue)(VkDevice device, uint32_t queueNodeIndex, uint32_t queueIndex, VkQueue* pQueue);
typedef VkResult (VKAPI *PFN_vkQueueSubmit)(VkQueue queue, uint32_t cmdBufferCount, const VkCmdBuffer* pCmdBuffers, VkFence fence);
typedef VkResult (VKAPI *PFN_vkQueueAddMemReferences)(VkQueue queue, uint32_t count, const VkDeviceMemory* pMems);
typedef VkResult (VKAPI *PFN_vkQueueRemoveMemReferences)(VkQueue queue, uint32_t count, const VkDeviceMemory* pMems);
typedef VkResult (VKAPI *PFN_vkQueueWaitIdle)(VkQueue queue);
typedef VkResult (VKAPI *PFN_vkDeviceWaitIdle)(VkDevice device);
typedef VkResult (VKAPI *PFN_vkAllocMemory)(VkDevice device, const VkMemoryAllocInfo* pAllocInfo, VkDeviceMemory* pMem);
typedef VkResult (VKAPI *PFN_vkFreeMemory)(VkDeviceMemory mem);
typedef VkResult (VKAPI *PFN_vkSetMemoryPriority)(VkDeviceMemory mem, VkMemoryPriority priority);
typedef VkResult (VKAPI *PFN_vkMapMemory)(VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData);
typedef VkResult (VKAPI *PFN_vkUnmapMemory)(VkDeviceMemory mem);
typedef VkResult (VKAPI *PFN_vkFlushMappedMemory)(VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size);
typedef VkResult (VKAPI *PFN_vkPinSystemMemory)(VkDevice device, const void* pSysMem, size_t memSize, VkDeviceMemory* pMem);
typedef VkResult (VKAPI *PFN_vkGetMultiDeviceCompatibility)(VkPhysicalDevice physicalDevice0, VkPhysicalDevice physicalDevice1, VkPhysicalDeviceCompatibilityInfo* pInfo);
typedef VkResult (VKAPI *PFN_vkOpenSharedMemory)(VkDevice device, const VkMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMem);
typedef VkResult (VKAPI *PFN_vkOpenSharedSemaphore)(VkDevice device, const VkSemaphoreOpenInfo* pOpenInfo, VkSemaphore* pSemaphore);
typedef VkResult (VKAPI *PFN_vkOpenPeerMemory)(VkDevice device, const VkPeerMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMem);
typedef VkResult (VKAPI *PFN_vkOpenPeerImage)(VkDevice device, const VkPeerImageOpenInfo* pOpenInfo, VkImage* pImage, VkDeviceMemory* pMem);
typedef VkResult (VKAPI *PFN_vkDestroyObject)(VkObject object);
typedef VkResult (VKAPI *PFN_vkGetObjectInfo)(VkBaseObject object, VkObjectInfoType infoType, size_t* pDataSize, void* pData);
typedef VkResult (VKAPI *PFN_vkQueueBindObjectMemory)(VkQueue queue, VkObject object, uint32_t allocationIdx, VkDeviceMemory mem, VkDeviceSize offset);
typedef VkResult (VKAPI *PFN_vkQueueBindObjectMemoryRange)(VkQueue queue, VkObject object, uint32_t allocationIdx, VkDeviceSize rangeOffset, VkDeviceSize rangeSize, VkDeviceMemory mem, VkDeviceSize memOffset);
typedef VkResult (VKAPI *PFN_vkQueueBindImageMemoryRange)(VkQueue queue, VkImage image, uint32_t allocationIdx, const VkImageMemoryBindInfo* pBindInfo, VkDeviceMemory mem, VkDeviceSize memOffset);
typedef VkResult (VKAPI *PFN_vkCreateFence)(VkDevice device, const VkFenceCreateInfo* pCreateInfo, VkFence* pFence);
typedef VkResult (VKAPI *PFN_vkResetFences)(VkDevice device, uint32_t fenceCount, VkFence* pFences);
typedef VkResult (VKAPI *PFN_vkGetFenceStatus)(VkFence fence);
typedef VkResult (VKAPI *PFN_vkWaitForFences)(VkDevice device, uint32_t fenceCount, const VkFence* pFences, bool32_t waitAll, uint64_t timeout);
typedef VkResult (VKAPI *PFN_vkCreateSemaphore)(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, VkSemaphore* pSemaphore);
typedef VkResult (VKAPI *PFN_vkQueueSignalSemaphore)(VkQueue queue, VkSemaphore semaphore);
typedef VkResult (VKAPI *PFN_vkQueueWaitSemaphore)(VkQueue queue, VkSemaphore semaphore);
typedef VkResult (VKAPI *PFN_vkCreateEvent)(VkDevice device, const VkEventCreateInfo* pCreateInfo, VkEvent* pEvent);
typedef VkResult (VKAPI *PFN_vkGetEventStatus)(VkEvent event);
typedef VkResult (VKAPI *PFN_vkSetEvent)(VkEvent event);
typedef VkResult (VKAPI *PFN_vkResetEvent)(VkEvent event);
typedef VkResult (VKAPI *PFN_vkCreateQueryPool)(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool);
typedef VkResult (VKAPI *PFN_vkGetQueryPoolResults)(VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData, VkQueryResultFlags flags);
typedef VkResult (VKAPI *PFN_vkGetFormatInfo)(VkDevice device, VkFormat format, VkFormatInfoType infoType, size_t* pDataSize, void* pData);
typedef VkResult (VKAPI *PFN_vkCreateBuffer)(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer);
typedef VkResult (VKAPI *PFN_vkCreateBufferView)(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView);
typedef VkResult (VKAPI *PFN_vkCreateImage)(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage);
typedef VkResult (VKAPI *PFN_vkGetImageSubresourceInfo)(VkImage image, const VkImageSubresource* pSubresource, VkSubresourceInfoType infoType, size_t* pDataSize, void* pData);
typedef VkResult (VKAPI *PFN_vkCreateImageView)(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView);
typedef VkResult (VKAPI *PFN_vkCreateColorAttachmentView)(VkDevice device, const VkColorAttachmentViewCreateInfo* pCreateInfo, VkColorAttachmentView* pView);
typedef VkResult (VKAPI *PFN_vkCreateDepthStencilView)(VkDevice device, const VkDepthStencilViewCreateInfo* pCreateInfo, VkDepthStencilView* pView);
typedef VkResult (VKAPI *PFN_vkCreateShader)(VkDevice device, const VkShaderCreateInfo* pCreateInfo, VkShader* pShader);
typedef VkResult (VKAPI *PFN_vkCreateGraphicsPipeline)(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline);
typedef VkResult (VKAPI *PFN_vkCreateGraphicsPipelineDerivative)(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline basePipeline, VkPipeline* pPipeline);
typedef VkResult (VKAPI *PFN_vkCreateComputePipeline)(VkDevice device, const VkComputePipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline);
typedef VkResult (VKAPI *PFN_vkStorePipeline)(VkPipeline pipeline, size_t* pDataSize, void* pData);
typedef VkResult (VKAPI *PFN_vkLoadPipeline)(VkDevice device, size_t dataSize, const void* pData, VkPipeline* pPipeline);
typedef VkResult (VKAPI *PFN_vkLoadPipelineDerivative)(VkDevice device, size_t dataSize, const void* pData, VkPipeline basePipeline, VkPipeline* pPipeline);
typedef VkResult (VKAPI *PFN_vkCreateSampler)(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler);
typedef VkResult (VKAPI *PFN_vkCreateDescriptorSetLayout)(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout);
typedef VkResult (VKAPI *PFN_vkCreateDescriptorSetLayoutChain)(VkDevice device, uint32_t setLayoutArrayCount, const VkDescriptorSetLayout* pSetLayoutArray, VkDescriptorSetLayoutChain* pLayoutChain);
typedef VkResult (VKAPI *PFN_vkBeginDescriptorPoolUpdate)(VkDevice device, VkDescriptorUpdateMode updateMode);
typedef VkResult (VKAPI *PFN_vkEndDescriptorPoolUpdate)(VkDevice device, VkCmdBuffer cmd);
typedef VkResult (VKAPI *PFN_vkCreateDescriptorPool)(VkDevice device, VkDescriptorPoolUsage poolUsage, uint32_t maxSets, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool);
typedef VkResult (VKAPI *PFN_vkResetDescriptorPool)(VkDescriptorPool descriptorPool);
typedef VkResult (VKAPI *PFN_vkAllocDescriptorSets)(VkDescriptorPool descriptorPool, VkDescriptorSetUsage setUsage, uint32_t count, const VkDescriptorSetLayout* pSetLayouts, VkDescriptorSet* pDescriptorSets, uint32_t* pCount);
typedef void     (VKAPI *PFN_vkClearDescriptorSets)(VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets);
typedef void     (VKAPI *PFN_vkUpdateDescriptors)(VkDescriptorSet descriptorSet, uint32_t updateCount, const void** ppUpdateArray);
typedef VkResult (VKAPI *PFN_vkCreateDynamicViewportState)(VkDevice device, const VkDynamicVpStateCreateInfo* pCreateInfo, VkDynamicVpState* pState);
typedef VkResult (VKAPI *PFN_vkCreateDynamicRasterState)(VkDevice device, const VkDynamicRsStateCreateInfo* pCreateInfo, VkDynamicRsState* pState);
typedef VkResult (VKAPI *PFN_vkCreateDynamicColorBlendState)(VkDevice device, const VkDynamicCbStateCreateInfo* pCreateInfo, VkDynamicCbState* pState);
typedef VkResult (VKAPI *PFN_vkCreateDynamicDepthStencilState)(VkDevice device, const VkDynamicDsStateCreateInfo* pCreateInfo, VkDynamicDsState* pState);
typedef VkResult (VKAPI *PFN_vkCreateCommandBuffer)(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo, VkCmdBuffer* pCmdBuffer);
typedef VkResult (VKAPI *PFN_vkBeginCommandBuffer)(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo);
typedef VkResult (VKAPI *PFN_vkEndCommandBuffer)(VkCmdBuffer cmdBuffer);
typedef VkResult (VKAPI *PFN_vkResetCommandBuffer)(VkCmdBuffer cmdBuffer);
typedef void     (VKAPI *PFN_vkCmdBindPipeline)(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
typedef void     (VKAPI *PFN_vkCmdBindDynamicStateObject)(VkCmdBuffer cmdBuffer, VkStateBindPoint stateBindPoint, VkDynamicStateObject state);
typedef void     (VKAPI *PFN_vkCmdBindDescriptorSets)(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t layoutChainSlot, uint32_t count, const VkDescriptorSet* pDescriptorSets, const uint32_t* pUserData);
typedef void     (VKAPI *PFN_vkCmdBindIndexBuffer)(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
typedef void     (VKAPI *PFN_vkCmdBindVertexBuffers)(VkCmdBuffer cmdBuffer, uint32_t startBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
typedef void     (VKAPI *PFN_vkCmdDraw)(VkCmdBuffer cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount);
typedef void     (VKAPI *PFN_vkCmdDrawIndexed)(VkCmdBuffer cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount);
typedef void     (VKAPI *PFN_vkCmdDrawIndirect)(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride);
typedef void     (VKAPI *PFN_vkCmdDrawIndexedIndirect)(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride);
typedef void     (VKAPI *PFN_vkCmdDispatch)(VkCmdBuffer cmdBuffer, uint32_t x, uint32_t y, uint32_t z);
typedef void     (VKAPI *PFN_vkCmdDispatchIndirect)(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset);
typedef void     (VKAPI *PFN_vkCmdCopyBuffer)(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer destBuffer, uint32_t regionCount, const VkBufferCopy* pRegions);
typedef void     (VKAPI *PFN_vkCmdCopyImage)(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
typedef void     (VKAPI *PFN_vkCmdBlitImage)(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageBlit* pRegions);
typedef void     (VKAPI *PFN_vkCmdCopyBufferToImage)(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);
typedef void     (VKAPI *PFN_vkCmdCopyImageToBuffer)(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer destBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
typedef void     (VKAPI *PFN_vkCmdCloneImageData)(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout);
typedef void     (VKAPI *PFN_vkCmdUpdateBuffer)(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize dataSize, const uint32_t* pData);
typedef void     (VKAPI *PFN_vkCmdFillBuffer)(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize fillSize, uint32_t data);
typedef void     (VKAPI *PFN_vkCmdClearColorImage)(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, VkClearColor color, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
typedef void     (VKAPI *PFN_vkCmdClearDepthStencil)(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
typedef void     (VKAPI *PFN_vkCmdResolveImage)(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageResolve* pRegions);
typedef void     (VKAPI *PFN_vkCmdSetEvent)(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent);
typedef void     (VKAPI *PFN_vkCmdResetEvent)(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent);
typedef void     (VKAPI *PFN_vkCmdWaitEvents)(VkCmdBuffer cmdBuffer, VkWaitEvent waitEvent, uint32_t eventCount, const VkEvent* pEvents, uint32_t memBarrierCount, const void** ppMemBarriers);
typedef void     (VKAPI *PFN_vkCmdPipelineBarrier)(VkCmdBuffer cmdBuffer, VkWaitEvent waitEvent, uint32_t pipeEventCount, const VkPipeEvent* pPipeEvents, uint32_t memBarrierCount, const void** ppMemBarriers);
typedef void     (VKAPI *PFN_vkCmdBeginQuery)(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot, VkQueryControlFlags flags);
typedef void     (VKAPI *PFN_vkCmdEndQuery)(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot);
typedef void     (VKAPI *PFN_vkCmdResetQueryPool)(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount);
typedef void     (VKAPI *PFN_vkCmdWriteTimestamp)(VkCmdBuffer cmdBuffer, VkTimestampType timestampType, VkBuffer destBuffer, VkDeviceSize destOffset);
typedef void     (VKAPI *PFN_vkCmdCopyQueryPoolResults)(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize destStride, VkQueryResultFlags flags);
typedef void     (VKAPI *PFN_vkCmdInitAtomicCounters)(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData);
typedef void     (VKAPI *PFN_vkCmdLoadAtomicCounters)(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer srcBuffer, VkDeviceSize srcOffset);
typedef void     (VKAPI *PFN_vkCmdSaveAtomicCounters)(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer destBuffer, VkDeviceSize destOffset);
typedef VkResult (VKAPI *PFN_vkCreateFramebuffer)(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer);
typedef VkResult (VKAPI *PFN_vkCreateRenderPass)(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass);
typedef void     (VKAPI *PFN_vkCmdBeginRenderPass)(VkCmdBuffer cmdBuffer, const VkRenderPassBegin* pRenderPassBegin);
typedef void     (VKAPI *PFN_vkCmdEndRenderPass)(VkCmdBuffer cmdBuffer, VkRenderPass renderPass);

#ifdef VK_PROTOTYPES

// Device initialization

VkResult VKAPI vkCreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    VkInstance*                                 pInstance);

VkResult VKAPI vkDestroyInstance(
    VkInstance                                  instance);

VkResult VKAPI vkEnumeratePhysicalDevices(
    VkInstance                                  instance,
    uint32_t*                                   pPhysicalDeviceCount,
    VkPhysicalDevice*                           pPhysicalDevices);

VkResult VKAPI vkGetPhysicalDeviceInfo(
    VkPhysicalDevice                            physicalDevice,
    VkPhysicalDeviceInfoType                    infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

void * VKAPI vkGetProcAddr(
    VkPhysicalDevice                            physicalDevice,
    const char*                                 pName);

// Device functions

VkResult VKAPI vkCreateDevice(
    VkPhysicalDevice                            physicalDevice,
    const VkDeviceCreateInfo*                   pCreateInfo,
    VkDevice*                                   pDevice);

VkResult VKAPI vkDestroyDevice(
    VkDevice                                    device);

// Extension discovery functions
VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData);

VkResult VKAPI vkGetPhysicalDeviceExtensionInfo(
                                               VkPhysicalDevice gpu,
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData);

// Layer discovery functions

VkResult VKAPI vkEnumerateLayers(
    VkPhysicalDevice                            physicalDevice,
    size_t                                      maxLayerCount,
    size_t                                      maxStringSize,
    size_t*                                     pOutLayerCount,
    char* const*                                pOutLayers,
    void*                                       pReserved);

// Queue functions

VkResult VKAPI vkGetDeviceQueue(
    VkDevice                                    device,
    uint32_t                                    queueNodeIndex,
    uint32_t                                    queueIndex,
    VkQueue*                                    pQueue);

VkResult VKAPI vkQueueSubmit(
    VkQueue                                     queue,
    uint32_t                                    cmdBufferCount,
    const VkCmdBuffer*                          pCmdBuffers,
    VkFence                                     fence);

VkResult VKAPI vkQueueAddMemReferences(
    VkQueue                                     queue,
    uint32_t                                    count,
    const VkDeviceMemory*                       pMems);

VkResult VKAPI vkQueueRemoveMemReferences(
    VkQueue                                     queue,
    uint32_t                                    count,
    const VkDeviceMemory*                       pMems);

VkResult VKAPI vkQueueWaitIdle(
    VkQueue                                     queue);

VkResult VKAPI vkDeviceWaitIdle(
    VkDevice                                    device);

// Memory functions

VkResult VKAPI vkAllocMemory(
    VkDevice                                    device,
    const VkMemoryAllocInfo*                    pAllocInfo,
    VkDeviceMemory*                             pMem);

VkResult VKAPI vkFreeMemory(
    VkDeviceMemory                              mem);

VkResult VKAPI vkSetMemoryPriority(
    VkDeviceMemory                              mem,
    VkMemoryPriority                            priority);

VkResult VKAPI vkMapMemory(
    VkDeviceMemory                              mem,
    VkDeviceSize                                offset,
    VkDeviceSize                                size,
    VkMemoryMapFlags                            flags,
    void**                                      ppData);

VkResult VKAPI vkUnmapMemory(
    VkDeviceMemory                              mem);

VkResult VKAPI vkFlushMappedMemory(
    VkDeviceMemory                              mem,
    VkDeviceSize                                offset,
    VkDeviceSize                                size);

VkResult VKAPI vkPinSystemMemory(
    VkDevice                                    device,
    const void*                                 pSysMem,
    size_t                                      memSize,
    VkDeviceMemory*                             pMem);

// Multi-device functions

VkResult VKAPI vkGetMultiDeviceCompatibility(
    VkPhysicalDevice                            physicalDevice0,
    VkPhysicalDevice                            physicalDevice1,
    VkPhysicalDeviceCompatibilityInfo*          pInfo);

VkResult VKAPI vkOpenSharedMemory(
    VkDevice                                    device,
    const VkMemoryOpenInfo*                     pOpenInfo,
    VkDeviceMemory*                             pMem);

VkResult VKAPI vkOpenSharedSemaphore(
    VkDevice                                    device,
    const VkSemaphoreOpenInfo*                  pOpenInfo,
    VkSemaphore*                                pSemaphore);

VkResult VKAPI vkOpenPeerMemory(
    VkDevice                                    device,
    const VkPeerMemoryOpenInfo*                 pOpenInfo,
    VkDeviceMemory*                             pMem);

VkResult VKAPI vkOpenPeerImage(
    VkDevice                                    device,
    const VkPeerImageOpenInfo*                  pOpenInfo,
    VkImage*                                    pImage,
    VkDeviceMemory*                             pMem);

// Generic API object functions

VkResult VKAPI vkDestroyObject(
    VkObject                                    object);

VkResult VKAPI vkGetObjectInfo(
    VkBaseObject                                object,
    VkObjectInfoType                            infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

// Memory management API functions

VkResult VKAPI vkQueueBindObjectMemory(
    VkQueue                                     queue,
    VkObject                                    object,
    uint32_t                                    allocationIdx,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset);

VkResult VKAPI vkQueueBindObjectMemoryRange(
    VkQueue                                     queue,
    VkObject                                    object,
    uint32_t                                    allocationIdx,
    VkDeviceSize                                rangeOffset,
    VkDeviceSize                                rangeSize,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset);

VkResult VKAPI vkQueueBindImageMemoryRange(
    VkQueue                                     queue,
    VkImage                                     image,
    uint32_t                                    allocationIdx,
    const VkImageMemoryBindInfo*                pBindInfo,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset);

// Fence functions

VkResult VKAPI vkCreateFence(
    VkDevice                                    device,
    const VkFenceCreateInfo*                    pCreateInfo,
    VkFence*                                    pFence);

VkResult VKAPI vkResetFences(
    VkDevice                                    device,
    uint32_t                                    fenceCount,
    VkFence*                                    pFences);

VkResult VKAPI vkGetFenceStatus(
    VkFence                                     fence);

VkResult VKAPI vkWaitForFences(
    VkDevice                                    device,
    uint32_t                                    fenceCount,
    const VkFence*                              pFences,
    bool32_t                                    waitAll,
    uint64_t                                    timeout); // timeout in nanoseconds

// Queue semaphore functions

VkResult VKAPI vkCreateSemaphore(
    VkDevice                                    device,
    const VkSemaphoreCreateInfo*                pCreateInfo,
    VkSemaphore*                                pSemaphore);

VkResult VKAPI vkQueueSignalSemaphore(
    VkQueue                                     queue,
    VkSemaphore                                 semaphore);

VkResult VKAPI vkQueueWaitSemaphore(
    VkQueue                                     queue,
    VkSemaphore                                 semaphore);

// Event functions

VkResult VKAPI vkCreateEvent(
    VkDevice                                    device,
    const VkEventCreateInfo*                    pCreateInfo,
    VkEvent*                                    pEvent);

VkResult VKAPI vkGetEventStatus(
    VkEvent                                     event);

VkResult VKAPI vkSetEvent(
    VkEvent                                     event);

VkResult VKAPI vkResetEvent(
    VkEvent                                     event);

// Query functions

VkResult VKAPI vkCreateQueryPool(
    VkDevice                                    device,
    const VkQueryPoolCreateInfo*                pCreateInfo,
    VkQueryPool*                                pQueryPool);

VkResult VKAPI vkGetQueryPoolResults(
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    size_t*                                     pDataSize,
    void*                                       pData,
    VkQueryResultFlags                          flags);

// Format capabilities

VkResult VKAPI vkGetFormatInfo(
    VkDevice                                    device,
    VkFormat                                    format,
    VkFormatInfoType                            infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

// Buffer functions

VkResult VKAPI vkCreateBuffer(
    VkDevice                                    device,
    const VkBufferCreateInfo*                   pCreateInfo,
    VkBuffer*                                   pBuffer);

// Buffer view functions

VkResult VKAPI vkCreateBufferView(
    VkDevice                                    device,
    const VkBufferViewCreateInfo*               pCreateInfo,
    VkBufferView*                               pView);

// Image functions

VkResult VKAPI vkCreateImage(
    VkDevice                                    device,
    const VkImageCreateInfo*                    pCreateInfo,
    VkImage*                                    pImage);

VkResult VKAPI vkGetImageSubresourceInfo(
    VkImage                                     image,
    const VkImageSubresource*                   pSubresource,
    VkSubresourceInfoType                       infoType,
    size_t*                                     pDataSize,
    void*                                       pData);

// Image view functions

VkResult VKAPI vkCreateImageView(
    VkDevice                                    device,
    const VkImageViewCreateInfo*                pCreateInfo,
    VkImageView*                                pView);

VkResult VKAPI vkCreateColorAttachmentView(
    VkDevice                                    device,
    const VkColorAttachmentViewCreateInfo*      pCreateInfo,
    VkColorAttachmentView*                      pView);

VkResult VKAPI vkCreateDepthStencilView(
    VkDevice                                    device,
    const VkDepthStencilViewCreateInfo*         pCreateInfo,
    VkDepthStencilView*                         pView);

// Shader functions

VkResult VKAPI vkCreateShader(
    VkDevice                                    device,
    const VkShaderCreateInfo*                   pCreateInfo,
    VkShader*                                   pShader);

// Pipeline functions

VkResult VKAPI vkCreateGraphicsPipeline(
    VkDevice                                    device,
    const VkGraphicsPipelineCreateInfo*         pCreateInfo,
    VkPipeline*                                 pPipeline);

VkResult VKAPI vkCreateGraphicsPipelineDerivative(
    VkDevice                                    device,
    const VkGraphicsPipelineCreateInfo*         pCreateInfo,
    VkPipeline                                  basePipeline,
    VkPipeline*                                 pPipeline);

VkResult VKAPI vkCreateComputePipeline(
    VkDevice                                    device,
    const VkComputePipelineCreateInfo*          pCreateInfo,
    VkPipeline*                                 pPipeline);

VkResult VKAPI vkStorePipeline(
    VkPipeline                                  pipeline,
    size_t*                                     pDataSize,
    void*                                       pData);

VkResult VKAPI vkLoadPipeline(
    VkDevice                                    device,
    size_t                                      dataSize,
    const void*                                 pData,
    VkPipeline*                                 pPipeline);

VkResult VKAPI vkLoadPipelineDerivative(
    VkDevice                                    device,
    size_t                                      dataSize,
    const void*                                 pData,
    VkPipeline                                  basePipeline,
    VkPipeline*                                 pPipeline);

// Sampler functions

VkResult VKAPI vkCreateSampler(
    VkDevice                                    device,
    const VkSamplerCreateInfo*                  pCreateInfo,
    VkSampler*                                  pSampler);

// Descriptor set functions

VkResult VKAPI vkCreateDescriptorSetLayout(
    VkDevice                                    device,
    const VkDescriptorSetLayoutCreateInfo*      pCreateInfo,
    VkDescriptorSetLayout*                      pSetLayout);

VkResult VKAPI vkCreateDescriptorSetLayoutChain(
    VkDevice                                    device,
    uint32_t                                    setLayoutArrayCount,
    const VkDescriptorSetLayout*                pSetLayoutArray,
    VkDescriptorSetLayoutChain*                 pLayoutChain);

VkResult VKAPI vkBeginDescriptorPoolUpdate(
    VkDevice                                    device,
    VkDescriptorUpdateMode                      updateMode);

VkResult VKAPI vkEndDescriptorPoolUpdate(
    VkDevice                                    device,
    VkCmdBuffer                                 cmd);

VkResult VKAPI vkCreateDescriptorPool(
    VkDevice                                    device,
    VkDescriptorPoolUsage                       poolUsage,
    uint32_t                                    maxSets,
    const VkDescriptorPoolCreateInfo*           pCreateInfo,
    VkDescriptorPool*                           pDescriptorPool);

VkResult VKAPI vkResetDescriptorPool(
    VkDescriptorPool                            descriptorPool);

VkResult VKAPI vkAllocDescriptorSets(
    VkDescriptorPool                            descriptorPool,
    VkDescriptorSetUsage                        setUsage,
    uint32_t                                    count,
    const VkDescriptorSetLayout*                pSetLayouts,
    VkDescriptorSet*                            pDescriptorSets,
    uint32_t*                                   pCount);

void VKAPI vkClearDescriptorSets(
    VkDescriptorPool                            descriptorPool,
    uint32_t                                    count,
    const VkDescriptorSet*                      pDescriptorSets);

void VKAPI vkUpdateDescriptors(
    VkDescriptorSet                             descriptorSet,
    uint32_t                                    updateCount,
    const void**                                ppUpdateArray);

// State object functions

VkResult VKAPI vkCreateDynamicViewportState(
    VkDevice                                    device,
    const VkDynamicVpStateCreateInfo*           pCreateInfo,
    VkDynamicVpState*                           pState);

VkResult VKAPI vkCreateDynamicRasterState(
    VkDevice                                    device,
    const VkDynamicRsStateCreateInfo*           pCreateInfo,
    VkDynamicRsState*                           pState);

VkResult VKAPI vkCreateDynamicColorBlendState(
    VkDevice                                    device,
    const VkDynamicCbStateCreateInfo*           pCreateInfo,
    VkDynamicCbState*                           pState);

VkResult VKAPI vkCreateDynamicDepthStencilState(
    VkDevice                                    device,
    const VkDynamicDsStateCreateInfo*           pCreateInfo,
    VkDynamicDsState*                           pState);

// Command buffer functions

VkResult VKAPI vkCreateCommandBuffer(
    VkDevice                                    device,
    const VkCmdBufferCreateInfo*                pCreateInfo,
    VkCmdBuffer*                                pCmdBuffer);

VkResult VKAPI vkBeginCommandBuffer(
    VkCmdBuffer                                 cmdBuffer,
    const VkCmdBufferBeginInfo*                 pBeginInfo);

VkResult VKAPI vkEndCommandBuffer(
    VkCmdBuffer                                 cmdBuffer);

VkResult VKAPI vkResetCommandBuffer(
    VkCmdBuffer                                 cmdBuffer);

// Command buffer building functions

void VKAPI vkCmdBindPipeline(
    VkCmdBuffer                                 cmdBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    VkPipeline                                  pipeline);

void VKAPI vkCmdBindDynamicStateObject(
    VkCmdBuffer                                 cmdBuffer,
    VkStateBindPoint                            stateBindPoint,
    VkDynamicStateObject                        dynamicState);

void VKAPI vkCmdBindDescriptorSets(
    VkCmdBuffer                                 cmdBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    uint32_t                                    layoutChainSlot,
    uint32_t                                    count,
    const VkDescriptorSet*                      pDescriptorSets,
    const uint32_t*                             pUserData);

void VKAPI vkCmdBindIndexBuffer(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    VkIndexType                                 indexType);

void VKAPI vkCmdBindVertexBuffers(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    startBinding,
    uint32_t                                    bindingCount,
    const VkBuffer*                             pBuffers,
    const VkDeviceSize*                         pOffsets);

void VKAPI vkCmdDraw(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    firstVertex,
    uint32_t                                    vertexCount,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount);

void VKAPI vkCmdDrawIndexed(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    firstIndex,
    uint32_t                                    indexCount,
    int32_t                                     vertexOffset,
    uint32_t                                    firstInstance,
    uint32_t                                    instanceCount);

void VKAPI vkCmdDrawIndirect(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    count,
    uint32_t                                    stride);

void VKAPI vkCmdDrawIndexedIndirect(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    count,
    uint32_t                                    stride);

void VKAPI vkCmdDispatch(
    VkCmdBuffer                                 cmdBuffer,
    uint32_t                                    x,
    uint32_t                                    y,
    uint32_t                                    z);

void VKAPI vkCmdDispatchIndirect(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset);

void VKAPI vkCmdCopyBuffer(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    srcBuffer,
    VkBuffer                                    destBuffer,
    uint32_t                                    regionCount,
    const VkBufferCopy*                         pRegions);

void VKAPI vkCmdCopyImage(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout,
    uint32_t                                    regionCount,
    const VkImageCopy*                          pRegions);

void VKAPI vkCmdBlitImage(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout,
    uint32_t                                    regionCount,
    const VkImageBlit*                          pRegions);

void VKAPI vkCmdCopyBufferToImage(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    srcBuffer,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions);

void VKAPI vkCmdCopyImageToBuffer(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkBuffer                                    destBuffer,
    uint32_t                                    regionCount,
    const VkBufferImageCopy*                    pRegions);

void VKAPI vkCmdCloneImageData(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout);

void VKAPI vkCmdUpdateBuffer(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    destBuffer,
    VkDeviceSize                                destOffset,
    VkDeviceSize                                dataSize,
    const uint32_t*                             pData);

void VKAPI vkCmdFillBuffer(
    VkCmdBuffer                                 cmdBuffer,
    VkBuffer                                    destBuffer,
    VkDeviceSize                                destOffset,
    VkDeviceSize                                fillSize,
    uint32_t                                    data);

void VKAPI vkCmdClearColorImage(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    VkClearColor                                color,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges);

void VKAPI vkCmdClearDepthStencil(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     image,
    VkImageLayout                               imageLayout,
    float                                       depth,
    uint32_t                                    stencil,
    uint32_t                                    rangeCount,
    const VkImageSubresourceRange*              pRanges);

void VKAPI vkCmdResolveImage(
    VkCmdBuffer                                 cmdBuffer,
    VkImage                                     srcImage,
    VkImageLayout                               srcImageLayout,
    VkImage                                     destImage,
    VkImageLayout                               destImageLayout,
    uint32_t                                    regionCount,
    const VkImageResolve*                       pRegions);

void VKAPI vkCmdSetEvent(
    VkCmdBuffer                                 cmdBuffer,
    VkEvent                                     event,
    VkPipeEvent                                 pipeEvent);

void VKAPI vkCmdResetEvent(
    VkCmdBuffer                                 cmdBuffer,
    VkEvent                                     event,
    VkPipeEvent                                 pipeEvent);

void VKAPI vkCmdWaitEvents(
    VkCmdBuffer                                 cmdBuffer,
    VkWaitEvent                                 waitEvent,
    uint32_t                                    eventCount,
    const VkEvent*                              pEvents,
    uint32_t                                    memBarrierCount,
    const void**                                ppMemBarriers);

void VKAPI vkCmdPipelineBarrier(
    VkCmdBuffer                                 cmdBuffer,
    VkWaitEvent                                 waitEvent,
    uint32_t                                    pipeEventCount,
    const VkPipeEvent*                          pPipeEvents,
    uint32_t                                    memBarrierCount,
    const void**                                ppMemBarriers);

void VKAPI vkCmdBeginQuery(
    VkCmdBuffer                                 cmdBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    slot,
    VkQueryControlFlags                         flags);

void VKAPI vkCmdEndQuery(
    VkCmdBuffer                                 cmdBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    slot);

void VKAPI vkCmdResetQueryPool(
    VkCmdBuffer                                 cmdBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount);

void VKAPI vkCmdWriteTimestamp(
    VkCmdBuffer                                 cmdBuffer,
    VkTimestampType                             timestampType,
    VkBuffer                                    destBuffer,
    VkDeviceSize                                destOffset);

void VKAPI vkCmdCopyQueryPoolResults(
    VkCmdBuffer                                 cmdBuffer,
    VkQueryPool                                 queryPool,
    uint32_t                                    startQuery,
    uint32_t                                    queryCount,
    VkBuffer                                    destBuffer,
    VkDeviceSize                                destOffset,
    VkDeviceSize                                destStride,
    VkQueryResultFlags                          flags);

void VKAPI vkCmdInitAtomicCounters(
    VkCmdBuffer                                 cmdBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    const uint32_t*                             pData);

void VKAPI vkCmdLoadAtomicCounters(
    VkCmdBuffer                                 cmdBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    VkBuffer                                    srcBuffer,
    VkDeviceSize                                srcOffset);

void VKAPI vkCmdSaveAtomicCounters(
    VkCmdBuffer                                 cmdBuffer,
    VkPipelineBindPoint                         pipelineBindPoint,
    uint32_t                                    startCounter,
    uint32_t                                    counterCount,
    VkBuffer                                    destBuffer,
    VkDeviceSize                                destOffset);

VkResult VKAPI vkCreateFramebuffer(
    VkDevice                                    device,
    const VkFramebufferCreateInfo*              pCreateInfo,
    VkFramebuffer*                              pFramebuffer);

VkResult VKAPI vkCreateRenderPass(
    VkDevice                                    device,
    const VkRenderPassCreateInfo*               pCreateInfo,
    VkRenderPass*                               pRenderPass);

void VKAPI vkCmdBeginRenderPass(
    VkCmdBuffer                                 cmdBuffer,
    const VkRenderPassBegin*                    pRenderPassBegin);

void VKAPI vkCmdEndRenderPass(
    VkCmdBuffer                                 cmdBuffer,
    VkRenderPass                                renderPass);

#endif // VK_PROTOTYPES

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __VULKAN_H__
