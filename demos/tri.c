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
#include <vk_wsi_lunarg.h>
#include "vk_debug_report_lunarg.h"

#include "icd-spv.h"

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1
#define VERTEX_BUFFER_BIND_ID 0
#define APP_SHORT_NAME "tri"
#define APP_LONG_NAME "The Vulkan Triangle Demo Program"

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

// NOTE: If the following values (copied from "loader_platform.h") change, they
// need to change here as well:
#define LAYER_NAMES_ENV "VK_LAYER_NAMES"
#define LAYER_NAMES_REGISTRY_VALUE "VK_LAYER_NAMES"
#else  // _WIN32

#define ERR_EXIT(err_msg, err_class)                    \
    do {                                                \
        printf(err_msg);                                \
        fflush(stdout);                                 \
        exit(1);                                        \
   } while (0)
#endif // _WIN32

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
    demo->fp##entrypoint = vkGetDeviceProcAddr(dev, "vk"#entrypoint);   \
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

void dbgFunc(
    VkFlags                    msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
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
        return;
    }

#ifdef _WIN32
    MessageBox(NULL, message, "Alert", MB_OK);
#else
    printf("%s\n",message);
    fflush(stdout);
#endif
    free(message);
}

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
#endif // _WIN32
	bool prepared;
    bool use_staging_buffer;
    bool use_glsl;

    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    VkPhysicalDeviceProperties gpu_props;
    VkPhysicalDeviceQueueProperties *queue_props;
    uint32_t graphics_queue_node_index;

    int width, height;
    VkFormat format;

    PFN_vkCreateSwapChainWSI fpCreateSwapChainWSI;
    PFN_vkDestroySwapChainWSI fpDestroySwapChainWSI;
    PFN_vkGetSwapChainInfoWSI fpGetSwapChainInfoWSI;
    PFN_vkQueuePresentWSI fpQueuePresentWSI;

    VkSwapChainWSI swap_chain;
    struct {
        VkImage image;
        VkDeviceMemory mem;

        VkColorAttachmentView view;
    } buffers[DEMO_BUFFER_COUNT];

    struct {
        VkFormat format;

        VkImage image;
        VkDeviceMemory mem;
        VkDepthStencilView view;
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
    VkPipeline pipeline;

    VkDynamicVpState viewport;
    VkDynamicRsState raster;
    VkDynamicCbState color_blend;
    VkDynamicDsState depth_stencil;

    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    VkPhysicalDeviceMemoryProperties memory_properties;

    bool validate;
    PFN_vkDbgCreateMsgCallback dbgCreateMsgCallback;
    VkDbgMsgCallback msg_callback;

    bool quit;
    uint32_t current_buffer;
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

    err = vkQueueSubmit(demo->queue, 1, cmd_bufs, VK_NULL_HANDLE);
    assert(!err);

    err = vkQueueWaitIdle(demo->queue);
    assert(!err);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_COMMAND_BUFFER, demo->setup_cmd);
    demo->setup_cmd = VK_NULL_HANDLE;
}

