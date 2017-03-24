/*
 * LunarGravity - gravityshader.cpp
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

#include <cstring>
#include <fstream>
#include <string>
#include <sstream>

#include "gravityinstanceextif.hpp"
#include "gravitydeviceextif.hpp"
#include "gravitylogger.hpp"
#include "gravitydevicememory.hpp"
#include "gravityshader.hpp"

GravityShader::GravityShader(GravityInstanceExtIf *inst_ext_if, GravityDeviceExtIf *dev_ext_if,
                               GravityDeviceMemoryManager *dev_memory_mgr) {
    m_read = false;
    m_shader_prefix = "";
    for (uint8_t shader = 0; shader < GRAVITY_SHADER_NUM_STAGES; shader++) {
        m_shader_data[shader].valid = false;
        m_shader_data[shader].vk_shader_flag = (VkShaderStageFlagBits)(1 << shader);
        m_shader_data[shader].vk_shader_module = VK_NULL_HANDLE;
    }
    m_inst_ext_if = inst_ext_if;
    m_dev_ext_if = dev_ext_if;
    m_dev_memory_mgr = dev_memory_mgr;
}

GravityShader::~GravityShader() {
    Cleanup();
}

void GravityShader::Cleanup() {
    for (uint8_t shader = 0; shader < GRAVITY_SHADER_NUM_STAGES; shader++) {
        if (VK_NULL_HANDLE != m_shader_data[shader].vk_shader_module) {
            vkDestroyShaderModule(m_dev_ext_if->m_device, m_shader_data[shader].vk_shader_module, NULL);
            m_shader_data[shader].vk_shader_module = VK_NULL_HANDLE;
        }
        m_shader_data[shader].valid = false;
    }

    m_read = false;
    m_shader_prefix = "";
}

bool GravityShader::Read(std::string const &shader_prefix) {
    GravityLogger &logger = GravityLogger::getInstance();
    std::string real_shader_prefix = "resources/shaders/";
    real_shader_prefix += shader_prefix;
    m_shader_prefix = shader_prefix;
    m_read = false;

    for (uint8_t shader = 0; shader < GRAVITY_SHADER_NUM_STAGES; shader++) {
        std::string full_shader_name = real_shader_prefix;
        switch (shader) {
            case GRAVITY_SHADER_VERTEX:
                full_shader_name += "-vs.spv";
                break;
            case GRAVITY_SHADER_TESSELLATION_CONTROL:
                full_shader_name += "-cs.spv";
                break;
            case GRAVITY_SHADER_TESSELLATION_EVALUATION:
                full_shader_name += "-es.spv";
                break;
            case GRAVITY_SHADER_GEOMETRY:
                full_shader_name += "-gs.spv";
                break;
            case GRAVITY_SHADER_FRAGMENT:
                full_shader_name += "-fs.spv";
                break;
            default:
                continue;
        }
        std::ifstream *infile = nullptr;
        std::string line;
        std::stringstream strstream;
        size_t shader_spv_size;
        char* shader_spv_content;

        infile = new std::ifstream(full_shader_name.c_str(), std::ifstream::in | std::ios::binary);
        if (nullptr == infile || infile->fail()) {
            continue;
        }
        strstream << infile->rdbuf();
        infile->close();

        shader_spv_size = strstream.str().size();

        // Read teh file contents
        shader_spv_content = new char[shader_spv_size];
        memcpy(shader_spv_content, strstream.str().c_str(), shader_spv_size);
        delete infile;

        m_shader_data[shader].valid = true;

        VkShaderModuleCreateInfo shader_module_create_info;
        shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader_module_create_info.pNext = nullptr;
        shader_module_create_info.codeSize = shader_spv_size;
        shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(shader_spv_content);
        shader_module_create_info.flags = 0;
        VkResult vk_result = vkCreateShaderModule(m_dev_ext_if->m_device, &shader_module_create_info, NULL, &m_shader_data[shader].vk_shader_module);
        if (VK_SUCCESS != vk_result) {
            m_shader_data[shader].valid = false;
            std::string error_msg = "GravityTexture::Read failed to read shader ";
            error_msg += full_shader_name;
            error_msg += " with error ";
            error_msg += vk_result;
            logger.LogError(error_msg);
        }
        delete[] shader_spv_content;
        shader_spv_content = nullptr;
        shader_spv_size = 0;
    }

    // At least need a vertex and fragment shader
    if (m_shader_data[GRAVITY_SHADER_VERTEX].valid && m_shader_data[GRAVITY_SHADER_FRAGMENT].valid) {
        m_read = true;
    }

    if (!m_read) {
        Cleanup();
    }

    return m_read;
}

bool GravityShader::Load() {
    return true;
}

bool GravityShader::Unload() {
    return true;
}
