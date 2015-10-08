/*
 * Vulkan
 *
 * Copyright (C) 2014-2015 LunarG, Inc.
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
 * Draw a textured triangle with depth testing.  This is written against Intel
 * ICD.  It does not do state transition nor object memory binding like it
 * should.  It also does no error checking.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#define APP_NAME_STR_LEN 80
#else  // _WIN32
#include <xcb/xcb.h>
#endif // _WIN32

#include <vulkan.h>
#include "vk_debug_report_lunarg.h"
#include <vk_ext_khr_swapchain.h>
#include <vk_ext_khr_device_swapchain.h>

#include "icd-spv.h"

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1
#define VERTEX_BUFFER_BIND_ID 0
#define APP_SHORT_NAME "tri"
#define APP_LONG_NAME "The Vulkan Triangle Demo Program"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#ifdef _WIN32
#define ERR_EXIT(err_msg, err_class)                    \
    do {                                                \
        MessageBox(NULL, err_msg, err_class, MB_OK);    \
        exit(1);                                        \
   } while (0)
#else  // _WIN32

#define ERR_EXIT(err_msg, err_class)                    \
    do {                                                \
        printf(err_msg);                                \
        fflush(stdout);                                 \
        exit(1);                                        \
   } while (0)
#endif // _WIN32

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                        \
{                                                                       \
    demo->fp##entrypoint = (PFN_vk##entrypoint) vkGetInstanceProcAddr(inst, "vk"#entrypoint); \
    if (demo->fp##entrypoint == NULL) {                                 \
        ERR_EXIT("vkGetInstanceProcAddr failed to find vk"#entrypoint,  \
                 "vkGetInstanceProcAddr Failure");                      \
    }                                                                   \
}

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
    demo->fp##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);   \
    if (demo->fp##entrypoint == NULL) {                                 \
        ERR_EXIT("vkGetDeviceProcAddr failed to find vk"#entrypoint,    \
                 "vkGetDeviceProcAddr Failure");                        \
    }                                                                   \
}

struct texture_object {
    VkSampler sampler;

    VkImage image;
    VkImageLayout imageLayout;

    VkDeviceMemory mem;
    VkImageView view;
    int32_t tex_width, tex_height;
};

VkBool32 dbgFunc(
    VkFlags                             msgFlags,
    VkDbgObjectType                     objType,
    uint64_t                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData)
{
    char *message = (char *) malloc(strlen(pMsg)+100);

    assert (message);

    if (msgFlags & VK_DBG_REPORT_ERROR_BIT) {
        sprintf(message,"ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
    } else if (msgFlags & VK_DBG_REPORT_WARN_BIT) {
        sprintf(message,"WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode, pMsg);
    } else {
        return false;
    }

#ifdef _WIN32
    MessageBox(NULL, message, "Alert", MB_OK);
#else
    printf("%s\n",message);
    fflush(stdout);
#endif
    free(message);

    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return false;
}

typedef struct _SwapchainBuffers {
    VkImage image;
    VkCmdBuffer cmd;
    VkImageView view;
} SwapchainBuffers;

struct demo {
#ifdef _WIN32
#define APP_NAME_STR_LEN 80
    HINSTANCE connection;        // hInstance - Windows Instance
    char name[APP_NAME_STR_LEN]; // Name to put on the window/icon
    HWND        window;          // hWnd - window handle
#else  // _WIN32
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;
    VkPlatformHandleXcbKHR platform_handle_xcb;
#endif // _WIN32
	bool prepared;
    bool use_staging_buffer;
    bool use_glsl;

    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    VkPhysicalDeviceProperties gpu_props;
    VkQueueFamilyProperties *queue_props;
    uint32_t graphics_queue_node_index;

    int width, height;
    VkFormat format;
    VkColorSpaceKHR color_space;

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetSurfacePropertiesKHR fpGetSurfacePropertiesKHR;
    PFN_vkGetSurfaceFormatsKHR fpGetSurfaceFormatsKHR;
    PFN_vkGetSurfacePresentModesKHR fpGetSurfacePresentModesKHR;
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;
    VkSurfaceDescriptionWindowKHR surface_description;
    uint32_t swapchainImageCount;
    VkSwapchainKHR swap_chain;
    SwapchainBuffers *buffers;

    VkCmdPool cmd_pool;

    struct {
        VkFormat format;

        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } depth;

    struct texture_object textures[DEMO_TEXTURE_COUNT];

    struct {
        VkBuffer buf;
        VkDeviceMemory mem;

        VkPipelineVertexInputStateCreateInfo vi;
        VkVertexInputBindingDescription vi_bindings[1];
        VkVertexInputAttributeDescription vi_attrs[2];
    } vertices;

    VkCmdBuffer setup_cmd;  // Command Buffer for initialization commands
    VkCmdBuffer draw_cmd;  // Command Buffer for drawing commands
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    VkFramebuffer framebuffers[DEMO_BUFFER_COUNT];

    VkPhysicalDeviceMemoryProperties memory_properties;

    bool validate;
    PFN_vkDbgCreateMsgCallback dbgCreateMsgCallback;
    PFN_vkDbgDestroyMsgCallback dbgDestroyMsgCallback;
    VkDbgMsgCallback msg_callback;

    float depthStencil;
    float depthIncrement;

    bool quit;
    uint32_t current_buffer;
    uint32_t queue_count;
};

static VkResult memory_type_from_properties(struct demo *demo, uint32_t typeBits, VkFlags properties, uint32_t *typeIndex)
{
     // Search memtypes to find first index with those properties
     for (uint32_t i = 0; i < 32; i++) {
         if ((typeBits & 1) == 1) {
             // Type is available, does it match user properties?
             if ((demo->memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                 *typeIndex = i;
                 return VK_SUCCESS;
             }
         }
         typeBits >>= 1;
     }
     // No memory types matched, return failure
     return VK_UNSUPPORTED;
}

static void demo_flush_init_cmd(struct demo *demo)
{
    VkResult U_ASSERT_ONLY err;

    if (demo->setup_cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(demo->setup_cmd);
    assert(!err);

    const VkCmdBuffer cmd_bufs[] = { demo->setup_cmd };
    VkFence nullFence = {VK_NULL_HANDLE};

    err = vkQueueSubmit(demo->queue, 1, cmd_bufs, nullFence);
    assert(!err);

    err = vkQueueWaitIdle(demo->queue);
    assert(!err);

    vkDestroyCommandBuffer(demo->device, demo->setup_cmd);
    demo->setup_cmd = VK_NULL_HANDLE;
}

static void demo_set_image_layout(
        struct demo *demo,
        VkImage image,
        VkImageAspectFlags aspectMask,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout)
{
    VkResult U_ASSERT_ONLY err;

    if (demo->setup_cmd == VK_NULL_HANDLE) {
        const VkCmdBufferCreateInfo cmd = {
            .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .cmdPool = demo->cmd_pool,
            .level = VK_CMD_BUFFER_LEVEL_PRIMARY,
            .flags = 0,
        };

        err = vkCreateCommandBuffer(demo->device, &cmd, &demo->setup_cmd);
        assert(!err);

        VkCmdBufferBeginInfo cmd_buf_info = {
            .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
                VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
            .renderPass = { VK_NULL_HANDLE },
            .subpass = 0,
            .framebuffer = { VK_NULL_HANDLE },
        };
        err = vkBeginCommandBuffer(demo->setup_cmd, &cmd_buf_info);
    }

    VkImageMemoryBarrier image_memory_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = NULL,
        .outputMask = 0,
        .inputMask = 0,
        .oldLayout = old_image_layout,
        .newLayout = new_image_layout,
        .image = image,
        .subresourceRange = { aspectMask, 0, 1, 0, 0 }
    };

    if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL) {
        /* Make sure anything that was copying from this image has completed */
        image_memory_barrier.inputMask = VK_MEMORY_INPUT_TRANSFER_BIT;
    }

    if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        /* Make sure any Copy or CPU writes to image are flushed */
        image_memory_barrier.outputMask = VK_MEMORY_OUTPUT_TRANSFER_BIT | VK_MEMORY_OUTPUT_HOST_WRITE_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(demo->setup_cmd, src_stages, dest_stages, false, 1, (const void * const*)&pmemory_barrier);
}

