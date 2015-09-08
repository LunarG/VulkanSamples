/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 LunarG, Inc.
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

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

int main(int argc, char **argv)
{
    VkResult res;
    struct sample_info info = {};
    char sample_title[] = "Vertex Buffer Sample";

    init_global_layer_properties(info);
    info.instance_extension_names.push_back(VK_WSI_SWAPCHAIN_EXTENSION_NAME);
    info.device_extension_names.push_back(VK_WSI_DEVICE_SWAPCHAIN_EXTENSION_NAME);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    info.width = info.height = 500;
    init_connection(info);
    init_window(info);
    init_wsi(info);
    init_and_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_renderpass(info);
    init_framebuffers(info);

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
    buf_info.size = sizeof(info.MVP);
    buf_info.queueFamilyCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(info.device, &buf_info, &info.vertex_buffer.buf);
    assert(!res);

    VkMemoryRequirements mem_reqs;
    res = vkGetBufferMemoryRequirements(info.device, info.vertex_buffer.buf, &mem_reqs);
    assert(!res);

    VkMemoryAllocInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    res = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(!res);

    res = vkAllocMemory(info.device, &alloc_info, &(info.vertex_buffer.mem));
    assert(!res);

    uint8_t *pData;
    res = vkMapMemory(info.device, info.vertex_buffer.mem, 0, 0, 0, (void **) &pData);
    assert(!res);

    memcpy(pData, g_vb_solid_face_colors_Data, sizeof(g_vb_solid_face_colors_Data));

    res = vkUnmapMemory(info.device, info.vertex_buffer.mem);
    assert(!res);

    res = vkBindBufferMemory(info.device,
            info.vertex_buffer.buf,
            info.vertex_buffer.mem, 0);
    assert(!res);

    VkBufferViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_info.pNext = NULL;
    view_info.buffer = info.vertex_buffer.buf;
    view_info.viewType = VK_BUFFER_VIEW_TYPE_RAW;
    view_info.offset = 0;
    view_info.range = sizeof(info.MVP);
    view_info.format = VK_FORMAT_UNDEFINED;

    res = vkCreateBufferView(info.device, &view_info, &info.vertex_buffer.view);
    assert(!res);

    info.vertex_buffer.desc.bufferView = info.vertex_buffer.view;

    /* We won't use these here, but we will need this info when creating the pipeline */
    info.vi_binding.binding = 0;
    info.vi_binding.stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX;
    info.vi_binding.strideInBytes = sizeof(g_vb_solid_face_colors_Data[0]);

    info.vi_attribs[0].binding = 0;
    info.vi_attribs[0].location = 0;
    info.vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[0].offsetInBytes = 0;
    info.vi_attribs[1].binding = 0;
    info.vi_attribs[1].location = 1;
    info.vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[1].offsetInBytes = 16;

    const VkDeviceSize offsets[1] = {0};

    /* We cannot bind the vertex buffer until we begin a renderpass */
    VkClearValue clear_values[2];
    clear_values[0].color.f32[0] = 0.2f;
    clear_values[0].color.f32[1] = 0.2f;
    clear_values[0].color.f32[2] = 0.2f;
    clear_values[0].color.f32[3] = 0.2f;
    clear_values[1].ds.depth     = 1.0f;
    clear_values[1].ds.stencil   = 0;

    VkRenderPassBeginInfo rp_begin = {};
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = info.render_pass;
    rp_begin.framebuffer = info.framebuffers[info.current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = info.width;
    rp_begin.renderArea.extent.height = info.height;
    rp_begin.attachmentCount = 2;
    rp_begin.pAttachmentClearValues = clear_values;

    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);

    vkCmdBindVertexBuffers(info.cmd,
                           0,                       /* Start Binding */
                           1,                       /* Binding Count */
                           &info.vertex_buffer.buf, /* pBuffers */
                           offsets);                /* pOffsets */

    vkCmdEndRenderPass(info.cmd);
    end_and_submit_command_buffer(info);
    /* VULKAN_KEY_END */

    vkFreeMemory(info.device, info.depth.mem);
    vkDestroyAttachmentView(info.device, info.depth.view);
    vkDestroyImage(info.device, info.depth.image);
    info.fpDestroySwapChainWSI(info.device, info.swap_chain);
    for (int i = 0; i < SAMPLE_BUFFER_COUNT; i++) {
        vkDestroyFramebuffer(info.device, info.framebuffers[i]);
    }

    for (int i = 0; i < info.swapChainImageCount; i++) {
        vkDestroyAttachmentView(info.device, info.buffers[i].view);
    }
    vkDestroyRenderPass(info.device, info.render_pass);
    vkDestroyCommandBuffer(info.device, info.cmd);
    vkDestroyCommandPool(info.device, info.cmd_pool);
    vkFreeMemory(info.device, info.vertex_buffer.mem);
    vkDestroyBufferView(info.device, info.vertex_buffer.view);
    vkDestroyBuffer(info.device, info.vertex_buffer.buf);
    vkDestroyDevice(info.device);
    vkDestroyInstance(info.inst);
    destroy_window(info);
}
