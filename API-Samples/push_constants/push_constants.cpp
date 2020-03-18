/*
 * Vulkan Samples
 *
 * Copyright (C) 2015-2020 Valve Corporation
 * Copyright (C) 2015-2020 LunarG, Inc.
 * Copyright (C) 2015-2020 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

int sample_main(int argc, char *argv[]) {
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Simple Push Constants";
    const bool depthPresent = true;

    process_command_line_args(info, argc, argv);
    init_global_layer_properties(info);
    init_instance_extension_names(info);
    init_device_extension_names(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_window_size(info, 500, 500);
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
    init_uniform_buffer(info);
    init_renderpass(info, depthPresent);
#include "push_constants.vert.h"
#include "push_constants.frag.h"
    VkShaderModuleCreateInfo vert_info = {};
    VkShaderModuleCreateInfo frag_info = {};
    vert_info.sType = frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_info.codeSize = sizeof(push_constants_vert);
    vert_info.pCode = push_constants_vert;
    frag_info.codeSize = sizeof(push_constants_frag);
    frag_info.pCode = push_constants_frag;
    init_shaders(info, &vert_info, &frag_info);
    init_framebuffers(info, depthPresent);
    init_vertex_buffer(info, g_vb_texture_Data, sizeof(g_vb_texture_Data), sizeof(g_vb_texture_Data[0]), true);

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
    resource_layout_info[0].pBindings = resource_binding;

    VkDescriptorSetLayout descriptor_layouts[1] = {};
    res = vkCreateDescriptorSetLayout(info.device, resource_layout_info, NULL, &descriptor_layouts[0]);
    assert(res == VK_SUCCESS);

    /* VULKAN_KEY_START */

    // Set up our push constant range, which mirrors the declaration of
    const unsigned push_constant_range_count = 1;
    VkPushConstantRange push_constant_ranges[push_constant_range_count] = {};
    push_constant_ranges[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push_constant_ranges[0].offset = 0;
    push_constant_ranges[0].size = 8;

    // Create pipeline layout, including push constant info.

    // Create pipeline layout with multiple descriptor sets
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo[1] = {};
    pipelineLayoutCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo[0].pNext = NULL;
    pipelineLayoutCreateInfo[0].pushConstantRangeCount = push_constant_range_count;
    pipelineLayoutCreateInfo[0].pPushConstantRanges = push_constant_ranges;
    pipelineLayoutCreateInfo[0].setLayoutCount = descriptor_set_count;
    pipelineLayoutCreateInfo[0].pSetLayouts = descriptor_layouts;
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
    alloc_info[0].descriptorSetCount = descriptor_set_count;
    alloc_info[0].pSetLayouts = descriptor_layouts;

    // Populate descriptor sets
    VkDescriptorSet descriptor_sets[descriptor_set_count] = {};
    res = vkAllocateDescriptorSets(info.device, alloc_info, descriptor_sets);
    assert(res == VK_SUCCESS);

    // Using empty brace initializer on the next line triggers a bug in older
    // versions of gcc, so memset instead
    VkWriteDescriptorSet descriptor_writes[resource_count];
    memset(descriptor_writes, 0, sizeof(descriptor_writes));

    // Populate with info about our uniform buffer for MVP
    descriptor_writes[0] = {};
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].pNext = NULL;
    descriptor_writes[0].dstSet = descriptor_sets[0];
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].pBufferInfo = &info.uniform_data.buffer_info;  // populated by init_uniform_buffer()
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].dstBinding = 0;

    vkUpdateDescriptorSets(info.device, resource_count, descriptor_writes, 0, NULL);

    // Create our push constant data, which matches shader expectations
    unsigned pushConstants[2] = {};
    pushConstants[0] = (unsigned)2;
    pushConstants[1] = (unsigned)0x3F800000;

    // Ensure we have enough room for push constant data
    if (sizeof(pushConstants) > info.gpu_props.limits.maxPushConstantsSize) assert(0 && "Too many push constants");

    vkCmdPushConstants(info.cmd, info.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), pushConstants);

    /* VULKAN_KEY_END */

    init_pipeline_cache(info);
    init_pipeline(info, depthPresent);
    init_presentable_image(info);

    VkClearValue clear_values[2];
    init_clear_color_and_depth(info, clear_values);

    VkRenderPassBeginInfo rp_begin;
    init_render_pass_begin_info(info, rp_begin);
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout, 0, NUM_DESCRIPTOR_SETS,
                            descriptor_sets, 0, NULL);

    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);

    init_viewports(info);
    init_scissors(info);

    vkCmdDraw(info.cmd, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);
    res = vkEndCommandBuffer(info.cmd);
    assert(res == VK_SUCCESS);

    VkFence drawFence = {};
    init_fence(info, drawFence);
    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {};
    init_submit_info(info, submit_info, pipe_stage_flags);

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.graphics_queue, 1, &submit_info, drawFence);
    assert(res == VK_SUCCESS);

    /* Now present the image in the window */
    VkPresentInfoKHR present = {};
    init_present_info(info, present);

    /* Make sure command buffer is finished before presenting */
    do {
        res = vkWaitForFences(info.device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    } while (res == VK_TIMEOUT);
    assert(res == VK_SUCCESS);
    res = vkQueuePresentKHR(info.present_queue, &present);
    assert(res == VK_SUCCESS);

    wait_seconds(1);
    if (info.save_images) write_ppm(info, "push_constants");

    vkDestroyFence(info.device, drawFence, NULL);
    vkDestroySemaphore(info.device, info.imageAcquiredSemaphore, NULL);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);
    // instead of destroy_descriptor_pool(info);
    vkDestroyDescriptorPool(info.device, descriptor_pool[0], NULL);
    destroy_vertex_buffer(info);
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);
    // instead of destroy_descriptor_and_pipeline_layouts(info);
    for (int i = 0; i < descriptor_set_count; i++) vkDestroyDescriptorSetLayout(info.device, descriptor_layouts[i], NULL);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout, NULL);
    destroy_uniform_buffer(info);
    destroy_depth_buffer(info);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_device(info);
    destroy_window(info);
    destroy_instance(info);
    return 0;
}