static void demo_set_image_layout(
        struct demo *demo,
        VkImage image,
        VkImageAspect aspect,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout)
{
    VkResult U_ASSERT_ONLY err;

    if (demo->setup_cmd == VK_NULL_HANDLE) {
        const VkCmdBufferCreateInfo cmd = {
            .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .queueNodeIndex = demo->graphics_queue_node_index,
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
        .subresourceRange = { aspect, 0, 1, 0, 0 }
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

    vkCmdPipelineBarrier(demo->setup_cmd, src_stages, dest_stages, false, 1, (const void **)&pmemory_barrier);
}

static void demo_draw_build_cmd(struct demo *demo)
{
    const VkColorAttachmentBindInfo color_attachment = {
        .view = demo->buffers[demo->current_buffer].view,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkDepthStencilBindInfo depth_stencil = {
        .view = demo->depth.view,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkClearColorValue clear_color = {
        .f32 = { 0.2f, 0.2f, 0.2f, 0.2f },
    };
    const float clear_depth = 0.9f;
    VkCmdBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
            VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
    };
    VkResult U_ASSERT_ONLY err;
    VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    const VkFramebufferCreateInfo fb_info = {
         .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
         .pNext = NULL,
         .colorAttachmentCount = 1,
         .pColorAttachments = (VkColorAttachmentBindInfo*) &color_attachment,
         .pDepthStencilAttachment = (VkDepthStencilBindInfo*) &depth_stencil,
         .sampleCount = 1,
         .width  = demo->width,
         .height = demo->height,
         .layers = 1,
    };
    VkRenderPassCreateInfo rp_info;
    VkRenderPassBegin rp_begin;

    rp_begin.contents = VK_RENDER_PASS_CONTENTS_INLINE;

    memset(&rp_info, 0 , sizeof(rp_info));
    err = vkCreateFramebuffer(demo->device, &fb_info, &rp_begin.framebuffer);
    assert(!err);
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.renderArea.extent.width = demo->width;
    rp_info.renderArea.extent.height = demo->height;
    rp_info.colorAttachmentCount = fb_info.colorAttachmentCount;
    rp_info.pColorFormats = &demo->format;
    rp_info.pColorLayouts = &color_attachment.layout;
    rp_info.pColorLoadOps = &load_op;
    rp_info.pColorStoreOps = &store_op;
    rp_info.pColorLoadClearValues = &clear_color;
    rp_info.depthStencilFormat = VK_FORMAT_D16_UNORM;
    rp_info.depthStencilLayout = depth_stencil.layout;
    rp_info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    rp_info.depthLoadClearValue = clear_depth;
    rp_info.depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    rp_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.stencilLoadClearValue = 0;
    rp_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    err = vkCreateRenderPass(demo->device, &rp_info, &(rp_begin.renderPass));
    assert(!err);

    err = vkBeginCommandBuffer(demo->draw_cmd, &cmd_buf_info);
    assert(!err);

    vkCmdBeginRenderPass(demo->draw_cmd, &rp_begin);
    vkCmdBindPipeline(demo->draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    vkCmdBindDescriptorSets(demo->draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline_layout,
            0, 1, & demo->desc_set, 0, NULL);

    vkCmdBindDynamicStateObject(demo->draw_cmd, VK_STATE_BIND_POINT_VIEWPORT, demo->viewport);
    vkCmdBindDynamicStateObject(demo->draw_cmd, VK_STATE_BIND_POINT_RASTER, demo->raster);
    vkCmdBindDynamicStateObject(demo->draw_cmd, VK_STATE_BIND_POINT_COLOR_BLEND,
                                     demo->color_blend);
    vkCmdBindDynamicStateObject(demo->draw_cmd, VK_STATE_BIND_POINT_DEPTH_STENCIL,
                                     demo->depth_stencil);

    VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(demo->draw_cmd, VERTEX_BUFFER_BIND_ID, 1, &demo->vertices.buf, offsets);

    vkCmdBeginRenderPass(demo->draw_cmd, &rp_begin);

    vkCmdDraw(demo->draw_cmd, 0, 3, 0, 1);
    vkCmdEndRenderPass(demo->draw_cmd);

    err = vkEndCommandBuffer(demo->draw_cmd);
    assert(!err);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_RENDER_PASS, rp_begin.renderPass);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_FRAMEBUFFER, rp_begin.framebuffer);
}

static void demo_draw(struct demo *demo)
{
    const VkPresentInfoWSI present = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_WSI,
        .pNext = NULL,
        .image = demo->buffers[demo->current_buffer].image,
        .flipInterval = 0,
    };
    VkResult U_ASSERT_ONLY err;

    demo_draw_build_cmd(demo);

    err = vkQueueSubmit(demo->queue, 1, &demo->draw_cmd, VK_NULL_HANDLE);
    assert(!err);

    err = demo->fpQueuePresentWSI(demo->queue, &present);
    assert(!err);

    demo->current_buffer = (demo->current_buffer + 1) % DEMO_BUFFER_COUNT;

    err = vkQueueWaitIdle(demo->queue);
    assert(err == VK_SUCCESS);
}

static void demo_prepare_buffers(struct demo *demo)
{
    const VkSwapChainCreateInfoWSI swap_chain = {
        .sType = VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI,
        .pNext = NULL,
        .pNativeWindowSystemHandle = demo->connection,
        .pNativeWindowHandle = (void *) (intptr_t) demo->window,
        .displayCount = 1,
        .imageCount = DEMO_BUFFER_COUNT,
        .imageFormat = demo->format,
        .imageExtent = {
            .width = demo->width,
            .height = demo->height,
        },
        .imageArraySize = 1,
        .imageUsageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    };
    VkSwapChainImageInfoWSI images[DEMO_BUFFER_COUNT];
    size_t images_size = sizeof(images);
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    err = demo->fpCreateSwapChainWSI(demo->device, &swap_chain, &demo->swap_chain);
    assert(!err);

    err = demo->fpGetSwapChainInfoWSI(demo->swap_chain,
            VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_WSI,
            &images_size, images);
    assert(!err && images_size == sizeof(images));

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        VkColorAttachmentViewCreateInfo color_attachment_view = {
            .sType = VK_STRUCTURE_TYPE_COLOR_ATTACHMENT_VIEW_CREATE_INFO,
            .pNext = NULL,
            .format = demo->format,
            .mipLevel = 0,
            .baseArraySlice = 0,
            .arraySize = 1,
        };

        demo->buffers[i].image = images[i].image;
        demo->buffers[i].mem = images[i].memory;

        demo_set_image_layout(demo, demo->buffers[i].image,
                               VK_IMAGE_ASPECT_COLOR,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        color_attachment_view.image = demo->buffers[i].image;

        err = vkCreateColorAttachmentView(demo->device,
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
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_BIT,
        .flags = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memoryTypeIndex = 0,
    };
    VkDepthStencilViewCreateInfo view = {
        .sType = VK_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = VK_NULL_HANDLE,
        .mipLevel = 0,
        .baseArraySlice = 0,
        .arraySize = 1,
        .flags = 0,
    };

    VkMemoryRequirements mem_reqs;
    VkResult U_ASSERT_ONLY err;

    demo->depth.format = depth_format;

    /* create image */
    err = vkCreateImage(demo->device, &image,
            &demo->depth.image);
    assert(!err);

    /* get memory requirements for this object */
    err = vkGetObjectMemoryRequirements(demo->device,
                    VK_OBJECT_TYPE_IMAGE, demo->depth.image, &mem_reqs);

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
    err = vkBindObjectMemory(demo->device,
            VK_OBJECT_TYPE_IMAGE, demo->depth.image,
            demo->depth.mem, 0);
    assert(!err);

    demo_set_image_layout(demo, demo->depth.image,
                           VK_IMAGE_ASPECT_DEPTH,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    /* create image view */
    view.image = demo->depth.image;
    err = vkCreateDepthStencilView(demo->device, &view,
            &demo->depth.view);
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

    err = vkGetObjectMemoryRequirements(demo->device,
                VK_OBJECT_TYPE_IMAGE, tex_obj->image, &mem_reqs);

    mem_alloc.allocationSize  = mem_reqs.size;
    err = memory_type_from_properties(demo, mem_reqs.memoryTypeBits, mem_props, &mem_alloc.memoryTypeIndex);
    assert(!err);

    /* allocate memory */
    err = vkAllocMemory(demo->device, &mem_alloc, &tex_obj->mem);
    assert(!err);

    /* bind memory */
    err = vkBindObjectMemory(demo->device,
            VK_OBJECT_TYPE_IMAGE, tex_obj->image,
            tex_obj->mem, 0);
    assert(!err);

    if (mem_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        const VkImageSubresource subres = {
            .aspect = VK_IMAGE_ASPECT_COLOR,
            .mipLevel = 0,
            .arraySlice = 0,
        };
        VkSubresourceLayout layout;
        void *data;
        int32_t x, y;

        err = vkGetImageSubresourceLayout(demo->device, tex_obj->image, &subres, &layout);
        assert(!err);

        err = vkMapMemory(demo->device, tex_obj->mem, 0, 0, 0, &data);
        assert(!err);

        for (y = 0; y < tex_height; y++) {
            uint32_t *row = (uint32_t *) ((char *) data + layout.rowPitch * y);
            for (x = 0; x < tex_width; x++)
                row[x] = tex_colors[(x & 1) ^ (y & 1)];
        }

        err = vkUnmapMemory(demo->device, tex_obj->mem);
        assert(!err);
    }

    tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    demo_set_image_layout(demo, tex_obj->image,
                           VK_IMAGE_ASPECT_COLOR,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           tex_obj->imageLayout);
    /* setting the image layout does not reference the actual memory so no need to add a mem ref */
}

static void demo_destroy_texture_image(struct demo *demo, struct texture_object *tex_obj)
{
    /* clean up staging resources */
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_IMAGE, tex_obj->image);
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

    err = vkGetPhysicalDeviceFormatInfo(demo->gpu, tex_format, &props);
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
                                   VK_IMAGE_ASPECT_COLOR,
                                   staging_texture.imageLayout,
                                   VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL);

            demo_set_image_layout(demo, demo->textures[i].image,
                                   VK_IMAGE_ASPECT_COLOR,
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
                                   VK_IMAGE_ASPECT_COLOR,
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
            .addressU = VK_TEX_ADDRESS_WRAP,
            .addressV = VK_TEX_ADDRESS_WRAP,
            .addressW = VK_TEX_ADDRESS_WRAP,
            .mipLodBias = 0.0f,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        };
        VkImageViewCreateInfo view = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = VK_NULL_HANDLE,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = tex_format,
            .channels = { VK_CHANNEL_SWIZZLE_R,
                          VK_CHANNEL_SWIZZLE_G,
                          VK_CHANNEL_SWIZZLE_B,
                          VK_CHANNEL_SWIZZLE_A, },
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR, 0, 1, 0, 1 },
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

    err = vkGetObjectMemoryRequirements(demo->device,
            VK_OBJECT_TYPE_BUFFER, demo->vertices.buf, &mem_reqs);

    mem_alloc.allocationSize  = mem_reqs.size;
    err = memory_type_from_properties(demo,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &mem_alloc.memoryTypeIndex);
    assert(!err);

    err = vkAllocMemory(demo->device, &mem_alloc, &demo->vertices.mem);
    assert(!err);

    err = vkMapMemory(demo->device, demo->vertices.mem, 0, 0, 0, &data);
    assert(!err);

    memcpy(data, vb, sizeof(vb));

    err = vkUnmapMemory(demo->device, demo->vertices.mem);
    assert(!err);

    err = vkBindObjectMemory(demo->device,
            VK_OBJECT_TYPE_BUFFER, demo->vertices.buf,
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

static VkShader demo_prepare_shader(struct demo *demo,
                                      VkShaderStage stage,
                                      const void *code,
                                      size_t size)
{
    VkShaderModuleCreateInfo moduleCreateInfo;
    VkShaderCreateInfo shaderCreateInfo;
    VkShaderModule shaderModule;
    VkShader shader;
    VkResult err;


    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;

    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    shaderCreateInfo.pNext = NULL;

    if (!demo->use_glsl) {
        moduleCreateInfo.codeSize = size;
        moduleCreateInfo.pCode = code;
        moduleCreateInfo.flags = 0;
        err = vkCreateShaderModule(demo->device, &moduleCreateInfo, &shaderModule);
        if (err) {
            free((void *) moduleCreateInfo.pCode);
        }

        shaderCreateInfo.flags = 0;
        shaderCreateInfo.module = shaderModule;
        err = vkCreateShader(demo->device, &shaderCreateInfo, &shader);
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

        err = vkCreateShaderModule(demo->device, &moduleCreateInfo, &shaderModule);
        if (err) {
            free((void *) moduleCreateInfo.pCode);
        }

        shaderCreateInfo.flags = 0;
        shaderCreateInfo.module = shaderModule;
        err = vkCreateShader(demo->device, &shaderCreateInfo, &shader);
    }
    return shader;
}

char *demo_read_spv(const char *filename, size_t *psize)
{
    long int size;
    void *shader_code;

    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    shader_code = malloc(size);
    fread(shader_code, size, 1, fp);

    *psize = size;

    return shader_code;
}

static VkShader demo_prepare_vs(struct demo *demo)
{
    if (!demo->use_glsl) {
       void *vertShaderCode;
       size_t size;

       vertShaderCode = demo_read_spv("tri-vert.spv", &size);

       return demo_prepare_shader(demo, VK_SHADER_STAGE_VERTEX,
          vertShaderCode, size);
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
                                   (const void *) vertShaderText,
                                   strlen(vertShaderText));
    }
}

static VkShader demo_prepare_fs(struct demo *demo)
{
    if (!demo->use_glsl) {
       void *fragShaderCode;
       size_t size;

       fragShaderCode = demo_read_spv("tri-frag.spv", &size);

       return demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT,
          fragShaderCode, size);
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
                                   (const void *) fragShaderText,
                                   strlen(fragShaderText));
    }
}

