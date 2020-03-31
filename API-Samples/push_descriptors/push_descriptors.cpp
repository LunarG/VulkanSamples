/*
 * Vulkan Samples
 *
 * Copyright (C) 2017-2020 Valve Corporation
 * Copyright (C) 2017-2020 LunarG, Inc.
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
Use Push Descriptors to Draw Textured Cube
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

/* This sample is the same as the draw_textured_cube sample except that it
 * uses push descriptors instead of descriptor sets.  The output should be
 * the same as draw_textured_cube, a cube with LunarG texture
 */

/* We've setup cmake to process push_descriptors.vert and push_descriptors.frag           */
/* files containing the glsl shader code for this sample.  The generate-spirv script uses */
/* glslangValidator to compile the glsl into spir-v and places the spir-v into a struct   */
/* into a generated header file                                                           */

int sample_main(int argc, char *argv[]) {
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Draw Textured Cube with Push Descriptors";
    const bool depthPresent = true;

    process_command_line_args(info, argc, argv);
    init_global_layer_properties(info);
    init_instance_extension_names(info);
    init_device_extension_names(info);
    /* VULKAN_KEY_START */
    uint32_t extension_count;
    char *layer_name = NULL;
    res = vkEnumerateInstanceExtensionProperties(layer_name, &extension_count, NULL);
    assert(res == VK_SUCCESS);
    // To use PUSH_DESCRIPTOR, you must also specify GET_PHYSICAL_DEVICE_PROPERTIES_2
    bool supports_device_properties_2 = false;
    if (extension_count > 0) {
        std::vector<VkExtensionProperties> available_instance_extensions(extension_count);

        res = vkEnumerateInstanceExtensionProperties(layer_name, &extension_count, &available_instance_extensions[0]);
        assert(res == VK_SUCCESS);
        for (const auto &extension_props : available_instance_extensions) {
            if (strcmp(extension_props.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0) {
                supports_device_properties_2 = true;
                break;
            }
        }
    }
    if (!supports_device_properties_2) {
        std::cout << "No GET_PHYSICAL_DEVICE_PROPERTIES_2 extension" << std::endl;
        return 0;
    }
    info.instance_extension_names.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    init_instance(info, sample_title);
    init_enumerate_device(info);
    // Once instance is created, need to make sure the extension is available
    bool supports_push_descriptors = false;
    res = vkEnumerateDeviceExtensionProperties(info.gpus[0], nullptr, &extension_count, nullptr);
    assert(res == VK_SUCCESS);
    if (extension_count > 0) {
        std::vector<VkExtensionProperties> available_device_extensions(extension_count);

        res = vkEnumerateDeviceExtensionProperties(info.gpus[0], nullptr, &extension_count, &available_device_extensions[0]);
        assert(res == VK_SUCCESS);
        for (const auto &extension_props : available_device_extensions) {
            if (strcmp(extension_props.extensionName, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME) == 0) {
                supports_push_descriptors = true;
                break;
            }
        }
    }
    if (!supports_push_descriptors) {
        std::cout << "No extension for push descriptors" << std::endl;
        return 0;
    }
    info.device_extension_names.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);

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
    init_texture(info);
    init_uniform_buffer(info);
    // Need to specify that descriptor set layout will be for push descriptors
    init_descriptor_and_pipeline_layouts(info, true, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
    init_renderpass(info, depthPresent);
#include "push_descriptors.vert.h"
#include "push_descriptors.frag.h"
    VkShaderModuleCreateInfo vert_info = {};
    VkShaderModuleCreateInfo frag_info = {};
    vert_info.sType = frag_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vert_info.codeSize = sizeof(push_descriptors_vert);
    vert_info.pCode = push_descriptors_vert;
    frag_info.codeSize = sizeof(push_descriptors_frag);
    frag_info.pCode = push_descriptors_frag;
    init_shaders(info, &vert_info, &frag_info);

    init_framebuffers(info, depthPresent);
    init_vertex_buffer(info, g_vb_texture_Data, sizeof(g_vb_texture_Data), sizeof(g_vb_texture_Data[0]), true);
    init_pipeline_cache(info);
    init_pipeline(info, depthPresent);

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    VkSemaphore imageAcquiredSemaphore;
    VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
    imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    imageAcquiredSemaphoreCreateInfo.pNext = NULL;
    imageAcquiredSemaphoreCreateInfo.flags = 0;

    res = vkCreateSemaphore(info.device, &imageAcquiredSemaphoreCreateInfo, NULL, &imageAcquiredSemaphore);
    assert(res == VK_SUCCESS);

    // Get the index of the next available swapchain image:
    res = vkAcquireNextImageKHR(info.device, info.swap_chain, UINT64_MAX, imageAcquiredSemaphore, VK_NULL_HANDLE,
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

    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline);

    VkWriteDescriptorSet descriptor_writes[2];
    memset(descriptor_writes, 0, sizeof(descriptor_writes));

    // Setup descriptor writes to pass to vkCmdPushDescriptorSetKHR
    // Populate with info about our uniform buffer
    descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].pNext = NULL;
    descriptor_writes[0].dstSet = 0;  // dstSet is ignored by the extension
    descriptor_writes[0].descriptorCount = 1;
    descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].pBufferInfo = &info.uniform_data.buffer_info;  // populated by init_uniform_buffer()
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].dstBinding = 0;

    // Populate with info about our sampled image
    descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].pNext = NULL;
    descriptor_writes[1].dstSet = 0;  // dstSet is ignored by the extension
    descriptor_writes[1].descriptorCount = 1;
    descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[1].pImageInfo = &info.texture_data.image_info;  // populated by init_texture()
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].dstBinding = 1;

    // Find address of extension call and make the call
    PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR =
        (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(info.device, "vkCmdPushDescriptorSetKHR");
    assert(vkCmdPushDescriptorSetKHR != nullptr);
    vkCmdPushDescriptorSetKHR(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout, 0, 2, descriptor_writes);

    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);

    init_viewports(info);
    init_scissors(info);

    vkCmdDraw(info.cmd, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);
    res = vkEndCommandBuffer(info.cmd);
    assert(res == VK_SUCCESS);

    const VkCommandBuffer cmd_bufs[] = {info.cmd};
    VkFenceCreateInfo fenceInfo;
    VkFence drawFence;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(info.device, &fenceInfo, NULL, &drawFence);

    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info[1] = {};
    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 1;
    submit_info[0].pWaitSemaphores = &imageAcquiredSemaphore;
    submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = cmd_bufs;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.graphics_queue, 1, submit_info, drawFence);
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
    do {
        res = vkWaitForFences(info.device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    } while (res == VK_TIMEOUT);
    assert(res == VK_SUCCESS);
    res = vkQueuePresentKHR(info.present_queue, &present);
    assert(res == VK_SUCCESS);

    wait_seconds(1);
    /* VULKAN_KEY_END */
    if (info.save_images) write_ppm(info, "drawtexturedcube");

    vkDestroyFence(info.device, drawFence, NULL);
    vkDestroySemaphore(info.device, imageAcquiredSemaphore, NULL);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);
    destroy_textures(info);
    destroy_descriptor_pool(info);
    destroy_vertex_buffer(info);
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);
    destroy_descriptor_and_pipeline_layouts(info);
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
