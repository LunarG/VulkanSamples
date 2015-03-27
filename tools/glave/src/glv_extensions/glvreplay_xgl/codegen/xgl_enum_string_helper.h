#pragma once

#include <xgl.h>


static inline const char* string_XGL_PRIMITIVE_TOPOLOGY(XGL_PRIMITIVE_TOPOLOGY input_value)
{
    switch ((XGL_PRIMITIVE_TOPOLOGY)input_value)
    {
        case XGL_TOPOLOGY_LINE_LIST:
            return "XGL_TOPOLOGY_LINE_LIST";
        case XGL_TOPOLOGY_LINE_LIST_ADJ:
            return "XGL_TOPOLOGY_LINE_LIST_ADJ";
        case XGL_TOPOLOGY_LINE_STRIP:
            return "XGL_TOPOLOGY_LINE_STRIP";
        case XGL_TOPOLOGY_LINE_STRIP_ADJ:
            return "XGL_TOPOLOGY_LINE_STRIP_ADJ";
        case XGL_TOPOLOGY_PATCH:
            return "XGL_TOPOLOGY_PATCH";
        case XGL_TOPOLOGY_POINT_LIST:
            return "XGL_TOPOLOGY_POINT_LIST";
        case XGL_TOPOLOGY_QUAD_LIST:
            return "XGL_TOPOLOGY_QUAD_LIST";
        case XGL_TOPOLOGY_QUAD_STRIP:
            return "XGL_TOPOLOGY_QUAD_STRIP";
        case XGL_TOPOLOGY_RECT_LIST:
            return "XGL_TOPOLOGY_RECT_LIST";
        case XGL_TOPOLOGY_TRIANGLE_LIST:
            return "XGL_TOPOLOGY_TRIANGLE_LIST";
        case XGL_TOPOLOGY_TRIANGLE_LIST_ADJ:
            return "XGL_TOPOLOGY_TRIANGLE_LIST_ADJ";
        case XGL_TOPOLOGY_TRIANGLE_STRIP:
            return "XGL_TOPOLOGY_TRIANGLE_STRIP";
        case XGL_TOPOLOGY_TRIANGLE_STRIP_ADJ:
            return "XGL_TOPOLOGY_TRIANGLE_STRIP_ADJ";
        default:
            return "Unhandled XGL_PRIMITIVE_TOPOLOGY";
    }
}


static inline const char* string_XGL_BLEND_FUNC(XGL_BLEND_FUNC input_value)
{
    switch ((XGL_BLEND_FUNC)input_value)
    {
        case XGL_BLEND_FUNC_ADD:
            return "XGL_BLEND_FUNC_ADD";
        case XGL_BLEND_FUNC_MAX:
            return "XGL_BLEND_FUNC_MAX";
        case XGL_BLEND_FUNC_MIN:
            return "XGL_BLEND_FUNC_MIN";
        case XGL_BLEND_FUNC_REVERSE_SUBTRACT:
            return "XGL_BLEND_FUNC_REVERSE_SUBTRACT";
        case XGL_BLEND_FUNC_SUBTRACT:
            return "XGL_BLEND_FUNC_SUBTRACT";
        default:
            return "Unhandled XGL_BLEND_FUNC";
    }
}


static inline const char* string_XGL_MEMORY_REF_FLAGS(XGL_MEMORY_REF_FLAGS input_value)
{
    switch ((XGL_MEMORY_REF_FLAGS)input_value)
    {
        case XGL_MEMORY_REF_READ_ONLY_BIT:
            return "XGL_MEMORY_REF_READ_ONLY_BIT";
        default:
            return "Unhandled XGL_MEMORY_REF_FLAGS";
    }
}


static inline const char* string_XGL_IMAGE_VIEW_TYPE(XGL_IMAGE_VIEW_TYPE input_value)
{
    switch ((XGL_IMAGE_VIEW_TYPE)input_value)
    {
        case XGL_IMAGE_VIEW_1D:
            return "XGL_IMAGE_VIEW_1D";
        case XGL_IMAGE_VIEW_2D:
            return "XGL_IMAGE_VIEW_2D";
        case XGL_IMAGE_VIEW_3D:
            return "XGL_IMAGE_VIEW_3D";
        case XGL_IMAGE_VIEW_CUBE:
            return "XGL_IMAGE_VIEW_CUBE";
        default:
            return "Unhandled XGL_IMAGE_VIEW_TYPE";
    }
}


static inline const char* string_XGL_MEMORY_OUTPUT_FLAGS(XGL_MEMORY_OUTPUT_FLAGS input_value)
{
    switch ((XGL_MEMORY_OUTPUT_FLAGS)input_value)
    {
        case XGL_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT:
            return "XGL_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT";
        case XGL_MEMORY_OUTPUT_COPY_BIT:
            return "XGL_MEMORY_OUTPUT_COPY_BIT";
        case XGL_MEMORY_OUTPUT_CPU_WRITE_BIT:
            return "XGL_MEMORY_OUTPUT_CPU_WRITE_BIT";
        case XGL_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT:
            return "XGL_MEMORY_OUTPUT_DEPTH_STENCIL_ATTACHMENT_BIT";
        case XGL_MEMORY_OUTPUT_SHADER_WRITE_BIT:
            return "XGL_MEMORY_OUTPUT_SHADER_WRITE_BIT";
        default:
            return "Unhandled XGL_MEMORY_OUTPUT_FLAGS";
    }
}


static inline const char* string_XGL_DESCRIPTOR_SET_USAGE(XGL_DESCRIPTOR_SET_USAGE input_value)
{
    switch ((XGL_DESCRIPTOR_SET_USAGE)input_value)
    {
        case XGL_DESCRIPTOR_SET_USAGE_ONE_SHOT:
            return "XGL_DESCRIPTOR_SET_USAGE_ONE_SHOT";
        case XGL_DESCRIPTOR_SET_USAGE_STATIC:
            return "XGL_DESCRIPTOR_SET_USAGE_STATIC";
        default:
            return "Unhandled XGL_DESCRIPTOR_SET_USAGE";
    }
}


static inline const char* string_XGL_STATE_BIND_POINT(XGL_STATE_BIND_POINT input_value)
{
    switch ((XGL_STATE_BIND_POINT)input_value)
    {
        case XGL_STATE_BIND_COLOR_BLEND:
            return "XGL_STATE_BIND_COLOR_BLEND";
        case XGL_STATE_BIND_DEPTH_STENCIL:
            return "XGL_STATE_BIND_DEPTH_STENCIL";
        case XGL_STATE_BIND_RASTER:
            return "XGL_STATE_BIND_RASTER";
        case XGL_STATE_BIND_VIEWPORT:
            return "XGL_STATE_BIND_VIEWPORT";
        default:
            return "Unhandled XGL_STATE_BIND_POINT";
    }
}


static inline const char* string_XGL_LOGIC_OP(XGL_LOGIC_OP input_value)
{
    switch ((XGL_LOGIC_OP)input_value)
    {
        case XGL_LOGIC_OP_AND:
            return "XGL_LOGIC_OP_AND";
        case XGL_LOGIC_OP_AND_INVERTED:
            return "XGL_LOGIC_OP_AND_INVERTED";
        case XGL_LOGIC_OP_AND_REVERSE:
            return "XGL_LOGIC_OP_AND_REVERSE";
        case XGL_LOGIC_OP_CLEAR:
            return "XGL_LOGIC_OP_CLEAR";
        case XGL_LOGIC_OP_COPY:
            return "XGL_LOGIC_OP_COPY";
        case XGL_LOGIC_OP_COPY_INVERTED:
            return "XGL_LOGIC_OP_COPY_INVERTED";
        case XGL_LOGIC_OP_EQUIV:
            return "XGL_LOGIC_OP_EQUIV";
        case XGL_LOGIC_OP_INVERT:
            return "XGL_LOGIC_OP_INVERT";
        case XGL_LOGIC_OP_NAND:
            return "XGL_LOGIC_OP_NAND";
        case XGL_LOGIC_OP_NOOP:
            return "XGL_LOGIC_OP_NOOP";
        case XGL_LOGIC_OP_NOR:
            return "XGL_LOGIC_OP_NOR";
        case XGL_LOGIC_OP_OR:
            return "XGL_LOGIC_OP_OR";
        case XGL_LOGIC_OP_OR_INVERTED:
            return "XGL_LOGIC_OP_OR_INVERTED";
        case XGL_LOGIC_OP_OR_REVERSE:
            return "XGL_LOGIC_OP_OR_REVERSE";
        case XGL_LOGIC_OP_SET:
            return "XGL_LOGIC_OP_SET";
        case XGL_LOGIC_OP_XOR:
            return "XGL_LOGIC_OP_XOR";
        default:
            return "Unhandled XGL_LOGIC_OP";
    }
}


static inline const char* string_XGL_DESCRIPTOR_TYPE(XGL_DESCRIPTOR_TYPE input_value)
{
    switch ((XGL_DESCRIPTOR_TYPE)input_value)
    {
        case XGL_DESCRIPTOR_TYPE_IMAGE:
            return "XGL_DESCRIPTOR_TYPE_IMAGE";
        case XGL_DESCRIPTOR_TYPE_IMAGE_BUFFER:
            return "XGL_DESCRIPTOR_TYPE_IMAGE_BUFFER";
        case XGL_DESCRIPTOR_TYPE_RAW_BUFFER:
            return "XGL_DESCRIPTOR_TYPE_RAW_BUFFER";
        case XGL_DESCRIPTOR_TYPE_RAW_BUFFER_DYNAMIC:
            return "XGL_DESCRIPTOR_TYPE_RAW_BUFFER_DYNAMIC";
        case XGL_DESCRIPTOR_TYPE_SAMPLER:
            return "XGL_DESCRIPTOR_TYPE_SAMPLER";
        case XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE:
            return "XGL_DESCRIPTOR_TYPE_SAMPLER_TEXTURE";
        case XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER:
            return "XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER";
        case XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER_DYNAMIC:
            return "XGL_DESCRIPTOR_TYPE_SHADER_STORAGE_BUFFER_DYNAMIC";
        case XGL_DESCRIPTOR_TYPE_TEXTURE:
            return "XGL_DESCRIPTOR_TYPE_TEXTURE";
        case XGL_DESCRIPTOR_TYPE_TEXTURE_BUFFER:
            return "XGL_DESCRIPTOR_TYPE_TEXTURE_BUFFER";
        case XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return "XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        case XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return "XGL_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
        default:
            return "Unhandled XGL_DESCRIPTOR_TYPE";
    }
}


static inline const char* string_XGL_SUBRESOURCE_INFO_TYPE(XGL_SUBRESOURCE_INFO_TYPE input_value)
{
    switch ((XGL_SUBRESOURCE_INFO_TYPE)input_value)
    {
        case XGL_INFO_TYPE_SUBRESOURCE_LAYOUT:
            return "XGL_INFO_TYPE_SUBRESOURCE_LAYOUT";
        default:
            return "Unhandled XGL_SUBRESOURCE_INFO_TYPE";
    }
}