static void demo_prepare_pipeline(struct demo *demo)
{
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;

    VkPipelineVertexInputStateCreateInfo vi;
    VkPipelineIaStateCreateInfo ia;
    VkPipelineRsStateCreateInfo rs;
    VkPipelineCbStateCreateInfo cb;
    VkPipelineDsStateCreateInfo ds;
    VkPipelineVpStateCreateInfo vp;
    VkPipelineMsStateCreateInfo ms;

    VkResult U_ASSERT_ONLY err;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = demo->pipeline_layout;

    vi = demo->vertices.vi;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
    rs.fillMode = VK_FILL_MODE_SOLID;
    rs.cullMode = VK_CULL_MODE_BACK;
    rs.frontFace = VK_FRONT_FACE_CW;
    rs.depthClipEnable = VK_TRUE;

    memset(&cb, 0, sizeof(cb));
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_CB_STATE_CREATE_INFO;
    VkPipelineCbAttachmentState att_state[1];
    memset(att_state, 0, sizeof(att_state));
    att_state[0].format = demo->format;
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE;
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;

    memset(&vp, 0, sizeof(vp));
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VP_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.clipOrigin = VK_COORDINATE_ORIGIN_UPPER_LEFT;

    memset(&ds, 0, sizeof(ds));
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO;
    ds.format = demo->depth.format;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_EQUAL;
    ds.depthBoundsEnable = VK_FALSE;
    ds.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO;
    ms.sampleMask = 1;
    ms.multisampleEnable = VK_FALSE;
    ms.rasterSamples = 1;

    // Two stages: vs and fs
    pipeline.stageCount = 2;
    VkPipelineShaderStageCreateInfo shaderStages[2];
    memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

    shaderStages[0].sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage                = VK_SHADER_STAGE_VERTEX;
    shaderStages[0].shader               = demo_prepare_vs(demo);
    shaderStages[0].linkConstBufferCount = 0;

    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT;
    shaderStages[1].shader = demo_prepare_fs(demo);

    pipeline.pVertexInputState = &vi;
    pipeline.pIaState = &ia;
    pipeline.pRsState = &rs;
    pipeline.pCbState = &cb;
    pipeline.pMsState = &ms;
    pipeline.pVpState = &vp;
    pipeline.pDsState = &ds;
    pipeline.pStages  = shaderStages;

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache(demo->device, &pipelineCache, &demo->pipelineCache);
    assert(!err);
    err = vkCreateGraphicsPipelines(demo->device, demo->pipelineCache, 1, &pipeline, &demo->pipeline);
    assert(!err);

    for (uint32_t i = 0; i < pipeline.stageCount; i++) {
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_SHADER, shaderStages[i].shader);
    }
}

