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
Draw a Multitextured Quad who's coordinates dynamically change with time.
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

static const VertexUV g_vb_textured_quad[] =
{
    { XYZ1(-1.5f, -1.5f, -1.0f), UV(0.f, 0.f) },
    { XYZ1( 1.5f, -1.5f, -1.0f), UV(1.f, 0.f) },
    { XYZ1(-1.5f,  1.5f, -1.0f), UV(0.f, 1.f) },
    { XYZ1(-1.5f,  1.5f, -1.0f), UV(0.f, 0.f) },
    { XYZ1( 1.5f, -1.5f, -1.0f), UV(1.f, 0.f) },
    { XYZ1( 1.5f,  1.5f, -1.0f), UV(1.f, 1.f) },
};

// For this sample, we'll start with GLSL so the shader function is plain
// and then use the glslang GLSLtoSPV utility to convert it to SPIR-V for
// the driver.  We do this for clarity rather than using pre-compiled    
// SPIR-V                                                                
//
const char* vertShaderText =
                "#version 400\n"
                "#extension GL_ARB_separate_shader_objects  : require\n"
                "#extension GL_ARB_shading_language_420pack : require\n"
                "\n"
                "layout (location = 0) in  vec4 pos;\n"
                "layout (location = 1) in  vec2 inTexCoords;\n"
                "layout (location = 0) out vec2 outTexCoords;\n"
                "\n"
                "void main() {\n"
                "    outTexCoords = inTexCoords;\n"
                "    gl_Position  = pos;\n"
                "}\n";

const char* fragShaderText=
                "#version 400\n"
                "#extension GL_ARB_separate_shader_objects  : require\n"
                "#extension GL_ARB_shading_language_420pack : require\n"
                "\n"
                "layout (binding = 0) uniform buf\n"
                "{\n"
                "    float msSinceStart;\n"
                "    float msMaxTime;\n"
                "} ubuf;\n"
                "\n"
                "layout (binding = 1) uniform sampler2D tex0;   // Neighborhood\n"
                "layout (binding = 2) uniform sampler2D tex1;   // Dusk Sky\n"
                "layout (binding = 3) uniform sampler2D tex2;   // Clouds\n"
                "layout (binding = 4) uniform sampler2D tex3;   // Neighborhood lights\n"
                "layout (binding = 5) uniform sampler2D tex4;   // Stars\n"
                "layout (binding = 6) uniform sampler2D tex5;   // Fog\n"
                "layout (binding = 7) uniform sampler2D tex6;   // Moon\n"
                "\n"
                "layout (location = 0) in  vec2 texcoord;\n"
                "layout (location = 0) out vec4 outColor;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    vec4 curColor;\n"
                "\n"
                "    // Apply neighborhood texture\n"
                "    curColor = textureLod(tex0, texcoord, 0.0);\n"
                "\n"
                "    // If neighborhood texture alpha is 0, then this is part of the sky\n"
                "    if (curColor.a == 0.0)\n"
                "    {\n"
                "        curColor = textureLod(tex1, texcoord, 0.0);\n"
                "    }\n"
                "    outColor = curColor;"
                "}\n";

