/*
 * Vulkan Samples
 *
 * Copyright (C) 2015-2016 Valve Corporation
 * Copyright (C) 2015-2016 LunarG, Inc.
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
 */

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
Inititalize Texture
*/

/* This is part of the draw cube progression */

#include <sys/stat.h>
#include <util_init.hpp>
#include <assert.h>
#include <cstdlib>

int sample_main(int argc, char *argv[]) {
    VkResult U_ASSERT_ONLY res;
    bool U_ASSERT_ONLY pass;
    struct sample_info info = {};
    char sample_title[] = "Texture Initialization Sample";

    init_global_layer_properties(info);
    init_instance_extension_names(info);
    init_device_extension_names(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_connection(info);
    init_window_size(info, 50, 50);
    init_window(info);
    init_swapchain_extension(info);
    init_device(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);

    /* VULKAN_KEY_START */
    /*
     * Set up textures:
     * - Create a linear tiled image
     * - Map it and write the texture data into it
     * - If linear images cannot be used as textures, create an optimally
     *       tiled image and blit from the linearly tiled image to the optimally
     *       tiled image
     * -
     * -
     * -
     */

    struct texture_object texObj;
    std::string filename = get_base_data_dir();
    filename.append("lunarg.ppm");

#ifndef __ANDROID__
    struct stat statstruct;
    if (stat(filename.c_str(), &statstruct)) {
        filename = "../../API-Samples/data/lunarg.ppm";
    }
#endif

    if (!read_ppm(filename.c_str(), texObj.tex_width, texObj.tex_height, 0, NULL)) {
        std::cout << "Could not read texture file lunarg.ppm\n";
        exit(-1);
    }

    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(info.gpus[0], VK_FORMAT_R8G8B8A8_UNORM, &formatProps);

    /* See if we can use a linear tiled image for a texture, if not, we will
     * need a staging buffer for the texture data */
    texObj.needs_staging = (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT));

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_create_info.extent.width = texObj.tex_width;
    image_create_info.extent.height = texObj.tex_height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = NUM_SAMPLES;
    image_create_info.tiling = texObj.needs_staging ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;
    image_create_info.initialLayout = texObj.needs_staging ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PREINITIALIZED;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    if (texObj.needs_staging) {
        image_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = NULL;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.flags = 0;

    VkMemoryAllocateInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;

    VkMemoryRequirements mem_reqs;

    res = vkCreateImage(info.device, &image_create_info, NULL, &texObj.image);
    assert(res == VK_SUCCESS);

    vkGetImageMemoryRequirements(info.device, texObj.image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;

    VkFlags requirements = texObj.needs_staging ? 0 : (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    pass = memory_type_from_properties(info, mem_reqs.memoryTypeBits, requirements, &mem_alloc.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory");

    /* allocate memory */
    res = vkAllocateMemory(info.device, &mem_alloc, NULL, &(texObj.image_memory));
    assert(res == VK_SUCCESS);

    /* bind memory */
    res = vkBindImageMemory(info.device, texObj.image, texObj.image_memory, 0);
    assert(res == VK_SUCCESS);

    VkImageSubresource subres = {};
    subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subres.mipLevel = 0;
    subres.arrayLayer = 0;

    VkSubresourceLayout layout = {};
    void *data;

    if (texObj.needs_staging) {
        /* Need a staging buffer to map and copy texture into */

        VkBufferCreateInfo buffer_create_info = {};
        buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_create_info.pNext = NULL;
        buffer_create_info.flags = 0;
        buffer_create_info.size = texObj.tex_width * texObj.tex_height * 4;
        buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        buffer_create_info.queueFamilyIndexCount = 0;
        buffer_create_info.pQueueFamilyIndices = NULL;
        res = vkCreateBuffer(info.device, &buffer_create_info, NULL, &texObj.buffer);
        assert(res == VK_SUCCESS);

        VkMemoryAllocateInfo buf_mem_alloc = {};
        buf_mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        buf_mem_alloc.pNext = NULL;
        buf_mem_alloc.allocationSize = 0;
        buf_mem_alloc.memoryTypeIndex = 0;

        vkGetBufferMemoryRequirements(info.device, texObj.buffer, &mem_reqs);
        buf_mem_alloc.allocationSize = mem_reqs.size;

        requirements = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        pass = memory_type_from_properties(info, mem_reqs.memoryTypeBits, requirements, &buf_mem_alloc.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");

        /* allocate memory */
        res = vkAllocateMemory(info.device, &buf_mem_alloc, NULL, &(texObj.buffer_memory));
        assert(res == VK_SUCCESS);

        /* bind memory */
        res = vkBindBufferMemory(info.device, texObj.buffer, texObj.buffer_memory, 0);
        assert(res == VK_SUCCESS);
    } else {
        texObj.buffer = VK_NULL_HANDLE;
        texObj.buffer_memory = VK_NULL_HANDLE;

        /* Get the subresource layout so we know what the row pitch is */
        vkGetImageSubresourceLayout(info.device, texObj.image, &subres, &layout);
    }

    VkDeviceMemory mapped_memory = texObj.needs_staging ? texObj.buffer_memory : texObj.image_memory;
    res = vkMapMemory(info.device, mapped_memory, 0, mem_reqs.size, 0, &data);
    assert(res == VK_SUCCESS);

    /* Read the ppm file into the mappable image's memory */
    if (!read_ppm(filename.c_str(), texObj.tex_width, texObj.tex_height,
                  texObj.needs_staging ? (texObj.tex_width * 4) : layout.rowPitch, (unsigned char *)data)) {
        std::cout << "Could not load texture file lunarg.ppm\n";
        exit(-1);
    }

    vkUnmapMemory(info.device, mapped_memory);

    if (!texObj.needs_staging) {
        /* If we can use the linear tiled image as a texture, just do it */
        texObj.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        set_image_layout(info, texObj.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, texObj.imageLayout,
                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    } else {
        /* Since we're going to blit to the texture image, set its layout to
         * DESTINATION_OPTIMAL */
        set_image_layout(info, texObj.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy copy_region;
        copy_region.bufferOffset = 0;
        copy_region.bufferRowLength = texObj.tex_width;
        copy_region.bufferImageHeight = texObj.tex_height;
        copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.imageSubresource.mipLevel = 0;
        copy_region.imageSubresource.baseArrayLayer = 0;
        copy_region.imageSubresource.layerCount = 1;
        copy_region.imageOffset.x = 0;
        copy_region.imageOffset.y = 0;
        copy_region.imageOffset.z = 0;
        copy_region.imageExtent.width = texObj.tex_width;
        copy_region.imageExtent.height = texObj.tex_height;
        copy_region.imageExtent.depth = 1;

        /* Put the copy command into the command buffer */
        vkCmdCopyBufferToImage(info.cmd, texObj.buffer, texObj.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

        /* Set the layout for the texture image from DESTINATION_OPTIMAL to
         * SHADER_READ_ONLY */
        texObj.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        set_image_layout(info, texObj.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texObj.imageLayout,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }
    execute_end_command_buffer(info);
    execute_queue_command_buffer(info);

    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.mipLodBias = 0.0;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.maxAnisotropy = 1;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerCreateInfo.minLod = 0.0;
    samplerCreateInfo.maxLod = 0.0;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    /* create sampler */
    res = vkCreateSampler(info.device, &samplerCreateInfo, NULL, &texObj.sampler);
    assert(res == VK_SUCCESS);

    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.pNext = NULL;
    view_info.image = VK_NULL_HANDLE;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    view_info.components.r = VK_COMPONENT_SWIZZLE_R;
    view_info.components.g = VK_COMPONENT_SWIZZLE_G;
    view_info.components.b = VK_COMPONENT_SWIZZLE_B;
    view_info.components.a = VK_COMPONENT_SWIZZLE_A;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    /* create image view */
    view_info.image = texObj.image;
    res = vkCreateImageView(info.device, &view_info, NULL, &texObj.view);
    assert(res == VK_SUCCESS);

    info.textures.push_back(texObj);
    /* VULKAN_KEY_END */

    /* Clean Up */
    vkDestroySampler(info.device, texObj.sampler, NULL);
    vkDestroyImageView(info.device, texObj.view, NULL);
    vkDestroyImage(info.device, texObj.image, NULL);
    vkFreeMemory(info.device, texObj.image_memory, NULL);
    /* Release the resources for the staging image */
    vkFreeMemory(info.device, texObj.buffer_memory, NULL);
    vkDestroyBuffer(info.device, texObj.buffer, NULL);

    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_device(info);
    destroy_window(info);
    destroy_instance(info);
    return 0;
}
