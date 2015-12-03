/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 Valve Corporation
 * Copyright (C) 2015 Google, Inc.
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
Use push constants in a simple shader, validate the correct value was read.
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

// This sample submits two push constants and pairs them with a shader
// that simply reads in the values, ensures they are correct.  If correct
// values are read, the shader draws green.  If incorrect, shader draws red.

/* For this sample, we'll start with GLSL so the shader function is plain */
/* and then use the glslang GLSLtoSPV utility to convert it to SPIR-V for */
/* the driver.  We do this for clarity rather than using pre-compiled     */
/* SPIR-V                                                                 */

const char *vertShaderText =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "layout (std140, set = 0, binding = 0) uniform buf {\n"
        "    mat4 mvp;\n"
        "} ubuf;\n"
        "layout (location = 0) in vec4 pos;\n"
        "layout (location = 1) in vec2 inTexCoords;\n"
        "layout (location = 0) out vec2 outTexCoords;\n"
        "void main() {\n"
        "   gl_Position = ubuf.mvp * pos;\n"
        "   outTexCoords = inTexCoords;\n"
        "   // GL->VK conventions\n"
        "   gl_Position.y = -gl_Position.y;\n"
        "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
        "}\n";

const char *fragShaderText=
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "layout(push_constant) uniform pushBlock {\n"
        "    int iFoo;\n"
        "    float fBar;\n"
        "} pushConstantsBlock;\n"
        "layout (location = 0) in vec2 inTexCoords;\n"
        "layout (location = 0) out vec4 outColor;\n"
        "void main() {\n"

        "    vec4 green = vec4(0.0, 1.0, 0.0, 1.0);\n"
        "    vec4 red   = vec4(1.0, 0.0, 0.0, 1.0);\n"

        // Start with passing color
        "    vec4 resColor = green;\n"

        // See if we've read in the correct push constants
        "    if (pushConstantsBlock.iFoo != 2)\n"
        "        resColor = red;\n"
        "    if (pushConstantsBlock.fBar != 1.0f)\n"
        "        resColor = red;\n"

        // Create a border to see the cube more easily
        "   if (inTexCoords.x < 0.01 || inTexCoords.x > 0.99)\n"
        "       resColor *= vec4(0.1, 0.1, 0.1, 1.0);\n"
        "   if (inTexCoords.y < 0.01 || inTexCoords.y > 0.99)\n"
        "       resColor *= vec4(0.1, 0.1, 0.1, 1.0);\n"
        "   outColor = resColor;\n"
        "}\n";


