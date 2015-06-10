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
#define _GNU_SOURCE
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

#include "linmath.h"
#include <png.h>

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1
#define APP_SHORT_NAME "cube"
#define APP_LONG_NAME "The Vulkan Cube Demo Program"

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

/*
 * structure to track all objects related to a texture.
 */
struct texture_object {
    VkSampler sampler;

    VkImage image;
    VkImageLayout imageLayout;

    VkDeviceMemory mem;
    VkImageView view;
    int32_t tex_width, tex_height;
};

static char *tex_files[] = {
    "lunarg-logo-256x256-solid.png"
};

struct vkcube_vs_uniform {
    // Must start with MVP
    float       mvp[4][4];
    float       position[12*3][4];
    float       color[12*3][4];
};

struct vktexcube_vs_uniform {
    // Must start with MVP
    float       mvp[4][4];
    float       position[12*3][4];
    float       attr[12*3][4];
};

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
struct Vertex
{
    float     posX, posY, posZ, posW;    // Position data
    float     r, g, b, a;                // Color
};

struct VertexPosTex
{
    float     posX, posY, posZ, posW;    // Position data
    float     u, v, s, t;                // Texcoord
};

#define XYZ1(_x_, _y_, _z_)         (_x_), (_y_), (_z_), 1.f
#define UV(_u_, _v_)                (_u_), (_v_), 0.f, 1.f

static const float g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Z side
     1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Y side
     1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // +Y side
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  // +X side
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // +Z side
    -1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = {
    0.0f, 0.0f,  // -X side
    1.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Z side
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Y side
    1.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // +Y side
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,

    1.0f, 1.0f,  // +X side
    0.0f, 1.0f,
    0.0f, 0.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,

    0.0f, 1.0f,  // +Z side
    0.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
};

void dumpMatrix(const char *note, mat4x4 MVP)
{
    int i;

    printf("%s: \n", note);
    for (i=0; i<4; i++) {
        printf("%f, %f, %f, %f\n", MVP[i][0], MVP[i][1], MVP[i][2], MVP[i][3]);
    }
    printf("\n");
    fflush(stdout);
}

void dumpVec4(const char *note, vec4 vector)
{
    printf("%s: \n", note);
        printf("%f, %f, %f, %f\n", vector[0], vector[1], vector[2], vector[3]);
    printf("\n");
    fflush(stdout);
}

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
    uint32_t graphics_queue_node_index;
    VkPhysicalDeviceProperties *gpu_props;
    VkPhysicalDeviceQueueProperties *queue_props;

    VkFramebuffer framebuffer;
    int width, height;
    VkFormat format;

    VkDisplayPropertiesWSI *display_props;
    int num_displays;
    PFN_vkGetDisplayInfoWSI fpGetDisplayInfoWSI;
    PFN_vkCreateSwapChainWSI fpCreateSwapChainWSI;
    PFN_vkDestroySwapChainWSI fpDestroySwapChainWSI;
    PFN_vkGetSwapChainInfoWSI fpGetSwapChainInfoWSI;
    PFN_vkQueuePresentWSI fpQueuePresentWSI;
    VkSwapChainWSI swap_chain;
    struct {
        VkImage image;
        VkDeviceMemory mem;
        VkCmdBuffer cmd;

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
        VkBufferView view;
        VkDescriptorInfo desc;
    } uniform_data;

    VkCmdBuffer cmd;  // Buffer for initialization commands
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipeline pipeline;

    VkDynamicVpState viewport;
    VkDynamicRsState raster;
    VkDynamicCbState color_blend;
    VkDynamicDsState depth_stencil;

    mat4x4 projection_matrix;
    mat4x4 view_matrix;
    mat4x4 model_matrix;

    float spin_angle;
    float spin_increment;
    bool pause;

    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    bool quit;
    bool validate;
    PFN_vkDbgCreateMsgCallback dbgCreateMsgCallback;
    VkDbgMsgCallback msg_callback;

    uint32_t current_buffer;
};

static void demo_flush_init_cmd(struct demo *demo)
{
    VkResult U_ASSERT_ONLY err;

    if (demo->cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(demo->cmd);
    assert(!err);

    const VkCmdBuffer cmd_bufs[] = { demo->cmd };

    err = vkQueueSubmit(demo->queue, 1, cmd_bufs, VK_NULL_HANDLE);
    assert(!err);

    err = vkQueueWaitIdle(demo->queue);
    assert(!err);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_COMMAND_BUFFER, demo->cmd);
    demo->cmd = VK_NULL_HANDLE;
}

static void demo_set_image_layout(
        struct demo *demo,
        VkImage image,
        VkImageAspect aspect,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout)
{
    VkResult U_ASSERT_ONLY err;

    if (demo->cmd == VK_NULL_HANDLE) {
        const VkCmdBufferCreateInfo cmd = {
            .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
            .pNext = NULL,
            .queueNodeIndex = demo->graphics_queue_node_index,
            .flags = 0,
        };

        err = vkCreateCommandBuffer(demo->device, &cmd, &demo->cmd);
        assert(!err);

        VkCmdBufferBeginInfo cmd_buf_info = {
            .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
            .pNext = NULL,
            .flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
                VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT,
        };
        err = vkBeginCommandBuffer(demo->cmd, &cmd_buf_info);
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
        image_memory_barrier.outputMask = VK_MEMORY_OUTPUT_HOST_WRITE_BIT | VK_MEMORY_OUTPUT_TRANSFER_BIT;
    }

    VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

    VkPipeEvent set_events[] = { VK_PIPE_EVENT_TOP_OF_PIPE };

    vkCmdPipelineBarrier(demo->cmd, VK_WAIT_EVENT_TOP_OF_PIPE, 1, set_events, 1, (const void **)&pmemory_barrier);
}