static void demo_prepare_dynamic_states(struct demo *demo)
{
    VkDynamicVpStateCreateInfo viewport_create;
    VkDynamicRsStateCreateInfo raster;
    VkDynamicCbStateCreateInfo color_blend;
    VkDynamicDsStateCreateInfo depth_stencil;
    VkResult U_ASSERT_ONLY err;

    memset(&viewport_create, 0, sizeof(viewport_create));
    viewport_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_VP_STATE_CREATE_INFO;
    viewport_create.viewportAndScissorCount = 1;
    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float) demo->height;
    viewport.width = (float) demo->width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport_create.pViewports = &viewport;
    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    viewport_create.pScissors = &scissor;

    memset(&raster, 0, sizeof(raster));
    raster.sType = VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO;
    raster.lineWidth = 1.0;

    memset(&color_blend, 0, sizeof(color_blend));
    color_blend.sType = VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO;
    color_blend.blendConst[0] = 1.0f;
    color_blend.blendConst[1] = 1.0f;
    color_blend.blendConst[2] = 1.0f;
    color_blend.blendConst[3] = 1.0f;

    memset(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO;
    depth_stencil.minDepthBounds = 0.0f;
    depth_stencil.maxDepthBounds = 1.0f;
    depth_stencil.stencilBackRef = 0;
    depth_stencil.stencilFrontRef = 0;
    depth_stencil.stencilReadMask = 0xff;
    depth_stencil.stencilWriteMask = 0xff;

    err = vkCreateDynamicViewportState(demo->device, &viewport_create, &demo->viewport);
    assert(!err);

    err = vkCreateDynamicRasterState(demo->device, &raster, &demo->raster);
    assert(!err);

    err = vkCreateDynamicColorBlendState(demo->device,
            &color_blend, &demo->color_blend);
    assert(!err);

    err = vkCreateDynamicDepthStencilState(demo->device,
            &depth_stencil, &demo->depth_stencil);
    assert(!err);
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
        .count = 1,
        .pTypeCount = &type_count,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateDescriptorPool(demo->device,
            VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1,
            &descriptor_pool, &demo->desc_pool);
    assert(!err);
}

