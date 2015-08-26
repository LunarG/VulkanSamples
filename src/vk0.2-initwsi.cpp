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
Inititalize WSI
*/

#include <util_init.hpp>
#include <assert.h>
#include <cstdlib>

int main(int argc, char **argv)
{
    VkResult res;
    struct sample_info info = {};
    char sample_title[] = "WSI Initialization Sample";

    /*
     * Set up WSI:
     * - Get supported uses for all queues
     * - Try to find a queue that supports both graphics and WSI present
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

    /* VULKAN_KEY_START */
    GET_INSTANCE_PROC_ADDR(info.inst, GetPhysicalDeviceSurfaceSupportWSI);
    GET_DEVICE_PROC_ADDR(info.device, GetSurfaceInfoWSI);
    GET_DEVICE_PROC_ADDR(info.device, CreateSwapChainWSI);
    GET_DEVICE_PROC_ADDR(info.device, CreateSwapChainWSI);
    GET_DEVICE_PROC_ADDR(info.device, DestroySwapChainWSI);
    GET_DEVICE_PROC_ADDR(info.device, GetSwapChainInfoWSI);
    GET_DEVICE_PROC_ADDR(info.device, AcquireNextImageWSI);
    GET_DEVICE_PROC_ADDR(info.device, QueuePresentWSI);

    res = vkGetPhysicalDeviceQueueCount(info.gpu, &info.queue_count);
    assert(!res);
    assert(info.queue_count >= 1);

    info.queue_props.reserve(info.queue_count);
    res = vkGetPhysicalDeviceQueueProperties(info.gpu, info.queue_count, info.queue_props.data());
    assert(!res);
    assert(info.queue_count >= 1);

    // Construct the WSI surface description:
    info.surface_description.sType = VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_WSI;
    info.surface_description.pNext = NULL;
#ifdef _WIN32
    info.surface_description.platform = VK_PLATFORM_WIN32_WSI;
    info.surface_description.pPlatformHandle = info.connection;
    info.surface_description.pPlatformWindow = info.window;
#else  // _WIN32
    info.platform_handle_xcb.connection = info.connection;
    info.platform_handle_xcb.root = info.screen->root;
    info.surface_description.platform = VK_PLATFORM_XCB_WSI;
    info.surface_description.pPlatformHandle = &info.platform_handle_xcb;
    info.surface_description.pPlatformWindow = &info.window;
#endif // _WIN32

    // Iterate over each queue to learn whether it supports presenting to WSI:
    VkBool32* supportsPresent = (VkBool32 *)malloc(info.queue_count * sizeof(VkBool32));
    for (uint32_t i = 0; i < info.queue_count; i++) {
        info.fpGetPhysicalDeviceSurfaceSupportWSI(info.gpu, i,
                                                   (VkSurfaceDescriptionWSI *) &info.surface_description,
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

    res = vkGetDeviceQueue(info.device, info.graphics_queue_family_index,
            0, &info.queue);
    assert(!res);

    // Get the list of VkFormats that are supported:
    size_t formatsSize;
    res = info.fpGetSurfaceInfoWSI(info.device,
                                    (VkSurfaceDescriptionWSI *) &info.surface_description,
                                    VK_SURFACE_INFO_TYPE_FORMATS_WSI,
                                    &formatsSize, NULL);
    assert(!res);
    VkSurfaceFormatPropertiesWSI *surfFormats = (VkSurfaceFormatPropertiesWSI *)malloc(formatsSize);
    res = info.fpGetSurfaceInfoWSI(info.device,
                                    (VkSurfaceDescriptionWSI *) &info.surface_description,
                                    VK_SURFACE_INFO_TYPE_FORMATS_WSI,
                                    &formatsSize, surfFormats);
    assert(!res);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    size_t formatCount = formatsSize / sizeof(VkSurfaceFormatPropertiesWSI);
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        info.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        assert(formatCount >= 1);
        info.format = surfFormats[0].format;
    }

    size_t capsSize;
    size_t presentModesSize;
    res = info.fpGetSurfaceInfoWSI(info.device,
        (const VkSurfaceDescriptionWSI *)&info.surface_description,
        VK_SURFACE_INFO_TYPE_PROPERTIES_WSI, &capsSize, NULL);
    assert(!res);
    res = info.fpGetSurfaceInfoWSI(info.device,
        (const VkSurfaceDescriptionWSI *)&info.surface_description,
        VK_SURFACE_INFO_TYPE_PRESENT_MODES_WSI, &presentModesSize, NULL);
    assert(!res);

    VkSurfacePropertiesWSI *surfProperties =
        (VkSurfacePropertiesWSI *)malloc(capsSize);
    VkSurfacePresentModePropertiesWSI *presentModes =
        (VkSurfacePresentModePropertiesWSI *)malloc(presentModesSize);

    res = info.fpGetSurfaceInfoWSI(info.device,
        (const VkSurfaceDescriptionWSI *)&info.surface_description,
        VK_SURFACE_INFO_TYPE_PROPERTIES_WSI, &capsSize, surfProperties);
    assert(!res);
    res = info.fpGetSurfaceInfoWSI(info.device,
        (const VkSurfaceDescriptionWSI *)&info.surface_description,
        VK_SURFACE_INFO_TYPE_PRESENT_MODES_WSI, &presentModesSize, presentModes);
    assert(!res);

    VkExtent2D swapChainExtent;
    // width and height are either both -1, or both not -1.
    if (surfProperties->currentExtent.width == -1)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapChainExtent.width = info.width;
        swapChainExtent.height = info.height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapChainExtent = surfProperties->currentExtent;
    }

    // If mailbox mode is available, use it, as is the lowest-latency non-
    // tearing mode.  If not, try IMMEDIATE which will usually be available,
    // and is fastest (though it tears).  If not, fall back to FIFO which is
    // always available.
    VkPresentModeWSI swapChainPresentMode = VK_PRESENT_MODE_FIFO_WSI;
    size_t presentModeCount = presentModesSize / sizeof(VkSurfacePresentModePropertiesWSI);
    for (size_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i].presentMode == VK_PRESENT_MODE_MAILBOX_WSI) {
            swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_WSI;
            break;
        }
        if ((swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_WSI) &&
            (presentModes[i].presentMode == VK_PRESENT_MODE_IMMEDIATE_WSI)) {
            swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_WSI;
        }
    }