static void demo_draw_build_cmd(struct demo *demo, VkCmdBuffer cmd_buf)
{
    const VkColorAttachmentBindInfo color_attachment = {
        .view = demo->buffers[demo->current_buffer].view,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    const VkDepthStencilBindInfo depth_stencil = {
        .view = demo->depth.view,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    const VkClearColor clear_color = {
        .color.floatColor = { 0.2f, 0.2f, 0.2f, 0.2f },
        .useRawValue = false,
    };
    const float clear_depth = 1.0f;
    VkImageSubresourceRange clear_range;
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
    rp_info.depthLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.depthLoadClearValue = clear_depth;
    rp_info.depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    rp_info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp_info.stencilLoadClearValue = 0;
    rp_info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    err = vkCreateRenderPass(demo->device, &rp_info, &rp_begin.renderPass);
    assert(!err);

    err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);
    assert(!err);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
            0, 1, &demo->desc_set, 0, NULL);

    vkCmdBindDynamicStateObject(cmd_buf, VK_STATE_BIND_POINT_VIEWPORT, demo->viewport);
    vkCmdBindDynamicStateObject(cmd_buf, VK_STATE_BIND_POINT_RASTER, demo->raster);
    vkCmdBindDynamicStateObject(cmd_buf, VK_STATE_BIND_POINT_COLOR_BLEND,
                                     demo->color_blend);
    vkCmdBindDynamicStateObject(cmd_buf, VK_STATE_BIND_POINT_DEPTH_STENCIL,
                                     demo->depth_stencil);

    vkCmdBeginRenderPass(cmd_buf, &rp_begin);
    clear_range.aspect = VK_IMAGE_ASPECT_DEPTH;
    clear_range.baseMipLevel = 0;
    clear_range.mipLevels = 1;
    clear_range.baseArraySlice = 0;
    clear_range.arraySize = 1;

    vkCmdClearDepthStencil(cmd_buf, demo->depth.image,
            VK_IMAGE_LAYOUT_CLEAR_OPTIMAL,
            clear_depth, 0, 1, &clear_range);

    vkCmdDraw(cmd_buf, 0, 12 * 3, 0, 1);
    vkCmdEndRenderPass(cmd_buf, rp_begin.renderPass);

    err = vkEndCommandBuffer(cmd_buf);
    assert(!err);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_RENDER_PASS, rp_begin.renderPass);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_FRAMEBUFFER, rp_begin.framebuffer);
}


void demo_update_data_buffer(struct demo *demo)
{
    mat4x4 MVP, Model, VP;
    int matrixSize = sizeof(MVP);
    uint8_t *pData;
    VkResult U_ASSERT_ONLY err;

    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);

    // Rotate 22.5 degrees around the Y axis
    mat4x4_dup(Model, demo->model_matrix);
    mat4x4_rotate(demo->model_matrix, Model, 0.0f, 1.0f, 0.0f, (float)degreesToRadians(demo->spin_angle));
    mat4x4_mul(MVP, VP, demo->model_matrix);

    err = vkMapMemory(demo->device, demo->uniform_data.mem, 0, 0, 0, (void **) &pData);
    assert(!err);

    memcpy(pData, (const void*) &MVP[0][0], matrixSize);

    err = vkUnmapMemory(demo->device, demo->uniform_data.mem);
    assert(!err);
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

    err = vkQueueSubmit(demo->queue, 1, &demo->buffers[demo->current_buffer].cmd,
            VK_NULL_HANDLE);
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
        .memProps = VK_MEMORY_PROPERTY_DEVICE_ONLY,
        .memPriority = VK_MEMORY_PRIORITY_NORMAL,
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
    size_t mem_reqs_size = sizeof(VkMemoryRequirements);
    VkResult U_ASSERT_ONLY err;

    demo->depth.format = depth_format;

    /* create image */
    err = vkCreateImage(demo->device, &image,
            &demo->depth.image);
    assert(!err);

    err = vkGetObjectInfo(demo->device,
                    VK_OBJECT_TYPE_IMAGE, demo->depth.image,
                    VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                    &mem_reqs_size, &mem_reqs);
      mem_alloc.allocationSize = mem_reqs.size;

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

/** loadTexture
 *     loads a png file into an memory object, using cstdio , libpng.
 *
 *        \param demo : Needed to access VK calls
 *     \param filename : the png file to be loaded
 *     \param width : width of png, to be updated as a side effect of this function
 *     \param height : height of png, to be updated as a side effect of this function
 *
 *     \return bool : an opengl texture id.  true if successful?,
 *                     should be validated by the client of this function.
 *
 * Source: http://en.wikibooks.org/wiki/OpenGL_Programming/Intermediate/Textures
 * Modified to copy image to memory
 *
 */