int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Simple Push Constants";

    init_global_layer_properties(info);
    info.instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef _WIN32
    info.instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
    info.instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
    info.device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    info.width = info.height = 500;
    init_connection(info);
    init_window(info);
    init_swapchain_extension(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_uniform_buffer(info);
    init_renderpass(info, true);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info, true);
    init_vertex_buffer(info, g_vb_texture_Data, sizeof(g_vb_texture_Data),
                       sizeof(g_vb_texture_Data[0]), true);

    // Set up one descriptor sets
    static const unsigned descriptor_set_count = 1;
    static const unsigned resource_count = 1;

    // Create binding and layout for the following, matching contents of shader
    //   binding 0 = uniform buffer (MVP)

    VkDescriptorSetLayoutBinding resource_binding[resource_count] = {};
    resource_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    resource_binding[0].descriptorCount = 1;
    resource_binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    resource_binding[0].pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo resource_layout_info[1] = {};
    resource_layout_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    resource_layout_info[0].pNext = NULL;
    resource_layout_info[0].bindingCount = resource_count;
    resource_layout_info[0].pBinding = resource_binding;

    VkDescriptorSetLayout descriptor_layouts[0] = {};
    res = vkCreateDescriptorSetLayout(info.device, resource_layout_info, NULL, &descriptor_layouts[0]);
    assert(res == VK_SUCCESS);

    /* VULKAN_KEY_START */

    // Set up our push constant range, which mirrors the declaration of
    const unsigned push_constant_range_count = 1;
    VkPushConstantRange push_constant_ranges[push_constant_range_count] = {};
    push_constant_ranges[0].stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    push_constant_ranges[0].offset = 0;
    push_constant_ranges[0].size = 8;

    // Create pipeline layout, including push constant info.

    // Create pipeline layout with multiple descriptor sets
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo[1] = {};
    pipelineLayoutCreateInfo[0].sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo[0].pNext                  = NULL;
    pipelineLayoutCreateInfo[0].pushConstantRangeCount = push_constant_range_count;
    pipelineLayoutCreateInfo[0].pPushConstantRanges    = push_constant_ranges;
    pipelineLayoutCreateInfo[0].setLayoutCount         = descriptor_set_count;
    pipelineLayoutCreateInfo[0].pSetLayouts            = descriptor_layouts;
    res = vkCreatePipelineLayout(info.device, pipelineLayoutCreateInfo, NULL, &info.pipeline_layout);
    assert(res == VK_SUCCESS);

    // Create a single pool to contain data for our descriptor set
    VkDescriptorPoolSize type_count[2] = {};
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].descriptorCount = 1;
    type_count[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    type_count[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo pool_info[1] = {};
    pool_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info[0].pNext = NULL;
    pool_info[0].maxSets = descriptor_set_count;
    pool_info[0].poolSizeCount = sizeof(type_count) / sizeof(VkDescriptorPoolSize);
    pool_info[0].pPoolSizes = type_count;

    VkDescriptorPool descriptor_pool[1] = {};
    res = vkCreateDescriptorPool(info.device, pool_info, NULL, descriptor_pool);
    assert(res == VK_SUCCESS);

    VkDescriptorSetAllocateInfo alloc_info[1];
    alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info[0].pNext = NULL;
    alloc_info[0].descriptorPool = descriptor_pool[0];
    alloc_info[0].setLayoutCount = descriptor_set_count;
    alloc_info[0].pSetLayouts = descriptor_layouts;

    // Populate descriptor sets
    VkDescriptorSet descriptor_sets[descriptor_set_count] = {};
    res = vkAllocateDescriptorSets(info.device, alloc_info, descriptor_sets);
    assert(res == VK_SUCCESS);

    // Using empty brace initializer on the next line triggers a bug in older versions of gcc, so memset instead
    VkWriteDescriptorSet descriptor_writes[resource_count];
    memset(descriptor_writes, 0, sizeof(descriptor_writes));

    // Populate with info about our uniform buffer for MVP
    descriptor_writes[0] = {};
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].pNext = NULL;
    descriptor_writes[0].dstSet = descriptor_sets[0];
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].pBufferInfo = &info.uniform_data.buffer_info; // populated by init_uniform_buffer()
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].dstBinding = 0;

    vkUpdateDescriptorSets(info.device, resource_count, descriptor_writes, 0, NULL);

    // Create our push constant data, which matches shader expectations
    unsigned pushConstants[2] = {};
    pushConstants[0] = (unsigned) 2;
    pushConstants[1] = (unsigned) 0x3F800000;

    // Ensure we have enough room for push constant data
    if (sizeof(pushConstants) > info.gpu_props.limits.maxPushConstantsSize)
        assert(0 && "Too many push constants");

    vkCmdPushConstants(info.cmd, info.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), pushConstants);

    /* VULKAN_KEY_END */


    // Call remaining boilerplate utils
    init_pipeline_cache(info);
    init_pipeline(info, true);

    // The remaining is identical to drawtexturedcube
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
    presentCompleteSemaphoreCreateInfo.flags = 0;

    res = vkCreateSemaphore(info.device,
                            &presentCompleteSemaphoreCreateInfo,
                            NULL,
                            &presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    // Get the index of the next available swapchain image:
    res = info.fpAcquireNextImageKHR(info.device, info.swap_chain,
                                      UINT64_MAX,
                                      presentCompleteSemaphore,
                                      NULL,
                                      &info.current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(res == VK_SUCCESS);

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

    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout,
            0, descriptor_set_count, descriptor_sets, 0, NULL);

    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);
    VkViewport viewport;
    viewport.height = (float) info.height;
    viewport.width = (float) info.width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(info.cmd, NUM_VIEWPORTS, &viewport);

    VkRect2D scissor;
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(info.cmd, NUM_SCISSORS, &scissor);

    vkCmdDraw(info.cmd, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);

    execute_pre_present_barrier(info);

    res = vkEndCommandBuffer(info.cmd);
    assert(res == VK_SUCCESS);

    const VkCommandBuffer cmd_bufs[] = { info.cmd };
    VkFenceCreateInfo fenceInfo;
    VkFence drawFence;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(info.device, &fenceInfo, NULL, &drawFence);

    VkSubmitInfo submit_info[1] = {};
    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 1;
    submit_info[0].pWaitSemaphores = &presentCompleteSemaphore;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = cmd_bufs;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, submit_info, drawFence);
    assert(res == VK_SUCCESS);

    /* Now present the image in the window */

    VkPresentInfoKHR present;
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &info.swap_chain;
    present.pImageIndices = &info.current_buffer;
    present.pWaitSemaphores = NULL;
    present.waitSemaphoreCount = 0;
    present.pResults = NULL;

    /* Make sure command buffer is finished before presenting */
    res = vkWaitForFences(info.device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    assert(res == VK_SUCCESS);
    res = info.fpQueuePresentKHR(info.queue, &present);
    assert(res == VK_SUCCESS);

    wait_seconds(1);

    vkDestroySemaphore(info.device, presentCompleteSemaphore, NULL);
    vkDestroyFence(info.device, drawFence, NULL);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);

    // instead of destroy_descriptor_pool(info);
    vkDestroyDescriptorPool(info.device, descriptor_pool[0], NULL);

    destroy_vertex_buffer(info);
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);

    //instead of destroy_descriptor_and_pipeline_layouts(info);
    for(int i = 0; i < descriptor_set_count; i++)
        vkDestroyDescriptorSetLayout(info.device, descriptor_layouts[i], NULL);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout, NULL);

    destroy_uniform_buffer(info);
    destroy_depth_buffer(info);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
}
