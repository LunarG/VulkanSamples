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
Draw Cube With Utilities
*/

#include <util_init.hpp>
#include "SPIRV/GlslangToSpv.h"
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

void init_and_begin_command_buffer(struct sample_info &info);

static const char *vertShaderText =
    "#version 140\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (binding = 0) uniform bufferVals {\n"
    "    mat4 mvp;\n"
    "} myBufferVals;\n"
    "layout (location = 0) in vec4 pos;\n"
    "layout (location = 1) in vec4 inColor;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "   outColor = inColor;\n"
    "   gl_Position = myBufferVals.mvp * pos;\n"
    "\n"
    "   // GL->VK conventions\n"
    "   gl_Position.y = -gl_Position.y;\n"
    "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
    "}\n";

static const char *fragShaderText=
    "#version 140\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "#extension GL_ARB_shading_language_420pack : enable\n"
    "layout (location = 0) in vec4 color;\n"
    "layout (location = 0) out vec4 outColor;\n"
    "void main() {\n"
    "   outColor = color;\n"
    "}\n";

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Draw Cube";

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
    init_and_begin_command_buffer(info);
    init_device_queue(info);
    init_swap_chain(info);
    init_depth_buffer(info);
    init_uniform_buffer(info);
    init_descriptor_and_pipeline_layouts(info, false);
    init_renderpass(info);
    init_shaders(info, vertShaderText, fragShaderText);
    init_framebuffers(info);
    init_vertex_buffer(info, g_vb_solid_face_colors_Data, sizeof(g_vb_solid_face_colors_Data),
                               sizeof(g_vb_solid_face_colors_Data[0]), false);
    init_descriptor_set(info, false);
    init_pipeline(info);

    /* VULKAN_KEY_START */

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth     = 1.0f;
    clear_values[1].depthStencil.stencil   = 0;

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

    VkRenderPassBeginInfo rp_begin;
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = info.render_pass;
    rp_begin.framebuffer = info.framebuffers[info.current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = info.width;
    rp_begin.renderArea.extent.height = info.height;
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    vkCmdBeginRenderPass(info.cmd, &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);

    vkCmdBindPipeline(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  info.pipeline);
    vkCmdBindDescriptorSets(info.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, info.pipeline_layout,
            0, NUM_DESCRIPTOR_SETS, info.desc_set.data(), 0, NULL);

    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(info.cmd, 0, 1, &info.vertex_buffer.buf, offsets);

    VkViewport viewport;
    viewport.height = (float) info.height;
    viewport.width = (float) info.width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.originX = 0;
    viewport.originY = 0;
    vkCmdSetViewport(info.cmd, 1, &viewport);

    VkRect2D scissor;
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(info.cmd, 1, &scissor);

    vkCmdDraw(info.cmd, 12 * 3, 1, 0, 0);
    vkCmdEndRenderPass(info.cmd);

    res = vkEndCommandBuffer(info.cmd);
    assert(res == VK_SUCCESS);

    const VkCmdBuffer cmd_bufs[] = { info.cmd };
    VkFence nullFence = { VK_NULL_HANDLE };

    /* Make sure buffer is ready for rendering */
    vkQueueWaitSemaphore(info.queue, presentCompleteSemaphore);

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
    // TODO: Deal with the VK_SUBOPTIMAL_WSI and VK_ERROR_OUT_OF_DATE_WSI
    // return codes
    assert(res == VK_SUCCESS);

    /* VULKAN_KEY_END */

    vkDestroySemaphore(info.device, presentCompleteSemaphore);
    vkDestroyPipeline(info.device, info.pipeline);
    vkDestroyPipelineCache(info.device, info.pipelineCache);
    vkDestroyBuffer(info.device, info.uniform_data.buf);
    vkFreeMemory(info.device, info.uniform_data.mem);
    for(int i = 0; i < NUM_DESCRIPTOR_SETS; i++)
        vkDestroyDescriptorSetLayout(info.device, info.desc_layout[i]);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout);
    vkDestroyDescriptorPool(info.device, info.desc_pool);
    vkDestroyShader(info.device,info.shaderStages[0].shader);
    vkDestroyShader(info.device,info.shaderStages[1].shader);
    vkDestroyShaderModule(info.device, info.vert_shader_module);
    vkDestroyShaderModule(info.device, info.frag_shader_module);
    vkDestroyCommandBuffer(info.device, info.cmd);
    vkDestroyCommandPool(info.device, info.cmd_pool);
    vkDestroyImageView(info.device, info.depth.view);
    vkDestroyImage(info.device, info.depth.image);
    vkFreeMemory(info.device, info.depth.mem);
    vkDestroyBuffer(info.device, info.vertex_buffer.buf);
    vkFreeMemory(info.device, info.vertex_buffer.mem);
    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        vkDestroyImageView(info.device, info.buffers[i].view);
    }
    info.fpDestroySwapchainKHR(info.device, info.swap_chain);
    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        vkDestroyFramebuffer(info.device, info.framebuffers[i]);
    }
    free(info.framebuffers);
    vkDestroyRenderPass(info.device, info.render_pass);
    vkDestroyDevice(info.device);
    vkDestroyInstance(info.inst);
    destroy_window(info);
}

