/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
create Vulkan depth buffer
*/

#include <util_init.hpp>
#include <assert.h>
#include <cstdlib>

int main(int argc, char **argv)
{
    VkResult res;
    struct sample_info info = {};
    char sample_title[] = "Depth Buffer Sample";

    /*
     * Make a depth buffer:
     * - Create an Image to be the depth buffer
     * - Find memory requirements
     * - Allocate and bind memory
     * - Set the image layout
     * - Create an attachment view
     */

    init_global_layer_properties(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    res = vkGetPhysicalDeviceMemoryProperties(info.gpu, &info.memory_properties);
    assert(!res);

    /* HACK - Do this the right way once we have WSI code to call */
    info.graphics_queue_family_index = 0;

    VkCmdPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_CMD_POOL_CREATE_INFO;
    cmd_pool_info.pNext = NULL;
    cmd_pool_info.queueFamilyIndex = info.graphics_queue_family_index;
    cmd_pool_info.flags = 0;

    res = vkCreateCommandPool(info.device, &cmd_pool_info, &info.cmd_pool);
    assert(!res);
    res = vkGetDeviceQueue(info.device, info.graphics_queue_family_index,
            0, &info.queue);
    assert(!res);

    info.width = info.height = 50;


    /* VULKAN_KEY_START */
    VkImageCreateInfo image_info = {};
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    VkFormatProperties props;
    res = vkGetPhysicalDeviceFormatProperties(info.gpu, depth_format, &props);
    assert(!res);
    if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        image_info.tiling = VK_IMAGE_TILING_LINEAR;
    } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    } else {
        /* Try other depth formats? */
        std::cout << "VK_FORMAT_D16_UNORM Unsupported.\n";
        exit(-1);
    }

    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = depth_format;
    image_info.extent.width = info.width;
    image_info.extent.height = info.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arraySize = 1;
    image_info.samples = 1;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_BIT;
    image_info.flags = 0;

    VkMemoryAllocInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;

    VkAttachmentViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_ATTACHMENT_VIEW_CREATE_INFO;
    view_info.pNext = NULL;
    view_info.image.handle = VK_NULL_HANDLE;
    view_info.format = depth_format;
    view_info.mipLevel = 0;
    view_info.baseArraySlice = 0;
    view_info.arraySize = 1;
    view_info.flags = 0;

    VkMemoryRequirements mem_reqs;

    info.depth.format = depth_format;

    /* Create image */
    res = vkCreateImage(info.device, &image_info,
                        &info.depth.image);
    assert(!res);

    res = vkGetImageMemoryRequirements(info.device,
                                       info.depth.image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;
    /* Use the memory properties to determine the type of memory required */
    res = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_DEVICE_ONLY,
                                      &mem_alloc.memoryTypeIndex);
    assert(!res);

    /* Allocate memory */
    res = vkAllocMemory(info.device, &mem_alloc, &info.depth.mem);
    assert(!res);

    /* Bind memory */
    res = vkBindImageMemory(info.device, info.depth.image,
                            info.depth.mem, 0);
    assert(!res);

    /* Set the image layout to depth stencil optimal */
    set_image_layout(info, info.depth.image,
                          VK_IMAGE_ASPECT_DEPTH,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    /* Create image view */
    view_info.image = info.depth.image;
    res = vkCreateAttachmentView(info.device, &view_info, &info.depth.view);
    assert(!res);

    /* VULKAN_KEY_END */

    /* Clean Up */
    vkDestroyCommandBuffer(info.device, info.cmd);
    vkDestroyCommandPool(info.device, info.cmd_pool);
    vkFreeMemory(info.device, info.depth.mem);
    vkDestroyAttachmentView(info.device, info.depth.view);
    vkDestroyImage(info.device, info.depth.image);
    vkDestroyDevice(info.device);
    return 0;

}