bool loadTexture(const char *filename, uint8_t *rgba_data,
                 VkSubresourceLayout *layout,
                 int32_t *width, int32_t *height)
{
  //header for testing if it is a png
  png_byte header[8];
  int is_png, bit_depth, color_type, rowbytes;
  size_t retval;
  png_uint_32 i, twidth, theight;
  png_structp  png_ptr;
  png_infop info_ptr, end_info;
  png_byte *image_data;
  png_bytep *row_pointers;

  //open file as binary
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    return false;
  }

  //read the header
  retval = fread(header, 1, 8, fp);
  if (retval != 8) {
      fclose(fp);
      return false;
  }

  //test if png
  is_png = !png_sig_cmp(header, 0, 8);
  if (!is_png) {
    fclose(fp);
    return false;
  }

  //create png struct
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
      NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    return (false);
  }

  //create png info struct
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
    fclose(fp);
    return (false);
  }

  //create png info struct
  end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
    fclose(fp);
    return (false);
  }

  //png error stuff, not sure libpng man suggests this.
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return (false);
  }

  //init png reading
  png_init_io(png_ptr, fp);

  //let libpng know you already read the first 8 bytes
  png_set_sig_bytes(png_ptr, 8);

  // read all the info up to the image data
  png_read_info(png_ptr, info_ptr);

  // get info about png
  png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type,
      NULL, NULL, NULL);

  //update width and height based on png info
  *width = twidth;
  *height = theight;

  // Require that incoming texture be 8bits per color component
  // and 4 components (RGBA).
  if (png_get_bit_depth(png_ptr, info_ptr) != 8 ||
      png_get_channels(png_ptr, info_ptr) != 4) {
      return false;
  }

  if (rgba_data == NULL) {
      // If data pointer is null, we just want the width & height
      // clean up memory and close stuff
      png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
      fclose(fp);

      return true;
  }

  // Update the png info struct.
  png_read_update_info(png_ptr, info_ptr);

  // Row size in bytes.
  rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  // Allocate the image_data as a big block, to be given to opengl
  image_data = (png_byte *)malloc(rowbytes * theight * sizeof(png_byte));
  if (!image_data) {
    //clean up memory and close stuff
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    return false;
  }

  // row_pointers is for pointing to image_data for reading the png with libpng
  row_pointers = (png_bytep *)malloc(theight * sizeof(png_bytep));
  if (!row_pointers) {
    //clean up memory and close stuff
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    // delete[] image_data;
    fclose(fp);
    return false;
  }
  // set the individual row_pointers to point at the correct offsets of image_data
  for (i = 0; i < theight; ++i)
    row_pointers[theight - 1 - i] = rgba_data + i * layout->rowPitch;

  // read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers);

  // clean up memory and close stuff
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  free(row_pointers);
  free(image_data);
  fclose(fp);

  return true;
}

static void demo_prepare_texture_image(struct demo *demo,
                                       const char *filename,
                                       struct texture_object *tex_obj,
                                       VkImageTiling tiling,
                                       VkImageUsageFlags usage,
                                       VkFlags mem_props)
{
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    int32_t tex_width;
    int32_t tex_height;
    VkResult U_ASSERT_ONLY err;

    if (!loadTexture(filename, NULL, NULL, &tex_width, &tex_height))
    {
        printf("Failed to load textures\n");
        fflush(stdout);
        exit(1);
    }

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
        .memProps = mem_props,
        .memPriority = VK_MEMORY_PRIORITY_NORMAL,
    };

    VkMemoryRequirements mem_reqs;
    size_t mem_reqs_size = sizeof(VkMemoryRequirements);

    err = vkCreateImage(demo->device, &image_create_info,
            &tex_obj->image);
    assert(!err);

    err = vkGetObjectInfo(demo->device,
                VK_OBJECT_TYPE_IMAGE, tex_obj->image,
                VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                &mem_reqs_size, &mem_reqs);
    assert(!err && mem_reqs_size == sizeof(VkMemoryRequirements));

    mem_alloc.allocationSize = mem_reqs.size;

    /* allocate memory */
    err = vkAllocMemory(demo->device, &mem_alloc,
                &(tex_obj->mem));
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
        size_t layout_size = sizeof(VkSubresourceLayout);
        void *data;

        err = vkGetImageSubresourceInfo(demo->device, tex_obj->image, &subres,
                                         VK_SUBRESOURCE_INFO_TYPE_LAYOUT,
                                         &layout_size, &layout);
        assert(!err && layout_size == sizeof(layout));

        err = vkMapMemory(demo->device, tex_obj->mem, 0, 0, 0, &data);
        assert(!err);

        if (!loadTexture(filename, data, &layout, &tex_width, &tex_height)) {
            fprintf(stderr, "Error loading texture: %s\n", filename);
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

static void demo_destroy_texture_image(struct demo *demo, struct texture_object *tex_objs)
{
    /* clean up staging resources */
    vkFreeMemory(demo->device, tex_objs->mem);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_IMAGE, tex_objs->image);
}

