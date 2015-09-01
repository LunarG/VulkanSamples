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
Initialize Framebuffer
*/

#include <util_init.hpp>
#include <assert.h>
#include <cstdlib>

int main(int argc, char **argv)
{
    VkResult res;
    struct sample_info info = {};
    char sample_title[] = "Init Framebuffer Sample";

    init_global_layer_properties(info);
    info.instance_extension_names.push_back(VK_WSI_SWAPCHAIN_EXTENSION_NAME);
    info.device_extension_names.push_back(VK_WSI_DEVICE_SWAPCHAIN_EXTENSION_NAME);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    init_connection(info);
    info.width = info.height = 50;
    init_window(info);
    init_wsi(info);
    init_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_renderpass(info);

    /* VULKAN_KEY_START */
    VkAttachmentBindInfo attachments[2];
    attachments[0].view.handle = VK_NULL_HANDLE;
    attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[1].view = info.depth.view;
    attachments[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = info.render_pass;
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = attachments;
    fb_info.width  = info.width;
    fb_info.height = info.height;
    fb_info.layers = 1;

    uint32_t i;
    for (i = 0; i < SAMPLE_BUFFER_COUNT; i++) {
        attachments[0].view = info.buffers[i].view;
        res = vkCreateFramebuffer(info.device, &fb_info, &info.framebuffers[i]);
        assert(!res);
    }
    /* VULKAN_KEY_END */

    vkFreeMemory(info.device, info.depth.mem);
    vkDestroyAttachmentView(info.device, info.depth.view);
    vkDestroyImage(info.device, info.depth.image);
    info.fpDestroySwapChainWSI(info.device, info.swap_chain);
    for (int i = 0; i < SAMPLE_BUFFER_COUNT; i++) {
        vkDestroyFramebuffer(info.device, info.framebuffers[i]);
    }

    for (int i = 0; i < info.swapChainImageCount; i++) {
        vkDestroyAttachmentView(info.device, info.buffers[i].view);
    }
    vkDestroyCommandBuffer(info.device, info.cmd);
    vkDestroyCommandPool(info.device, info.cmd_pool);
    vkDestroyRenderPass(info.device, info.render_pass);
    vkDestroyDevice(info.device);
}