VkResult init_global_layer_properties(struct sample_info &info)
{
    uint32_t instance_layer_count;
    VkLayerProperties *vk_props = NULL;
    VkResult res;

    /*
     * It's possible, though very rare, that the number of
     * instance layers could change. For example, installing something
     * could include new layers that the loader would pick up
     * between the initial query for the count and the
     * request for VkLayerProperties. The loader indicates that
     * by returning a VK_INCOMPLETE status and will update the
     * the count parameter.
     * The count parameter will be updated with the number of
     * entries loaded into the data pointer - in case the number
     * of layers went down or is smaller than the size given.
     */
    do {
        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        if (res)
            return res;

        if (instance_layer_count == 0) {
            return VK_SUCCESS;
        }

        vk_props = (VkLayerProperties *) realloc(vk_props, instance_layer_count * sizeof(VkLayerProperties));

        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props);
    } while (res == VK_INCOMPLETE);

    /*
     * Now gather the extension list for each instance layer.
     */
    for (uint32_t i = 0; i < instance_layer_count; i++) {
        layer_properties layer_props;
        layer_props.properties = vk_props[i];
        res = init_global_extension_properties(layer_props);
        if (res)
            return res;
        info.instance_layer_properties.push_back(layer_props);
    }
    free(vk_props);

    return res;
}

VkResult init_global_extension_properties(
        layer_properties &layer_props)
{
    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count;
    VkResult res;
    char *layer_name = NULL;

    layer_name = layer_props.properties.layerName;

    do {
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, NULL);
        if (res)
            return res;

        if (instance_extension_count == 0) {
            return VK_SUCCESS;
        }

        layer_props.extensions.resize(instance_extension_count);
        instance_extensions = layer_props.extensions.data();
        res = vkEnumerateInstanceExtensionProperties(
                  layer_name,
                  &instance_extension_count,
                  instance_extensions);
    } while (res == VK_INCOMPLETE);

    return res;
}

VkResult init_instance(struct sample_info &info, char const*const app_short_name)
{
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = app_short_name;
    app_info.appVersion = 1;
    app_info.pEngineName = app_short_name;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pAppInfo = &app_info;
    inst_info.pAllocCb = NULL;
    inst_info.layerCount = info.instance_layer_names.size();
    inst_info.ppEnabledLayerNames = info.instance_layer_names.size() ? info.instance_layer_names.data() : NULL;
    inst_info.extensionCount = info.instance_extension_names.size();
    inst_info.ppEnabledExtensionNames = info.instance_extension_names.data();

    VkResult res = vkCreateInstance(&inst_info, &info.inst);
    assert(res == VK_SUCCESS);

    return res;
}

VkResult init_enumerate_device(struct sample_info &info, uint32_t gpu_count)
{
    uint32_t const U_ASSERT_ONLY req_count = gpu_count;
    VkResult res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, NULL);
    assert(gpu_count);
    info.gpus.resize(gpu_count);
    res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, info.gpus.data());
    assert(!res && gpu_count >= req_count);

    return res;
}

VkResult init_device(struct sample_info &info)
{
    VkResult res;

    /* This is as good a place as any to do this */
    res = vkGetPhysicalDeviceMemoryProperties(info.gpus[0], &info.memory_properties);
    assert(res == VK_SUCCESS);

    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueRecordCount = 1;
    device_info.pRequestedQueues = &queue_info;
    device_info.layerCount = info.device_layer_names.size();
    device_info.ppEnabledLayerNames =
            device_info.layerCount ? info.device_layer_names.data() : NULL;
    device_info.extensionCount = info.device_extension_names.size();
    device_info.ppEnabledExtensionNames =
            device_info.extensionCount ? info.device_extension_names.data() : NULL;
    device_info.pEnabledFeatures = NULL;

    res = vkCreateDevice(info.gpus[0], &device_info, &info.device);
    assert(res == VK_SUCCESS);

    return res;
}

