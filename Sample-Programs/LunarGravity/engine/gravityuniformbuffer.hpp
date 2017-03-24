/*
 * LunarGravity - gravityuniformbuffer.hpp
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

class GravityUniformBuffer {
   public:
    // Create a protected constructor
    GravityUniformBuffer(GravityInstanceExtIf *inst_ext_if, GravityDeviceExtIf *dev_ext_if, GravityDeviceMemoryManager *dev_memory,
                         uint32_t size);

    // We don't want any copy constructors
    GravityUniformBuffer(const GravityUniformBuffer &uniform_buffer) = delete;
    GravityUniformBuffer &operator=(const GravityUniformBuffer &uniform_buffer) = delete;

    // Make the destructor public
    virtual ~GravityUniformBuffer();

    bool Load();
    void *Map(uint64_t offset, uint64_t size);
    void Unmap();
    bool Bind(uint32_t offset);
    bool Unload();
    uint32_t Size() { return m_size; }

   protected:
    GravityInstanceExtIf *m_inst_ext_if;
    GravityDeviceExtIf *m_dev_ext_if;
    GravityDeviceMemoryManager *m_dev_memory_mgr;

    uint32_t m_size;
    VkBuffer m_vk_buffer;
    GravityDeviceMemory m_memory;
    void *m_cpu_addr;

    void Cleanup();
};