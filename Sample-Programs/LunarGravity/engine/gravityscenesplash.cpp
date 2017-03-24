/*
 * LunarGravity - gravityscenesplash.cpp
 *
 * Copyright (C) 2017 LunarG, Inc.
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
 *
 * Author: Mark Young <marky@lunarg.com>
 */

#include "gravitylogger.hpp"
#include "gravitydeviceextif.hpp"
#include "gravitydevicememory.hpp"
#include "gravitytexture.hpp"
#include "gravityshader.hpp"
#include "gravityuniformbuffer.hpp"
#include "gravityscenesplash.hpp"

GravitySceneSplash::GravitySceneSplash(std::string &scene_file, Json::Value &root, GravityInstanceExtIf *inst_ext_if)
    : GravityScene(scene_file, root, inst_ext_if) {
    GravityLogger &logger = GravityLogger::getInstance();
    if (root["texture"].isNull() || root["shader"].isNull() || root["vertices"].isNull()) {
        std::string error_msg = "GravitySceneSplash::GravitySceneSplash - Scene file ";
        error_msg += m_scene_file.c_str();
        error_msg += " invalid \"data\" section";
        logger.LogError(error_msg);
        exit(-1);
    }

    // Clear color (optional)
    Json::Value clear_json = root["clear color"];
    if (clear_json.isNull() || clear_json.size() < 4) {
        m_clear_color[0] = 0.0f;
        m_clear_color[1] = 0.0f;
        m_clear_color[2] = 0.0f;
        m_clear_color[3] = 1.0f;
    } else {
        uint8_t index = 0;
        for (Json::ValueIterator clear_it = clear_json.begin(); clear_it != clear_json.end(); clear_it++) {
            Json::Value cur_data = (*clear_it);
            m_clear_color[index++] = cur_data.asFloat();
            if (index == 4) {
                break;
            }
        }
    }

    // Read the texture data
    Json::Value texture = root["texture"];
    if (texture["name"].isNull() || texture["file"].isNull() || texture["index"].isNull()) {
        std::string error_msg = "GravitySceneSplash::GravitySceneSplash - texture in scene file ";
        error_msg += m_scene_file.c_str();
        error_msg += " missing sub-sections";
        logger.LogError(error_msg);
        exit(-1);
    }
    m_texture.name = texture["name"].asString();
    m_texture.file = texture["file"].asString();
    m_texture.index = static_cast<uint8_t>(texture["index"].asUInt());
    m_texture.texture = nullptr;

    // Read the shader data
    Json::Value shader_node = root["shader"];
    if (shader_node["name"].isNull() || shader_node["prefix"].isNull()) {
        std::string error_msg = "GravitySceneSplash::GravitySceneSplash - shaderin scene file ";
        error_msg += m_scene_file.c_str();
        error_msg += " missing sub-sections";
        logger.LogError(error_msg);
        exit(-1);
    }
    m_shader.name = shader_node["name"].asString();
    m_shader.prefix = shader_node["prefix"].asString();
    m_shader.shader = nullptr;

    // Read the vertex data
    Json::Value vertices = root["vertices"];
    if (vertices["vertex components"].isNull() || vertices["texture coord groups"].isNull() ||
        vertices["texture coord components"].isNull() || vertices["data"].isNull() || !vertices["data"].isArray()) {
        std::string error_msg = "GravitySceneSplash::GravitySceneSplash - vertices in scene file ";
        error_msg += m_scene_file.c_str();
        error_msg += " missing sub-sections";
        logger.LogError(error_msg);
        exit(-1);
    }
    m_vertices.num_vert_comps = static_cast<uint8_t>(vertices["vertex components"].asUInt());
    m_vertices.num_tex_coords = static_cast<uint8_t>(vertices["texture coord groups"].asUInt());
    m_vertices.num_tex_coord_comps = static_cast<uint8_t>(vertices["texture coord components"].asUInt());

    if (m_vertices.num_tex_coords > 1) {
        logger.LogWarning("GravitySceneSplash::GravitySceneSplash - too many texcoords groups.");
    }

    Json::Value vert_data = vertices["data"];
    m_vertices.data.resize(vert_data.size());
    uint8_t vert = 0;
    if (!vert_data.isNull() && vert_data.isArray()) {
        for (Json::ValueIterator vert_data_it = vert_data.begin(); vert_data_it != vert_data.end(); vert_data_it++) {
            Json::Value cur_data = (*vert_data_it);
            m_vertices.data[vert++] = cur_data.asFloat();
        }
    }

    m_uniform_buffer.uniform_buffer = nullptr;
    m_vk_render_pass = VK_NULL_HANDLE;
    m_vk_pipeline_layout = VK_NULL_HANDLE;
    m_vk_desc_set_layout = VK_NULL_HANDLE;
}

