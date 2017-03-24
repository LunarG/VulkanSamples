/*
 * LunarGravity - gravitydevicememory.hpp
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

#include <vulkan/vulkan.h>

class GravityInstanceExtIf;
class GravityDeviceExtIf;

struct GravityDeviceMemory {
    VkMemoryRequirements vk_mem_reqs;
    uint32_t vk_memory_type_index;
    uint32_t vk_memory_heap_index;
    VkDeviceMemory vk_device_memory;
};

class GravityDeviceMemoryManager {
   public:
    GravityDeviceMemoryManager(GravityInstanceExtIf *inst_ext_if, VkPhysicalDevice *phys_dev);
    ~GravityDeviceMemoryManager();

    GravityDeviceMemoryManager(GravityDeviceMemoryManager const &) = delete;
    void operator=(GravityDeviceMemoryManager const &) = delete;

    void SetupDevIf(GravityDeviceExtIf *dev_ext_if);

    bool AllocateMemory(GravityDeviceMemory &memory, const VkMemoryPropertyFlags &flags);
    bool MapMemory(GravityDeviceMemory &memory, VkDeviceSize offset, VkDeviceSize size, void** ppData);
    void UnmapMemory(GravityDeviceMemory &memory);
    bool FreeMemory(GravityDeviceMemory &memory);

   private:
    GravityInstanceExtIf *m_inst_ext_if;
    GravityDeviceExtIf *m_dev_ext_if;
    VkPhysicalDevice *m_vk_phys_dev;
    VkPhysicalDeviceMemoryProperties m_vk_dev_mem_props;
    VkPhysicalDeviceLimits m_vk_dev_limits;
};