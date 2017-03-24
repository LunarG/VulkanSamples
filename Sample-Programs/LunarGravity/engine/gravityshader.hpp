/*
 * LunarGravity - gravityshader.hpp
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
#include <vulkan/vulkan.h>

class GravityInstanceExtIf;
class GravityDeviceExtIf;
struct GravityDeviceMemory;
class GravityDeviceMemoryManager;

enum GravityShaderStage {
    GRAVITY_SHADER_VERTEX = 0,
    GRAVITY_SHADER_TESSELLATION_CONTROL,
    GRAVITY_SHADER_TESSELLATION_EVALUATION,
    GRAVITY_SHADER_GEOMETRY,
    GRAVITY_SHADER_FRAGMENT,
    GRAVITY_SHADER_NUM_STAGES
};

struct GravityShaderData {
    bool valid;
    VkShaderStageFlagBits vk_shader_flag;
    VkShaderModule vk_shader_module;
};

class GravityShader {
   public:
    // Create a protected constructor
    GravityShader(GravityInstanceExtIf *inst_ext_if, GravityDeviceExtIf *dev_ext_if, GravityDeviceMemoryManager *dev_memory);

    // We don't want any copy constructors
    GravityShader(const GravityShader &shader) = delete;
    GravityShader &operator=(const GravityShader &shader) = delete;

    // Make the destructor public
    virtual ~GravityShader();

    bool Read(std::string const &shader_prefix);
    bool Load();
    bool Unload();

   protected:
    GravityInstanceExtIf *m_inst_ext_if;
    GravityDeviceExtIf *m_dev_ext_if;
    GravityDeviceMemoryManager *m_dev_memory_mgr;

    bool m_read;
    std::string m_shader_prefix;
    GravityShaderData m_shader_data[GRAVITY_SHADER_NUM_STAGES];

    void Cleanup();
};