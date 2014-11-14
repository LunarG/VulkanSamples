#pragma once

#include <xgl.h>

static const char* string_XGL_MEMORY_PRIORITY(XGL_MEMORY_PRIORITY input_value)
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

    }
    return "Unhandled XGL_MEMORY_PRIORITY";
}


static const char* string_XGL_IMAGE_ASPECT(XGL_IMAGE_ASPECT input_value)
{
    switch ((XGL_IMAGE_ASPECT)input_value)
    {

    case XGL_IMAGE_ASPECT_COLOR:
        return "XGL_IMAGE_ASPECT_COLOR";

    case XGL_IMAGE_ASPECT_DEPTH:
        return "XGL_IMAGE_ASPECT_DEPTH";

    case XGL_IMAGE_ASPECT_STENCIL:
        return "XGL_IMAGE_ASPECT_STENCIL";

    }
    return "Unhandled XGL_IMAGE_ASPECT";
}


static const char* string_XGL_NUM_FORMAT(XGL_NUM_FORMAT input_value)
{
    switch ((XGL_NUM_FORMAT)input_value)
    {

    case XGL_NUM_FMT_DS:
        return "XGL_NUM_FMT_DS";

    case XGL_NUM_FMT_FLOAT:
        return "XGL_NUM_FMT_FLOAT";

    case XGL_NUM_FMT_SINT:
        return "XGL_NUM_FMT_SINT";

    case XGL_NUM_FMT_SNORM:
        return "XGL_NUM_FMT_SNORM";

    case XGL_NUM_FMT_SRGB:
        return "XGL_NUM_FMT_SRGB";

    case XGL_NUM_FMT_UINT:
        return "XGL_NUM_FMT_UINT";

    case XGL_NUM_FMT_UNDEFINED:
        return "XGL_NUM_FMT_UNDEFINED";

    case XGL_NUM_FMT_UNORM:
        return "XGL_NUM_FMT_UNORM";

    }
    return "Unhandled XGL_NUM_FORMAT";
}


static const char* string_XGL_SUBRESOURCE_INFO_TYPE(XGL_SUBRESOURCE_INFO_TYPE input_value)
{
    switch ((XGL_SUBRESOURCE_INFO_TYPE)input_value)
    {

    case XGL_INFO_TYPE_SUBRESOURCE_LAYOUT:
        return "XGL_INFO_TYPE_SUBRESOURCE_LAYOUT";

    }
    return "Unhandled XGL_SUBRESOURCE_INFO_TYPE";
}


static const char* string_XGL_IMAGE_USAGE_FLAGS(XGL_IMAGE_USAGE_FLAGS input_value)
{
    switch ((XGL_IMAGE_USAGE_FLAGS)input_value)
    {

    case XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
        return "XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";

    case XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT:
        return "XGL_IMAGE_USAGE_DEPTH_STENCIL_BIT";

    case XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT:
        return "XGL_IMAGE_USAGE_SHADER_ACCESS_READ_BIT";

    case XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT:
        return "XGL_IMAGE_USAGE_SHADER_ACCESS_WRITE_BIT";

    }
    return "Unhandled XGL_IMAGE_USAGE_FLAGS";
}


static const char* string_XGL_TEX_FILTER(XGL_TEX_FILTER input_value)
{
    switch ((XGL_TEX_FILTER)input_value)
    {

    case XGL_TEX_FILTER_LINEAR:
        return "XGL_TEX_FILTER_LINEAR";

    case XGL_TEX_FILTER_NEAREST:
        return "XGL_TEX_FILTER_NEAREST";

    }
    return "Unhandled XGL_TEX_FILTER";
}


static const char* string_XGL_DESCRIPTOR_SET_SLOT_TYPE(XGL_DESCRIPTOR_SET_SLOT_TYPE input_value)
{
    switch ((XGL_DESCRIPTOR_SET_SLOT_TYPE)input_value)
    {

    case XGL_SLOT_NEXT_DESCRIPTOR_SET:
        return "XGL_SLOT_NEXT_DESCRIPTOR_SET";

    case XGL_SLOT_SHADER_RESOURCE:
        return "XGL_SLOT_SHADER_RESOURCE";

    case XGL_SLOT_SHADER_SAMPLER:
        return "XGL_SLOT_SHADER_SAMPLER";

    case XGL_SLOT_SHADER_UAV:
        return "XGL_SLOT_SHADER_UAV";

    case XGL_SLOT_UNUSED:
        return "XGL_SLOT_UNUSED";

    }
    return "Unhandled XGL_DESCRIPTOR_SET_SLOT_TYPE";
}