GravitySceneSplash::~GravitySceneSplash() {}

bool GravitySceneSplash::Load(GravityDeviceExtIf *dev_ext_if, GravityDeviceMemoryManager *dev_memory_mgr, VkFormat rt_color_format, VkFormat rt_depth_stencil_format) {
    GravityLogger &logger = GravityLogger::getInstance();

    if (!GravityScene::Load(dev_ext_if, dev_memory_mgr, rt_color_format, rt_depth_stencil_format)) {
        logger.LogError("GravitySceneSplash::Load - failed GravityScene::Load");
        return false;
    }

    // Create and read the texture, but don't actually load it until we start this scene.  That way
    // we only use the memory that we need.
    GravityTexture *texture = new GravityTexture(m_inst_ext_if, m_dev_ext_if, m_dev_memory_mgr);
    if (nullptr == texture) {
        logger.LogError("GravitySceneSplash::Load - failed to allocate GravityTexture");
        return false;
    }
    if (!texture->Read(m_texture.file)) {
        logger.LogError("GravitySceneSplash::Load - failed to read GravityTexture");
        return false;
    }

    m_texture.texture = texture;

    // Create and read the shader contents.  Again, don't actually load anything
    GravityShader *shader = new GravityShader(m_inst_ext_if, m_dev_ext_if, m_dev_memory_mgr);
    if (nullptr == shader) {
        logger.LogError("GravitySceneSplash::Load - failed to allocate GravityShader");
        return false;
    }
    if (!shader->Read(m_shader.prefix)) {
        logger.LogError("GravitySceneSplash::Load - failed to read GravityShader");
        return false;
    }
    m_shader.shader = shader;

    // Create and read the uniform buffer contents.  Again, don't actually load anything
    m_uniform_buffer.size = 20480;  // Start with 20k
    GravityUniformBuffer *uniform_buffer =
        new GravityUniformBuffer(m_inst_ext_if, m_dev_ext_if, m_dev_memory_mgr, m_uniform_buffer.size);
    if (nullptr == uniform_buffer) {
        logger.LogError("GravitySceneSplash::Load - failed to allocate GravityUniformBuffer");
        m_uniform_buffer.size = 0;
        return false;
    }
    m_uniform_buffer.offset = 0;
    m_uniform_buffer.uniform_buffer = uniform_buffer;

    return true;
}

