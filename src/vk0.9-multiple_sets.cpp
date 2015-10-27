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
Use multiple descriptor sets to draw a textured cube.
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

// This sample is based on drawtexturedcube, but rather than place both
// UBO and sampled image into a single set, we split it up to demonstrate
// how to create, submit, and reference multiple sets in a shader

/* For this sample, we'll start with GLSL so the shader function is plain */
/* and then use the glslang GLSLtoSPV utility to convert it to SPIR-V for */
/* the driver.  We do this for clarity rather than using pre-compiled     */
/* SPIR-V                                                                 */

const char *vertShaderText =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        // Note, set = 0 here
        "layout (set = 0, binding = 0) uniform bufferVals {\n"
        "    mat4 mvp;\n"
        "} myBufferVals;\n"
        // And set = 1 here
        "layout (set = 1, binding = 0) uniform sampler2D surface;\n"
        "layout (location = 0) in vec4 pos;\n"
        "layout (location = 1) in vec2 inTexCoords;\n"
        "layout (location = 0) out vec4 outColor;\n"
        "layout (location = 1) out vec2 outTexCoords;\n"
        "void main() {\n"
        "   outColor = texture(surface, vec2(0.0));\n"
        "   outTexCoords = inTexCoords;\n"
        "   gl_Position = myBufferVals.mvp * pos;\n"
        "   // GL->VK conventions\n"
        "   gl_Position.y = -gl_Position.y;\n"
        "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
        "}\n";

