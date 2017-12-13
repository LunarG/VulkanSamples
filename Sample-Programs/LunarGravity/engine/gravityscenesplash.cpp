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
    m_splash_texture.name = texture["name"].asString();
    m_splash_texture.file = texture["file"].asString();
    m_splash_texture.index = static_cast<uint8_t>(texture["index"].asUInt());
    m_splash_texture.gravity_texture = nullptr;

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
    if (vertices["position components"].isNull() || vertices["texture coord groups"].isNull() ||
        vertices["texture coord components"].isNull() || vertices["data"].isNull() || !vertices["data"].isArray()) {
        std::string error_msg = "GravitySceneSplash::GravitySceneSplash - vertices in scene file ";
        error_msg += m_scene_file.c_str();
        error_msg += " missing sub-sections";
        logger.LogError(error_msg);
        exit(-1);
    }
    m_vertices.num_pos_comps = static_cast<uint8_t>(vertices["position components"].asUInt());
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
    m_vk_desc_pool = VK_NULL_HANDLE;
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
    if (!texture->Read(m_splash_texture.file)) {
        logger.LogError("GravitySceneSplash::Load - failed to read GravityTexture");
        return false;
    }
    m_splash_texture.gravity_texture = texture;

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

bool GravitySceneSplash::Start(VkRenderPass render_pass, VkCommandBuffer &cmd_buf) {
    GravityLogger &logger = GravityLogger::getInstance();

    // Load the texture onto the HW
    if (!m_splash_texture.gravity_texture->Load(cmd_buf)) {
        logger.LogError("GravitySceneSplash::Start - failed to load GravityTexture");
        return false;
    }

    m_vk_render_pass = render_pass;

    // Fill in the uniform buffer first with an identify MVP, then the vertex data
    uint32_t stride = m_vertices.num_pos_comps + (m_vertices.num_tex_coords * m_vertices.num_tex_coord_comps);
    uint64_t num_verts = m_vertices.data.size() / stride;
    uint64_t stride_bytes = sizeof(float) * (16 + num_verts * (m_vertices.num_pos_comps + (m_vertices.num_tex_coords * 4)));
    uint64_t total_data_size_bytes = stride_bytes * m_num_framebuffers;
    if (!m_uniform_buffer.uniform_buffer->Load(static_cast<uint32_t>(stride_bytes))) {
        logger.LogError("GravitySceneSplash::Start - failed to load GravityUniformBuffer");
        return false;
    }
    float *uni_buf_data = reinterpret_cast<float *>(m_uniform_buffer.uniform_buffer->Map(0, total_data_size_bytes));
    // Make a copy per framebuffer to render as fast as possible.
    for (uint32_t copy = 0; copy < m_num_framebuffers; ++copy) {
        // Identity MVP Matrix
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
        for (uint32_t vert = 0; vert < num_verts; ++vert) {
            // Put in the position info
            for (uint32_t comp = 0; comp < m_vertices.num_pos_comps; ++comp) {
                *uni_buf_data++ = m_vertices.data[(vert * stride) + comp];
            }
            // Write out each texture coordinate.
            for (uint32_t tc = 0; tc < m_vertices.num_tex_coords; ++tc) {
                uint32_t comp = 0;
                uint32_t tc_offset = (vert * stride) + m_vertices.num_pos_comps + (tc * m_vertices.num_tex_coord_comps);
                // Write out each texture coordinate component.  We always write out 4 per
                // Texture coordinate since it makes strides easier to use.
                for (comp = 0; comp < m_vertices.num_tex_coord_comps; comp++) {
                    *uni_buf_data++ = m_vertices.data[tc_offset + comp];
                }
                for (; comp < 4; ++comp) {
                    *uni_buf_data++ = 0;
                }
            }
        }
    }
    m_uniform_buffer.uniform_buffer->Unmap();

    // Bind the Vulkan uniform buffer to the memory we just wrote to
    if (!m_uniform_buffer.uniform_buffer->Bind()) {
        logger.LogError("GravitySceneSplash::Start - failed to bind uniform buffer");
        return false;
    }

    // Setup a descriptor set per framebuffer, so we can allow rendering a framebuffer while
    // simultaneously building the next one, and freeing the previous one.
    m_framebuffer_desc_set.resize(m_num_framebuffers);

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
    VkResult vk_result = vkCreateDescriptorSetLayout(m_dev_ext_if->m_vk_device, &descriptor_layout, nullptr, &m_vk_desc_set_layout);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravitySceneSplash::Start failed vk3riptorSetLayout with error ";
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
    vk_result = vkCreatePipelineLayout(m_dev_ext_if->m_vk_device, &pipeline_create_info, nullptr, &m_vk_pipeline_layout);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravitySceneSplash::Start failed vkCreatePipelineLayout with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
    pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    // Create a pipeline cache
    vk_result = vkCreatePipelineCache(m_dev_ext_if->m_vk_device, &pipeline_cache_create_info, nullptr, &m_vk_pipeline_cache);
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
    vk_result = vkCreateGraphicsPipelines(m_dev_ext_if->m_vk_device, m_vk_pipeline_cache, 1,
                                          &graphics_pipeline_create_info, nullptr, &m_vk_pipeline);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravitySceneSplash::Start failed vkCreateGraphicsPipelines with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    VkDescriptorPoolSize desc_pool_size_info[2];
    desc_pool_size_info[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc_pool_size_info[0].descriptorCount = m_num_framebuffers;
    desc_pool_size_info[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    desc_pool_size_info[1].descriptorCount = m_num_framebuffers;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = NULL;
    descriptor_pool_create_info.maxSets = m_num_framebuffers;
    descriptor_pool_create_info.poolSizeCount = 2;
    descriptor_pool_create_info.pPoolSizes = desc_pool_size_info;

    // Create a descriptor pool to create the descriptor sets out of.
    vk_result = vkCreateDescriptorPool(m_dev_ext_if->m_vk_device, &descriptor_pool_create_info, nullptr,
                                       &m_vk_desc_pool);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravitySceneSplash::Start failed vkCreateDescriptorPool with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // We want to allocate one descriptor set for now.
    VkDescriptorSetAllocateInfo desc_set_alloc_info = {};
    desc_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    desc_set_alloc_info.pNext = nullptr;
    desc_set_alloc_info.descriptorPool = m_vk_desc_pool;
    desc_set_alloc_info.descriptorSetCount = 1;
    desc_set_alloc_info.pSetLayouts = &m_vk_desc_set_layout;

    VkDescriptorImageInfo descriptor_image_info = m_splash_texture.gravity_texture->GetVkDescriptorImageInfo();

    VkWriteDescriptorSet write_descriptor_sets[2];
    write_descriptor_sets[0] = {};
    write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_sets[0].descriptorCount = 1;
    write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_sets[1] = {};
    write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_sets[1].dstBinding = 1;
    write_descriptor_sets[1].descriptorCount = 1;
    write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_sets[1].pImageInfo = &descriptor_image_info;

    for (uint32_t i = 0; i < m_num_framebuffers; i++) {
        VkDescriptorBufferInfo desc_buf_info = m_uniform_buffer.uniform_buffer->GetDescriptorInfo(m_cur_framebuffer);
        write_descriptor_sets[0].pBufferInfo = &desc_buf_info;
        vk_result = vkAllocateDescriptorSets(m_dev_ext_if->m_vk_device, &desc_set_alloc_info, &m_framebuffer_desc_set[i]);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravitySceneSplash::Start failed vkAllocateDescriptorSets with error ";
            error_msg += vk_result;
            logger.LogError(error_msg);
            return false;
        }
        write_descriptor_sets[0].dstSet = m_framebuffer_desc_set[i];
        write_descriptor_sets[1].dstSet = m_framebuffer_desc_set[i];
        vkUpdateDescriptorSets(m_dev_ext_if->m_vk_device, 2, write_descriptor_sets, 0, NULL);
    }

    return true;
}

bool GravitySceneSplash::Update(float comp_time, float game_time) {
    bool success = true;
    return success;
}

bool GravitySceneSplash::Draw(VkCommandBuffer &cmd_buf) {
    bool success = true;
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vk_pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vk_pipeline_layout, 0, 1,
                            &m_framebuffer_desc_set[m_cur_framebuffer], 0, nullptr);

#if 0
    // Set a full render area viewport/scissor
    VkViewport viewport = {};
    VkRect2D scissor = {};
    viewport.height = (float)m_height;
    viewport.width = (float)m_width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);
    scissor.extent.width = m_width;
    scissor.extent.height = m_height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    // Do the draw
    vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);