static const char* string_XGL_TEX_ADDRESS(XGL_TEX_ADDRESS input_value)
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

    }
    return "Unhandled XGL_TEX_ADDRESS";
}


static const char* string_XGL_QUERY_TYPE(XGL_QUERY_TYPE input_value)
{
    switch ((XGL_QUERY_TYPE)input_value)
    {

    case XGL_QUERY_OCCLUSION:
        return "XGL_QUERY_OCCLUSION";

    case XGL_QUERY_PIPELINE_STATISTICS:
        return "XGL_QUERY_PIPELINE_STATISTICS";

    }
    return "Unhandled XGL_QUERY_TYPE";
}


static const char* string_XGL_ATOMIC_OP(XGL_ATOMIC_OP input_value)
{
    switch ((XGL_ATOMIC_OP)input_value)
    {

    case XGL_ATOMIC_ADD_INT32:
        return "XGL_ATOMIC_ADD_INT32";

    case XGL_ATOMIC_ADD_INT64:
        return "XGL_ATOMIC_ADD_INT64";

    case XGL_ATOMIC_AND_INT32:
        return "XGL_ATOMIC_AND_INT32";

    case XGL_ATOMIC_AND_INT64:
        return "XGL_ATOMIC_AND_INT64";

    case XGL_ATOMIC_DEC_UINT32:
        return "XGL_ATOMIC_DEC_UINT32";

    case XGL_ATOMIC_DEC_UINT64:
        return "XGL_ATOMIC_DEC_UINT64";

    case XGL_ATOMIC_INC_UINT32:
        return "XGL_ATOMIC_INC_UINT32";

    case XGL_ATOMIC_INC_UINT64:
        return "XGL_ATOMIC_INC_UINT64";

    case XGL_ATOMIC_MAX_SINT32:
        return "XGL_ATOMIC_MAX_SINT32";

    case XGL_ATOMIC_MAX_SINT64:
        return "XGL_ATOMIC_MAX_SINT64";

    case XGL_ATOMIC_MAX_UINT32:
        return "XGL_ATOMIC_MAX_UINT32";

    case XGL_ATOMIC_MAX_UINT64:
        return "XGL_ATOMIC_MAX_UINT64";

    case XGL_ATOMIC_MIN_SINT32:
        return "XGL_ATOMIC_MIN_SINT32";

    case XGL_ATOMIC_MIN_SINT64:
        return "XGL_ATOMIC_MIN_SINT64";

    case XGL_ATOMIC_MIN_UINT32:
        return "XGL_ATOMIC_MIN_UINT32";

    case XGL_ATOMIC_MIN_UINT64:
        return "XGL_ATOMIC_MIN_UINT64";

    case XGL_ATOMIC_OR_INT32:
        return "XGL_ATOMIC_OR_INT32";

    case XGL_ATOMIC_OR_INT64:
        return "XGL_ATOMIC_OR_INT64";

    case XGL_ATOMIC_SUB_INT32:
        return "XGL_ATOMIC_SUB_INT32";

    case XGL_ATOMIC_SUB_INT64:
        return "XGL_ATOMIC_SUB_INT64";

    case XGL_ATOMIC_XOR_INT32:
        return "XGL_ATOMIC_XOR_INT32";

    case XGL_ATOMIC_XOR_INT64:
        return "XGL_ATOMIC_XOR_INT64";

    }
    return "Unhandled XGL_ATOMIC_OP";
}


static const char* string_XGL_PROVOKING_VERTEX_CONVENTION(XGL_PROVOKING_VERTEX_CONVENTION input_value)
{
    switch ((XGL_PROVOKING_VERTEX_CONVENTION)input_value)
    {

    case XGL_PROVOKING_VERTEX_FIRST:
        return "XGL_PROVOKING_VERTEX_FIRST";

    case XGL_PROVOKING_VERTEX_LAST:
        return "XGL_PROVOKING_VERTEX_LAST";

    }
    return "Unhandled XGL_PROVOKING_VERTEX_CONVENTION";
}


static const char* string_XGL_MEMORY_HEAP_INFO_TYPE(XGL_MEMORY_HEAP_INFO_TYPE input_value)
{
    switch ((XGL_MEMORY_HEAP_INFO_TYPE)input_value)
    {

    case XGL_INFO_TYPE_MEMORY_HEAP_PROPERTIES:
        return "XGL_INFO_TYPE_MEMORY_HEAP_PROPERTIES";

    }
    return "Unhandled XGL_MEMORY_HEAP_INFO_TYPE";
}