const char *fragShaderText=
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "#extension GL_ARB_shading_language_420pack : enable\n"
        "layout (location = 0) in vec4 inColor;\n"
        "layout (location = 1) in vec2 inTexCoords;\n"
        "layout (location = 0) out vec4 outColor;\n"
        "void main() {\n"
        "    vec4 resColor = inColor;\n"
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
    char sample_title[] = "Multiple Descriptor Sets";

    init_global_layer_properties(info);
    info.instance_extension_names.push_back(VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME);
    info.device_extension_names.push_back(VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME);
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
    // Sample from a green texture to easily see that we've pulled correct texel value
    const char* textureName = "green.ppm";
    init_texture(info, textureName);
    init_uniform_buffer(info);
    init_renderpass(info, true);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info, true);
    init_vertex_buffer(info, g_vb_texture_Data, sizeof(g_vb_texture_Data),
                       sizeof(g_vb_texture_Data[0]), true);

    /* VULKAN_KEY_START */

    // Set up two descriptor sets
    static const unsigned descriptor_set_count = 2;

    // Create first layout to contain uniform buffer data
    VkDescriptorSetLayoutBinding uniform_binding[1] = {};
    uniform_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_binding[0].arraySize = 1;
    uniform_binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uniform_binding[0].pImmutableSamplers = NULL;
    VkDescriptorSetLayoutCreateInfo uniform_layout_info[1] = {};
    uniform_layout_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uniform_layout_info[0].pNext = NULL;
    uniform_layout_info[0].count = 1;
    uniform_layout_info[0].pBinding = uniform_binding;

    // Create second layout containing combined sampler/image data
    VkDescriptorSetLayoutBinding sampler2D_binding[1] = {};
    sampler2D_binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler2D_binding[0].arraySize = 1;
    sampler2D_binding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    sampler2D_binding[0].pImmutableSamplers = NULL;
    VkDescriptorSetLayoutCreateInfo sampler2D_layout_info[1] = {};
    sampler2D_layout_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sampler2D_layout_info[0].pNext = NULL;
    sampler2D_layout_info[0].count = 1;
    sampler2D_layout_info[0].pBinding = sampler2D_binding;

    // Create multiple sets, using each createInfo
    static const unsigned uniform_set_index = 0;
    static const unsigned sampler_set_index = 1;
    VkDescriptorSetLayout descriptor_layouts[descriptor_set_count] = {};
    res = vkCreateDescriptorSetLayout(info.device, uniform_layout_info, &descriptor_layouts[uniform_set_index]);
    assert(res == VK_SUCCESS);
    res = vkCreateDescriptorSetLayout(info.device, sampler2D_layout_info, &descriptor_layouts[sampler_set_index]);
    assert(res == VK_SUCCESS);

    // Create pipeline layout with multiple descriptor sets
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo[1] = {};
    pipelineLayoutCreateInfo[0].sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo[0].pNext                  = NULL;
    pipelineLayoutCreateInfo[0].pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo[0].pPushConstantRanges    = NULL;
    pipelineLayoutCreateInfo[0].descriptorSetCount     = descriptor_set_count;
    pipelineLayoutCreateInfo[0].pSetLayouts            = descriptor_layouts;
    res = vkCreatePipelineLayout(info.device, pipelineLayoutCreateInfo, &info.pipeline_layout);
    assert(res == VK_SUCCESS);

    // Create a single pool to contain data for our two descriptor sets
    VkDescriptorTypeCount type_count[2] = {};
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].count = 1;
    type_count[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    type_count[1].count = 1;

    VkDescriptorPoolCreateInfo pool_info[1] = {};
    pool_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info[0].pNext = NULL;
    pool_info[0].poolUsage = VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT;
    pool_info[0].maxSets = descriptor_set_count;
    pool_info[0].count = sizeof(type_count) / sizeof(VkDescriptorTypeCount);
    pool_info[0].pTypeCount = type_count;

    VkDescriptorPool descriptor_pool[1] = {};
    res = vkCreateDescriptorPool(info.device, pool_info, descriptor_pool);
    assert(res == VK_SUCCESS);

    // Populate the descriptor sets
    VkDescriptorSet descriptor_sets[descriptor_set_count] = {};
    res = vkAllocDescriptorSets(
            info.device,
            descriptor_pool[0],
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            descriptor_set_count,
            descriptor_layouts,
            descriptor_sets);
    assert(res == VK_SUCCESS);

    // Using empty brace initializer on the next line triggers a bug in older versions of gcc, so memset instead
    VkWriteDescriptorSet descriptor_writes[2];
    memset(descriptor_writes, 0, sizeof(descriptor_writes));

    // Populate with info about our uniform buffer
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].pNext = NULL;
    descriptor_writes[0].destSet = descriptor_sets[uniform_set_index];
    descriptor_writes[0].count = 1;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].pDescriptors = &info.uniform_data.desc; // populated by init_uniform_buffer()
    descriptor_writes[0].destArrayElement = 0;
    descriptor_writes[0].destBinding = 0;

    // Populate with info about our sampled image
    descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].pNext = NULL;
    descriptor_writes[1].destSet = descriptor_sets[sampler_set_index];
    descriptor_writes[1].count = 1;
    descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[1].pDescriptors = &info.texture_data.desc; // populated by init_texture()
    descriptor_writes[1].destArrayElement = 0;
    descriptor_writes[1].destBinding = 0;

    vkUpdateDescriptorSets(info.device, descriptor_set_count, descriptor_writes, 0, NULL);

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
                            &presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    // Get the index of the next available swapchain image:
    res = info.fpAcquireNextImageKHR(info.device, info.swap_chain,
                                      UINT64_MAX,
                                      presentCompleteSemaphore,
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

    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);

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
    viewport.originX = 0;
    viewport.originY = 0;
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

    /* Make sure buffer is ready for rendering */
    res = vkQueueWaitSemaphore(info.queue, presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    const VkCmdBuffer cmd_bufs[] = { info.cmd };
    VkFence nullFence = { VK_NULL_HANDLE };

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, cmd_bufs, nullFence);
    assert(res == VK_SUCCESS);

    /* Now present the image in the window */

    VkPresentInfoKHR present;
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.swapchains = &info.swap_chain;
    present.imageIndices = &info.current_buffer;

    res = info.fpQueuePresentKHR(info.queue, &present);
    assert(res == VK_SUCCESS);

    wait_seconds(1);

    vkDestroySemaphore(info.device, presentCompleteSemaphore);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);
    destroy_texture(info);

    // instead of destroy_descriptor_pool(info);
    vkDestroyDescriptorPool(info.device, descriptor_pool[0]);

    destroy_vertex_buffer(info);
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);

    //instead of destroy_descriptor_and_pipeline_layouts(info);
    for(int i = 0; i < descriptor_set_count; i++)
        vkDestroyDescriptorSetLayout(info.device, descriptor_layouts[i]);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout);

    destroy_uniform_buffer(info);
    destroy_depth_buffer(info);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
}