void init_connection(struct sample_info &info)
{
#ifndef _WIN32
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    info.connection = xcb_connect(NULL, &scr);
    if (info.connection == NULL) {
        std::cout << "Cannot find a compatible Vulkan ICD.\n";
        exit(-1);
    }

    setup = xcb_get_setup(info.connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    info.screen = iter.data;
#endif // _WIN32
}

VkResult memory_type_from_properties(struct sample_info &info, uint32_t typeBits, VkFlags properties, uint32_t *typeIndex)
{
     // Search memtypes to find first index with those properties
     for (uint32_t i = 0; i < 32; i++) {
         if ((typeBits & 1) == 1) {
             // Type is available, does it match user properties?
             if ((info.memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                 *typeIndex = i;
                 return VK_SUCCESS;
             }
         }
         typeBits >>= 1;
     }
     // No memory types matched, return failure
     return VK_UNSUPPORTED;
}

void set_image_layout(
        struct sample_info &info,
        VkImage image,
        VkImageAspectFlags aspectMask,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout)
{
    /* DEPENDS on info.cmd and info.queue initialized */

    assert(info.cmd != VK_NULL_HANDLE);
    assert(info.queue != VK_NULL_HANDLE);

    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = NULL;
    image_memory_barrier.outputMask = 0;
    image_memory_barrier.inputMask = 0;
    image_memory_barrier.oldLayout = old_image_layout;
    image_memory_barrier.newLayout = new_image_layout;
    image_memory_barrier.image = image;
    image_memory_barrier.subresourceRange.aspectMask = aspectMask;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.mipLevels = 1;
    image_memory_barrier.subresourceRange.arraySize = 0;

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.inputMask = VK_MEMORY_INPUT_TRANSFER_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.outputMask = VK_MEMORY_OUTPUT_HOST_WRITE_BIT | VK_MEMORY_OUTPUT_TRANSFER_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(info.cmd, src_stages, dest_stages, false, 1, (const void * const*)&pmemory_barrier);
}


void init_and_begin_command_buffer(struct sample_info &info)
{
    /* DEPENDS on init_wsi() */

    VkResult U_ASSERT_ONLY res;

    VkCmdPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_CMD_POOL_CREATE_INFO;
    cmd_pool_info.pNext = NULL;
    cmd_pool_info.queueFamilyIndex = info.graphics_queue_family_index;
    cmd_pool_info.flags = 0;

    res = vkCreateCommandPool(info.device, &cmd_pool_info, &info.cmd_pool);
    assert(res == VK_SUCCESS);

    VkCmdBufferCreateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    cmd.pNext = NULL;
    cmd.cmdPool = info.cmd_pool;
    cmd.level = VK_CMD_BUFFER_LEVEL_PRIMARY;
    cmd.flags = 0;

    res = vkCreateCommandBuffer(info.device, &cmd, &info.cmd);
    assert(res == VK_SUCCESS);

    VkCmdBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.renderPass = 0;  /* May only set renderPass and framebuffer */
    cmd_buf_info.framebuffer = 0; /* for secondary command buffers           */
    cmd_buf_info.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
                         VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;

    res = vkBeginCommandBuffer(info.cmd, &cmd_buf_info);
    assert(res == VK_SUCCESS);
}

void init_device_queue(struct sample_info &info)
{
    /* DEPENDS on init_wsi() */

    VkResult U_ASSERT_ONLY res;
    res = vkGetDeviceQueue(info.device, info.graphics_queue_family_index,
            0, &info.queue);
    assert(res == VK_SUCCESS);
}

void init_swap_chain(struct sample_info &info)
{
    /* DEPENDS on info.cmd and info.queue initialized */

    VkResult U_ASSERT_ONLY res;
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
    swap_chain.oldSwapchain.handle = 0;
    swap_chain.clipped = true;
    swap_chain.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swap_chain.imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_chain.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_chain.queueFamilyCount = 0;
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

    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        swap_chain_buffer sc_buffer;

        VkImageViewCreateInfo color_image_view = {};
        color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        color_image_view.pNext = NULL;
        color_image_view.format = info.format;
        color_image_view.channels.r = VK_CHANNEL_SWIZZLE_R;
        color_image_view.channels.g = VK_CHANNEL_SWIZZLE_G;
        color_image_view.channels.b = VK_CHANNEL_SWIZZLE_B;
        color_image_view.channels.a = VK_CHANNEL_SWIZZLE_A;
        color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        color_image_view.subresourceRange.baseMipLevel = 0;
        color_image_view.subresourceRange.mipLevels = 1;
        color_image_view.subresourceRange.baseArrayLayer = 0;
        color_image_view.subresourceRange.arraySize = 1;
        color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        color_image_view.flags = 0;


        sc_buffer.image = swapchainImages[i];

        set_image_layout(info, sc_buffer.image,
                               VK_IMAGE_ASPECT_COLOR_BIT,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        color_image_view.image = sc_buffer.image;

        res = vkCreateImageView(info.device,
                &color_image_view, &sc_buffer.view);
        info.buffers.push_back(sc_buffer);
        assert(res == VK_SUCCESS);
    }
    info.current_buffer = 0;
}

void init_depth_buffer(struct sample_info &info)
{
    VkResult U_ASSERT_ONLY res;
    VkImageCreateInfo image_info = {};
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    VkFormatProperties props;
    res = vkGetPhysicalDeviceFormatProperties(info.gpus[0], depth_format, &props);
    assert(res == VK_SUCCESS);
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
    image_info.queueFamilyCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_info.flags = 0;

    VkMemoryAllocInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;

    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.pNext = NULL;
    view_info.image = VK_NULL_HANDLE;
    view_info.format = depth_format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.mipLevels = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.arraySize = 1;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.flags = 0;

    VkMemoryRequirements mem_reqs;

    info.depth.format = depth_format;

    /* Create image */
    res = vkCreateImage(info.device, &image_info,
                        &info.depth.image);
    assert(res == VK_SUCCESS);

    res = vkGetImageMemoryRequirements(info.device,
                                       info.depth.image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;
    /* Use the memory properties to determine the type of memory required */
    res = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_DEVICE_ONLY,
                                      &mem_alloc.memoryTypeIndex);
    assert(res == VK_SUCCESS);

    /* Allocate memory */
    res = vkAllocMemory(info.device, &mem_alloc, &info.depth.mem);
    assert(res == VK_SUCCESS);

    /* Bind memory */
    res = vkBindImageMemory(info.device, info.depth.image,
                            info.depth.mem, 0);
    assert(res == VK_SUCCESS);

    /* Set the image layout to depth stencil optimal */
    set_image_layout(info, info.depth.image,
                          VK_IMAGE_ASPECT_DEPTH_BIT,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    /* Create image view */
   view_info.image = info.depth.image;
   res = vkCreateImageView(info.device, &view_info, &info.depth.view);
   assert(res == VK_SUCCESS);
}

void init_uniform_buffer(struct sample_info &info)
{
    VkResult U_ASSERT_ONLY res;
    info.Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    info.View       = glm::lookAt(
                          glm::vec3(0,3,10), // Camera is at (0,3,10), in World Space
                          glm::vec3(0,0,0), // and looks at the origin
                          glm::vec3(0,-1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                          );
    info.Model = glm::mat4(1.0f);
    info.MVP = info.Projection * info.View * info.Model;

    /* VULKAN_KEY_START */
    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(info.MVP);
    buf_info.queueFamilyCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(info.device, &buf_info, &info.uniform_data.buf);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements mem_reqs;
    res = vkGetBufferMemoryRequirements(info.device, info.uniform_data.buf, &mem_reqs);
    assert(res == VK_SUCCESS);

    VkMemoryAllocInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    res = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(res == VK_SUCCESS);

    res = vkAllocMemory(info.device, &alloc_info, &(info.uniform_data.mem));
    assert(res == VK_SUCCESS);

    uint8_t *pData;
    res = vkMapMemory(info.device, info.uniform_data.mem, 0, 0, 0, (void **) &pData);
    assert(res == VK_SUCCESS);

    memcpy(pData, &info.MVP, sizeof(info.MVP));

    vkUnmapMemory(info.device, info.uniform_data.mem);

    res = vkBindBufferMemory(info.device,
            info.uniform_data.buf,
            info.uniform_data.mem, 0);
    assert(res == VK_SUCCESS);

    info.uniform_data.desc.bufferInfo.buffer = info.uniform_data.buf;
    info.uniform_data.desc.bufferInfo.offset = 0;
    info.uniform_data.desc.bufferInfo.range = sizeof(info.MVP);
}

void init_descriptor_and_pipeline_layouts(struct sample_info &info, bool use_texture)
{
    VkDescriptorSetLayoutBinding layout_bindings[2];
    layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_bindings[0].arraySize = 1;
    layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_bindings[0].pImmutableSamplers = NULL;

    if (use_texture)
    {
        layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layout_bindings[1].arraySize = 1;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layout_bindings[1].pImmutableSamplers = NULL;
    }

    /* Next take layout bindings and use them to create a descriptor set layout */
    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = NULL;
    descriptor_layout.count = use_texture?2:1;
    descriptor_layout.pBinding = layout_bindings;

    VkResult U_ASSERT_ONLY err;

    info.desc_layout.resize(NUM_DESCRIPTOR_SETS);
    err = vkCreateDescriptorSetLayout(info.device,
            &descriptor_layout, info.desc_layout.data());
    assert(!err);

    /* Now use the descriptor layout to create a pipeline layout */
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext                  = NULL;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pPipelineLayoutCreateInfo.pPushConstantRanges    = NULL;
    pPipelineLayoutCreateInfo.descriptorSetCount     = NUM_DESCRIPTOR_SETS;
    pPipelineLayoutCreateInfo.pSetLayouts            = info.desc_layout.data();

    err = vkCreatePipelineLayout(info.device,
                                 &pPipelineLayoutCreateInfo,
                                 &info.pipeline_layout);
    assert(!err);
}

void init_renderpass(struct sample_info &info)
{
    /* DEPENDS on init_swap_chain() and init_depth_buffer() */

    VkResult U_ASSERT_ONLY res;
    /* Need attachments for render target and depth buffer */
    VkAttachmentDescription attachments[2];
    attachments[0].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION;
    attachments[0].pNext = NULL;
    attachments[0].format = info.format;
    attachments[0].samples = 1;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].flags = 0;

    attachments[1].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION;
    attachments[1].pNext = NULL;
    attachments[1].format = info.depth.format;
    attachments[1].samples = 1;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].flags = 0;

    VkAttachmentReference color_reference = {};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION;
    subpass.pNext = NULL;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = NULL;
    subpass.depthStencilAttachment.attachment = 1;
    subpass.depthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    subpass.preserveCount = 0;
    subpass.pPreserveAttachments = NULL;

    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = NULL;
    rp_info.attachmentCount = 2;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 0;
    rp_info.pDependencies = NULL;

    res = vkCreateRenderPass(info.device, &rp_info, &info.render_pass);
    assert(res == VK_SUCCESS);
}

void init_shaders(struct sample_info &info, const char *vertShaderText, const char *fragShaderText)
{
    VkResult U_ASSERT_ONLY res;
    bool U_ASSERT_ONLY retVal;

    std::vector<unsigned int> vtx_spv;
    info.shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX;
    info.shaderStages[0].pNext  = NULL;
    info.shaderStages[0].pSpecializationInfo = NULL;

    init_glslang();
    retVal = GLSLtoSPV(VK_SHADER_STAGE_VERTEX, vertShaderText, vtx_spv);
    assert(retVal);

    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = vtx_spv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = vtx_spv.data();
    res = vkCreateShaderModule(info.device, &moduleCreateInfo, &info.vert_shader_module);
    assert(res == VK_SUCCESS);

    VkShaderCreateInfo shaderCreateInfo;
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    shaderCreateInfo.pNext = NULL;
    shaderCreateInfo.flags = 0;
    shaderCreateInfo.module = info.vert_shader_module;
    shaderCreateInfo.pName = "main";
    shaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX;
    res = vkCreateShader(info.device, &shaderCreateInfo, &info.shaderStages[0].shader);
    assert(res == VK_SUCCESS);

    std::vector<unsigned int> frag_spv;
    info.shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT;
    info.shaderStages[1].pNext  = NULL;
    info.shaderStages[1].pSpecializationInfo = NULL;

    retVal = GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT, fragShaderText, frag_spv);
    assert(retVal);

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = frag_spv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = frag_spv.data();
    res = vkCreateShaderModule(info.device, &moduleCreateInfo, &info.frag_shader_module);
    assert(res == VK_SUCCESS);

    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    shaderCreateInfo.pNext = NULL;
    shaderCreateInfo.flags = 0;
    shaderCreateInfo.module = info.frag_shader_module;
    shaderCreateInfo.pName = "main";
    shaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT;
    res = vkCreateShader(info.device, &shaderCreateInfo, &info.shaderStages[1].shader);
    assert(res == VK_SUCCESS);

    finalize_glslang();
}

void init_framebuffers(struct sample_info &info)
{
    /* DEPENDS on init_depth_buffer(), init_renderpass() and init_wsi() */

    VkResult U_ASSERT_ONLY res;
    VkImageView attachments[2];
    attachments[1] = info.depth.view;

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

    info.framebuffers = (VkFramebuffer *) malloc(info.swapchainImageCount * sizeof(VkFramebuffer));
    assert(info.framebuffers);

    for (i = 0; i < info.swapchainImageCount; i++) {
        attachments[0] = info.buffers[i].view;
        res = vkCreateFramebuffer(info.device, &fb_info, &info.framebuffers[i]);
        assert(res == VK_SUCCESS);
    }
}

void init_vertex_buffer(struct sample_info &info, const void *vertexData, uint32_t dataSize, uint32_t dataStride, bool use_texture)
{
    VkResult U_ASSERT_ONLY res;

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.size = sizeof(g_vb_solid_face_colors_Data);
    buf_info.queueFamilyCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(info.device, &buf_info, &info.vertex_buffer.buf);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements mem_reqs;
    res = vkGetBufferMemoryRequirements(info.device, info.vertex_buffer.buf, &mem_reqs);
    assert(res == VK_SUCCESS);

    VkMemoryAllocInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    res = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(res == VK_SUCCESS);

    res = vkAllocMemory(info.device, &alloc_info, &(info.vertex_buffer.mem));
    assert(res == VK_SUCCESS);

    uint8_t *pData;
    res = vkMapMemory(info.device, info.vertex_buffer.mem, 0, 0, 0, (void **) &pData);
    assert(res == VK_SUCCESS);

    memcpy(pData, vertexData, dataSize);

    vkUnmapMemory(info.device, info.vertex_buffer.mem);

    res = vkBindBufferMemory(info.device,
            info.vertex_buffer.buf,
            info.vertex_buffer.mem, 0);
    assert(res == VK_SUCCESS);

    info.vi_binding.binding = 0;
    info.vi_binding.stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX;
    info.vi_binding.strideInBytes = dataStride;

    info.vi_attribs[0].binding = 0;
    info.vi_attribs[0].location = 0;
    info.vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[0].offsetInBytes = 0;
    info.vi_attribs[1].binding = 0;
    info.vi_attribs[1].location = 1;
    info.vi_attribs[1].format = use_texture?VK_FORMAT_R32G32_SFLOAT:VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[1].offsetInBytes = 16;
}

void init_descriptor_set(struct sample_info &info, bool use_texture)
{
    /* DEPENDS on init_uniform_buffer() and init_descriptor_and_pipeline_layouts() */

    VkResult U_ASSERT_ONLY res;
    VkDescriptorInfo tex_desc;

    VkDescriptorTypeCount type_count[2];
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].count = 1;
    if (use_texture)
    {
        type_count[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        type_count[1].count = 1;
    }

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = NULL;
    descriptor_pool.poolUsage = VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT;
    descriptor_pool.maxSets = 1;
    descriptor_pool.count = use_texture?2:1;
    descriptor_pool.pTypeCount = type_count;

    res = vkCreateDescriptorPool(info.device,
        &descriptor_pool, &info.desc_pool);
    assert(res == VK_SUCCESS);

    info.desc_set.resize(NUM_DESCRIPTOR_SETS);
    res = vkAllocDescriptorSets(info.device, info.desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            NUM_DESCRIPTOR_SETS, info.desc_layout.data(),
            info.desc_set.data());
    assert(res == VK_SUCCESS);

    VkWriteDescriptorSet writes[2];

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = NULL;
    writes[0].destSet = info.desc_set[0];
    writes[0].count = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pDescriptors = &info.uniform_data.desc;
    writes[0].destArrayElement = 0;
    writes[0].destBinding = 0;

    if (use_texture)
    {
        tex_desc.imageView = 0;
        tex_desc.bufferView = 0;
        tex_desc.imageView = info.textures[0].view;
        tex_desc.sampler = info.textures[0].sampler;
        tex_desc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].destSet = info.desc_set[0];
        writes[1].destBinding = 1;
        writes[1].count = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pDescriptors = &tex_desc;
        writes[1].destArrayElement = 0;
    }

    vkUpdateDescriptorSets(info.device, use_texture?2:1, writes, 0, NULL);
}

