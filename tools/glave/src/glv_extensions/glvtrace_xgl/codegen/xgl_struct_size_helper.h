//This is the copyright
//#includes, #defines, globals and such...
#include <stdio.h>
#include <stdlib.h>

// Function Prototypes
size_t get_struct_chain_size(const void* pStruct);
size_t get_dynamic_struct_size(const void* pStruct);
size_t xgl_size_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_application_info(const XGL_APPLICATION_INFO* pStruct);
size_t xgl_size_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct);
size_t xgl_size_xgl_update_samplers(const XGL_UPDATE_SAMPLERS* pStruct);
size_t xgl_size_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct);
size_t xgl_size_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct);
size_t xgl_size_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_buffer_memory_requirements(const XGL_BUFFER_MEMORY_REQUIREMENTS* pStruct);
size_t xgl_size_xgl_update_buffers(const XGL_UPDATE_BUFFERS* pStruct);
size_t xgl_size_xgl_vertex_input_attribute_description(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pStruct);
size_t xgl_size_xgl_pipeline_vertex_input_create_info(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pStruct);
size_t xgl_size_xgl_dynamic_rs_state_create_info(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct);
size_t xgl_size_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct);
size_t xgl_size_xgl_dynamic_vp_state_create_info(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_memory_ref(const XGL_MEMORY_REF* pStruct);
size_t xgl_size_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct);
size_t xgl_size_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct);
size_t xgl_size_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct);
size_t xgl_size_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct);
size_t xgl_size_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct);
size_t xgl_size_xgl_vertex_input_binding_description(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pStruct);
size_t xgl_size_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_framebuffer_create_info(const XGL_FRAMEBUFFER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_extent2d(const XGL_EXTENT2D* pStruct);
size_t xgl_size_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct);
size_t xgl_size_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_offset2d(const XGL_OFFSET2D* pStruct);
size_t xgl_size_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_clear_color_value(const XGL_CLEAR_COLOR_VALUE* pStruct);
size_t xgl_size_xgl_buffer_memory_barrier(const XGL_BUFFER_MEMORY_BARRIER* pStruct);
size_t xgl_size_xgl_pipeline_ms_state_create_info(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_update_images(const XGL_UPDATE_IMAGES* pStruct);
size_t xgl_size_xgl_cmd_buffer_begin_info(const XGL_CMD_BUFFER_BEGIN_INFO* pStruct);
size_t xgl_size_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct);
size_t xgl_size_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_rect(const XGL_RECT* pStruct);
size_t xgl_size_xgl_cmd_buffer_graphics_begin_info(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pStruct);
size_t xgl_size_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct);
size_t xgl_size_xgl_update_as_copy(const XGL_UPDATE_AS_COPY* pStruct);
size_t xgl_size_xgl_image_copy(const XGL_IMAGE_COPY* pStruct);
size_t xgl_size_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct);
size_t xgl_size_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct);
size_t xgl_size_xgl_dynamic_ds_state_create_info(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_image_memory_barrier(const XGL_IMAGE_MEMORY_BARRIER* pStruct);
size_t xgl_size_xgl_pipeline_ds_state_create_info(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_barrier(const XGL_PIPELINE_BARRIER* pStruct);
size_t xgl_size_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct);
size_t xgl_size_xgl_memory_alloc_image_info(const XGL_MEMORY_ALLOC_IMAGE_INFO* pStruct);
size_t xgl_size_xgl_update_sampler_textures(const XGL_UPDATE_SAMPLER_TEXTURES* pStruct);
size_t xgl_size_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct);
size_t xgl_size_xgl_descriptor_type_count(const XGL_DESCRIPTOR_TYPE_COUNT* pStruct);
size_t xgl_size_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct);
size_t xgl_size_xgl_descriptor_region_create_info(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pStruct);
size_t xgl_size_xgl_viewport(const XGL_VIEWPORT* pStruct);
size_t xgl_size_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct);
size_t xgl_size_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct);
size_t xgl_size_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct);
size_t xgl_size_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct);
size_t xgl_size_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct);
size_t xgl_size_xgl_memory_barrier(const XGL_MEMORY_BARRIER* pStruct);
size_t xgl_size_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct);
size_t xgl_size_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct);
size_t xgl_size_xgl_clear_color(const XGL_CLEAR_COLOR* pStruct);
size_t xgl_size_xgl_dynamic_cb_state_create_info(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_buffer_view_create_info(const XGL_BUFFER_VIEW_CREATE_INFO* pStruct);
size_t xgl_size_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_event_wait_info(const XGL_EVENT_WAIT_INFO* pStruct);
size_t xgl_size_xgl_buffer_image_copy(const XGL_BUFFER_IMAGE_COPY* pStruct);
size_t xgl_size_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct);
size_t xgl_size_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct);
size_t xgl_size_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct);
size_t xgl_size_xgl_memory_alloc_buffer_info(const XGL_MEMORY_ALLOC_BUFFER_INFO* pStruct);
size_t xgl_size_xgl_buffer_copy(const XGL_BUFFER_COPY* pStruct);
size_t xgl_size_xgl_image_memory_bind_info(const XGL_IMAGE_MEMORY_BIND_INFO* pStruct);
size_t xgl_size_xgl_image_memory_requirements(const XGL_IMAGE_MEMORY_REQUIREMENTS* pStruct);
size_t xgl_size_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct);
size_t xgl_size_xgl_offset3d(const XGL_OFFSET3D* pStruct);
size_t xgl_size_xgl_buffer_view_attach_info(const XGL_BUFFER_VIEW_ATTACH_INFO* pStruct);
size_t xgl_size_xgl_extent3d(const XGL_EXTENT3D* pStruct);
size_t xgl_size_xgl_sampler_image_view_info(const XGL_SAMPLER_IMAGE_VIEW_INFO* pStruct);
size_t xgl_size_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct);
size_t xgl_size_xgl_layer_create_info(const XGL_LAYER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_vp_state_create_info(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_buffer_create_info(const XGL_BUFFER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_render_pass_create_info(const XGL_RENDER_PASS_CREATE_INFO* pStruct);
size_t xgl_size_xgl_descriptor_set_layout_create_info(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pStruct);
size_t xgl_size_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct);
size_t xgl_size_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct);
size_t xgl_size_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct);


