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
Create Graphics Pipeline
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
    char sample_title[] = "Graphics Pipeline Sample";

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
    init_uniform_buffer(info);
    init_renderpass(info);
    init_framebuffers(info);
    init_vertex_buffer(info, g_vb_solid_face_colors_Data, sizeof(g_vb_solid_face_colors_Data),
                       sizeof(g_vb_solid_face_colors_Data[0]), false);
    init_descriptor_and_pipeline_layouts(info, false);
    init_descriptor_pool(info, false);
    init_descriptor_set(info, false);
    init_shaders(info, vertShaderText, fragShaderText);

    /* VULKAN_KEY_START */
    VkPipelineCacheCreateInfo pipelineCache;
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCache.pNext = NULL;
    pipelineCache.initialData = 0;
    pipelineCache.initialSize = 0;
    pipelineCache.maxSize = 0;

    res = vkCreatePipelineCache(info.device, &pipelineCache, &info.pipelineCache);
    assert(res == VK_SUCCESS);

    VkDynamicState                         dynamicStateEnables[VK_DYNAMIC_STATE_NUM];
    VkPipelineDynamicStateCreateInfo       dynamicState = {};
    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = NULL;
    dynamicState.pDynamicStates = dynamicStateEnables;
    dynamicState.dynamicStateCount = 0;

    VkPipelineVertexInputStateCreateInfo vi;
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.pNext = NULL;
    vi.bindingCount = 1;
    vi.pVertexBindingDescriptions = &info.vi_binding;
    vi.attributeCount = 2;
    vi.pVertexAttributeDescriptions = info.vi_attribs;

    VkPipelineInputAssemblyStateCreateInfo ia;
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.pNext = NULL;
    ia.primitiveRestartEnable = VK_FALSE;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterStateCreateInfo rs;
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTER_STATE_CREATE_INFO;
    rs.pNext = NULL;
    rs.fillMode = VK_FILL_MODE_SOLID;
    rs.cullMode = VK_CULL_MODE_BACK;
    rs.frontFace = VK_FRONT_FACE_CCW;
    rs.depthClipEnable = VK_TRUE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.depthBias = 0;
    rs.depthBiasClamp = 0;
    rs.slopeScaledDepthBias = 0;
    rs.lineWidth = 0;

    VkPipelineColorBlendStateCreateInfo cb;
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.pNext = NULL;
    VkPipelineColorBlendAttachmentState att_state[1];
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    att_state[0].blendOpAlpha = VK_BLEND_OP_ADD;
    att_state[0].blendOpColor = VK_BLEND_OP_ADD;
    att_state[0].srcBlendColor = VK_BLEND_ZERO;
    att_state[0].destBlendColor = VK_BLEND_ZERO;
    att_state[0].srcBlendAlpha = VK_BLEND_ZERO;
    att_state[0].destBlendAlpha = VK_BLEND_ZERO;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;
    cb.logicOpEnable = VK_FALSE;
    cb.logicOp = VK_LOGIC_OP_NOOP;
    cb.alphaToCoverageEnable = VK_FALSE;
    cb.alphaToOneEnable = VK_FALSE;
    cb.blendConst[0] = 1.0f;
    cb.blendConst[1] = 1.0f;
    cb.blendConst[2] = 1.0f;
    cb.blendConst[3] = 1.0f;

    VkPipelineViewportStateCreateInfo vp = {};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.pNext = NULL;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDepthStencilStateCreateInfo ds;
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.pNext = NULL;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.minDepthBounds = 0;
    ds.maxDepthBounds = 0;
    ds.stencilTestEnable = VK_FALSE;
    ds.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    ds.front = ds.back;

    VkPipelineMultisampleStateCreateInfo   ms;
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pNext = NULL;
    ms.pSampleMask = NULL;
    ms.rasterSamples = 1;
    ms.sampleShadingEnable = VK_FALSE;
    ms.minSampleShading = 0.0;

    VkGraphicsPipelineCreateInfo pipeline;
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.pNext               = NULL;
    pipeline.layout              = info.pipeline_layout;
    pipeline.basePipelineHandle  = 0;
    pipeline.basePipelineIndex   = 0;
    pipeline.flags               = 0;
    pipeline.pVertexInputState   = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterState        = &rs;
    pipeline.pColorBlendState    = &cb;
    pipeline.pTessellationState  = NULL;
    pipeline.pMultisampleState   = &ms;
    pipeline.pDynamicState       = &dynamicState;
    pipeline.pViewportState      = &vp;
    pipeline.pDepthStencilState  = &ds;
    pipeline.pStages             = info.shaderStages;
    pipeline.stageCount          = 2;
    pipeline.renderPass          = info.render_pass;
    pipeline.subpass             = 0;

    res = vkCreateGraphicsPipelines(info.device, info.pipelineCache, 1, &pipeline, &info.pipeline);
    assert(res == VK_SUCCESS);
    execute_end_command_buffer(info);
    execute_queue_command_buffer(info);
    /* VULKAN_KEY_END */

    vkDestroyPipeline(info.device, info.pipeline);
    vkDestroyPipelineCache(info.device, info.pipelineCache);
    vkDestroyBuffer(info.device, info.uniform_data.buf);
    vkFreeMemory(info.device, info.uniform_data.mem);
    for(int i = 0; i < NUM_DESCRIPTOR_SETS; i++)
        vkDestroyDescriptorSetLayout(info.device, info.desc_layout[i]);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout);
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
    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        vkDestroyFramebuffer(info.device, info.framebuffers[i]);
    }
    free(info.framebuffers);
    vkDestroyRenderPass(info.device, info.render_pass);
    vkDestroyDevice(info.device);
    vkDestroyInstance(info.inst);
    destroy_window(info);
}