static void demo_prepare_textures(struct demo *demo)
{
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties props;
    size_t size = sizeof(props);
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    err = vkGetFormatInfo(demo->device, tex_format,
                           VK_FORMAT_INFO_TYPE_PROPERTIES,
                           &size, &props);
    assert(!err);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {

        if ((props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) && !demo->use_staging_buffer) {
            /* Device can texture using linear textures */
            demo_prepare_texture_image(demo, tex_files[i], &demo->textures[i],
                                       VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
            /* Must use staging buffer to copy linear texture to optimized */
            struct texture_object staging_texture;

            memset(&staging_texture, 0, sizeof(staging_texture));
            demo_prepare_texture_image(demo, tex_files[i], &staging_texture,
                                       VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

            demo_prepare_texture_image(demo, tex_files[i], &demo->textures[i],
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
            vkCmdCopyImage(demo->cmd,
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
            /* Can't support VK_FORMAT_R8G8B8A8_UNORM !? */
            assert(!"No support for R8G8B8A8_UNORM as texture image format");
        }

        const VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = NULL,
            .magFilter = VK_TEX_FILTER_NEAREST,
            .minFilter = VK_TEX_FILTER_NEAREST,
            .mipMode = VK_TEX_MIPMAP_MODE_BASE,
            .addressU = VK_TEX_ADDRESS_CLAMP,
            .addressV = VK_TEX_ADDRESS_CLAMP,
            .addressW = VK_TEX_ADDRESS_CLAMP,
            .mipLodBias = 0.0f,
            .maxAnisotropy = 1,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 0.0f,
            .borderColor = VK_BORDER_COLOR_OPAQUE_WHITE,
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
            .minLod = 0.0f,
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

void demo_prepare_cube_data_buffer(struct demo *demo)
{
    VkBufferCreateInfo buf_info;
    VkBufferViewCreateInfo view_info;
    VkMemoryAllocInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memProps = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memPriority = VK_MEMORY_PRIORITY_NORMAL,
    };
    VkMemoryRequirements mem_reqs;
    size_t mem_reqs_size = sizeof(VkMemoryRequirements);
    uint8_t *pData;
    int i;
    mat4x4 MVP, VP;
    VkResult U_ASSERT_ONLY err;
    struct vktexcube_vs_uniform data;

    mat4x4_mul(VP, demo->projection_matrix, demo->view_matrix);
    mat4x4_mul(MVP, VP, demo->model_matrix);
    memcpy(data.mvp, MVP, sizeof(MVP));
//    dumpMatrix("MVP", MVP);

    for (i=0; i<12*3; i++) {
        data.position[i][0] = g_vertex_buffer_data[i*3];
        data.position[i][1] = g_vertex_buffer_data[i*3+1];
        data.position[i][2] = g_vertex_buffer_data[i*3+2];
        data.position[i][3] = 1.0f;
        data.attr[i][0] = g_uv_buffer_data[2*i];
        data.attr[i][1] = g_uv_buffer_data[2*i + 1];
        data.attr[i][2] = 0;
        data.attr[i][3] = 0;
    }

    memset(&buf_info, 0, sizeof(buf_info));
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = sizeof(data);
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    err = vkCreateBuffer(demo->device, &buf_info, &demo->uniform_data.buf);
    assert(!err);

    err = vkGetObjectInfo(demo->device,
            VK_OBJECT_TYPE_BUFFER, demo->uniform_data.buf,
            VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
            &mem_reqs_size, &mem_reqs);
    assert(!err && mem_reqs_size == sizeof(mem_reqs));

    alloc_info.allocationSize = mem_reqs.size;

    err = vkAllocMemory(demo->device, &alloc_info, &(demo->uniform_data.mem));
    assert(!err);

    err = vkMapMemory(demo->device, demo->uniform_data.mem, 0, 0, 0, (void **) &pData);
    assert(!err);

    memcpy(pData, &data, sizeof data);

    err = vkUnmapMemory(demo->device, demo->uniform_data.mem);
    assert(!err);

    err = vkBindObjectMemory(demo->device,
            VK_OBJECT_TYPE_BUFFER, demo->uniform_data.buf,
            demo->uniform_data.mem, 0);
    assert(!err);

    memset(&view_info, 0, sizeof(view_info));
    view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_info.buffer = demo->uniform_data.buf;
    view_info.viewType = VK_BUFFER_VIEW_TYPE_RAW;
    view_info.offset = 0;
    view_info.range = sizeof(data);

    err = vkCreateBufferView(demo->device, &view_info, &demo->uniform_data.view);
    assert(!err);

    demo->uniform_data.desc.bufferView = demo->uniform_data.view;
}

static void demo_prepare_descriptor_layout(struct demo *demo)
{
    const VkDescriptorSetLayoutBinding layout_bindings[2] = {
        [0] = {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .arraySize = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = NULL,
        },
        [1] = {
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .arraySize = DEMO_TEXTURE_COUNT,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = NULL,
        },
    };
    const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 2,
        .pBinding = layout_bindings,
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
    VkShaderCreateInfo createInfo;
    VkShader shader;
    VkResult err;


    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    createInfo.pNext = NULL;

    if (!demo->use_glsl) {
        createInfo.codeSize = size;
        createInfo.pCode = code;
        createInfo.flags = 0;

        err = vkCreateShader(demo->device, &createInfo, &shader);
        if (err) {
            free((void *) createInfo.pCode);
        }
    } else {
        // Create fake SPV structure to feed GLSL
        // to the driver "under the covers"
        createInfo.codeSize = 3 * sizeof(uint32_t) + size + 1;
        createInfo.pCode = malloc(createInfo.codeSize);
        createInfo.flags = 0;

        /* try version 0 first: VkShaderStage followed by GLSL */
        ((uint32_t *) createInfo.pCode)[0] = ICD_SPV_MAGIC;
        ((uint32_t *) createInfo.pCode)[1] = 0;
        ((uint32_t *) createInfo.pCode)[2] = stage;
        memcpy(((uint32_t *) createInfo.pCode + 3), code, size + 1);

        err = vkCreateShader(demo->device, &createInfo, &shader);
        if (err) {
            free((void *) createInfo.pCode);
            return (VkShader) VK_NULL_HANDLE;
        }
    }
    return shader;
}

char *demo_read_spv(const char *filename, size_t *psize)
{
    long int size;
    size_t U_ASSERT_ONLY retval;
    void *shader_code;

    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;

    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);

    fseek(fp, 0L, SEEK_SET);

    shader_code = malloc(size);
    retval = fread(shader_code, size, 1, fp);
    assert(retval == 1);

    *psize = size;

    return shader_code;
}

static VkShader demo_prepare_vs(struct demo *demo)
{
    if (!demo->use_glsl) {
        void *vertShaderCode;
        size_t size;

        vertShaderCode = demo_read_spv("cube-vert.spv", &size);

        return demo_prepare_shader(demo, VK_SHADER_STAGE_VERTEX,
                                   vertShaderCode, size);
    } else {
        static const char *vertShaderText =
                "#version 140\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "#extension GL_ARB_shading_language_420pack : enable\n"
                "\n"
                "layout(binding = 0) uniform buf {\n"
                "        mat4 MVP;\n"
                "        vec4 position[12*3];\n"
                "        vec4 attr[12*3];\n"
                "} ubuf;\n"
                "\n"
                "layout (location = 0) out vec4 texcoord;\n"
                "\n"
                "void main() \n"
                "{\n"
                "   texcoord = ubuf.attr[gl_VertexID];\n"
                "   gl_Position = ubuf.MVP * ubuf.position[gl_VertexID];\n"
                "\n"
                "   // GL->VK conventions\n"
                "   gl_Position.y = -gl_Position.y;\n"
                "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
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

        fragShaderCode = demo_read_spv("cube-frag.spv", &size);

        return demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT,
                                   fragShaderCode, size);
    } else {
        static const char *fragShaderText =
                "#version 140\n"
                "#extension GL_ARB_separate_shader_objects : enable\n"
                "#extension GL_ARB_shading_language_420pack : enable\n"
                "layout (binding = 1) uniform sampler2D tex;\n"
                "\n"
                "layout (location = 0) in vec4 texcoord;\n"
                "layout (location = 0) out vec4 uFragColor;\n"
                "void main() {\n"
                "   uFragColor = texture(tex, texcoord.xy);\n"
                "}\n";

        return demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT,
                                   (const void *) fragShaderText,
                                   strlen(fragShaderText));
    }
}

static void demo_prepare_pipeline(struct demo *demo)
{
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineIaStateCreateInfo ia;
    VkPipelineRsStateCreateInfo rs;
    VkPipelineCbStateCreateInfo cb;
    VkPipelineDsStateCreateInfo ds;
    VkPipelineShaderStageCreateInfo vs;
    VkPipelineShaderStageCreateInfo fs;
    VkPipelineVpStateCreateInfo vp;
    VkPipelineMsStateCreateInfo ms;
    VkResult U_ASSERT_ONLY err;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = demo->pipeline_layout;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_IA_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RS_STATE_CREATE_INFO;
    rs.fillMode = VK_FILL_MODE_SOLID;
    rs.cullMode = VK_CULL_MODE_BACK;
    rs.frontFace = VK_FRONT_FACE_CCW;
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

    memset(&vs, 0, sizeof(vs));
    vs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.shader.stage = VK_SHADER_STAGE_VERTEX;
    vs.shader.shader = demo_prepare_vs(demo);
    assert(vs.shader.shader != VK_NULL_HANDLE);

    memset(&fs, 0, sizeof(fs));
    fs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs.shader.stage = VK_SHADER_STAGE_FRAGMENT;
    fs.shader.shader = demo_prepare_fs(demo);
    assert(fs.shader.shader != VK_NULL_HANDLE);

    memset(&ms, 0, sizeof(ms));
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO;
    ms.sampleMask = 1;
    ms.multisampleEnable = VK_FALSE;
    ms.samples = 1;

    pipeline.pNext = (const void *) &ia;
    ia.pNext = (const void *) &rs;
    rs.pNext = (const void *) &cb;
    cb.pNext = (const void *) &ms;
    ms.pNext = (const void *) &vp;
    vp.pNext = (const void *) &ds;
    ds.pNext = (const void *) &vs;
    vs.pNext = (const void *) &fs;

    err = vkCreateGraphicsPipeline(demo->device, &pipeline, &demo->pipeline);
    assert(!err);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_SHADER, vs.shader.shader);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_SHADER, fs.shader.shader);
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
    VkRect scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    viewport_create.pScissors = &scissor;

    memset(&raster, 0, sizeof(raster));
    raster.sType = VK_STRUCTURE_TYPE_DYNAMIC_RS_STATE_CREATE_INFO;
    raster.pointSize = 1.0;
    raster.lineWidth = 1.0;

    memset(&color_blend, 0, sizeof(color_blend));
    color_blend.sType = VK_STRUCTURE_TYPE_DYNAMIC_CB_STATE_CREATE_INFO;
    color_blend.blendConst[0] = 1.0f;
    color_blend.blendConst[1] = 1.0f;
    color_blend.blendConst[2] = 1.0f;
    color_blend.blendConst[3] = 1.0f;

    memset(&depth_stencil, 0, sizeof(depth_stencil));
    depth_stencil.sType = VK_STRUCTURE_TYPE_DYNAMIC_DS_STATE_CREATE_INFO;
    depth_stencil.minDepth = 0.0f;
    depth_stencil.maxDepth = 1.0f;
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
    const VkDescriptorTypeCount type_counts[2] = {
        [0] = {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .count = 1,
        },
        [1] = {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .count = DEMO_TEXTURE_COUNT,
        },
    };
    const VkDescriptorPoolCreateInfo descriptor_pool = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .count = 2,
        .pTypeCount = type_counts,
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
    VkWriteDescriptorSet writes[2];
    VkResult U_ASSERT_ONLY err;
    uint32_t count;
    uint32_t i;

    err = vkAllocDescriptorSets(demo->device, demo->desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            1, &demo->desc_layout,
            &demo->desc_set, &count);
    assert(!err && count == 1);

    vkClearDescriptorSets(demo->device, demo->desc_pool, 1, &demo->desc_set);

    memset(&tex_descs, 0, sizeof(tex_descs));
    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        tex_descs[i].sampler = demo->textures[i].sampler;
        tex_descs[i].imageView = demo->textures[i].view;
        tex_descs[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    memset(&writes, 0, sizeof(writes));

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].destSet = demo->desc_set;
    writes[0].count = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pDescriptors = &demo->uniform_data.desc;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].destSet = demo->desc_set;
    writes[1].destBinding = 1;
    writes[1].count = DEMO_TEXTURE_COUNT;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pDescriptors = tex_descs;

    err = vkUpdateDescriptorSets(demo->device, 2, writes, 0, NULL);
    assert(!err);
}