static const char* string_XGL_PRIMITIVE_TOPOLOGY(XGL_PRIMITIVE_TOPOLOGY input_value)
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

    }
    return "Unhandled XGL_PRIMITIVE_TOPOLOGY";
}


static const char* string_XGL_BLEND_FUNC(XGL_BLEND_FUNC input_value)
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

    }
    return "Unhandled XGL_BLEND_FUNC";
}


static const char* string_XGL_SYSTEM_ALLOC_TYPE(XGL_SYSTEM_ALLOC_TYPE input_value)
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

    }
    return "Unhandled XGL_SYSTEM_ALLOC_TYPE";
}


static const char* string_XGL_MEMORY_STATE(XGL_MEMORY_STATE input_value)
{
    switch ((XGL_MEMORY_STATE)input_value)
    {

    case XGL_MEMORY_STATE_COMPUTE_SHADER_READ_ONLY:
        return "XGL_MEMORY_STATE_COMPUTE_SHADER_READ_ONLY";

    case XGL_MEMORY_STATE_COMPUTE_SHADER_READ_WRITE:
        return "XGL_MEMORY_STATE_COMPUTE_SHADER_READ_WRITE";

    case XGL_MEMORY_STATE_COMPUTE_SHADER_WRITE_ONLY:
        return "XGL_MEMORY_STATE_COMPUTE_SHADER_WRITE_ONLY";

    case XGL_MEMORY_STATE_DATA_TRANSFER:
        return "XGL_MEMORY_STATE_DATA_TRANSFER";

    case XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY:
        return "XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_ONLY";

    case XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_WRITE:
        return "XGL_MEMORY_STATE_GRAPHICS_SHADER_READ_WRITE";

    case XGL_MEMORY_STATE_GRAPHICS_SHADER_WRITE_ONLY:
        return "XGL_MEMORY_STATE_GRAPHICS_SHADER_WRITE_ONLY";

    case XGL_MEMORY_STATE_INDEX_DATA:
        return "XGL_MEMORY_STATE_INDEX_DATA";

    case XGL_MEMORY_STATE_INDIRECT_ARG:
        return "XGL_MEMORY_STATE_INDIRECT_ARG";

    case XGL_MEMORY_STATE_MULTI_SHADER_READ_ONLY:
        return "XGL_MEMORY_STATE_MULTI_SHADER_READ_ONLY";

    case XGL_MEMORY_STATE_QUEUE_ATOMIC:
        return "XGL_MEMORY_STATE_QUEUE_ATOMIC";

    case XGL_MEMORY_STATE_WRITE_TIMESTAMP:
        return "XGL_MEMORY_STATE_WRITE_TIMESTAMP";

    }
    return "Unhandled XGL_MEMORY_STATE";
}


static const char* string_XGL_QUERY_CONTROL_FLAGS(XGL_QUERY_CONTROL_FLAGS input_value)
{
    switch ((XGL_QUERY_CONTROL_FLAGS)input_value)
    {

    case XGL_QUERY_IMPRECISE_DATA_BIT:
        return "XGL_QUERY_IMPRECISE_DATA_BIT";

    }
    return "Unhandled XGL_QUERY_CONTROL_FLAGS";
}


static const char* string_XGL_FORMAT_INFO_TYPE(XGL_FORMAT_INFO_TYPE input_value)
{
    switch ((XGL_FORMAT_INFO_TYPE)input_value)
    {

    case XGL_INFO_TYPE_FORMAT_PROPERTIES:
        return "XGL_INFO_TYPE_FORMAT_PROPERTIES";

    }
    return "Unhandled XGL_FORMAT_INFO_TYPE";
}


static const char* string_XGL_STATE_BIND_POINT(XGL_STATE_BIND_POINT input_value)
{
    switch ((XGL_STATE_BIND_POINT)input_value)
    {

    case XGL_STATE_BIND_COLOR_BLEND:
        return "XGL_STATE_BIND_COLOR_BLEND";

    case XGL_STATE_BIND_DEPTH_STENCIL:
        return "XGL_STATE_BIND_DEPTH_STENCIL";

    case XGL_STATE_BIND_MSAA:
        return "XGL_STATE_BIND_MSAA";

    case XGL_STATE_BIND_RASTER:
        return "XGL_STATE_BIND_RASTER";

    case XGL_STATE_BIND_VIEWPORT:
        return "XGL_STATE_BIND_VIEWPORT";

    }
    return "Unhandled XGL_STATE_BIND_POINT";
}