static inline const char* string_XGL_MEMORY_TYPE(XGL_MEMORY_TYPE input_value)
{
    switch ((XGL_MEMORY_TYPE)input_value)
    {
        case XGL_MEMORY_TYPE_BUFFER:
            return "XGL_MEMORY_TYPE_BUFFER";
        case XGL_MEMORY_TYPE_IMAGE:
            return "XGL_MEMORY_TYPE_IMAGE";
        case XGL_MEMORY_TYPE_OTHER:
            return "XGL_MEMORY_TYPE_OTHER";
        default:
            return "Unhandled XGL_MEMORY_TYPE";
    }
}


static inline const char* string_XGL_MEMORY_PRIORITY(XGL_MEMORY_PRIORITY input_value)
{
    switch ((XGL_MEMORY_PRIORITY)input_value)
    {
        case XGL_MEMORY_PRIORITY_HIGH:
            return "XGL_MEMORY_PRIORITY_HIGH";
        case XGL_MEMORY_PRIORITY_LOW:
            return "XGL_MEMORY_PRIORITY_LOW";
        case XGL_MEMORY_PRIORITY_NORMAL:
            return "XGL_MEMORY_PRIORITY_NORMAL";
        case XGL_MEMORY_PRIORITY_UNUSED:
            return "XGL_MEMORY_PRIORITY_UNUSED";
        case XGL_MEMORY_PRIORITY_VERY_HIGH:
            return "XGL_MEMORY_PRIORITY_VERY_HIGH";
        case XGL_MEMORY_PRIORITY_VERY_LOW:
            return "XGL_MEMORY_PRIORITY_VERY_LOW";
        default:
            return "Unhandled XGL_MEMORY_PRIORITY";
    }
}


static inline const char* string_XGL_VERTEX_INPUT_STEP_RATE(XGL_VERTEX_INPUT_STEP_RATE input_value)
{
    switch ((XGL_VERTEX_INPUT_STEP_RATE)input_value)
    {
        case XGL_VERTEX_INPUT_STEP_RATE_DRAW:
            return "XGL_VERTEX_INPUT_STEP_RATE_DRAW";
        case XGL_VERTEX_INPUT_STEP_RATE_INSTANCE:
            return "XGL_VERTEX_INPUT_STEP_RATE_INSTANCE";
        case XGL_VERTEX_INPUT_STEP_RATE_VERTEX:
            return "XGL_VERTEX_INPUT_STEP_RATE_VERTEX";
        default:
            return "Unhandled XGL_VERTEX_INPUT_STEP_RATE";
    }
}


static inline const char* string_XGL_DESCRIPTOR_UPDATE_MODE(XGL_DESCRIPTOR_UPDATE_MODE input_value)
{
    switch ((XGL_DESCRIPTOR_UPDATE_MODE)input_value)
    {
        case XGL_DESCRIPTOR_UDPATE_MODE_COPY:
            return "XGL_DESCRIPTOR_UDPATE_MODE_COPY";
        case XGL_DESCRIPTOR_UPDATE_MODE_FASTEST:
            return "XGL_DESCRIPTOR_UPDATE_MODE_FASTEST";
        default:
            return "Unhandled XGL_DESCRIPTOR_UPDATE_MODE";
    }
}


static inline const char* string_XGL_ATTACHMENT_STORE_OP(XGL_ATTACHMENT_STORE_OP input_value)
{
    switch ((XGL_ATTACHMENT_STORE_OP)input_value)
    {
        case XGL_ATTACHMENT_STORE_OP_DONT_CARE:
            return "XGL_ATTACHMENT_STORE_OP_DONT_CARE";
        case XGL_ATTACHMENT_STORE_OP_RESOLVE_MSAA:
            return "XGL_ATTACHMENT_STORE_OP_RESOLVE_MSAA";
        case XGL_ATTACHMENT_STORE_OP_STORE:
            return "XGL_ATTACHMENT_STORE_OP_STORE";
        default:
            return "Unhandled XGL_ATTACHMENT_STORE_OP";
    }
}


static inline const char* string_XGL_QUEUE_TYPE(XGL_QUEUE_TYPE input_value)
{
    switch ((XGL_QUEUE_TYPE)input_value)
    {
        case XGL_QUEUE_TYPE_COMPUTE:
            return "XGL_QUEUE_TYPE_COMPUTE";
        case XGL_QUEUE_TYPE_DMA:
            return "XGL_QUEUE_TYPE_DMA";
        case XGL_QUEUE_TYPE_GRAPHICS:
            return "XGL_QUEUE_TYPE_GRAPHICS";
        default:
            return "Unhandled XGL_QUEUE_TYPE";
    }
}


static inline const char* string_XGL_SYSTEM_ALLOC_TYPE(XGL_SYSTEM_ALLOC_TYPE input_value)
{
    switch ((XGL_SYSTEM_ALLOC_TYPE)input_value)
    {
        case XGL_SYSTEM_ALLOC_API_OBJECT:
            return "XGL_SYSTEM_ALLOC_API_OBJECT";
        case XGL_SYSTEM_ALLOC_DEBUG:
            return "XGL_SYSTEM_ALLOC_DEBUG";
        case XGL_SYSTEM_ALLOC_INTERNAL:
            return "XGL_SYSTEM_ALLOC_INTERNAL";
        case XGL_SYSTEM_ALLOC_INTERNAL_SHADER:
            return "XGL_SYSTEM_ALLOC_INTERNAL_SHADER";
        case XGL_SYSTEM_ALLOC_INTERNAL_TEMP:
            return "XGL_SYSTEM_ALLOC_INTERNAL_TEMP";
        default:
            return "Unhandled XGL_SYSTEM_ALLOC_TYPE";
    }
}


static inline const char* string_XGL_DEPTH_MODE(XGL_DEPTH_MODE input_value)
{
    switch ((XGL_DEPTH_MODE)input_value)
    {
        case XGL_DEPTH_MODE_NEGATIVE_ONE_TO_ONE:
            return "XGL_DEPTH_MODE_NEGATIVE_ONE_TO_ONE";
        case XGL_DEPTH_MODE_ZERO_TO_ONE:
            return "XGL_DEPTH_MODE_ZERO_TO_ONE";
        default:
            return "Unhandled XGL_DEPTH_MODE";
    }
}


static inline const char* string_XGL_IMAGE_TILING(XGL_IMAGE_TILING input_value)
{
    switch ((XGL_IMAGE_TILING)input_value)
    {
        case XGL_LINEAR_TILING:
            return "XGL_LINEAR_TILING";
        case XGL_OPTIMAL_TILING:
            return "XGL_OPTIMAL_TILING";
        default:
            return "Unhandled XGL_IMAGE_TILING";
    }
}


static inline const char* string_XGL_IMAGE_USAGE_FLAGS(XGL_IMAGE_USAGE_FLAGS input_value)
{
    switch ((XGL_IMAGE_USAGE_FLAGS)input_value)
    {
        case XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
            return "XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";
        case XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT:
            return "XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT";
        case XGL_IMAGE_USAGE_GENERAL:
            return "XGL_IMAGE_USAGE_GENERAL";
        case XGL_IMAGE_USAGE_IMAGE_BIT:
            return "XGL_IMAGE_USAGE_IMAGE_BIT";
        case XGL_IMAGE_USAGE_SHADER_ACCESS_ATOMIC_BIT:
            return "XGL_IMAGE_USAGE_SHADER_ACCESS_ATOMIC_BIT";
        case XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT:
            return "XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT";
        case XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT:
            return "XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT";
        case XGL_IMAGE_USAGE_TEXTURE_BIT:
            return "XGL_IMAGE_USAGE_TEXTURE_BIT";
        case XGL_IMAGE_USAGE_TRANSFER_DESTINATION_BIT:
            return "XGL_IMAGE_USAGE_TRANSFER_DESTINATION_BIT";
        case XGL_IMAGE_USAGE_TRANSFER_SOURCE_BIT:
            return "XGL_IMAGE_USAGE_TRANSFER_SOURCE_BIT";
        default:
            return "Unhandled XGL_IMAGE_USAGE_FLAGS";
    }
}


static inline const char* string_XGL_PHYSICAL_GPU_TYPE(XGL_PHYSICAL_GPU_TYPE input_value)
{
    switch ((XGL_PHYSICAL_GPU_TYPE)input_value)
    {
        case XGL_GPU_TYPE_DISCRETE:
            return "XGL_GPU_TYPE_DISCRETE";
        case XGL_GPU_TYPE_INTEGRATED:
            return "XGL_GPU_TYPE_INTEGRATED";
        case XGL_GPU_TYPE_OTHER:
            return "XGL_GPU_TYPE_OTHER";
        case XGL_GPU_TYPE_VIRTUAL:
            return "XGL_GPU_TYPE_VIRTUAL";
        default:
            return "Unhandled XGL_PHYSICAL_GPU_TYPE";
    }
}


static inline const char* string_XGL_STENCIL_OP(XGL_STENCIL_OP input_value)
{
    switch ((XGL_STENCIL_OP)input_value)
    {
        case XGL_STENCIL_OP_DEC_CLAMP:
            return "XGL_STENCIL_OP_DEC_CLAMP";
        case XGL_STENCIL_OP_DEC_WRAP:
            return "XGL_STENCIL_OP_DEC_WRAP";
        case XGL_STENCIL_OP_INC_CLAMP:
            return "XGL_STENCIL_OP_INC_CLAMP";
        case XGL_STENCIL_OP_INC_WRAP:
            return "XGL_STENCIL_OP_INC_WRAP";
        case XGL_STENCIL_OP_INVERT:
            return "XGL_STENCIL_OP_INVERT";
        case XGL_STENCIL_OP_KEEP:
            return "XGL_STENCIL_OP_KEEP";
        case XGL_STENCIL_OP_REPLACE:
            return "XGL_STENCIL_OP_REPLACE";
        case XGL_STENCIL_OP_ZERO:
            return "XGL_STENCIL_OP_ZERO";
        default:
            return "Unhandled XGL_STENCIL_OP";
    }
}


static inline const char* string_XGL_BORDER_COLOR_TYPE(XGL_BORDER_COLOR_TYPE input_value)
{
    switch ((XGL_BORDER_COLOR_TYPE)input_value)
    {
        case XGL_BORDER_COLOR_OPAQUE_BLACK:
            return "XGL_BORDER_COLOR_OPAQUE_BLACK";
        case XGL_BORDER_COLOR_OPAQUE_WHITE:
            return "XGL_BORDER_COLOR_OPAQUE_WHITE";
        case XGL_BORDER_COLOR_TRANSPARENT_BLACK:
            return "XGL_BORDER_COLOR_TRANSPARENT_BLACK";
        default:
            return "Unhandled XGL_BORDER_COLOR_TYPE";
    }
}