bool GravitySceneSplash::Start() {
    GravityLogger &logger = GravityLogger::getInstance();
    if (!m_texture.texture->Load()) {
        logger.LogError("GravitySceneSplash::Start - failed to load GravityTexture");
        return false;
    }
    if (!m_uniform_buffer.uniform_buffer->Load()) {
        logger.LogError("GravitySceneSplash::Start - failed to load GravityUniformBuffer");
        return false;
    }

    // Fill in the uniform buffer first with an identify MVP, then the vertex data
    uint32_t stride = m_vertices.num_vert_comps + (m_vertices.num_tex_coords * m_vertices.num_tex_coord_comps);
    uint64_t num_verts = m_vertices.data.size() / stride;
    uint64_t data_size = sizeof(float) * (16 + num_verts * (m_vertices.num_vert_comps + (m_vertices.num_tex_coords * 4)));
    float *uni_buf_data = reinterpret_cast<float *>(m_uniform_buffer.uniform_buffer->Map(0, data_size));
    *uni_buf_data++ = 1.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 1.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 1.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 0.0f;
    *uni_buf_data++ = 1.0f;

    // Fill in vertex data
    for (uint32_t vert = 0; vert < m_vertices.data.size(); vert++) {
        for (uint32_t comp = 0; comp < 6; comp++) {
            if (comp < m_vertices.num_vert_comps + m_vertices.num_tex_coord_comps) {
                *uni_buf_data++ = m_vertices.data[(vert * stride) + comp];
            }
        }
    }
    m_uniform_buffer.uniform_buffer->Unmap();

    if (!m_shader.shader->Load()) {
        logger.LogError("GravitySceneSplash::Start - failed to load GravityShader");
        return false;
    }

    // Fill out a descriptor set layout binding.  The layout is kind of like defining
    // the form and organization of the contents of the descriptor set, but not the
    // contents themselves.  Think defining a structure versus using it as a type.
    VkDescriptorSetLayoutBinding layout_bindings[2] = {};
    layout_bindings[0].binding = 0;
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].descriptorCount = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_bindings[0].pImmutableSamplers = nullptr;
    layout_bindings[1].binding = 1;
    layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_bindings[1].descriptorCount = 1;
    layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layout_bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = NULL;
    descriptor_layout.bindingCount = 2;
    descriptor_layout.pBindings = layout_bindings;

    // Create the descriptor set layout for this scene
    VkResult vk_result = vkCreateDescriptorSetLayout(m_dev_ext_if->m_device, &descriptor_layout, nullptr, &m_vk_desc_set_layout);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravitySceneSplash::Start failed vkCreateDescriptorSetLayout with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Create the pipeline layout.  Mostly it's a collection of descriptor set layouts.  Since we only
    // have one, it's not that big of a deal.
    VkPipelineLayoutCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_create_info.pNext = nullptr;
    pipeline_create_info.setLayoutCount = 1;
    pipeline_create_info.pSetLayouts = &m_vk_desc_set_layout;

    // Create the pipeline layout for this scene
    vk_result = vkCreatePipelineLayout(m_dev_ext_if->m_device, &pipeline_create_info, nullptr, &m_vk_pipeline_layout);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravitySceneSplash::Start failed vkCreatePipelineLayout with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Define the attacment descriptors.  The initial layouts for the attachment will
    // LAYOUT_UNDEFINED since we haven't yet defined anything inside of them.
    VkAttachmentDescription attachment_descs[2] = {};
    attachment_descs[0].flags = 0;
    attachment_descs[0].format = m_rt_color_format;
    attachment_descs[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachment_descs[1].flags = 0;
    attachment_descs[1].format = m_rt_depth_stencil_format;
    attachment_descs[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // We want to transition each to the optimal
    VkAttachmentReference attachment_refs[2] = {};
    attachment_refs[0].attachment = 0;
    attachment_refs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_refs[1].attachment = 1;
    attachment_refs[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Define what we're using for the one subpass we'll define, basically we're
    // just using color and depth on a graphics pipeline.
    VkSubpassDescription subpass_desc = {};
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = nullptr;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &attachment_refs[0];
    subpass_desc.pResolveAttachments = nullptr;
    subpass_desc.pDepthStencilAttachment = &attachment_refs[1];
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = nullptr;

    // Create a render pass with everything up above
    VkRenderPassCreateInfo renderpass_create_info = {};
    renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_create_info.pNext = nullptr;
    renderpass_create_info.flags = 0;
    renderpass_create_info.attachmentCount = 2;
    renderpass_create_info.pAttachments = attachment_descs;
    renderpass_create_info.subpassCount = 1;
    renderpass_create_info.pSubpasses = &subpass_desc;
    renderpass_create_info.dependencyCount = 0;
    renderpass_create_info.pDependencies = nullptr;

    // Create the render pass we'll use.
    vk_result = vkCreateRenderPass(m_dev_ext_if->m_device, &renderpass_create_info, nullptr, &m_vk_render_pass);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravitySceneSplash::Start failed vkCreateRenderPass with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

#if 0

    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;
    VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterizationStateCreateInfo rs;
    VkPipelineColorBlendStateCreateInfo cb;
    VkPipelineDepthStencilStateCreateInfo ds;
    VkPipelineViewportStateCreateInfo vp;
    VkPipelineMultisampleStateCreateInfo ms;
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkResult U_ASSERT_ONLY err;

    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = demo->pipeline_layout;

    memset(&vi, 0, sizeof(vi));
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.depthClampEnable = VK_FALSE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.lineWidth = 1.0f;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].colorWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] =
        VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] =
        VK_DYNAMIC_STATE_SCISSOR;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.back.failOp = VK_STENCIL_OP_KEEP;
    ds.back.passOp = VK_STENCIL_OP_KEEP;
    ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = NULL;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Two stages: vs and fs
    pipeline.stageCount = 2;
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = demo_prepare_vs(demo);
    shaderStages[0].pName = "main";

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = demo_prepare_fs(demo);
    shaderStages[1].pName = "main";

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache(demo->device, &pipelineCache, NULL,
                                &demo->pipelineCache);
    assert(!err);

    pipeline.pVertexInputState = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterizationState = &rs;
    pipeline.pColorBlendState = &cb;
    pipeline.pMultisampleState = &ms;
    pipeline.pViewportState = &vp;
    pipeline.pDepthStencilState = &ds;
    pipeline.pStages = shaderStages;
    pipeline.renderPass = m_vk_render_pass;
    pipeline.pDynamicState = &dynamicState;

    pipeline.renderPass = m_vk_render_pass;

    err = vkCreateGraphicsPipelines(demo->device, demo->pipelineCache, 1,
                                    &pipeline, NULL, &demo->pipeline);
    assert(!err);

    vkDestroyShaderModule(demo->device, demo->frag_shader_module, NULL);
    vkDestroyShaderModule(demo->device, demo->vert_shader_module, NULL);
    const VkDescriptorPoolSize type_counts[2] = {
            [0] =
                {
                 .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = demo->swapchainImageCount,
                },
            [1] =
                {
                 .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = demo->swapchainImageCount * DEMO_TEXTURE_COUNT,
                },
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .maxSets = demo->swapchainImageCount,
        .poolSizeCount = 2,
        .pPoolSizes = type_counts,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorPool(demo->device, &descriptor_pool, NULL,
                                 &demo->desc_pool);
    assert(!err);
    VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet writes[2];
    VkResult U_ASSERT_ONLY err;

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = demo->desc_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &demo->desc_layout};

    VkDescriptorBufferInfo buffer_info;
    buffer_info.offset = 0;
    buffer_info.range = sizeof(struct vktexcube_vs_uniform);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for (unsigned int i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        tex_descs[i].sampler = demo->textures[i].sampler;
        tex_descs[i].imageView = demo->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    memset(&writes, 0, sizeof(writes));

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &buffer_info;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = DEMO_TEXTURE_COUNT;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = tex_descs;

    for (unsigned int i = 0; i < demo->swapchainImageCount; i++) {
        err = vkAllocateDescriptorSets(demo->device, &alloc_info, &demo->swapchain_image_resources[i].descriptor_set);
        assert(!err);
        buffer_info.buffer = demo->swapchain_image_resources[i].uniform_buffer;
        writes[0].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        writes[1].dstSet = demo->swapchain_image_resources[i].descriptor_set;
        vkUpdateDescriptorSets(demo->device, 2, writes, 0, NULL);
    }

    VkImageView attachments[2];
    attachments[1] = demo->depth.view;

    const VkFramebufferCreateInfo fb_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = NULL,
        .renderPass = m_vk_render_pass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = demo->width,
        .height = demo->height,
        .layers = 1,
    };
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    for (i = 0; i < demo->swapchainImageCount; i++) {
        attachments[0] = demo->swapchain_image_resources[i].view;
        err = vkCreateFramebuffer(demo->device, &fb_info, NULL,
                                  &demo->swapchain_image_resources[i].framebuffer);
        assert(!err);
    }