static const char* string_XGL_CMD_BUFFER_BUILD_FLAGS(XGL_CMD_BUFFER_BUILD_FLAGS input_value)
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

    }
    return "Unhandled XGL_CMD_BUFFER_BUILD_FLAGS";
}


static const char* string_XGL_MEMORY_REF_FLAGS(XGL_MEMORY_REF_FLAGS input_value)
{
    switch ((XGL_MEMORY_REF_FLAGS)input_value)
    {

    case XGL_MEMORY_REF_READ_ONLY_BIT:
        return "XGL_MEMORY_REF_READ_ONLY_BIT";

    }
    return "Unhandled XGL_MEMORY_REF_FLAGS";
}


static const char* string_XGL_TIMESTAMP_TYPE(XGL_TIMESTAMP_TYPE input_value)
{
    switch ((XGL_TIMESTAMP_TYPE)input_value)
    {

    case XGL_TIMESTAMP_BOTTOM:
        return "XGL_TIMESTAMP_BOTTOM";

    case XGL_TIMESTAMP_TOP:
        return "XGL_TIMESTAMP_TOP";

    }
    return "Unhandled XGL_TIMESTAMP_TYPE";
}


static const char* string_XGL_MEMORY_HEAP_FLAGS(XGL_MEMORY_HEAP_FLAGS input_value)
{
    switch ((XGL_MEMORY_HEAP_FLAGS)input_value)
    {

    case XGL_MEMORY_HEAP_CPU_GPU_COHERENT_BIT:
        return "XGL_MEMORY_HEAP_CPU_GPU_COHERENT_BIT";

    case XGL_MEMORY_HEAP_CPU_UNCACHED_BIT:
        return "XGL_MEMORY_HEAP_CPU_UNCACHED_BIT";

    case XGL_MEMORY_HEAP_CPU_VISIBLE_BIT:
        return "XGL_MEMORY_HEAP_CPU_VISIBLE_BIT";

    case XGL_MEMORY_HEAP_CPU_WRITE_COMBINED_BIT:
        return "XGL_MEMORY_HEAP_CPU_WRITE_COMBINED_BIT";

    case XGL_MEMORY_HEAP_HOLDS_PINNED_BIT:
        return "XGL_MEMORY_HEAP_HOLDS_PINNED_BIT";

    case XGL_MEMORY_HEAP_SHAREABLE_BIT:
        return "XGL_MEMORY_HEAP_SHAREABLE_BIT";

    }
    return "Unhandled XGL_MEMORY_HEAP_FLAGS";
}


static const char* string_XGL_MEMORY_ALLOC_FLAGS(XGL_MEMORY_ALLOC_FLAGS input_value)
{
    switch ((XGL_MEMORY_ALLOC_FLAGS)input_value)
    {

    case XGL_MEMORY_ALLOC_SHAREABLE_BIT:
        return "XGL_MEMORY_ALLOC_SHAREABLE_BIT";

    case XGL_MEMORY_ALLOC_VIRTUAL_BIT:
        return "XGL_MEMORY_ALLOC_VIRTUAL_BIT";

    }
    return "Unhandled XGL_MEMORY_ALLOC_FLAGS";
}


static const char* string_XGL_PHYSICAL_GPU_TYPE(XGL_PHYSICAL_GPU_TYPE input_value)
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

    }
    return "Unhandled XGL_PHYSICAL_GPU_TYPE";
}


static const char* string_XGL_BORDER_COLOR_TYPE(XGL_BORDER_COLOR_TYPE input_value)
{
    switch ((XGL_BORDER_COLOR_TYPE)input_value)
    {

    case XGL_BORDER_COLOR_OPAQUE_BLACK:
        return "XGL_BORDER_COLOR_OPAQUE_BLACK";

    case XGL_BORDER_COLOR_OPAQUE_WHITE:
        return "XGL_BORDER_COLOR_OPAQUE_WHITE";

    case XGL_BORDER_COLOR_TRANSPARENT_BLACK:
        return "XGL_BORDER_COLOR_TRANSPARENT_BLACK";

    }
    return "Unhandled XGL_BORDER_COLOR_TYPE";
}