static inline const char* string_XGL_GPU_COMPATIBILITY_FLAGS(XGL_GPU_COMPATIBILITY_FLAGS input_value)
{
    switch ((XGL_GPU_COMPATIBILITY_FLAGS)input_value)
    {
        case XGL_GPU_COMPAT_ASIC_FEATURES_BIT:
            return "XGL_GPU_COMPAT_ASIC_FEATURES_BIT";
        case XGL_GPU_COMPAT_IQ_MATCH_BIT:
            return "XGL_GPU_COMPAT_IQ_MATCH_BIT";
        case XGL_GPU_COMPAT_PEER_TRANSFER_BIT:
            return "XGL_GPU_COMPAT_PEER_TRANSFER_BIT";
        case XGL_GPU_COMPAT_SHARED_GPU0_DISPLAY_BIT:
            return "XGL_GPU_COMPAT_SHARED_GPU0_DISPLAY_BIT";
        case XGL_GPU_COMPAT_SHARED_GPU1_DISPLAY_BIT:
            return "XGL_GPU_COMPAT_SHARED_GPU1_DISPLAY_BIT";
        case XGL_GPU_COMPAT_SHARED_MEMORY_BIT:
            return "XGL_GPU_COMPAT_SHARED_MEMORY_BIT";
        case XGL_GPU_COMPAT_SHARED_SYNC_BIT:
            return "XGL_GPU_COMPAT_SHARED_SYNC_BIT";
        default:
            return "Unhandled XGL_GPU_COMPATIBILITY_FLAGS";
    }
}


static inline const char* string_XGL_PIPELINE_CREATE_FLAGS(XGL_PIPELINE_CREATE_FLAGS input_value)
{
    switch ((XGL_PIPELINE_CREATE_FLAGS)input_value)
    {
        case XGL_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT:
            return "XGL_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT";
        default:
            return "Unhandled XGL_PIPELINE_CREATE_FLAGS";
    }
}


static inline const char* string_XGL_DEVICE_CREATE_FLAGS(XGL_DEVICE_CREATE_FLAGS input_value)
{
    switch ((XGL_DEVICE_CREATE_FLAGS)input_value)
    {
        case XGL_DEVICE_CREATE_MGPU_IQ_MATCH_BIT:
            return "XGL_DEVICE_CREATE_MGPU_IQ_MATCH_BIT";
        case XGL_DEVICE_CREATE_VALIDATION_BIT:
            return "XGL_DEVICE_CREATE_VALIDATION_BIT";
        default:
            return "Unhandled XGL_DEVICE_CREATE_FLAGS";
    }
}


static inline const char* string_XGL_OBJECT_INFO_TYPE(XGL_OBJECT_INFO_TYPE input_value)
{
    switch ((XGL_OBJECT_INFO_TYPE)input_value)
    {
        case XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS:
            return "XGL_INFO_TYPE_BUFFER_MEMORY_REQUIREMENTS";
        case XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS:
            return "XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS";
        case XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT:
            return "XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT";
        case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
            return "XGL_INFO_TYPE_MEMORY_REQUIREMENTS";
        default:
            return "Unhandled XGL_OBJECT_INFO_TYPE";
    }
}


static inline const char* string_XGL_TEX_MIPMAP_MODE(XGL_TEX_MIPMAP_MODE input_value)
{
    switch ((XGL_TEX_MIPMAP_MODE)input_value)
    {
        case XGL_TEX_MIPMAP_BASE:
            return "XGL_TEX_MIPMAP_BASE";
        case XGL_TEX_MIPMAP_LINEAR:
            return "XGL_TEX_MIPMAP_LINEAR";
        case XGL_TEX_MIPMAP_NEAREST:
            return "XGL_TEX_MIPMAP_NEAREST";
        default:
            return "Unhandled XGL_TEX_MIPMAP_MODE";
    }
}


static inline const char* string_XGL_STRUCTURE_TYPE(XGL_STRUCTURE_TYPE input_value)
{
    switch ((XGL_STRUCTURE_TYPE)input_value)
    {
        case XGL_STRUCTURE_TYPE_APPLICATION_INFO:
            return "XGL_STRUCTURE_TYPE_APPLICATION_INFO";
        case XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
            return "XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER";
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO:
            return "XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO";
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO:
            return "XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO";
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
            return "XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO";
        case XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_EVENT_WAIT_INFO:
            return "XGL_STRUCTURE_TYPE_EVENT_WAIT_INFO";
        case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
            return "XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER";
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO:
            return "XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO";
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
            return "XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO";
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
            return "XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO";
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO:
            return "XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO";
        case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:
            return "XGL_STRUCTURE_TYPE_MEMORY_BARRIER";
        case XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO:
            return "XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO";
        case XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO:
            return "XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO";
        case XGL_STRUCTURE_TYPE_PIPELINE_BARRIER:
            return "XGL_STRUCTURE_TYPE_PIPELINE_BARRIER";
        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_OPEN_INFO:
            return "XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_OPEN_INFO";
        case XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO:
            return "XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO";
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
            return "XGL_STRUCTURE_TYPE_UPDATE_AS_COPY";
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            return "XGL_STRUCTURE_TYPE_UPDATE_BUFFERS";
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            return "XGL_STRUCTURE_TYPE_UPDATE_IMAGES";
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            return "XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS";
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            return "XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES";
        default:
            return "Unhandled XGL_STRUCTURE_TYPE";
    }
}


static inline const char* string_XGL_TIMESTAMP_TYPE(XGL_TIMESTAMP_TYPE input_value)
{
    switch ((XGL_TIMESTAMP_TYPE)input_value)
    {
        case XGL_TIMESTAMP_BOTTOM:
            return "XGL_TIMESTAMP_BOTTOM";
        case XGL_TIMESTAMP_TOP:
            return "XGL_TIMESTAMP_TOP";
        default:
            return "Unhandled XGL_TIMESTAMP_TYPE";
    }
}


static inline const char* string_XGL_PROVOKING_VERTEX_CONVENTION(XGL_PROVOKING_VERTEX_CONVENTION input_value)
{
    switch ((XGL_PROVOKING_VERTEX_CONVENTION)input_value)
    {
        case XGL_PROVOKING_VERTEX_FIRST:
            return "XGL_PROVOKING_VERTEX_FIRST";
        case XGL_PROVOKING_VERTEX_LAST:
            return "XGL_PROVOKING_VERTEX_LAST";
        default:
            return "Unhandled XGL_PROVOKING_VERTEX_CONVENTION";
    }
}


static inline const char* string_XGL_PHYSICAL_GPU_INFO_TYPE(XGL_PHYSICAL_GPU_INFO_TYPE input_value)
{
    switch ((XGL_PHYSICAL_GPU_INFO_TYPE)input_value)
    {
        case XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES:
            return "XGL_INFO_TYPE_PHYSICAL_GPU_MEMORY_PROPERTIES";
        case XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE:
            return "XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE";
        case XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES:
            return "XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES";
        case XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES:
            return "XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES";
        default:
            return "Unhandled XGL_PHYSICAL_GPU_INFO_TYPE";
    }
}


static inline const char* string_XGL_TEX_FILTER(XGL_TEX_FILTER input_value)
{
    switch ((XGL_TEX_FILTER)input_value)
    {
        case XGL_TEX_FILTER_LINEAR:
            return "XGL_TEX_FILTER_LINEAR";
        case XGL_TEX_FILTER_NEAREST:
            return "XGL_TEX_FILTER_NEAREST";
        default:
            return "Unhandled XGL_TEX_FILTER";
    }
}


static inline const char* string_XGL_BLEND(XGL_BLEND input_value)
{
    switch ((XGL_BLEND)input_value)
    {
        case XGL_BLEND_CONSTANT_ALPHA:
            return "XGL_BLEND_CONSTANT_ALPHA";
        case XGL_BLEND_CONSTANT_COLOR:
            return "XGL_BLEND_CONSTANT_COLOR";
        case XGL_BLEND_DEST_ALPHA:
            return "XGL_BLEND_DEST_ALPHA";
        case XGL_BLEND_DEST_COLOR:
            return "XGL_BLEND_DEST_COLOR";
        case XGL_BLEND_ONE:
            return "XGL_BLEND_ONE";
        case XGL_BLEND_ONE_MINUS_CONSTANT_ALPHA:
            return "XGL_BLEND_ONE_MINUS_CONSTANT_ALPHA";
        case XGL_BLEND_ONE_MINUS_CONSTANT_COLOR:
            return "XGL_BLEND_ONE_MINUS_CONSTANT_COLOR";
        case XGL_BLEND_ONE_MINUS_DEST_ALPHA:
            return "XGL_BLEND_ONE_MINUS_DEST_ALPHA";
        case XGL_BLEND_ONE_MINUS_DEST_COLOR:
            return "XGL_BLEND_ONE_MINUS_DEST_COLOR";
        case XGL_BLEND_ONE_MINUS_SRC1_ALPHA:
            return "XGL_BLEND_ONE_MINUS_SRC1_ALPHA";
        case XGL_BLEND_ONE_MINUS_SRC1_COLOR:
            return "XGL_BLEND_ONE_MINUS_SRC1_COLOR";
        case XGL_BLEND_ONE_MINUS_SRC_ALPHA:
            return "XGL_BLEND_ONE_MINUS_SRC_ALPHA";
        case XGL_BLEND_ONE_MINUS_SRC_COLOR:
            return "XGL_BLEND_ONE_MINUS_SRC_COLOR";
        case XGL_BLEND_SRC1_ALPHA:
            return "XGL_BLEND_SRC1_ALPHA";
        case XGL_BLEND_SRC1_COLOR:
            return "XGL_BLEND_SRC1_COLOR";
        case XGL_BLEND_SRC_ALPHA:
            return "XGL_BLEND_SRC_ALPHA";
        case XGL_BLEND_SRC_ALPHA_SATURATE:
            return "XGL_BLEND_SRC_ALPHA_SATURATE";
        case XGL_BLEND_SRC_COLOR:
            return "XGL_BLEND_SRC_COLOR";
        case XGL_BLEND_ZERO:
            return "XGL_BLEND_ZERO";
        default:
            return "Unhandled XGL_BLEND";
    }
}


static inline const char* string_XGL_DESCRIPTOR_REGION_USAGE(XGL_DESCRIPTOR_REGION_USAGE input_value)
{
    switch ((XGL_DESCRIPTOR_REGION_USAGE)input_value)
    {
        case XGL_DESCRIPTOR_REGION_USAGE_DYNAMIC:
            return "XGL_DESCRIPTOR_REGION_USAGE_DYNAMIC";
        case XGL_DESCRIPTOR_REGION_USAGE_ONE_SHOT:
            return "XGL_DESCRIPTOR_REGION_USAGE_ONE_SHOT";
        default:
            return "Unhandled XGL_DESCRIPTOR_REGION_USAGE";
    }
}


static inline const char* string_XGL_CULL_MODE(XGL_CULL_MODE input_value)
{
    switch ((XGL_CULL_MODE)input_value)
    {
        case XGL_CULL_BACK:
            return "XGL_CULL_BACK";
        case XGL_CULL_FRONT:
            return "XGL_CULL_FRONT";
        case XGL_CULL_FRONT_AND_BACK:
            return "XGL_CULL_FRONT_AND_BACK";
        case XGL_CULL_NONE:
            return "XGL_CULL_NONE";
        default:
            return "Unhandled XGL_CULL_MODE";
    }
}