static void demo_draw_build_cmd(struct demo *demo)
{
    const VkCmdBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
            VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
        .renderPass = { VK_NULL_HANDLE },
        .subpass = 0,
        .framebuffer = { VK_NULL_HANDLE },
    };
    const VkClearValue clear_values[2] = {
        [0] = { .color.float32 = { 0.2f, 0.2f, 0.2f, 0.2f } },
        [1] = { .depthStencil = { demo->depthStencil, 0 } },
    };
    const VkRenderPassBeginInfo rp_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = demo->render_pass,
        .framebuffer = demo->framebuffers[demo->current_buffer],
        .renderArea.offset.x = 0,
        .renderArea.offset.y = 0,
        .renderArea.extent.width = demo->width,
        .renderArea.extent.height = demo->height,
        .clearValueCount = 2,
        .pClearValues = clear_values,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkBeginCommandBuffer(demo->draw_cmd, &cmd_buf_info);
    assert(!err);

    vkCmdBeginRenderPass(demo->draw_cmd, &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);
    vkCmdBindPipeline(demo->draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    vkCmdBindDescriptorSets(demo->draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline_layout,
            0, 1, & demo->desc_set, 0, NULL);

    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float) demo->height;
    viewport.width = (float) demo->width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    vkCmdSetViewport(demo->draw_cmd, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(demo->draw_cmd, 1, &scissor);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(demo->draw_cmd, VERTEX_BUFFER_BIND_ID, 1, &demo->vertices.buf, offsets);

    vkCmdDraw(demo->draw_cmd, 3, 1, 0, 0);
    vkCmdEndRenderPass(demo->draw_cmd);

    err = vkEndCommandBuffer(demo->draw_cmd);
    assert(!err);
}

static void demo_draw(struct demo *demo)
{
    VkResult U_ASSERT_ONLY err;
    VkSemaphore presentCompleteSemaphore;
    VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    err = vkCreateSemaphore(demo->device,
                            &presentCompleteSemaphoreCreateInfo,
                            &presentCompleteSemaphore);
    assert(!err);

    // Get the index of the next available swapchain image:
    err = demo->fpAcquireNextImageKHR(demo->device, demo->swap_chain,
                                      UINT64_MAX,
                                      presentCompleteSemaphore,
                                      &demo->current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(!err);

    // Wait for the present complete semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.
    vkQueueWaitSemaphore(demo->queue, presentCompleteSemaphore);

// FIXME/TODO: DEAL WITH VK_IMAGE_LAYOUT_PRESENT_SOURCE_KHR
    demo_draw_build_cmd(demo);
    VkFence nullFence = { VK_NULL_HANDLE };

    err = vkQueueSubmit(demo->queue, 1, &demo->draw_cmd, nullFence);
    assert(!err);

    VkPresentInfoKHR present = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = NULL,
        .swapchainCount = 1,
        .swapchains = &demo->swap_chain,
        .imageIndices = &demo->current_buffer,
    };

// TBD/TODO: SHOULD THE "present" PARAMETER BE "const" IN THE HEADER?
    err = demo->fpQueuePresentKHR(demo->queue, &present);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(!err);

    err = vkQueueWaitIdle(demo->queue);
    assert(err == VK_SUCCESS);

    vkDestroySemaphore(demo->device, presentCompleteSemaphore);
}

static void demo_prepare_buffers(struct demo *demo)
{
    VkResult U_ASSERT_ONLY err;

    // Check the surface proprties and formats
    VkSurfacePropertiesKHR surfProperties;
    err = demo->fpGetSurfacePropertiesKHR(demo->device,
        (const VkSurfaceDescriptionKHR *)&demo->surface_description,
        &surfProperties);
    assert(!err);

    uint32_t presentModeCount;
    err = demo->fpGetSurfacePresentModesKHR(demo->device,
        (const VkSurfaceDescriptionKHR *)&demo->surface_description,
        &presentModeCount, NULL);
    assert(!err);
    VkPresentModeKHR *presentModes =
        (VkPresentModeKHR *)malloc(presentModeCount * sizeof(VkPresentModeKHR));
    assert(presentModes);
    err = demo->fpGetSurfacePresentModesKHR(demo->device,
        (const VkSurfaceDescriptionKHR *)&demo->surface_description,
        &presentModeCount, presentModes);
    assert(!err);

    VkExtent2D swapchainExtent;
    // width and height are either both -1, or both not -1.
    if (surfProperties.currentExtent.width == -1)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = demo->width;
        swapchainExtent.height = demo->height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfProperties.currentExtent;
    }

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapchainImages = surfProperties.minImageCount + 1;
    if ((surfProperties.maxImageCount > 0) &&
        (desiredNumberOfSwapchainImages > surfProperties.maxImageCount))
    {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapchainImages = surfProperties.maxImageCount;
    }

    VkSurfaceTransformKHR preTransform;
    if (surfProperties.supportedTransforms & VK_SURFACE_TRANSFORM_NONE_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_NONE_KHR;
    } else {
        preTransform = surfProperties.currentTransform;
    }

    const VkSwapchainCreateInfoKHR swap_chain = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .pSurfaceDescription = (const VkSurfaceDescriptionKHR *)&demo->surface_description,
        .minImageCount = desiredNumberOfSwapchainImages,
        .imageFormat = demo->format,
        .imageColorSpace = demo->color_space,
        .imageExtent = {
            .width = swapchainExtent.width,
            .height = swapchainExtent.height,
        },
        .imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = preTransform,
        .imageArraySize = 1,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyCount = 0,
        .pQueueFamilyIndices = NULL,
        .presentMode = swapchainPresentMode,
        .oldSwapchain.handle = 0,
        .clipped = true,
    };
    uint32_t i;

    err = demo->fpCreateSwapchainKHR(demo->device, &swap_chain, &demo->swap_chain);
    assert(!err);

    err = demo->fpGetSwapchainImagesKHR(demo->device, demo->swap_chain,
                                        &demo->swapchainImageCount, NULL);
    assert(!err);

    VkImage* swapchainImages =
        (VkImage*)malloc(demo->swapchainImageCount * sizeof(VkImage));
    assert(swapchainImages);
    err = demo->fpGetSwapchainImagesKHR(demo->device, demo->swap_chain,
                                        &demo->swapchainImageCount,
                                        swapchainImages);
    assert(!err);

    demo->buffers = (SwapchainBuffers*)malloc(sizeof(SwapchainBuffers)*demo->swapchainImageCount);
    assert(demo->buffers);

    for (i = 0; i < demo->swapchainImageCount; i++) {
        VkImageViewCreateInfo color_attachment_view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = demo->format,
            .channels = {
                .r = VK_CHANNEL_SWIZZLE_R,
                .g = VK_CHANNEL_SWIZZLE_G,
                .b = VK_CHANNEL_SWIZZLE_B,
                .a = VK_CHANNEL_SWIZZLE_A,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .mipLevels = 1,
                .baseArrayLayer = 0,
                .arraySize = 1
            },
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .flags = 0,
        };

        demo->buffers[i].image = swapchainImages[i];

        demo_set_image_layout(demo, demo->buffers[i].image,
                               VK_IMAGE_ASPECT_COLOR_BIT,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        color_attachment_view.image = demo->buffers[i].image;

        err = vkCreateImageView(demo->device,
                &color_attachment_view, &demo->buffers[i].view);
        assert(!err);
    }

    demo->current_buffer = 0;
}

