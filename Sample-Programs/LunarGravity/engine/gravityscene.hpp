/*
 * LunarGravity - gravityscene.hpp
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

#include <string>

#include <json/json.h>

class GravityInstanceExtIf;
class GravityDeviceExtIf;
class GravityDeviceMemoryManager;

class GravityScene {
   public:
    // Factory Method
    static GravityScene *ReadFile(std::string scene_file, GravityInstanceExtIf *inst_ext_if);

    // Create a protected constructor
    GravityScene(std::string &scene_file, Json::Value &root, GravityInstanceExtIf *inst_ext_if) {
        m_scene_file = scene_file;
        m_root = root;
        m_inst_ext_if = inst_ext_if;
    }

    // We don't want any copy constructors
    GravityScene(const GravityScene &scene) = delete;
    GravityScene &operator=(const GravityScene &scene) = delete;

    // Make the destructor public
    virtual ~GravityScene() { ; }
    
    virtual bool Load(GravityDeviceExtIf *dev_ext_if, GravityDeviceMemoryManager *dev_memory_mgr, VkFormat rt_color_format, VkFormat rt_depth_stencil_format);
    virtual bool Start(VkRenderPass render_pass) = 0;
    virtual bool Update(float comp_time, float game_time) = 0;
    virtual bool Draw(VkCommandBuffer &cmd_buf) = 0;
    virtual bool End() = 0;
    virtual bool Unload() = 0;
    void SetDimensions(uint32_t width, uint32_t height) { m_width = width; m_height = height; }
    bool HasClearColor() { return m_has_clear_color; }
    const float* GetClearColor() { return m_clear_color; }
    bool HasClearDepth() { return m_has_clear_depth; }
    float GetClearDepth() { return m_clear_depth; }
    bool HasClearStencil() { return m_has_clear_stencil; }
    uint32_t GetClearStencil() { return m_clear_stencil; }

   protected:
    std::string m_scene_file;

    // Json Root info
    Json::Value m_root;
    GravityInstanceExtIf *m_inst_ext_if;
    GravityDeviceExtIf *m_dev_ext_if;
    GravityDeviceMemoryManager *m_dev_memory_mgr;
    uint32_t m_width;
    uint32_t m_height;
    VkFormat m_rt_color_format;
    VkFormat m_rt_depth_stencil_format;
    bool m_has_clear_color;
    float m_clear_color[4];
    bool m_has_clear_depth;
    float m_clear_depth;
    bool m_has_clear_stencil;
    uint32_t m_clear_stencil;
};