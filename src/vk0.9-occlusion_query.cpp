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
Use occlusion query
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

static const char *vertShaderText =
    "#version 140\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (binding = 0) uniform bufferVals {\n"
    "    mat4 mvp;\n"
    "} myBufferVals;\n"
    "layout (location = 0) in vec4 pos;\n"
    "layout (location = 1) in vec4 inColor;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "   outColor = inColor;\n"
    "   gl_Position = myBufferVals.mvp * pos;\n"
    "   gl_Position.y = -gl_Position.y;\n"
    "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
    "}\n";

static const char *fragShaderText=
    "#version 140\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (location = 0) in vec4 color;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "   outColor = color;\n"
    "}\n";

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Draw Cube";

    init_global_layer_properties(info);
    info.instance_extension_names.push_back(VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME);
    info.device_extension_names.push_back(VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME);
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
    init_uniform_buffer(info);
    init_descriptor_and_pipeline_layouts(info, false);
    init_renderpass(info);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info);
    init_vertex_buffer(info, g_vb_solid_face_colors_Data, sizeof(g_vb_solid_face_colors_Data),
                               sizeof(g_vb_solid_face_colors_Data[0]), false);
    init_descriptor_set(info, false);
    init_pipeline(info);

    /* VULKAN_KEY_START */

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth     = 1.0f;
    clear_values[1].depthStencil.stencil   = 0;

    VkSemaphore presentCompleteSemaphore;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo;
    presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreCreateInfo.pNext = NULL;
    presentCompleteSemaphoreCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    res = vkCreateSemaphore(info.device,
                            &presentCompleteSemaphoreCreateInfo,
                            &presentCompleteSemaphore);
    assert(!res);

    // Get the index of the next available swapchain image:
    res = info.fpAcquireNextImageKHR(info.device, info.swap_chain,
                                      UINT64_MAX,
                                      presentCompleteSemaphore,
                                      &info.current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(!res);

    VkQueryPool query_pool;
    VkQueryPoolCreateInfo query_pool_info;
    query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_info.pNext = NULL;
    query_pool_info.queryType = VK_QUERY_TYPE_OCCLUSION;
    query_pool_info.slots = 2;
    query_pool_info.pipelineStatistics = 0;

    res = vkCreateQueryPool(info.device,
                            &query_pool_info, &query_pool);
    assert(!res);

// vkGetPhysicalDeviceFeatures pFeatures->occlusionQueryNonConservative
//<enums name="VkQueryControlFlagBits">
//<enum name="VK_QUERY_CONTROL_CONSERVATIVE_BIT" comment="Allow conservative results to be collected by the query"/>
//<enums name="VkQueryResultFlagBits">
//<enum name="VK_QUERY_RESULT_DEFAULT"               comment="Results of the queries are immediately written to the destination buffer as 32-bit values"/>
//<enum name="VK_QUERY_RESULT_64_BIT"                comment="Results of the queries are written to the destination buffer as 64-bit values"/>
//<enum name="VK_QUERY_RESULT_WAIT_BIT"              comment="Results of the queries are waited on before proceeding with the result copy"/>
//<enum name="VK_QUERY_RESULT_WITH_AVAILABILITY_BIT" comment="Besides the results of the query, the availability of the results is also written"/>
//<enum name="VK_QUERY_RESULT_PARTIAL_BIT"           comment="Copy the partial results of the query even if the final results aren't available"/>

// vkCreateQueryPool
// vkCmdResetQueryPool
// vkCmdBeginQuery
// vkCmdEndQuery
// vkCmdCopyQueryPoolResults
// vkGetQueryPoolResults
// vkDestroyQueryPool


    VkRenderPassBeginInfo rp_begin;
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

    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);

    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout,
            0, 1, &info.desc_set, 0, NULL);

    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);

    VkViewport viewport;
    viewport.height = (float) info.height;
    viewport.width = (float) info.width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.originX = 0;
    viewport.originY = 0;
    vkCmdSetViewport(info.cmd, 1, &viewport);

    VkRect2D scissor;
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(info.cmd, 1, &scissor);

    vkCmdResetQueryPool(info.cmd, query_pool, 0 /*startQuery*/, 2 /*queryCount*/);

    vkCmdBeginQuery(info.cmd, query_pool, 0 /*slot*/, 0 /*flags*/);
    vkCmdEndQuery(info.cmd, query_pool, 0 /*slot*/);

    vkCmdBeginQuery(info.cmd, query_pool, 1 /*slot*/, 0 /*flags*/);

    vkCmdDraw(info.cmd, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);

    vkCmdEndQuery(info.cmd, query_pool, 1 /*slot*/);

// vkCmdCopyQueryPoolResults

    res = vkEndCommandBuffer(info.cmd);
    const VkCmdBuffer cmd_bufs[] = { info.cmd };
    VkFence nullFence = { VK_NULL_HANDLE };

    /* Make sure buffer is ready for rendering */
    vkQueueWaitSemaphore(info.queue, presentCompleteSemaphore);

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, cmd_bufs, nullFence);
    assert(!res);

    res = vkQueueWaitIdle(info.queue);
    assert(!res);

    uint64_t samples_passed[4];
    size_t data_size = sizeof(samples_passed);

    samples_passed[0] = 0;
    samples_passed[1] = 0;
    res = vkGetQueryPoolResults(info.device, query_pool,
        0 /*startQuery*/,
        2 /*queryCount*/,
        &data_size,
        samples_passed,
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    assert(!res);

    std::cout << "samples_passed[0] = " << samples_passed[0] << "\n";
    std::cout << "samples_passed[1] = " << samples_passed[1] << "\n";

    /* Now present the image in the window */

    VkPresentInfoKHR present;
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.swapchains = &info.swap_chain;
    present.imageIndices = &info.current_buffer;

    res = info.fpQueuePresentKHR(info.queue, &present);
    // TODO: Deal with the VK_SUBOPTIMAL_WSI and VK_ERROR_OUT_OF_DATE_WSI
    // return codes
    assert(!res);

    res = vkQueueWaitIdle(info.queue);
    assert(res == VK_SUCCESS);

    wait_seconds(1);
    /* VULKAN_KEY_END */

    vkDestroyQueryPool(info.device, query_pool);
    vkDestroySemaphore(info.device, presentCompleteSemaphore);
    vkDestroyPipeline(info.device, info.pipeline);
    vkDestroyPipelineCache(info.device, info.pipelineCache);
    vkDestroyBuffer(info.device, info.uniform_data.buf);
    vkFreeMemory(info.device, info.uniform_data.mem);
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
    vkDestroyImageView(info.device, info.depth.view);
    vkDestroyImage(info.device, info.depth.image);
    vkFreeMemory(info.device, info.depth.mem);
    vkDestroyBuffer(info.device, info.vertex_buffer.buf);
    vkFreeMemory(info.device, info.vertex_buffer.mem);
    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        vkDestroyImageView(info.device, info.buffers[i].view);
    }
    info.fpDestroySwapchainKHR(info.device, info.swap_chain);
    for (int i = 0; i < SAMPLE_BUFFER_COUNT; i++) {
        vkDestroyFramebuffer(info.device, info.framebuffers[i]);
    }
    vkDestroyRenderPass(info.device, info.render_pass);
    vkDestroyDevice(info.device);
    vkDestroyInstance(info.inst);
    destroy_window(info);
}
