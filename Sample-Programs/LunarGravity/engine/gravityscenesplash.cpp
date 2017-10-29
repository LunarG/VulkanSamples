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
    Json::Value clear_color_json = root["clear color"];
    if (clear_color_json.isNull() || clear_color_json.size() < 4) {
        m_has_clear_color = false;
        m_clear_color[0] = 0.0f;
        m_clear_color[1] = 0.0f;
        m_clear_color[2] = 0.0f;
        m_clear_color[3] = 1.0f;
    } else {
        m_has_clear_color = true;
        uint8_t index = 0;
        for (Json::ValueIterator clear_it = clear_color_json.begin(); clear_it != clear_color_json.end(); clear_it++) {
            Json::Value cur_data = (*clear_it);
            m_clear_color[index++] = cur_data.asFloat();
            if (index == 4) {
                break;
            }
        }
    }

    // Depth clear value (optional)
    Json::Value clear_depth_json = root["clear depth"];
    if (clear_depth_json.isNull()) {
        m_has_clear_depth = false;
        m_clear_depth = 1.0f;
    } else {
        m_has_clear_depth = true;
        m_clear_depth = clear_depth_json.asFloat();
    }

    // Stencil clear value (optional)
    Json::Value clear_stencil_json = root["clear stencil"];
    if (clear_stencil_json.isNull()) {
        m_has_clear_stencil = false;
        m_clear_stencil = 0;
    } else {
        m_has_clear_stencil = true;
        m_clear_stencil = clear_stencil_json.asUInt();
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
    m_vk_pipeline_cache = VK_NULL_HANDLE;
    m_vk_pipeline = VK_NULL_HANDLE;
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

bool GravitySceneSplash::Start(VkRenderPass render_pass) {
    GravityLogger &logger = GravityLogger::getInstance();
    if (!m_texture.texture->Load()) {
        logger.LogError("GravitySceneSplash::Start - failed to load GravityTexture");
        return false;
    }
    if (!m_uniform_buffer.uniform_buffer->Load()) {
        logger.LogError("GravitySceneSplash::Start - failed to load GravityUniformBuffer");
        return false;
    }

    m_vk_render_pass = render_pass;

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

    VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
    pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    // Create a pipeline cache
    vk_result = vkCreatePipelineCache(m_dev_ext_if->m_device, &pipeline_cache_create_info, nullptr, &m_vk_pipeline_cache);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravitySceneSplash::Start failed vkCreatePipelineCache with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // We don't use dynamic state in this, so just create a zeroed out struct.
    VkDynamicState dynamic_state_enables[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pDynamicStates = dynamic_state_enables;
    dynamic_state_enables[0] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamic_state_enables[1] = VK_DYNAMIC_STATE_SCISSOR;
    dynamic_state_create_info.dynamicStateCount = 2;

    // Setup the vertex input state.  Basically, we do nothing.
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {};
    pipeline_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Setup the vertex input assembly.  Basically, we just let the assembler know we're going to
    // use a triangle list.
    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info = {};
    pipeline_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_input_assembly_state_create_info.pNext = nullptr;
    pipeline_input_assembly_state_create_info.flags = 0;
    pipeline_input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Setup the rasterization state. We'll perform normal fill while culling backwards faces.
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info = {};
    pipeline_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    pipeline_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    pipeline_rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline_rasterization_state_create_info.depthClampEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    pipeline_rasterization_state_create_info.lineWidth = 1.0f;

    // Define the color blending state information
    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = {};
    pipeline_color_blend_attachment_state.colorWriteMask = 0xf;
    pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info = {};
    pipeline_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_color_blend_state_create_info.attachmentCount = 1;
    pipeline_color_blend_state_create_info.pAttachments = &pipeline_color_blend_attachment_state;

    // Define the viewport state.
    VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {};
    pipeline_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_viewport_state_create_info.viewportCount = 1;
    pipeline_viewport_state_create_info.scissorCount = 1;

    // Define the depth stencil state
    VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info = {};
    pipeline_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    pipeline_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    pipeline_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    pipeline_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.back.failOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.back.passOp = VK_STENCIL_OP_KEEP;
    pipeline_depth_stencil_state_create_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
    pipeline_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    pipeline_depth_stencil_state_create_info.front = pipeline_depth_stencil_state_create_info.back;

    // Define the multisample state (we don't use any so it's just set to defaults)
    VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info = {};
    pipeline_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_multisample_state_create_info.pSampleMask = nullptr;
    pipeline_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Setup the shader info
    std::vector<VkPipelineShaderStageCreateInfo> pipeline_shader_stage_create_info;
    if (!m_shader.shader->Load(pipeline_shader_stage_create_info)) {
        logger.LogError("GravitySceneSplash::Start failed loading shaders for scene");
        return false;
    }

    // Define the graphics pipeline based on the above info
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext = nullptr;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = pipeline_shader_stage_create_info.size();
    graphics_pipeline_create_info.pStages = pipeline_shader_stage_create_info.data();
    graphics_pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_create_info;
    graphics_pipeline_create_info.pTessellationState = nullptr;
    graphics_pipeline_create_info.pViewportState = &pipeline_viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &pipeline_multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    graphics_pipeline_create_info.layout = m_vk_pipeline_layout;
    graphics_pipeline_create_info.renderPass = m_vk_render_pass;
    graphics_pipeline_create_info.subpass = 0;
    graphics_pipeline_create_info.basePipelineIndex = 0;
    
    // Create the graphics pipeline
    vk_result = vkCreateGraphicsPipelines(m_dev_ext_if->m_device, m_vk_pipeline_cache, 1,
                                          &graphics_pipeline_create_info, nullptr, &m_vk_pipeline);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravitySceneSplash::Start failed vkCreateGraphicsPipelines with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

#if 0

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
    if (VK_NULL_HANDLE != m_vk_pipeline) {
        vkDestroyPipeline(m_dev_ext_if->m_device, m_vk_pipeline, nullptr);
        m_vk_pipeline = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vk_pipeline_cache) {
        vkDestroyPipelineCache(m_dev_ext_if->m_device, m_vk_pipeline_cache, nullptr);
        m_vk_pipeline_cache = VK_NULL_HANDLE;
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