static void demo_prepare(struct demo *demo)
{
    const VkCmdBufferCreateInfo cmd = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,
        .pNext = NULL,
        .queueNodeIndex = demo->graphics_queue_node_index,
        .flags = 0,
    };
    VkResult U_ASSERT_ONLY err;

    demo_prepare_buffers(demo);
    demo_prepare_depth(demo);
    demo_prepare_textures(demo);
    demo_prepare_cube_data_buffer(demo);

    demo_prepare_descriptor_layout(demo);
    demo_prepare_pipeline(demo);
    demo_prepare_dynamic_states(demo);

    for (int i = 0; i < DEMO_BUFFER_COUNT; i++) {
        err = vkCreateCommandBuffer(demo->device, &cmd, &demo->buffers[i].cmd);
        assert(!err);
    }

    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);


    for (int i = 0; i < DEMO_BUFFER_COUNT; i++) {
        demo->current_buffer = i;
        demo_draw_build_cmd(demo, demo->buffers[i].cmd);
    }

    /*
     * Prepare functions above may generate pipeline commands
     * that need to be flushed before beginning the render loop.
     */
    demo_flush_init_cmd(demo);

    demo->current_buffer = 0;
	demo->prepared = true;
}

#ifdef _WIN32
static void demo_run(struct demo *demo)
{
    if (!demo->prepared)
        return;
    // Wait for work to finish before updating MVP.
    vkDeviceWaitIdle(demo->device);
    demo_update_data_buffer(demo);

    demo_draw(demo);

    // Wait for work to finish before updating MVP.
    vkDeviceWaitIdle(demo->device);
}

