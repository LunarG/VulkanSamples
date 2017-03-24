/*
 * LunarGravity - gravitytexture.cpp
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
#include "gravitytexture.hpp"

GravityTexture::GravityTexture(GravityInstanceExtIf *inst_ext_if, GravityDeviceExtIf *dev_ext_if,
                               GravityDeviceMemoryManager *dev_memory_mgr) {
    m_read = false;
    m_filename = "";
    m_width = 0;
    m_height = 0;
    m_cpu_data = nullptr;
    m_inst_ext_if = inst_ext_if;
    m_dev_ext_if = dev_ext_if;
    m_dev_memory_mgr = dev_memory_mgr;
    memset(&m_memory, 0, sizeof(GravityDeviceMemory));
    m_memory.vk_device_memory = VK_NULL_HANDLE;
}

GravityTexture::~GravityTexture() {
    Cleanup();
}

void GravityTexture::Cleanup() {
//    vkDestroyImageView(m_dev_ext_if->m_device, demo->textures[i].view, NULL);
//    vkDestroySampler(m_dev_ext_if->m_device, demo->textures[i].sampler, NULL);
    if (VK_NULL_HANDLE != m_memory.vk_device_memory) {
        Unload();
    }
    vkDestroyImage(m_dev_ext_if->m_device, m_vk_image, NULL);

    if (nullptr != m_cpu_data) {
        delete[] m_cpu_data;
        m_cpu_data = nullptr;
    }
    m_read = false;
    m_filename = "";
    m_width = 0;
    m_height = 0;
}

bool GravityTexture::Read(std::string const &filename) {
    GravityLogger &logger = GravityLogger::getInstance();

    m_filename = filename;
    m_read = false;

    if (filename.substr(filename.find_last_of(".") + 1) == "ppm") {
        std::string real_texture = "resources/textures/";
        real_texture += filename;
        m_read = ReadPPM(real_texture);
    }

    if (m_read) {
        VkImageCreateInfo image_create_info = {};

        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = nullptr;
        image_create_info.flags = 0;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = m_vk_format;
        image_create_info.extent.width = m_width;
        image_create_info.extent.height = m_height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_LINEAR; // Brainpain - VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

        VkResult vk_result = vkCreateImage(m_dev_ext_if->m_device, &image_create_info, nullptr, &m_vk_image);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg =
                "GravityTexture::Read failed call to vkCreateImage with error ";
            error_msg += vk_result;
            logger.LogError(error_msg);
            goto out;
        }

        // Get the memory requirements for this image
        vkGetImageMemoryRequirements(m_dev_ext_if->m_device, m_vk_image, &m_memory.vk_mem_reqs);
    }

out:

    if (!m_read) {
        Cleanup();
    }

    return m_read;
}

bool GravityTexture::ReadPPM(std::string const &filename) {
    bool read = false;
    std::ifstream *infile = nullptr;
    std::string line;
    std::istringstream iss;
    GravityLogger &logger = GravityLogger::getInstance();

    infile = new std::ifstream(filename.c_str(), std::ifstream::in);
    if (nullptr == infile || infile->fail()) {
        std::string error_msg = "GravityTexture::ReadPPM - Failed to read PPM ";
        error_msg += filename.c_str();
        logger.LogError(error_msg);
        goto out;
    }

    std::getline(*infile, line);
    if (line != "P6") {
        goto out;
    }

    iss.clear();
    std::getline(*infile, line);
    iss.str(line);
    iss >> m_width >> m_height;
    m_num_comps = 4;
    m_comp_bytes = 1;

    iss.clear();
    uint32_t max_value;
    std::getline(*infile, line);
    iss.str(line);
    iss >> max_value;

    m_cpu_data = new uint8_t[m_width * m_height * m_num_comps * m_comp_bytes];
    infile->read(reinterpret_cast<char*>(m_cpu_data), m_width * m_height * m_num_comps * m_comp_bytes);

    m_vk_format = VK_FORMAT_R8G8B8A8_UNORM;

   read = true;

out:
    infile->close();
    delete infile;

    return read;
}

bool GravityTexture::Load() {
    GravityLogger &logger = GravityLogger::getInstance();
    VkResult vk_result;
    if (!m_dev_memory_mgr->AllocateMemory(m_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        logger.LogError("GravityTexture::Load failed GravityDeviceMemoryManager->AllocateMemory call");
        return false;
    }

    VkSubresourceLayout layout;
    void *data = nullptr;
    VkImageSubresource image_sub_resource = {};
    image_sub_resource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_sub_resource.mipLevel = 0;
    image_sub_resource.arrayLayer = 0;

    vkGetImageSubresourceLayout(m_dev_ext_if->m_device, m_vk_image, &image_sub_resource, &layout);

    vk_result = vkMapMemory(m_dev_ext_if->m_device, m_memory.vk_device_memory, 0, m_memory.vk_mem_reqs.size, 0, &data);
    if (VK_SUCCESS != vk_result) {
        logger.LogError("GravityTexture::Load failed vkMapMemory");
        return false;
    }

    uint8_t *gpu_memory = reinterpret_cast<uint8_t*>(data);
    for (uint32_t row = 0; row < m_height; row++) {
        uint8_t *pRow = m_cpu_data;
        for (uint32_t col = 0; col < m_width; col++) {
            memcpy(gpu_memory, pRow, m_num_comps * m_comp_bytes);
            uint8_t comp = m_num_comps;
            while (comp < 4) {
                gpu_memory[comp++] = 0;
            }
            pRow += 4;
            gpu_memory += 4;
        }

        // Make sure we keep the proper pitch since many GPUs require additional padding after
        // rows of texture data.
        gpu_memory += layout.rowPitch;
    }

    vkUnmapMemory(m_dev_ext_if->m_device, m_memory.vk_device_memory);

    // Bind the texture memory at this point
    vk_result = vkBindImageMemory(m_dev_ext_if->m_device, m_vk_image, m_memory.vk_device_memory, 0);
    if (VK_SUCCESS != vk_result) {
        logger.LogError("GravityTexture::Load failed vkBindImageMemory");
        return false;
    }

//    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    return true;
}

bool GravityTexture::Unload() {
    if (VK_NULL_HANDLE != m_memory.vk_device_memory) {
        m_dev_memory_mgr->FreeMemory(m_memory);
        m_memory.vk_device_memory = VK_NULL_HANDLE;
    }

    return true;
}
