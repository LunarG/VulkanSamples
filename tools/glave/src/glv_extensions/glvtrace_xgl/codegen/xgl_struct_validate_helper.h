//This is the copyright
//#includes, #defines, globals and such...
#include <xgl.h>
#include <xgl_enum_string_helper.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "xgl_enum_validate_helper.h"

// Function Prototypes
uint32_t xgl_validate_xgl_buffer_view_attach_info(const XGL_BUFFER_VIEW_ATTACH_INFO* pStruct);
uint32_t xgl_validate_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct);
uint32_t xgl_validate_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_layer_create_info(const XGL_LAYER_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_clear_color_value(const XGL_CLEAR_COLOR_VALUE* pStruct);
uint32_t xgl_validate_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_image_memory_bind_info(const XGL_IMAGE_MEMORY_BIND_INFO* pStruct);
uint32_t xgl_validate_xgl_memory_barrier(const XGL_MEMORY_BARRIER* pStruct);
uint32_t xgl_validate_xgl_memory_ref(const XGL_MEMORY_REF* pStruct);
uint32_t xgl_validate_xgl_image_memory_requirements(const XGL_IMAGE_MEMORY_REQUIREMENTS* pStruct);
uint32_t xgl_validate_xgl_image_memory_barrier(const XGL_IMAGE_MEMORY_BARRIER* pStruct);
uint32_t xgl_validate_xgl_update_sampler_textures(const XGL_UPDATE_SAMPLER_TEXTURES* pStruct);
uint32_t xgl_validate_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct);
uint32_t xgl_validate_xgl_buffer_memory_requirements(const XGL_BUFFER_MEMORY_REQUIREMENTS* pStruct);
uint32_t xgl_validate_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct);
uint32_t xgl_validate_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct);
uint32_t xgl_validate_xgl_buffer_view_create_info(const XGL_BUFFER_VIEW_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_pipeline_barrier(const XGL_PIPELINE_BARRIER* pStruct);
uint32_t xgl_validate_xgl_buffer_create_info(const XGL_BUFFER_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct);
uint32_t xgl_validate_xgl_pipeline_vertex_input_create_info(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct);
uint32_t xgl_validate_xgl_pipeline_ds_state_create_info(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_memory_alloc_buffer_info(const XGL_MEMORY_ALLOC_BUFFER_INFO* pStruct);
uint32_t xgl_validate_xgl_vertex_input_attribute_description(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pStruct);
uint32_t xgl_validate_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct);
uint32_t xgl_validate_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct);
uint32_t xgl_validate_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct);
uint32_t xgl_validate_xgl_buffer_image_copy(const XGL_BUFFER_IMAGE_COPY* pStruct);
uint32_t xgl_validate_xgl_dynamic_cb_state_create_info(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct);
uint32_t xgl_validate_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct);
uint32_t xgl_validate_xgl_cmd_buffer_graphics_begin_info(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pStruct);
uint32_t xgl_validate_xgl_extent3d(const XGL_EXTENT3D* pStruct);
uint32_t xgl_validate_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct);
uint32_t xgl_validate_xgl_offset3d(const XGL_OFFSET3D* pStruct);
uint32_t xgl_validate_xgl_offset2d(const XGL_OFFSET2D* pStruct);
uint32_t xgl_validate_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct);
uint32_t xgl_validate_xgl_viewport(const XGL_VIEWPORT* pStruct);
uint32_t xgl_validate_xgl_descriptor_type_count(const XGL_DESCRIPTOR_TYPE_COUNT* pStruct);
uint32_t xgl_validate_xgl_memory_alloc_image_info(const XGL_MEMORY_ALLOC_IMAGE_INFO* pStruct);
uint32_t xgl_validate_xgl_update_samplers(const XGL_UPDATE_SAMPLERS* pStruct);
uint32_t xgl_validate_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct);
uint32_t xgl_validate_xgl_descriptor_region_create_info(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct);
uint32_t xgl_validate_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct);
uint32_t xgl_validate_xgl_dynamic_ds_state_create_info(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct);
uint32_t xgl_validate_xgl_image_copy(const XGL_IMAGE_COPY* pStruct);
uint32_t xgl_validate_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct);
uint32_t xgl_validate_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct);
uint32_t xgl_validate_xgl_update_buffers(const XGL_UPDATE_BUFFERS* pStruct);
uint32_t xgl_validate_xgl_clear_color(const XGL_CLEAR_COLOR* pStruct);
uint32_t xgl_validate_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct);
uint32_t xgl_validate_xgl_dynamic_rs_state_create_info(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct);
uint32_t xgl_validate_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct);
uint32_t xgl_validate_xgl_cmd_buffer_begin_info(const XGL_CMD_BUFFER_BEGIN_INFO* pStruct);
uint32_t xgl_validate_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_update_images(const XGL_UPDATE_IMAGES* pStruct);
uint32_t xgl_validate_xgl_application_info(const XGL_APPLICATION_INFO* pStruct);
uint32_t xgl_validate_xgl_descriptor_set_layout_create_info(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_extent2d(const XGL_EXTENT2D* pStruct);
uint32_t xgl_validate_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct);
uint32_t xgl_validate_xgl_event_wait_info(const XGL_EVENT_WAIT_INFO* pStruct);
uint32_t xgl_validate_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct);
uint32_t xgl_validate_xgl_dynamic_vp_state_create_info(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_render_pass_create_info(const XGL_RENDER_PASS_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_buffer_memory_barrier(const XGL_BUFFER_MEMORY_BARRIER* pStruct);
uint32_t xgl_validate_xgl_pipeline_vp_state_create_info(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_pipeline_ms_state_create_info(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct);
uint32_t xgl_validate_xgl_update_as_copy(const XGL_UPDATE_AS_COPY* pStruct);
uint32_t xgl_validate_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct);
uint32_t xgl_validate_xgl_rect(const XGL_RECT* pStruct);
uint32_t xgl_validate_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_sampler_image_view_info(const XGL_SAMPLER_IMAGE_VIEW_INFO* pStruct);
uint32_t xgl_validate_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct);
uint32_t xgl_validate_xgl_vertex_input_binding_description(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pStruct);
uint32_t xgl_validate_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct);
uint32_t xgl_validate_xgl_buffer_copy(const XGL_BUFFER_COPY* pStruct);
uint32_t xgl_validate_xgl_framebuffer_create_info(const XGL_FRAMEBUFFER_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct);
uint32_t xgl_validate_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct);