#define WORK_AROUND_CODE
#ifdef WORK_AROUND_CODE
    uint32_t desiredNumberOfSwapChainImages = SAMPLE_BUFFER_COUNT;
#else  // WORK_AROUND_CODE
    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapChainImages = surfProperties->minImageCount + 1;
    if ((surfProperties->maxImageCount > 0) &&
        (desiredNumberOfSwapChainImages > surfProperties->maxImageCount))
    {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapChainImages = surfProperties->maxImageCount;
    }
#endif // WORK_AROUND_CODE

    VkSurfaceTransformWSI preTransform;
    if (surfProperties->supportedTransforms & VK_SURFACE_TRANSFORM_NONE_BIT_WSI) {
        preTransform = VK_SURFACE_TRANSFORM_NONE_WSI;
    } else {
        preTransform = surfProperties->currentTransform;
    }

    VkSwapChainCreateInfoWSI swap_chain = {};
    swap_chain.sType = VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI;
    swap_chain.pNext = NULL;
    swap_chain.pSurfaceDescription = (const VkSurfaceDescriptionWSI *)&info.surface_description;
    swap_chain.minImageCount = desiredNumberOfSwapChainImages;
    swap_chain.imageFormat = info.format;
    swap_chain.imageExtent.width = swapChainExtent.width;
    swap_chain.imageExtent.height = swapChainExtent.height;
    swap_chain.preTransform = preTransform;
    swap_chain.imageArraySize = 1;
    swap_chain.presentMode = swapChainPresentMode;
    swap_chain.oldSwapChain.handle = 0;
    swap_chain.clipped = true;

    res = info.fpCreateSwapChainWSI(info.device, &swap_chain, &info.swap_chain);
    assert(!res);

    size_t swapChainImagesSize;
    res = info.fpGetSwapChainInfoWSI(info.device, info.swap_chain,
                                      VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI,
                                      &swapChainImagesSize, NULL);
    assert(!res);

    VkSwapChainImagePropertiesWSI* swapChainImages = (VkSwapChainImagePropertiesWSI*)malloc(swapChainImagesSize);
    assert(swapChainImages);
    res = info.fpGetSwapChainInfoWSI(info.device, info.swap_chain,
                                      VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI,
                                      &swapChainImagesSize, swapChainImages);
    assert(!res);

#ifdef WORK_AROUND_CODE
    info.swapChainImageCount = SAMPLE_BUFFER_COUNT;
#else  // WORK_AROUND_CODE
    // The number of images within the swap chain is determined based on the size of the info returned
    info.swapChainImageCount = swapChainImagesSize / sizeof(VkSwapChainImagePropertiesWSI);
#endif // WORK_AROUND_CODE

    info.buffers.reserve(info.swapChainImageCount);

    for (int i = 0; i < info.swapChainImageCount; i++) {
        VkAttachmentViewCreateInfo color_attachment_view = {};
        color_attachment_view.sType = VK_STRUCTURE_TYPE_ATTACHMENT_VIEW_CREATE_INFO;
        color_attachment_view.pNext = NULL;
        color_attachment_view.format = info.format;
        color_attachment_view.mipLevel = 0;
        color_attachment_view.baseArraySlice = 0;
        color_attachment_view.arraySize = 1;


        info.buffers[i].image = swapChainImages[i].image;

        set_image_layout(info, info.buffers[i].image,
                               VK_IMAGE_ASPECT_COLOR,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        color_attachment_view.image = info.buffers[i].image;

        res = vkCreateAttachmentView(info.device,
                &color_attachment_view, &info.buffers[i].view);
        assert(!res);
    }
    /* VULKAN_KEY_END */

    /* Clean Up */
    vkDestroyCommandBuffer(info.device, info.cmd);
    vkDestroyCommandPool(info.device, info.cmd_pool);
    for (int i = 0; i < info.swapChainImageCount; i++) {
        vkDestroyAttachmentView(info.device, info.buffers[i].view);
    }
    vkDestroyDevice(info.device);
    return 0;
}
