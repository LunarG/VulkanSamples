/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 Valve Corporation
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

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
Create Vertex Buffer
*/

/* This is part of the draw cube progression */

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

int sample_main()
{
    VkResult U_ASSERT_ONLY res;
    bool U_ASSERT_ONLY pass;
    struct sample_info info = {};
    char sample_title[] = "Vertex Buffer Sample";
    const bool depthPresent = true;

    init_global_layer_properties(info);
    init_instance_extension_names(info);
    init_device_extension_names(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    get_window_size(&info.width, &info.height);
    init_connection(info);
    init_window(info);
    init_swapchain_extension(info);
    init_device(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_renderpass(info, depthPresent);
    init_framebuffers(info, depthPresent);

    /* VULKAN_KEY_START */
    /*
     * Set up a vertex buffer:
     * - Create a buffer
     * - Map it and write the vertex data into it
     * - Bind it using vkCmdBindVertexBuffers
     * - Later, at pipeline creation,
     * -      fill in vertex input part of the pipeline with relevent data
     */

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.size = sizeof(g_vb_solid_face_colors_Data);
    buf_info.queueFamilyIndexCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(info.device, &buf_info, NULL, &info.vertex_buffer.buf);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(info.device, info.vertex_buffer.buf, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    pass = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(pass);

    res = vkAllocateMemory(info.device, &alloc_info, NULL, &(info.vertex_buffer.mem));
    assert(res == VK_SUCCESS);

    uint8_t *pData;
    res = vkMapMemory(info.device, info.vertex_buffer.mem, 0, mem_reqs.size, 0, (void **) &pData);
    assert(res == VK_SUCCESS);

    memcpy(pData, g_vb_solid_face_colors_Data, sizeof(g_vb_solid_face_colors_Data));

    vkUnmapMemory(info.device, info.vertex_buffer.mem);

    res = vkBindBufferMemory(info.device,
            info.vertex_buffer.buf,
            info.vertex_buffer.mem, 0);
    assert(res == VK_SUCCESS);

    /* We won't use these here, but we will need this info when creating the pipeline */
    info.vi_binding.binding = 0;
    info.vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    info.vi_binding.stride = sizeof(g_vb_solid_face_colors_Data[0]);

    info.vi_attribs[0].binding = 0;
    info.vi_attribs[0].location = 0;
    info.vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[0].offset = 0;
    info.vi_attribs[1].binding = 0;
    info.vi_attribs[1].location = 1;
    info.vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[1].offset = 16;

    const VkDeviceSize offsets[1] = {0};

    /* We cannot bind the vertex buffer until we begin a renderpass */
    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth     = 1.0f;
    clear_values[1].depthStencil.stencil   = 0;

    VkRenderPassBeginInfo rp_begin = {};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = info.render_pass;
    rp_begin.framebuffer = info.framebuffers[info.current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = info.width;
    rp_begin.renderArea.extent.height = info.height;
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindVertexBuffers(info.cmd,
                           0,                       /* Start Binding */
                           1,                       /* Binding Count */
                           &info.vertex_buffer.buf, /* pBuffers */
                           offsets);                /* pOffsets */

    vkCmdEndRenderPass(info.cmd);
    execute_end_command_buffer(info);
    execute_queue_command_buffer(info);
    /* VULKAN_KEY_END */

    vkDestroyBuffer(info.device, info.vertex_buffer.buf, NULL);
    vkFreeMemory(info.device, info.vertex_buffer.mem, NULL);
    destroy_framebuffers(info);
    destroy_renderpass(info);
    destroy_depth_buffer(info);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
    return 0;
}
