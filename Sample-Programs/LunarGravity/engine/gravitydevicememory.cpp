/*
 * LunarGravity - gravitydevicememory.cpp
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
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <string>

#include "gravitylogger.hpp"
#include "gravityinstanceextif.hpp"
#include "gravitydeviceextif.hpp"
#include "gravitydevicememory.hpp"

GravityDeviceMemoryManager::GravityDeviceMemoryManager(GravityInstanceExtIf *inst_ext_if, VkPhysicalDevice *phys_dev) {
    VkPhysicalDeviceProperties phys_dev_props;
    m_inst_ext_if = inst_ext_if;
    m_dev_ext_if = nullptr;
    m_vk_phys_dev = phys_dev;
    m_vk_dev_mem_props = {};

    // Get Memory information and properties
    vkGetPhysicalDeviceMemoryProperties(*phys_dev, &m_vk_dev_mem_props);

    // Get the Memory limits
    vkGetPhysicalDeviceProperties(*phys_dev, &phys_dev_props);
    m_vk_dev_limits = phys_dev_props.limits;
}

GravityDeviceMemoryManager::~GravityDeviceMemoryManager() {
    m_inst_ext_if = nullptr;
    m_dev_ext_if = nullptr;
}

void GravityDeviceMemoryManager::SetupDevIf(GravityDeviceExtIf *dev_ext_if) { m_dev_ext_if = dev_ext_if; }

bool GravityDeviceMemoryManager::AllocateMemory(GravityDeviceMemory &memory, const VkMemoryPropertyFlags &flags) {
    GravityLogger &logger = GravityLogger::getInstance();
    VkMemoryAllocateInfo mem_alloc_info = {};
    VkResult vk_result = VK_SUCCESS;

    // Allocate the memory, in the correct location
    mem_alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc_info.pNext = NULL;
    mem_alloc_info.allocationSize = memory.vk_mem_reqs.size;
    mem_alloc_info.memoryTypeIndex = 0;

    bool found_mem_type = false;
    uint32_t type_bits = memory.vk_mem_reqs.memoryTypeBits;
    for (uint32_t mem_type = 0; mem_type < m_vk_dev_mem_props.memoryTypeCount; mem_type++) {
        if ((type_bits & 0x1) && ((m_vk_dev_mem_props.memoryTypes[mem_type].propertyFlags & flags) == flags)) {
            mem_alloc_info.memoryTypeIndex = mem_type;
            found_mem_type = true;
            memory.vk_memory_type_index = mem_type;
            memory.vk_memory_heap_index = m_vk_dev_mem_props.memoryTypes[mem_type].heapIndex;
            break;
        }
        type_bits >>= 1;
    }
    if (!found_mem_type) {
        logger.LogError(
            "GravityDeviceMemory::AllocateMemory failed to find device local memory"
            " type for depth stencil surface");
        return false;
    }

    if (m_vk_dev_mem_props.memoryHeaps[memory.vk_memory_heap_index].size < memory.vk_mem_reqs.size) {
        logger.LogError("GravityDeviceMemory::AllocateMemory not enough memory remaining in that heap");
        return false;
    }

    m_vk_dev_mem_props.memoryHeaps[memory.vk_memory_heap_index].size -= memory.vk_mem_reqs.size;

    // Allocate memory for the depth/stencil buffer
    vk_result = vkAllocateMemory(m_dev_ext_if->m_device, &mem_alloc_info, nullptr, &memory.vk_device_memory);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityDeviceMemory::AllocateMemory failed to allocate device memory with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    return true;
}

bool GravityDeviceMemoryManager::MapMemory(GravityDeviceMemory &memory, VkDeviceSize offset, VkDeviceSize size, void **ppData) {
    GravityLogger &logger = GravityLogger::getInstance();
    if (((m_vk_dev_limits.minMemoryMapAlignment - 1) & offset) != 0) {
        std::string error_msg = "GravityDeviceMemory::MapMemory offset ";
        error_msg += std::to_string(offset);
        error_msg += " does not meet alignment requirements of ";
        error_msg += std::to_string(m_vk_dev_limits.minMemoryMapAlignment);
        logger.LogError(error_msg);
        return false;
    }
    VkResult vk_result = vkMapMemory(m_dev_ext_if->m_device, memory.vk_device_memory, offset, size, 0, ppData);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityDeviceMemory::MapMemory failed to map device memory with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }
    return true;
}

void GravityDeviceMemoryManager::UnmapMemory(GravityDeviceMemory &memory) {
    vkUnmapMemory(m_dev_ext_if->m_device, memory.vk_device_memory);
}

bool GravityDeviceMemoryManager::FreeMemory(GravityDeviceMemory &memory) {
    if (VK_NULL_HANDLE != memory.vk_device_memory) {
        vkFreeMemory(m_dev_ext_if->m_device, memory.vk_device_memory, NULL);

        // Re-add the memory back to the heap size
        m_vk_dev_mem_props.memoryHeaps[memory.vk_memory_heap_index].size += memory.vk_mem_reqs.size;

        // Clear the memory handle so we don't accidentally try to re-free it.
        memory.vk_device_memory = VK_NULL_HANDLE;
        return true;
    }

    return false;
}