uint32_t xgl_validate_xgl_buffer_view_attach_info(const XGL_BUFFER_VIEW_ATTACH_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_IMAGE_TYPE(pStruct->imageType))
        return 0;
    if (!validate_XGL_FORMAT(pStruct->format))
        return 0;
    if (!xgl_validate_xgl_extent3d((const XGL_EXTENT3D*)&pStruct->extent))
        return 0;
    if (!validate_XGL_IMAGE_TILING(pStruct->tiling))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_IMAGE_VIEW_TYPE(pStruct->viewType))
        return 0;
    if (!validate_XGL_FORMAT(pStruct->format))
        return 0;
    if (!xgl_validate_xgl_channel_mapping((const XGL_CHANNEL_MAPPING*)&pStruct->channels))
        return 0;
    if (!xgl_validate_xgl_image_subresource_range((const XGL_IMAGE_SUBRESOURCE_RANGE*)&pStruct->subresourceRange))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_layer_create_info(const XGL_LAYER_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!xgl_validate_xgl_pipeline_shader((const XGL_PIPELINE_SHADER*)&pStruct->cs))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_clear_color_value(const XGL_CLEAR_COLOR_VALUE* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_COORDINATE_ORIGIN(pStruct->pointOrigin))
        return 0;
    if (!validate_XGL_PROVOKING_VERTEX_CONVENTION(pStruct->provokingVertex))
        return 0;
    if (!validate_XGL_FILL_MODE(pStruct->fillMode))
        return 0;
    if (!validate_XGL_CULL_MODE(pStruct->cullMode))
        return 0;
    if (!validate_XGL_FACE_ORIENTATION(pStruct->frontFace))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_image_memory_bind_info(const XGL_IMAGE_MEMORY_BIND_INFO* pStruct)
{
    if (!xgl_validate_xgl_image_subresource((const XGL_IMAGE_SUBRESOURCE*)&pStruct->subresource))
        return 0;
    if (!xgl_validate_xgl_offset3d((const XGL_OFFSET3D*)&pStruct->offset))
        return 0;
    if (!xgl_validate_xgl_extent3d((const XGL_EXTENT3D*)&pStruct->extent))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_memory_barrier(const XGL_MEMORY_BARRIER* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_memory_ref(const XGL_MEMORY_REF* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_image_memory_requirements(const XGL_IMAGE_MEMORY_REQUIREMENTS* pStruct)
{
    if (!validate_XGL_IMAGE_FORMAT_CLASS(pStruct->formatClass))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_image_memory_barrier(const XGL_IMAGE_MEMORY_BARRIER* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_IMAGE_LAYOUT(pStruct->oldLayout))
        return 0;
    if (!validate_XGL_IMAGE_LAYOUT(pStruct->newLayout))
        return 0;
    if (!xgl_validate_xgl_image_subresource_range((const XGL_IMAGE_SUBRESOURCE_RANGE*)&pStruct->subresourceRange))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_update_sampler_textures(const XGL_UPDATE_SAMPLER_TEXTURES* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (pStruct->pSamplerImageViews && !xgl_validate_xgl_sampler_image_view_info((const XGL_SAMPLER_IMAGE_VIEW_INFO*)pStruct->pSamplerImageViews))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct)
{
    if (!validate_XGL_PHYSICAL_GPU_TYPE(pStruct->gpuType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_buffer_memory_requirements(const XGL_BUFFER_MEMORY_REQUIREMENTS* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct)
{
    if (!validate_XGL_FORMAT(pStruct->format))
        return 0;
    if (!validate_XGL_BLEND(pStruct->srcBlendColor))
        return 0;
    if (!validate_XGL_BLEND(pStruct->destBlendColor))
        return 0;
    if (!validate_XGL_BLEND_FUNC(pStruct->blendFuncColor))
        return 0;
    if (!validate_XGL_BLEND(pStruct->srcBlendAlpha))
        return 0;
    if (!validate_XGL_BLEND(pStruct->destBlendAlpha))
        return 0;
    if (!validate_XGL_BLEND_FUNC(pStruct->blendFuncAlpha))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_buffer_view_create_info(const XGL_BUFFER_VIEW_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_BUFFER_VIEW_TYPE(pStruct->viewType))
        return 0;
    if (!validate_XGL_FORMAT(pStruct->format))
        return 0;
    if (!xgl_validate_xgl_channel_mapping((const XGL_CHANNEL_MAPPING*)&pStruct->channels))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_barrier(const XGL_PIPELINE_BARRIER* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_WAIT_EVENT(pStruct->waitEvent))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_buffer_create_info(const XGL_BUFFER_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_QUEUE_TYPE(pStruct->queueType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct)
{
    if (!validate_XGL_MEMORY_TYPE(pStruct->memType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_vertex_input_create_info(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (pStruct->pVertexBindingDescriptions && !xgl_validate_xgl_vertex_input_binding_description((const XGL_VERTEX_INPUT_BINDING_DESCRIPTION*)pStruct->pVertexBindingDescriptions))
        return 0;
    if (pStruct->pVertexAttributeDescriptions && !xgl_validate_xgl_vertex_input_attribute_description((const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION*)pStruct->pVertexAttributeDescriptions))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct)
{
    if (!validate_XGL_CHANNEL_SWIZZLE(pStruct->r))
        return 0;
    if (!validate_XGL_CHANNEL_SWIZZLE(pStruct->g))
        return 0;
    if (!validate_XGL_CHANNEL_SWIZZLE(pStruct->b))
        return 0;
    if (!validate_XGL_CHANNEL_SWIZZLE(pStruct->a))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_ds_state_create_info(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_FORMAT(pStruct->format))
        return 0;
    if (!validate_XGL_COMPARE_FUNC(pStruct->depthFunc))
        return 0;
    if (!xgl_validate_xgl_stencil_op_state((const XGL_STENCIL_OP_STATE*)&pStruct->front))
        return 0;
    if (!xgl_validate_xgl_stencil_op_state((const XGL_STENCIL_OP_STATE*)&pStruct->back))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_memory_alloc_buffer_info(const XGL_MEMORY_ALLOC_BUFFER_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_vertex_input_attribute_description(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pStruct)
{
    if (!validate_XGL_FORMAT(pStruct->format))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct)
{
    if (!validate_XGL_IMAGE_ASPECT(pStruct->aspect))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!xgl_validate_xgl_image_subresource_range((const XGL_IMAGE_SUBRESOURCE_RANGE*)&pStruct->msaaResolveSubResource))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct)
{
    if (!validate_XGL_IMAGE_ASPECT(pStruct->aspect))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_buffer_image_copy(const XGL_BUFFER_IMAGE_COPY* pStruct)
{
    if (!xgl_validate_xgl_image_subresource((const XGL_IMAGE_SUBRESOURCE*)&pStruct->imageSubresource))
        return 0;
    if (!xgl_validate_xgl_offset3d((const XGL_OFFSET3D*)&pStruct->imageOffset))
        return 0;
    if (!xgl_validate_xgl_extent3d((const XGL_EXTENT3D*)&pStruct->imageExtent))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_dynamic_cb_state_create_info(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_MEMORY_TYPE(pStruct->memType))
        return 0;
    if (!validate_XGL_MEMORY_PRIORITY(pStruct->memPriority))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_LOGIC_OP(pStruct->logicOp))
        return 0;
    if (pStruct->pAttachments && !xgl_validate_xgl_pipeline_cb_attachment_state((const XGL_PIPELINE_CB_ATTACHMENT_STATE*)pStruct->pAttachments))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_cmd_buffer_graphics_begin_info(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_extent3d(const XGL_EXTENT3D* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_offset3d(const XGL_OFFSET3D* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_offset2d(const XGL_OFFSET2D* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_viewport(const XGL_VIEWPORT* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_descriptor_type_count(const XGL_DESCRIPTOR_TYPE_COUNT* pStruct)
{
    if (!validate_XGL_DESCRIPTOR_TYPE(pStruct->type))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_memory_alloc_image_info(const XGL_MEMORY_ALLOC_IMAGE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_IMAGE_FORMAT_CLASS(pStruct->formatClass))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_update_samplers(const XGL_UPDATE_SAMPLERS* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_IMAGE_LAYOUT(pStruct->layout))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_descriptor_region_create_info(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (pStruct->pTypeCount && !xgl_validate_xgl_descriptor_type_count((const XGL_DESCRIPTOR_TYPE_COUNT*)pStruct->pTypeCount))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_dynamic_ds_state_create_info(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_image_copy(const XGL_IMAGE_COPY* pStruct)
{
    if (!xgl_validate_xgl_image_subresource((const XGL_IMAGE_SUBRESOURCE*)&pStruct->srcSubresource))
        return 0;
    if (!xgl_validate_xgl_offset3d((const XGL_OFFSET3D*)&pStruct->srcOffset))
        return 0;
    if (!xgl_validate_xgl_image_subresource((const XGL_IMAGE_SUBRESOURCE*)&pStruct->destSubresource))
        return 0;
    if (!xgl_validate_xgl_offset3d((const XGL_OFFSET3D*)&pStruct->destOffset))
        return 0;
    if (!xgl_validate_xgl_extent3d((const XGL_EXTENT3D*)&pStruct->extent))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_TEX_FILTER(pStruct->magFilter))
        return 0;
    if (!validate_XGL_TEX_FILTER(pStruct->minFilter))
        return 0;
    if (!validate_XGL_TEX_MIPMAP_MODE(pStruct->mipMode))
        return 0;
    if (!validate_XGL_TEX_ADDRESS(pStruct->addressU))
        return 0;
    if (!validate_XGL_TEX_ADDRESS(pStruct->addressV))
        return 0;
    if (!validate_XGL_TEX_ADDRESS(pStruct->addressW))
        return 0;
    if (!validate_XGL_COMPARE_FUNC(pStruct->compareFunc))
        return 0;
    if (!validate_XGL_BORDER_COLOR_TYPE(pStruct->borderColorType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_update_buffers(const XGL_UPDATE_BUFFERS* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType))
        return 0;
    if (pStruct->pBufferViews && !xgl_validate_xgl_buffer_view_attach_info((const XGL_BUFFER_VIEW_ATTACH_INFO*)pStruct->pBufferViews))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_clear_color(const XGL_CLEAR_COLOR* pStruct)
{
    if (!xgl_validate_xgl_clear_color_value((const XGL_CLEAR_COLOR_VALUE*)&pStruct->color))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct)
{
    if (!xgl_validate_xgl_image_subresource((const XGL_IMAGE_SUBRESOURCE*)&pStruct->srcSubresource))
        return 0;
    if (!xgl_validate_xgl_offset2d((const XGL_OFFSET2D*)&pStruct->srcOffset))
        return 0;
    if (!xgl_validate_xgl_image_subresource((const XGL_IMAGE_SUBRESOURCE*)&pStruct->destSubresource))
        return 0;
    if (!xgl_validate_xgl_offset2d((const XGL_OFFSET2D*)&pStruct->destOffset))
        return 0;
    if (!xgl_validate_xgl_extent2d((const XGL_EXTENT2D*)&pStruct->extent))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_dynamic_rs_state_create_info(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct)
{
    if (!validate_XGL_STENCIL_OP(pStruct->stencilFailOp))
        return 0;
    if (!validate_XGL_STENCIL_OP(pStruct->stencilPassOp))
        return 0;
    if (!validate_XGL_STENCIL_OP(pStruct->stencilDepthFailOp))
        return 0;
    if (!validate_XGL_COMPARE_FUNC(pStruct->stencilFunc))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct)
{
    if (!validate_XGL_IMAGE_LAYOUT(pStruct->layout))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_cmd_buffer_begin_info(const XGL_CMD_BUFFER_BEGIN_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!xgl_validate_xgl_pipeline_shader((const XGL_PIPELINE_SHADER*)&pStruct->shader))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_PRIMITIVE_TOPOLOGY(pStruct->topology))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_update_images(const XGL_UPDATE_IMAGES* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType))
        return 0;
    if (pStruct->pImageViews && !xgl_validate_xgl_image_view_attach_info((const XGL_IMAGE_VIEW_ATTACH_INFO*)pStruct->pImageViews))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_application_info(const XGL_APPLICATION_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_descriptor_set_layout_create_info(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_extent2d(const XGL_EXTENT2D* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct)
{
    if (!validate_XGL_IMAGE_LAYOUT(pStruct->layout))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_event_wait_info(const XGL_EVENT_WAIT_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_WAIT_EVENT(pStruct->waitEvent))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_dynamic_vp_state_create_info(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (pStruct->pViewports && !xgl_validate_xgl_viewport((const XGL_VIEWPORT*)pStruct->pViewports))
        return 0;
    if (pStruct->pScissors && !xgl_validate_xgl_rect((const XGL_RECT*)pStruct->pScissors))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_render_pass_create_info(const XGL_RENDER_PASS_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!xgl_validate_xgl_rect((const XGL_RECT*)&pStruct->renderArea))
        return 0;
    if (pStruct->pColorLoadClearValues && !xgl_validate_xgl_clear_color((const XGL_CLEAR_COLOR*)pStruct->pColorLoadClearValues))
        return 0;
    if (!validate_XGL_ATTACHMENT_LOAD_OP(pStruct->depthLoadOp))
        return 0;
    if (!validate_XGL_ATTACHMENT_STORE_OP(pStruct->depthStoreOp))
        return 0;
    if (!validate_XGL_ATTACHMENT_LOAD_OP(pStruct->stencilLoadOp))
        return 0;
    if (!validate_XGL_ATTACHMENT_STORE_OP(pStruct->stencilStoreOp))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_buffer_memory_barrier(const XGL_BUFFER_MEMORY_BARRIER* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_vp_state_create_info(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_COORDINATE_ORIGIN(pStruct->clipOrigin))
        return 0;
    if (!validate_XGL_DEPTH_MODE(pStruct->depthMode))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_ms_state_create_info(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct)
{
    if (!validate_XGL_PIPELINE_SHADER_STAGE(pStruct->stage))
        return 0;
    if (pStruct->pLinkConstBufferInfo && !xgl_validate_xgl_link_const_buffer((const XGL_LINK_CONST_BUFFER*)pStruct->pLinkConstBufferInfo))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_update_as_copy(const XGL_UPDATE_AS_COPY* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_rect(const XGL_RECT* pStruct)
{
    if (!xgl_validate_xgl_offset2d((const XGL_OFFSET2D*)&pStruct->offset))
        return 0;
    if (!xgl_validate_xgl_extent2d((const XGL_EXTENT2D*)&pStruct->extent))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (pStruct->pRequestedQueues && !xgl_validate_xgl_device_queue_create_info((const XGL_DEVICE_QUEUE_CREATE_INFO*)pStruct->pRequestedQueues))
        return 0;
    if (!validate_XGL_VALIDATION_LEVEL(pStruct->maxValidationLevel))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_sampler_image_view_info(const XGL_SAMPLER_IMAGE_VIEW_INFO* pStruct)
{
    if (pStruct->pImageView && !xgl_validate_xgl_image_view_attach_info((const XGL_IMAGE_VIEW_ATTACH_INFO*)pStruct->pImageView))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_vertex_input_binding_description(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pStruct)
{
    if (!validate_XGL_VERTEX_INPUT_STEP_RATE(pStruct->stepRate))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_buffer_copy(const XGL_BUFFER_COPY* pStruct)
{
    return 1;
}
uint32_t xgl_validate_xgl_framebuffer_create_info(const XGL_FRAMEBUFFER_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (pStruct->pColorAttachments && !xgl_validate_xgl_color_attachment_bind_info((const XGL_COLOR_ATTACHMENT_BIND_INFO*)pStruct->pColorAttachments))
        return 0;
    if (pStruct->pDepthStencilAttachment && !xgl_validate_xgl_depth_stencil_bind_info((const XGL_DEPTH_STENCIL_BIND_INFO*)pStruct->pDepthStencilAttachment))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_QUERY_TYPE(pStruct->queryType))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct)
{
    if (!validate_XGL_STRUCTURE_TYPE(pStruct->sType))
        return 0;
    if (!validate_XGL_FORMAT(pStruct->format))
        return 0;
    if (!xgl_validate_xgl_image_subresource_range((const XGL_IMAGE_SUBRESOURCE_RANGE*)&pStruct->msaaResolveSubResource))
        return 0;
    return 1;
}
uint32_t xgl_validate_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct)
{
    return 1;
}