static inline const char* string_XGL_BUFFER_VIEW_TYPE(XGL_BUFFER_VIEW_TYPE input_value)
{
    switch ((XGL_BUFFER_VIEW_TYPE)input_value)
    {
        case XGL_BUFFER_VIEW_RAW:
            return "XGL_BUFFER_VIEW_RAW";
        case XGL_BUFFER_VIEW_STRUCTURED:
            return "XGL_BUFFER_VIEW_STRUCTURED";
        case XGL_BUFFER_VIEW_TYPED:
            return "XGL_BUFFER_VIEW_TYPED";
        default:
            return "Unhandled XGL_BUFFER_VIEW_TYPE";
    }
}


static inline const char* string_XGL_RESULT(XGL_RESULT input_value)
{
    switch ((XGL_RESULT)input_value)
    {
        case XGL_ERROR_BAD_PIPELINE_DATA:
            return "XGL_ERROR_BAD_PIPELINE_DATA";
        case XGL_ERROR_BAD_SHADER_CODE:
            return "XGL_ERROR_BAD_SHADER_CODE";
        case XGL_ERROR_BUILDING_COMMAND_BUFFER:
            return "XGL_ERROR_BUILDING_COMMAND_BUFFER";
        case XGL_ERROR_DEVICE_ALREADY_CREATED:
            return "XGL_ERROR_DEVICE_ALREADY_CREATED";
        case XGL_ERROR_DEVICE_LOST:
            return "XGL_ERROR_DEVICE_LOST";
        case XGL_ERROR_INCOMPATIBLE_DEVICE:
            return "XGL_ERROR_INCOMPATIBLE_DEVICE";
        case XGL_ERROR_INCOMPATIBLE_DRIVER:
            return "XGL_ERROR_INCOMPATIBLE_DRIVER";
        case XGL_ERROR_INCOMPATIBLE_QUEUE:
            return "XGL_ERROR_INCOMPATIBLE_QUEUE";
        case XGL_ERROR_INCOMPLETE_COMMAND_BUFFER:
            return "XGL_ERROR_INCOMPLETE_COMMAND_BUFFER";
        case XGL_ERROR_INITIALIZATION_FAILED:
            return "XGL_ERROR_INITIALIZATION_FAILED";
        case XGL_ERROR_INVALID_ALIGNMENT:
            return "XGL_ERROR_INVALID_ALIGNMENT";
        case XGL_ERROR_INVALID_DESCRIPTOR_SET_DATA:
            return "XGL_ERROR_INVALID_DESCRIPTOR_SET_DATA";
        case XGL_ERROR_INVALID_EXTENSION:
            return "XGL_ERROR_INVALID_EXTENSION";
        case XGL_ERROR_INVALID_FLAGS:
            return "XGL_ERROR_INVALID_FLAGS";
        case XGL_ERROR_INVALID_FORMAT:
            return "XGL_ERROR_INVALID_FORMAT";
        case XGL_ERROR_INVALID_HANDLE:
            return "XGL_ERROR_INVALID_HANDLE";
        case XGL_ERROR_INVALID_IMAGE:
            return "XGL_ERROR_INVALID_IMAGE";
        case XGL_ERROR_INVALID_MEMORY_SIZE:
            return "XGL_ERROR_INVALID_MEMORY_SIZE";
        case XGL_ERROR_INVALID_OBJECT_TYPE:
            return "XGL_ERROR_INVALID_OBJECT_TYPE";
        case XGL_ERROR_INVALID_ORDINAL:
            return "XGL_ERROR_INVALID_ORDINAL";
        case XGL_ERROR_INVALID_POINTER:
            return "XGL_ERROR_INVALID_POINTER";
        case XGL_ERROR_INVALID_QUEUE_TYPE:
            return "XGL_ERROR_INVALID_QUEUE_TYPE";
        case XGL_ERROR_INVALID_VALUE:
            return "XGL_ERROR_INVALID_VALUE";
        case XGL_ERROR_MEMORY_MAP_FAILED:
            return "XGL_ERROR_MEMORY_MAP_FAILED";
        case XGL_ERROR_MEMORY_NOT_BOUND:
            return "XGL_ERROR_MEMORY_NOT_BOUND";
        case XGL_ERROR_MEMORY_UNMAP_FAILED:
            return "XGL_ERROR_MEMORY_UNMAP_FAILED";
        case XGL_ERROR_NOT_MAPPABLE:
            return "XGL_ERROR_NOT_MAPPABLE";
        case XGL_ERROR_NOT_SHAREABLE:
            return "XGL_ERROR_NOT_SHAREABLE";
        case XGL_ERROR_OUT_OF_GPU_MEMORY:
            return "XGL_ERROR_OUT_OF_GPU_MEMORY";
        case XGL_ERROR_OUT_OF_MEMORY:
            return "XGL_ERROR_OUT_OF_MEMORY";
        case XGL_ERROR_TOO_MANY_MEMORY_REFERENCES:
            return "XGL_ERROR_TOO_MANY_MEMORY_REFERENCES";
        case XGL_ERROR_UNAVAILABLE:
            return "XGL_ERROR_UNAVAILABLE";
        case XGL_ERROR_UNKNOWN:
            return "XGL_ERROR_UNKNOWN";
        case XGL_ERROR_UNSUPPORTED_SHADER_IL_VERSION:
            return "XGL_ERROR_UNSUPPORTED_SHADER_IL_VERSION";
        case XGL_EVENT_RESET:
            return "XGL_EVENT_RESET";
        case XGL_EVENT_SET:
            return "XGL_EVENT_SET";
        case XGL_NOT_READY:
            return "XGL_NOT_READY";
        case XGL_SUCCESS:
            return "XGL_SUCCESS";
        case XGL_TIMEOUT:
            return "XGL_TIMEOUT";
        case XGL_UNSUPPORTED:
            return "XGL_UNSUPPORTED";
        default:
            return "Unhandled XGL_RESULT";
    }
}


static inline const char* string_XGL_IMAGE_CREATE_FLAGS(XGL_IMAGE_CREATE_FLAGS input_value)
{
    switch ((XGL_IMAGE_CREATE_FLAGS)input_value)
    {
        case XGL_IMAGE_CREATE_CLONEABLE_BIT:
            return "XGL_IMAGE_CREATE_CLONEABLE_BIT";
        case XGL_IMAGE_CREATE_INVARIANT_DATA_BIT:
            return "XGL_IMAGE_CREATE_INVARIANT_DATA_BIT";
        case XGL_IMAGE_CREATE_MUTABLE_FORMAT_BIT:
            return "XGL_IMAGE_CREATE_MUTABLE_FORMAT_BIT";
        case XGL_IMAGE_CREATE_SHAREABLE_BIT:
            return "XGL_IMAGE_CREATE_SHAREABLE_BIT";
        case XGL_IMAGE_CREATE_SPARSE_BIT:
            return "XGL_IMAGE_CREATE_SPARSE_BIT";
        default:
            return "Unhandled XGL_IMAGE_CREATE_FLAGS";
    }
}


static inline const char* string_XGL_TEX_ADDRESS(XGL_TEX_ADDRESS input_value)
{
    switch ((XGL_TEX_ADDRESS)input_value)
    {
        case XGL_TEX_ADDRESS_CLAMP:
            return "XGL_TEX_ADDRESS_CLAMP";
        case XGL_TEX_ADDRESS_CLAMP_BORDER:
            return "XGL_TEX_ADDRESS_CLAMP_BORDER";
        case XGL_TEX_ADDRESS_MIRROR:
            return "XGL_TEX_ADDRESS_MIRROR";
        case XGL_TEX_ADDRESS_MIRROR_ONCE:
            return "XGL_TEX_ADDRESS_MIRROR_ONCE";
        case XGL_TEX_ADDRESS_WRAP:
            return "XGL_TEX_ADDRESS_WRAP";
        default:
            return "Unhandled XGL_TEX_ADDRESS";
    }
}


static inline const char* string_XGL_QUEUE_FLAGS(XGL_QUEUE_FLAGS input_value)
{
    switch ((XGL_QUEUE_FLAGS)input_value)
    {
        case XGL_QUEUE_COMPUTE_BIT:
            return "XGL_QUEUE_COMPUTE_BIT";
        case XGL_QUEUE_DMA_BIT:
            return "XGL_QUEUE_DMA_BIT";
        case XGL_QUEUE_EXTENDED_BIT:
            return "XGL_QUEUE_EXTENDED_BIT";
        case XGL_QUEUE_GRAPHICS_BIT:
            return "XGL_QUEUE_GRAPHICS_BIT";
        default:
            return "Unhandled XGL_QUEUE_FLAGS";
    }
}


static inline const char* string_XGL_QUERY_TYPE(XGL_QUERY_TYPE input_value)
{
    switch ((XGL_QUERY_TYPE)input_value)
    {
        case XGL_QUERY_OCCLUSION:
            return "XGL_QUERY_OCCLUSION";
        case XGL_QUERY_PIPELINE_STATISTICS:
            return "XGL_QUERY_PIPELINE_STATISTICS";
        default:
            return "Unhandled XGL_QUERY_TYPE";
    }
}


static inline const char* string_XGL_IMAGE_LAYOUT(XGL_IMAGE_LAYOUT input_value)
{
    switch ((XGL_IMAGE_LAYOUT)input_value)
    {
        case XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL:
            return "XGL_IMAGE_LAYOUT_CLEAR_OPTIMAL";
        case XGL_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return "XGL_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL";
        case XGL_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return "XGL_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
        case XGL_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return "XGL_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL";
        case XGL_IMAGE_LAYOUT_GENERAL:
            return "XGL_IMAGE_LAYOUT_GENERAL";
        case XGL_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return "XGL_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";
        case XGL_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL:
            return "XGL_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL";
        case XGL_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL:
            return "XGL_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL";
        default:
            return "Unhandled XGL_IMAGE_LAYOUT";
    }
}


static inline const char* string_XGL_QUERY_CONTROL_FLAGS(XGL_QUERY_CONTROL_FLAGS input_value)
{
    switch ((XGL_QUERY_CONTROL_FLAGS)input_value)
    {
        case XGL_QUERY_IMPRECISE_DATA_BIT:
            return "XGL_QUERY_IMPRECISE_DATA_BIT";
        default:
            return "Unhandled XGL_QUERY_CONTROL_FLAGS";
    }
}


static inline const char* string_XGL_FORMAT_INFO_TYPE(XGL_FORMAT_INFO_TYPE input_value)
{
    switch ((XGL_FORMAT_INFO_TYPE)input_value)
    {
        case XGL_INFO_TYPE_FORMAT_PROPERTIES:
            return "XGL_INFO_TYPE_FORMAT_PROPERTIES";
        default:
            return "Unhandled XGL_FORMAT_INFO_TYPE";
    }
}