static void demo_prepare_descriptor_set(struct demo *demo)
{
    VkDescriptorInfo tex_descs[DEMO_TEXTURE_COUNT];
    VkWriteDescriptorSet write;
    VkResult U_ASSERT_ONLY err;
    uint32_t count;
    uint32_t i;

    err = vkAllocDescriptorSets(demo->device, demo->desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            1, &demo->desc_layout,
            &demo->desc_set, &count);
    assert(!err && count == 1);

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

    err = vkUpdateDescriptorSets(demo->device, 1, &write, 0, NULL);
    assert(!err);
}

static void demo_prepare(struct demo *demo)
{
    const VkCmdBufferCreateInfo cmd = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .queueNodeIndex = demo->graphics_queue_node_index,
        .level = VK_CMD_BUFFER_LEVEL_PRIMARY,
        .flags = 0,
    };
    VkResult U_ASSERT_ONLY err;

    err = vkCreateCommandBuffer(demo->device, &cmd, &demo->draw_cmd);
    assert(!err);

    demo_prepare_buffers(demo);
    demo_prepare_depth(demo);
    demo_prepare_textures(demo);
    demo_prepare_vertices(demo);
    demo_prepare_descriptor_layout(demo);
    demo_prepare_pipeline(demo);
    demo_prepare_dynamic_states(demo);

    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);
    demo->prepared = true;
}