static const char* string_XGL_IMAGE_STATE(XGL_IMAGE_STATE input_value)
{
    switch ((XGL_IMAGE_STATE)input_value)
    {

    case XGL_IMAGE_STATE_CLEAR:
        return "XGL_IMAGE_STATE_CLEAR";

    case XGL_IMAGE_STATE_COMPUTE_SHADER_READ_ONLY:
        return "XGL_IMAGE_STATE_COMPUTE_SHADER_READ_ONLY";

    case XGL_IMAGE_STATE_COMPUTE_SHADER_READ_WRITE:
        return "XGL_IMAGE_STATE_COMPUTE_SHADER_READ_WRITE";

    case XGL_IMAGE_STATE_COMPUTE_SHADER_WRITE_ONLY:
        return "XGL_IMAGE_STATE_COMPUTE_SHADER_WRITE_ONLY";

    case XGL_IMAGE_STATE_DATA_TRANSFER:
        return "XGL_IMAGE_STATE_DATA_TRANSFER";

    case XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY:
        return "XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_ONLY";

    case XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_WRITE:
        return "XGL_IMAGE_STATE_GRAPHICS_SHADER_READ_WRITE";

    case XGL_IMAGE_STATE_GRAPHICS_SHADER_WRITE_ONLY:
        return "XGL_IMAGE_STATE_GRAPHICS_SHADER_WRITE_ONLY";

    case XGL_IMAGE_STATE_MULTI_SHADER_READ_ONLY:
        return "XGL_IMAGE_STATE_MULTI_SHADER_READ_ONLY";

    case XGL_IMAGE_STATE_RESOLVE_DESTINATION:
        return "XGL_IMAGE_STATE_RESOLVE_DESTINATION";

    case XGL_IMAGE_STATE_RESOLVE_SOURCE:
        return "XGL_IMAGE_STATE_RESOLVE_SOURCE";

    case XGL_IMAGE_STATE_TARGET_AND_SHADER_READ_ONLY:
        return "XGL_IMAGE_STATE_TARGET_AND_SHADER_READ_ONLY";

    case XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL:
        return "XGL_IMAGE_STATE_TARGET_RENDER_ACCESS_OPTIMAL";

    case XGL_IMAGE_STATE_TARGET_SHADER_ACCESS_OPTIMAL:
        return "XGL_IMAGE_STATE_TARGET_SHADER_ACCESS_OPTIMAL";

    case XGL_IMAGE_STATE_UNINITIALIZED_TARGET:
        return "XGL_IMAGE_STATE_UNINITIALIZED_TARGET";

    }
    return "Unhandled XGL_IMAGE_STATE";
}


static const char* string_XGL_TEX_MIPMAP_MODE(XGL_TEX_MIPMAP_MODE input_value)
{
    switch ((XGL_TEX_MIPMAP_MODE)input_value)
    {

    case XGL_TEX_MIPMAP_BASE:
        return "XGL_TEX_MIPMAP_BASE";

    case XGL_TEX_MIPMAP_LINEAR:
        return "XGL_TEX_MIPMAP_LINEAR";

    case XGL_TEX_MIPMAP_NEAREST:
        return "XGL_TEX_MIPMAP_NEAREST";

    }
    return "Unhandled XGL_TEX_MIPMAP_MODE";
}


static const char* string_XGL_LOGIC_OP(XGL_LOGIC_OP input_value)
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

    }
    return "Unhandled XGL_LOGIC_OP";
}


static const char* string_XGL_FORMAT_FEATURE_FLAGS(XGL_FORMAT_FEATURE_FLAGS input_value)
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

    }
    return "Unhandled XGL_FORMAT_FEATURE_FLAGS";
}


static const char* string_XGL_VALIDATION_LEVEL(XGL_VALIDATION_LEVEL input_value)
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

    }
    return "Unhandled XGL_VALIDATION_LEVEL";
}


static const char* string_XGL_CULL_MODE(XGL_CULL_MODE input_value)
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

    }
    return "Unhandled XGL_CULL_MODE";
}


static const char* string_XGL_PIPELINE_SHADER_STAGE(XGL_PIPELINE_SHADER_STAGE input_value)
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

    }
    return "Unhandled XGL_PIPELINE_SHADER_STAGE";
}


static const char* string_XGL_CHANNEL_SWIZZLE(XGL_CHANNEL_SWIZZLE input_value)
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

    }
    return "Unhandled XGL_CHANNEL_SWIZZLE";
}