static inline const char* string_XGL_PIPELINE_SHADER_STAGE(XGL_PIPELINE_SHADER_STAGE input_value)
{
    switch ((XGL_PIPELINE_SHADER_STAGE)input_value)
    {
        case XGL_SHADER_STAGE_COMPUTE:
            return "XGL_SHADER_STAGE_COMPUTE";
        case XGL_SHADER_STAGE_FRAGMENT:
            return "XGL_SHADER_STAGE_FRAGMENT";
        case XGL_SHADER_STAGE_GEOMETRY:
            return "XGL_SHADER_STAGE_GEOMETRY";
        case XGL_SHADER_STAGE_TESS_CONTROL:
            return "XGL_SHADER_STAGE_TESS_CONTROL";
        case XGL_SHADER_STAGE_TESS_EVALUATION:
            return "XGL_SHADER_STAGE_TESS_EVALUATION";
        case XGL_SHADER_STAGE_VERTEX:
            return "XGL_SHADER_STAGE_VERTEX";
        default:
            return "Unhandled XGL_PIPELINE_SHADER_STAGE";
    }
}


static inline const char* string_XGL_COORDINATE_ORIGIN(XGL_COORDINATE_ORIGIN input_value)
{
    switch ((XGL_COORDINATE_ORIGIN)input_value)
    {
        case XGL_COORDINATE_ORIGIN_LOWER_LEFT:
            return "XGL_COORDINATE_ORIGIN_LOWER_LEFT";
        case XGL_COORDINATE_ORIGIN_UPPER_LEFT:
            return "XGL_COORDINATE_ORIGIN_UPPER_LEFT";
        default:
            return "Unhandled XGL_COORDINATE_ORIGIN";
    }
}


static inline const char* string_XGL_VALIDATION_LEVEL(XGL_VALIDATION_LEVEL input_value)
{
    switch ((XGL_VALIDATION_LEVEL)input_value)
    {
        case XGL_VALIDATION_LEVEL_0:
            return "XGL_VALIDATION_LEVEL_0";
        case XGL_VALIDATION_LEVEL_1:
            return "XGL_VALIDATION_LEVEL_1";
        case XGL_VALIDATION_LEVEL_2:
            return "XGL_VALIDATION_LEVEL_2";
        case XGL_VALIDATION_LEVEL_3:
            return "XGL_VALIDATION_LEVEL_3";
        case XGL_VALIDATION_LEVEL_4:
            return "XGL_VALIDATION_LEVEL_4";
        default:
            return "Unhandled XGL_VALIDATION_LEVEL";
    }
}


static inline const char* string_XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS(XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS input_value)
{
    switch ((XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS)input_value)
    {
        case XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT:
            return "XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT";
        case XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT:
            return "XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT";
        default:
            return "Unhandled XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS";
    }
}


static inline const char* string_XGL_WAIT_EVENT(XGL_WAIT_EVENT input_value)
{
    switch ((XGL_WAIT_EVENT)input_value)
    {
        case XGL_WAIT_EVENT_BEFORE_RASTERIZATION:
            return "XGL_WAIT_EVENT_BEFORE_RASTERIZATION";
        case XGL_WAIT_EVENT_TOP_OF_PIPE:
            return "XGL_WAIT_EVENT_TOP_OF_PIPE";
        default:
            return "Unhandled XGL_WAIT_EVENT";
    }
}


static inline const char* string_XGL_BUFFER_CREATE_FLAGS(XGL_BUFFER_CREATE_FLAGS input_value)
{
    switch ((XGL_BUFFER_CREATE_FLAGS)input_value)
    {
        case XGL_BUFFER_CREATE_SHAREABLE_BIT:
            return "XGL_BUFFER_CREATE_SHAREABLE_BIT";
        case XGL_BUFFER_CREATE_SPARSE_BIT:
            return "XGL_BUFFER_CREATE_SPARSE_BIT";
        default:
            return "Unhandled XGL_BUFFER_CREATE_FLAGS";
    }
}


static inline const char* string_XGL_SET_EVENT(XGL_SET_EVENT input_value)
{
    switch ((XGL_SET_EVENT)input_value)
    {
        case XGL_SET_EVENT_COMPUTE_PIPELINE_COMPLETE:
            return "XGL_SET_EVENT_COMPUTE_PIPELINE_COMPLETE";
        case XGL_SET_EVENT_FRAGMENT_PROCESSING_COMPLETE:
            return "XGL_SET_EVENT_FRAGMENT_PROCESSING_COMPLETE";
        case XGL_SET_EVENT_GPU_COMMANDS_COMPLETE:
            return "XGL_SET_EVENT_GPU_COMMANDS_COMPLETE";
        case XGL_SET_EVENT_GRAPHICS_PIPELINE_COMPLETE:
            return "XGL_SET_EVENT_GRAPHICS_PIPELINE_COMPLETE";
        case XGL_SET_EVENT_TOP_OF_PIPE:
            return "XGL_SET_EVENT_TOP_OF_PIPE";
        case XGL_SET_EVENT_TRANSFER_COMPLETE:
            return "XGL_SET_EVENT_TRANSFER_COMPLETE";
        case XGL_SET_EVENT_VERTEX_PROCESSING_COMPLETE:
            return "XGL_SET_EVENT_VERTEX_PROCESSING_COMPLETE";
        default:
            return "Unhandled XGL_SET_EVENT";
    }
}


static inline const char* string_XGL_PIPELINE_BIND_POINT(XGL_PIPELINE_BIND_POINT input_value)
{
    switch ((XGL_PIPELINE_BIND_POINT)input_value)
    {
        case XGL_PIPELINE_BIND_POINT_COMPUTE:
            return "XGL_PIPELINE_BIND_POINT_COMPUTE";
        case XGL_PIPELINE_BIND_POINT_GRAPHICS:
            return "XGL_PIPELINE_BIND_POINT_GRAPHICS";
        default:
            return "Unhandled XGL_PIPELINE_BIND_POINT";
    }
}


static inline const char* string_XGL_COMPARE_FUNC(XGL_COMPARE_FUNC input_value)
{
    switch ((XGL_COMPARE_FUNC)input_value)
    {
        case XGL_COMPARE_ALWAYS:
            return "XGL_COMPARE_ALWAYS";
        case XGL_COMPARE_EQUAL:
            return "XGL_COMPARE_EQUAL";
        case XGL_COMPARE_GREATER:
            return "XGL_COMPARE_GREATER";
        case XGL_COMPARE_GREATER_EQUAL:
            return "XGL_COMPARE_GREATER_EQUAL";
        case XGL_COMPARE_LESS:
            return "XGL_COMPARE_LESS";
        case XGL_COMPARE_LESS_EQUAL:
            return "XGL_COMPARE_LESS_EQUAL";
        case XGL_COMPARE_NEVER:
            return "XGL_COMPARE_NEVER";
        case XGL_COMPARE_NOT_EQUAL:
            return "XGL_COMPARE_NOT_EQUAL";
        default:
            return "Unhandled XGL_COMPARE_FUNC";
    }
}


static inline const char* string_XGL_IMAGE_FORMAT_CLASS(XGL_IMAGE_FORMAT_CLASS input_value)
{
    switch ((XGL_IMAGE_FORMAT_CLASS)input_value)
    {
        case XGL_IMAGE_FORMAT_CLASS_128_BITS:
            return "XGL_IMAGE_FORMAT_CLASS_128_BITS";
        case XGL_IMAGE_FORMAT_CLASS_128_BIT_BLOCK:
            return "XGL_IMAGE_FORMAT_CLASS_128_BIT_BLOCK";
        case XGL_IMAGE_FORMAT_CLASS_16_BITS:
            return "XGL_IMAGE_FORMAT_CLASS_16_BITS";
        case XGL_IMAGE_FORMAT_CLASS_24_BITS:
            return "XGL_IMAGE_FORMAT_CLASS_24_BITS";
        case XGL_IMAGE_FORMAT_CLASS_32_BITS:
            return "XGL_IMAGE_FORMAT_CLASS_32_BITS";
        case XGL_IMAGE_FORMAT_CLASS_48_BITS:
            return "XGL_IMAGE_FORMAT_CLASS_48_BITS";
        case XGL_IMAGE_FORMAT_CLASS_64_BITS:
            return "XGL_IMAGE_FORMAT_CLASS_64_BITS";
        case XGL_IMAGE_FORMAT_CLASS_64_BIT_BLOCK:
            return "XGL_IMAGE_FORMAT_CLASS_64_BIT_BLOCK";
        case XGL_IMAGE_FORMAT_CLASS_8_BITS:
            return "XGL_IMAGE_FORMAT_CLASS_8_BITS";
        case XGL_IMAGE_FORMAT_CLASS_96_BITS:
            return "XGL_IMAGE_FORMAT_CLASS_96_BITS";
        case XGL_IMAGE_FORMAT_CLASS_D16:
            return "XGL_IMAGE_FORMAT_CLASS_D16";
        case XGL_IMAGE_FORMAT_CLASS_D16S8:
            return "XGL_IMAGE_FORMAT_CLASS_D16S8";
        case XGL_IMAGE_FORMAT_CLASS_D24:
            return "XGL_IMAGE_FORMAT_CLASS_D24";
        case XGL_IMAGE_FORMAT_CLASS_D24S8:
            return "XGL_IMAGE_FORMAT_CLASS_D24S8";
        case XGL_IMAGE_FORMAT_CLASS_D32:
            return "XGL_IMAGE_FORMAT_CLASS_D32";
        case XGL_IMAGE_FORMAT_CLASS_D32S8:
            return "XGL_IMAGE_FORMAT_CLASS_D32S8";
        case XGL_IMAGE_FORMAT_CLASS_LINEAR:
            return "XGL_IMAGE_FORMAT_CLASS_LINEAR";
        case XGL_IMAGE_FORMAT_CLASS_S8:
            return "XGL_IMAGE_FORMAT_CLASS_S8";
        default:
            return "Unhandled XGL_IMAGE_FORMAT_CLASS";
    }
}


static inline const char* string_XGL_FILL_MODE(XGL_FILL_MODE input_value)
{
    switch ((XGL_FILL_MODE)input_value)
    {
        case XGL_FILL_POINTS:
            return "XGL_FILL_POINTS";
        case XGL_FILL_SOLID:
            return "XGL_FILL_SOLID";
        case XGL_FILL_WIREFRAME:
            return "XGL_FILL_WIREFRAME";
        default:
            return "Unhandled XGL_FILL_MODE";
    }
}


static inline const char* string_XGL_MEMORY_PROPERTY_FLAGS(XGL_MEMORY_PROPERTY_FLAGS input_value)
{
    switch ((XGL_MEMORY_PROPERTY_FLAGS)input_value)
    {
        case XGL_MEMORY_PROPERTY_CPU_GPU_COHERENT_BIT:
            return "XGL_MEMORY_PROPERTY_CPU_GPU_COHERENT_BIT";
        case XGL_MEMORY_PROPERTY_CPU_UNCACHED_BIT:
            return "XGL_MEMORY_PROPERTY_CPU_UNCACHED_BIT";
        case XGL_MEMORY_PROPERTY_CPU_VISIBLE_BIT:
            return "XGL_MEMORY_PROPERTY_CPU_VISIBLE_BIT";
        case XGL_MEMORY_PROPERTY_CPU_WRITE_COMBINED_BIT:
            return "XGL_MEMORY_PROPERTY_CPU_WRITE_COMBINED_BIT";
        case XGL_MEMORY_PROPERTY_GPU_ONLY:
            return "XGL_MEMORY_PROPERTY_GPU_ONLY";
        case XGL_MEMORY_PROPERTY_PREFER_CPU_LOCAL:
            return "XGL_MEMORY_PROPERTY_PREFER_CPU_LOCAL";
        case XGL_MEMORY_PROPERTY_SHAREABLE_BIT:
            return "XGL_MEMORY_PROPERTY_SHAREABLE_BIT";
        default:
            return "Unhandled XGL_MEMORY_PROPERTY_FLAGS";
    }
}