// On MS-Windows, make this a global, so it's available to WndProc()
struct demo demo;

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_CLOSE: 
        PostQuitMessage(0);
        break;
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
    uint8_t event_code = event->response_type & 0x7f;
    switch (event_code) {
    case XCB_EXPOSE:
        // TODO: Resize window
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

            switch (key->detail) {
            case 0x9:           // Escape
                demo->quit = true;
                break;
            case 0x71:          // left arrow key
                demo->spin_angle += demo->spin_increment;
                break;
            case 0x72:          // right arrow key
                demo->spin_angle -= demo->spin_increment;
                break;
            case 0x41:
                demo->pause = !demo->pause;
                break;
            }
        }
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

        if (demo->pause) {
            event = xcb_wait_for_event(demo->connection);
        } else {
            event = xcb_poll_for_event(demo->connection);
        }
        if (event) {
            demo_handle_event(demo, event);
            free(event);
        }

        // Wait for work to finish before updating MVP.
        vkDeviceWaitIdle(demo->device);
        demo_update_data_buffer(demo);

        demo_draw(demo);

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
                    XCB_EVENT_MASK_EXPOSURE;

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
    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count = 0;
    VkExtensionProperties *device_extensions;
    uint32_t device_extension_count = 0;
    size_t extSize = sizeof(uint32_t);
    uint32_t total_extension_count = 0;
    err = vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_COUNT, 0, &extSize, &total_extension_count);
    assert(!err);

    VkExtensionProperties extProp;
    extSize = sizeof(VkExtensionProperties);
    bool32_t WSIextFound = 0;
    instance_extensions = malloc(sizeof(VkExtensionProperties) * total_extension_count);
    device_extensions = malloc(sizeof(VkExtensionProperties) * total_extension_count);
    for (uint32_t i = 0; i < total_extension_count; i++) {
        err = vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_PROPERTIES, i, &extSize, &extProp);
        if (!strcmp("VK_WSI_LunarG", extProp.name)) {
            WSIextFound = 1;
            memcpy(&instance_extensions[instance_extension_count++], &extProp, sizeof(VkExtensionProperties));
            memcpy(&device_extensions[device_extension_count++], &extProp, sizeof(VkExtensionProperties));
        }
        if (!strcmp("Validation", extProp.name)) {
            memcpy(&instance_extensions[instance_extension_count++], &extProp, sizeof(VkExtensionProperties));
            memcpy(&device_extensions[device_extension_count++], &extProp, sizeof(VkExtensionProperties));
        }
        if (!strcmp(DEBUG_REPORT_EXTENSION_NAME, extProp.name)) {
            memcpy(&instance_extensions[instance_extension_count++], &extProp, sizeof(VkExtensionProperties));
        }
    }
    if (!WSIextFound) {
        ERR_EXIT("vkGetGlobalExtensionInfo failed to find the "
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
        .extensionCount = instance_extension_count,
        .pEnabledExtensions = instance_extensions,
    };
    const VkDeviceQueueCreateInfo queue = {
        .queueNodeIndex = 0,
        .queueCount = 1,
    };

    VkDeviceCreateInfo device = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 1,
        .pRequestedQueues = &queue,
        .extensionCount = device_extension_count,
        .pEnabledExtensions = device_extensions,
        .flags = 0,
    };
    uint32_t gpu_count;
    uint32_t i;
    size_t data_size;
    uint32_t queue_count;

    if (demo->validate) {
        inst_info.extensionCount = 3;
        device.extensionCount = 3;
    }

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


    gpu_count = 1;
    err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, &demo->gpu);
    assert(!err && gpu_count == 1);


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

    demo->fpGetDisplayInfoWSI = vkGetDeviceProcAddr(demo->device, "vkGetDisplayInfoWSI");
    if (demo->fpGetDisplayInfoWSI == NULL)
        ERR_EXIT("vkGetDeviceProcAddr failed to find vkGetDisplayInfoWSI",
                                 "vkGetDeviceProcAddr Failure");
    demo->fpCreateSwapChainWSI = vkGetDeviceProcAddr(demo->device, "vkCreateSwapChainWSI");
    if (demo->fpCreateSwapChainWSI == NULL)
        ERR_EXIT("vkGetDeviceProcAddr failed to find vkCreateSwapChainWSI",
                                 "vkGetDeviceProcAddr Failure");
    demo->fpDestroySwapChainWSI = vkGetDeviceProcAddr(demo->device, "vkDestroySwapChainWSI");
    if (demo->fpDestroySwapChainWSI == NULL)
        ERR_EXIT("vkGetDeviceProcAddr failed to find vkDestroySwapChainWSI",
                                 "vkGetDeviceProcAddr Failure");
    demo->fpGetSwapChainInfoWSI = vkGetDeviceProcAddr(demo->device, "vkGetSwapChainInfoWSI");
    if (demo->fpGetSwapChainInfoWSI == NULL)
        ERR_EXIT("vkGetDeviceProcAddr failed to find vkGetSwapChainInfoWSI",
                                 "vkGetDeviceProcAddr Failure");
    demo->fpQueuePresentWSI = vkGetDeviceProcAddr(demo->device, "vkQueuePresentWSI");
    if (demo->fpQueuePresentWSI == NULL)
        ERR_EXIT("vkGetDeviceProcAddr failed to find vkQueuePresentWSI",
                                 "vkGetDeviceProcAddr Failure");

    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES,
                        &data_size, NULL);
    assert(!err);

    demo->gpu_props = (VkPhysicalDeviceProperties *) malloc(data_size);
    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES,
                        &data_size, demo->gpu_props);
    assert(!err);

    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES,
                        &data_size, NULL);
    assert(!err);

    demo->queue_props = (VkPhysicalDeviceQueueProperties *) malloc(data_size);
    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES,
                        &data_size, demo->queue_props);
    assert(!err);
    queue_count = (uint32_t)(data_size / sizeof(VkPhysicalDeviceQueueProperties));
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


    // Get the VkDisplayWSI's associated with this physical device:
    VkDisplayWSI display;
    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_DISPLAY_PROPERTIES_WSI,
                                  &data_size, NULL);
    if (err != VK_SUCCESS) {
        printf("The Vulkan installable client driver (ICD) does not support "
               "querying\nfor the swap-chain image format.  Therefore, am "
               "hardcoding this\nformat to  VK_FORMAT_B8G8R8A8_UNORM.\n");
        fflush(stdout);
        demo->format = VK_FORMAT_B8G8R8A8_UNORM;
        return;
    }
    demo->display_props = (VkDisplayPropertiesWSI *) malloc(data_size);
    err = vkGetPhysicalDeviceInfo(demo->gpu, VK_PHYSICAL_DEVICE_INFO_TYPE_DISPLAY_PROPERTIES_WSI,
                                  &data_size, demo->display_props);
    assert(!err);
    demo->num_displays = data_size / sizeof(VkDisplayPropertiesWSI);
    // For now, simply use the first display (TODO: Enhance this for the
    // future):
    display = demo->display_props[0].display;

    // Get a VkFormat to use with the VkDisplayWSI we are using:
    err = demo->fpGetDisplayInfoWSI(display, VK_DISPLAY_INFO_TYPE_FORMAT_PROPERTIES_WSI,
                              &data_size, NULL);
    VkDisplayFormatPropertiesWSI* display_format_props =
        (VkDisplayFormatPropertiesWSI*) malloc(data_size);
    err = demo->fpGetDisplayInfoWSI(display, VK_DISPLAY_INFO_TYPE_FORMAT_PROPERTIES_WSI,
                              &data_size, display_format_props);
    // For now, simply use the first VkFormat (TODO: Enhance this for the
    // future):
    demo->format = display_format_props[0].swapChainFormat;
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