static const char* string_XGL_DEVICE_CREATE_FLAGS(XGL_DEVICE_CREATE_FLAGS input_value)
{
    switch ((XGL_DEVICE_CREATE_FLAGS)input_value)
    {

    case XGL_DEVICE_CREATE_MGPU_IQ_MATCH_BIT:
        return "XGL_DEVICE_CREATE_MGPU_IQ_MATCH_BIT";

    case XGL_DEVICE_CREATE_VALIDATION_BIT:
        return "XGL_DEVICE_CREATE_VALIDATION_BIT";

    }
    return "Unhandled XGL_DEVICE_CREATE_FLAGS";
}


static const char* string_XGL_RESULT(XGL_RESULT input_value)
{
    switch ((XGL_RESULT)input_value)
    {

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

    }
    return "Unhandled XGL_RESULT";
}


static const char* string_XGL_PIPELINE_CREATE_FLAGS(XGL_PIPELINE_CREATE_FLAGS input_value)
{
    switch ((XGL_PIPELINE_CREATE_FLAGS)input_value)
    {

    case XGL_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT:
        return "XGL_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT";

    }
    return "Unhandled XGL_PIPELINE_CREATE_FLAGS";
}


static const char* string_XGL_SEMAPHORE_CREATE_FLAGS(XGL_SEMAPHORE_CREATE_FLAGS input_value)
{
    switch ((XGL_SEMAPHORE_CREATE_FLAGS)input_value)
    {

    case XGL_SEMAPHORE_CREATE_SHAREABLE_BIT:
        return "XGL_SEMAPHORE_CREATE_SHAREABLE_BIT";

    }
    return "Unhandled XGL_SEMAPHORE_CREATE_FLAGS";
}


static const char* string_XGL_STRUCTURE_TYPE(XGL_STRUCTURE_TYPE input_value)
{
    switch ((XGL_STRUCTURE_TYPE)input_value)
    {

    case XGL_STRUCTURE_TYPE_APPLICATION_INFO:
        return "XGL_STRUCTURE_TYPE_APPLICATION_INFO";

    case XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_COLOR_BLEND_STATE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_DEPTH_STENCIL_STATE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO:
        return "XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO";

    case XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO:
        return "XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO";

    case XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO:
        return "XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO";

    case XGL_STRUCTURE_TYPE_MEMORY_STATE_TRANSITION:
        return "XGL_STRUCTURE_TYPE_MEMORY_STATE_TRANSITION";

    case XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO:
        return "XGL_STRUCTURE_TYPE_MEMORY_VIEW_ATTACH_INFO";

    case XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_MSAA_STATE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO:
        return "XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO";

    case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_PIPELINE_DB_STATE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_RASTER_STATE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO";

    case XGL_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO:
        return "XGL_STRUCTURE_TYPE_SEMAPHORE_OPEN_INFO";

    case XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO:
        return "XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO";

    }
    return "Unhandled XGL_STRUCTURE_TYPE";
}


static const char* string_XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS(XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS input_value)
{
    switch ((XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS)input_value)
    {

    case XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT:
        return "XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_DEPTH_BIT";

    case XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT:
        return "XGL_DEPTH_STENCIL_VIEW_CREATE_READ_ONLY_STENCIL_BIT";

    }
    return "Unhandled XGL_DEPTH_STENCIL_VIEW_CREATE_FLAGS";
}


static const char* string_XGL_GPU_COMPATIBILITY_FLAGS(XGL_GPU_COMPATIBILITY_FLAGS input_value)
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

    }
    return "Unhandled XGL_GPU_COMPATIBILITY_FLAGS";
}


static const char* string_XGL_FILL_MODE(XGL_FILL_MODE input_value)
{
    switch ((XGL_FILL_MODE)input_value)
    {

    case XFL_FILL_POINTS:
        return "XFL_FILL_POINTS";

    case XGL_FILL_SOLID:
        return "XGL_FILL_SOLID";

    case XGL_FILL_WIREFRAME:
        return "XGL_FILL_WIREFRAME";

    }
    return "Unhandled XGL_FILL_MODE";
}


static const char* string_XGL_IMAGE_VIEW_TYPE(XGL_IMAGE_VIEW_TYPE input_value)
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

    }
    return "Unhandled XGL_IMAGE_VIEW_TYPE";
}


static const char* string_XGL_IMAGE_TYPE(XGL_IMAGE_TYPE input_value)
{
    switch ((XGL_IMAGE_TYPE)input_value)
    {

    case XGL_IMAGE_1D:
        return "XGL_IMAGE_1D";

    case XGL_IMAGE_2D:
        return "XGL_IMAGE_2D";

    case XGL_IMAGE_3D:
        return "XGL_IMAGE_3D";

    }
    return "Unhandled XGL_IMAGE_TYPE";
}