static inline const char* string_XGL_FORMAT(XGL_FORMAT input_value)
{
    switch ((XGL_FORMAT)input_value)
    {
        case XGL_FMT_ASTC_10x10_SRGB:
            return "XGL_FMT_ASTC_10x10_SRGB";
        case XGL_FMT_ASTC_10x10_UNORM:
            return "XGL_FMT_ASTC_10x10_UNORM";
        case XGL_FMT_ASTC_10x5_SRGB:
            return "XGL_FMT_ASTC_10x5_SRGB";
        case XGL_FMT_ASTC_10x5_UNORM:
            return "XGL_FMT_ASTC_10x5_UNORM";
        case XGL_FMT_ASTC_10x6_SRGB:
            return "XGL_FMT_ASTC_10x6_SRGB";
        case XGL_FMT_ASTC_10x6_UNORM:
            return "XGL_FMT_ASTC_10x6_UNORM";
        case XGL_FMT_ASTC_10x8_SRGB:
            return "XGL_FMT_ASTC_10x8_SRGB";
        case XGL_FMT_ASTC_10x8_UNORM:
            return "XGL_FMT_ASTC_10x8_UNORM";
        case XGL_FMT_ASTC_12x10_SRGB:
            return "XGL_FMT_ASTC_12x10_SRGB";
        case XGL_FMT_ASTC_12x10_UNORM:
            return "XGL_FMT_ASTC_12x10_UNORM";
        case XGL_FMT_ASTC_12x12_SRGB:
            return "XGL_FMT_ASTC_12x12_SRGB";
        case XGL_FMT_ASTC_12x12_UNORM:
            return "XGL_FMT_ASTC_12x12_UNORM";
        case XGL_FMT_ASTC_4x4_SRGB:
            return "XGL_FMT_ASTC_4x4_SRGB";
        case XGL_FMT_ASTC_4x4_UNORM:
            return "XGL_FMT_ASTC_4x4_UNORM";
        case XGL_FMT_ASTC_4x5_SRGB:
            return "XGL_FMT_ASTC_4x5_SRGB";
        case XGL_FMT_ASTC_4x5_UNORM:
            return "XGL_FMT_ASTC_4x5_UNORM";
        case XGL_FMT_ASTC_5x5_SRGB:
            return "XGL_FMT_ASTC_5x5_SRGB";
        case XGL_FMT_ASTC_5x5_UNORM:
            return "XGL_FMT_ASTC_5x5_UNORM";
        case XGL_FMT_ASTC_6x5_SRGB:
            return "XGL_FMT_ASTC_6x5_SRGB";
        case XGL_FMT_ASTC_6x5_UNORM:
            return "XGL_FMT_ASTC_6x5_UNORM";
        case XGL_FMT_ASTC_6x6_SRGB:
            return "XGL_FMT_ASTC_6x6_SRGB";
        case XGL_FMT_ASTC_6x6_UNORM:
            return "XGL_FMT_ASTC_6x6_UNORM";
        case XGL_FMT_ASTC_8x5_SRGB:
            return "XGL_FMT_ASTC_8x5_SRGB";
        case XGL_FMT_ASTC_8x5_UNORM:
            return "XGL_FMT_ASTC_8x5_UNORM";
        case XGL_FMT_ASTC_8x6_SRGB:
            return "XGL_FMT_ASTC_8x6_SRGB";
        case XGL_FMT_ASTC_8x6_UNORM:
            return "XGL_FMT_ASTC_8x6_UNORM";
        case XGL_FMT_ASTC_8x8_SRGB:
            return "XGL_FMT_ASTC_8x8_SRGB";
        case XGL_FMT_ASTC_8x8_UNORM:
            return "XGL_FMT_ASTC_8x8_UNORM";
        case XGL_FMT_B10G10R10A2_SINT:
            return "XGL_FMT_B10G10R10A2_SINT";
        case XGL_FMT_B10G10R10A2_SNORM:
            return "XGL_FMT_B10G10R10A2_SNORM";
        case XGL_FMT_B10G10R10A2_SSCALED:
            return "XGL_FMT_B10G10R10A2_SSCALED";
        case XGL_FMT_B10G10R10A2_UINT:
            return "XGL_FMT_B10G10R10A2_UINT";
        case XGL_FMT_B10G10R10A2_UNORM:
            return "XGL_FMT_B10G10R10A2_UNORM";
        case XGL_FMT_B10G10R10A2_USCALED:
            return "XGL_FMT_B10G10R10A2_USCALED";
        case XGL_FMT_B5G6R5_UNORM:
            return "XGL_FMT_B5G6R5_UNORM";
        case XGL_FMT_B5G6R5_USCALED:
            return "XGL_FMT_B5G6R5_USCALED";
        case XGL_FMT_B8G8R8A8_SINT:
            return "XGL_FMT_B8G8R8A8_SINT";
        case XGL_FMT_B8G8R8A8_SNORM:
            return "XGL_FMT_B8G8R8A8_SNORM";
        case XGL_FMT_B8G8R8A8_SRGB:
            return "XGL_FMT_B8G8R8A8_SRGB";
        case XGL_FMT_B8G8R8A8_SSCALED:
            return "XGL_FMT_B8G8R8A8_SSCALED";
        case XGL_FMT_B8G8R8A8_UINT:
            return "XGL_FMT_B8G8R8A8_UINT";
        case XGL_FMT_B8G8R8A8_UNORM:
            return "XGL_FMT_B8G8R8A8_UNORM";
        case XGL_FMT_B8G8R8A8_USCALED:
            return "XGL_FMT_B8G8R8A8_USCALED";
        case XGL_FMT_B8G8R8_SINT:
            return "XGL_FMT_B8G8R8_SINT";
        case XGL_FMT_B8G8R8_SNORM:
            return "XGL_FMT_B8G8R8_SNORM";
        case XGL_FMT_B8G8R8_SRGB:
            return "XGL_FMT_B8G8R8_SRGB";
        case XGL_FMT_B8G8R8_SSCALED:
            return "XGL_FMT_B8G8R8_SSCALED";
        case XGL_FMT_B8G8R8_UINT:
            return "XGL_FMT_B8G8R8_UINT";
        case XGL_FMT_B8G8R8_UNORM:
            return "XGL_FMT_B8G8R8_UNORM";
        case XGL_FMT_B8G8R8_USCALED:
            return "XGL_FMT_B8G8R8_USCALED";
        case XGL_FMT_BC1_SRGB:
            return "XGL_FMT_BC1_SRGB";
        case XGL_FMT_BC1_UNORM:
            return "XGL_FMT_BC1_UNORM";
        case XGL_FMT_BC2_SRGB:
            return "XGL_FMT_BC2_SRGB";
        case XGL_FMT_BC2_UNORM:
            return "XGL_FMT_BC2_UNORM";
        case XGL_FMT_BC3_SRGB:
            return "XGL_FMT_BC3_SRGB";
        case XGL_FMT_BC3_UNORM:
            return "XGL_FMT_BC3_UNORM";
        case XGL_FMT_BC4_SNORM:
            return "XGL_FMT_BC4_SNORM";
        case XGL_FMT_BC4_UNORM:
            return "XGL_FMT_BC4_UNORM";
        case XGL_FMT_BC5_SNORM:
            return "XGL_FMT_BC5_SNORM";
        case XGL_FMT_BC5_UNORM:
            return "XGL_FMT_BC5_UNORM";
        case XGL_FMT_BC6H_SFLOAT:
            return "XGL_FMT_BC6H_SFLOAT";
        case XGL_FMT_BC6H_UFLOAT:
            return "XGL_FMT_BC6H_UFLOAT";
        case XGL_FMT_BC7_SRGB:
            return "XGL_FMT_BC7_SRGB";
        case XGL_FMT_BC7_UNORM:
            return "XGL_FMT_BC7_UNORM";
        case XGL_FMT_D16_UNORM:
            return "XGL_FMT_D16_UNORM";
        case XGL_FMT_D16_UNORM_S8_UINT:
            return "XGL_FMT_D16_UNORM_S8_UINT";
        case XGL_FMT_D24_UNORM:
            return "XGL_FMT_D24_UNORM";
        case XGL_FMT_D24_UNORM_S8_UINT:
            return "XGL_FMT_D24_UNORM_S8_UINT";
        case XGL_FMT_D32_SFLOAT:
            return "XGL_FMT_D32_SFLOAT";
        case XGL_FMT_D32_SFLOAT_S8_UINT:
            return "XGL_FMT_D32_SFLOAT_S8_UINT";
        case XGL_FMT_EAC_R11G11_SNORM:
            return "XGL_FMT_EAC_R11G11_SNORM";
        case XGL_FMT_EAC_R11G11_UNORM:
            return "XGL_FMT_EAC_R11G11_UNORM";
        case XGL_FMT_EAC_R11_SNORM:
            return "XGL_FMT_EAC_R11_SNORM";
        case XGL_FMT_EAC_R11_UNORM:
            return "XGL_FMT_EAC_R11_UNORM";
        case XGL_FMT_ETC2_R8G8B8A1_UNORM:
            return "XGL_FMT_ETC2_R8G8B8A1_UNORM";
        case XGL_FMT_ETC2_R8G8B8A8_UNORM:
            return "XGL_FMT_ETC2_R8G8B8A8_UNORM";
        case XGL_FMT_ETC2_R8G8B8_UNORM:
            return "XGL_FMT_ETC2_R8G8B8_UNORM";
        case XGL_FMT_R10G10B10A2_SINT:
            return "XGL_FMT_R10G10B10A2_SINT";
        case XGL_FMT_R10G10B10A2_SNORM:
            return "XGL_FMT_R10G10B10A2_SNORM";
        case XGL_FMT_R10G10B10A2_SSCALED:
            return "XGL_FMT_R10G10B10A2_SSCALED";
        case XGL_FMT_R10G10B10A2_UINT:
            return "XGL_FMT_R10G10B10A2_UINT";
        case XGL_FMT_R10G10B10A2_UNORM:
            return "XGL_FMT_R10G10B10A2_UNORM";
        case XGL_FMT_R10G10B10A2_USCALED:
            return "XGL_FMT_R10G10B10A2_USCALED";
        case XGL_FMT_R11G11B10_UFLOAT:
            return "XGL_FMT_R11G11B10_UFLOAT";
        case XGL_FMT_R16G16B16A16_SFLOAT:
            return "XGL_FMT_R16G16B16A16_SFLOAT";
        case XGL_FMT_R16G16B16A16_SINT:
            return "XGL_FMT_R16G16B16A16_SINT";
        case XGL_FMT_R16G16B16A16_SNORM:
            return "XGL_FMT_R16G16B16A16_SNORM";
        case XGL_FMT_R16G16B16A16_SSCALED:
            return "XGL_FMT_R16G16B16A16_SSCALED";
        case XGL_FMT_R16G16B16A16_UINT:
            return "XGL_FMT_R16G16B16A16_UINT";
        case XGL_FMT_R16G16B16A16_UNORM:
            return "XGL_FMT_R16G16B16A16_UNORM";
        case XGL_FMT_R16G16B16A16_USCALED:
            return "XGL_FMT_R16G16B16A16_USCALED";
        case XGL_FMT_R16G16B16_SFLOAT:
            return "XGL_FMT_R16G16B16_SFLOAT";
        case XGL_FMT_R16G16B16_SINT:
            return "XGL_FMT_R16G16B16_SINT";
        case XGL_FMT_R16G16B16_SNORM:
            return "XGL_FMT_R16G16B16_SNORM";
        case XGL_FMT_R16G16B16_SSCALED:
            return "XGL_FMT_R16G16B16_SSCALED";
        case XGL_FMT_R16G16B16_UINT:
            return "XGL_FMT_R16G16B16_UINT";
        case XGL_FMT_R16G16B16_UNORM:
            return "XGL_FMT_R16G16B16_UNORM";
        case XGL_FMT_R16G16B16_USCALED:
            return "XGL_FMT_R16G16B16_USCALED";
        case XGL_FMT_R16G16_SFLOAT:
            return "XGL_FMT_R16G16_SFLOAT";
        case XGL_FMT_R16G16_SINT:
            return "XGL_FMT_R16G16_SINT";
        case XGL_FMT_R16G16_SNORM:
            return "XGL_FMT_R16G16_SNORM";
        case XGL_FMT_R16G16_SSCALED:
            return "XGL_FMT_R16G16_SSCALED";
        case XGL_FMT_R16G16_UINT:
            return "XGL_FMT_R16G16_UINT";
        case XGL_FMT_R16G16_UNORM:
            return "XGL_FMT_R16G16_UNORM";
        case XGL_FMT_R16G16_USCALED:
            return "XGL_FMT_R16G16_USCALED";
        case XGL_FMT_R16_SFLOAT:
            return "XGL_FMT_R16_SFLOAT";
        case XGL_FMT_R16_SINT:
            return "XGL_FMT_R16_SINT";
        case XGL_FMT_R16_SNORM:
            return "XGL_FMT_R16_SNORM";
        case XGL_FMT_R16_SSCALED:
            return "XGL_FMT_R16_SSCALED";
        case XGL_FMT_R16_UINT:
            return "XGL_FMT_R16_UINT";
        case XGL_FMT_R16_UNORM:
            return "XGL_FMT_R16_UNORM";
        case XGL_FMT_R16_USCALED:
            return "XGL_FMT_R16_USCALED";
        case XGL_FMT_R32G32B32A32_SFLOAT:
            return "XGL_FMT_R32G32B32A32_SFLOAT";
        case XGL_FMT_R32G32B32A32_SINT:
            return "XGL_FMT_R32G32B32A32_SINT";
        case XGL_FMT_R32G32B32A32_UINT:
            return "XGL_FMT_R32G32B32A32_UINT";
        case XGL_FMT_R32G32B32_SFLOAT:
            return "XGL_FMT_R32G32B32_SFLOAT";
        case XGL_FMT_R32G32B32_SINT:
            return "XGL_FMT_R32G32B32_SINT";
        case XGL_FMT_R32G32B32_UINT:
            return "XGL_FMT_R32G32B32_UINT";
        case XGL_FMT_R32G32_SFLOAT:
            return "XGL_FMT_R32G32_SFLOAT";
        case XGL_FMT_R32G32_SINT:
            return "XGL_FMT_R32G32_SINT";
        case XGL_FMT_R32G32_UINT:
            return "XGL_FMT_R32G32_UINT";
        case XGL_FMT_R32_SFLOAT:
            return "XGL_FMT_R32_SFLOAT";
        case XGL_FMT_R32_SINT:
            return "XGL_FMT_R32_SINT";
        case XGL_FMT_R32_UINT:
            return "XGL_FMT_R32_UINT";
        case XGL_FMT_R4G4B4A4_UNORM:
            return "XGL_FMT_R4G4B4A4_UNORM";
        case XGL_FMT_R4G4B4A4_USCALED:
            return "XGL_FMT_R4G4B4A4_USCALED";
        case XGL_FMT_R4G4_UNORM:
            return "XGL_FMT_R4G4_UNORM";
        case XGL_FMT_R4G4_USCALED:
            return "XGL_FMT_R4G4_USCALED";
        case XGL_FMT_R5G5B5A1_UNORM:
            return "XGL_FMT_R5G5B5A1_UNORM";
        case XGL_FMT_R5G5B5A1_USCALED:
            return "XGL_FMT_R5G5B5A1_USCALED";
        case XGL_FMT_R5G6B5_UNORM:
            return "XGL_FMT_R5G6B5_UNORM";
        case XGL_FMT_R5G6B5_USCALED:
            return "XGL_FMT_R5G6B5_USCALED";
        case XGL_FMT_R64G64B64A64_SFLOAT:
            return "XGL_FMT_R64G64B64A64_SFLOAT";
        case XGL_FMT_R64G64B64_SFLOAT:
            return "XGL_FMT_R64G64B64_SFLOAT";
        case XGL_FMT_R64G64_SFLOAT:
            return "XGL_FMT_R64G64_SFLOAT";
        case XGL_FMT_R64_SFLOAT:
            return "XGL_FMT_R64_SFLOAT";
        case XGL_FMT_R8G8B8A8_SINT:
            return "XGL_FMT_R8G8B8A8_SINT";
        case XGL_FMT_R8G8B8A8_SNORM:
            return "XGL_FMT_R8G8B8A8_SNORM";
        case XGL_FMT_R8G8B8A8_SRGB:
            return "XGL_FMT_R8G8B8A8_SRGB";
        case XGL_FMT_R8G8B8A8_SSCALED:
            return "XGL_FMT_R8G8B8A8_SSCALED";
        case XGL_FMT_R8G8B8A8_UINT:
            return "XGL_FMT_R8G8B8A8_UINT";
        case XGL_FMT_R8G8B8A8_UNORM:
            return "XGL_FMT_R8G8B8A8_UNORM";
        case XGL_FMT_R8G8B8A8_USCALED:
            return "XGL_FMT_R8G8B8A8_USCALED";
        case XGL_FMT_R8G8B8_SINT:
            return "XGL_FMT_R8G8B8_SINT";
        case XGL_FMT_R8G8B8_SNORM:
            return "XGL_FMT_R8G8B8_SNORM";
        case XGL_FMT_R8G8B8_SRGB:
            return "XGL_FMT_R8G8B8_SRGB";
        case XGL_FMT_R8G8B8_SSCALED:
            return "XGL_FMT_R8G8B8_SSCALED";
        case XGL_FMT_R8G8B8_UINT:
            return "XGL_FMT_R8G8B8_UINT";
        case XGL_FMT_R8G8B8_UNORM:
            return "XGL_FMT_R8G8B8_UNORM";
        case XGL_FMT_R8G8B8_USCALED:
            return "XGL_FMT_R8G8B8_USCALED";
        case XGL_FMT_R8G8_SINT:
            return "XGL_FMT_R8G8_SINT";
        case XGL_FMT_R8G8_SNORM:
            return "XGL_FMT_R8G8_SNORM";
        case XGL_FMT_R8G8_SRGB:
            return "XGL_FMT_R8G8_SRGB";
        case XGL_FMT_R8G8_SSCALED:
            return "XGL_FMT_R8G8_SSCALED";
        case XGL_FMT_R8G8_UINT:
            return "XGL_FMT_R8G8_UINT";
        case XGL_FMT_R8G8_UNORM:
            return "XGL_FMT_R8G8_UNORM";
        case XGL_FMT_R8G8_USCALED:
            return "XGL_FMT_R8G8_USCALED";
        case XGL_FMT_R8_SINT:
            return "XGL_FMT_R8_SINT";
        case XGL_FMT_R8_SNORM:
            return "XGL_FMT_R8_SNORM";
        case XGL_FMT_R8_SRGB:
            return "XGL_FMT_R8_SRGB";
        case XGL_FMT_R8_SSCALED:
            return "XGL_FMT_R8_SSCALED";
        case XGL_FMT_R8_UINT:
            return "XGL_FMT_R8_UINT";
        case XGL_FMT_R8_UNORM:
            return "XGL_FMT_R8_UNORM";
        case XGL_FMT_R8_USCALED:
            return "XGL_FMT_R8_USCALED";
        case XGL_FMT_R9G9B9E5_UFLOAT:
            return "XGL_FMT_R9G9B9E5_UFLOAT";
        case XGL_FMT_S8_UINT:
            return "XGL_FMT_S8_UINT";
        case XGL_FMT_UNDEFINED:
            return "XGL_FMT_UNDEFINED";
        default:
            return "Unhandled XGL_FORMAT";
    }
}


