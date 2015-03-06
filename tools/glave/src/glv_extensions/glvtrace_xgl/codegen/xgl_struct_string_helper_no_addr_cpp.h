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
#include <xgl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include "xgl_enum_string_helper.h"
using namespace std;

// Function Prototypes
string dynamic_display(const void* pStruct, const string prefix);
string xgl_print_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct, const string prefix);
string xgl_print_xgl_application_info(const XGL_APPLICATION_INFO* pStruct, const string prefix);
string xgl_print_xgl_buffer_copy(const XGL_BUFFER_COPY* pStruct, const string prefix);
string xgl_print_xgl_buffer_create_info(const XGL_BUFFER_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_buffer_image_copy(const XGL_BUFFER_IMAGE_COPY* pStruct, const string prefix);
string xgl_print_xgl_buffer_memory_barrier(const XGL_BUFFER_MEMORY_BARRIER* pStruct, const string prefix);
string xgl_print_xgl_buffer_memory_requirements(const XGL_BUFFER_MEMORY_REQUIREMENTS* pStruct, const string prefix);
string xgl_print_xgl_buffer_view_attach_info(const XGL_BUFFER_VIEW_ATTACH_INFO* pStruct, const string prefix);
string xgl_print_xgl_buffer_view_create_info(const XGL_BUFFER_VIEW_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct, const string prefix);
string xgl_print_xgl_clear_color(const XGL_CLEAR_COLOR* pStruct, const string prefix);
string xgl_print_xgl_clear_color_value(const XGL_CLEAR_COLOR_VALUE* pStruct, const string prefix);
string xgl_print_xgl_cmd_buffer_begin_info(const XGL_CMD_BUFFER_BEGIN_INFO* pStruct, const string prefix);
string xgl_print_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_cmd_buffer_graphics_begin_info(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pStruct, const string prefix);
string xgl_print_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct, const string prefix);
string xgl_print_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct, const string prefix);
string xgl_print_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_descriptor_region_create_info(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_descriptor_set_layout_create_info(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_descriptor_type_count(const XGL_DESCRIPTOR_TYPE_COUNT* pStruct, const string prefix);
string xgl_print_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct, const string prefix);
string xgl_print_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct, const string prefix);
string xgl_print_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct, const string prefix);
string xgl_print_xgl_dynamic_cb_state_create_info(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_dynamic_ds_state_create_info(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_dynamic_rs_state_create_info(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_dynamic_vp_state_create_info(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_event_wait_info(const XGL_EVENT_WAIT_INFO* pStruct, const string prefix);
string xgl_print_xgl_extent2d(const XGL_EXTENT2D* pStruct, const string prefix);
string xgl_print_xgl_extent3d(const XGL_EXTENT3D* pStruct, const string prefix);
string xgl_print_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct, const string prefix);
string xgl_print_xgl_framebuffer_create_info(const XGL_FRAMEBUFFER_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct, const string prefix);
string xgl_print_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_image_copy(const XGL_IMAGE_COPY* pStruct, const string prefix);
string xgl_print_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_image_memory_barrier(const XGL_IMAGE_MEMORY_BARRIER* pStruct, const string prefix);
string xgl_print_xgl_image_memory_bind_info(const XGL_IMAGE_MEMORY_BIND_INFO* pStruct, const string prefix);
string xgl_print_xgl_image_memory_requirements(const XGL_IMAGE_MEMORY_REQUIREMENTS* pStruct, const string prefix);
string xgl_print_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct, const string prefix);
string xgl_print_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct, const string prefix);
string xgl_print_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct, const string prefix);
string xgl_print_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct, const string prefix);
string xgl_print_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_layer_create_info(const XGL_LAYER_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct, const string prefix);
string xgl_print_xgl_memory_alloc_buffer_info(const XGL_MEMORY_ALLOC_BUFFER_INFO* pStruct, const string prefix);
string xgl_print_xgl_memory_alloc_image_info(const XGL_MEMORY_ALLOC_IMAGE_INFO* pStruct, const string prefix);
string xgl_print_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct, const string prefix);
string xgl_print_xgl_memory_barrier(const XGL_MEMORY_BARRIER* pStruct, const string prefix);
string xgl_print_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct, const string prefix);
string xgl_print_xgl_memory_ref(const XGL_MEMORY_REF* pStruct, const string prefix);
string xgl_print_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct, const string prefix);
string xgl_print_xgl_offset2d(const XGL_OFFSET2D* pStruct, const string prefix);
string xgl_print_xgl_offset3d(const XGL_OFFSET3D* pStruct, const string prefix);
string xgl_print_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct, const string prefix);
string xgl_print_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct, const string prefix);
string xgl_print_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct, const string prefix);
string xgl_print_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct, const string prefix);
string xgl_print_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct, const string prefix);
string xgl_print_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct, const string prefix);
string xgl_print_xgl_pipeline_barrier(const XGL_PIPELINE_BARRIER* pStruct, const string prefix);
string xgl_print_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct, const string prefix);
string xgl_print_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_pipeline_ds_state_create_info(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_pipeline_ms_state_create_info(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct, const string prefix);
string xgl_print_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct, const string prefix);
string xgl_print_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_pipeline_vertex_input_create_info(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_pipeline_vp_state_create_info(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct, const string prefix);
string xgl_print_xgl_rect(const XGL_RECT* pStruct, const string prefix);
string xgl_print_xgl_render_pass_create_info(const XGL_RENDER_PASS_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_sampler_image_view_info(const XGL_SAMPLER_IMAGE_VIEW_INFO* pStruct, const string prefix);
string xgl_print_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct, const string prefix);
string xgl_print_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct, const string prefix);
string xgl_print_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct, const string prefix);
string xgl_print_xgl_update_as_copy(const XGL_UPDATE_AS_COPY* pStruct, const string prefix);
string xgl_print_xgl_update_buffers(const XGL_UPDATE_BUFFERS* pStruct, const string prefix);
string xgl_print_xgl_update_images(const XGL_UPDATE_IMAGES* pStruct, const string prefix);
string xgl_print_xgl_update_samplers(const XGL_UPDATE_SAMPLERS* pStruct, const string prefix);
string xgl_print_xgl_update_sampler_textures(const XGL_UPDATE_SAMPLER_TEXTURES* pStruct, const string prefix);
string xgl_print_xgl_vertex_input_attribute_description(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pStruct, const string prefix);
string xgl_print_xgl_vertex_input_binding_description(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pStruct, const string prefix);
string xgl_print_xgl_viewport(const XGL_VIEWPORT* pStruct, const string prefix);