static void demo_init(struct demo *demo, int argc, char **argv)
{
    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};

    memset(demo, 0, sizeof(*demo));

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--use_staging") == 0) {
            demo->use_staging_buffer = true;
            continue;
        }
        if (strcmp(argv[i], "--use_glsl") == 0) {
            demo->use_glsl = true;
            continue;
        }
        if (strcmp(argv[i], "--validate") == 0) {
            demo->validate = true;
            continue;
        }

        fprintf(stderr, "Usage:\n  %s [--use_staging] [--validate}\n", APP_SHORT_NAME);
        fflush(stderr);
        exit(1);
    }

    demo_init_connection(demo);
    demo_init_vk(demo);

    demo->width = 500;
    demo->height = 500;

    demo->spin_angle = 0.01f;
    demo->spin_increment = 0.01f;
    demo->pause = false;

    mat4x4_perspective(demo->projection_matrix, (float)degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
    mat4x4_look_at(demo->view_matrix, eye, origin, up);
    mat4x4_identity(demo->model_matrix);
}

static void demo_cleanup(struct demo *demo)
{
    uint32_t i;

    demo->prepared = false;

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DESCRIPTOR_SET, demo->desc_set);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, demo->desc_pool);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DYNAMIC_VP_STATE, demo->viewport);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DYNAMIC_RS_STATE, demo->raster);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DYNAMIC_CB_STATE, demo->color_blend);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DYNAMIC_DS_STATE, demo->depth_stencil);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_PIPELINE, demo->pipeline);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_PIPELINE_LAYOUT, demo->pipeline_layout);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, demo->desc_layout);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_IMAGE_VIEW, demo->textures[i].view);
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_IMAGE, demo->textures[i].image);
        vkFreeMemory(demo->device, demo->textures[i].mem);
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_SAMPLER, demo->textures[i].sampler);
    }
    demo->fpDestroySwapChainWSI(demo->swap_chain);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_DEPTH_STENCIL_VIEW, demo->depth.view);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_IMAGE, demo->depth.image);
    vkFreeMemory(demo->device, demo->depth.mem);

    vkDestroyObject(demo->device, VK_OBJECT_TYPE_BUFFER_VIEW, demo->uniform_data.view);
    vkDestroyObject(demo->device, VK_OBJECT_TYPE_BUFFER, demo->uniform_data.buf);
    vkFreeMemory(demo->device, demo->uniform_data.mem);

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW, demo->buffers[i].view);
        vkDestroyObject(demo->device, VK_OBJECT_TYPE_COMMAND_BUFFER, demo->buffers[i].cmd);
    }

    vkDestroyDevice(demo->device);
    vkDestroyInstance(demo->inst);

#ifndef _WIN32
    xcb_destroy_window(demo->connection, demo->window);
    xcb_disconnect(demo->connection);
#endif // _WIN32
}

#ifdef _WIN32
extern int __getmainargs(
        int * _Argc,
        char *** _Argv,
        char *** _Env,
        int _DoWildCard,
        int * new_mode);

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR pCmdLine,
                   int nCmdShow)
{
    MSG msg;         // message
    bool done;        // flag saying when app is complete
    int argc;
    char** argv;
    char** env;
    int new_mode = 0;

    __getmainargs(&argc,&argv,&env,0,&new_mode);

    demo_init(&demo, argc, argv);
    demo.connection = hInstance;
    strncpy(demo.name, "cube", APP_NAME_STR_LEN);
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
int main(int argc, char **argv)
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