static inline const char* string_XGL_FACE_ORIENTATION(XGL_FACE_ORIENTATION input_value)
{
    switch ((XGL_FACE_ORIENTATION)input_value)
    {
        case XGL_FRONT_FACE_CCW:
            return "XGL_FRONT_FACE_CCW";
        case XGL_FRONT_FACE_CW:
            return "XGL_FRONT_FACE_CW";
        default:
            return "Unhandled XGL_FACE_ORIENTATION";
    }
}


static inline const char* string_XGL_IMAGE_TYPE(XGL_IMAGE_TYPE input_value)
{
    switch ((XGL_IMAGE_TYPE)input_value)
    {
        case XGL_IMAGE_1D:
            return "XGL_IMAGE_1D";
        case XGL_IMAGE_2D:
            return "XGL_IMAGE_2D";
        case XGL_IMAGE_3D:
            return "XGL_IMAGE_3D";
        default:
            return "Unhandled XGL_IMAGE_TYPE";
    }
}


static inline const char* string_XGL_ATTACHMENT_LOAD_OP(XGL_ATTACHMENT_LOAD_OP input_value)
{
    switch ((XGL_ATTACHMENT_LOAD_OP)input_value)
    {
        case XGL_ATTACHMENT_LOAD_OP_CLEAR:
            return "XGL_ATTACHMENT_LOAD_OP_CLEAR";
        case XGL_ATTACHMENT_LOAD_OP_DONT_CARE:
            return "XGL_ATTACHMENT_LOAD_OP_DONT_CARE";
        case XGL_ATTACHMENT_LOAD_OP_LOAD:
            return "XGL_ATTACHMENT_LOAD_OP_LOAD";
        default:
            return "Unhandled XGL_ATTACHMENT_LOAD_OP";
    }
}