static void demo_prepare_depth(struct demo *demo)
{
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    const VkImageCreateInfo image = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = depth_format,
        .extent = { demo->width, demo->height, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .flags = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memoryTypeIndex = 0,
    };
    VkImageViewCreateInfo view = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image.handle = VK_NULL_HANDLE,
        .format = depth_format,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel = 0,
            .mipLevels = 1,
            .baseArrayLayer = 0,
            .arraySize = 1
        },
        .flags = 0,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
    };

    VkMemoryRequirements mem_reqs;
    VkResult U_ASSERT_ONLY err;

    demo->depth.format = depth_format;

    /* create image */
    err = vkCreateImage(demo->device, &image,
            &demo->depth.image);
    assert(!err);

    /* get memory requirements for this object */
    err = vkGetImageMemoryRequirements(demo->device, demo->depth.image,
                                       &mem_reqs);

    /* select memory size and type */
    mem_alloc.allocationSize = mem_reqs.size;
    err = memory_type_from_properties(demo,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_DEVICE_ONLY,
                                      &mem_alloc.memoryTypeIndex);
    assert(!err);

    /* allocate memory */
    err = vkAllocMemory(demo->device, &mem_alloc, &demo->depth.mem);
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory(demo->device, demo->depth.image,
                            demo->depth.mem, 0);
    assert(!err);

    demo_set_image_layout(demo, demo->depth.image,
                           VK_IMAGE_ASPECT_DEPTH_BIT,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    /* create image view */
    view.image = demo->depth.image;
    err = vkCreateImageView(demo->device, &view, &demo->depth.view);
    assert(!err);
}

static void demo_prepare_texture_image(struct demo *demo,
                                       const uint32_t *tex_colors,
                                       struct texture_object *tex_obj,
                                       VkImageTiling tiling,
                                       VkImageUsageFlags usage,
                                       VkFlags mem_props)
{
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t tex_width = 2;
    const int32_t tex_height = 2;
    VkResult U_ASSERT_ONLY err;

    tex_obj->tex_width = tex_width;
    tex_obj->tex_height = tex_height;

    const VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = tex_format,
        .extent = { tex_width, tex_height, 1 },
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = tiling,
        .usage = usage,
        .flags = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memoryTypeIndex = 0,
    };

    VkMemoryRequirements mem_reqs;

    err = vkCreateImage(demo->device, &image_create_info,
            &tex_obj->image);
    assert(!err);

    err = vkGetImageMemoryRequirements(demo->device, tex_obj->image, &mem_reqs);

    mem_alloc.allocationSize  = mem_reqs.size;
    err = memory_type_from_properties(demo, mem_reqs.memoryTypeBits, mem_props, &mem_alloc.memoryTypeIndex);
    assert(!err);

    /* allocate memory */
    err = vkAllocMemory(demo->device, &mem_alloc, &tex_obj->mem);
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory(demo->device, tex_obj->image,
            tex_obj->mem, 0);
    assert(!err);

    if (mem_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspect = VK_IMAGE_ASPECT_COLOR,
            .mipLevel = 0,
            .arrayLayer = 0,
        };
        VkSubresourceLayout layout;
        void *data;
        int32_t x, y;

        err = vkGetImageSubresourceLayout(demo->device, tex_obj->image, &subres, &layout);
        assert(!err);

        err = vkMapMemory(demo->device, tex_obj->mem, 0, mem_alloc.allocationSize, 0, &data);
        assert(!err);

        for (y = 0; y < tex_height; y++) {
            uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
            for (x = 0; x < tex_width; x++)
                row[x] = tex_colors[(x & 1) ^ (y & 1)];
        }

        vkUnmapMemory(demo->device, tex_obj->mem);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    demo_set_image_layout(demo, tex_obj->image,
                           VK_IMAGE_ASPECT_COLOR_BIT,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           tex_obj->imageLayout);
    /* setting the image layout does not reference the actual memory so no need to add a mem ref */
}

static void demo_destroy_texture_image(struct demo *demo, struct texture_object *tex_obj)
{
    /* clean up staging resources */
    vkDestroyImage(demo->device, tex_obj->image);
    vkFreeMemory(demo->device, tex_obj->mem);
}

