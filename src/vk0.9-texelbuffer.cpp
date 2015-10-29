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
Use a texel buffer to draw a magenta triangle
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>

#define DEPTH_PRESENT false

/* For this sample, we'll start with GLSL so the shader function is plain */
/* and then use the glslang GLSLtoSPV utility to convert it to SPIR-V for */
/* the driver.  We do this for clarity rather than using pre-compiled     */
/* SPIR-V                                                                 */

static const char *vertShaderText =
    "#version 140\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (std140, binding = 0) uniform samplerBuffer texels;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "vec2 vertices[3];\n"
    "float r;\n"
    "float g;\n"
    "float b;\n"
    "void main() {\n"
    "    r = texelFetch(texels, 0).r;\n"
    "    g = texelFetch(texels, 1).r;\n"
    "    b = texelFetch(texels, 2).r;\n"
    "    outColor = vec4(r, g, b, 1.0);\n"
    "    vertices[0] = vec2(-1.0, -1.0);\n"
    "    vertices[1] = vec2( 1.0, -1.0);\n"
    "    vertices[2] = vec2( 0.0,  1.0);\n"
    "    gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
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
    char sample_title[] = "Texel Buffer Sample";
    float texels[] = {1.0, 0.0, 1.0};

    init_global_layer_properties(info);
    info.instance_extension_names.push_back(VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME);
    info.device_extension_names.push_back(VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);

    if (info.gpu_props.limits.maxTexelBufferSize < sizeof(texels)) {
        std::cout << "maxTexelBufferSize too small\n";
        exit(-1);
    }

    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(info.gpus[0], VK_FORMAT_R32_SFLOAT, &props);
    if (!(props.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT)) {
        std::cout << "R32_SFLOAT format unsupported for texel buffer\n";
        exit(-1);
    }

    info.width = info.height = 500;
    init_connection(info);
    init_window(info);
    init_swapchain_extension(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    buf_info.size = sizeof(texels);
    buf_info.queueFamilyCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    VkBuffer texelBuf;
    res = vkCreateBuffer(info.device, &buf_info, &texelBuf);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(info.device, texelBuf, &mem_reqs);

    VkMemoryAllocInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    res = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(res == VK_SUCCESS);

    VkDeviceMemory texelMem;
    res = vkAllocMemory(info.device, &alloc_info, &texelMem);
    assert(res == VK_SUCCESS);

    uint8_t *pData;
    res = vkMapMemory(info.device, texelMem, 0, 0, 0, (void **) &pData);
    assert(res == VK_SUCCESS);

    memcpy(pData, &texels, sizeof(texels));

    vkUnmapMemory(info.device, texelMem);

    res = vkBindBufferMemory(info.device,
            texelBuf,
            texelMem, 0);
    assert(res == VK_SUCCESS);

    VkBufferViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_info.pNext = NULL;
    view_info.buffer = texelBuf;
    view_info.format = VK_FORMAT_R32_SFLOAT;
    view_info.offset = 0;
    view_info.range = sizeof(texels);

    VkDescriptorInfo texelDesc = {};
    res = vkCreateBufferView(info.device, &view_info,
            &texelDesc.bufferView);
    assert(res == VK_SUCCESS);

    texelDesc.bufferInfo.buffer = texelBuf;
    texelDesc.bufferInfo.offset = 0;
    texelDesc.bufferInfo.range = sizeof(texels);


    // init_descriptor_and_pipeline_layouts(info, false);
    VkDescriptorSetLayoutBinding layout_bindings[1];
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    layout_bindings[0].arraySize = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_bindings[0].pImmutableSamplers = NULL;

    /* Next take layout bindings and use them to create a descriptor set layout */
    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = NULL;
    descriptor_layout.count = 1;
    descriptor_layout.pBinding = layout_bindings;

    info.desc_layout.resize(NUM_DESCRIPTOR_SETS);
    res = vkCreateDescriptorSetLayout(info.device,
            &descriptor_layout, info.desc_layout.data());
    assert(res == VK_SUCCESS);

    /* Now use the descriptor layout to create a pipeline layout */
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext                  = NULL;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pPipelineLayoutCreateInfo.pPushConstantRanges    = NULL;
    pPipelineLayoutCreateInfo.descriptorSetCount     = NUM_DESCRIPTOR_SETS;
    pPipelineLayoutCreateInfo.pSetLayouts            = info.desc_layout.data();

    res = vkCreatePipelineLayout(info.device,
                                 &pPipelineLayoutCreateInfo,
                                 &info.pipeline_layout);
    assert(res == VK_SUCCESS);

    init_renderpass(info, DEPTH_PRESENT);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info, DEPTH_PRESENT);

    VkDescriptorTypeCount type_count[1];
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    type_count[0].count = 1;

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = NULL;
    descriptor_pool.poolUsage = VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT;
    descriptor_pool.maxSets = 1;
    descriptor_pool.count = 1;
    descriptor_pool.pTypeCount = type_count;

    res = vkCreateDescriptorPool(info.device,
        &descriptor_pool, &info.desc_pool);
    assert(res == VK_SUCCESS);

    info.desc_set.resize(NUM_DESCRIPTOR_SETS);
    res = vkAllocDescriptorSets(info.device, info.desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            NUM_DESCRIPTOR_SETS, info.desc_layout.data(),
            info.desc_set.data());
    assert(res == VK_SUCCESS);

    VkWriteDescriptorSet writes[1];

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].destSet = info.desc_set[0];
    writes[0].destBinding = 1;
    writes[0].count = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    writes[0].pDescriptors = &texelDesc;
    writes[0].destArrayElement = 0;

    vkUpdateDescriptorSets(info.device, 1, writes, 0, NULL);


    init_pipeline_cache(info);
    init_pipeline(info, DEPTH_PRESENT);

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
    uint32_t offset = 0;
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout,
            0, NUM_DESCRIPTOR_SETS, info.desc_set.data(), 1, &offset);

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

    vkCmdDraw(info.cmd, 3, 1, 0, 0);

    vkCmdEndRenderPass(info.cmd);

    execute_pre_present_barrier(info);

    res = vkEndCommandBuffer(info.cmd);
    const VkCmdBuffer cmd_bufs[] = { info.cmd };

    /* Make sure buffer is ready for rendering */
    res = vkQueueWaitSemaphore(info.queue, presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    execute_queue_cmdbuf(info, cmd_bufs);
    execute_present_image(info);

    wait_seconds(1);
    /* VULKAN_KEY_END */

    vkDestroySemaphore(info.device, presentCompleteSemaphore);
    destroy_pipeline(info);
    destroy_pipeline_cache(info);
    destroy_descriptor_pool(info);
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);
    destroy_descriptor_and_pipeline_layouts(info);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
}