static const char* string_XGL_INDEX_TYPE(XGL_INDEX_TYPE input_value)
{
    switch ((XGL_INDEX_TYPE)input_value)
    {

    case XGL_INDEX_16:
        return "XGL_INDEX_16";

    case XGL_INDEX_32:
        return "XGL_INDEX_32";

    case XGL_INDEX_8:
        return "XGL_INDEX_8";

    }
    return "Unhandled XGL_INDEX_TYPE";
}


static const char* string_XGL_IMAGE_CREATE_FLAGS(XGL_IMAGE_CREATE_FLAGS input_value)
{
    switch ((XGL_IMAGE_CREATE_FLAGS)input_value)
    {

    case XGL_IMAGE_CREATE_CLONEABLE_BIT:
        return "XGL_IMAGE_CREATE_CLONEABLE_BIT";

    case XGL_IMAGE_CREATE_INVARIANT_DATA_BIT:
        return "XGL_IMAGE_CREATE_INVARIANT_DATA_BIT";

    case XGL_IMAGE_CREATE_SHAREABLE_BIT:
        return "XGL_IMAGE_CREATE_SHAREABLE_BIT";

    }
    return "Unhandled XGL_IMAGE_CREATE_FLAGS";
}


static const char* string_XGL_QUEUE_TYPE(XGL_QUEUE_TYPE input_value)
{
    switch ((XGL_QUEUE_TYPE)input_value)
    {

    case XGL_QUEUE_TYPE_COMPUTE:
        return "XGL_QUEUE_TYPE_COMPUTE";

    case XGL_QUEUE_TYPE_DMA:
        return "XGL_QUEUE_TYPE_DMA";

    case XGL_QUEUE_TYPE_GRAPHICS:
        return "XGL_QUEUE_TYPE_GRAPHICS";

    }
    return "Unhandled XGL_QUEUE_TYPE";
}


static const char* string_XGL_STENCIL_OP(XGL_STENCIL_OP input_value)
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

    }
    return "Unhandled XGL_STENCIL_OP";
}


static const char* string_XGL_CHANNEL_FORMAT(XGL_CHANNEL_FORMAT input_value)
{
    switch ((XGL_CHANNEL_FORMAT)input_value)
    {

    case XGL_CH_FMT_B5G6R5:
        return "XGL_CH_FMT_B5G6R5";

    case XGL_CH_FMT_B8G8R8A8:
        return "XGL_CH_FMT_B8G8R8A8";

    case XGL_CH_FMT_BC1:
        return "XGL_CH_FMT_BC1";

    case XGL_CH_FMT_BC2:
        return "XGL_CH_FMT_BC2";

    case XGL_CH_FMT_BC3:
        return "XGL_CH_FMT_BC3";

    case XGL_CH_FMT_BC4:
        return "XGL_CH_FMT_BC4";

    case XGL_CH_FMT_BC5:
        return "XGL_CH_FMT_BC5";

    case XGL_CH_FMT_BC6S:
        return "XGL_CH_FMT_BC6S";

    case XGL_CH_FMT_BC6U:
        return "XGL_CH_FMT_BC6U";

    case XGL_CH_FMT_BC7:
        return "XGL_CH_FMT_BC7";

    case XGL_CH_FMT_R10G10B10A2:
        return "XGL_CH_FMT_R10G10B10A2";

    case XGL_CH_FMT_R10G11B11:
        return "XGL_CH_FMT_R10G11B11";

    case XGL_CH_FMT_R11G11B10:
        return "XGL_CH_FMT_R11G11B10";

    case XGL_CH_FMT_R16:
        return "XGL_CH_FMT_R16";

    case XGL_CH_FMT_R16G16:
        return "XGL_CH_FMT_R16G16";

    case XGL_CH_FMT_R16G16B16A16:
        return "XGL_CH_FMT_R16G16B16A16";

    case XGL_CH_FMT_R16G8:
        return "XGL_CH_FMT_R16G8";

    case XGL_CH_FMT_R32:
        return "XGL_CH_FMT_R32";

    case XGL_CH_FMT_R32G32:
        return "XGL_CH_FMT_R32G32";

    case XGL_CH_FMT_R32G32B32:
        return "XGL_CH_FMT_R32G32B32";

    case XGL_CH_FMT_R32G32B32A32:
        return "XGL_CH_FMT_R32G32B32A32";

    case XGL_CH_FMT_R32G8:
        return "XGL_CH_FMT_R32G8";

    case XGL_CH_FMT_R4G4:
        return "XGL_CH_FMT_R4G4";

    case XGL_CH_FMT_R4G4B4A4:
        return "XGL_CH_FMT_R4G4B4A4";

    case XGL_CH_FMT_R5G5B5A1:
        return "XGL_CH_FMT_R5G5B5A1";

    case XGL_CH_FMT_R5G6B5:
        return "XGL_CH_FMT_R5G6B5";

    case XGL_CH_FMT_R8:
        return "XGL_CH_FMT_R8";

    case XGL_CH_FMT_R8G8:
        return "XGL_CH_FMT_R8G8";

    case XGL_CH_FMT_R8G8B8A8:
        return "XGL_CH_FMT_R8G8B8A8";

    case XGL_CH_FMT_R9G9B9E5:
        return "XGL_CH_FMT_R9G9B9E5";

    case XGL_CH_FMT_UNDEFINED:
        return "XGL_CH_FMT_UNDEFINED";

    }
    return "Unhandled XGL_CHANNEL_FORMAT";
}