static void demo_prepare_textures(struct demo *demo)
{
    const VkFormat tex_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkFormatProperties props;
    const uint32_t tex_colors[DEMO_TEXTURE_COUNT][2] = {
        { 0xffff0000, 0xff00ff00 },
    };
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    err = vkGetPhysicalDeviceFormatProperties(demo->gpu, tex_format, &props);
    assert(!err);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        if ((props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !demo->use_staging_buffer) {
            /* Device can texture using linear textures */
            demo_prepare_texture_image(demo, tex_colors[i], &demo->textures[i],
                                       VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT){
            /* Must use staging buffer to copy linear texture to optimized */
            struct texture_object staging_texture;

            memset(&staging_texture, 0, sizeof(staging_texture));
            demo_prepare_texture_image(demo, tex_colors[i], &staging_texture,
                                       VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

            demo_prepare_texture_image(demo, tex_colors[i], &demo->textures[i],
                                       VK_IMAGE_TILING_OPTIMAL,
                                       (VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
                                       VK_MEMORY_PROPERTY_DEVICE_ONLY);

            demo_set_image_layout(demo, staging_texture.image,
                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                   staging_texture.imageLayout,
                                   VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL);

            demo_set_image_layout(demo, demo->textures[i].image,
                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                   demo->textures[i].imageLayout,
                                   VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL);

            VkImageCopy copy_region = {
                .srcSubresource = { VK_IMAGE_ASPECT_COLOR, 0, 0 },
                .srcOffset = { 0, 0, 0 },
                .destSubresource = { VK_IMAGE_ASPECT_COLOR, 0, 0 },
                .destOffset = { 0, 0, 0 },
                .extent = { staging_texture.tex_width, staging_texture.tex_height, 1 },
            };
            vkCmdCopyImage(demo->setup_cmd,
                            staging_texture.image, VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL,
                            demo->textures[i].image, VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,
                            1, &copy_region);

            demo_set_image_layout(demo, demo->textures[i].image,
                                   VK_IMAGE_ASPECT_COLOR_BIT,
                                   VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,
                                   demo->textures[i].imageLayout);

            demo_flush_init_cmd(demo);

            demo_destroy_texture_image(demo, &staging_texture);
        } else {
            /* Can't support VK_FORMAT_B8G8R8A8_UNORM !? */
            assert(!"No support for B8G8R8A8_UNORM as texture image format");
        }

        const VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_TEX_FILTER_NEAREST,
            .minFilter = VK_TEX_FILTER_NEAREST,
            .mipMode = VK_TEX_MIPMAP_MODE_BASE,
            .addressModeU = VK_TEX_ADDRESS_MODE_WRAP,
            .addressModeV = VK_TEX_ADDRESS_MODE_WRAP,
            .addressModeW = VK_TEX_ADDRESS_MODE_WRAP,
            .mipLodBias = 0.0f,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE,
        };
        VkImageViewCreateInfo view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image.handle = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = tex_format,
            .channels = { VK_CHANNEL_SWIZZLE_R,
                          VK_CHANNEL_SWIZZLE_G,
                          VK_CHANNEL_SWIZZLE_B,
                          VK_CHANNEL_SWIZZLE_A, },
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            .flags = 0,
        };

        /* create sampler */
        err = vkCreateSampler(demo->device, &sampler,
                &demo->textures[i].sampler);
        assert(!err);

        /* create image view */
        view.image = demo->textures[i].image;
        err = vkCreateImageView(demo->device, &view,
                                 &demo->textures[i].view);
        assert(!err);
    }
}

static void demo_prepare_vertices(struct demo *demo)
{
    const float vb[3][5] = {
        /*      position             texcoord */
        { -1.0f, -1.0f,  0.2f,      0.0f, 0.0f },
        {  1.0f, -1.0f,  0.25f,     1.0f, 0.0f },
        {  0.0f,  1.0f,  1.0f,      0.5f, 1.0f },
    };
    const VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .size = sizeof(vb),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .flags = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memoryTypeIndex = 0,
    };
    VkMemoryRequirements mem_reqs;
    VkResult U_ASSERT_ONLY err;
    void *data;

    memset(&demo->vertices, 0, sizeof(demo->vertices));

    err = vkCreateBuffer(demo->device, &buf_info, &demo->vertices.buf);
    assert(!err);

    err = vkGetBufferMemoryRequirements(demo->device,
            demo->vertices.buf, &mem_reqs);

    mem_alloc.allocationSize  = mem_reqs.size;
    err = memory_type_from_properties(demo,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &mem_alloc.memoryTypeIndex);
    assert(!err);

    err = vkAllocMemory(demo->device, &mem_alloc, &demo->vertices.mem);
    assert(!err);

    err = vkMapMemory(demo->device, demo->vertices.mem, 0, mem_alloc.allocationSize, 0, &data);
    assert(!err);

    memcpy(data, vb, sizeof(vb));

    vkUnmapMemory(demo->device, demo->vertices.mem);

    err = vkBindBufferMemory(demo->device, demo->vertices.buf,
            demo->vertices.mem, 0);
    assert(!err);

    demo->vertices.vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    demo->vertices.vi.pNext = NULL;
    demo->vertices.vi.bindingCount = 1;
    demo->vertices.vi.pVertexBindingDescriptions = demo->vertices.vi_bindings;
    demo->vertices.vi.attributeCount = 2;
    demo->vertices.vi.pVertexAttributeDescriptions = demo->vertices.vi_attrs;

    demo->vertices.vi_bindings[0].binding = VERTEX_BUFFER_BIND_ID;
    demo->vertices.vi_bindings[0].strideInBytes = sizeof(vb[0]);
    demo->vertices.vi_bindings[0].stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX;

    demo->vertices.vi_attrs[0].binding = VERTEX_BUFFER_BIND_ID;
    demo->vertices.vi_attrs[0].location = 0;
    demo->vertices.vi_attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    demo->vertices.vi_attrs[0].offsetInBytes = 0;

    demo->vertices.vi_attrs[1].binding = VERTEX_BUFFER_BIND_ID;
    demo->vertices.vi_attrs[1].location = 1;
    demo->vertices.vi_attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
    demo->vertices.vi_attrs[1].offsetInBytes = sizeof(float) * 3;
}

static void demo_prepare_descriptor_layout(struct demo *demo)
{
    const VkDescriptorSetLayoutBinding layout_binding = {
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .arraySize = DEMO_TEXTURE_COUNT,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = NULL,
    };
    const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &layout_binding,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorSetLayout(demo->device,
            &descriptor_layout, &demo->desc_layout);
    assert(!err);

    const VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext              = NULL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &demo->desc_layout,
    };

    err = vkCreatePipelineLayout(demo->device,
                                 &pPipelineLayoutCreateInfo,
                                 &demo->pipeline_layout);
    assert(!err);
}

static void demo_prepare_render_pass(struct demo *demo)
{
    const VkAttachmentDescription attachments[2] = {
        [0] = {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION,
            .pNext = NULL,
            .format = demo->format,
            .samples = 1,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
        [1] = {
            .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION,
            .pNext = NULL,
            .format = demo->depth.format,
            .samples = 1,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };
    const VkAttachmentReference color_reference = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkSubpassDescription subpass = {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION,
        .pNext = NULL,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .flags = 0,
        .inputCount = 0,
        .pInputAttachments = NULL,
        .colorCount = 1,
        .pColorAttachments = &color_reference,
        .pResolveAttachments = NULL,
        .depthStencilAttachment = {
            .attachment = 1,
            .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
        .preserveCount = 0,
        .pPreserveAttachments = NULL,
    };
    const VkRenderPassCreateInfo rp_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = NULL,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 0,
        .pDependencies = NULL,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateRenderPass(demo->device, &rp_info, &demo->render_pass);
    assert(!err);
}

static VkShader demo_prepare_shader(struct demo *demo,
                                      VkShaderStage stage,
                                      VkShaderModule* pShaderModule,
                                      const void *code,
                                      size_t size)
{
    VkShaderModuleCreateInfo moduleCreateInfo;
    VkShaderCreateInfo shaderCreateInfo;
    VkShader shader;
    VkResult U_ASSERT_ONLY err;


    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;

    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    shaderCreateInfo.pNext = NULL;

    if (!demo->use_glsl) {
        moduleCreateInfo.codeSize = size;
        moduleCreateInfo.pCode = code;
        moduleCreateInfo.flags = 0;
        err = vkCreateShaderModule(demo->device, &moduleCreateInfo, pShaderModule);
        assert(!err);

        shaderCreateInfo.flags = 0;
        shaderCreateInfo.module = *pShaderModule;
        shaderCreateInfo.pName = "main";
        shaderCreateInfo.stage = stage;
        err = vkCreateShader(demo->device, &shaderCreateInfo, &shader);
        assert(!err);
    } else {
        // Create fake SPV structure to feed GLSL
        // to the driver "under the covers"
        moduleCreateInfo.codeSize = 3 * sizeof(uint32_t) + size + 1;
        moduleCreateInfo.pCode = malloc(moduleCreateInfo.codeSize);
        moduleCreateInfo.flags = 0;

        /* try version 0 first: VkShaderStage followed by GLSL */
        ((uint32_t *) moduleCreateInfo.pCode)[0] = ICD_SPV_MAGIC;
        ((uint32_t *) moduleCreateInfo.pCode)[1] = 0;
        ((uint32_t *) moduleCreateInfo.pCode)[2] = stage;
        memcpy(((uint32_t *) moduleCreateInfo.pCode + 3), code, size + 1);

        err = vkCreateShaderModule(demo->device, &moduleCreateInfo, pShaderModule);
        assert(!err);

        shaderCreateInfo.flags = 0;
        shaderCreateInfo.module = *pShaderModule;
        shaderCreateInfo.pName = "main";
        shaderCreateInfo.stage = stage;
        err = vkCreateShader(demo->device, &shaderCreateInfo, &shader);
        assert(!err);
        free((void *) moduleCreateInfo.pCode);
    }
    return shader;
}

char *demo_read_spv(const char *filename, size_t *psize)
{
    long int size;
    void *shader_code;
    size_t retVal;

    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    shader_code = malloc(size);
    retVal = fread(shader_code, size, 1, fp);
    if (!retVal) return NULL;

    *psize = size;

    return shader_code;
}

static VkShader demo_prepare_vs(struct demo *demo)
{
    if (!demo->use_glsl) {
        VkShader shader;
        void *vertShaderCode;
        size_t size;

        vertShaderCode = demo_read_spv("tri-vert.spv", &size);

        shader = demo_prepare_shader(demo, VK_SHADER_STAGE_VERTEX,
                                     &demo->vert_shader_module,
                                     vertShaderCode, size);
        free(vertShaderCode);
        return shader;
    } else {
        static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 1) in vec2 attr;\n"
            "out vec2 texcoord;\n"
            "void main() {\n"
            "   texcoord = attr;\n"
            "   gl_Position = pos;\n"
            "}\n";

        return demo_prepare_shader(demo, VK_SHADER_STAGE_VERTEX,
                                   &demo->vert_shader_module,
                                   (const void *) vertShaderText,
                                   strlen(vertShaderText));
    }
}

static VkShader demo_prepare_fs(struct demo *demo)
{
    if (!demo->use_glsl) {
        VkShader shader;
        void *fragShaderCode;
        size_t size;

        fragShaderCode = demo_read_spv("tri-frag.spv", &size);

        shader = demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT,
                                     &demo->frag_shader_module,
                                     fragShaderCode, size);

        free(fragShaderCode);
        return shader;
    } else {
        static const char *fragShaderText =
                "#version 140\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "#extension GL_ARB_shading_language_420pack : enable\n"
                "layout (binding = 0) uniform sampler2D tex;\n"
                "layout (location = 0) in vec2 texcoord;\n"
                "layout (location = 0) out vec4 uFragColor;\n"
                "void main() {\n"
                "   uFragColor = texture(tex, texcoord);\n"
                "}\n";

        return demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT,
                                   &demo->frag_shader_module,
                                   (const void *) fragShaderText,
                                   strlen(fragShaderText));
    }
}