#endif

    return true;
}

bool GravitySceneSplash::Update(float comp_time, float game_time) {
    bool success = true;
    return success;
}

bool GravitySceneSplash::Draw(VkCommandBuffer &cmd_buf) {
    bool success = true;
    VkCommandBufferBeginInfo cmd_buf_info = {};
    VkClearValue clear_values[2] = {};
    VkRenderPassBeginInfo rp_begin = {};

    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = nullptr;
    cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    cmd_buf_info.pInheritanceInfo = nullptr;

    clear_values[0].color.float32[0] = m_clear_color[0];
    clear_values[0].color.float32[1] = m_clear_color[1];
    clear_values[0].color.float32[2] = m_clear_color[2];
    clear_values[0].color.float32[3] = m_clear_color[3];
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = nullptr;
    rp_begin.renderPass = m_vk_render_pass;
#if 0
    rp_begin.framebuffer = demo->swapchain_image_resources[demo->current_buffer].framebuffer, rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = demo->width;
    rp_begin.renderArea.extent.height = demo->height;
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);
    assert(!err);
    vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline_layout, 0, 1,
                            &demo->swapchain_image_resources[demo->current_buffer].descriptor_set, 0, NULL);
    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float)demo->height;
    viewport.width = (float)demo->width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
    vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);
    // Note that ending the renderpass changes the image's layout from
    // COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
    vkCmdEndRenderPass(cmd_buf);

    if (demo->separate_present_queue) {
        // We have to transfer ownership from the graphics queue family to the
        // present queue family to be able to present.  Note that we don't have
        // to transfer from present queue family back to graphics queue family at
        // the start of the next frame because we don't care about the image's
        // contents at that point.
        VkImageMemoryBarrier image_ownership_barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                        .pNext = NULL,
                                                        .srcAccessMask = 0,
                                                        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                                        .srcQueueFamilyIndex = demo->graphics_queue_family_index,
                                                        .dstQueueFamilyIndex = demo->present_queue_family_index,
                                                        .image = demo->swapchain_image_resources[demo->current_buffer].image,
                                                        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
                             NULL, 0, NULL, 1, &image_ownership_barrier);
    }
    err = vkEndCommandBuffer(cmd_buf);
    assert(!err);
