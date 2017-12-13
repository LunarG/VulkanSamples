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
    m_vk_image = VK_NULL_HANDLE;
    memset(&m_memory, 0, sizeof(GravityDeviceMemory));
    m_memory.vk_device_memory = VK_NULL_HANDLE;
    m_requires_staging_texture = false;
    m_staging_vk_image = VK_NULL_HANDLE;
    memset(&m_staging_memory, 0, sizeof(GravityDeviceMemory));
    m_staging_memory.vk_device_memory = VK_NULL_HANDLE;
    m_vk_sampler = VK_NULL_HANDLE;
    m_vk_image_view = VK_NULL_HANDLE;
}

GravityTexture::~GravityTexture() {
    Cleanup();
}

void GravityTexture::FreeStagingData() {
    if (!m_requires_staging_texture) {
        return;
    }
    if (VK_NULL_HANDLE != m_staging_vk_image) {
        vkDestroyImage(m_dev_ext_if->m_vk_device, m_staging_vk_image, NULL);
    }
}

void GravityTexture::Cleanup() {
    Unload();
    FreeStagingData();

//    vkDestroyImageView(m_dev_ext_if->m_vk_device, demo->textures[i].view, NULL);
//    vkDestroySampler(m_dev_ext_if->m_vk_device, demo->textures[i].sampler, NULL);
    if (VK_NULL_HANDLE != m_vk_image) {
        vkDestroyImage(m_dev_ext_if->m_vk_device, m_vk_image, NULL);
    }

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
    m_filename = filename;
    m_read = false;

    if (filename.substr(filename.find_last_of(".") + 1) == "ppm") {
        std::string real_texture = "resources/textures/";
        real_texture += filename;
        m_read = ReadPPM(real_texture);
        if (!m_read) {
            Cleanup();
        }
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

bool GravityTexture::CreateVkImage(VkImageTiling image_tiling, VkImageUsageFlags image_usage, VkImage &vk_image) {
    GravityLogger &logger = GravityLogger::getInstance();
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
    image_create_info.tiling = image_tiling;
    image_create_info.usage = image_usage;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    VkResult vk_result = vkCreateImage(m_dev_ext_if->m_vk_device, &image_create_info, nullptr, &vk_image);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityTexture::Read failed call to vkCreateImage with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }
    return true;
}

bool GravityTexture::Load(VkCommandBuffer &cmd_buf) {
    GravityLogger &logger = GravityLogger::getInstance();
    VkResult vk_result;
    VkFormatProperties format_properties = {};
    VkSubresourceLayout subresource_layout = {};
    VkImageSubresource image_subresource = {};
    VkImageMemoryBarrier image_memory_barrier;
    VkImageMemoryBarrier* ptr_image_memory_barrier = &image_memory_barrier;
    void *data = nullptr;

    vkGetPhysicalDeviceFormatProperties(m_dev_ext_if->m_vk_physical_device, m_vk_format, &format_properties);

    // We can copy linear textures directly, so don't use a staging texture
    if (0 != (format_properties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
        CreateVkImage(VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT, m_vk_image);

        // Get the memory requirements for this image
        vkGetImageMemoryRequirements(m_dev_ext_if->m_vk_device, m_vk_image, &m_memory.vk_mem_reqs);
        if (!m_dev_memory_mgr->AllocateMemory(m_memory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            logger.LogError("GravityTexture::Load failed GravityDeviceMemoryManager->AllocateMemory call");
            return false;
        }

        // Bind the texture memory at this point
        vk_result = vkBindImageMemory(m_dev_ext_if->m_vk_device, m_vk_image, m_memory.vk_device_memory, 0);
        if (VK_SUCCESS != vk_result) {
            logger.LogError("GravityTexture::Load failed vkBindImageMemory");
            return false;
        }

        // Get the image subresource and copy the texture contents to it.
        image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_subresource.mipLevel = 0;
        image_subresource.arrayLayer = 0;
        vkGetImageSubresourceLayout(m_dev_ext_if->m_vk_device, m_vk_image, &image_subresource, &subresource_layout);
        vk_result = vkMapMemory(m_dev_ext_if->m_vk_device, m_memory.vk_device_memory, 0, m_memory.vk_mem_reqs.size, 0, &data);
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
            gpu_memory += subresource_layout.rowPitch;
        }
        vkUnmapMemory(m_dev_ext_if->m_vk_device, m_memory.vk_device_memory);

        // We need to transition to shader read, and fragment shader needs to wait for it.
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        image_memory_barrier.dstAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_memory_barrier.image = m_vk_image;
        image_memory_barrier.srcQueueFamilyIndex = m_dev_ext_if->m_vk_graphics_queue.family_index;
        image_memory_barrier.dstQueueFamilyIndex = m_dev_ext_if->m_vk_graphics_queue.family_index;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, ptr_image_memory_barrier);
    // Do not pass go, use a staging texture.
    } else {
        // Setup the staging texture image
        CreateVkImage(VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, m_staging_vk_image);
        vkGetImageMemoryRequirements(m_dev_ext_if->m_vk_device, m_staging_vk_image, &m_staging_memory.vk_mem_reqs);
        if (!m_dev_memory_mgr->AllocateMemory(m_staging_memory, (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
            logger.LogError("GravityTexture::Load failed GravityDeviceMemoryManager->AllocateMemory call on staging texture");
            return false;
        }

        // Bind the texture memory at this point
        vk_result = vkBindImageMemory(m_dev_ext_if->m_vk_device, m_staging_vk_image, m_staging_memory.vk_device_memory, 0);
        if (VK_SUCCESS != vk_result) {
            logger.LogError("GravityTexture::Load failed vkBindImageMemory for staging texture");
            return false;
        }

        // Get the image subresource and copy the texture contents to the staging texture.
        image_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_subresource.mipLevel = 0;
        image_subresource.arrayLayer = 0;
        vkGetImageSubresourceLayout(m_dev_ext_if->m_vk_device, m_staging_vk_image, &image_subresource, &subresource_layout);
        vk_result = vkMapMemory(m_dev_ext_if->m_vk_device, m_staging_memory.vk_device_memory, 0, m_staging_memory.vk_mem_reqs.size, 0, &data);
        if (VK_SUCCESS != vk_result) {
            logger.LogError("GravityTexture::Load failed vkMapMemory for staging texture");
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
            gpu_memory += subresource_layout.rowPitch;
        }
        vkUnmapMemory(m_dev_ext_if->m_vk_device, m_staging_memory.vk_device_memory);

        // Setup the resulting texture image
        CreateVkImage(VK_IMAGE_TILING_OPTIMAL, (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT), m_vk_image);
        vkGetImageMemoryRequirements(m_dev_ext_if->m_vk_device, m_vk_image, &m_memory.vk_mem_reqs);
        if (!m_dev_memory_mgr->AllocateMemory(m_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            logger.LogError("GravityTexture::Load failed GravityDeviceMemoryManager->AllocateMemory call on texture");
            return false;
        }

        // We need to transition the staging so it can be used for a read, and transition the target
        // so it can be written to.
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        image_memory_barrier.image = m_staging_vk_image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, ptr_image_memory_barrier);
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = 0;
        image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.image = m_vk_image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, ptr_image_memory_barrier);

        // Now, copy the data from the staging texture into the final texture
        VkImageCopy copy_region = {};
        copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.srcSubresource.mipLevel = 0;
        copy_region.srcSubresource.baseArrayLayer = 0;
        copy_region.srcSubresource.layerCount = 1;
        copy_region.srcOffset.x = 0;
        copy_region.srcOffset.y = 0;
        copy_region.srcOffset.z = 0;
        copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.dstSubresource.mipLevel = 0;
        copy_region.dstSubresource.baseArrayLayer = 0;
        copy_region.dstSubresource.layerCount = 1;
        copy_region.dstOffset.x = 0;
        copy_region.dstOffset.y = 0;
        copy_region.dstOffset.z = 0;
        copy_region.extent.width = m_width;
        copy_region.extent.height = m_height;
        copy_region.extent.depth = 1;
        vkCmdCopyImage(cmd_buf, m_staging_vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vk_image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

        // Now that we've copied into the destination image, we need to transition to shader
        // read, and fragment shader needs to wait for it.
        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = nullptr;
        image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        image_memory_barrier.dstAccessMask = (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT);
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_memory_barrier.image = m_vk_image;
        image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.levelCount = 1;
        image_memory_barrier.subresourceRange.baseArrayLayer = 0;
        image_memory_barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, ptr_image_memory_barrier);
    }

    // The texture should be ready by this point, so setup a simple sampler
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.magFilter = VK_FILTER_NEAREST;
    sampler_create_info.minFilter = VK_FILTER_NEAREST;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_FALSE;
    sampler_create_info.maxAnisotropy = 1;
    sampler_create_info.compareOp = VK_COMPARE_OP_NEVER;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    vk_result = vkCreateSampler(m_dev_ext_if->m_vk_device, &sampler_create_info, nullptr, &m_vk_sampler);
    if (VK_SUCCESS != vk_result) {
        logger.LogError("GravityTexture::Load failed to create sampler");
        return false;
    }

    // Create an imageview to the texture
    VkImageViewCreateInfo imageview_create_info = {};
    imageview_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageview_create_info.pNext = nullptr;
    imageview_create_info.image = VK_NULL_HANDLE;
    imageview_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageview_create_info.format = m_vk_format;
    imageview_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
    imageview_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
    imageview_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
    imageview_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
    imageview_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageview_create_info.subresourceRange.baseMipLevel = 0;
    imageview_create_info.subresourceRange.levelCount = 1;
    imageview_create_info.subresourceRange.baseArrayLayer = 0;
    imageview_create_info.subresourceRange.layerCount = 1;
    imageview_create_info.flags = 0;
    imageview_create_info.image = m_vk_image;
    vk_result = vkCreateImageView(m_dev_ext_if->m_vk_device, &imageview_create_info, nullptr, &m_vk_image_view);
    if (VK_SUCCESS != vk_result) {
        logger.LogError("GravityTexture::Load failed to create imageview");
        return false;
    }

    return true;
}

bool GravityTexture::Unload() {
    if (VK_NULL_HANDLE != m_vk_image_view) {
        vkDestroyImageView(m_dev_ext_if->m_vk_device, m_vk_image_view, nullptr);
        m_vk_image_view = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vk_sampler) {
        vkDestroySampler(m_dev_ext_if->m_vk_device, m_vk_sampler, nullptr);
        m_vk_sampler = VK_NULL_HANDLE;
    }
    if (VK_NULL_HANDLE != m_vk_image) {
        vkDestroyImage(m_dev_ext_if->m_vk_device, m_vk_image, nullptr);
        m_vk_image = VK_NULL_HANDLE;
    } 
    if (VK_NULL_HANDLE != m_memory.vk_device_memory) {
        m_dev_memory_mgr->FreeMemory(m_memory);
        m_memory.vk_device_memory = VK_NULL_HANDLE;
    }
    if (m_requires_staging_texture) {
        if (VK_NULL_HANDLE != m_staging_vk_image) {
            vkDestroyImage(m_dev_ext_if->m_vk_device, m_staging_vk_image, nullptr);
            m_staging_vk_image = VK_NULL_HANDLE;
        } 
        if (VK_NULL_HANDLE != m_staging_memory.vk_device_memory) {
            m_dev_memory_mgr->FreeMemory(m_staging_memory);
            m_staging_memory.vk_device_memory = VK_NULL_HANDLE;
        }
    }

    return true;
}

VkDescriptorImageInfo GravityTexture::GetVkDescriptorImageInfo() {
    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.sampler = m_vk_sampler;
    descriptor_image_info.imageView = m_vk_image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    return descriptor_image_info;
}