static void demo_prepare_pipeline(struct demo *demo)
{
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;

    VkPipelineVertexInputStateCreateInfo   vi;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterStateCreateInfo        rs;
    VkPipelineColorBlendStateCreateInfo    cb;
    VkPipelineDepthStencilStateCreateInfo  ds;
    VkPipelineViewportStateCreateInfo      vp;
    VkPipelineMultisampleStateCreateInfo   ms;
    VkDynamicState                         dynamicStateEnables[VK_DYNAMIC_STATE_NUM];
    VkPipelineDynamicStateCreateInfo       dynamicState;

    VkResult U_ASSERT_ONLY err;

    memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
    memset(&dynamicState, 0, sizeof dynamicState);
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = demo->pipeline_layout;

    vi = demo->vertices.vi;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTER_STATE_CREATE_INFO;
    rs.fillMode = VK_FILL_MODE_SOLID;
    rs.cullMode = VK_CULL_MODE_BACK;
    rs.frontFace = VK_FRONT_FACE_CW;
    rs.depthClipEnable = VK_TRUE;
    rs.rasterizerDiscardEnable = VK_FALSE;
    rs.depthBiasEnable = VK_FALSE;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkPipelineColorBlendAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    vp.scissorCount = 1;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_EQUAL;
    ds.depthBoundsTestEnable = VK_FALSE;
    ds.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pSampleMask = NULL;
    ms.rasterSamples = 1;

    // Two stages: vs and fs
    pipeline.stageCount = 2;
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    shaderStages[0].sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage                = VK_SHADER_STAGE_VERTEX;
    shaderStages[0].shader               = demo_prepare_vs(demo);

    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT;
    shaderStages[1].shader = demo_prepare_fs(demo);

    pipeline.pVertexInputState   = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterState        = &rs;
    pipeline.pColorBlendState    = &cb;
    pipeline.pMultisampleState   = &ms;
    pipeline.pViewportState      = &vp;
    pipeline.pDepthStencilState  = &ds;
    pipeline.pStages             = shaderStages;
    pipeline.renderPass          = demo->render_pass;
    pipeline.pDynamicState       = &dynamicState;

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache(demo->device, &pipelineCache, &demo->pipelineCache);
    assert(!err);
    err = vkCreateGraphicsPipelines(demo->device, demo->pipelineCache, 1, &pipeline, &demo->pipeline);
    assert(!err);

    vkDestroyPipelineCache(demo->device, demo->pipelineCache);

    for (uint32_t i = 0; i < pipeline.stageCount; i++) {
        vkDestroyShader(demo->device, shaderStages[i].shader);
    }
    vkDestroyShaderModule(demo->device, demo->frag_shader_module);
    vkDestroyShaderModule(demo->device, demo->vert_shader_module);
}

static void demo_prepare_descriptor_pool(struct demo *demo)
{
    const VkDescriptorTypeCount type_count = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .count = DEMO_TEXTURE_COUNT,
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .poolUsage = VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT,
        .maxSets = 1,
        .count = 1,
        .pTypeCount = &type_count,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorPool(demo->device,
            &descriptor_pool, &demo->desc_pool);
    assert(!err);
}

static void demo_prepare_descriptor_set(struct demo *demo)
{
    VkDescriptorInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet write;
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    err = vkAllocDescriptorSets(demo->device, demo->desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            1, &demo->desc_layout,
            &demo->desc_set);
    assert(!err);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        tex_descs[i].sampler = demo->textures[i].sampler;
        tex_descs[i].imageView = demo->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    memset(&write, 0, sizeof(write));
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.destSet = demo->desc_set;
    write.count = DEMO_TEXTURE_COUNT;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pDescriptors = tex_descs;

    vkUpdateDescriptorSets(demo->device, 1, &write, 0, NULL);
}

static void demo_prepare_framebuffers(struct demo *demo)
{
    VkImageView attachments[2];
    attachments[1] = demo->depth.view;

    const VkFramebufferCreateInfo fb_info = {
         .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .pNext = NULL,
         .renderPass = demo->render_pass,
         .attachmentCount = 2,
         .pAttachments = attachments,
         .width  = demo->width,
         .height = demo->height,
         .layers = 1,
    };
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        attachments[0]= demo->buffers[i].view;
        err = vkCreateFramebuffer(demo->device, &fb_info, &demo->framebuffers[i]);
        assert(!err);
    }
}

static void demo_prepare(struct demo *demo)
{
    VkResult U_ASSERT_ONLY err;

    const VkCmdPoolCreateInfo cmd_pool_info = {
        .sType = VK_STRUCTURE_TYPE_CMD_POOL_CREATE_INFO,
        .pNext = NULL,
        .queueFamilyIndex = demo->graphics_queue_node_index,
        .flags = 0,
    };
    err = vkCreateCommandPool(demo->device, &cmd_pool_info, &demo->cmd_pool);
    assert(!err);

    const VkCmdBufferCreateInfo cmd = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .cmdPool = demo->cmd_pool,
        .level = VK_CMD_BUFFER_LEVEL_PRIMARY,
        .flags = 0,
    };
    err = vkCreateCommandBuffer(demo->device, &cmd, &demo->draw_cmd);
    assert(!err);

    demo_prepare_buffers(demo);
    demo_prepare_depth(demo);
    demo_prepare_textures(demo);
    demo_prepare_vertices(demo);
    demo_prepare_descriptor_layout(demo);
    demo_prepare_render_pass(demo);
    demo_prepare_pipeline(demo);

    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);

    demo_prepare_framebuffers(demo);

    demo->prepared = true;
}

