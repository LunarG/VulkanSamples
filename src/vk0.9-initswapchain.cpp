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
Inititalize Swapchain
*/

#include <util_init.hpp>
#include <assert.h>
#include <cstdlib>

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Swapchain Initialization Sample";

    /*
     * Set up swapchain:
     * - Get supported uses for all queues
     * - Try to find a queue that supports both graphics and present
     * - If no queue supports both, find a present queue and make sure we have a graphics queue
     * - Get a list of supported formats and use the first one
     * - Get surface properties and present modes and use them to create a swap chain
     * - Create swap chain buffers
     * - For each buffer, create a color attachment view and set its layout to color attachment
     */

    init_global_layer_properties(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    init_connection(info);
    info.width = info.height = 50;
    init_window(info);    


    /* VULKAN_KEY_START */
    GET_INSTANCE_PROC_ADDR(info.inst, GetPhysicalDeviceSurfaceSupportKHR);
    GET_DEVICE_PROC_ADDR(info.device, GetSurfacePropertiesKHR);
    GET_DEVICE_PROC_ADDR(info.device, GetSurfaceFormatsKHR);
    GET_DEVICE_PROC_ADDR(info.device, GetSurfacePresentModesKHR);
    GET_DEVICE_PROC_ADDR(info.device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(info.device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(info.device, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(info.device, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(info.device, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(info.device, QueuePresentKHR);

    vkGetPhysicalDeviceQueueFamilyProperties(info.gpus[0], &info.queue_count, NULL);
    assert(info.queue_count >= 1);

    info.queue_props.resize(info.queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(info.gpus[0], &info.queue_count, info.queue_props.data());
    assert(info.queue_count >= 1);

    // Construct the surface description:
    info.surface_description.sType = VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_KHR;
    info.surface_description.pNext = NULL;
#ifdef _WIN32
    info.surface_description.platform = VK_PLATFORM_WIN32_KHR;
    info.surface_description.pPlatformHandle = info.connection;
    info.surface_description.pPlatformWindow = info.window;
#else  // _WIN32
    info.platform_handle_xcb.connection = info.connection;
    info.platform_handle_xcb.root = info.screen->root;
    info.surface_description.platform = VK_PLATFORM_XCB_KHR;
    info.surface_description.pPlatformHandle = &info.platform_handle_xcb;
    info.surface_description.pPlatformWindow = &info.window;
#endif // _WIN32

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32* supportsPresent = (VkBool32 *)malloc(info.queue_count * sizeof(VkBool32));
    for (uint32_t i = 0; i < info.queue_count; i++) {
        info.fpGetPhysicalDeviceSurfaceSupportKHR(info.gpus[0], i,
                                                   (VkSurfaceDescriptionKHR *) &info.surface_description,
                                                   &supportsPresent[i]);
    }

    // Search for a graphics queue and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex  = UINT32_MAX;
    for (uint32_t i = 0; i < info.queue_count; i++) {
        if ((info.queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphicsQueueNodeIndex == UINT32_MAX) {
                graphicsQueueNodeIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    if (presentQueueNodeIndex == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < info.queue_count; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    free(supportsPresent);

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) {
        std::cout << "Could not find a graphics and a present queue\nCould not find a graphics and a present queue\n";
        exit(-1);
    }

    info.graphics_queue_family_index = graphicsQueueNodeIndex;

    // Get the list of VkFormats that are supported:
    uint32_t formatCount;
    res = info.fpGetSurfaceFormatsKHR(info.device,
                                    (VkSurfaceDescriptionKHR *) &info.surface_description,
                                     &formatCount, NULL);
    assert(res == VK_SUCCESS);
    VkSurfaceFormatKHR *surfFormats = (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    res = info.fpGetSurfaceFormatsKHR(info.device,
                                    (VkSurfaceDescriptionKHR *) &info.surface_description,
                                     &formatCount, surfFormats);
    assert(res == VK_SUCCESS);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        info.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        assert(formatCount >= 1);
        info.format = surfFormats[0].format;
    }

    VkSurfacePropertiesKHR surfProperties;

    res = info.fpGetSurfacePropertiesKHR(info.device,
        (const VkSurfaceDescriptionKHR *)&info.surface_description,
        &surfProperties);
    assert(res == VK_SUCCESS);

    uint32_t presentModeCount;
    res = info.fpGetSurfacePresentModesKHR(info.device,
        (const VkSurfaceDescriptionKHR *)&info.surface_description,
        &presentModeCount, NULL);
    assert(res == VK_SUCCESS);
    VkPresentModeKHR *presentModes =
        (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));

    res = info.fpGetSurfacePresentModesKHR(info.device,
        (const VkSurfaceDescriptionKHR *)&info.surface_description,
        &presentModeCount, presentModes);
    assert(res == VK_SUCCESS);

    VkExtent2D swapChainExtent;
    // width and height are either both -1, or both not -1.
    if (surfProperties.currentExtent.width == -1)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapChainExtent.width = info.width;
        swapChainExtent.height = info.height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapChainExtent = surfProperties.currentExtent;
    }

    // If mailbox mode is available, use it, as is the lowest-latency non-
    // tearing mode.  If not, try IMMEDIATE which will usually be available,
    // and is fastest (though it tears).  If not, fall back to FIFO which is
    // always available.
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (size_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
            (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapChainImages = surfProperties.minImageCount + 1;
    if ((surfProperties.maxImageCount > 0) &&
        (desiredNumberOfSwapChainImages > surfProperties.maxImageCount))
    {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapChainImages = surfProperties.maxImageCount;
    }

    VkSurfaceTransformKHR preTransform;
    if (surfProperties.supportedTransforms & VK_SURFACE_TRANSFORM_NONE_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_NONE_KHR;
    } else {
        preTransform = surfProperties.currentTransform;
    }

    VkSwapchainCreateInfoKHR swap_chain = {};
    swap_chain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_chain.pNext = NULL;
    swap_chain.pSurfaceDescription = (const VkSurfaceDescriptionKHR *)&info.surface_description;
    swap_chain.minImageCount = desiredNumberOfSwapChainImages;
    swap_chain.imageFormat = info.format;
    swap_chain.imageExtent.width = swapChainExtent.width;
    swap_chain.imageExtent.height = swapChainExtent.height;
    swap_chain.preTransform = preTransform;
    swap_chain.imageArraySize = 1;
    swap_chain.presentMode = swapchainPresentMode;
    swap_chain.oldSwapchain = NULL;
    swap_chain.clipped = true;
    swap_chain.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swap_chain.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_chain.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_chain.queueFamilyIndexCount = 0;
    swap_chain.pQueueFamilyIndices = NULL;

    res = info.fpCreateSwapchainKHR(info.device, &swap_chain, &info.swap_chain);
    assert(res == VK_SUCCESS);

    res = info.fpGetSwapchainImagesKHR(info.device, info.swap_chain,
                                      &info.swapchainImageCount, NULL);
    assert(res == VK_SUCCESS);

    VkImage* swapchainImages = (VkImage*)malloc(info.swapchainImageCount * sizeof(VkImage));
    assert(swapchainImages);
    res = info.fpGetSwapchainImagesKHR(info.device, info.swap_chain,
                                      &info.swapchainImageCount, swapchainImages);
    assert(res == VK_SUCCESS);

    info.buffers.resize(info.swapchainImageCount);

    /* Going to need a command buffer to send the memory barriers in set_image_layout       */
    /* but we couldn't have created one before we knew what our graphics_queue_family_index */
    /* is, but now that we have it, create the command buffer                               */
    init_command_pool(info);
    init_command_buffer(info);
    execute_begin_command_buffer(info);
    vkGetDeviceQueue(info.device, info.graphics_queue_family_index,
            0, &info.queue);

    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        VkImageViewCreateInfo color_image_view = {};
        color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view.pNext = NULL;
        color_image_view.format = info.format;
        color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
        color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
        color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
        color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
        color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view.subresourceRange.baseMipLevel = 0;
        color_image_view.subresourceRange.levelCount = 1;
        color_image_view.subresourceRange.baseArrayLayer = 0;
        color_image_view.subresourceRange.enabledLayerNameCount = 1;
        color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view.flags = 0;


        info.buffers[i].image = swapchainImages[i];

        set_image_layout(info, info.buffers[i].image,
                               VK_IMAGE_ASPECT_COLOR_BIT,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        color_image_view.image = info.buffers[i].image;

        res = vkCreateImageView(info.device,
                &color_image_view, NULL, &info.buffers[i].view);
        assert(res == VK_SUCCESS);
    }
    execute_end_command_buffer(info);
    execute_queue_command_buffer(info);
    /* VULKAN_KEY_END */

    /* Clean Up */
    VkCommandBuffer cmd_bufs[1] = { info.cmd };
    vkFreeCommandBuffers(info.device, info.cmd_pool, 1, cmd_bufs);
    vkDestroyCommandPool(info.device, info.cmd_pool, NULL);
    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        vkDestroyImageView(info.device, info.buffers[i].view, NULL);
    }
    destroy_device(info);
    destroy_instance(info);
    destroy_window(info);

    return 0;
}