void init_pipeline(struct sample_info &info)
{
    VkResult U_ASSERT_ONLY res;

    VkPipelineCacheCreateInfo pipelineCache;
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCache.pNext = NULL;
    pipelineCache.initialData = 0;
    pipelineCache.initialSize = 0;
    pipelineCache.maxSize = 0;

    res = vkCreatePipelineCache(info.device, &pipelineCache, &info.pipelineCache);
    assert(res == VK_SUCCESS);

    VkDynamicState                         dynamicStateEnables[VK_DYNAMIC_STATE_NUM];
    VkPipelineDynamicStateCreateInfo       dynamicState = {};
    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = NULL;
    dynamicState.pDynamicStates = dynamicStateEnables;
    dynamicState.dynamicStateCount = 0;

    VkPipelineVertexInputStateCreateInfo vi;
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.pNext = NULL;
    vi.bindingCount = 1;
    vi.pVertexBindingDescriptions = &info.vi_binding;
    vi.attributeCount = 2;
    vi.pVertexAttributeDescriptions = info.vi_attribs;

    VkPipelineInputAssemblyStateCreateInfo ia;
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.pNext = NULL;
    ia.primitiveRestartEnable = VK_FALSE;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterStateCreateInfo rs;
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTER_STATE_CREATE_INFO;
    rs.pNext = NULL;
    rs.fillMode = VK_FILL_MODE_SOLID;
    rs.cullMode = VK_CULL_MODE_BACK;
    rs.frontFace = VK_FRONT_FACE_CW;
    rs.depthClipEnable = VK_TRUE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;
    rs.depthBias = 0;
    rs.depthBiasClamp = 0;
    rs.slopeScaledDepthBias = 0;
    rs.lineWidth = 0;

    VkPipelineColorBlendStateCreateInfo cb;
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.pNext = NULL;
    VkPipelineColorBlendAttachmentState att_state[1];
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    att_state[0].blendOpAlpha = VK_BLEND_OP_ADD;
    att_state[0].blendOpColor = VK_BLEND_OP_ADD;
    att_state[0].srcBlendColor = VK_BLEND_ZERO;
    att_state[0].destBlendColor = VK_BLEND_ZERO;
    att_state[0].srcBlendAlpha = VK_BLEND_ZERO;
    att_state[0].destBlendAlpha = VK_BLEND_ZERO;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;
    cb.logicOpEnable = VK_FALSE;
    cb.logicOp = VK_LOGIC_OP_NOOP;
    cb.alphaToCoverageEnable = VK_FALSE;
    cb.alphaToOneEnable = VK_FALSE;
    cb.blendConst[0] = 1.0f;
    cb.blendConst[1] = 1.0f;
    cb.blendConst[2] = 1.0f;
    cb.blendConst[3] = 1.0f;

    VkPipelineViewportStateCreateInfo vp;
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.pNext = NULL;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

    VkPipelineDepthStencilStateCreateInfo ds;
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.pNext = NULL;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.minDepthBounds = 0;
    ds.maxDepthBounds = 0;
    ds.stencilTestEnable = VK_FALSE;
    ds.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    VkPipelineMultisampleStateCreateInfo   ms;
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pNext = NULL;
    ms.pSampleMask = NULL;
    ms.rasterSamples = 1;
    ms.sampleShadingEnable = VK_FALSE;
    ms.minSampleShading = 0.0;

    VkGraphicsPipelineCreateInfo pipeline;
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.pNext               = NULL;
    pipeline.layout              = info.pipeline_layout;
    pipeline.basePipelineHandle  = 0;
    pipeline.basePipelineIndex   = 0;
    pipeline.flags               = 0;
    pipeline.pVertexInputState   = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterState        = &rs;
    pipeline.pColorBlendState    = &cb;
    pipeline.pTessellationState  = NULL;
    pipeline.pMultisampleState   = &ms;
    pipeline.pDynamicState       = &dynamicState;
    pipeline.pViewportState      = &vp;
    pipeline.pDepthStencilState  = &ds;
    pipeline.pStages             = info.shaderStages;
    pipeline.stageCount          = 2;
    pipeline.renderPass          = info.render_pass;
    pipeline.subpass             = 0;

    res = vkCreateGraphicsPipelines(info.device, info.pipelineCache, 1, &pipeline, &info.pipeline);
    assert(res == VK_SUCCESS);
}

