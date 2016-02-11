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
Use per-thread command buffers to draw 3 triangles
*/

/* Set up Vulkan pipeline and use three threads to create 3       */
/* command buffers, each using a vertex buffer to draw a triangle */

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include <samples_platform.h>

struct Vertex
{
    float posX, posY, posZ, posW;    // Position data
    float r, g, b, a;                // Color
};
#define XYZ1(_x_, _y_, _z_)         (_x_), (_y_), (_z_), 1.f
static const Vertex triData[] =
{
   { XYZ1( -0.25, -0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1(  0.25, -0.25, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1(  0,  0.25, 0 ),    XYZ1( 1.f, 0.f, 0.f ) },
   { XYZ1( -0.75, -0.25, 0 ), XYZ1( 0.f, 1.f, 0.f ) },
   { XYZ1( -0.25, -0.25, 0 ), XYZ1( 0.f, 1.f, 0.f ) },
   { XYZ1(  -0.5,  0.25, 0 ), XYZ1( 0.f, 1.f, 0.f ) },
   { XYZ1(  0.25, -0.25, 0 ), XYZ1( 0.f, 0.f, 1.f ) },
   { XYZ1(  0.75, -0.25, 0 ), XYZ1( 0.f, 0.f, 1.f ) },
   { XYZ1(  0.5,  0.25, 0 ),  XYZ1( 0.f, 0.f, 1.f ) },
};

struct {
        VkBuffer buf;
        VkDeviceMemory mem;
} vertex_buffer[3];

VkCommandBuffer threadCmdBufs[4];

static void * per_thread_code(void *arg);

/* For this sample, we'll start with GLSL so the shader function is plain */
/* and then use the glslang GLSLtoSPV utility to convert it to SPIR-V for */
/* the driver.  We do this for clarity rather than using pre-compiled     */
/* SPIR-V                                                                 */

static const char *vertShaderText =
    "#version 400\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (location = 0) in vec4 pos;\n"
    "layout (location = 1) in vec4 inColor;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "out gl_PerVertex { \n"
    "    vec4 gl_Position;\n"
    "};\n"
    "void main() {\n"
    "    outColor = inColor;\n"
    "    gl_Position = pos;\n"
    "}\n";

static const char *fragShaderText=
    "#version 400\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (location = 0) in vec4 color;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "   outColor = color;\n"
    "}\n";

static struct sample_info info = {};
int sample_main()
{
    VkResult U_ASSERT_ONLY res;

    char sample_title[] = "MT Cmd Buffer Sample";
    const bool depthPresent = false;

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

    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo;
    presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreCreateInfo.pNext = NULL;
    presentCompleteSemaphoreCreateInfo.flags = 0;

    res = vkCreateSemaphore(info.device,
                            &presentCompleteSemaphoreCreateInfo,
                            NULL,
                            &info.presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    // Get the index of the next available swapchain image:
    res = vkAcquireNextImageKHR(info.device, info.swap_chain,
                                      UINT64_MAX,
                                      info.presentCompleteSemaphore,
                                      VK_NULL_HANDLE,
                                      &info.current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(res == VK_SUCCESS);


    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext                  = NULL;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pPipelineLayoutCreateInfo.pPushConstantRanges    = NULL;
    pPipelineLayoutCreateInfo.setLayoutCount         = 0;
    pPipelineLayoutCreateInfo.pSetLayouts            = NULL;

    res = vkCreatePipelineLayout(info.device,
                                 &pPipelineLayoutCreateInfo, NULL,
                                 &info.pipeline_layout);
    assert(res == VK_SUCCESS);
    init_renderpass(info, depthPresent, false); // Can't clear in renderpass load because we re-use pipeline
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info, depthPresent);

    /* The binding and attributes should be the same for all 3 vertex buffers, so init here */
    info.vi_binding.binding = 0;
    info.vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    info.vi_binding.stride = sizeof(triData[0]);

    info.vi_attribs[0].binding = 0;
    info.vi_attribs[0].location = 0;
    info.vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[0].offset = 0;
    info.vi_attribs[1].binding = 0;
    info.vi_attribs[1].location = 1;
    info.vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[1].offset = 16;

    init_pipeline_cache(info);
    init_pipeline(info, depthPresent);

    VkImageSubresourceRange srRange = {};
    srRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    srRange.baseMipLevel = 0;
    srRange.levelCount = VK_REMAINING_MIP_LEVELS;
    srRange.baseArrayLayer = 0;
    srRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    VkClearColorValue clear_color[1];
    clear_color[0].float32[0] = 0.2f;
    clear_color[0].float32[1] = 0.2f;
    clear_color[0].float32[2] = 0.2f;
    clear_color[0].float32[3] = 0.2f;

    /* We need to do the clear here instead of as a load op since all 3 threads share the */
    /* same pipeline / renderpass                                                         */
    set_image_layout(info, info.buffers[info.current_buffer].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkCmdClearColorImage(info.cmd,
           info.buffers[info.current_buffer].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
           clear_color, 1, &srRange );
    set_image_layout(info, info.buffers[info.current_buffer].image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    res = vkEndCommandBuffer(info.cmd);
    const VkCommandBuffer cmd_bufs[] = { info.cmd };
    VkFence nullFence = VK_NULL_HANDLE;

    VkSubmitInfo submit_info[1] = {};
    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 1;
    submit_info[0].pWaitSemaphores = &info.presentCompleteSemaphore;
    submit_info[0].pWaitDstStageMask = NULL;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = cmd_bufs;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, submit_info, nullFence);
    assert(!res);

    /* VULKAN_KEY_START */

    VkCommandBufferAllocateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd.pNext = NULL;
    cmd.commandPool = info.cmd_pool;
    cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd.commandBufferCount = 4;

    res = vkAllocateCommandBuffers(info.device, &cmd, threadCmdBufs);
    assert(res == VK_SUCCESS);

    sample_platform_thread vk_threads[3];
    for (size_t i = 0; i < 3; i++) {
        sample_platform_thread_create(&vk_threads[i], &per_thread_code, (void *) i);
    }

    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = 0;
    cmd_buf_info.pInheritanceInfo = NULL;
    res = vkBeginCommandBuffer(threadCmdBufs[3], &cmd_buf_info);
    assert(res == VK_SUCCESS);

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
    vkCmdPipelineBarrier(threadCmdBufs[3], VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, NULL, 0, NULL, 1, &prePresentBarrier);

    res = vkEndCommandBuffer(threadCmdBufs[3]);
    assert(res == VK_SUCCESS);

    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 0;
    submit_info[0].pWaitSemaphores = NULL;
    submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
    submit_info[0].commandBufferCount = 4; /* 3 from threads + prePresentBarrier */
    submit_info[0].pCommandBuffers = threadCmdBufs;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    /* Wait for all of the threads to finish */
    for (int i = 0; i < 3; i++) {
        sample_platform_thread_join(vk_threads[i], NULL);
    }

    VkFenceCreateInfo fenceInfo;
    VkFence drawFence;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(info.device, &fenceInfo, NULL, &drawFence);

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, submit_info, drawFence);
    assert(!res);

    /* Make sure command buffer is finished before presenting */
    do {
        res = vkWaitForFences(info.device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    } while(res == VK_TIMEOUT);
    assert(res == VK_SUCCESS);

    execute_present_image(info);

    wait_seconds(1);
    /* VULKAN_KEY_END */

    vkFreeCommandBuffers(info.device, info.cmd_pool, 3, threadCmdBufs);
    vkDestroyBuffer(info.device, vertex_buffer[0].buf, NULL);
    vkDestroyBuffer(info.device, vertex_buffer[1].buf, NULL);
    vkDestroyBuffer(info.device, vertex_buffer[2].buf, NULL);
    vkFreeMemory(info.device, vertex_buffer[0].mem, NULL);
    vkFreeMemory(info.device, vertex_buffer[1].mem, NULL);
    vkFreeMemory(info.device, vertex_buffer[2].mem, NULL);
    vkDestroySemaphore(info.device, info.presentCompleteSemaphore, NULL);
    vkDestroyFence(info.device, drawFence, NULL);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout, NULL);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
    return 0;
}

static void * per_thread_code(void *arg)
{
    /* This code should be executed by each of the three threads.  It will  */
    /* create a vertex buffer with position and color per vertex, then load */
    /* commands into the thread's designated command buffer to draw the     */
    /* triangle                                                             */
    VkResult U_ASSERT_ONLY res;
    // int threadNum = *((int*)(&arg));
    size_t threadNum = (size_t) arg;

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.size = 3*sizeof(triData[0]);
    buf_info.queueFamilyIndexCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(info.device, &buf_info, NULL, &vertex_buffer[threadNum].buf);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(info.device, vertex_buffer[threadNum].buf, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    bool pass;
    pass = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(pass);

    res = vkAllocateMemory(info.device, &alloc_info, NULL, &(vertex_buffer[threadNum].mem));
    assert(res == VK_SUCCESS);

    uint8_t *pData;
    res = vkMapMemory(info.device, vertex_buffer[threadNum].mem, 0, mem_reqs.size, 0, (void **) &pData);
    assert(res == VK_SUCCESS);

    memcpy(pData, &triData[threadNum*3], 3*sizeof(triData[0]));

    vkUnmapMemory(info.device, vertex_buffer[threadNum].mem);

    res = vkBindBufferMemory(info.device,
            vertex_buffer[threadNum].buf,
            vertex_buffer[threadNum].mem, 0);
    assert(res == VK_SUCCESS);

    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = 0;
    cmd_buf_info.pInheritanceInfo = NULL;

    res = vkBeginCommandBuffer(threadCmdBufs[threadNum], &cmd_buf_info);
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
    rp_begin.clearValueCount = 0;
    rp_begin.pClearValues = NULL;

    vkCmdBeginRenderPass(threadCmdBufs[threadNum], &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(threadCmdBufs[threadNum], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  info.pipeline);
    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(threadCmdBufs[threadNum], 0, 1, &vertex_buffer[threadNum].buf, offsets);

    VkViewport viewport;
    viewport.height = (float) info.height;
    viewport.width = (float) info.width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.x = 0;
    viewport.y = 0;
    vkCmdSetViewport(threadCmdBufs[threadNum], 0, NUM_VIEWPORTS, &viewport);

    VkRect2D scissor;
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(threadCmdBufs[threadNum], 0, NUM_SCISSORS, &scissor);

    vkCmdDraw(threadCmdBufs[threadNum], 3, 1, 0, 0);
    vkCmdEndRenderPass(threadCmdBufs[threadNum]);

    res = vkEndCommandBuffer(threadCmdBufs[threadNum]);
    assert(res == VK_SUCCESS);

    return NULL;
}