static const char* string_XGL_COMPARE_FUNC(XGL_COMPARE_FUNC input_value)
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

    }
    return "Unhandled XGL_COMPARE_FUNC";
}


static const char* string_XGL_HEAP_MEMORY_TYPE(XGL_HEAP_MEMORY_TYPE input_value)
{
    switch ((XGL_HEAP_MEMORY_TYPE)input_value)
    {

    case XGL_HEAP_MEMORY_EMBEDDED:
        return "XGL_HEAP_MEMORY_EMBEDDED";

    case XGL_HEAP_MEMORY_LOCAL:
        return "XGL_HEAP_MEMORY_LOCAL";

    case XGL_HEAP_MEMORY_OTHER:
        return "XGL_HEAP_MEMORY_OTHER";

    case XGL_HEAP_MEMORY_REMOTE:
        return "XGL_HEAP_MEMORY_REMOTE";

    }
    return "Unhandled XGL_HEAP_MEMORY_TYPE";
}


static const char* string_XGL_OBJECT_INFO_TYPE(XGL_OBJECT_INFO_TYPE input_value)
{
    switch ((XGL_OBJECT_INFO_TYPE)input_value)
    {

    case XGL_INFO_TYPE_MEMORY_REQUIREMENTS:
        return "XGL_INFO_TYPE_MEMORY_REQUIREMENTS";

    }
    return "Unhandled XGL_OBJECT_INFO_TYPE";
}


static const char* string_XGL_IMAGE_TILING(XGL_IMAGE_TILING input_value)
{
    switch ((XGL_IMAGE_TILING)input_value)
    {

    case XGL_LINEAR_TILING:
        return "XGL_LINEAR_TILING";

    case XGL_OPTIMAL_TILING:
        return "XGL_OPTIMAL_TILING";

    }
    return "Unhandled XGL_IMAGE_TILING";
}


static const char* string_XGL_PHYSICAL_GPU_INFO_TYPE(XGL_PHYSICAL_GPU_INFO_TYPE input_value)
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

    }
    return "Unhandled XGL_PHYSICAL_GPU_INFO_TYPE";
}


static const char* string_XGL_FACE_ORIENTATION(XGL_FACE_ORIENTATION input_value)
{
    switch ((XGL_FACE_ORIENTATION)input_value)
    {

    case XGL_FRONT_FACE_CCW:
        return "XGL_FRONT_FACE_CCW";

    case XGL_FRONT_FACE_CW:
        return "XGL_FRONT_FACE_CW";

    }
    return "Unhandled XGL_FACE_ORIENTATION";
}


static const char* string_XGL_QUEUE_FLAGS(XGL_QUEUE_FLAGS input_value)
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

    }
    return "Unhandled XGL_QUEUE_FLAGS";
}


static const char* string_XGL_PIPELINE_BIND_POINT(XGL_PIPELINE_BIND_POINT input_value)
{
    switch ((XGL_PIPELINE_BIND_POINT)input_value)
    {

    case XGL_PIPELINE_BIND_POINT_COMPUTE:
        return "XGL_PIPELINE_BIND_POINT_COMPUTE";

    case XGL_PIPELINE_BIND_POINT_GRAPHICS:
        return "XGL_PIPELINE_BIND_POINT_GRAPHICS";

    }
    return "Unhandled XGL_PIPELINE_BIND_POINT";
}


static const char* string_XGL_BLEND(XGL_BLEND input_value)
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

    }
    return "Unhandled XGL_BLEND";
}