#ifdef _WIN32
static void demo_run(struct demo *demo)
{
    if (!demo->prepared)
        return;
    demo_draw(demo);
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
        demo_run(&demo);
        return 0;
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

        event = xcb_wait_for_event(demo->connection);
        if (event) {
            demo_handle_event(demo, event);
            free(event);
        }
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

static void demo_init_vk(struct demo *demo)
{
    VkResult err;
    char *extension_names[64];
    char *layer_names[64];
    VkExtensionProperties *instance_extensions;
    VkLayerProperties *instance_layers;
    VkLayerProperties *device_layers;
    uint32_t instance_extension_count = 0;
    uint32_t instance_layer_count = 0;
    uint32_t enabled_extension_count = 0;
    uint32_t enabled_layer_count = 0;

    /* Look for validation layers */
    VkBool32 validation_found = 0;
    err = vkGetGlobalLayerProperties(&instance_layer_count, NULL);
    assert(!err);

    memset(layer_names, 0, sizeof(layer_names));
    instance_layers = malloc(sizeof(VkLayerProperties) * instance_layer_count);
    err = vkGetGlobalLayerProperties(&instance_layer_count, instance_layers);
    assert(!err);
    for (uint32_t i = 0; i < instance_layer_count; i++) {
        if (!validation_found && demo->validate && !strcmp("Validation", instance_layers[i].layerName)) {
            layer_names[enabled_layer_count++] = "Validation";
            validation_found = 1;
        }
        assert(enabled_layer_count < 64);
    }
    if (demo->validate && !validation_found) {
        ERR_EXIT("vkGetGlobalLayerProperties failed to find any "
                 "\"Validation\" layers.\n\n"
                 "Please look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }

    err = vkGetGlobalExtensionProperties(NULL, &instance_extension_count, NULL);
    assert(!err);

    VkBool32 WSIextFound = 0;
    memset(extension_names, 0, sizeof(extension_names));
    instance_extensions = malloc(sizeof(VkExtensionProperties) * instance_extension_count);
    err = vkGetGlobalExtensionProperties(NULL, &instance_extension_count, instance_extensions);
    assert(!err);
    for (uint32_t i = 0; i < instance_extension_count; i++) {
        if (!strcmp(VK_WSI_LUNARG_EXTENSION_NAME, instance_extensions[i].extName)) {
            WSIextFound = 1;
            extension_names[enabled_extension_count++] = VK_WSI_LUNARG_EXTENSION_NAME;
        }
        if (!strcmp(DEBUG_REPORT_EXTENSION_NAME, instance_extensions[i].extName)) {
            if (demo->validate) {
                extension_names[enabled_extension_count++] = DEBUG_REPORT_EXTENSION_NAME;
            }
        }
        assert(enabled_extension_count < 64);
    }
    if (!WSIextFound) {
        ERR_EXIT("vkGetGlobalExtensionProperties failed to find the "
                 "\"VK_WSI_LunarG\" extension.\n\nDo you have a compatible "
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
    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pAppInfo = &app,
        .pAllocCb = NULL,
        .layerCount = enabled_layer_count,
        .ppEnabledLayerNames = (const char *const*) layer_names,
        .extensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const*) extension_names,
    };
    const VkDeviceQueueCreateInfo queue = {
        .queueNodeIndex = 0,
        .queueCount = 1,
    };

    uint32_t gpu_count;
    uint32_t i;
    uint32_t queue_count;

    err = vkCreateInstance(&inst_info, &demo->inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        ERR_EXIT("Cannot find a compatible Vulkan installable client driver "
                 "(ICD).\n\nPlease look at the Getting Started guide for "
                 "additional information.\n",
                 "vkCreateInstance Failure");
    } else if (err == VK_ERROR_INVALID_EXTENSION) {
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

    gpu_count = 1;
    err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, &demo->gpu);
    assert(!err && gpu_count == 1);

    /* Look for validation layers */
    validation_found = 0;
    enabled_layer_count = 0;
    uint32_t device_layer_count = 0;
    err = vkGetPhysicalDeviceLayerProperties(demo->gpu, &device_layer_count, NULL);
    assert(!err);

    memset(layer_names, 0, sizeof(layer_names));
    device_layers = malloc(sizeof(VkLayerProperties) * device_layer_count);
    err = vkGetPhysicalDeviceLayerProperties(demo->gpu, &device_layer_count, device_layers);
    assert(!err);
    for (uint32_t i = 0; i < device_layer_count; i++) {
        if (!validation_found && demo->validate &&
            !strcmp("Validation", device_layers[i].layerName)) {
            layer_names[enabled_layer_count++] = "Validation";
            validation_found = 1;
        }
        assert(enabled_layer_count < 64);
    }
    if (demo->validate && !validation_found) {
        ERR_EXIT("vkGetGlobalLayerProperties failed to find any "
                 "\"Validation\" layers.\n\n"
                 "Please look at the Getting Started guide for additional "
                 "information.\n",
                 "vkCreateInstance Failure");
    }

    /* Don't need any device extensions */
    /* TODO: WSI device extension will go here eventually */

    VkDeviceCreateInfo device = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 1,
        .pRequestedQueues = &queue,
        .layerCount = enabled_layer_count,
        .ppEnabledLayerNames = (const char*const*) layer_names,
        .extensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .flags = 0,
    };

    if (demo->validate) {
        demo->dbgCreateMsgCallback = vkGetInstanceProcAddr((VkPhysicalDevice) NULL, "vkDbgCreateMsgCallback");
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
        case VK_ERROR_INVALID_POINTER:
            ERR_EXIT("dbgCreateMsgCallback: Invalid pointer\n",
                     "dbgCreateMsgCallback Failure");
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

    free(device_layers);

    GET_DEVICE_PROC_ADDR(demo->device, CreateSwapChainWSI);
    GET_DEVICE_PROC_ADDR(demo->device, CreateSwapChainWSI);
    GET_DEVICE_PROC_ADDR(demo->device, DestroySwapChainWSI);
    GET_DEVICE_PROC_ADDR(demo->device, GetSwapChainInfoWSI);
    GET_DEVICE_PROC_ADDR(demo->device, QueuePresentWSI);

    err = vkGetPhysicalDeviceProperties(demo->gpu, &demo->gpu_props);
    assert(!err);

    err = vkGetPhysicalDeviceQueueCount(demo->gpu, &queue_count);
    assert(!err);

    demo->queue_props = (VkPhysicalDeviceQueueProperties *) malloc(queue_count * sizeof(VkPhysicalDeviceQueueProperties));
    err = vkGetPhysicalDeviceQueueProperties(demo->gpu, queue_count, demo->queue_props);
    assert(!err);
    assert(queue_count >= 1);

    // Graphics queue and MemMgr queue can be separate.
    // TODO: Add support for separate queues, including synchronization,
    //       and appropriate tracking for QueueSubmit
    for (i = 0; i < queue_count; i++) {
        if (demo->queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            break;
    }
    assert(i < queue_count);
    demo->graphics_queue_node_index = i;

    err = vkGetDeviceQueue(demo->device, demo->graphics_queue_node_index,
            0, &demo->queue);
    assert(!err);

    // for now hardcode format till get WSI support
    demo->format = VK_FORMAT_B8G8R8A8_UNORM;

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
}

static void demo_cleanup(struct demo *demo)
{
    uint32_t i;

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DESCRIPTOR_SET, demo->desc_set);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, demo->desc_pool);

    if (demo->setup_cmd) {
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_COMMAND_BUFFER, demo->setup_cmd);
    }
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_COMMAND_BUFFER, demo->draw_cmd);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DYNAMIC_VP_STATE, demo->viewport);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DYNAMIC_RS_STATE, demo->raster);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DYNAMIC_CB_STATE, demo->color_blend);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DYNAMIC_DS_STATE, demo->depth_stencil);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_PIPELINE, demo->pipeline);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, demo->pipeline_layout);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, demo->desc_layout);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_BUFFER, demo->vertices.buf);
    vkFreeMemory(demo->device, demo->vertices.mem);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_IMAGE_VIEW, demo->textures[i].view);
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_IMAGE, demo->textures[i].image);
        vkFreeMemory(demo->device, demo->textures[i].mem);
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_SAMPLER, demo->textures[i].sampler);
    }

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DEPTH_STENCIL_VIEW, demo->depth.view);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_IMAGE, demo->depth.image);
    vkFreeMemory(demo->device, demo->depth.mem);

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW, demo->buffers[i].view);
    }
    demo->fpDestroySwapChainWSI(demo->swap_chain);

    vkDestroyDevice(demo->device);
    vkDestroyInstance(demo->inst);

#ifndef _WIN32
    xcb_destroy_window(demo->connection, demo->window);
    xcb_disconnect(demo->connection);
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

    demo_prepare(&demo);
    demo_run(&demo);

    demo_cleanup(&demo);

    return 0;
}
#endif // _WIN32