#ifdef _WIN32
static void demo_run(struct demo *demo)
{
    if (!demo->prepared)
        return;
    demo_draw(demo);

    if (demo->depthStencil > 0.99f)
        demo->depthIncrement = -0.001f;
    if (demo->depthStencil < 0.8f)
        demo->depthIncrement = 0.001f;

    demo->depthStencil += demo->depthIncrement;
}

// On MS-Windows, make this a global, so it's available to WndProc()
struct demo demo;

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    char tmp_str[] = APP_LONG_NAME;

    switch(uMsg)
    {
    case WM_CREATE:
        return 0;
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
        if (demo.prepared) {
            demo_run(&demo);
            return 0;
        }
    default:
        break;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

static void demo_create_window(struct demo *demo)
{
    WNDCLASSEX  win_class;

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = demo->connection; // hInstance
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = demo->name;
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    // Register window class:
    if (!RegisterClassEx(&win_class)) {
        // It didn't work, so try to give a useful error:
        printf("Unexpected error trying to start the application!\n");
        fflush(stdout);
        exit(1);
    }
    // Create window with the registered class:
    RECT wr = { 0, 0, demo->width, demo->height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    demo->window = CreateWindowEx(0,
                                  demo->name,           // class name
                                  demo->name,           // app name
                                  WS_OVERLAPPEDWINDOW | // window style
                                  WS_VISIBLE |
                                  WS_SYSMENU,
                                  100,100,              // x/y coords
                                  wr.right-wr.left,     // width
                                  wr.bottom-wr.top,     // height
                                  NULL,                 // handle to parent
                                  NULL,                 // handle to menu
                                  demo->connection,     // hInstance
                                  NULL);                // no extra parameters
    if (!demo->window) {
        // It didn't work, so try to give a useful error:
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }
}
#else  // _WIN32

static void demo_handle_event(struct demo *demo,
                              const xcb_generic_event_t *event)
{
    switch (event->response_type & 0x7f) {
    case XCB_EXPOSE:
        demo_draw(demo);
        break;
    case XCB_CLIENT_MESSAGE:
        if((*(xcb_client_message_event_t*)event).data.data32[0] ==
           (*demo->atom_wm_delete_window).atom) {
            demo->quit = true;
        }
        break;
    case XCB_KEY_RELEASE:
        {
            const xcb_key_release_event_t *key =
                (const xcb_key_release_event_t *) event;

            if (key->detail == 0x9)
                demo->quit = true;
        }
        break;
    case XCB_DESTROY_NOTIFY:
        demo->quit = true;
        break;
    default:
        break;
    }
}

static void demo_run(struct demo *demo)
{
    xcb_flush(demo->connection);

    while (!demo->quit) {
        xcb_generic_event_t *event;

        event = xcb_poll_for_event(demo->connection);
        if (event) {
            demo_handle_event(demo, event);
            free(event);
        }

        demo_draw(demo);

        if (demo->depthStencil > 0.99f)
            demo->depthIncrement = -0.001f;
        if (demo->depthStencil < 0.8f)
            demo->depthIncrement = 0.001f;

        demo->depthStencil += demo->depthIncrement;

        // Wait for work to finish before updating MVP.
        vkDeviceWaitIdle(demo->device);
    }
}

static void demo_create_window(struct demo *demo)
{
    uint32_t value_mask, value_list[32];

    demo->window = xcb_generate_id(demo->connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = demo->screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_EXPOSURE |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(demo->connection,
            XCB_COPY_FROM_PARENT,
            demo->window, demo->screen->root,
            0, 0, demo->width, demo->height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            demo->screen->root_visual,
            value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(demo->connection, 1, 12,
                                                      "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(demo->connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(demo->connection, 0, 16, "WM_DELETE_WINDOW");
    demo->atom_wm_delete_window = xcb_intern_atom_reply(demo->connection, cookie2, 0);

    xcb_change_property(demo->connection, XCB_PROP_MODE_REPLACE,
                        demo->window, (*reply).atom, 4, 32, 1,
                        &(*demo->atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(demo->connection, demo->window);
}
#endif // _WIN32

/*
 * Return 1 (true) if all layer names specified in check_names
 * can be found in given layer properties.
 */
static VkBool32 demo_check_layers(uint32_t check_count, char **check_names,
                              uint32_t layer_count, VkLayerProperties *layers)
{
    for (uint32_t i = 0; i < check_count; i++) {
        VkBool32 found = 0;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(check_names[i], layers[j].layerName)) {
                found = 1;
            }
        }
        if (!found) {
            fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
            return 0;
        }
    }
    return 1;
}

void* VKAPI myalloc(
    void*                           pUserData,
    size_t                          size,
    size_t                          alignment,
    VkSystemAllocType               allocType)
{
    return malloc(size);
}
void VKAPI myfree(
    void*                           pUserData,
    void*                           pMem)
{
    free(pMem);
}
static void demo_init_vk(struct demo *demo)
{
    VkResult err;
    char *extension_names[64];
    char *layer_names[64];
    VkExtensionProperties *instance_extensions;
    VkPhysicalDevice *physical_devices;
    VkLayerProperties *instance_layers;
    VkLayerProperties *device_layers;
    uint32_t instance_extension_count = 0;
    uint32_t instance_layer_count = 0;
    uint32_t enabled_extension_count = 0;
    uint32_t enabled_layer_count = 0;

    char *instance_validation_layers[] = {
        "MemTracker",
    };

    char *device_validation_layers[] = {
        "MemTracker",
    };

    /* Look for validation layers */
    VkBool32 validation_found = 0;
    err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
    assert(!err);

    instance_layers = malloc(sizeof(VkLayerProperties) * instance_layer_count);
    err = vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layers);
    assert(!err);

    if (demo->validate) {
        validation_found = demo_check_layers(ARRAY_SIZE(instance_validation_layers), instance_validation_layers,
                                             instance_layer_count, instance_layers);
        if (!validation_found) {
            ERR_EXIT("vkEnumerateInstanceLayerProperties failed to find"
                     "required validation layer.\n\n"
                     "Please look at the Getting Started guide for additional "
                     "information.\n",
                     "vkCreateInstance Failure");
        }
        enabled_layer_count = ARRAY_SIZE(instance_validation_layers);
    }

    err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);
    assert(!err);

    VkBool32 swapchainExtFound = 0;
    memset(extension_names, 0, sizeof(extension_names));
    instance_extensions = malloc(sizeof(VkExtensionProperties) * instance_extension_count);
    err = vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions);
    assert(!err);
    for (uint32_t i = 0; i < instance_extension_count; i++) {
        if (!strcmp("VK_EXT_KHR_swapchain", instance_extensions[i].extName)) {
            swapchainExtFound = 1;
            extension_names[enabled_extension_count++] = "VK_EXT_KHR_swapchain";
        }
        if (!strcmp(VK_DEBUG_REPORT_EXTENSION_NAME, instance_extensions[i].extName)) {
            if (demo->validate) {
                extension_names[enabled_extension_count++] = VK_DEBUG_REPORT_EXTENSION_NAME;
            }
        }
        assert(enabled_extension_count < 64);
    }
    if (!swapchainExtFound) {
        ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the "
                 "\"VK_EXT_KHR_swapchain\" extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = APP_SHORT_NAME,
        .appVersion = 0,
        .pEngineName = APP_SHORT_NAME,
        .engineVersion = 0,
        .apiVersion = VK_API_VERSION,
    };
    VkAllocCallbacks cb = {
        .pUserData = NULL,
        .pfnAlloc = myalloc,
        .pfnFree = myfree,
    };
    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pAppInfo = &app,
        .pAllocCb = &cb,
        .layerCount = enabled_layer_count,
        .ppEnabledLayerNames = (const char *const*) layer_names,
        .extensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const*) extension_names,
    };
    const VkDeviceQueueCreateInfo queue = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = NULL,
        .queueFamilyIndex = 0,
        .queueCount = 1,
    };
    uint32_t gpu_count;

    err = vkCreateInstance(&inst_info, &demo->inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        ERR_EXIT("Cannot find a compatible Vulkan installable client driver "
                 "(ICD).\n\nPlease look at the Getting Started guide for "
                 "additional information.\n",
                 "vkCreateInstance Failure");
    } else if (err == VK_ERROR_EXTENSION_NOT_PRESENT) {
        ERR_EXIT("Cannot find a specified extension library"
                 ".\nMake sure your layers path is set appropriately\n",
                 "vkCreateInstance Failure");
    } else if (err) {
        ERR_EXIT("vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
                 "installable client driver (ICD) installed?\nPlease look at "
                 "the Getting Started guide for additional information.\n",
                 "vkCreateInstance Failure");
    }

    free(instance_layers);
    free(instance_extensions);

    /* Make initial call to query gpu_count, then second call for gpu info*/
    err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, NULL);
    assert(!err && gpu_count > 0);
    physical_devices = malloc(sizeof(VkPhysicalDevice) * gpu_count);
    err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, physical_devices);
    assert(!err);
    /* For tri demo we just grab the first physical device */
    demo->gpu = physical_devices[0];
    free(physical_devices);

    /* Look for validation layers */
    validation_found = 0;
    enabled_layer_count = 0;
    uint32_t device_layer_count = 0;
    err = vkEnumerateDeviceLayerProperties(demo->gpu, &device_layer_count, NULL);
    assert(!err);

    device_layers = malloc(sizeof(VkLayerProperties) * device_layer_count);
    err = vkEnumerateDeviceLayerProperties(demo->gpu, &device_layer_count, device_layers);
    assert(!err);

    if (demo->validate) {
        validation_found = demo_check_layers(ARRAY_SIZE(device_validation_layers), device_validation_layers,
                                             device_layer_count, device_layers);
        if (!validation_found) {
            ERR_EXIT("vkEnumerateDeviceLayerProperties failed to find"
                     "a required validation layer.\n\n"
                     "Please look at the Getting Started guide for additional "
                     "information.\n",
                     "vkCreateDevice Failure");
        }
        enabled_layer_count = ARRAY_SIZE(device_validation_layers);
    }

    uint32_t device_extension_count = 0;
    VkExtensionProperties *device_extensions = NULL;
    err = vkEnumerateDeviceExtensionProperties(
              demo->gpu, NULL, &device_extension_count, NULL);
    assert(!err);

    swapchainExtFound = 0;
    enabled_extension_count = 0;
    memset(extension_names, 0, sizeof(extension_names));
    device_extensions = malloc(sizeof(VkExtensionProperties) * device_extension_count);
    err = vkEnumerateDeviceExtensionProperties(
              demo->gpu, NULL, &device_extension_count, device_extensions);
    assert(!err);

    for (uint32_t i = 0; i < device_extension_count; i++) {
        if (!strcmp("VK_EXT_KHR_device_swapchain", device_extensions[i].extName)) {
            swapchainExtFound = 1;
            extension_names[enabled_extension_count++] = "VK_EXT_KHR_device_swapchain";
        }
        assert(enabled_extension_count < 64);
    }
    if (!swapchainExtFound) {
        ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find the "
                 "\"VK_EXT_KHR_device_swapchain\" extension.\n\nDo you have a compatible "
                 "Vulkan installable client driver (ICD) installed?\nPlease "
                 "look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }

    VkDeviceCreateInfo device = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 1,
        .pRequestedQueues = &queue,
        .layerCount = enabled_layer_count,
        .ppEnabledLayerNames = (const char *const*) ((demo->validate) ? device_validation_layers : NULL),
        .extensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const*) extension_names,
    };

    if (demo->validate) {
        demo->dbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback) vkGetInstanceProcAddr(demo->inst, "vkDbgCreateMsgCallback");
        if (!demo->dbgCreateMsgCallback) {
            ERR_EXIT("GetProcAddr: Unable to find vkDbgCreateMsgCallback\n",
                     "vkGetProcAddr Failure");
        }
        err = demo->dbgCreateMsgCallback(
                  demo->inst,
                  VK_DBG_REPORT_ERROR_BIT | VK_DBG_REPORT_WARN_BIT,
                  dbgFunc, NULL,
                  &demo->msg_callback);
        switch (err) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            ERR_EXIT("dbgCreateMsgCallback: out of host memory\n",
                     "dbgCreateMsgCallback Failure");
            break;
        default:
            ERR_EXIT("dbgCreateMsgCallback: unknown failure\n",
                     "dbgCreateMsgCallback Failure");
            break;
        }
    }


    err = vkCreateDevice(demo->gpu, &device, &demo->device);
    assert(!err);

    GET_INSTANCE_PROC_ADDR(demo->inst, GetPhysicalDeviceSurfaceSupportKHR);
    GET_DEVICE_PROC_ADDR(demo->device, GetSurfacePropertiesKHR);
    GET_DEVICE_PROC_ADDR(demo->device, GetSurfaceFormatsKHR);
    GET_DEVICE_PROC_ADDR(demo->device, GetSurfacePresentModesKHR);
    GET_DEVICE_PROC_ADDR(demo->device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(demo->device, CreateSwapchainKHR);
    GET_DEVICE_PROC_ADDR(demo->device, DestroySwapchainKHR);
    GET_DEVICE_PROC_ADDR(demo->device, GetSwapchainImagesKHR);
    GET_DEVICE_PROC_ADDR(demo->device, AcquireNextImageKHR);
    GET_DEVICE_PROC_ADDR(demo->device, QueuePresentKHR);

    err = vkGetPhysicalDeviceProperties(demo->gpu, &demo->gpu_props);

    // Query with NULL data to get count
    err = vkGetPhysicalDeviceQueueFamilyProperties(demo->gpu, &demo->queue_count, NULL);
    assert(!err);

    demo->queue_props = (VkQueueFamilyProperties *) malloc(demo->queue_count * sizeof(VkQueueFamilyProperties));
    err = vkGetPhysicalDeviceQueueFamilyProperties(demo->gpu, &demo->queue_count, demo->queue_props);
    assert(!err);
    assert(demo->queue_count >= 1);

    // Graphics queue and MemMgr queue can be separate.
    // TODO: Add support for separate queues, including synchronization,
    //       and appropriate tracking for QueueSubmit
}

