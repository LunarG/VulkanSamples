/*
 * LunarGravity - gravityscenesplash.hpp
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

#pragma once

#include <vector>

#include "gravityscene.hpp"

class GravityTexture;
class GravityShader;
class GravityUniformBuffer;

struct SplashTexture {
    std::string name;
    std::string file;
    uint8_t index;
    GravityTexture *gravity_texture;
};

struct SplashShader {
    std::string name;
    std::string prefix;
    GravityShader *shader;
};

struct SplashShaderVerts {
    uint8_t num_pos_comps;
    uint8_t num_tex_coords;
    uint8_t num_tex_coord_comps;
    std::vector<float> data;
};

struct SplashUniformBuffer {
    uint32_t size;
    uint32_t offset;
    GravityUniformBuffer *uniform_buffer;
};

class GravitySceneSplash : public GravityScene {
   public:
    // Create a protected constructor
    GravitySceneSplash(std::string &scene_file, Json::Value &root, GravityInstanceExtIf *inst_ext_if);

    // Make the destructor public
    virtual ~GravitySceneSplash();

    virtual bool Load(GravityDeviceExtIf *dev_ext_if, GravityDeviceMemoryManager *dev_memory_mgr, VkFormat rt_color_format, VkFormat rt_depth_stencil_format);
    virtual bool Start(VkRenderPass render_pass, VkCommandBuffer &cmd_buf);
    virtual bool Update(float comp_time, float game_time);
    virtual bool Draw(VkCommandBuffer &cmd_buf);
    virtual bool End();
    virtual bool Unload();

   protected:
    // We don't want any copy constructors
    GravitySceneSplash(const GravitySceneSplash &scene) = delete;
    GravitySceneSplash &operator=(const GravitySceneSplash &scene) = delete;

    SplashShader m_shader;
    SplashTexture m_splash_texture;
    SplashUniformBuffer m_uniform_buffer;
    SplashShaderVerts m_vertices;

    VkDescriptorSetLayout m_vk_desc_set_layout;
    VkPipelineLayout m_vk_pipeline_layout;
    VkRenderPass m_vk_render_pass;
    VkPipelineCache m_vk_pipeline_cache;
    VkPipeline m_vk_pipeline;
    VkDescriptorPool m_vk_desc_pool;
    std::vector<VkDescriptorSet> m_framebuffer_desc_set;
};