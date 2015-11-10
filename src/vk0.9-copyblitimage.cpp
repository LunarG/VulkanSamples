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
Copy/blit image
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Copy/Blit Image";
    VkImageCreateInfo image_info;
    VkImage bltSrcImage;
    VkImage bltDstImage;
    VkMemoryRequirements memReq;
    VkMemoryAllocInfo memAllocInfo;
    VkDeviceMemory dmem;
    unsigned int *pImgMem;

    init_global_layer_properties(info);
    info.instance_extension_names.push_back(VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME);
    info.device_extension_names.push_back(VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    info.width = info.height = 500;
    init_connection(info);
    init_window(info);
    init_swapchain_extension(info);
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);

    /* VULKAN_KEY_START */

    VkSemaphore presentCompleteSemaphore;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo;
    presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    presentCompleteSemaphoreCreateInfo.pNext = NULL;
    presentCompleteSemaphoreCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    res = vkCreateSemaphore(info.device,
                            &presentCompleteSemaphoreCreateInfo,
                            &presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    // Get the index of the next available swapchain image:
    res = info.fpAcquireNextImageKHR(info.device, info.swap_chain,
                                      UINT64_MAX,
                                      presentCompleteSemaphore,
                                      &info.current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(res == VK_SUCCESS);

    
    // Create an image, map it, and write some values to the image

    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = info.format;
    image_info.extent.width = info.width;
    image_info.extent.height = info.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arraySize = 1;
    image_info.samples = NUM_SAMPLES;
    image_info.queueFamilyCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT;
    image_info.flags = 0;
    image_info.tiling = VK_IMAGE_TILING_LINEAR;
    res = vkCreateImage(info.device, &image_info, &bltSrcImage);

    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    memAllocInfo.pNext = NULL;

    res = vkGetImageMemoryRequirements(info.device, bltSrcImage, &memReq);
    res = memory_type_from_properties(info,
                                      memReq.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &memAllocInfo.memoryTypeIndex);
    memAllocInfo.allocationSize = memReq.size;
    res = vkAllocMemory(info.device, &memAllocInfo, &dmem);
    res = vkBindImageMemory(info.device, bltSrcImage, dmem, 0);
    res = vkMapMemory(info.device, dmem, 0,0,0,(void **)&pImgMem);
    for (int k = 0; k < memReq.size/4; k++) *pImgMem++ = 0xff0000ff;

    // Flush the mapped memory and then unmap it
    VkMappedMemoryRange memRange;
    memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memRange.pNext = NULL;
    memRange.mem = dmem;
    memRange.offset = 0;
    memRange.size = memReq.size;
    res = vkFlushMappedMemoryRanges(info.device, 1, &memRange);
    vkUnmapMemory(info.device, dmem);

    bltDstImage = info.buffers[info.current_buffer].image;

    // Do a image copy to part of the dst image
    VkImageCopy cregion;
    cregion.srcSubresource.aspect = VK_IMAGE_ASPECT_COLOR;
    cregion.srcSubresource.mipLevel = 0;
    cregion.srcSubresource.arrayLayer = 0;
    cregion.srcSubresource.arraySize = 1;
    cregion.srcOffset.x = 0;
    cregion.srcOffset.y = 0;
    cregion.srcOffset.z = 0;
    cregion.destSubresource.aspect = VK_IMAGE_ASPECT_COLOR;
    cregion.destSubresource.mipLevel = 0;
    cregion.destSubresource.arrayLayer = 0;
    cregion.destSubresource.arraySize = 1;
    cregion.destOffset.x = 0;
    cregion.destOffset.y = 0;
    cregion.destOffset.z = 0;
    cregion.extent.width = info.width/2;
    cregion.extent.height = info.height/2;
    cregion.extent.depth = 1;

	vkCmdCopyImage(info.cmd, bltSrcImage, VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL, bltDstImage,
          VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL, 1, &cregion);

    // Do a blit to all of the dst image
	VkImageBlit region;
    region.srcSubresource.aspect = VK_IMAGE_ASPECT_COLOR;
    region.srcSubresource.mipLevel = 0;
    region.srcSubresource.arrayLayer = 0;
    region.srcSubresource.arraySize = 1;
    region.srcOffset.x = 0;
    region.srcOffset.y = 0;
    region.srcOffset.z = 0;
    region.srcExtent.width = info.width;
    region.srcExtent.height = info.height;
    region.srcExtent.depth = 1;
    region.destSubresource.aspect = VK_IMAGE_ASPECT_COLOR;
    region.destSubresource.mipLevel = 0;
    region.destSubresource.arrayLayer = 0;
    region.destSubresource.arraySize = 1;
    region.destOffset.x = 0;
    region.destOffset.y = 0;
    region.destOffset.z = 0;
    region.destExtent.width = info.width;
    region.destExtent.height = info.height;
    region.destExtent.depth = 1;

	vkCmdBlitImage(info.cmd, bltSrcImage, VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL, bltDstImage,
        VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL, 1, &region, VK_TEX_FILTER_LINEAR);

    VkImageMemoryBarrier prePresentBarrier = {};
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = NULL;
    prePresentBarrier.outputMask = VK_MEMORY_OUTPUT_COLOR_ATTACHMENT_BIT;
    prePresentBarrier.inputMask = 0;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SOURCE_KHR;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.destQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    prePresentBarrier.subresourceRange.baseMipLevel = 0;
    prePresentBarrier.subresourceRange.mipLevels = 1;
    prePresentBarrier.subresourceRange.baseArrayLayer = 0;
    prePresentBarrier.subresourceRange.arraySize = 1;
    prePresentBarrier.image = info.buffers[info.current_buffer].image;
    VkImageMemoryBarrier *pmemory_barrier = &prePresentBarrier;
    vkCmdPipelineBarrier(info.cmd, VK_PIPELINE_STAGE_ALL_GPU_COMMANDS, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_FALSE, 1, (const void * const*)&pmemory_barrier);

    res = vkEndCommandBuffer(info.cmd);
    const VkCmdBuffer cmd_bufs[] = { info.cmd };
    VkFence nullFence = { VK_NULL_HANDLE };

    /* Make sure buffer is ready for rendering */
    res = vkQueueWaitSemaphore(info.queue, presentCompleteSemaphore);
    assert(res == VK_SUCCESS);

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(info.queue, 1, cmd_bufs, nullFence);
    assert(res == VK_SUCCESS);

    /* Now present the image in the window */

    VkPresentInfoKHR present;
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.swapchains = &info.swap_chain;
    present.imageIndices = &info.current_buffer;

    res = info.fpQueuePresentKHR(info.queue, &present);
    assert(res == VK_SUCCESS);

    wait_seconds(1);
    /* VULKAN_KEY_END */

    vkDestroySemaphore(info.device, presentCompleteSemaphore);
    destroy_swap_chain(info);
    destroy_command_buffer(info);
    destroy_command_pool(info);
    destroy_window(info);
    destroy_device(info);
    destroy_instance(info);
}
