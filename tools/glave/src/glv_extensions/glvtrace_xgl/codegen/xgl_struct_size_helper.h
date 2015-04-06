/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * XGL
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
//#includes, #defines, globals and such...
#include <stdio.h>
#include <stdlib.h>
#include <xgl.h>

// Function Prototypes
size_t get_struct_chain_size(const void* pStruct);
size_t get_dynamic_struct_size(const void* pStruct);
size_t xgl_size_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct);
size_t xgl_size_xgl_application_info(const XGL_APPLICATION_INFO* pStruct);
size_t xgl_size_xgl_buffer_copy(const XGL_BUFFER_COPY* pStruct);
size_t xgl_size_xgl_buffer_create_info(const XGL_BUFFER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_buffer_image_copy(const XGL_BUFFER_IMAGE_COPY* pStruct);
size_t xgl_size_xgl_buffer_memory_barrier(const XGL_BUFFER_MEMORY_BARRIER* pStruct);
size_t xgl_size_xgl_buffer_memory_requirements(const XGL_BUFFER_MEMORY_REQUIREMENTS* pStruct);
size_t xgl_size_xgl_buffer_view_attach_info(const XGL_BUFFER_VIEW_ATTACH_INFO* pStruct);
size_t xgl_size_xgl_buffer_view_create_info(const XGL_BUFFER_VIEW_CREATE_INFO* pStruct);
size_t xgl_size_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct);
size_t xgl_size_xgl_clear_color(const XGL_CLEAR_COLOR* pStruct);
size_t xgl_size_xgl_clear_color_value(const XGL_CLEAR_COLOR_VALUE* pStruct);
size_t xgl_size_xgl_cmd_buffer_begin_info(const XGL_CMD_BUFFER_BEGIN_INFO* pStruct);
size_t xgl_size_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_cmd_buffer_graphics_begin_info(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pStruct);
size_t xgl_size_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct);
size_t xgl_size_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct);
size_t xgl_size_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct);
size_t xgl_size_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct);
size_t xgl_size_xgl_descriptor_region_create_info(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pStruct);
size_t xgl_size_xgl_descriptor_set_layout_create_info(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pStruct);
size_t xgl_size_xgl_descriptor_type_count(const XGL_DESCRIPTOR_TYPE_COUNT* pStruct);
size_t xgl_size_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct);
size_t xgl_size_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct);
size_t xgl_size_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct);
size_t xgl_size_xgl_dynamic_cb_state_create_info(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_dynamic_ds_state_create_info(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_dynamic_rs_state_create_info(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_dynamic_vp_state_create_info(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct);
size_t xgl_size_xgl_event_wait_info(const XGL_EVENT_WAIT_INFO* pStruct);
size_t xgl_size_xgl_extent2d(const XGL_EXTENT2D* pStruct);
size_t xgl_size_xgl_extent3d(const XGL_EXTENT3D* pStruct);
size_t xgl_size_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct);
size_t xgl_size_xgl_framebuffer_create_info(const XGL_FRAMEBUFFER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct);
size_t xgl_size_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_image_copy(const XGL_IMAGE_COPY* pStruct);
size_t xgl_size_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_image_memory_barrier(const XGL_IMAGE_MEMORY_BARRIER* pStruct);
size_t xgl_size_xgl_image_memory_bind_info(const XGL_IMAGE_MEMORY_BIND_INFO* pStruct);
size_t xgl_size_xgl_image_memory_requirements(const XGL_IMAGE_MEMORY_REQUIREMENTS* pStruct);
size_t xgl_size_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct);
size_t xgl_size_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct);
size_t xgl_size_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct);
size_t xgl_size_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct);
size_t xgl_size_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct);
size_t xgl_size_xgl_layer_create_info(const XGL_LAYER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct);
size_t xgl_size_xgl_memory_alloc_buffer_info(const XGL_MEMORY_ALLOC_BUFFER_INFO* pStruct);
size_t xgl_size_xgl_memory_alloc_image_info(const XGL_MEMORY_ALLOC_IMAGE_INFO* pStruct);
size_t xgl_size_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct);
size_t xgl_size_xgl_memory_barrier(const XGL_MEMORY_BARRIER* pStruct);
size_t xgl_size_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct);
size_t xgl_size_xgl_memory_ref(const XGL_MEMORY_REF* pStruct);
size_t xgl_size_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct);
size_t xgl_size_xgl_offset2d(const XGL_OFFSET2D* pStruct);
size_t xgl_size_xgl_offset3d(const XGL_OFFSET3D* pStruct);
size_t xgl_size_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct);
size_t xgl_size_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct);
size_t xgl_size_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct);
size_t xgl_size_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct);
size_t xgl_size_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct);
size_t xgl_size_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct);
size_t xgl_size_xgl_pipeline_barrier(const XGL_PIPELINE_BARRIER* pStruct);
size_t xgl_size_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct);
size_t xgl_size_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_ds_state_create_info(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_ms_state_create_info(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct);
size_t xgl_size_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct);
size_t xgl_size_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_vertex_input_create_info(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pStruct);
size_t xgl_size_xgl_pipeline_vp_state_create_info(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct);
size_t xgl_size_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct);
size_t xgl_size_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct);
size_t xgl_size_xgl_rect(const XGL_RECT* pStruct);
size_t xgl_size_xgl_render_pass_create_info(const XGL_RENDER_PASS_CREATE_INFO* pStruct);
size_t xgl_size_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_sampler_image_view_info(const XGL_SAMPLER_IMAGE_VIEW_INFO* pStruct);
size_t xgl_size_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct);
size_t xgl_size_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct);
size_t xgl_size_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct);
size_t xgl_size_xgl_update_as_copy(const XGL_UPDATE_AS_COPY* pStruct);
size_t xgl_size_xgl_update_buffers(const XGL_UPDATE_BUFFERS* pStruct);
size_t xgl_size_xgl_update_images(const XGL_UPDATE_IMAGES* pStruct);
size_t xgl_size_xgl_update_samplers(const XGL_UPDATE_SAMPLERS* pStruct);
size_t xgl_size_xgl_update_sampler_textures(const XGL_UPDATE_SAMPLER_TEXTURES* pStruct);
size_t xgl_size_xgl_vertex_input_attribute_description(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pStruct);
size_t xgl_size_xgl_vertex_input_binding_description(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pStruct);
size_t xgl_size_xgl_viewport(const XGL_VIEWPORT* pStruct);