string xgl_print_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    final_str = prefix + "pUserData = " + ss[0].str() + "\n" + prefix + "pfnAlloc = " + ss[1].str() + "\n" + prefix + "pfnFree = " + ss[2].str() + "\n";
    return final_str;
}
string xgl_print_xgl_application_info(const XGL_APPLICATION_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2] << pStruct->appVersion;
    ss[3].str("addr");
    ss[4] << pStruct->engineVersion;
    ss[5] << pStruct->apiVersion;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "pAppName = " + ss[1].str() + "\n" + prefix + "appVersion = " + ss[2].str() + "\n" + prefix + "pEngineName = " + ss[3].str() + "\n" + prefix + "engineVersion = " + ss[4].str() + "\n" + prefix + "apiVersion = " + ss[5].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_buffer_copy(const XGL_BUFFER_COPY* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    final_str = prefix + "srcOffset = " + ss[0].str() + "\n" + prefix + "destOffset = " + ss[1].str() + "\n" + prefix + "copySize = " + ss[2].str() + "\n";
    return final_str;
}
string xgl_print_xgl_buffer_create_info(const XGL_BUFFER_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2] << pStruct->usage;
    ss[3] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "size = " + ss[1].str() + "\n" + prefix + "usage = " + ss[2].str() + "\n" + prefix + "flags = " + ss[3].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_buffer_image_copy(const XGL_BUFFER_IMAGE_COPY* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[3];
    tmp_str = xgl_print_xgl_image_subresource(&pStruct->imageSubresource, extra_indent);
    ss[0] << &pStruct->imageSubresource;
    stp_strs[0] = " " + prefix + "imageSubresource (addr)\n" + tmp_str;
    ss[0].str("");
    tmp_str = xgl_print_xgl_offset3d(&pStruct->imageOffset, extra_indent);
    ss[1] << &pStruct->imageOffset;
    stp_strs[1] = " " + prefix + "imageOffset (addr)\n" + tmp_str;
    ss[1].str("");
    tmp_str = xgl_print_xgl_extent3d(&pStruct->imageExtent, extra_indent);
    ss[2] << &pStruct->imageExtent;
    stp_strs[2] = " " + prefix + "imageExtent (addr)\n" + tmp_str;
    ss[2].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    ss[3].str("addr");
    final_str = prefix + "bufferOffset = " + ss[0].str() + "\n" + prefix + "imageSubresource = " + ss[1].str() + "\n" + prefix + "imageOffset = " + ss[2].str() + "\n" + prefix + "imageExtent = " + ss[3].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_buffer_memory_barrier(const XGL_BUFFER_MEMORY_BARRIER* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->outputMask;
    ss[2] << pStruct->inputMask;
    ss[3].str("addr");
    ss[4].str("addr");
    ss[5].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "outputMask = " + ss[1].str() + "\n" + prefix + "inputMask = " + ss[2].str() + "\n" + prefix + "buffer = " + ss[3].str() + "\n" + prefix + "offset = " + ss[4].str() + "\n" + prefix + "size = " + ss[5].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_buffer_memory_requirements(const XGL_BUFFER_MEMORY_REQUIREMENTS* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
    ss[0] << pStruct->usage;
    final_str = prefix + "usage = " + ss[0].str() + "\n";
    return final_str;
}
string xgl_print_xgl_buffer_view_attach_info(const XGL_BUFFER_VIEW_ATTACH_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "view = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_buffer_view_create_info(const XGL_BUFFER_VIEW_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_channel_mapping(&pStruct->channels, extra_indent);
    ss[1] << &pStruct->channels;
    stp_strs[1] = " " + prefix + "channels (addr)\n" + tmp_str;
    ss[1].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    ss[3].str("addr");
    ss[4].str("addr");
    ss[5].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "buffer = " + ss[1].str() + "\n" + prefix + "viewType = " + string_XGL_BUFFER_VIEW_TYPE(pStruct->viewType) + "\n" + prefix + "stride = " + ss[2].str() + "\n" + prefix + "format = " + string_XGL_FORMAT(pStruct->format) + "\n" + prefix + "channels = " + ss[3].str() + "\n" + prefix + "offset = " + ss[4].str() + "\n" + prefix + "range = " + ss[5].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    final_str = prefix + "r = " + string_XGL_CHANNEL_SWIZZLE(pStruct->r) + "\n" + prefix + "g = " + string_XGL_CHANNEL_SWIZZLE(pStruct->g) + "\n" + prefix + "b = " + string_XGL_CHANNEL_SWIZZLE(pStruct->b) + "\n" + prefix + "a = " + string_XGL_CHANNEL_SWIZZLE(pStruct->a) + "\n";
    return final_str;
}
string xgl_print_xgl_clear_color(const XGL_CLEAR_COLOR* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    tmp_str = xgl_print_xgl_clear_color_value(&pStruct->color, extra_indent);
    ss[0] << &pStruct->color;
    stp_strs[0] = " " + prefix + "color (addr)\n" + tmp_str;
    ss[0].str("");
    ss[0].str("addr");
    ss[1].str(pStruct->useRawValue ? "TRUE" : "FALSE");
    final_str = prefix + "color = " + ss[0].str() + "\n" + prefix + "useRawValue = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_clear_color_value(const XGL_CLEAR_COLOR_VALUE* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[2];
    stp_strs[0] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < 4; i++) {
        index_ss.str("");
        index_ss << i;
        ss[0] << pStruct->floatColor[i];
        stp_strs[0] += " " + prefix + "floatColor[" + index_ss.str() + "] = " + ss[0].str() + "\n";
        ss[0].str("");
    }
    stp_strs[1] = "";
    for (uint32_t i = 0; i < 4; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << pStruct->rawColor[i];
        stp_strs[1] += " " + prefix + "rawColor[" + index_ss.str() + "] = " + ss[1].str() + "\n";
        ss[1].str("");
    }
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "floatColor = " + ss[0].str() + "\n" + prefix + "rawColor = " + ss[1].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_cmd_buffer_begin_info(const XGL_CMD_BUFFER_BEGIN_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "flags = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->queueNodeIndex;
    ss[2] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "queueNodeIndex = " + ss[1].str() + "\n" + prefix + "flags = " + ss[2].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_cmd_buffer_graphics_begin_info(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "renderPass = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
    ss[0].str("addr");
    final_str = prefix + "view = " + ss[0].str() + "\n" + prefix + "layout = " + string_XGL_IMAGE_LAYOUT(pStruct->layout) + "\n";
    return final_str;
}
string xgl_print_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_image_subresource_range(&pStruct->msaaResolveSubResource, extra_indent);
    ss[1] << &pStruct->msaaResolveSubResource;
    stp_strs[1] = " " + prefix + "msaaResolveSubResource (addr)\n" + tmp_str;
    ss[1].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2] << pStruct->mipLevel;
    ss[3] << pStruct->baseArraySlice;
    ss[4] << pStruct->arraySize;
    ss[5].str("addr");
    ss[6].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "image = " + ss[1].str() + "\n" + prefix + "format = " + string_XGL_FORMAT(pStruct->format) + "\n" + prefix + "mipLevel = " + ss[2].str() + "\n" + prefix + "baseArraySlice = " + ss[3].str() + "\n" + prefix + "arraySize = " + ss[4].str() + "\n" + prefix + "msaaResolveImage = " + ss[5].str() + "\n" + prefix + "msaaResolveSubResource = " + ss[6].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_pipeline_shader(&pStruct->cs, extra_indent);
    ss[1] << &pStruct->cs;
    stp_strs[1] = " " + prefix + "cs (addr)\n" + tmp_str;
    ss[1].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2] << pStruct->flags;
    ss[3].str("addr");
    ss[4] << pStruct->localSizeX;
    ss[5] << pStruct->localSizeY;
    ss[6] << pStruct->localSizeZ;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "cs = " + ss[1].str() + "\n" + prefix + "flags = " + ss[2].str() + "\n" + prefix + "lastSetLayout = " + ss[3].str() + "\n" + prefix + "localSizeX = " + ss[4].str() + "\n" + prefix + "localSizeY = " + ss[5].str() + "\n" + prefix + "localSizeZ = " + ss[6].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
    ss[0].str("addr");
    final_str = prefix + "view = " + ss[0].str() + "\n" + prefix + "layout = " + string_XGL_IMAGE_LAYOUT(pStruct->layout) + "\n";
    return final_str;
}
string xgl_print_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_image_subresource_range(&pStruct->msaaResolveSubResource, extra_indent);
    ss[1] << &pStruct->msaaResolveSubResource;
    stp_strs[1] = " " + prefix + "msaaResolveSubResource (addr)\n" + tmp_str;
    ss[1].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2] << pStruct->mipLevel;
    ss[3] << pStruct->baseArraySlice;
    ss[4] << pStruct->arraySize;
    ss[5].str("addr");
    ss[6].str("addr");
    ss[7] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "image = " + ss[1].str() + "\n" + prefix + "mipLevel = " + ss[2].str() + "\n" + prefix + "baseArraySlice = " + ss[3].str() + "\n" + prefix + "arraySize = " + ss[4].str() + "\n" + prefix + "msaaResolveImage = " + ss[5].str() + "\n" + prefix + "msaaResolveSubResource = " + ss[6].str() + "\n" + prefix + "flags = " + ss[7].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_descriptor_region_create_info(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->count; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << &pStruct->pTypeCount[i];
        tmp_str = xgl_print_xgl_descriptor_type_count(&pStruct->pTypeCount[i], extra_indent);
        stp_strs[1] += " " + prefix + "pTypeCount[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[1].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->count;
    ss[2].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "count = " + ss[1].str() + "\n" + prefix + "pTypeCount = " + ss[2].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_descriptor_set_layout_create_info(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->count;
    ss[2] << pStruct->stageFlags;
    ss[3].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "descriptorType = " + string_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType) + "\n" + prefix + "count = " + ss[1].str() + "\n" + prefix + "stageFlags = " + ss[2].str() + "\n" + prefix + "immutableSampler = " + ss[3].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_descriptor_type_count(const XGL_DESCRIPTOR_TYPE_COUNT* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
    ss[0] << pStruct->count;
    final_str = prefix + "type = " + string_XGL_DESCRIPTOR_TYPE(pStruct->type) + "\n" + prefix + "count = " + ss[0].str() + "\n";
    return final_str;
}
string xgl_print_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[3];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->queueRecordCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << &pStruct->pRequestedQueues[i];
        tmp_str = xgl_print_xgl_device_queue_create_info(&pStruct->pRequestedQueues[i], extra_indent);
        stp_strs[1] += " " + prefix + "pRequestedQueues[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[1].str("");
    }
    stp_strs[2] = "";
    for (uint32_t i = 0; i < pStruct->extensionCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[2] << pStruct->ppEnabledExtensionNames[i];
        stp_strs[2] += " " + prefix + "ppEnabledExtensionNames[" + index_ss.str() + "] = " + ss[2].str() + "\n";
        ss[2].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->queueRecordCount;
    ss[2].str("addr");
    ss[3] << pStruct->extensionCount;
    ss[4] << pStruct->ppEnabledExtensionNames;
    ss[5] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "queueRecordCount = " + ss[1].str() + "\n" + prefix + "pRequestedQueues = " + ss[2].str() + "\n" + prefix + "extensionCount = " + ss[3].str() + "\n" + prefix + "ppEnabledExtensionNames = " + ss[4].str() + "\n" + prefix + "maxValidationLevel = " + string_XGL_VALIDATION_LEVEL(pStruct->maxValidationLevel) + "\n" + prefix + "flags = " + ss[5].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0] << pStruct->queueNodeIndex;
    ss[1] << pStruct->queueCount;
    final_str = prefix + "queueNodeIndex = " + ss[0].str() + "\n" + prefix + "queueCount = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    ss[0] << pStruct->x;
    ss[1] << pStruct->y;
    ss[2] << pStruct->z;
    final_str = prefix + "x = " + ss[0].str() + "\n" + prefix + "y = " + ss[1].str() + "\n" + prefix + "z = " + ss[2].str() + "\n";
    return final_str;
}
string xgl_print_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    ss[0] << pStruct->indexCount;
    ss[1] << pStruct->instanceCount;
    ss[2] << pStruct->firstIndex;
    ss[3] << pStruct->vertexOffset;
    ss[4] << pStruct->firstInstance;
    final_str = prefix + "indexCount = " + ss[0].str() + "\n" + prefix + "instanceCount = " + ss[1].str() + "\n" + prefix + "firstIndex = " + ss[2].str() + "\n" + prefix + "vertexOffset = " + ss[3].str() + "\n" + prefix + "firstInstance = " + ss[4].str() + "\n";
    return final_str;
}
string xgl_print_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    ss[0] << pStruct->vertexCount;
    ss[1] << pStruct->instanceCount;
    ss[2] << pStruct->firstVertex;
    ss[3] << pStruct->firstInstance;
    final_str = prefix + "vertexCount = " + ss[0].str() + "\n" + prefix + "instanceCount = " + ss[1].str() + "\n" + prefix + "firstVertex = " + ss[2].str() + "\n" + prefix + "firstInstance = " + ss[3].str() + "\n";
    return final_str;
}
string xgl_print_xgl_dynamic_cb_state_create_info(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < 4; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << pStruct->blendConst[i];
        stp_strs[1] += " " + prefix + "blendConst[" + index_ss.str() + "] = " + ss[1].str() + "\n";
        ss[1].str("");
    }
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "blendConst = " + ss[1].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_dynamic_ds_state_create_info(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->minDepth;
    ss[2] << pStruct->maxDepth;
    ss[3] << pStruct->stencilReadMask;
    ss[4] << pStruct->stencilWriteMask;
    ss[5] << pStruct->stencilFrontRef;
    ss[6] << pStruct->stencilBackRef;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "minDepth = " + ss[1].str() + "\n" + prefix + "maxDepth = " + ss[2].str() + "\n" + prefix + "stencilReadMask = " + ss[3].str() + "\n" + prefix + "stencilWriteMask = " + ss[4].str() + "\n" + prefix + "stencilFrontRef = " + ss[5].str() + "\n" + prefix + "stencilBackRef = " + ss[6].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_dynamic_rs_state_create_info(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->depthBias;
    ss[2] << pStruct->depthBiasClamp;
    ss[3] << pStruct->slopeScaledDepthBias;
    ss[4] << pStruct->pointSize;
    ss[5] << pStruct->pointFadeThreshold;
    ss[6] << pStruct->lineWidth;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "depthBias = " + ss[1].str() + "\n" + prefix + "depthBiasClamp = " + ss[2].str() + "\n" + prefix + "slopeScaledDepthBias = " + ss[3].str() + "\n" + prefix + "pointSize = " + ss[4].str() + "\n" + prefix + "pointFadeThreshold = " + ss[5].str() + "\n" + prefix + "lineWidth = " + ss[6].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_dynamic_vp_state_create_info(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[3];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->viewportAndScissorCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << &pStruct->pViewports[i];
        tmp_str = xgl_print_xgl_viewport(&pStruct->pViewports[i], extra_indent);
        stp_strs[1] += " " + prefix + "pViewports[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[1].str("");
    }
    stp_strs[2] = "";
    for (uint32_t i = 0; i < pStruct->viewportAndScissorCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[2] << &pStruct->pScissors[i];
        tmp_str = xgl_print_xgl_rect(&pStruct->pScissors[i], extra_indent);
        stp_strs[2] += " " + prefix + "pScissors[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[2].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->viewportAndScissorCount;
    ss[2].str("addr");
    ss[3].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "viewportAndScissorCount = " + ss[1].str() + "\n" + prefix + "pViewports = " + ss[2].str() + "\n" + prefix + "pScissors = " + ss[3].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "flags = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_event_wait_info(const XGL_EVENT_WAIT_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[3];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->eventCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << pStruct->pEvents[i];
        stp_strs[1] += " " + prefix + "pEvents[" + index_ss.str() + "] = " + ss[1].str() + "\n";
        ss[1].str("");
    }
    stp_strs[2] = "";
    for (uint32_t i = 0; i < pStruct->memBarrierCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[2] << pStruct->ppMemBarriers[i];
        stp_strs[2] += " " + prefix + "ppMemBarriers[" + index_ss.str() + "] = " + ss[2].str() + "\n";
        ss[2].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->eventCount;
    ss[2].str("addr");
    ss[3] << pStruct->memBarrierCount;
    ss[4].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "eventCount = " + ss[1].str() + "\n" + prefix + "pEvents = " + ss[2].str() + "\n" + prefix + "waitEvent = " + string_XGL_WAIT_EVENT(pStruct->waitEvent) + "\n" + prefix + "memBarrierCount = " + ss[3].str() + "\n" + prefix + "ppMemBarriers = " + ss[4].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_extent2d(const XGL_EXTENT2D* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0] << pStruct->width;
    ss[1] << pStruct->height;
    final_str = prefix + "width = " + ss[0].str() + "\n" + prefix + "height = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_extent3d(const XGL_EXTENT3D* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    ss[0] << pStruct->width;
    ss[1] << pStruct->height;
    ss[2] << pStruct->depth;
    final_str = prefix + "width = " + ss[0].str() + "\n" + prefix + "height = " + ss[1].str() + "\n" + prefix + "depth = " + ss[2].str() + "\n";
    return final_str;
}
string xgl_print_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "flags = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0] << pStruct->linearTilingFeatures;
    ss[1] << pStruct->optimalTilingFeatures;
    final_str = prefix + "linearTilingFeatures = " + ss[0].str() + "\n" + prefix + "optimalTilingFeatures = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_framebuffer_create_info(const XGL_FRAMEBUFFER_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[8];
    string stp_strs[3];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->colorAttachmentCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << &pStruct->pColorAttachments[i];
        tmp_str = xgl_print_xgl_color_attachment_bind_info(&pStruct->pColorAttachments[i], extra_indent);
        stp_strs[1] += " " + prefix + "pColorAttachments[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[1].str("");
    }
    if (pStruct->pDepthStencilAttachment) {
        tmp_str = xgl_print_xgl_depth_stencil_bind_info(pStruct->pDepthStencilAttachment, extra_indent);
        ss[2] << &pStruct->pDepthStencilAttachment;
        stp_strs[2] = " " + prefix + "pDepthStencilAttachment (addr)\n" + tmp_str;
        ss[2].str("");
    }
    else
        stp_strs[2] = "";
    ss[0].str("addr");
    ss[1] << pStruct->colorAttachmentCount;
    ss[2].str("addr");
    ss[3].str("addr");
    ss[4] << pStruct->sampleCount;
    ss[5] << pStruct->width;
    ss[6] << pStruct->height;
    ss[7] << pStruct->layers;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "colorAttachmentCount = " + ss[1].str() + "\n" + prefix + "pColorAttachments = " + ss[2].str() + "\n" + prefix + "pDepthStencilAttachment = " + ss[3].str() + "\n" + prefix + "sampleCount = " + ss[4].str() + "\n" + prefix + "width = " + ss[5].str() + "\n" + prefix + "height = " + ss[6].str() + "\n" + prefix + "layers = " + ss[7].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
    ss[0] << pStruct->compatibilityFlags;
    final_str = prefix + "compatibilityFlags = " + ss[0].str() + "\n";
    return final_str;
}
string xgl_print_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->flags;
    ss[2].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "flags = " + ss[1].str() + "\n" + prefix + "lastSetLayout = " + ss[2].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_image_copy(const XGL_IMAGE_COPY* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[5];
    tmp_str = xgl_print_xgl_image_subresource(&pStruct->srcSubresource, extra_indent);
    ss[0] << &pStruct->srcSubresource;
    stp_strs[0] = " " + prefix + "srcSubresource (addr)\n" + tmp_str;
    ss[0].str("");
    tmp_str = xgl_print_xgl_offset3d(&pStruct->srcOffset, extra_indent);
    ss[1] << &pStruct->srcOffset;
    stp_strs[1] = " " + prefix + "srcOffset (addr)\n" + tmp_str;
    ss[1].str("");
    tmp_str = xgl_print_xgl_image_subresource(&pStruct->destSubresource, extra_indent);
    ss[2] << &pStruct->destSubresource;
    stp_strs[2] = " " + prefix + "destSubresource (addr)\n" + tmp_str;
    ss[2].str("");
    tmp_str = xgl_print_xgl_offset3d(&pStruct->destOffset, extra_indent);
    ss[3] << &pStruct->destOffset;
    stp_strs[3] = " " + prefix + "destOffset (addr)\n" + tmp_str;
    ss[3].str("");
    tmp_str = xgl_print_xgl_extent3d(&pStruct->extent, extra_indent);
    ss[4] << &pStruct->extent;
    stp_strs[4] = " " + prefix + "extent (addr)\n" + tmp_str;
    ss[4].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    ss[3].str("addr");
    ss[4].str("addr");
    final_str = prefix + "srcSubresource = " + ss[0].str() + "\n" + prefix + "srcOffset = " + ss[1].str() + "\n" + prefix + "destSubresource = " + ss[2].str() + "\n" + prefix + "destOffset = " + ss[3].str() + "\n" + prefix + "extent = " + ss[4].str() + "\n" + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_extent3d(&pStruct->extent, extra_indent);
    ss[1] << &pStruct->extent;
    stp_strs[1] = " " + prefix + "extent (addr)\n" + tmp_str;
    ss[1].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2] << pStruct->mipLevels;
    ss[3] << pStruct->arraySize;
    ss[4] << pStruct->samples;
    ss[5] << pStruct->usage;
    ss[6] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "imageType = " + string_XGL_IMAGE_TYPE(pStruct->imageType) + "\n" + prefix + "format = " + string_XGL_FORMAT(pStruct->format) + "\n" + prefix + "extent = " + ss[1].str() + "\n" + prefix + "mipLevels = " + ss[2].str() + "\n" + prefix + "arraySize = " + ss[3].str() + "\n" + prefix + "samples = " + ss[4].str() + "\n" + prefix + "tiling = " + string_XGL_IMAGE_TILING(pStruct->tiling) + "\n" + prefix + "usage = " + ss[5].str() + "\n" + prefix + "flags = " + ss[6].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_image_memory_barrier(const XGL_IMAGE_MEMORY_BARRIER* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_image_subresource_range(&pStruct->subresourceRange, extra_indent);
    ss[1] << &pStruct->subresourceRange;
    stp_strs[1] = " " + prefix + "subresourceRange (addr)\n" + tmp_str;
    ss[1].str("");
    ss[0].str("addr");
    ss[1] << pStruct->outputMask;
    ss[2] << pStruct->inputMask;
    ss[3].str("addr");
    ss[4].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "outputMask = " + ss[1].str() + "\n" + prefix + "inputMask = " + ss[2].str() + "\n" + prefix + "oldLayout = " + string_XGL_IMAGE_LAYOUT(pStruct->oldLayout) + "\n" + prefix + "newLayout = " + string_XGL_IMAGE_LAYOUT(pStruct->newLayout) + "\n" + prefix + "image = " + ss[3].str() + "\n" + prefix + "subresourceRange = " + ss[4].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_image_memory_bind_info(const XGL_IMAGE_MEMORY_BIND_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[3];
    tmp_str = xgl_print_xgl_image_subresource(&pStruct->subresource, extra_indent);
    ss[0] << &pStruct->subresource;
    stp_strs[0] = " " + prefix + "subresource (addr)\n" + tmp_str;
    ss[0].str("");
    tmp_str = xgl_print_xgl_offset3d(&pStruct->offset, extra_indent);
    ss[1] << &pStruct->offset;
    stp_strs[1] = " " + prefix + "offset (addr)\n" + tmp_str;
    ss[1].str("");
    tmp_str = xgl_print_xgl_extent3d(&pStruct->extent, extra_indent);
    ss[2] << &pStruct->extent;
    stp_strs[2] = " " + prefix + "extent (addr)\n" + tmp_str;
    ss[2].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    final_str = prefix + "subresource = " + ss[0].str() + "\n" + prefix + "offset = " + ss[1].str() + "\n" + prefix + "extent = " + ss[2].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_image_memory_requirements(const XGL_IMAGE_MEMORY_REQUIREMENTS* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0] << pStruct->usage;
    ss[1] << pStruct->samples;
    final_str = prefix + "usage = " + ss[0].str() + "\n" + prefix + "formatClass = " + string_XGL_IMAGE_FORMAT_CLASS(pStruct->formatClass) + "\n" + prefix + "samples = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[5];
    tmp_str = xgl_print_xgl_image_subresource(&pStruct->srcSubresource, extra_indent);
    ss[0] << &pStruct->srcSubresource;
    stp_strs[0] = " " + prefix + "srcSubresource (addr)\n" + tmp_str;
    ss[0].str("");
    tmp_str = xgl_print_xgl_offset2d(&pStruct->srcOffset, extra_indent);
    ss[1] << &pStruct->srcOffset;
    stp_strs[1] = " " + prefix + "srcOffset (addr)\n" + tmp_str;
    ss[1].str("");
    tmp_str = xgl_print_xgl_image_subresource(&pStruct->destSubresource, extra_indent);
    ss[2] << &pStruct->destSubresource;
    stp_strs[2] = " " + prefix + "destSubresource (addr)\n" + tmp_str;
    ss[2].str("");
    tmp_str = xgl_print_xgl_offset2d(&pStruct->destOffset, extra_indent);
    ss[3] << &pStruct->destOffset;
    stp_strs[3] = " " + prefix + "destOffset (addr)\n" + tmp_str;
    ss[3].str("");
    tmp_str = xgl_print_xgl_extent2d(&pStruct->extent, extra_indent);
    ss[4] << &pStruct->extent;
    stp_strs[4] = " " + prefix + "extent (addr)\n" + tmp_str;
    ss[4].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    ss[3].str("addr");
    ss[4].str("addr");
    final_str = prefix + "srcSubresource = " + ss[0].str() + "\n" + prefix + "srcOffset = " + ss[1].str() + "\n" + prefix + "destSubresource = " + ss[2].str() + "\n" + prefix + "destOffset = " + ss[3].str() + "\n" + prefix + "extent = " + ss[4].str() + "\n" + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0] << pStruct->mipLevel;
    ss[1] << pStruct->arraySlice;
    final_str = prefix + "aspect = " + string_XGL_IMAGE_ASPECT(pStruct->aspect) + "\n" + prefix + "mipLevel = " + ss[0].str() + "\n" + prefix + "arraySlice = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    ss[0] << pStruct->baseMipLevel;
    ss[1] << pStruct->mipLevels;
    ss[2] << pStruct->baseArraySlice;
    ss[3] << pStruct->arraySize;
    final_str = prefix + "aspect = " + string_XGL_IMAGE_ASPECT(pStruct->aspect) + "\n" + prefix + "baseMipLevel = " + ss[0].str() + "\n" + prefix + "mipLevels = " + ss[1].str() + "\n" + prefix + "baseArraySlice = " + ss[2].str() + "\n" + prefix + "arraySize = " + ss[3].str() + "\n";
    return final_str;
}
string xgl_print_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "view = " + ss[1].str() + "\n" + prefix + "layout = " + string_XGL_IMAGE_LAYOUT(pStruct->layout) + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[3];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_channel_mapping(&pStruct->channels, extra_indent);
    ss[1] << &pStruct->channels;
    stp_strs[1] = " " + prefix + "channels (addr)\n" + tmp_str;
    ss[1].str("");
    tmp_str = xgl_print_xgl_image_subresource_range(&pStruct->subresourceRange, extra_indent);
    ss[2] << &pStruct->subresourceRange;
    stp_strs[2] = " " + prefix + "subresourceRange (addr)\n" + tmp_str;
    ss[2].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    ss[3].str("addr");
    ss[4] << pStruct->minLod;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "image = " + ss[1].str() + "\n" + prefix + "viewType = " + string_XGL_IMAGE_VIEW_TYPE(pStruct->viewType) + "\n" + prefix + "format = " + string_XGL_FORMAT(pStruct->format) + "\n" + prefix + "channels = " + ss[2].str() + "\n" + prefix + "subresourceRange = " + ss[3].str() + "\n" + prefix + "minLod = " + ss[4].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_layer_create_info(const XGL_LAYER_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->layerCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << pStruct->ppActiveLayerNames[i];
        stp_strs[1] += " " + prefix + "ppActiveLayerNames[" + index_ss.str() + "] = " + ss[1].str() + "\n";
        ss[1].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->layerCount;
    ss[2] << pStruct->ppActiveLayerNames;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "layerCount = " + ss[1].str() + "\n" + prefix + "ppActiveLayerNames = " + ss[2].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    ss[0] << pStruct->bufferId;
    ss[1].str("addr");
    ss[2].str("addr");
    final_str = prefix + "bufferId = " + ss[0].str() + "\n" + prefix + "bufferSize = " + ss[1].str() + "\n" + prefix + "pBufferData = " + ss[2].str() + "\n";
    return final_str;
}
string xgl_print_xgl_memory_alloc_buffer_info(const XGL_MEMORY_ALLOC_BUFFER_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->usage;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "usage = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_memory_alloc_image_info(const XGL_MEMORY_ALLOC_IMAGE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->usage;
    ss[2] << pStruct->samples;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "usage = " + ss[1].str() + "\n" + prefix + "formatClass = " + string_XGL_IMAGE_FORMAT_CLASS(pStruct->formatClass) + "\n" + prefix + "samples = " + ss[2].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2] << pStruct->memProps;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "allocationSize = " + ss[1].str() + "\n" + prefix + "memProps = " + ss[2].str() + "\n" + prefix + "memType = " + string_XGL_MEMORY_TYPE(pStruct->memType) + "\n" + prefix + "memPriority = " + string_XGL_MEMORY_PRIORITY(pStruct->memPriority) + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_memory_barrier(const XGL_MEMORY_BARRIER* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->outputMask;
    ss[2] << pStruct->inputMask;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "outputMask = " + ss[1].str() + "\n" + prefix + "inputMask = " + ss[2].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "sharedMem = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_memory_ref(const XGL_MEMORY_REF* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0].str("addr");
    ss[1] << pStruct->flags;
    final_str = prefix + "mem = " + ss[0].str() + "\n" + prefix + "flags = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    ss[3] << pStruct->memProps;
    final_str = prefix + "size = " + ss[0].str() + "\n" + prefix + "alignment = " + ss[1].str() + "\n" + prefix + "granularity = " + ss[2].str() + "\n" + prefix + "memProps = " + ss[3].str() + "\n" + prefix + "memType = " + string_XGL_MEMORY_TYPE(pStruct->memType) + "\n";
    return final_str;
}
string xgl_print_xgl_offset2d(const XGL_OFFSET2D* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0] << pStruct->x;
    ss[1] << pStruct->y;
    final_str = prefix + "x = " + ss[0].str() + "\n" + prefix + "y = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_offset3d(const XGL_OFFSET3D* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    ss[0] << pStruct->x;
    ss[1] << pStruct->y;
    ss[2] << pStruct->z;
    final_str = prefix + "x = " + ss[0].str() + "\n" + prefix + "y = " + ss[1].str() + "\n" + prefix + "z = " + ss[2].str() + "\n";
    return final_str;
}
string xgl_print_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
    ss[0].str("addr");
    final_str = prefix + "originalImage = " + ss[0].str() + "\n";
    return final_str;
}
string xgl_print_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "originalMem = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0].str(pStruct->supportsMigration ? "TRUE" : "FALSE");
    ss[1].str(pStruct->supportsPinning ? "TRUE" : "FALSE");
    final_str = prefix + "supportsMigration = " + ss[0].str() + "\n" + prefix + "supportsPinning = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    ss[0] << pStruct->maxGpuClock;
    ss[1] << pStruct->aluPerClock;
    ss[2] << pStruct->texPerClock;
    ss[3] << pStruct->primsPerClock;
    ss[4] << pStruct->pixelsPerClock;
    final_str = prefix + "maxGpuClock = " + ss[0].str() + "\n" + prefix + "aluPerClock = " + ss[1].str() + "\n" + prefix + "texPerClock = " + ss[2].str() + "\n" + prefix + "primsPerClock = " + ss[3].str() + "\n" + prefix + "pixelsPerClock = " + ss[4].str() + "\n";
    return final_str;
}
string xgl_print_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[14];
    string stp_strs[1];
    stp_strs[0] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < XGL_MAX_PHYSICAL_GPU_NAME; i++) {
        index_ss.str("");
        index_ss << i;
        ss[0] << pStruct->gpuName[i];
        stp_strs[0] += " " + prefix + "gpuName[" + index_ss.str() + "] = " + ss[0].str() + "\n";
        ss[0].str("");
    }
    ss[0] << pStruct->apiVersion;
    ss[1] << pStruct->driverVersion;
    ss[2] << pStruct->vendorId;
    ss[3] << pStruct->deviceId;
    ss[4] << pStruct->gpuName;
    ss[5] << pStruct->maxMemRefsPerSubmission;
    ss[6].str("addr");
    ss[7] << pStruct->maxBoundDescriptorSets;
    ss[8] << pStruct->maxThreadGroupSize;
    ss[9] << pStruct->timestampFrequency;
    ss[10].str(pStruct->multiColorAttachmentClears ? "TRUE" : "FALSE");
    ss[11] << pStruct->maxDescriptorSets;
    ss[12] << pStruct->maxViewports;
    ss[13] << pStruct->maxColorAttachments;
    final_str = prefix + "apiVersion = " + ss[0].str() + "\n" + prefix + "driverVersion = " + ss[1].str() + "\n" + prefix + "vendorId = " + ss[2].str() + "\n" + prefix + "deviceId = " + ss[3].str() + "\n" + prefix + "gpuType = " + string_XGL_PHYSICAL_GPU_TYPE(pStruct->gpuType) + "\n" + prefix + "gpuName = " + ss[4].str() + "\n" + prefix + "maxMemRefsPerSubmission = " + ss[5].str() + "\n" + prefix + "maxInlineMemoryUpdateSize = " + ss[6].str() + "\n" + prefix + "maxBoundDescriptorSets = " + ss[7].str() + "\n" + prefix + "maxThreadGroupSize = " + ss[8].str() + "\n" + prefix + "timestampFrequency = " + ss[9].str() + "\n" + prefix + "multiColorAttachmentClears = " + ss[10].str() + "\n" + prefix + "maxDescriptorSets = " + ss[11].str() + "\n" + prefix + "maxViewports = " + ss[12].str() + "\n" + prefix + "maxColorAttachments = " + ss[13].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    ss[0] << pStruct->queueFlags;
    ss[1] << pStruct->queueCount;
    ss[2] << pStruct->maxAtomicCounters;
    ss[3].str(pStruct->supportsTimestamps ? "TRUE" : "FALSE");
    final_str = prefix + "queueFlags = " + ss[0].str() + "\n" + prefix + "queueCount = " + ss[1].str() + "\n" + prefix + "maxAtomicCounters = " + ss[2].str() + "\n" + prefix + "supportsTimestamps = " + ss[3].str() + "\n";
    return final_str;
}
string xgl_print_xgl_pipeline_barrier(const XGL_PIPELINE_BARRIER* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[3];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->eventCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << pStruct->pEvents[i];
        stp_strs[1] += " " + prefix + "pEvents[" + index_ss.str() + "] = " + ss[1].str() + "\n";
        ss[1].str("");
    }
    stp_strs[2] = "";
    for (uint32_t i = 0; i < pStruct->memBarrierCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[2] << pStruct->ppMemBarriers[i];
        stp_strs[2] += " " + prefix + "ppMemBarriers[" + index_ss.str() + "] = " + ss[2].str() + "\n";
        ss[2].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->eventCount;
    ss[2] << pStruct->memBarrierCount;
    ss[3].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "eventCount = " + ss[1].str() + "\n" + prefix + "pEvents = " + string_XGL_SET_EVENT(*pStruct->pEvents) + "\n" + prefix + "waitEvent = " + string_XGL_WAIT_EVENT(pStruct->waitEvent) + "\n" + prefix + "memBarrierCount = " + ss[2].str() + "\n" + prefix + "ppMemBarriers = " + ss[3].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0].str(pStruct->blendEnable ? "TRUE" : "FALSE");
    ss[1] << (uint32_t)pStruct->channelWriteMask;
    final_str = prefix + "blendEnable = " + ss[0].str() + "\n" + prefix + "format = " + string_XGL_FORMAT(pStruct->format) + "\n" + prefix + "srcBlendColor = " + string_XGL_BLEND(pStruct->srcBlendColor) + "\n" + prefix + "destBlendColor = " + string_XGL_BLEND(pStruct->destBlendColor) + "\n" + prefix + "blendFuncColor = " + string_XGL_BLEND_FUNC(pStruct->blendFuncColor) + "\n" + prefix + "srcBlendAlpha = " + string_XGL_BLEND(pStruct->srcBlendAlpha) + "\n" + prefix + "destBlendAlpha = " + string_XGL_BLEND(pStruct->destBlendAlpha) + "\n" + prefix + "blendFuncAlpha = " + string_XGL_BLEND_FUNC(pStruct->blendFuncAlpha) + "\n" + prefix + "channelWriteMask = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->attachmentCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << &pStruct->pAttachments[i];
        tmp_str = xgl_print_xgl_pipeline_cb_attachment_state(&pStruct->pAttachments[i], extra_indent);
        stp_strs[1] += " " + prefix + "pAttachments[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[1].str("");
    }
    ss[0].str("addr");
    ss[1].str(pStruct->alphaToCoverageEnable ? "TRUE" : "FALSE");
    ss[2].str(pStruct->logicOpEnable ? "TRUE" : "FALSE");
    ss[3] << pStruct->attachmentCount;
    ss[4].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "alphaToCoverageEnable = " + ss[1].str() + "\n" + prefix + "logicOpEnable = " + ss[2].str() + "\n" + prefix + "logicOp = " + string_XGL_LOGIC_OP(pStruct->logicOp) + "\n" + prefix + "attachmentCount = " + ss[3].str() + "\n" + prefix + "pAttachments = " + ss[4].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_ds_state_create_info(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[3];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_stencil_op_state(&pStruct->front, extra_indent);
    ss[1] << &pStruct->front;
    stp_strs[1] = " " + prefix + "front (addr)\n" + tmp_str;
    ss[1].str("");
    tmp_str = xgl_print_xgl_stencil_op_state(&pStruct->back, extra_indent);
    ss[2] << &pStruct->back;
    stp_strs[2] = " " + prefix + "back (addr)\n" + tmp_str;
    ss[2].str("");
    ss[0].str("addr");
    ss[1].str(pStruct->depthTestEnable ? "TRUE" : "FALSE");
    ss[2].str(pStruct->depthWriteEnable ? "TRUE" : "FALSE");
    ss[3].str(pStruct->depthBoundsEnable ? "TRUE" : "FALSE");
    ss[4].str(pStruct->stencilTestEnable ? "TRUE" : "FALSE");
    ss[5].str("addr");
    ss[6].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "format = " + string_XGL_FORMAT(pStruct->format) + "\n" + prefix + "depthTestEnable = " + ss[1].str() + "\n" + prefix + "depthWriteEnable = " + ss[2].str() + "\n" + prefix + "depthFunc = " + string_XGL_COMPARE_FUNC(pStruct->depthFunc) + "\n" + prefix + "depthBoundsEnable = " + ss[3].str() + "\n" + prefix + "stencilTestEnable = " + ss[4].str() + "\n" + prefix + "front = " + ss[5].str() + "\n" + prefix + "back = " + ss[6].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str(pStruct->disableVertexReuse ? "TRUE" : "FALSE");
    ss[2].str(pStruct->primitiveRestartEnable ? "TRUE" : "FALSE");
    ss[3] << pStruct->primitiveRestartIndex;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "topology = " + string_XGL_PRIMITIVE_TOPOLOGY(pStruct->topology) + "\n" + prefix + "disableVertexReuse = " + ss[1].str() + "\n" + prefix + "primitiveRestartEnable = " + ss[2].str() + "\n" + prefix + "primitiveRestartIndex = " + ss[3].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_ms_state_create_info(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->samples;
    ss[2].str(pStruct->multisampleEnable ? "TRUE" : "FALSE");
    ss[3].str(pStruct->sampleShadingEnable ? "TRUE" : "FALSE");
    ss[4] << pStruct->minSampleShading;
    ss[5] << pStruct->sampleMask;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "samples = " + ss[1].str() + "\n" + prefix + "multisampleEnable = " + ss[2].str() + "\n" + prefix + "sampleShadingEnable = " + ss[3].str() + "\n" + prefix + "minSampleShading = " + ss[4].str() + "\n" + prefix + "sampleMask = " + ss[5].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str(pStruct->depthClipEnable ? "TRUE" : "FALSE");
    ss[2].str(pStruct->rasterizerDiscardEnable ? "TRUE" : "FALSE");
    ss[3].str(pStruct->programPointSize ? "TRUE" : "FALSE");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "depthClipEnable = " + ss[1].str() + "\n" + prefix + "rasterizerDiscardEnable = " + ss[2].str() + "\n" + prefix + "programPointSize = " + ss[3].str() + "\n" + prefix + "pointOrigin = " + string_XGL_COORDINATE_ORIGIN(pStruct->pointOrigin) + "\n" + prefix + "provokingVertex = " + string_XGL_PROVOKING_VERTEX_CONVENTION(pStruct->provokingVertex) + "\n" + prefix + "fillMode = " + string_XGL_FILL_MODE(pStruct->fillMode) + "\n" + prefix + "cullMode = " + string_XGL_CULL_MODE(pStruct->cullMode) + "\n" + prefix + "frontFace = " + string_XGL_FACE_ORIENTATION(pStruct->frontFace) + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
    stp_strs[0] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->linkConstBufferCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[0] << &pStruct->pLinkConstBufferInfo[i];
        tmp_str = xgl_print_xgl_link_const_buffer(&pStruct->pLinkConstBufferInfo[i], extra_indent);
        stp_strs[0] += " " + prefix + "pLinkConstBufferInfo[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[0].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->linkConstBufferCount;
    ss[2].str("addr");
    final_str = prefix + "stage = " + string_XGL_PIPELINE_SHADER_STAGE(pStruct->stage) + "\n" + prefix + "shader = " + ss[0].str() + "\n" + prefix + "linkConstBufferCount = " + ss[1].str() + "\n" + prefix + "pLinkConstBufferInfo = " + ss[2].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_pipeline_shader(&pStruct->shader, extra_indent);
    ss[1] << &pStruct->shader;
    stp_strs[1] = " " + prefix + "shader (addr)\n" + tmp_str;
    ss[1].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "shader = " + ss[1].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[11];
    ss[0] << pStruct->fsInvocations;
    ss[1] << pStruct->cPrimitives;
    ss[2] << pStruct->cInvocations;
    ss[3] << pStruct->vsInvocations;
    ss[4] << pStruct->gsInvocations;
    ss[5] << pStruct->gsPrimitives;
    ss[6] << pStruct->iaPrimitives;
    ss[7] << pStruct->iaVertices;
    ss[8] << pStruct->tcsInvocations;
    ss[9] << pStruct->tesInvocations;
    ss[10] << pStruct->csInvocations;
    final_str = prefix + "fsInvocations = " + ss[0].str() + "\n" + prefix + "cPrimitives = " + ss[1].str() + "\n" + prefix + "cInvocations = " + ss[2].str() + "\n" + prefix + "vsInvocations = " + ss[3].str() + "\n" + prefix + "gsInvocations = " + ss[4].str() + "\n" + prefix + "gsPrimitives = " + ss[5].str() + "\n" + prefix + "iaPrimitives = " + ss[6].str() + "\n" + prefix + "iaVertices = " + ss[7].str() + "\n" + prefix + "tcsInvocations = " + ss[8].str() + "\n" + prefix + "tesInvocations = " + ss[9].str() + "\n" + prefix + "csInvocations = " + ss[10].str() + "\n";
    return final_str;
}
string xgl_print_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->patchControlPoints;
    ss[2] << pStruct->optimalTessFactor;
    ss[3] << pStruct->fixedTessFactor;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "patchControlPoints = " + ss[1].str() + "\n" + prefix + "optimalTessFactor = " + ss[2].str() + "\n" + prefix + "fixedTessFactor = " + ss[3].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_vertex_input_create_info(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[3];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->bindingCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << &pStruct->pVertexBindingDescriptions[i];
        tmp_str = xgl_print_xgl_vertex_input_binding_description(&pStruct->pVertexBindingDescriptions[i], extra_indent);
        stp_strs[1] += " " + prefix + "pVertexBindingDescriptions[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[1].str("");
    }
    stp_strs[2] = "";
    for (uint32_t i = 0; i < pStruct->attributeCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[2] << &pStruct->pVertexAttributeDescriptions[i];
        tmp_str = xgl_print_xgl_vertex_input_attribute_description(&pStruct->pVertexAttributeDescriptions[i], extra_indent);
        stp_strs[2] += " " + prefix + "pVertexAttributeDescriptions[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[2].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->bindingCount;
    ss[2].str("addr");
    ss[3] << pStruct->attributeCount;
    ss[4].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "bindingCount = " + ss[1].str() + "\n" + prefix + "pVertexBindingDescriptions = " + ss[2].str() + "\n" + prefix + "attributeCount = " + ss[3].str() + "\n" + prefix + "pVertexAttributeDescriptions = " + ss[4].str() + "\n" + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_pipeline_vp_state_create_info(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->numViewports;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "numViewports = " + ss[1].str() + "\n" + prefix + "clipOrigin = " + string_XGL_COORDINATE_ORIGIN(pStruct->clipOrigin) + "\n" + prefix + "depthMode = " + string_XGL_DEPTH_MODE(pStruct->depthMode) + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->slots;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "queryType = " + string_XGL_QUERY_TYPE(pStruct->queryType) + "\n" + prefix + "slots = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[3];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->initialCount;
    ss[2] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "initialCount = " + ss[1].str() + "\n" + prefix + "flags = " + ss[2].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "sharedSemaphore = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_rect(const XGL_RECT* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[2];
    tmp_str = xgl_print_xgl_offset2d(&pStruct->offset, extra_indent);
    ss[0] << &pStruct->offset;
    stp_strs[0] = " " + prefix + "offset (addr)\n" + tmp_str;
    ss[0].str("");
    tmp_str = xgl_print_xgl_extent2d(&pStruct->extent, extra_indent);
    ss[1] << &pStruct->extent;
    stp_strs[1] = " " + prefix + "extent (addr)\n" + tmp_str;
    ss[1].str("");
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "offset = " + ss[0].str() + "\n" + prefix + "extent = " + ss[1].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_render_pass_create_info(const XGL_RENDER_PASS_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[7];
    string stp_strs[5];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    tmp_str = xgl_print_xgl_rect(&pStruct->renderArea, extra_indent);
    ss[1] << &pStruct->renderArea;
    stp_strs[1] = " " + prefix + "renderArea (addr)\n" + tmp_str;
    ss[1].str("");
    stp_strs[2] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->colorAttachmentCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[2] << pStruct->pColorLoadOps[i];
        stp_strs[2] += " " + prefix + "pColorLoadOps[" + index_ss.str() + "] = " + ss[2].str() + "\n";
        ss[2].str("");
    }
    stp_strs[3] = "";
    for (uint32_t i = 0; i < pStruct->colorAttachmentCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[3] << pStruct->pColorStoreOps[i];
        stp_strs[3] += " " + prefix + "pColorStoreOps[" + index_ss.str() + "] = " + ss[3].str() + "\n";
        ss[3].str("");
    }
    stp_strs[4] = "";
    for (uint32_t i = 0; i < pStruct->colorAttachmentCount; i++) {
        index_ss.str("");
        index_ss << i;
        ss[4] << &pStruct->pColorLoadClearValues[i];
        tmp_str = xgl_print_xgl_clear_color(&pStruct->pColorLoadClearValues[i], extra_indent);
        stp_strs[4] += " " + prefix + "pColorLoadClearValues[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[4].str("");
    }
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    ss[3] << pStruct->colorAttachmentCount;
    ss[4].str("addr");
    ss[5] << pStruct->depthLoadClearValue;
    ss[6] << pStruct->stencilLoadClearValue;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "renderArea = " + ss[1].str() + "\n" + prefix + "framebuffer = " + ss[2].str() + "\n" + prefix + "colorAttachmentCount = " + ss[3].str() + "\n" + prefix + "pColorLoadOps = " + string_XGL_ATTACHMENT_LOAD_OP(*pStruct->pColorLoadOps) + "\n" + prefix + "pColorStoreOps = " + string_XGL_ATTACHMENT_STORE_OP(*pStruct->pColorStoreOps) + "\n" + prefix + "pColorLoadClearValues = " + ss[4].str() + "\n" + prefix + "depthLoadOp = " + string_XGL_ATTACHMENT_LOAD_OP(pStruct->depthLoadOp) + "\n" + prefix + "depthLoadClearValue = " + ss[5].str() + "\n" + prefix + "depthStoreOp = " + string_XGL_ATTACHMENT_STORE_OP(pStruct->depthStoreOp) + "\n" + prefix + "stencilLoadOp = " + string_XGL_ATTACHMENT_LOAD_OP(pStruct->stencilLoadOp) + "\n" + prefix + "stencilLoadClearValue = " + ss[6].str() + "\n" + prefix + "stencilStoreOp = " + string_XGL_ATTACHMENT_STORE_OP(pStruct->stencilStoreOp) + "\n" + stp_strs[4] + stp_strs[3] + stp_strs[2] + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[5];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1] << pStruct->mipLodBias;
    ss[2] << pStruct->maxAnisotropy;
    ss[3] << pStruct->minLod;
    ss[4] << pStruct->maxLod;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "magFilter = " + string_XGL_TEX_FILTER(pStruct->magFilter) + "\n" + prefix + "minFilter = " + string_XGL_TEX_FILTER(pStruct->minFilter) + "\n" + prefix + "mipMode = " + string_XGL_TEX_MIPMAP_MODE(pStruct->mipMode) + "\n" + prefix + "addressU = " + string_XGL_TEX_ADDRESS(pStruct->addressU) + "\n" + prefix + "addressV = " + string_XGL_TEX_ADDRESS(pStruct->addressV) + "\n" + prefix + "addressW = " + string_XGL_TEX_ADDRESS(pStruct->addressW) + "\n" + prefix + "mipLodBias = " + ss[1].str() + "\n" + prefix + "maxAnisotropy = " + ss[2].str() + "\n" + prefix + "compareFunc = " + string_XGL_COMPARE_FUNC(pStruct->compareFunc) + "\n" + prefix + "minLod = " + ss[3].str() + "\n" + prefix + "maxLod = " + ss[4].str() + "\n" + prefix + "borderColorType = " + string_XGL_BORDER_COLOR_TYPE(pStruct->borderColorType) + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_sampler_image_view_info(const XGL_SAMPLER_IMAGE_VIEW_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    string stp_strs[1];
    if (pStruct->pImageView) {
        tmp_str = xgl_print_xgl_image_view_attach_info(pStruct->pImageView, extra_indent);
        ss[0] << &pStruct->pImageView;
        stp_strs[0] = " " + prefix + "pImageView (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    final_str = prefix + "pSampler = " + ss[0].str() + "\n" + prefix + "pImageView = " + ss[1].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    ss[3] << pStruct->flags;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "codeSize = " + ss[1].str() + "\n" + prefix + "pCode = " + ss[2].str() + "\n" + prefix + "flags = " + ss[3].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    final_str = prefix + "stencilFailOp = " + string_XGL_STENCIL_OP(pStruct->stencilFailOp) + "\n" + prefix + "stencilPassOp = " + string_XGL_STENCIL_OP(pStruct->stencilPassOp) + "\n" + prefix + "stencilDepthFailOp = " + string_XGL_STENCIL_OP(pStruct->stencilDepthFailOp) + "\n" + prefix + "stencilFunc = " + string_XGL_COMPARE_FUNC(pStruct->stencilFunc) + "\n";
    return final_str;
}
string xgl_print_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2].str("addr");
    ss[3].str("addr");
    final_str = prefix + "offset = " + ss[0].str() + "\n" + prefix + "size = " + ss[1].str() + "\n" + prefix + "rowPitch = " + ss[2].str() + "\n" + prefix + "depthPitch = " + ss[3].str() + "\n";
    return final_str;
}
string xgl_print_xgl_update_as_copy(const XGL_UPDATE_AS_COPY* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[1];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    ss[0].str("addr");
    ss[1].str("addr");
    ss[2] << pStruct->descriptorIndex;
    ss[3] << pStruct->count;
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "descriptorType = " + string_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType) + "\n" + prefix + "descriptorSet = " + ss[1].str() + "\n" + prefix + "descriptorIndex = " + ss[2].str() + "\n" + prefix + "count = " + ss[3].str() + "\n" + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_update_buffers(const XGL_UPDATE_BUFFERS* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->count; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << pStruct->pBufferViews[i];
        tmp_str = xgl_print_xgl_buffer_view_attach_info(pStruct->pBufferViews[i], extra_indent);
        stp_strs[1] += " " + prefix + "pBufferViews[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[1].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->index;
    ss[2] << pStruct->count;
    ss[3].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "descriptorType = " + string_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType) + "\n" + prefix + "index = " + ss[1].str() + "\n" + prefix + "count = " + ss[2].str() + "\n" + prefix + "pBufferViews = " + ss[3].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_update_images(const XGL_UPDATE_IMAGES* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->count; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << pStruct->pImageViews[i];
        tmp_str = xgl_print_xgl_image_view_attach_info(pStruct->pImageViews[i], extra_indent);
        stp_strs[1] += " " + prefix + "pImageViews[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[1].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->index;
    ss[2] << pStruct->count;
    ss[3].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "descriptorType = " + string_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType) + "\n" + prefix + "index = " + ss[1].str() + "\n" + prefix + "count = " + ss[2].str() + "\n" + prefix + "pImageViews = " + ss[3].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_update_samplers(const XGL_UPDATE_SAMPLERS* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->count; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << pStruct->pSamplers[i];
        stp_strs[1] += " " + prefix + "pSamplers[" + index_ss.str() + "] = " + ss[1].str() + "\n";
        ss[1].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->index;
    ss[2] << pStruct->count;
    ss[3].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "index = " + ss[1].str() + "\n" + prefix + "count = " + ss[2].str() + "\n" + prefix + "pSamplers = " + ss[3].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_update_sampler_textures(const XGL_UPDATE_SAMPLER_TEXTURES* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[4];
    string stp_strs[2];
    if (pStruct->pNext) {
        tmp_str = dynamic_display((void*)pStruct->pNext, prefix);
        ss[0] << &pStruct->pNext;
        stp_strs[0] = " " + prefix + "pNext (addr)\n" + tmp_str;
        ss[0].str("");
    }
    else
        stp_strs[0] = "";
    stp_strs[1] = "";
    stringstream index_ss;
    for (uint32_t i = 0; i < pStruct->count; i++) {
        index_ss.str("");
        index_ss << i;
        ss[1] << &pStruct->pSamplerImageViews[i];
        tmp_str = xgl_print_xgl_sampler_image_view_info(&pStruct->pSamplerImageViews[i], extra_indent);
        stp_strs[1] += " " + prefix + "pSamplerImageViews[" + index_ss.str() + "] (addr)\n" + tmp_str;
        ss[1].str("");
    }
    ss[0].str("addr");
    ss[1] << pStruct->index;
    ss[2] << pStruct->count;
    ss[3].str("addr");
    final_str = prefix + "sType = " + string_XGL_STRUCTURE_TYPE(pStruct->sType) + "\n" + prefix + "pNext = " + ss[0].str() + "\n" + prefix + "index = " + ss[1].str() + "\n" + prefix + "count = " + ss[2].str() + "\n" + prefix + "pSamplerImageViews = " + ss[3].str() + "\n" + stp_strs[1] + stp_strs[0];
    return final_str;
}
string xgl_print_xgl_vertex_input_attribute_description(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[2];
    ss[0] << pStruct->binding;
    ss[1] << pStruct->offsetInBytes;
    final_str = prefix + "binding = " + ss[0].str() + "\n" + prefix + "format = " + string_XGL_FORMAT(pStruct->format) + "\n" + prefix + "offsetInBytes = " + ss[1].str() + "\n";
    return final_str;
}
string xgl_print_xgl_vertex_input_binding_description(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[1];
    ss[0] << pStruct->strideInBytes;
    final_str = prefix + "strideInBytes = " + ss[0].str() + "\n" + prefix + "stepRate = " + string_XGL_VERTEX_INPUT_STEP_RATE(pStruct->stepRate) + "\n";
    return final_str;
}
string xgl_print_xgl_viewport(const XGL_VIEWPORT* pStruct, const string prefix)
{
    string final_str;
    string tmp_str;
    string extra_indent = "  " + prefix;
    stringstream ss[6];
    ss[0] << pStruct->originX;
    ss[1] << pStruct->originY;
    ss[2] << pStruct->width;
    ss[3] << pStruct->height;
    ss[4] << pStruct->minDepth;
    ss[5] << pStruct->maxDepth;
    final_str = prefix + "originX = " + ss[0].str() + "\n" + prefix + "originY = " + ss[1].str() + "\n" + prefix + "width = " + ss[2].str() + "\n" + prefix + "height = " + ss[3].str() + "\n" + prefix + "minDepth = " + ss[4].str() + "\n" + prefix + "maxDepth = " + ss[5].str() + "\n";
    return final_str;
}
string string_convert_helper(const void* toString, const string prefix)
{
    stringstream ss;
    ss << toString;
    string final_str = prefix + ss.str();
    return final_str;
}
string dynamic_display(const void* pStruct, const string prefix)
{
    // Cast to APP_INFO ptr initially just to pull sType off struct
    if (pStruct == NULL) {

        return NULL;
    }

    XGL_STRUCTURE_TYPE sType = ((XGL_APPLICATION_INFO*)pStruct)->sType;
    string indent = "    ";
    indent += prefix;
    switch (sType)
    {
        case XGL_STRUCTURE_TYPE_APPLICATION_INFO:
        {
            return xgl_print_xgl_application_info((XGL_APPLICATION_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_BUFFER_CREATE_INFO:
        {
            return xgl_print_xgl_buffer_create_info((XGL_BUFFER_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
        {
            return xgl_print_xgl_buffer_memory_barrier((XGL_BUFFER_MEMORY_BARRIER*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_ATTACH_INFO:
        {
            return xgl_print_xgl_buffer_view_attach_info((XGL_BUFFER_VIEW_ATTACH_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO:
        {
            return xgl_print_xgl_buffer_view_create_info((XGL_BUFFER_VIEW_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO:
        {
            return xgl_print_xgl_cmd_buffer_begin_info((XGL_CMD_BUFFER_BEGIN_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO:
        {
            return xgl_print_xgl_cmd_buffer_create_info((XGL_CMD_BUFFER_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO:
        {
            return xgl_print_xgl_cmd_buffer_graphics_begin_info((XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO:
        {
            return xgl_print_xgl_color_attachment_view_create_info((XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO:
        {
            return xgl_print_xgl_compute_pipeline_create_info((XGL_COMPUTE_PIPELINE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO:
        {
            return xgl_print_xgl_depth_stencil_view_create_info((XGL_DEPTH_STENCIL_VIEW_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_REGION_CREATE_INFO:
        {
            return xgl_print_xgl_descriptor_region_create_info((XGL_DESCRIPTOR_REGION_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO:
        {
            return xgl_print_xgl_descriptor_set_layout_create_info((XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_DEVICE_CREATE_INFO:
        {
            return xgl_print_xgl_device_create_info((XGL_DEVICE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_dynamic_cb_state_create_info((XGL_DYNAMIC_CB_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_dynamic_ds_state_create_info((XGL_DYNAMIC_DS_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_dynamic_rs_state_create_info((XGL_DYNAMIC_RS_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_dynamic_vp_state_create_info((XGL_DYNAMIC_VP_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_EVENT_CREATE_INFO:
        {
            return xgl_print_xgl_event_create_info((XGL_EVENT_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_EVENT_WAIT_INFO:
        {
            return xgl_print_xgl_event_wait_info((XGL_EVENT_WAIT_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO:
        {
            return xgl_print_xgl_fence_create_info((XGL_FENCE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO:
        {
            return xgl_print_xgl_framebuffer_create_info((XGL_FRAMEBUFFER_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO:
        {
            return xgl_print_xgl_graphics_pipeline_create_info((XGL_GRAPHICS_PIPELINE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO:
        {
            return xgl_print_xgl_image_create_info((XGL_IMAGE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
        {
            return xgl_print_xgl_image_memory_barrier((XGL_IMAGE_MEMORY_BARRIER*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_ATTACH_INFO:
        {
            return xgl_print_xgl_image_view_attach_info((XGL_IMAGE_VIEW_ATTACH_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO:
        {
            return xgl_print_xgl_image_view_create_info((XGL_IMAGE_VIEW_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO:
        {
            return xgl_print_xgl_layer_create_info((XGL_LAYER_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_BUFFER_INFO:
        {
            return xgl_print_xgl_memory_alloc_buffer_info((XGL_MEMORY_ALLOC_BUFFER_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO:
        {
            return xgl_print_xgl_memory_alloc_image_info((XGL_MEMORY_ALLOC_IMAGE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO:
        {
            return xgl_print_xgl_memory_alloc_info((XGL_MEMORY_ALLOC_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_BARRIER:
        {
            return xgl_print_xgl_memory_barrier((XGL_MEMORY_BARRIER*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_MEMORY_OPEN_INFO:
        {
            return xgl_print_xgl_memory_open_info((XGL_MEMORY_OPEN_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PEER_MEMORY_OPEN_INFO:
        {
            return xgl_print_xgl_peer_memory_open_info((XGL_PEER_MEMORY_OPEN_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_BARRIER:
        {
            return xgl_print_xgl_pipeline_barrier((XGL_PIPELINE_BARRIER*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_pipeline_cb_state_create_info((XGL_PIPELINE_CB_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_pipeline_ds_state_create_info((XGL_PIPELINE_DS_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_pipeline_ia_state_create_info((XGL_PIPELINE_IA_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_pipeline_ms_state_create_info((XGL_PIPELINE_MS_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_pipeline_rs_state_create_info((XGL_PIPELINE_RS_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO:
        {
            return xgl_print_xgl_pipeline_shader_stage_create_info((XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_TESS_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_pipeline_tess_state_create_info((XGL_PIPELINE_TESS_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_CREATE_INFO:
        {
            return xgl_print_xgl_pipeline_vertex_input_create_info((XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO:
        {
            return xgl_print_xgl_pipeline_vp_state_create_info((XGL_PIPELINE_VP_STATE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO:
        {
            return xgl_print_xgl_query_pool_create_info((XGL_QUERY_POOL_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_CREATE_INFO:
        {
            return xgl_print_xgl_queue_semaphore_create_info((XGL_QUEUE_SEMAPHORE_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_QUEUE_SEMAPHORE_OPEN_INFO:
        {
            return xgl_print_xgl_queue_semaphore_open_info((XGL_QUEUE_SEMAPHORE_OPEN_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO:
        {
            return xgl_print_xgl_render_pass_create_info((XGL_RENDER_PASS_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_SAMPLER_CREATE_INFO:
        {
            return xgl_print_xgl_sampler_create_info((XGL_SAMPLER_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_SHADER_CREATE_INFO:
        {
            return xgl_print_xgl_shader_create_info((XGL_SHADER_CREATE_INFO*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
        {
            return xgl_print_xgl_update_as_copy((XGL_UPDATE_AS_COPY*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
        {
            return xgl_print_xgl_update_buffers((XGL_UPDATE_BUFFERS*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
        {
            return xgl_print_xgl_update_images((XGL_UPDATE_IMAGES*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
        {
            return xgl_print_xgl_update_samplers((XGL_UPDATE_SAMPLERS*)pStruct, indent);
        }
        break;
        case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
        {
            return xgl_print_xgl_update_sampler_textures((XGL_UPDATE_SAMPLER_TEXTURES*)pStruct, indent);
        }
        break;
        default:
        return NULL;
    }
}