size_t xgl_size_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_IA_STATE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_application_info(const XGL_APPLICATION_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_APPLICATION_INFO);
        structSize += sizeof(char)*(1+strlen(pStruct->pAppName));
        structSize += sizeof(char)*(1+strlen(pStruct->pEngineName));
    }
    return structSize;
}
size_t xgl_size_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_LINK_CONST_BUFFER);
        structSize += pStruct->bufferSize;
    }
    return structSize;
}
size_t xgl_size_xgl_update_samplers(const XGL_UPDATE_SAMPLERS* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_UPDATE_SAMPLERS);
        structSize += pStruct->count*sizeof(XGL_SAMPLER);
    }
    return structSize;
}
size_t xgl_size_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_STENCIL_OP_STATE);
    }
    return structSize;
}
size_t xgl_size_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PEER_IMAGE_OPEN_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DEVICE_CREATE_INFO);
        uint32_t i = 0;
        for (i = 0; i < pStruct->queueRecordCount; i++) {
            structSize += xgl_size_xgl_device_queue_create_info(&pStruct->pRequestedQueues[i]);
        }
        for (i = 0; i < pStruct->extensionCount; i++) {
            structSize += (sizeof(char*) + (sizeof(char) * (1 + strlen(pStruct->ppEnabledExtensionNames[i]))));
        }
    }
    return structSize;
}
size_t xgl_size_xgl_buffer_memory_requirements(const XGL_BUFFER_MEMORY_REQUIREMENTS* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_BUFFER_MEMORY_REQUIREMENTS);
    }
    return structSize;
}
size_t xgl_size_xgl_update_buffers(const XGL_UPDATE_BUFFERS* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_UPDATE_BUFFERS);
        uint32_t i = 0;
        for (i = 0; i < pStruct->count; i++) {
            structSize += (sizeof(XGL_BUFFER_VIEW_ATTACH_INFO*) + xgl_size_xgl_buffer_view_attach_info(pStruct->pBufferViews[i]));
        }
    }
    return structSize;
}
size_t xgl_size_xgl_vertex_input_attribute_description(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_vertex_input_create_info(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO);
        uint32_t i = 0;
        for (i = 0; i < pStruct->bindingCount; i++) {
            structSize += xgl_size_xgl_vertex_input_binding_description(&pStruct->pVertexBindingDescriptions[i]);
        }
        for (i = 0; i < pStruct->attributeCount; i++) {
            structSize += xgl_size_xgl_vertex_input_attribute_description(&pStruct->pVertexAttributeDescriptions[i]);
        }
    }
    return structSize;
}
size_t xgl_size_xgl_dynamic_rs_state_create_info(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DYNAMIC_RS_STATE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_SUBRESOURCE_LAYOUT);
    }
    return structSize;
}
size_t xgl_size_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DRAW_INDEXED_INDIRECT_CMD);
    }
    return structSize;
}
size_t xgl_size_xgl_dynamic_vp_state_create_info(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DYNAMIC_VP_STATE_CREATE_INFO);
        uint32_t i = 0;
        for (i = 0; i < pStruct->viewportAndScissorCount; i++) {
            structSize += xgl_size_xgl_viewport(&pStruct->pViewports[i]);
        }
        for (i = 0; i < pStruct->viewportAndScissorCount; i++) {
            structSize += xgl_size_xgl_rect(&pStruct->pScissors[i]);
        }
    }
    return structSize;
}
size_t xgl_size_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_SHADER_CREATE_INFO);
        structSize += pStruct->codeSize;
    }
    return structSize;
}
size_t xgl_size_xgl_memory_ref(const XGL_MEMORY_REF* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_MEMORY_REF);
    }
    return structSize;
}
size_t xgl_size_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_FORMAT_PROPERTIES);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_STATISTICS_DATA);
    }
    return structSize;
}
size_t xgl_size_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_ALLOC_CALLBACKS);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_CB_STATE_CREATE_INFO);
        uint32_t i = 0;
        for (i = 0; i < pStruct->attachmentCount; i++) {
            structSize += xgl_size_xgl_pipeline_cb_attachment_state(&pStruct->pAttachments[i]);
        }
    }
    return structSize;
}
size_t xgl_size_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DRAW_INDIRECT_CMD);
    }
    return structSize;
}
size_t xgl_size_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_CB_ATTACHMENT_STATE);
    }
    return structSize;
}
size_t xgl_size_xgl_vertex_input_binding_description(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_VERTEX_INPUT_BINDING_DESCRIPTION);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_SHADER_STAGE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_FENCE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_framebuffer_create_info(const XGL_FRAMEBUFFER_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_FRAMEBUFFER_CREATE_INFO);
        uint32_t i = 0;
        for (i = 0; i < pStruct->colorAttachmentCount; i++) {
            structSize += xgl_size_xgl_color_attachment_bind_info(&pStruct->pColorAttachments[i]);
        }
        structSize += xgl_size_xgl_depth_stencil_bind_info(pStruct->pDepthStencilAttachment);
    }
    return structSize;
}
size_t xgl_size_xgl_extent2d(const XGL_EXTENT2D* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_EXTENT2D);
    }
    return structSize;
}
size_t xgl_size_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_SUBRESOURCE_RANGE);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_TESS_STATE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_offset2d(const XGL_OFFSET2D* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_OFFSET2D);
    }
    return structSize;
}
size_t xgl_size_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_QUEUE_SEMAPHORE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_clear_color_value(const XGL_CLEAR_COLOR_VALUE* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_CLEAR_COLOR_VALUE);
    }
    return structSize;
}
size_t xgl_size_xgl_buffer_memory_barrier(const XGL_BUFFER_MEMORY_BARRIER* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_BUFFER_MEMORY_BARRIER);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_ms_state_create_info(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_MS_STATE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_RS_STATE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_update_images(const XGL_UPDATE_IMAGES* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_UPDATE_IMAGES);
        uint32_t i = 0;
        for (i = 0; i < pStruct->count; i++) {
            structSize += (sizeof(XGL_IMAGE_VIEW_ATTACH_INFO*) + xgl_size_xgl_image_view_attach_info(pStruct->pImageViews[i]));
        }
    }
    return structSize;
}
size_t xgl_size_xgl_cmd_buffer_begin_info(const XGL_CMD_BUFFER_BEGIN_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_CMD_BUFFER_BEGIN_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_VIEW_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_rect(const XGL_RECT* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_RECT);
    }
    return structSize;
}
size_t xgl_size_xgl_cmd_buffer_graphics_begin_info(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DEVICE_QUEUE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_MEMORY_OPEN_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_update_as_copy(const XGL_UPDATE_AS_COPY* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_UPDATE_AS_COPY);
    }
    return structSize;
}
size_t xgl_size_xgl_image_copy(const XGL_IMAGE_COPY* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_COPY);
    }
    return structSize;
}
size_t xgl_size_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_RESOLVE);
    }
    return structSize;
}
size_t xgl_size_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_dynamic_ds_state_create_info(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DYNAMIC_DS_STATE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_image_memory_barrier(const XGL_IMAGE_MEMORY_BARRIER* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_MEMORY_BARRIER);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_ds_state_create_info(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_DS_STATE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_barrier(const XGL_PIPELINE_BARRIER* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_BARRIER);
        structSize += pStruct->eventCount*sizeof(XGL_SET_EVENT);
        structSize += pStruct->memBarrierCount*(sizeof(void*) + sizeof(XGL_IMAGE_MEMORY_BARRIER));
    }
    return structSize;
}
size_t xgl_size_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PHYSICAL_GPU_MEMORY_PROPERTIES);
    }
    return structSize;
}
size_t xgl_size_xgl_memory_alloc_image_info(const XGL_MEMORY_ALLOC_IMAGE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_MEMORY_ALLOC_IMAGE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_update_sampler_textures(const XGL_UPDATE_SAMPLER_TEXTURES* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_UPDATE_SAMPLER_TEXTURES);
        uint32_t i = 0;
        for (i = 0; i < pStruct->count; i++) {
            structSize += xgl_size_xgl_sampler_image_view_info(&pStruct->pSamplerImageViews[i]);
        }
    }
    return structSize;
}
size_t xgl_size_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PEER_MEMORY_OPEN_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_descriptor_type_count(const XGL_DESCRIPTOR_TYPE_COUNT* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DESCRIPTOR_TYPE_COUNT);
    }
    return structSize;
}
size_t xgl_size_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DISPATCH_INDIRECT_CMD);
    }
    return structSize;
}
size_t xgl_size_xgl_descriptor_region_create_info(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DESCRIPTOR_REGION_CREATE_INFO);
        uint32_t i = 0;
        for (i = 0; i < pStruct->count; i++) {
            structSize += xgl_size_xgl_descriptor_type_count(&pStruct->pTypeCount[i]);
        }
    }
    return structSize;
}
size_t xgl_size_xgl_viewport(const XGL_VIEWPORT* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_VIEWPORT);
    }
    return structSize;
}
size_t xgl_size_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_QUERY_POOL_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_CMD_BUFFER_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DEPTH_STENCIL_BIND_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_MEMORY_REQUIREMENTS);
    }
    return structSize;
}
size_t xgl_size_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_QUEUE_SEMAPHORE_OPEN_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_memory_barrier(const XGL_MEMORY_BARRIER* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_MEMORY_BARRIER);
    }
    return structSize;
}
size_t xgl_size_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PHYSICAL_GPU_PERFORMANCE);
    }
    return structSize;
}
size_t xgl_size_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_CHANNEL_MAPPING);
    }
    return structSize;
}
size_t xgl_size_xgl_clear_color(const XGL_CLEAR_COLOR* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_CLEAR_COLOR);
    }
    return structSize;
}
size_t xgl_size_xgl_dynamic_cb_state_create_info(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DYNAMIC_CB_STATE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_buffer_view_create_info(const XGL_BUFFER_VIEW_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_BUFFER_VIEW_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_SAMPLER_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_event_wait_info(const XGL_EVENT_WAIT_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_EVENT_WAIT_INFO);
        structSize += pStruct->eventCount*sizeof(XGL_EVENT);
        structSize += pStruct->memBarrierCount*(sizeof(void*) + sizeof(XGL_IMAGE_MEMORY_BARRIER));
    }
    return structSize;
}
size_t xgl_size_xgl_buffer_image_copy(const XGL_BUFFER_IMAGE_COPY* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_BUFFER_IMAGE_COPY);
    }
    return structSize;
}
size_t xgl_size_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_EVENT_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PHYSICAL_GPU_PROPERTIES);
    }
    return structSize;
}
size_t xgl_size_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_VIEW_ATTACH_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_memory_alloc_buffer_info(const XGL_MEMORY_ALLOC_BUFFER_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_MEMORY_ALLOC_BUFFER_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_buffer_copy(const XGL_BUFFER_COPY* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_BUFFER_COPY);
    }
    return structSize;
}
size_t xgl_size_xgl_image_memory_bind_info(const XGL_IMAGE_MEMORY_BIND_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_MEMORY_BIND_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_image_memory_requirements(const XGL_IMAGE_MEMORY_REQUIREMENTS* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_SHADER);
        uint32_t i = 0;
        for (i = 0; i < pStruct->linkConstBufferCount; i++) {
            structSize += xgl_size_xgl_link_const_buffer(&pStruct->pLinkConstBufferInfo[i]);
        }
    }
    return structSize;
}
size_t xgl_size_xgl_offset3d(const XGL_OFFSET3D* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_OFFSET3D);
    }
    return structSize;
}
size_t xgl_size_xgl_buffer_view_attach_info(const XGL_BUFFER_VIEW_ATTACH_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_BUFFER_VIEW_ATTACH_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_extent3d(const XGL_EXTENT3D* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_EXTENT3D);
    }
    return structSize;
}
size_t xgl_size_xgl_sampler_image_view_info(const XGL_SAMPLER_IMAGE_VIEW_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_SAMPLER_IMAGE_VIEW_INFO);
        structSize += xgl_size_xgl_image_view_attach_info(pStruct->pImageView);
    }
    return structSize;
}
size_t xgl_size_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_IMAGE_SUBRESOURCE);
    }
    return structSize;
}
size_t xgl_size_xgl_layer_create_info(const XGL_LAYER_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_LAYER_CREATE_INFO);
        uint32_t i = 0;
        for (i = 0; i < pStruct->layerCount; i++) {
            structSize += (sizeof(char*) + (sizeof(char) * (1 + strlen(pStruct->ppActiveLayerNames[i]))));
        }
    }
    return structSize;
}
size_t xgl_size_xgl_pipeline_vp_state_create_info(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PIPELINE_VP_STATE_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_buffer_create_info(const XGL_BUFFER_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_BUFFER_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_render_pass_create_info(const XGL_RENDER_PASS_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_RENDER_PASS_CREATE_INFO);
        structSize += pStruct->colorAttachmentCount*sizeof(XGL_ATTACHMENT_LOAD_OP);
        structSize += pStruct->colorAttachmentCount*sizeof(XGL_ATTACHMENT_STORE_OP);
        uint32_t i = 0;
        for (i = 0; i < pStruct->colorAttachmentCount; i++) {
            structSize += xgl_size_xgl_clear_color(&pStruct->pColorLoadClearValues[i]);
        }
    }
    return structSize;
}
size_t xgl_size_xgl_descriptor_set_layout_create_info(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_GPU_COMPATIBILITY_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_MEMORY_ALLOC_INFO);
    }
    return structSize;
}
size_t xgl_size_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct)
{
    size_t structSize = 0;
    if (pStruct) {
        structSize = sizeof(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES);
    }
    return structSize;
}
size_t get_struct_chain_size(const void* pStruct)
{
    // Just use XGL_APPLICATION_INFO as struct until actual type is resolved
    XGL_APPLICATION_INFO* pNext = (XGL_APPLICATION_INFO*)pStruct;
    size_t structSize = 0;
    while (pNext) {
        switch (pNext->sType) {
            case XGL_STRUCTURE_TYPE_APPLICATION_INFO:
            {
                structSize += xgl_size_xgl_application_info((XGL_APPLICATION_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
            {
                structSize += xgl_size_xgl_buffer_create_info((XGL_BUFFER_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
            {
                structSize += xgl_size_xgl_buffer_memory_barrier((XGL_BUFFER_MEMORY_BARRIER*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO:
            {
                structSize += xgl_size_xgl_buffer_view_attach_info((XGL_BUFFER_VIEW_ATTACH_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
            {
                structSize += xgl_size_xgl_buffer_view_create_info((XGL_BUFFER_VIEW_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO:
            {
                structSize += xgl_size_xgl_cmd_buffer_begin_info((XGL_CMD_BUFFER_BEGIN_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO:
            {
                structSize += xgl_size_xgl_cmd_buffer_create_info((XGL_CMD_BUFFER_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
            {
                structSize += xgl_size_xgl_cmd_buffer_graphics_begin_info((XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO:
            {
                structSize += xgl_size_xgl_color_attachment_view_create_info((XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_compute_pipeline_create_info((XGL_COMPUTE_PIPELINE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO:
            {
                structSize += xgl_size_xgl_depth_stencil_view_create_info((XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO:
            {
                structSize += xgl_size_xgl_descriptor_region_create_info((XGL_DESCRIPTOR_REGION_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
            {
                structSize += xgl_size_xgl_descriptor_set_layout_create_info((XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_device_create_info((XGL_DEVICE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_dynamic_cb_state_create_info((XGL_DYNAMIC_CB_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_dynamic_ds_state_create_info((XGL_DYNAMIC_DS_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_dynamic_rs_state_create_info((XGL_DYNAMIC_RS_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_dynamic_vp_state_create_info((XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
            {
                structSize += xgl_size_xgl_event_create_info((XGL_EVENT_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_EVENT_WAIT_INFO:
            {
                structSize += xgl_size_xgl_event_wait_info((XGL_EVENT_WAIT_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_fence_create_info((XGL_FENCE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
            {
                structSize += xgl_size_xgl_framebuffer_create_info((XGL_FRAMEBUFFER_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_graphics_pipeline_create_info((XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_image_create_info((XGL_IMAGE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
            {
                structSize += xgl_size_xgl_image_memory_barrier((XGL_IMAGE_MEMORY_BARRIER*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO:
            {
                structSize += xgl_size_xgl_image_view_attach_info((XGL_IMAGE_VIEW_ATTACH_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
            {
                structSize += xgl_size_xgl_image_view_create_info((XGL_IMAGE_VIEW_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO:
            {
                structSize += xgl_size_xgl_layer_create_info((XGL_LAYER_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
            {
                structSize += xgl_size_xgl_memory_alloc_buffer_info((XGL_MEMORY_ALLOC_BUFFER_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
            {
                structSize += xgl_size_xgl_memory_alloc_image_info((XGL_MEMORY_ALLOC_IMAGE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO:
            {
                structSize += xgl_size_xgl_memory_alloc_info((XGL_MEMORY_ALLOC_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:
            {
                structSize += xgl_size_xgl_memory_barrier((XGL_MEMORY_BARRIER*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO:
            {
                structSize += xgl_size_xgl_memory_open_info((XGL_MEMORY_OPEN_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO:
            {
                structSize += xgl_size_xgl_peer_memory_open_info((XGL_PEER_MEMORY_OPEN_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_BARRIER:
            {
                structSize += xgl_size_xgl_pipeline_barrier((XGL_PIPELINE_BARRIER*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_pipeline_cb_state_create_info((XGL_PIPELINE_CB_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_pipeline_ds_state_create_info((XGL_PIPELINE_DS_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_pipeline_ia_state_create_info((XGL_PIPELINE_IA_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_pipeline_ms_state_create_info((XGL_PIPELINE_MS_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_pipeline_rs_state_create_info((XGL_PIPELINE_RS_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_pipeline_shader_stage_create_info((XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_pipeline_tess_state_create_info((XGL_PIPELINE_TESS_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
            {
                structSize += xgl_size_xgl_pipeline_vertex_input_create_info((XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_pipeline_vp_state_create_info((XGL_PIPELINE_VP_STATE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
            {
                structSize += xgl_size_xgl_query_pool_create_info((XGL_QUERY_POOL_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_CREATE_INFO:
            {
                structSize += xgl_size_xgl_queue_semaphore_create_info((XGL_QUEUE_SEMAPHORE_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_OPEN_INFO:
            {
                structSize += xgl_size_xgl_queue_semaphore_open_info((XGL_QUEUE_SEMAPHORE_OPEN_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
            {
                structSize += xgl_size_xgl_render_pass_create_info((XGL_RENDER_PASS_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
            {
                structSize += xgl_size_xgl_sampler_create_info((XGL_SAMPLER_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO:
            {
                structSize += xgl_size_xgl_shader_create_info((XGL_SHADER_CREATE_INFO*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
            {
                structSize += xgl_size_xgl_update_as_copy((XGL_UPDATE_AS_COPY*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            {
                structSize += xgl_size_xgl_update_buffers((XGL_UPDATE_BUFFERS*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            {
                structSize += xgl_size_xgl_update_images((XGL_UPDATE_IMAGES*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            {
                structSize += xgl_size_xgl_update_samplers((XGL_UPDATE_SAMPLERS*)pNext);
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            {
                structSize += xgl_size_xgl_update_sampler_textures((XGL_UPDATE_SAMPLER_TEXTURES*)pNext);
                break;
            }
            default:
                assert(0);
                structSize += 0;
        }
        pNext = (XGL_APPLICATION_INFO*)pNext->pNext;
    }
    return structSize;
}
size_t get_dynamic_struct_size(const void* pStruct)
{
    // Just use XGL_APPLICATION_INFO as struct until actual type is resolved
    XGL_APPLICATION_INFO* pNext = (XGL_APPLICATION_INFO*)pStruct;
    size_t structSize = 0;
    switch (pNext->sType) {
        case XGL_STRUCTURE_TYPE_APPLICATION_INFO:
        {
            structSize += xgl_size_xgl_application_info((XGL_APPLICATION_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
        {
            structSize += xgl_size_xgl_buffer_create_info((XGL_BUFFER_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
        {
            structSize += xgl_size_xgl_buffer_memory_barrier((XGL_BUFFER_MEMORY_BARRIER*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO:
        {
            structSize += xgl_size_xgl_buffer_view_attach_info((XGL_BUFFER_VIEW_ATTACH_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
        {
            structSize += xgl_size_xgl_buffer_view_create_info((XGL_BUFFER_VIEW_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO:
        {
            structSize += xgl_size_xgl_cmd_buffer_begin_info((XGL_CMD_BUFFER_BEGIN_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO:
        {
            structSize += xgl_size_xgl_cmd_buffer_create_info((XGL_CMD_BUFFER_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
        {
            structSize += xgl_size_xgl_cmd_buffer_graphics_begin_info((XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO:
        {
            structSize += xgl_size_xgl_color_attachment_view_create_info((XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_compute_pipeline_create_info((XGL_COMPUTE_PIPELINE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO:
        {
            structSize += xgl_size_xgl_depth_stencil_view_create_info((XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO:
        {
            structSize += xgl_size_xgl_descriptor_region_create_info((XGL_DESCRIPTOR_REGION_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
        {
            structSize += xgl_size_xgl_descriptor_set_layout_create_info((XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_device_create_info((XGL_DEVICE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_dynamic_cb_state_create_info((XGL_DYNAMIC_CB_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_dynamic_ds_state_create_info((XGL_DYNAMIC_DS_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_dynamic_rs_state_create_info((XGL_DYNAMIC_RS_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_dynamic_vp_state_create_info((XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
        {
            structSize += xgl_size_xgl_event_create_info((XGL_EVENT_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_EVENT_WAIT_INFO:
        {
            structSize += xgl_size_xgl_event_wait_info((XGL_EVENT_WAIT_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_fence_create_info((XGL_FENCE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
        {
            structSize += xgl_size_xgl_framebuffer_create_info((XGL_FRAMEBUFFER_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_graphics_pipeline_create_info((XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_image_create_info((XGL_IMAGE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
        {
            structSize += xgl_size_xgl_image_memory_barrier((XGL_IMAGE_MEMORY_BARRIER*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO:
        {
            structSize += xgl_size_xgl_image_view_attach_info((XGL_IMAGE_VIEW_ATTACH_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
        {
            structSize += xgl_size_xgl_image_view_create_info((XGL_IMAGE_VIEW_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO:
        {
            structSize += xgl_size_xgl_layer_create_info((XGL_LAYER_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
        {
            structSize += xgl_size_xgl_memory_alloc_buffer_info((XGL_MEMORY_ALLOC_BUFFER_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
        {
            structSize += xgl_size_xgl_memory_alloc_image_info((XGL_MEMORY_ALLOC_IMAGE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO:
        {
            structSize += xgl_size_xgl_memory_alloc_info((XGL_MEMORY_ALLOC_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:
        {
            structSize += xgl_size_xgl_memory_barrier((XGL_MEMORY_BARRIER*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO:
        {
            structSize += xgl_size_xgl_memory_open_info((XGL_MEMORY_OPEN_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO:
        {
            structSize += xgl_size_xgl_peer_memory_open_info((XGL_PEER_MEMORY_OPEN_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_BARRIER:
        {
            structSize += xgl_size_xgl_pipeline_barrier((XGL_PIPELINE_BARRIER*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_pipeline_cb_state_create_info((XGL_PIPELINE_CB_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_pipeline_ds_state_create_info((XGL_PIPELINE_DS_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_pipeline_ia_state_create_info((XGL_PIPELINE_IA_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_pipeline_ms_state_create_info((XGL_PIPELINE_MS_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_pipeline_rs_state_create_info((XGL_PIPELINE_RS_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_pipeline_shader_stage_create_info((XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_pipeline_tess_state_create_info((XGL_PIPELINE_TESS_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
        {
            structSize += xgl_size_xgl_pipeline_vertex_input_create_info((XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_pipeline_vp_state_create_info((XGL_PIPELINE_VP_STATE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
        {
            structSize += xgl_size_xgl_query_pool_create_info((XGL_QUERY_POOL_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_CREATE_INFO:
        {
            structSize += xgl_size_xgl_queue_semaphore_create_info((XGL_QUEUE_SEMAPHORE_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_OPEN_INFO:
        {
            structSize += xgl_size_xgl_queue_semaphore_open_info((XGL_QUEUE_SEMAPHORE_OPEN_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
        {
            structSize += xgl_size_xgl_render_pass_create_info((XGL_RENDER_PASS_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
        {
            structSize += xgl_size_xgl_sampler_create_info((XGL_SAMPLER_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO:
        {
            structSize += xgl_size_xgl_shader_create_info((XGL_SHADER_CREATE_INFO*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
        {
            structSize += xgl_size_xgl_update_as_copy((XGL_UPDATE_AS_COPY*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
        {
            structSize += xgl_size_xgl_update_buffers((XGL_UPDATE_BUFFERS*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
        {
            structSize += xgl_size_xgl_update_images((XGL_UPDATE_IMAGES*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
        {
            structSize += xgl_size_xgl_update_samplers((XGL_UPDATE_SAMPLERS*)pNext);
            break;
        }
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
        {
            structSize += xgl_size_xgl_update_sampler_textures((XGL_UPDATE_SAMPLER_TEXTURES*)pNext);
            break;
        }
        default:
            assert(0);
            structSize += 0;
    }
    return structSize;
}