#endif // Brainpain

    if (++m_cur_framebuffer >= m_num_framebuffers) {
        m_cur_framebuffer = 0;
    }
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
    if (!m_splash_texture.gravity_texture->Unload()) {
        logger.LogError("GravitySceneSplash::End - failed to unload GravityTexture");
        return false;
    }
    return true;
}

bool GravitySceneSplash::Unload() {
    if (VK_NULL_HANDLE != m_vk_pipeline) {
        vkDestroyDescriptorPool(m_dev_ext_if->m_vk_device, m_vk_desc_pool, nullptr);
        m_vk_desc_pool = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vk_pipeline) {
        vkDestroyPipeline(m_dev_ext_if->m_vk_device, m_vk_pipeline, nullptr);
        m_vk_pipeline = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vk_pipeline_cache) {
        vkDestroyPipelineCache(m_dev_ext_if->m_vk_device, m_vk_pipeline_cache, nullptr);
        m_vk_pipeline_cache = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vk_pipeline_layout) {
        vkDestroyPipelineLayout(m_dev_ext_if->m_vk_device, m_vk_pipeline_layout, nullptr);
        m_vk_pipeline_layout = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vk_desc_set_layout) {
        vkDestroyDescriptorSetLayout(m_dev_ext_if->m_vk_device, m_vk_desc_set_layout, nullptr);
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
    if (nullptr != m_splash_texture.gravity_texture) {
        delete m_splash_texture.gravity_texture;
        m_splash_texture.gravity_texture = nullptr;
    }
    return true;
}