int main(
    int     argc,
    char**  argv)
{
    VkResult U_ASSERT_ONLY  res;
    bool U_ASSERT_ONLY      pass;
    struct sample_info      info            = {};
    char                    sampleTitle[]   = "Draw Multitextured Quad";
    const bool              depthPresent    = true;
    float                   timeData[2]     = { 0.0f, 0.0f };

    // Setup the instance and device.
    //
    init_global_layer_properties(info);
    init_instance_extension_names(info);
    init_device_extension_names(info);
    init_instance(info, sampleTitle);
    init_enumerate_device(info);
    init_device(info);

    // Setup the window width/height to something big
    //
    info.width  = 1000;
    info.height = 1000;
    init_connection(info);
    init_window(info);

    init_swapchain_extension(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_texture(info);

    // Setup the uniform buffer for the time data we need to pass to the app
    //
    VkBufferCreateInfo bufInfo      = {};
    bufInfo.sType                   = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufInfo.pNext                   = NULL;
    bufInfo.usage                   = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufInfo.size                    = sizeof(timeData) * 4;
    bufInfo.queueFamilyIndexCount   = 0;
    bufInfo.pQueueFamilyIndices     = NULL;
    bufInfo.sharingMode             = VK_SHARING_MODE_EXCLUSIVE;
    bufInfo.flags                   = 0;
    res = vkCreateBuffer(info.device, &bufInfo, NULL, &info.uniform_data.buf);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(info.device, info.uniform_data.buf, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext                = NULL;
    allocInfo.memoryTypeIndex      = 0;
    allocInfo.allocationSize       = memReqs.size;

    pass = memory_type_from_properties(
                info,
                memReqs.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                &allocInfo.memoryTypeIndex);
    assert(pass);

    res = vkAllocateMemory(info.device, &allocInfo, NULL, &(info.uniform_data.mem));
    assert(res == VK_SUCCESS);

    uint8_t *pData;
    res = vkMapMemory(info.device, info.uniform_data.mem, 0, memReqs.size, 0, (void **)&pData);
    assert(res == VK_SUCCESS);

    memcpy(pData, &info.MVP, sizeof(timeData));

    vkUnmapMemory(info.device, info.uniform_data.mem);

    res = vkBindBufferMemory(
                info.device,
                info.uniform_data.buf,
                info.uniform_data.mem, 0);
    assert(res == VK_SUCCESS);

    info.uniform_data.buffer_info.buffer = info.uniform_data.buf;
    info.uniform_data.buffer_info.offset = 0;
    info.uniform_data.buffer_info.range  = sizeof(timeData);



    init_descriptor_and_pipeline_layouts(info, true);
    init_renderpass(info, depthPresent);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info, depthPresent);
    init_vertex_buffer(
        info,
        g_vb_textured_quad,
        sizeof(g_vb_textured_quad),
        sizeof(g_vb_textured_quad[0]),
        true);
    init_descriptor_pool(info, true);
    init_descriptor_set(info, true);
    init_pipeline_cache(info);
    init_pipeline(info, depthPresent);

    /* VULKAN_KEY_START */

    VkClearValue clearValues[2];
    clearValues[0].color.float32[0]     = 0.2f;
    clearValues[0].color.float32[1]     = 0.2f;
    clearValues[0].color.float32[2]     = 0.2f;
    clearValues[0].color.float32[3]     = 0.2f;
    clearValues[1].depthStencil.depth   = 1.0f;
    clearValues[1].depthStencil.stencil = 0;

    VkSemaphore             presentCompleteSemaphore;
    VkSemaphoreCreateInfo   presentCompleteSemaphoreCreateInfo;
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
    rp_begin.pClearValues = clearValues;

    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout,
            0, NUM_DESCRIPTOR_SETS, info.desc_set.data(), 0, NULL);

    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);

    init_viewports(info);
    init_scissors(info);

    vkCmdDraw(info.cmd, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);

    VkImageMemoryBarrier prePresentBarrier = {};
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = NULL;
    prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    prePresentBarrier.subresourceRange.baseMipLevel = 0;
    prePresentBarrier.subresourceRange.levelCount = 1;
    prePresentBarrier.subresourceRange.baseArrayLayer = 0;
    prePresentBarrier.subresourceRange.layerCount = 1;
    prePresentBarrier.image = info.buffers[info.current_buffer].image;
    vkCmdPipelineBarrier(info.cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0, NULL, 1, &prePresentBarrier);

    res = vkEndCommandBuffer(info.cmd);
    assert(res == VK_SUCCESS);

    const VkCommandBuffer cmd_bufs[] = { info.cmd };
    VkFenceCreateInfo   fenceInfo;
    VkFence             drawFence;
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
    /* VULKAN_KEY_END */

    vkDestroyFence(info.device, drawFence, NULL);
    vkDestroySemaphore(info.device, presentCompleteSemaphore, NULL);
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
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
}