#ifdef _WIN32
static void run(struct sample_info *info)
{
 /* Placeholder for samples that want to show dynamic content */
}

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    struct sample_info *info = reinterpret_cast<struct sample_info*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch(uMsg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        run(info);
        return 0;
    default:
        break;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void init_window(struct sample_info &info)
{
    WNDCLASSEX  win_class;
    assert(info.width > 0);
    assert(info.height > 0);

    info.connection = GetModuleHandle(NULL);
    sprintf(info.name, "Sample");

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = info.connection; // hInstance
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = info.name;
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    // Register window class:
    if (!RegisterClassEx(&win_class)) {
        // It didn't work, so try to give a useful error:
        printf("Unexpected error trying to start the application!\n");
        fflush(stdout);
        exit(1);
    }
    // Create window with the registered class:
    RECT wr = { 0, 0, info.width, info.height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    info.window = CreateWindowEx(0,
                                  info.name,           // class name
                                  info.name,           // app name
                                  WS_OVERLAPPEDWINDOW | // window style
                                  WS_VISIBLE |
                                  WS_SYSMENU,
                                  100,100,              // x/y coords
                                  wr.right-wr.left,     // width
                                  wr.bottom-wr.top,     // height
                                  NULL,                 // handle to parent
                                  NULL,                 // handle to menu
                                  info.connection,     // hInstance
                                  NULL);                // no extra parameters
    if (!info.window) {
        // It didn't work, so try to give a useful error:
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }
    SetWindowLongPtr(info.window, GWLP_USERDATA, (LONG_PTR) &info);
}

void destroy_window(struct sample_info &info)
{
    DestroyWindow(info.window);
}
#else
void init_window(struct sample_info &info)
{
    assert(info.width > 0);
    assert(info.height > 0);

    uint32_t value_mask, value_list[32];

    info.window = xcb_generate_id(info.connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = info.screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(info.connection,
            XCB_COPY_FROM_PARENT,
            info.window, info.screen->root,
            0, 0, info.width, info.height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            info.screen->root_visual,
            value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(info.connection, 1, 12,
                                                      "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(info.connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(info.connection, 0, 16, "WM_DELETE_WINDOW");
    info.atom_wm_delete_window = xcb_intern_atom_reply(info.connection, cookie2, 0);

    xcb_change_property(info.connection, XCB_PROP_MODE_REPLACE,
                        info.window, (*reply).atom, 4, 32, 1,
                        &(*info.atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(info.connection, info.window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive runs
    const uint32_t coords[] = {100,  100};
    xcb_configure_window(info.connection, info.window,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
    xcb_flush(info.connection);

    xcb_generic_event_t *e;
    while ((e = xcb_wait_for_event(info.connection))) {
       if  ((e->response_type & ~0x80) == XCB_EXPOSE)
           break;
    }

}

void destroy_window(struct sample_info &info)
{
    xcb_destroy_window(info.connection, info.window);
    xcb_disconnect(info.connection);
}
#endif // _WIN32

void init_swapchain_extension(struct sample_info &info)
{
    /* DEPENDS on init_connection() and init_window() */

    VkResult U_ASSERT_ONLY res;

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

    res = vkGetPhysicalDeviceQueueFamilyProperties(info.gpus[0], &info.queue_count, NULL);
    assert(res == VK_SUCCESS);
    assert(info.queue_count >= 1);

    info.queue_props.resize(info.queue_count);
    res = vkGetPhysicalDeviceQueueFamilyProperties(info.gpus[0], &info.queue_count, info.queue_props.data());
    assert(res == VK_SUCCESS);
    assert(info.queue_count >= 1);

    // Construct the WSI surface description:
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

    // Iterate over each queue to learn whether it supports presenting to WSI:
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
}

void init_resources(TBuiltInResource &Resources)
{
    Resources.maxLights = 32;
    Resources.maxClipPlanes = 6;
    Resources.maxTextureUnits = 32;
    Resources.maxTextureCoords = 32;
    Resources.maxVertexAttribs = 64;
    Resources.maxVertexUniformComponents = 4096;
    Resources.maxVaryingFloats = 64;
    Resources.maxVertexTextureImageUnits = 32;
    Resources.maxCombinedTextureImageUnits = 80;
    Resources.maxTextureImageUnits = 32;
    Resources.maxFragmentUniformComponents = 4096;
    Resources.maxDrawBuffers = 32;
    Resources.maxVertexUniformVectors = 128;
    Resources.maxVaryingVectors = 8;
    Resources.maxFragmentUniformVectors = 16;
    Resources.maxVertexOutputVectors = 16;
    Resources.maxFragmentInputVectors = 15;
    Resources.minProgramTexelOffset = -8;
    Resources.maxProgramTexelOffset = 7;
    Resources.maxClipDistances = 8;
    Resources.maxComputeWorkGroupCountX = 65535;
    Resources.maxComputeWorkGroupCountY = 65535;
    Resources.maxComputeWorkGroupCountZ = 65535;
    Resources.maxComputeWorkGroupSizeX = 1024;
    Resources.maxComputeWorkGroupSizeY = 1024;
    Resources.maxComputeWorkGroupSizeZ = 64;
    Resources.maxComputeUniformComponents = 1024;
    Resources.maxComputeTextureImageUnits = 16;
    Resources.maxComputeImageUniforms = 8;
    Resources.maxComputeAtomicCounters = 8;
    Resources.maxComputeAtomicCounterBuffers = 1;
    Resources.maxVaryingComponents = 60;
    Resources.maxVertexOutputComponents = 64;
    Resources.maxGeometryInputComponents = 64;
    Resources.maxGeometryOutputComponents = 128;
    Resources.maxFragmentInputComponents = 128;
    Resources.maxImageUnits = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    Resources.maxCombinedShaderOutputResources = 8;
    Resources.maxImageSamples = 0;
    Resources.maxVertexImageUniforms = 0;
    Resources.maxTessControlImageUniforms = 0;
    Resources.maxTessEvaluationImageUniforms = 0;
    Resources.maxGeometryImageUniforms = 0;
    Resources.maxFragmentImageUniforms = 8;
    Resources.maxCombinedImageUniforms = 8;
    Resources.maxGeometryTextureImageUnits = 16;
    Resources.maxGeometryOutputVertices = 256;
    Resources.maxGeometryTotalOutputComponents = 1024;
    Resources.maxGeometryUniformComponents = 1024;
    Resources.maxGeometryVaryingComponents = 64;
    Resources.maxTessControlInputComponents = 128;
    Resources.maxTessControlOutputComponents = 128;
    Resources.maxTessControlTextureImageUnits = 16;
    Resources.maxTessControlUniformComponents = 1024;
    Resources.maxTessControlTotalOutputComponents = 4096;
    Resources.maxTessEvaluationInputComponents = 128;
    Resources.maxTessEvaluationOutputComponents = 128;
    Resources.maxTessEvaluationTextureImageUnits = 16;
    Resources.maxTessEvaluationUniformComponents = 1024;
    Resources.maxTessPatchComponents = 120;
    Resources.maxPatchVertices = 32;
    Resources.maxTessGenLevel = 64;
    Resources.maxViewports = 16;
    Resources.maxVertexAtomicCounters = 0;
    Resources.maxTessControlAtomicCounters = 0;
    Resources.maxTessEvaluationAtomicCounters = 0;
    Resources.maxGeometryAtomicCounters = 0;
    Resources.maxFragmentAtomicCounters = 8;
    Resources.maxCombinedAtomicCounters = 8;
    Resources.maxAtomicCounterBindings = 1;
    Resources.maxVertexAtomicCounterBuffers = 0;
    Resources.maxTessControlAtomicCounterBuffers = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers = 0;
    Resources.maxGeometryAtomicCounterBuffers = 0;
    Resources.maxFragmentAtomicCounterBuffers = 1;
    Resources.maxCombinedAtomicCounterBuffers = 1;
    Resources.maxAtomicCounterBufferSize = 16384;
    Resources.maxTransformFeedbackBuffers = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances = 8;
    Resources.maxCombinedClipAndCullDistances = 8;
    Resources.maxSamples = 4;
    Resources.limits.nonInductiveForLoops = 1;
    Resources.limits.whileLoops = 1;
    Resources.limits.doWhileLoops = 1;
    Resources.limits.generalUniformIndexing = 1;
    Resources.limits.generalAttributeMatrixVectorIndexing = 1;
    Resources.limits.generalVaryingIndexing = 1;
    Resources.limits.generalSamplerIndexing = 1;
    Resources.limits.generalVariableIndexing = 1;
    Resources.limits.generalConstantMatrixVectorIndexing = 1;

}

EShLanguage FindLanguage(const VkShaderStage shader_type)
{
    switch (shader_type) {
    case VK_SHADER_STAGE_VERTEX:
        return EShLangVertex;

    case VK_SHADER_STAGE_TESS_CONTROL:
        return EShLangTessControl;

    case VK_SHADER_STAGE_TESS_EVALUATION:
        return EShLangTessEvaluation;

    case VK_SHADER_STAGE_GEOMETRY:
        return EShLangGeometry;

    case VK_SHADER_STAGE_FRAGMENT:
        return EShLangFragment;

    case VK_SHADER_STAGE_COMPUTE:
        return EShLangCompute;

    default:
        return EShLangVertex;
    }
}

void init_glslang()
{
    glslang::InitializeProcess();
}

void finalize_glslang()
{
    glslang::FinalizeProcess();
}

//
// Compile a given string containing GLSL into SPV for use by VK
// Return value of false means an error was encountered.
//
bool GLSLtoSPV(const VkShaderStage shader_type,
               const char *pshader,
               std::vector<unsigned int> &spirv)
{
    glslang::TProgram& program = *new glslang::TProgram;
    const char *shaderStrings[1];
    TBuiltInResource Resources;
    init_resources(Resources);

    EShMessages messages = EShMsgDefault;

    EShLanguage stage = FindLanguage(shader_type);
    glslang::TShader* shader = new glslang::TShader(stage);

    shaderStrings[0] = pshader;
    shader->setStrings(shaderStrings, 1);

    if (! shader->parse(&Resources, 100, false, messages)) {
        puts(shader->getInfoLog());
        puts(shader->getInfoDebugLog());
        return false; // something didn't work
    }

    program.addShader(shader);

    //
    // Program-level processing...
    //

    if (! program.link(messages)) {
        puts(shader->getInfoLog());
        puts(shader->getInfoDebugLog());
        return false;
    }

    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

    return true;
}