static void demo_init_vk_swapchain(struct demo *demo)
{
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    // Construct the surface description:
    demo->surface_description.sType = VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_KHR;
    demo->surface_description.pNext = NULL;
#ifdef _WIN32
    demo->surface_description.platform = VK_PLATFORM_WIN32_KHR;
    demo->surface_description.pPlatformHandle = demo->connection;
    demo->surface_description.pPlatformWindow = demo->window;
#else  // _WIN32
    demo->platform_handle_xcb.connection = demo->connection;
    demo->platform_handle_xcb.root = demo->screen->root;
    demo->surface_description.platform = VK_PLATFORM_XCB_KHR;
    demo->surface_description.pPlatformHandle = &demo->platform_handle_xcb;
    demo->surface_description.pPlatformWindow = &demo->window;
#endif // _WIN32

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32* supportsPresent = (VkBool32 *)malloc(demo->queue_count * sizeof(VkBool32));
    for (i = 0; i < demo->queue_count; i++) {
        demo->fpGetPhysicalDeviceSurfaceSupportKHR(demo->gpu, i,
                                                   (VkSurfaceDescriptionKHR *) &demo->surface_description,
                                                   &supportsPresent[i]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex  = UINT32_MAX;
    for (i = 0; i < demo->queue_count; i++) {
        if ((demo->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
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
        for (uint32_t i = 0; i < demo->queue_count; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    free(supportsPresent);

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) {
        ERR_EXIT("Could not find a graphics and a present queue\n",
                 "Swapchain Initialization Failure");
    }

    // TODO: Add support for separate queues, including presentation,
    //       synchronization, and appropriate tracking for QueueSubmit
    // While it is possible for an application to use a separate graphics and a
    // present queues, this demo program assumes it is only using one:
    if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
        ERR_EXIT("Could not find a common graphics and a present queue\n",
                 "Swapchain Initialization Failure");
    }

    demo->graphics_queue_node_index = graphicsQueueNodeIndex;

    err = vkGetDeviceQueue(demo->device, demo->graphics_queue_node_index,
            0, &demo->queue);
    assert(!err);

    // Get the list of VkFormat's that are supported:
    uint32_t formatCount;
    err = demo->fpGetSurfaceFormatsKHR(demo->device,
                                    (VkSurfaceDescriptionKHR *) &demo->surface_description,
                                    &formatCount, NULL);
    assert(!err);
    VkSurfaceFormatKHR *surfFormats =
        (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    err = demo->fpGetSurfaceFormatsKHR(demo->device,
                                    (VkSurfaceDescriptionKHR *) &demo->surface_description,
                                    &formatCount, surfFormats);
    assert(!err);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        demo->format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        assert(formatCount >= 1);
        demo->format = surfFormats[0].format;
    }
    demo->color_space = surfFormats[0].colorSpace;

    // Get Memory information and properties
    err = vkGetPhysicalDeviceMemoryProperties(demo->gpu, &demo->memory_properties);
    assert(!err);
}

static void demo_init_connection(struct demo *demo)
{
#ifndef _WIN32
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    demo->connection = xcb_connect(NULL, &scr);
    if (demo->connection == NULL) {
        printf("Cannot find a compatible Vulkan installable client driver "
               "(ICD).\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    setup = xcb_get_setup(demo->connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    demo->screen = iter.data;
#endif // _WIN32
}

#ifdef _WIN32
static void demo_init(struct demo *demo, HINSTANCE hInstance, LPSTR pCmdLine)
#else  // _WIN32
static void demo_init(struct demo *demo, const int argc, const char *argv[])
#endif // _WIN32
{
    bool argv_error = false;

    memset(demo, 0, sizeof(*demo));

#ifdef _WIN32
    demo->connection = hInstance;
    strncpy(demo->name, APP_SHORT_NAME, APP_NAME_STR_LEN);

    if (strncmp(pCmdLine, "--use_staging", strlen("--use_staging")) == 0)
        demo->use_staging_buffer = true;
    else if (strncmp(pCmdLine, "--use_glsl", strlen("--use_glsl")) == 0)
        demo->use_glsl = true;
    else if (strlen(pCmdLine) != 0) {
        fprintf(stderr, "Do not recognize argument \"%s\".\n", pCmdLine);
        argv_error = true;
    }
#else  // _WIN32
    for (int i = 0; i < argc; i++) {
        if (strncmp(argv[i], "--use_staging", strlen("--use_staging")) == 0)
            demo->use_staging_buffer = true;
        else if (strncmp(argv[i], "--use_glsl", strlen("--use_glsl")) == 0)
            demo->use_glsl = true;
    }
#endif // _WIN32
    if (argv_error) {
        fprintf(stderr, "Usage:\n  %s [--use_staging]\n", APP_SHORT_NAME);
        fflush(stderr);
        exit(1);
    }

    demo_init_connection(demo);
    demo_init_vk(demo);

    demo->width = 300;
    demo->height = 300;
    demo->depthStencil = 1.0;
    demo->depthIncrement = -0.01f;
}

static void demo_cleanup(struct demo *demo)
{
    uint32_t i;

    demo->prepared = false;

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        vkDestroyFramebuffer(demo->device, demo->framebuffers[i]);
    }
    vkDestroyDescriptorPool(demo->device, demo->desc_pool);

    if (demo->setup_cmd) {
        vkDestroyCommandBuffer(demo->device, demo->setup_cmd);
    }
    vkDestroyCommandBuffer(demo->device, demo->draw_cmd);
    vkDestroyCommandPool(demo->device, demo->cmd_pool);

    vkDestroyPipeline(demo->device, demo->pipeline);
    vkDestroyRenderPass(demo->device, demo->render_pass);
    vkDestroyPipelineLayout(demo->device, demo->pipeline_layout);
    vkDestroyDescriptorSetLayout(demo->device, demo->desc_layout);

    vkDestroyBuffer(demo->device, demo->vertices.buf);
    vkFreeMemory(demo->device, demo->vertices.mem);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(demo->device, demo->textures[i].view);
        vkDestroyImage(demo->device, demo->textures[i].image);
        vkFreeMemory(demo->device, demo->textures[i].mem);
        vkDestroySampler(demo->device, demo->textures[i].sampler);
    }

    for (i = 0; i < demo->swapchainImageCount; i++) {
        vkDestroyImageView(demo->device, demo->buffers[i].view);
    }

    vkDestroyImageView(demo->device, demo->depth.view);
    vkDestroyImage(demo->device, demo->depth.image);
    vkFreeMemory(demo->device, demo->depth.mem);

    demo->fpDestroySwapchainKHR(demo->device, demo->swap_chain);
    free(demo->buffers);

    vkDestroyDevice(demo->device);
    vkDestroyInstance(demo->inst);

    free(demo->queue_props);

#ifndef _WIN32
    xcb_destroy_window(demo->connection, demo->window);
    xcb_disconnect(demo->connection);
    free(demo->atom_wm_delete_window);
#endif // _WIN32
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR pCmdLine,
                     int nCmdShow)
{
    MSG msg;         // message
    bool done;        // flag saying when app is complete

    demo_init(&demo, hInstance, pCmdLine);
    demo_create_window(&demo);
    demo_init_vk_swapchain(&demo);

    demo_prepare(&demo);

    done = false; //initialize loop condition variable
    /* main message loop*/
    while(!done)
    {
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (msg.message == WM_QUIT) //check for a quit message
        {
            done = true; //if found, quit app
        }
        else
        {
            /* Translate and dispatch to event queue*/
            TranslateMessage(&msg); 
            DispatchMessage(&msg);
        }
    }

    demo_cleanup(&demo);

    return (int) msg.wParam;
}
#else  // _WIN32
int main(const int argc, const char *argv[])
{
    struct demo demo;

    demo_init(&demo, argc, argv);
    demo_create_window(&demo);
    demo_init_vk_swapchain(&demo);

    demo_prepare(&demo);
    demo_run(&demo);

    demo_cleanup(&demo);

    return 0;
}
#endif // _WIN32
