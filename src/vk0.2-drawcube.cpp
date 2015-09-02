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
Draw Cube
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
    char sample_title[] = "Draw Cube";

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
    init_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_uniform_buffer(info);
    init_renderpass(info);
    init_framebuffers(info);
    init_vertex_buffer(info);
    init_descriptor_and_pipeline_layouts(info);
    init_descriptor_set(info);
    init_shaders(info);
    init_pipeline(info);
    init_dynamic_state(info);

    /* VULKAN_KEY_START */
    VkCmdBufferBeginInfo cmd_buf_info;
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT;
    cmd_buf_info.renderPass = 0;  /* May only set renderPass and framebuffer */
    cmd_buf_info.framebuffer = 0; /* for secondary command buffers           */
    VkClearValue clear_values[2];
    clear_values[0].color.f32[0] = 0.2f;
    clear_values[0].color.f32[1] = 0.2f;
    clear_values[0].color.f32[2] = 0.2f;
    clear_values[0].color.f32[3] = 0.2f;
    clear_values[1].ds.depth     = 1.0f;
    clear_values[1].ds.stencil   = 0;

    VkRenderPassBeginInfo rp_begin;
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

    res = vkBeginCommandBuffer(info.cmd, &cmd_buf_info);
    assert(!res);

    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);

    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout,
            0, 1, &info.desc_set, 0, NULL);

    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);

    vkCmdBindDynamicViewportState(info.cmd, info.dyn_viewport);
    vkCmdBindDynamicRasterState(info.cmd,  info.dyn_raster);
    vkCmdBindDynamicColorBlendState(info.cmd, info.dyn_blend);
    vkCmdBindDynamicDepthStencilState(info.cmd, info.dyn_depth);

    vkCmdDraw(info.cmd, 0, 12 * 3, 0, 1);
    vkCmdEndRenderPass(info.cmd);

    res = vkEndCommandBuffer(info.cmd);
    const VkCmdBuffer cmd_bufs[] = { info.cmd };
    VkFence nullFence = { VK_NULL_HANDLE };

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, cmd_bufs, nullFence);
    assert(!res);

    res = vkQueueWaitIdle(info.queue);
    assert(!res);


    /* Now present the image in the window */

    VkPresentInfoWSI present;
    present.sType = VK_STRUCTURE_TYPE_QUEUE_PRESENT_INFO_WSI;
    present.pNext = NULL;
    present.swapChainCount = 1;
    present.swapChains = &info.swap_chain;
    present.imageIndices = &info.current_buffer;

    res = info.fpQueuePresentWSI(info.queue, &present);
    // TODO: Deal with the VK_SUBOPTIMAL_WSI and VK_ERROR_OUT_OF_DATE_WSI
    // return codes
    assert(!res);

    res = vkQueueWaitIdle(info.queue);
    assert(res == VK_SUCCESS);

    wait_seconds(1);
    /* VULKAN_KEY_END */

    vkDestroyDynamicViewportState(info.device, info.dyn_viewport);
    vkDestroyDynamicRasterState(info.device, info.dyn_raster);
    vkDestroyDynamicColorBlendState(info.device, info.dyn_blend);
    vkDestroyDynamicDepthStencilState(info.device, info.dyn_depth);
    vkDestroyPipeline(info.device, info.pipeline);
    vkDestroyPipelineCache(info.device, info.pipelineCache);
    vkFreeMemory(info.device, info.uniform_data.mem);
    vkDestroyBufferView(info.device, info.uniform_data.view);
    vkDestroyBuffer(info.device, info.uniform_data.buf);
    vkDestroyDescriptorSetLayout(info.device, info.desc_layout);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout);
    vkFreeDescriptorSets(info.device, info.desc_pool, 1, &info.desc_set);
    vkDestroyDescriptorPool(info.device, info.desc_pool);
    vkDestroyShader(info.device,info.shaderStages[0].shader);
    vkDestroyShader(info.device,info.shaderStages[1].shader);
    vkDestroyShaderModule(info.device, info.vert_shader_module);
    vkDestroyShaderModule(info.device, info.frag_shader_module);
    vkDestroyCommandBuffer(info.device, info.cmd);
    vkDestroyCommandPool(info.device, info.cmd_pool);
    vkFreeMemory(info.device, info.depth.mem);
    vkDestroyAttachmentView(info.device, info.depth.view);
    vkDestroyImage(info.device, info.depth.image);
    vkFreeMemory(info.device, info.vertex_buffer.mem);
    vkDestroyBufferView(info.device, info.vertex_buffer.view);
    vkDestroyBuffer(info.device, info.vertex_buffer.buf);
    for (int i = 0; i < info.swapChainImageCount; i++) {
        vkDestroyAttachmentView(info.device, info.buffers[i].view);
    }
    info.fpDestroySwapChainWSI(info.device, info.swap_chain);
    for (int i = 0; i < SAMPLE_BUFFER_COUNT; i++) {
        vkDestroyFramebuffer(info.device, info.framebuffers[i]);
    }
    vkDestroyRenderPass(info.device, info.render_pass);
    vkDestroyDevice(info.device);

}
