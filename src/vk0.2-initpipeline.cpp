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

int main(int argc, char **argv)
{
    VkResult res;
    struct sample_info info = {};
    char sample_title[] = "Graphics Pipeline Sample";

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
    init_and_begin_command_buffer(info);
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

    /* VULKAN_KEY_START */
    VkPipelineCacheCreateInfo pipelineCache;
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCache.pNext = NULL;
    pipelineCache.initialData = 0;
    pipelineCache.initialSize = 0;
    pipelineCache.maxSize = 0;

    res = vkCreatePipelineCache(info.device, &pipelineCache, &info.pipelineCache);
    assert(!res);

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

    VkPipelineColorBlendStateCreateInfo cb;
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.pNext = NULL;
    VkPipelineColorBlendAttachmentState att_state[1];
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE; /* All the other fields in att_state should be ignored if this is false */
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;
    cb.logicOpEnable = VK_FALSE;         /* All logic op parameters should be ignored if this is false */
    cb.alphaToCoverageEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo vp;
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.pNext = NULL;
    vp.viewportCount = 1;

    VkPipelineDepthStencilStateCreateInfo ds;
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.pNext = NULL;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_EQUAL;
    ds.depthBoundsEnable = VK_FALSE;
    ds.stencilTestEnable = VK_FALSE;
    ds.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    ds.front = ds.back;

    VkPipelineMultisampleStateCreateInfo   ms;
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pNext = NULL;
    ms.sampleMask = 1;
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
    pipeline.pViewportState      = &vp;
    pipeline.pDepthStencilState  = &ds;
    pipeline.pStages             = info.shaderStages;
    pipeline.stageCount          = 2;
    pipeline.renderPass          = info.render_pass;
    pipeline.subpass             = 0;

    res = vkCreateGraphicsPipelines(info.device, info.pipelineCache, 1, &pipeline, &info.pipeline);
    assert(!res);
    end_and_submit_command_buffer(info);
    /* VULKAN_KEY_END */

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
    vkDestroyInstance(info.inst);
    destroy_window(info);
}