#endif
    return success;
}

bool GravitySceneSplash::End() {
    GravityLogger &logger = GravityLogger::getInstance();
    if (!m_shader.shader->Unload()) {
        logger.LogError("GravitySceneSplash::End - failed to unload GravityShader");
        return false;
    }
    if (!m_uniform_buffer.uniform_buffer->Unload()) {
        logger.LogError("GravitySceneSplash::End - failed to unload GravityUniformBuffer");
        return false;
    }
    if (!m_texture.texture->Unload()) {
        logger.LogError("GravitySceneSplash::End - failed to unload GravityTexture");
        return false;
    }
    return true;
}

bool GravitySceneSplash::Unload() {
    if (VK_NULL_HANDLE != m_vk_render_pass) {
        vkDestroyRenderPass(m_dev_ext_if->m_device, m_vk_render_pass, nullptr);
        m_vk_render_pass = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vk_pipeline_layout) {
        vkDestroyPipelineLayout(m_dev_ext_if->m_device, m_vk_pipeline_layout, nullptr);
        m_vk_pipeline_layout = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vk_desc_set_layout) {
        vkDestroyDescriptorSetLayout(m_dev_ext_if->m_device, m_vk_desc_set_layout, nullptr);
        m_vk_desc_set_layout = VK_NULL_HANDLE;
    }
    if (nullptr != m_shader.shader) {
        delete m_shader.shader;
        m_shader.shader = nullptr;
    }
    if (nullptr != m_uniform_buffer.uniform_buffer) {
        delete m_uniform_buffer.uniform_buffer;
        m_uniform_buffer.uniform_buffer = nullptr;
    }
    if (nullptr != m_texture.texture) {
        delete m_texture.texture;
        m_texture.texture = nullptr;
    }
    return true;
}
