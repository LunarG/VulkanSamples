/*
 * LunarGravity - gravitytexture.hpp
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

class GravityTexture {
   public:
    // Create a protected constructor
    GravityTexture(GravityInstanceExtIf *inst_ext_if, GravityDeviceExtIf *dev_ext_if, GravityDeviceMemoryManager *dev_memory);

    // We don't want any copy constructors
    GravityTexture(const GravityTexture &texture) = delete;
    GravityTexture &operator=(const GravityTexture &texture) = delete;

    // Make the destructor public
    virtual ~GravityTexture();

    bool Read(std::string const &filename);
    bool Load();
    bool Unload();

   protected:
    GravityInstanceExtIf *m_inst_ext_if;
    GravityDeviceExtIf *m_dev_ext_if;
    GravityDeviceMemoryManager *m_dev_memory_mgr;

    bool m_read;
    std::string m_filename;
    uint32_t m_width;
    uint32_t m_height;
    uint8_t m_num_comps;
    uint8_t m_comp_bytes;
    uint8_t *m_cpu_data;
    VkFormat m_vk_format;
    VkImage m_vk_image;
    GravityDeviceMemory m_memory;

    bool ReadPPM(std::string const &filename);
    void Cleanup();
};