static inline const char* string_XGL_CHANNEL_SWIZZLE(XGL_CHANNEL_SWIZZLE input_value)
{
    switch ((XGL_CHANNEL_SWIZZLE)input_value)
    {
        case XGL_CHANNEL_SWIZZLE_A:
            return "XGL_CHANNEL_SWIZZLE_A";
        case XGL_CHANNEL_SWIZZLE_B:
            return "XGL_CHANNEL_SWIZZLE_B";
        case XGL_CHANNEL_SWIZZLE_G:
            return "XGL_CHANNEL_SWIZZLE_G";
        case XGL_CHANNEL_SWIZZLE_ONE:
            return "XGL_CHANNEL_SWIZZLE_ONE";
        case XGL_CHANNEL_SWIZZLE_R:
            return "XGL_CHANNEL_SWIZZLE_R";
        case XGL_CHANNEL_SWIZZLE_ZERO:
            return "XGL_CHANNEL_SWIZZLE_ZERO";
        default:
            return "Unhandled XGL_CHANNEL_SWIZZLE";
    }
}


static inline const char* string_XGL_SHADER_STAGE_FLAGS(XGL_SHADER_STAGE_FLAGS input_value)
{
    switch ((XGL_SHADER_STAGE_FLAGS)input_value)
    {
        case XGL_SHADER_STAGE_FLAGS_ALL:
            return "XGL_SHADER_STAGE_FLAGS_ALL";
        case XGL_SHADER_STAGE_FLAGS_COMPUTE_BIT:
            return "XGL_SHADER_STAGE_FLAGS_COMPUTE_BIT";
        case XGL_SHADER_STAGE_FLAGS_FRAGMENT_BIT:
            return "XGL_SHADER_STAGE_FLAGS_FRAGMENT_BIT";
        case XGL_SHADER_STAGE_FLAGS_GEOMETRY_BIT:
            return "XGL_SHADER_STAGE_FLAGS_GEOMETRY_BIT";
        case XGL_SHADER_STAGE_FLAGS_TESS_CONTROL_BIT:
            return "XGL_SHADER_STAGE_FLAGS_TESS_CONTROL_BIT";
        case XGL_SHADER_STAGE_FLAGS_TESS_EVALUATION_BIT:
            return "XGL_SHADER_STAGE_FLAGS_TESS_EVALUATION_BIT";
        case XGL_SHADER_STAGE_FLAGS_VERTEX_BIT:
            return "XGL_SHADER_STAGE_FLAGS_VERTEX_BIT";
        default:
            return "Unhandled XGL_SHADER_STAGE_FLAGS";
    }
}


static inline const char* string_XGL_IMAGE_ASPECT(XGL_IMAGE_ASPECT input_value)
{
    switch ((XGL_IMAGE_ASPECT)input_value)
    {
        case XGL_IMAGE_ASPECT_COLOR:
            return "XGL_IMAGE_ASPECT_COLOR";
        case XGL_IMAGE_ASPECT_DEPTH:
            return "XGL_IMAGE_ASPECT_DEPTH";
        case XGL_IMAGE_ASPECT_STENCIL:
            return "XGL_IMAGE_ASPECT_STENCIL";
        default:
            return "Unhandled XGL_IMAGE_ASPECT";
    }
}


static inline const char* string_XGL_BUFFER_USAGE_FLAGS(XGL_BUFFER_USAGE_FLAGS input_value)
{
    switch ((XGL_BUFFER_USAGE_FLAGS)input_value)
    {
        case XGL_BUFFER_USAGE_GENERAL:
            return "XGL_BUFFER_USAGE_GENERAL";
        case XGL_BUFFER_USAGE_IMAGE_BUFFER_BIT:
            return "XGL_BUFFER_USAGE_IMAGE_BUFFER_BIT";
        case XGL_BUFFER_USAGE_INDEX_FETCH_BIT:
            return "XGL_BUFFER_USAGE_INDEX_FETCH_BIT";
        case XGL_BUFFER_USAGE_INDIRECT_PARAMETER_FETCH_BIT:
            return "XGL_BUFFER_USAGE_INDIRECT_PARAMETER_FETCH_BIT";
        case XGL_BUFFER_USAGE_RAW_BUFFER_BIT:
            return "XGL_BUFFER_USAGE_RAW_BUFFER_BIT";
        case XGL_BUFFER_USAGE_SHADER_ACCESS_ATOMIC_BIT:
            return "XGL_BUFFER_USAGE_SHADER_ACCESS_ATOMIC_BIT";
        case XGL_BUFFER_USAGE_SHADER_ACCESS_READ_BIT:
            return "XGL_BUFFER_USAGE_SHADER_ACCESS_READ_BIT";
        case XGL_BUFFER_USAGE_SHADER_ACCESS_WRITE_BIT:
            return "XGL_BUFFER_USAGE_SHADER_ACCESS_WRITE_BIT";
        case XGL_BUFFER_USAGE_SHADER_STORAGE_BIT:
            return "XGL_BUFFER_USAGE_SHADER_STORAGE_BIT";
        case XGL_BUFFER_USAGE_TEXTURE_BUFFER_BIT:
            return "XGL_BUFFER_USAGE_TEXTURE_BUFFER_BIT";
        case XGL_BUFFER_USAGE_TRANSFER_DESTINATION_BIT:
            return "XGL_BUFFER_USAGE_TRANSFER_DESTINATION_BIT";
        case XGL_BUFFER_USAGE_TRANSFER_SOURCE_BIT:
            return "XGL_BUFFER_USAGE_TRANSFER_SOURCE_BIT";
        case XGL_BUFFER_USAGE_UNIFORM_READ_BIT:
            return "XGL_BUFFER_USAGE_UNIFORM_READ_BIT";
        case XGL_BUFFER_USAGE_VERTEX_FETCH_BIT:
            return "XGL_BUFFER_USAGE_VERTEX_FETCH_BIT";
        default:
            return "Unhandled XGL_BUFFER_USAGE_FLAGS";
    }
}


static inline const char* string_XGL_SEMAPHORE_CREATE_FLAGS(XGL_SEMAPHORE_CREATE_FLAGS input_value)
{
    switch ((XGL_SEMAPHORE_CREATE_FLAGS)input_value)
    {
        case XGL_SEMAPHORE_CREATE_SHAREABLE_BIT:
            return "XGL_SEMAPHORE_CREATE_SHAREABLE_BIT";
        default:
            return "Unhandled XGL_SEMAPHORE_CREATE_FLAGS";
    }
}


static inline const char* string_XGL_MEMORY_INPUT_FLAGS(XGL_MEMORY_INPUT_FLAGS input_value)
{
    switch ((XGL_MEMORY_INPUT_FLAGS)input_value)
    {
        case XGL_MEMORY_INPUT_COLOR_ATTACHMENT_BIT:
            return "XGL_MEMORY_INPUT_COLOR_ATTACHMENT_BIT";
        case XGL_MEMORY_INPUT_COPY_BIT:
            return "XGL_MEMORY_INPUT_COPY_BIT";
        case XGL_MEMORY_INPUT_CPU_READ_BIT:
            return "XGL_MEMORY_INPUT_CPU_READ_BIT";
        case XGL_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT:
            return "XGL_MEMORY_INPUT_DEPTH_STENCIL_ATTACHMENT_BIT";
        case XGL_MEMORY_INPUT_INDEX_FETCH_BIT:
            return "XGL_MEMORY_INPUT_INDEX_FETCH_BIT";
        case XGL_MEMORY_INPUT_INDIRECT_COMMAND_BIT:
            return "XGL_MEMORY_INPUT_INDIRECT_COMMAND_BIT";
        case XGL_MEMORY_INPUT_SHADER_READ_BIT:
            return "XGL_MEMORY_INPUT_SHADER_READ_BIT";
        case XGL_MEMORY_INPUT_UNIFORM_READ_BIT:
            return "XGL_MEMORY_INPUT_UNIFORM_READ_BIT";
        case XGL_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT:
            return "XGL_MEMORY_INPUT_VERTEX_ATTRIBUTE_FETCH_BIT";
        default:
            return "Unhandled XGL_MEMORY_INPUT_FLAGS";
    }
}


static inline const char* string_XGL_INDEX_TYPE(XGL_INDEX_TYPE input_value)
{
    switch ((XGL_INDEX_TYPE)input_value)
    {
        case XGL_INDEX_16:
            return "XGL_INDEX_16";
        case XGL_INDEX_32:
            return "XGL_INDEX_32";
        case XGL_INDEX_8:
            return "XGL_INDEX_8";
        default:
            return "Unhandled XGL_INDEX_TYPE";
    }
}


static inline const char* string_XGL_CMD_BUFFER_BUILD_FLAGS(XGL_CMD_BUFFER_BUILD_FLAGS input_value)
{
    switch ((XGL_CMD_BUFFER_BUILD_FLAGS)input_value)
    {
        case XGL_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT:
            return "XGL_CMD_BUFFER_OPTIMIZE_DESCRIPTOR_SET_SWITCH_BIT";
        case XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT:
            return "XGL_CMD_BUFFER_OPTIMIZE_GPU_SMALL_BATCH_BIT";
        case XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT:
            return "XGL_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT";
        case XGL_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH_BIT:
            return "XGL_CMD_BUFFER_OPTIMIZE_PIPELINE_SWITCH_BIT";
        default:
            return "Unhandled XGL_CMD_BUFFER_BUILD_FLAGS";
    }
}


static inline const char* string_XGL_FORMAT_FEATURE_FLAGS(XGL_FORMAT_FEATURE_FLAGS input_value)
{
    switch ((XGL_FORMAT_FEATURE_FLAGS)input_value)
    {
        case XGL_FORMAT_COLOR_ATTACHMENT_BLEND_BIT:
            return "XGL_FORMAT_COLOR_ATTACHMENT_BLEND_BIT";
        case XGL_FORMAT_COLOR_ATTACHMENT_WRITE_BIT:
            return "XGL_FORMAT_COLOR_ATTACHMENT_WRITE_BIT";
        case XGL_FORMAT_CONVERSION_BIT:
            return "XGL_FORMAT_CONVERSION_BIT";
        case XGL_FORMAT_DEPTH_ATTACHMENT_BIT:
            return "XGL_FORMAT_DEPTH_ATTACHMENT_BIT";
        case XGL_FORMAT_IMAGE_COPY_BIT:
            return "XGL_FORMAT_IMAGE_COPY_BIT";
        case XGL_FORMAT_IMAGE_SHADER_READ_BIT:
            return "XGL_FORMAT_IMAGE_SHADER_READ_BIT";
        case XGL_FORMAT_IMAGE_SHADER_WRITE_BIT:
            return "XGL_FORMAT_IMAGE_SHADER_WRITE_BIT";
        case XGL_FORMAT_MEMORY_SHADER_ACCESS_BIT:
            return "XGL_FORMAT_MEMORY_SHADER_ACCESS_BIT";
        case XGL_FORMAT_MSAA_ATTACHMENT_BIT:
            return "XGL_FORMAT_MSAA_ATTACHMENT_BIT";
        case XGL_FORMAT_STENCIL_ATTACHMENT_BIT:
            return "XGL_FORMAT_STENCIL_ATTACHMENT_BIT";
        default:
            return "Unhandled XGL_FORMAT_FEATURE_FLAGS";
    }
}

