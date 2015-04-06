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
#include "xgl_enum_string_helper.h"

// Function Prototypes
char* dynamic_display(const void* pStruct, const char* prefix);
char* xgl_print_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct, const char* prefix);
char* xgl_print_xgl_application_info(const XGL_APPLICATION_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_buffer_copy(const XGL_BUFFER_COPY* pStruct, const char* prefix);
char* xgl_print_xgl_buffer_create_info(const XGL_BUFFER_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_buffer_image_copy(const XGL_BUFFER_IMAGE_COPY* pStruct, const char* prefix);
char* xgl_print_xgl_buffer_memory_barrier(const XGL_BUFFER_MEMORY_BARRIER* pStruct, const char* prefix);
char* xgl_print_xgl_buffer_memory_requirements(const XGL_BUFFER_MEMORY_REQUIREMENTS* pStruct, const char* prefix);
char* xgl_print_xgl_buffer_view_attach_info(const XGL_BUFFER_VIEW_ATTACH_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_buffer_view_create_info(const XGL_BUFFER_VIEW_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct, const char* prefix);
char* xgl_print_xgl_clear_color(const XGL_CLEAR_COLOR* pStruct, const char* prefix);
char* xgl_print_xgl_clear_color_value(const XGL_CLEAR_COLOR_VALUE* pStruct, const char* prefix);
char* xgl_print_xgl_cmd_buffer_begin_info(const XGL_CMD_BUFFER_BEGIN_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_cmd_buffer_graphics_begin_info(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_descriptor_region_create_info(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_descriptor_set_layout_create_info(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_descriptor_type_count(const XGL_DESCRIPTOR_TYPE_COUNT* pStruct, const char* prefix);
char* xgl_print_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct, const char* prefix);
char* xgl_print_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct, const char* prefix);
char* xgl_print_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct, const char* prefix);
char* xgl_print_xgl_dynamic_cb_state_create_info(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_dynamic_ds_state_create_info(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_dynamic_rs_state_create_info(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_dynamic_vp_state_create_info(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_event_wait_info(const XGL_EVENT_WAIT_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_extent2d(const XGL_EXTENT2D* pStruct, const char* prefix);
char* xgl_print_xgl_extent3d(const XGL_EXTENT3D* pStruct, const char* prefix);
char* xgl_print_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct, const char* prefix);
char* xgl_print_xgl_framebuffer_create_info(const XGL_FRAMEBUFFER_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_image_copy(const XGL_IMAGE_COPY* pStruct, const char* prefix);
char* xgl_print_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_image_memory_barrier(const XGL_IMAGE_MEMORY_BARRIER* pStruct, const char* prefix);
char* xgl_print_xgl_image_memory_bind_info(const XGL_IMAGE_MEMORY_BIND_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_image_memory_requirements(const XGL_IMAGE_MEMORY_REQUIREMENTS* pStruct, const char* prefix);
char* xgl_print_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct, const char* prefix);
char* xgl_print_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct, const char* prefix);
char* xgl_print_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct, const char* prefix);
char* xgl_print_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_layer_create_info(const XGL_LAYER_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct, const char* prefix);
char* xgl_print_xgl_memory_alloc_buffer_info(const XGL_MEMORY_ALLOC_BUFFER_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_memory_alloc_image_info(const XGL_MEMORY_ALLOC_IMAGE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_memory_barrier(const XGL_MEMORY_BARRIER* pStruct, const char* prefix);
char* xgl_print_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_memory_ref(const XGL_MEMORY_REF* pStruct, const char* prefix);
char* xgl_print_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct, const char* prefix);
char* xgl_print_xgl_offset2d(const XGL_OFFSET2D* pStruct, const char* prefix);
char* xgl_print_xgl_offset3d(const XGL_OFFSET3D* pStruct, const char* prefix);
char* xgl_print_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct, const char* prefix);
char* xgl_print_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct, const char* prefix);
char* xgl_print_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct, const char* prefix);
char* xgl_print_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_barrier(const XGL_PIPELINE_BARRIER* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_ds_state_create_info(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_ms_state_create_info(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_vertex_input_create_info(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_pipeline_vp_state_create_info(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_rect(const XGL_RECT* pStruct, const char* prefix);
char* xgl_print_xgl_render_pass_create_info(const XGL_RENDER_PASS_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_sampler_image_view_info(const XGL_SAMPLER_IMAGE_VIEW_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct, const char* prefix);
char* xgl_print_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct, const char* prefix);
char* xgl_print_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct, const char* prefix);
char* xgl_print_xgl_update_as_copy(const XGL_UPDATE_AS_COPY* pStruct, const char* prefix);
char* xgl_print_xgl_update_buffers(const XGL_UPDATE_BUFFERS* pStruct, const char* prefix);
char* xgl_print_xgl_update_images(const XGL_UPDATE_IMAGES* pStruct, const char* prefix);
char* xgl_print_xgl_update_samplers(const XGL_UPDATE_SAMPLERS* pStruct, const char* prefix);
char* xgl_print_xgl_update_sampler_textures(const XGL_UPDATE_SAMPLER_TEXTURES* pStruct, const char* prefix);
char* xgl_print_xgl_vertex_input_attribute_description(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pStruct, const char* prefix);
char* xgl_print_xgl_vertex_input_binding_description(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pStruct, const char* prefix);
char* xgl_print_xgl_viewport(const XGL_VIEWPORT* pStruct, const char* prefix);

#if defined(_WIN32)
// Microsoft did not implement C99 in Visual Studio; but started adding it with
// VS2013.  However, VS2013 still did not have snprintf().  The following is a
// work-around.
#define snprintf _snprintf
#endif // _WIN32

char* xgl_print_xgl_alloc_callbacks(const XGL_ALLOC_CALLBACKS* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%spUserData = %p\n%spfnAlloc = %p\n%spfnFree = %p\n", prefix, (pStruct->pUserData), prefix, (void*)(pStruct->pfnAlloc), prefix, (void*)(pStruct->pfnFree));
    return str;
}
char* xgl_print_xgl_application_info(const XGL_APPLICATION_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%spAppName = %p\n%sappVersion = %u\n%spEngineName = %p\n%sengineVersion = %u\n%sapiVersion = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->pAppName), prefix, (pStruct->appVersion), prefix, (pStruct->pEngineName), prefix, (pStruct->engineVersion), prefix, (pStruct->apiVersion));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_buffer_copy(const XGL_BUFFER_COPY* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssrcOffset = %p\n%sdestOffset = %p\n%scopySize = %p\n", prefix, (void*)(pStruct->srcOffset), prefix, (void*)(pStruct->destOffset), prefix, (void*)(pStruct->copySize));
    return str;
}
char* xgl_print_xgl_buffer_create_info(const XGL_BUFFER_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%ssize = %p\n%susage = %u\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->size), prefix, (pStruct->usage), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_buffer_image_copy(const XGL_BUFFER_IMAGE_COPY* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    tmpStr = xgl_print_xgl_image_subresource(&pStruct->imageSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %simageSubresource (%p)\n%s", prefix, (void*)&pStruct->imageSubresource, tmpStr);
    tmpStr = xgl_print_xgl_offset3d(&pStruct->imageOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %simageOffset (%p)\n%s", prefix, (void*)&pStruct->imageOffset, tmpStr);
    tmpStr = xgl_print_xgl_extent3d(&pStruct->imageExtent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %simageExtent (%p)\n%s", prefix, (void*)&pStruct->imageExtent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sbufferOffset = %p\n%simageSubresource = %p\n%simageOffset = %p\n%simageExtent = %p\n", prefix, (void*)(pStruct->bufferOffset), prefix, (void*)&(pStruct->imageSubresource), prefix, (void*)&(pStruct->imageOffset), prefix, (void*)&(pStruct->imageExtent));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_buffer_memory_barrier(const XGL_BUFFER_MEMORY_BARRIER* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%soutputMask = %u\n%sinputMask = %u\n%sbuffer = %p\n%soffset = %p\n%ssize = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->outputMask), prefix, (pStruct->inputMask), prefix, (void*)(pStruct->buffer), prefix, (void*)(pStruct->offset), prefix, (void*)(pStruct->size));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_buffer_memory_requirements(const XGL_BUFFER_MEMORY_REQUIREMENTS* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%susage = %u\n", prefix, (pStruct->usage));
    return str;
}
char* xgl_print_xgl_buffer_view_attach_info(const XGL_BUFFER_VIEW_ATTACH_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sview = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->view));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_buffer_view_create_info(const XGL_BUFFER_VIEW_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_channel_mapping(&pStruct->channels, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %schannels (%p)\n%s", prefix, (void*)&pStruct->channels, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sbuffer = %p\n%sviewType = %s\n%sstride = %p\n%sformat = %s\n%schannels = %p\n%soffset = %p\n%srange = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->buffer), prefix, string_XGL_BUFFER_VIEW_TYPE(pStruct->viewType), prefix, (void*)(pStruct->stride), prefix, string_XGL_FORMAT(pStruct->format), prefix, (void*)&(pStruct->channels), prefix, (void*)(pStruct->offset), prefix, (void*)(pStruct->range));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_channel_mapping(const XGL_CHANNEL_MAPPING* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sr = %s\n%sg = %s\n%sb = %s\n%sa = %s\n", prefix, string_XGL_CHANNEL_SWIZZLE(pStruct->r), prefix, string_XGL_CHANNEL_SWIZZLE(pStruct->g), prefix, string_XGL_CHANNEL_SWIZZLE(pStruct->b), prefix, string_XGL_CHANNEL_SWIZZLE(pStruct->a));
    return str;
}
char* xgl_print_xgl_clear_color(const XGL_CLEAR_COLOR* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    tmpStr = xgl_print_xgl_clear_color_value(&pStruct->color, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %scolor (%p)\n%s", prefix, (void*)&pStruct->color, tmpStr);
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%scolor = %p\n%suseRawValue = %s\n", prefix, (void*)&(pStruct->color), prefix, (pStruct->useRawValue) ? "TRUE" : "FALSE");
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_clear_color_value(const XGL_CLEAR_COLOR_VALUE* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sfloatColor = %p\n%srawColor = %p\n", prefix, (void*)(pStruct->floatColor), prefix, (void*)(pStruct->rawColor));
    return str;
}
char* xgl_print_xgl_cmd_buffer_begin_info(const XGL_CMD_BUFFER_BEGIN_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_cmd_buffer_create_info(const XGL_CMD_BUFFER_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%squeueType = %s\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_QUEUE_TYPE(pStruct->queueType), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_cmd_buffer_graphics_begin_info(const XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%srenderPass = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->renderPass));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_color_attachment_bind_info(const XGL_COLOR_ATTACHMENT_BIND_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sview = %p\n%slayout = %s\n", prefix, (void*)(pStruct->view), prefix, string_XGL_IMAGE_LAYOUT(pStruct->layout));
    return str;
}
char* xgl_print_xgl_color_attachment_view_create_info(const XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_image_subresource_range(&pStruct->msaaResolveSubResource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %smsaaResolveSubResource (%p)\n%s", prefix, (void*)&pStruct->msaaResolveSubResource, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%simage = %p\n%sformat = %s\n%smipLevel = %u\n%sbaseArraySlice = %u\n%sarraySize = %u\n%smsaaResolveImage = %p\n%smsaaResolveSubResource = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->image), prefix, string_XGL_FORMAT(pStruct->format), prefix, (pStruct->mipLevel), prefix, (pStruct->baseArraySlice), prefix, (pStruct->arraySize), prefix, (void*)(pStruct->msaaResolveImage), prefix, (void*)&(pStruct->msaaResolveSubResource));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_compute_pipeline_create_info(const XGL_COMPUTE_PIPELINE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_pipeline_shader(&pStruct->cs, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %scs (%p)\n%s", prefix, (void*)&pStruct->cs, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%scs = %p\n%sflags = %u\n%slastSetLayout = %p\n%slocalSizeX = %u\n%slocalSizeY = %u\n%slocalSizeZ = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)&(pStruct->cs), prefix, (pStruct->flags), prefix, (void*)(pStruct->lastSetLayout), prefix, (pStruct->localSizeX), prefix, (pStruct->localSizeY), prefix, (pStruct->localSizeZ));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_depth_stencil_bind_info(const XGL_DEPTH_STENCIL_BIND_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sview = %p\n%slayout = %s\n", prefix, (void*)(pStruct->view), prefix, string_XGL_IMAGE_LAYOUT(pStruct->layout));
    return str;
}
char* xgl_print_xgl_depth_stencil_view_create_info(const XGL_DEPTH_STENCIL_VIEW_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_image_subresource_range(&pStruct->msaaResolveSubResource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %smsaaResolveSubResource (%p)\n%s", prefix, (void*)&pStruct->msaaResolveSubResource, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%simage = %p\n%smipLevel = %u\n%sbaseArraySlice = %u\n%sarraySize = %u\n%smsaaResolveImage = %p\n%smsaaResolveSubResource = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->image), prefix, (pStruct->mipLevel), prefix, (pStruct->baseArraySlice), prefix, (pStruct->arraySize), prefix, (void*)(pStruct->msaaResolveImage), prefix, (void*)&(pStruct->msaaResolveSubResource), prefix, (pStruct->flags));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_descriptor_region_create_info(const XGL_DESCRIPTOR_REGION_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pTypeCount) {
        tmpStr = xgl_print_xgl_descriptor_type_count(pStruct->pTypeCount, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spTypeCount (%p)\n%s", prefix, (void*)pStruct->pTypeCount, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%scount = %u\n%spTypeCount = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->count), prefix, (void*)(pStruct->pTypeCount));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_descriptor_set_layout_create_info(const XGL_DESCRIPTOR_SET_LAYOUT_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sdescriptorType = %s\n%scount = %u\n%sstageFlags = %u\n%simmutableSampler = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType), prefix, (pStruct->count), prefix, (pStruct->stageFlags), prefix, (void*)(pStruct->immutableSampler));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_descriptor_type_count(const XGL_DESCRIPTOR_TYPE_COUNT* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%stype = %s\n%scount = %u\n", prefix, string_XGL_DESCRIPTOR_TYPE(pStruct->type), prefix, (pStruct->count));
    return str;
}
char* xgl_print_xgl_device_create_info(const XGL_DEVICE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pRequestedQueues) {
        tmpStr = xgl_print_xgl_device_queue_create_info(pStruct->pRequestedQueues, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spRequestedQueues (%p)\n%s", prefix, (void*)pStruct->pRequestedQueues, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%squeueRecordCount = %u\n%spRequestedQueues = %p\n%sextensionCount = %u\n%sppEnabledExtensionNames = %s\n%smaxValidationLevel = %s\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->queueRecordCount), prefix, (void*)(pStruct->pRequestedQueues), prefix, (pStruct->extensionCount), prefix, (pStruct->ppEnabledExtensionNames)[0], prefix, string_XGL_VALIDATION_LEVEL(pStruct->maxValidationLevel), prefix, (pStruct->flags));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_device_queue_create_info(const XGL_DEVICE_QUEUE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%squeueNodeIndex = %u\n%squeueCount = %u\n", prefix, (pStruct->queueNodeIndex), prefix, (pStruct->queueCount));
    return str;
}
char* xgl_print_xgl_dispatch_indirect_cmd(const XGL_DISPATCH_INDIRECT_CMD* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sx = %u\n%sy = %u\n%sz = %u\n", prefix, (pStruct->x), prefix, (pStruct->y), prefix, (pStruct->z));
    return str;
}
char* xgl_print_xgl_draw_indexed_indirect_cmd(const XGL_DRAW_INDEXED_INDIRECT_CMD* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sindexCount = %u\n%sinstanceCount = %u\n%sfirstIndex = %u\n%svertexOffset = %i\n%sfirstInstance = %u\n", prefix, (pStruct->indexCount), prefix, (pStruct->instanceCount), prefix, (pStruct->firstIndex), prefix, (pStruct->vertexOffset), prefix, (pStruct->firstInstance));
    return str;
}
char* xgl_print_xgl_draw_indirect_cmd(const XGL_DRAW_INDIRECT_CMD* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%svertexCount = %u\n%sinstanceCount = %u\n%sfirstVertex = %u\n%sfirstInstance = %u\n", prefix, (pStruct->vertexCount), prefix, (pStruct->instanceCount), prefix, (pStruct->firstVertex), prefix, (pStruct->firstInstance));
    return str;
}
char* xgl_print_xgl_dynamic_cb_state_create_info(const XGL_DYNAMIC_CB_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sblendConst = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->blendConst));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_dynamic_ds_state_create_info(const XGL_DYNAMIC_DS_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sminDepth = %f\n%smaxDepth = %f\n%sstencilReadMask = %u\n%sstencilWriteMask = %u\n%sstencilFrontRef = %u\n%sstencilBackRef = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->minDepth), prefix, (pStruct->maxDepth), prefix, (pStruct->stencilReadMask), prefix, (pStruct->stencilWriteMask), prefix, (pStruct->stencilFrontRef), prefix, (pStruct->stencilBackRef));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_dynamic_rs_state_create_info(const XGL_DYNAMIC_RS_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sdepthBias = %f\n%sdepthBiasClamp = %f\n%sslopeScaledDepthBias = %f\n%spointSize = %f\n%spointFadeThreshold = %f\n%slineWidth = %f\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->depthBias), prefix, (pStruct->depthBiasClamp), prefix, (pStruct->slopeScaledDepthBias), prefix, (pStruct->pointSize), prefix, (pStruct->pointFadeThreshold), prefix, (pStruct->lineWidth));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_dynamic_vp_state_create_info(const XGL_DYNAMIC_VP_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pViewports) {
        tmpStr = xgl_print_xgl_viewport(pStruct->pViewports, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spViewports (%p)\n%s", prefix, (void*)pStruct->pViewports, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pScissors) {
        tmpStr = xgl_print_xgl_rect(pStruct->pScissors, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spScissors (%p)\n%s", prefix, (void*)pStruct->pScissors, tmpStr);
    }
    else
        stp_strs[2] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sviewportAndScissorCount = %u\n%spViewports = %p\n%spScissors = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->viewportAndScissorCount), prefix, (void*)(pStruct->pViewports), prefix, (void*)(pStruct->pScissors));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_event_create_info(const XGL_EVENT_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_event_wait_info(const XGL_EVENT_WAIT_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%seventCount = %u\n%spEvents = %p\n%swaitEvent = %s\n%smemBarrierCount = %u\n%sppMemBarriers = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->eventCount), prefix, (void*)(pStruct->pEvents), prefix, string_XGL_WAIT_EVENT(pStruct->waitEvent), prefix, (pStruct->memBarrierCount), prefix, (void*)(pStruct->ppMemBarriers));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_extent2d(const XGL_EXTENT2D* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%swidth = %i\n%sheight = %i\n", prefix, (pStruct->width), prefix, (pStruct->height));
    return str;
}
char* xgl_print_xgl_extent3d(const XGL_EXTENT3D* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%swidth = %i\n%sheight = %i\n%sdepth = %i\n", prefix, (pStruct->width), prefix, (pStruct->height), prefix, (pStruct->depth));
    return str;
}
char* xgl_print_xgl_fence_create_info(const XGL_FENCE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_format_properties(const XGL_FORMAT_PROPERTIES* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%slinearTilingFeatures = %u\n%soptimalTilingFeatures = %u\n", prefix, (pStruct->linearTilingFeatures), prefix, (pStruct->optimalTilingFeatures));
    return str;
}
char* xgl_print_xgl_framebuffer_create_info(const XGL_FRAMEBUFFER_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pColorAttachments) {
        tmpStr = xgl_print_xgl_color_attachment_bind_info(pStruct->pColorAttachments, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spColorAttachments (%p)\n%s", prefix, (void*)pStruct->pColorAttachments, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pDepthStencilAttachment) {
        tmpStr = xgl_print_xgl_depth_stencil_bind_info(pStruct->pDepthStencilAttachment, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spDepthStencilAttachment (%p)\n%s", prefix, (void*)pStruct->pDepthStencilAttachment, tmpStr);
    }
    else
        stp_strs[2] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%scolorAttachmentCount = %u\n%spColorAttachments = %p\n%spDepthStencilAttachment = %p\n%ssampleCount = %u\n%swidth = %u\n%sheight = %u\n%slayers = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->colorAttachmentCount), prefix, (void*)(pStruct->pColorAttachments), prefix, (void*)(pStruct->pDepthStencilAttachment), prefix, (pStruct->sampleCount), prefix, (pStruct->width), prefix, (pStruct->height), prefix, (pStruct->layers));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_gpu_compatibility_info(const XGL_GPU_COMPATIBILITY_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%scompatibilityFlags = %u\n", prefix, (pStruct->compatibilityFlags));
    return str;
}
char* xgl_print_xgl_graphics_pipeline_create_info(const XGL_GRAPHICS_PIPELINE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sflags = %u\n%slastSetLayout = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->flags), prefix, (void*)(pStruct->lastSetLayout));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_image_copy(const XGL_IMAGE_COPY* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[5];
    tmpStr = xgl_print_xgl_image_subresource(&pStruct->srcSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %ssrcSubresource (%p)\n%s", prefix, (void*)&pStruct->srcSubresource, tmpStr);
    tmpStr = xgl_print_xgl_offset3d(&pStruct->srcOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %ssrcOffset (%p)\n%s", prefix, (void*)&pStruct->srcOffset, tmpStr);
    tmpStr = xgl_print_xgl_image_subresource(&pStruct->destSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sdestSubresource (%p)\n%s", prefix, (void*)&pStruct->destSubresource, tmpStr);
    tmpStr = xgl_print_xgl_offset3d(&pStruct->destOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[3] = (char*)malloc(len);
    snprintf(stp_strs[3], len, " %sdestOffset (%p)\n%s", prefix, (void*)&pStruct->destOffset, tmpStr);
    tmpStr = xgl_print_xgl_extent3d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[4] = (char*)malloc(len);
    snprintf(stp_strs[4], len, " %sextent (%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + strlen(stp_strs[4]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssrcSubresource = %p\n%ssrcOffset = %p\n%sdestSubresource = %p\n%sdestOffset = %p\n%sextent = %p\n", prefix, (void*)&(pStruct->srcSubresource), prefix, (void*)&(pStruct->srcOffset), prefix, (void*)&(pStruct->destSubresource), prefix, (void*)&(pStruct->destOffset), prefix, (void*)&(pStruct->extent));
    for (int32_t stp_index = 4; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_image_create_info(const XGL_IMAGE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_extent3d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sextent (%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%simageType = %s\n%sformat = %s\n%sextent = %p\n%smipLevels = %u\n%sarraySize = %u\n%ssamples = %u\n%stiling = %s\n%susage = %u\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_IMAGE_TYPE(pStruct->imageType), prefix, string_XGL_FORMAT(pStruct->format), prefix, (void*)&(pStruct->extent), prefix, (pStruct->mipLevels), prefix, (pStruct->arraySize), prefix, (pStruct->samples), prefix, string_XGL_IMAGE_TILING(pStruct->tiling), prefix, (pStruct->usage), prefix, (pStruct->flags));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_image_memory_barrier(const XGL_IMAGE_MEMORY_BARRIER* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_image_subresource_range(&pStruct->subresourceRange, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %ssubresourceRange (%p)\n%s", prefix, (void*)&pStruct->subresourceRange, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%soutputMask = %u\n%sinputMask = %u\n%soldLayout = %s\n%snewLayout = %s\n%simage = %p\n%ssubresourceRange = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->outputMask), prefix, (pStruct->inputMask), prefix, string_XGL_IMAGE_LAYOUT(pStruct->oldLayout), prefix, string_XGL_IMAGE_LAYOUT(pStruct->newLayout), prefix, (void*)(pStruct->image), prefix, (void*)&(pStruct->subresourceRange));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_image_memory_bind_info(const XGL_IMAGE_MEMORY_BIND_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    tmpStr = xgl_print_xgl_image_subresource(&pStruct->subresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %ssubresource (%p)\n%s", prefix, (void*)&pStruct->subresource, tmpStr);
    tmpStr = xgl_print_xgl_offset3d(&pStruct->offset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %soffset (%p)\n%s", prefix, (void*)&pStruct->offset, tmpStr);
    tmpStr = xgl_print_xgl_extent3d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sextent (%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssubresource = %p\n%soffset = %p\n%sextent = %p\n", prefix, (void*)&(pStruct->subresource), prefix, (void*)&(pStruct->offset), prefix, (void*)&(pStruct->extent));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_image_memory_requirements(const XGL_IMAGE_MEMORY_REQUIREMENTS* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%susage = %u\n%sformatClass = %s\n%ssamples = %u\n", prefix, (pStruct->usage), prefix, string_XGL_IMAGE_FORMAT_CLASS(pStruct->formatClass), prefix, (pStruct->samples));
    return str;
}
char* xgl_print_xgl_image_resolve(const XGL_IMAGE_RESOLVE* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[5];
    tmpStr = xgl_print_xgl_image_subresource(&pStruct->srcSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %ssrcSubresource (%p)\n%s", prefix, (void*)&pStruct->srcSubresource, tmpStr);
    tmpStr = xgl_print_xgl_offset2d(&pStruct->srcOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %ssrcOffset (%p)\n%s", prefix, (void*)&pStruct->srcOffset, tmpStr);
    tmpStr = xgl_print_xgl_image_subresource(&pStruct->destSubresource, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sdestSubresource (%p)\n%s", prefix, (void*)&pStruct->destSubresource, tmpStr);
    tmpStr = xgl_print_xgl_offset2d(&pStruct->destOffset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[3] = (char*)malloc(len);
    snprintf(stp_strs[3], len, " %sdestOffset (%p)\n%s", prefix, (void*)&pStruct->destOffset, tmpStr);
    tmpStr = xgl_print_xgl_extent2d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[4] = (char*)malloc(len);
    snprintf(stp_strs[4], len, " %sextent (%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + strlen(stp_strs[3]) + strlen(stp_strs[4]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssrcSubresource = %p\n%ssrcOffset = %p\n%sdestSubresource = %p\n%sdestOffset = %p\n%sextent = %p\n", prefix, (void*)&(pStruct->srcSubresource), prefix, (void*)&(pStruct->srcOffset), prefix, (void*)&(pStruct->destSubresource), prefix, (void*)&(pStruct->destOffset), prefix, (void*)&(pStruct->extent));
    for (int32_t stp_index = 4; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_image_subresource(const XGL_IMAGE_SUBRESOURCE* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%saspect = %s\n%smipLevel = %u\n%sarraySlice = %u\n", prefix, string_XGL_IMAGE_ASPECT(pStruct->aspect), prefix, (pStruct->mipLevel), prefix, (pStruct->arraySlice));
    return str;
}
char* xgl_print_xgl_image_subresource_range(const XGL_IMAGE_SUBRESOURCE_RANGE* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%saspect = %s\n%sbaseMipLevel = %u\n%smipLevels = %u\n%sbaseArraySlice = %u\n%sarraySize = %u\n", prefix, string_XGL_IMAGE_ASPECT(pStruct->aspect), prefix, (pStruct->baseMipLevel), prefix, (pStruct->mipLevels), prefix, (pStruct->baseArraySlice), prefix, (pStruct->arraySize));
    return str;
}
char* xgl_print_xgl_image_view_attach_info(const XGL_IMAGE_VIEW_ATTACH_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sview = %p\n%slayout = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->view), prefix, string_XGL_IMAGE_LAYOUT(pStruct->layout));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_image_view_create_info(const XGL_IMAGE_VIEW_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_channel_mapping(&pStruct->channels, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %schannels (%p)\n%s", prefix, (void*)&pStruct->channels, tmpStr);
    tmpStr = xgl_print_xgl_image_subresource_range(&pStruct->subresourceRange, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %ssubresourceRange (%p)\n%s", prefix, (void*)&pStruct->subresourceRange, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%simage = %p\n%sviewType = %s\n%sformat = %s\n%schannels = %p\n%ssubresourceRange = %p\n%sminLod = %f\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->image), prefix, string_XGL_IMAGE_VIEW_TYPE(pStruct->viewType), prefix, string_XGL_FORMAT(pStruct->format), prefix, (void*)&(pStruct->channels), prefix, (void*)&(pStruct->subresourceRange), prefix, (pStruct->minLod));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_layer_create_info(const XGL_LAYER_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%slayerCount = %u\n%sppActiveLayerNames = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->layerCount), prefix, (pStruct->ppActiveLayerNames)[0]);
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_link_const_buffer(const XGL_LINK_CONST_BUFFER* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sbufferId = %u\n%sbufferSize = %p\n%spBufferData = %p\n", prefix, (pStruct->bufferId), prefix, (void*)(pStruct->bufferSize), prefix, (pStruct->pBufferData));
    return str;
}
char* xgl_print_xgl_memory_alloc_buffer_info(const XGL_MEMORY_ALLOC_BUFFER_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%susage = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->usage));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_memory_alloc_image_info(const XGL_MEMORY_ALLOC_IMAGE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%susage = %u\n%sformatClass = %s\n%ssamples = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->usage), prefix, string_XGL_IMAGE_FORMAT_CLASS(pStruct->formatClass), prefix, (pStruct->samples));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_memory_alloc_info(const XGL_MEMORY_ALLOC_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sallocationSize = %p\n%smemProps = %u\n%smemType = %s\n%smemPriority = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->allocationSize), prefix, (pStruct->memProps), prefix, string_XGL_MEMORY_TYPE(pStruct->memType), prefix, string_XGL_MEMORY_PRIORITY(pStruct->memPriority));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_memory_barrier(const XGL_MEMORY_BARRIER* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%soutputMask = %u\n%sinputMask = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->outputMask), prefix, (pStruct->inputMask));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_memory_open_info(const XGL_MEMORY_OPEN_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%ssharedMem = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->sharedMem));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_memory_ref(const XGL_MEMORY_REF* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%smem = %p\n%sflags = %u\n", prefix, (void*)(pStruct->mem), prefix, (pStruct->flags));
    return str;
}
char* xgl_print_xgl_memory_requirements(const XGL_MEMORY_REQUIREMENTS* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssize = %p\n%salignment = %p\n%sgranularity = %p\n%smemProps = %u\n%smemType = %s\n", prefix, (void*)(pStruct->size), prefix, (void*)(pStruct->alignment), prefix, (void*)(pStruct->granularity), prefix, (pStruct->memProps), prefix, string_XGL_MEMORY_TYPE(pStruct->memType));
    return str;
}
char* xgl_print_xgl_offset2d(const XGL_OFFSET2D* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sx = %i\n%sy = %i\n", prefix, (pStruct->x), prefix, (pStruct->y));
    return str;
}
char* xgl_print_xgl_offset3d(const XGL_OFFSET3D* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sx = %i\n%sy = %i\n%sz = %i\n", prefix, (pStruct->x), prefix, (pStruct->y), prefix, (pStruct->z));
    return str;
}
char* xgl_print_xgl_peer_image_open_info(const XGL_PEER_IMAGE_OPEN_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%soriginalImage = %p\n", prefix, (void*)(pStruct->originalImage));
    return str;
}
char* xgl_print_xgl_peer_memory_open_info(const XGL_PEER_MEMORY_OPEN_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%soriginalMem = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->originalMem));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_physical_gpu_memory_properties(const XGL_PHYSICAL_GPU_MEMORY_PROPERTIES* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssupportsMigration = %s\n%ssupportsPinning = %s\n", prefix, (pStruct->supportsMigration) ? "TRUE" : "FALSE", prefix, (pStruct->supportsPinning) ? "TRUE" : "FALSE");
    return str;
}
char* xgl_print_xgl_physical_gpu_performance(const XGL_PHYSICAL_GPU_PERFORMANCE* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%smaxGpuClock = %f\n%saluPerClock = %f\n%stexPerClock = %f\n%sprimsPerClock = %f\n%spixelsPerClock = %f\n", prefix, (pStruct->maxGpuClock), prefix, (pStruct->aluPerClock), prefix, (pStruct->texPerClock), prefix, (pStruct->primsPerClock), prefix, (pStruct->pixelsPerClock));
    return str;
}
char* xgl_print_xgl_physical_gpu_properties(const XGL_PHYSICAL_GPU_PROPERTIES* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sapiVersion = %u\n%sdriverVersion = %u\n%svendorId = %u\n%sdeviceId = %u\n%sgpuType = %s\n%sgpuName = %s\n%smaxMemRefsPerSubmission = %u\n%smaxInlineMemoryUpdateSize = %p\n%smaxBoundDescriptorSets = %u\n%smaxThreadGroupSize = %u\n%stimestampFrequency = %lu\n%smultiColorAttachmentClears = %s\n%smaxDescriptorSets = %u\n%smaxViewports = %u\n%smaxColorAttachments = %u\n", prefix, (pStruct->apiVersion), prefix, (pStruct->driverVersion), prefix, (pStruct->vendorId), prefix, (pStruct->deviceId), prefix, string_XGL_PHYSICAL_GPU_TYPE(pStruct->gpuType), prefix, (pStruct->gpuName), prefix, (pStruct->maxMemRefsPerSubmission), prefix, (void*)(pStruct->maxInlineMemoryUpdateSize), prefix, (pStruct->maxBoundDescriptorSets), prefix, (pStruct->maxThreadGroupSize), prefix, (pStruct->timestampFrequency), prefix, (pStruct->multiColorAttachmentClears) ? "TRUE" : "FALSE", prefix, (pStruct->maxDescriptorSets), prefix, (pStruct->maxViewports), prefix, (pStruct->maxColorAttachments));
    return str;
}
char* xgl_print_xgl_physical_gpu_queue_properties(const XGL_PHYSICAL_GPU_QUEUE_PROPERTIES* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%squeueFlags = %u\n%squeueCount = %u\n%smaxAtomicCounters = %u\n%ssupportsTimestamps = %s\n", prefix, (pStruct->queueFlags), prefix, (pStruct->queueCount), prefix, (pStruct->maxAtomicCounters), prefix, (pStruct->supportsTimestamps) ? "TRUE" : "FALSE");
    return str;
}
char* xgl_print_xgl_pipeline_barrier(const XGL_PIPELINE_BARRIER* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%seventCount = %u\n%spEvents = %p\n%swaitEvent = %s\n%smemBarrierCount = %u\n%sppMemBarriers = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->eventCount), prefix, (void*)(pStruct->pEvents), prefix, string_XGL_WAIT_EVENT(pStruct->waitEvent), prefix, (pStruct->memBarrierCount), prefix, (void*)(pStruct->ppMemBarriers));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_cb_attachment_state(const XGL_PIPELINE_CB_ATTACHMENT_STATE* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sblendEnable = %s\n%sformat = %s\n%ssrcBlendColor = %s\n%sdestBlendColor = %s\n%sblendFuncColor = %s\n%ssrcBlendAlpha = %s\n%sdestBlendAlpha = %s\n%sblendFuncAlpha = %s\n%schannelWriteMask = %hu\n", prefix, (pStruct->blendEnable) ? "TRUE" : "FALSE", prefix, string_XGL_FORMAT(pStruct->format), prefix, string_XGL_BLEND(pStruct->srcBlendColor), prefix, string_XGL_BLEND(pStruct->destBlendColor), prefix, string_XGL_BLEND_FUNC(pStruct->blendFuncColor), prefix, string_XGL_BLEND(pStruct->srcBlendAlpha), prefix, string_XGL_BLEND(pStruct->destBlendAlpha), prefix, string_XGL_BLEND_FUNC(pStruct->blendFuncAlpha), prefix, (pStruct->channelWriteMask));
    return str;
}
char* xgl_print_xgl_pipeline_cb_state_create_info(const XGL_PIPELINE_CB_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pAttachments) {
        tmpStr = xgl_print_xgl_pipeline_cb_attachment_state(pStruct->pAttachments, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spAttachments (%p)\n%s", prefix, (void*)pStruct->pAttachments, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%salphaToCoverageEnable = %s\n%slogicOpEnable = %s\n%slogicOp = %s\n%sattachmentCount = %u\n%spAttachments = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->alphaToCoverageEnable) ? "TRUE" : "FALSE", prefix, (pStruct->logicOpEnable) ? "TRUE" : "FALSE", prefix, string_XGL_LOGIC_OP(pStruct->logicOp), prefix, (pStruct->attachmentCount), prefix, (void*)(pStruct->pAttachments));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_ds_state_create_info(const XGL_PIPELINE_DS_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_stencil_op_state(&pStruct->front, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sfront (%p)\n%s", prefix, (void*)&pStruct->front, tmpStr);
    tmpStr = xgl_print_xgl_stencil_op_state(&pStruct->back, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[2] = (char*)malloc(len);
    snprintf(stp_strs[2], len, " %sback (%p)\n%s", prefix, (void*)&pStruct->back, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sformat = %s\n%sdepthTestEnable = %s\n%sdepthWriteEnable = %s\n%sdepthFunc = %s\n%sdepthBoundsEnable = %s\n%sstencilTestEnable = %s\n%sfront = %p\n%sback = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_FORMAT(pStruct->format), prefix, (pStruct->depthTestEnable) ? "TRUE" : "FALSE", prefix, (pStruct->depthWriteEnable) ? "TRUE" : "FALSE", prefix, string_XGL_COMPARE_FUNC(pStruct->depthFunc), prefix, (pStruct->depthBoundsEnable) ? "TRUE" : "FALSE", prefix, (pStruct->stencilTestEnable) ? "TRUE" : "FALSE", prefix, (void*)&(pStruct->front), prefix, (void*)&(pStruct->back));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_ia_state_create_info(const XGL_PIPELINE_IA_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%stopology = %s\n%sdisableVertexReuse = %s\n%sprimitiveRestartEnable = %s\n%sprimitiveRestartIndex = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_PRIMITIVE_TOPOLOGY(pStruct->topology), prefix, (pStruct->disableVertexReuse) ? "TRUE" : "FALSE", prefix, (pStruct->primitiveRestartEnable) ? "TRUE" : "FALSE", prefix, (pStruct->primitiveRestartIndex));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_ms_state_create_info(const XGL_PIPELINE_MS_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%ssamples = %u\n%smultisampleEnable = %s\n%ssampleShadingEnable = %s\n%sminSampleShading = %f\n%ssampleMask = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->samples), prefix, (pStruct->multisampleEnable) ? "TRUE" : "FALSE", prefix, (pStruct->sampleShadingEnable) ? "TRUE" : "FALSE", prefix, (pStruct->minSampleShading), prefix, (pStruct->sampleMask));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_rs_state_create_info(const XGL_PIPELINE_RS_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sdepthClipEnable = %s\n%srasterizerDiscardEnable = %s\n%sprogramPointSize = %s\n%spointOrigin = %s\n%sprovokingVertex = %s\n%sfillMode = %s\n%scullMode = %s\n%sfrontFace = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->depthClipEnable) ? "TRUE" : "FALSE", prefix, (pStruct->rasterizerDiscardEnable) ? "TRUE" : "FALSE", prefix, (pStruct->programPointSize) ? "TRUE" : "FALSE", prefix, string_XGL_COORDINATE_ORIGIN(pStruct->pointOrigin), prefix, string_XGL_PROVOKING_VERTEX_CONVENTION(pStruct->provokingVertex), prefix, string_XGL_FILL_MODE(pStruct->fillMode), prefix, string_XGL_CULL_MODE(pStruct->cullMode), prefix, string_XGL_FACE_ORIENTATION(pStruct->frontFace));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_shader(const XGL_PIPELINE_SHADER* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pLinkConstBufferInfo) {
        tmpStr = xgl_print_xgl_link_const_buffer(pStruct->pLinkConstBufferInfo, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spLinkConstBufferInfo (%p)\n%s", prefix, (void*)pStruct->pLinkConstBufferInfo, tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sstage = %s\n%sshader = %p\n%slinkConstBufferCount = %u\n%spLinkConstBufferInfo = %p\n", prefix, string_XGL_PIPELINE_SHADER_STAGE(pStruct->stage), prefix, (void*)(pStruct->shader), prefix, (pStruct->linkConstBufferCount), prefix, (void*)(pStruct->pLinkConstBufferInfo));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_shader_stage_create_info(const XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_pipeline_shader(&pStruct->shader, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sshader (%p)\n%s", prefix, (void*)&pStruct->shader, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sshader = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)&(pStruct->shader));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_statistics_data(const XGL_PIPELINE_STATISTICS_DATA* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sfsInvocations = %lu\n%scPrimitives = %lu\n%scInvocations = %lu\n%svsInvocations = %lu\n%sgsInvocations = %lu\n%sgsPrimitives = %lu\n%siaPrimitives = %lu\n%siaVertices = %lu\n%stcsInvocations = %lu\n%stesInvocations = %lu\n%scsInvocations = %lu\n", prefix, (pStruct->fsInvocations), prefix, (pStruct->cPrimitives), prefix, (pStruct->cInvocations), prefix, (pStruct->vsInvocations), prefix, (pStruct->gsInvocations), prefix, (pStruct->gsPrimitives), prefix, (pStruct->iaPrimitives), prefix, (pStruct->iaVertices), prefix, (pStruct->tcsInvocations), prefix, (pStruct->tesInvocations), prefix, (pStruct->csInvocations));
    return str;
}
char* xgl_print_xgl_pipeline_tess_state_create_info(const XGL_PIPELINE_TESS_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%spatchControlPoints = %u\n%soptimalTessFactor = %f\n%sfixedTessFactor = %f\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->patchControlPoints), prefix, (pStruct->optimalTessFactor), prefix, (pStruct->fixedTessFactor));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_vertex_input_create_info(const XGL_PIPELINE_VERTEX_INPUT_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pVertexBindingDescriptions) {
        tmpStr = xgl_print_xgl_vertex_input_binding_description(pStruct->pVertexBindingDescriptions, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spVertexBindingDescriptions (%p)\n%s", prefix, (void*)pStruct->pVertexBindingDescriptions, tmpStr);
    }
    else
        stp_strs[1] = "";
    if (pStruct->pVertexAttributeDescriptions) {
        tmpStr = xgl_print_xgl_vertex_input_attribute_description(pStruct->pVertexAttributeDescriptions, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spVertexAttributeDescriptions (%p)\n%s", prefix, (void*)pStruct->pVertexAttributeDescriptions, tmpStr);
    }
    else
        stp_strs[2] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sbindingCount = %u\n%spVertexBindingDescriptions = %p\n%sattributeCount = %u\n%spVertexAttributeDescriptions = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->bindingCount), prefix, (void*)(pStruct->pVertexBindingDescriptions), prefix, (pStruct->attributeCount), prefix, (void*)(pStruct->pVertexAttributeDescriptions));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_pipeline_vp_state_create_info(const XGL_PIPELINE_VP_STATE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%snumViewports = %u\n%sclipOrigin = %s\n%sdepthMode = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->numViewports), prefix, string_XGL_COORDINATE_ORIGIN(pStruct->clipOrigin), prefix, string_XGL_DEPTH_MODE(pStruct->depthMode));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_query_pool_create_info(const XGL_QUERY_POOL_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%squeryType = %s\n%sslots = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_QUERY_TYPE(pStruct->queryType), prefix, (pStruct->slots));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_queue_semaphore_create_info(const XGL_QUEUE_SEMAPHORE_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sinitialCount = %u\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->initialCount), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_queue_semaphore_open_info(const XGL_QUEUE_SEMAPHORE_OPEN_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%ssharedSemaphore = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->sharedSemaphore));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_rect(const XGL_RECT* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    tmpStr = xgl_print_xgl_offset2d(&pStruct->offset, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[0] = (char*)malloc(len);
    snprintf(stp_strs[0], len, " %soffset (%p)\n%s", prefix, (void*)&pStruct->offset, tmpStr);
    tmpStr = xgl_print_xgl_extent2d(&pStruct->extent, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %sextent (%p)\n%s", prefix, (void*)&pStruct->extent, tmpStr);
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%soffset = %p\n%sextent = %p\n", prefix, (void*)&(pStruct->offset), prefix, (void*)&(pStruct->extent));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_render_pass_create_info(const XGL_RENDER_PASS_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[3];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    tmpStr = xgl_print_xgl_rect(&pStruct->renderArea, extra_indent);
    len = 256+strlen(tmpStr);
    stp_strs[1] = (char*)malloc(len);
    snprintf(stp_strs[1], len, " %srenderArea (%p)\n%s", prefix, (void*)&pStruct->renderArea, tmpStr);
    if (pStruct->pColorLoadClearValues) {
        tmpStr = xgl_print_xgl_clear_color(pStruct->pColorLoadClearValues, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[2] = (char*)malloc(len);
        snprintf(stp_strs[2], len, " %spColorLoadClearValues (%p)\n%s", prefix, (void*)pStruct->pColorLoadClearValues, tmpStr);
    }
    else
        stp_strs[2] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + strlen(stp_strs[2]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%srenderArea = %p\n%sframebuffer = %p\n%scolorAttachmentCount = %u\n%spColorLoadOps = %p\n%spColorStoreOps = %p\n%spColorLoadClearValues = %p\n%sdepthLoadOp = %s\n%sdepthLoadClearValue = %f\n%sdepthStoreOp = %s\n%sstencilLoadOp = %s\n%sstencilLoadClearValue = %u\n%sstencilStoreOp = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)&(pStruct->renderArea), prefix, (void*)(pStruct->framebuffer), prefix, (pStruct->colorAttachmentCount), prefix, (void*)(pStruct->pColorLoadOps), prefix, (void*)(pStruct->pColorStoreOps), prefix, (void*)(pStruct->pColorLoadClearValues), prefix, string_XGL_ATTACHMENT_LOAD_OP(pStruct->depthLoadOp), prefix, (pStruct->depthLoadClearValue), prefix, string_XGL_ATTACHMENT_STORE_OP(pStruct->depthStoreOp), prefix, string_XGL_ATTACHMENT_LOAD_OP(pStruct->stencilLoadOp), prefix, (pStruct->stencilLoadClearValue), prefix, string_XGL_ATTACHMENT_STORE_OP(pStruct->stencilStoreOp));
    for (int32_t stp_index = 2; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_sampler_create_info(const XGL_SAMPLER_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%smagFilter = %s\n%sminFilter = %s\n%smipMode = %s\n%saddressU = %s\n%saddressV = %s\n%saddressW = %s\n%smipLodBias = %f\n%smaxAnisotropy = %u\n%scompareFunc = %s\n%sminLod = %f\n%smaxLod = %f\n%sborderColorType = %s\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_TEX_FILTER(pStruct->magFilter), prefix, string_XGL_TEX_FILTER(pStruct->minFilter), prefix, string_XGL_TEX_MIPMAP_MODE(pStruct->mipMode), prefix, string_XGL_TEX_ADDRESS(pStruct->addressU), prefix, string_XGL_TEX_ADDRESS(pStruct->addressV), prefix, string_XGL_TEX_ADDRESS(pStruct->addressW), prefix, (pStruct->mipLodBias), prefix, (pStruct->maxAnisotropy), prefix, string_XGL_COMPARE_FUNC(pStruct->compareFunc), prefix, (pStruct->minLod), prefix, (pStruct->maxLod), prefix, string_XGL_BORDER_COLOR_TYPE(pStruct->borderColorType));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_sampler_image_view_info(const XGL_SAMPLER_IMAGE_VIEW_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pImageView) {
        tmpStr = xgl_print_xgl_image_view_attach_info(pStruct->pImageView, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spImageView (%p)\n%s", prefix, (void*)pStruct->pImageView, tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%spSampler = %p\n%spImageView = %p\n", prefix, (void*)(pStruct->pSampler), prefix, (void*)(pStruct->pImageView));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_shader_create_info(const XGL_SHADER_CREATE_INFO* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%scodeSize = %p\n%spCode = %p\n%sflags = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (void*)(pStruct->codeSize), prefix, (pStruct->pCode), prefix, (pStruct->flags));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_stencil_op_state(const XGL_STENCIL_OP_STATE* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sstencilFailOp = %s\n%sstencilPassOp = %s\n%sstencilDepthFailOp = %s\n%sstencilFunc = %s\n", prefix, string_XGL_STENCIL_OP(pStruct->stencilFailOp), prefix, string_XGL_STENCIL_OP(pStruct->stencilPassOp), prefix, string_XGL_STENCIL_OP(pStruct->stencilDepthFailOp), prefix, string_XGL_COMPARE_FUNC(pStruct->stencilFunc));
    return str;
}
char* xgl_print_xgl_subresource_layout(const XGL_SUBRESOURCE_LAYOUT* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%soffset = %p\n%ssize = %p\n%srowPitch = %p\n%sdepthPitch = %p\n", prefix, (void*)(pStruct->offset), prefix, (void*)(pStruct->size), prefix, (void*)(pStruct->rowPitch), prefix, (void*)(pStruct->depthPitch));
    return str;
}
char* xgl_print_xgl_update_as_copy(const XGL_UPDATE_AS_COPY* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sdescriptorType = %s\n%sdescriptorSet = %p\n%sdescriptorIndex = %u\n%scount = %u\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType), prefix, (void*)(pStruct->descriptorSet), prefix, (pStruct->descriptorIndex), prefix, (pStruct->count));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_update_buffers(const XGL_UPDATE_BUFFERS* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pBufferViews) {
        tmpStr = xgl_print_xgl_buffer_view_attach_info(pStruct->pBufferViews[0], extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spBufferViews (%p)\n%s", prefix, (void*)pStruct->pBufferViews, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sdescriptorType = %s\n%sindex = %u\n%scount = %u\n%spBufferViews = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType), prefix, (pStruct->index), prefix, (pStruct->count), prefix, (void*)(pStruct->pBufferViews));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_update_images(const XGL_UPDATE_IMAGES* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pImageViews) {
        tmpStr = xgl_print_xgl_image_view_attach_info(pStruct->pImageViews[0], extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spImageViews (%p)\n%s", prefix, (void*)pStruct->pImageViews, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sdescriptorType = %s\n%sindex = %u\n%scount = %u\n%spImageViews = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, string_XGL_DESCRIPTOR_TYPE(pStruct->descriptorType), prefix, (pStruct->index), prefix, (pStruct->count), prefix, (void*)(pStruct->pImageViews));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_update_samplers(const XGL_UPDATE_SAMPLERS* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[1];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    len = strlen(stp_strs[0]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sindex = %u\n%scount = %u\n%spSamplers = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->index), prefix, (pStruct->count), prefix, (void*)(pStruct->pSamplers));
    for (int32_t stp_index = 0; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_update_sampler_textures(const XGL_UPDATE_SAMPLER_TEXTURES* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    char* tmpStr;
    char* extra_indent = (char*)malloc(strlen(prefix) + 3);
    strcpy(extra_indent, "  ");
    strncat(extra_indent, prefix, strlen(prefix));
    char* stp_strs[2];
    if (pStruct->pNext) {
        tmpStr = dynamic_display((void*)pStruct->pNext, prefix);
        len = 256+strlen(tmpStr);
        stp_strs[0] = (char*)malloc(len);
        snprintf(stp_strs[0], len, " %spNext (%p)\n%s", prefix, (void*)pStruct->pNext, tmpStr);
        free(tmpStr);
    }
    else
        stp_strs[0] = "";
    if (pStruct->pSamplerImageViews) {
        tmpStr = xgl_print_xgl_sampler_image_view_info(pStruct->pSamplerImageViews, extra_indent);
        len = 256+strlen(tmpStr)+strlen(prefix);
        stp_strs[1] = (char*)malloc(len);
        snprintf(stp_strs[1], len, " %spSamplerImageViews (%p)\n%s", prefix, (void*)pStruct->pSamplerImageViews, tmpStr);
    }
    else
        stp_strs[1] = "";
    len = strlen(stp_strs[0]) + strlen(stp_strs[1]) + sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%ssType = %s\n%spNext = %p\n%sindex = %u\n%scount = %u\n%spSamplerImageViews = %p\n", prefix, string_XGL_STRUCTURE_TYPE(pStruct->sType), prefix, (pStruct->pNext), prefix, (pStruct->index), prefix, (pStruct->count), prefix, (void*)(pStruct->pSamplerImageViews));
    for (int32_t stp_index = 1; stp_index >= 0; stp_index--) {
        if (0 < strlen(stp_strs[stp_index])) {
            strncat(str, stp_strs[stp_index], strlen(stp_strs[stp_index]));
            free(stp_strs[stp_index]);
        }
    }
    free(extra_indent);
    return str;
}
char* xgl_print_xgl_vertex_input_attribute_description(const XGL_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sbinding = %u\n%sformat = %s\n%soffsetInBytes = %u\n", prefix, (pStruct->binding), prefix, string_XGL_FORMAT(pStruct->format), prefix, (pStruct->offsetInBytes));
    return str;
}
char* xgl_print_xgl_vertex_input_binding_description(const XGL_VERTEX_INPUT_BINDING_DESCRIPTION* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%sstrideInBytes = %u\n%sstepRate = %s\n", prefix, (pStruct->strideInBytes), prefix, string_XGL_VERTEX_INPUT_STEP_RATE(pStruct->stepRate));
    return str;
}
char* xgl_print_xgl_viewport(const XGL_VIEWPORT* pStruct, const char* prefix)
{
    char* str;
    size_t len;
    len = sizeof(char)*1024;
    str = (char*)malloc(len);
    snprintf(str, len, "%soriginX = %f\n%soriginY = %f\n%swidth = %f\n%sheight = %f\n%sminDepth = %f\n%smaxDepth = %f\n", prefix, (pStruct->originX), prefix, (pStruct->originY), prefix, (pStruct->width), prefix, (pStruct->height), prefix, (pStruct->minDepth), prefix, (pStruct->maxDepth));
    return str;
}
char* dynamic_display(const void* pStruct, const char* prefix)
{
    // Cast to APP_INFO ptr initially just to pull sType off struct
    if (pStruct == NULL) {
        return NULL;
    }
    XGL_STRUCTURE_TYPE sType = ((XGL_APPLICATION_INFO*)pStruct)->sType;
    char indent[100];
    strcpy(indent, "    ");
    strcat(indent, prefix);
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