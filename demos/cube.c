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
#include <vk_ext_khr_swapchain.h>
#include <vk_ext_khr_device_swapchain.h>
#include "vk_debug_report_lunarg.h"

#include "icd-spv.h"

#include "vk_sdk_platform.h"
#include "linmath.h"
#include <png.h>

#define DEMO_BUFFER_COUNT 2
#define DEMO_TEXTURE_COUNT 1
#define APP_SHORT_NAME "cube"
#define APP_LONG_NAME "The Vulkan Cube Demo Program"

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
        // We know that we're submitting queues without fences, ignore this warning
        if (strstr(pMsg, "vkQueueSubmit parameter, VkFence fence, is null pointer")){
            return false;
        }
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
    uint32_t graphics_queue_node_index;
    VkPhysicalDeviceProperties gpu_props;
    VkQueueFamilyProperties *queue_props;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VkFramebuffer framebuffer;
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
        VkBufferView view;
        VkDescriptorInfo desc;
    } uniform_data;

    VkCmdBuffer cmd;  // Buffer for initialization commands
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    VkDynamicViewportState dynamic_viewport;
    VkDynamicLineWidthState dynamic_line_width;
    VkDynamicDepthBiasState dynamic_depth_bias;
    VkDynamicBlendState dynamic_blend;
    VkDynamicDepthBoundsState dynamic_depth_bounds;
    VkDynamicStencilState dynamic_stencil;

    mat4x4 projection_matrix;
    mat4x4 view_matrix;
    mat4x4 model_matrix;

    float spin_angle;
    float spin_increment;
    bool pause;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    VkFramebuffer framebuffers[DEMO_BUFFER_COUNT];

    bool quit;
    int32_t curFrame;
    int32_t frameCount;
    bool validate;
    bool use_break;
    PFN_vkDbgCreateMsgCallback dbgCreateMsgCallback;
    PFN_vkDbgDestroyMsgCallback dbgDestroyMsgCallback;
    PFN_vkDbgMsgCallback dbgBreakCallback;
    VkDbgMsgCallback msg_callback;

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

    if (demo->cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(demo->cmd);
    assert(!err);

    const VkCmdBuffer cmd_bufs[] = { demo->cmd };
    VkFence nullFence = { VK_NULL_HANDLE };

    err = vkQueueSubmit(demo->queue, 1, cmd_bufs, nullFence);
    assert(!err);

    err = vkQueueWaitIdle(demo->queue);
    assert(!err);

    vkDestroyCommandBuffer(demo->device, demo->cmd);
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
            .cmdPool = demo->cmd_pool,
            .level = VK_CMD_BUFFER_LEVEL_PRIMARY,
            .flags = 0,
        };

        err = vkCreateCommandBuffer(demo->device, &cmd, &demo->cmd);
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

    VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(demo->cmd, src_stages, dest_stages, false, 1, (const void * const*)&pmemory_barrier);
}

static void demo_draw_build_cmd(struct demo *demo, VkCmdBuffer cmd_buf)
{
    const VkCmdBufferBeginInfo cmd_buf_info = {
        .sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT,
        .renderPass = { VK_NULL_HANDLE },
        .subpass = 0,
        .framebuffer = { VK_NULL_HANDLE },
    };
    const VkClearValue clear_values[2] = {
        [0] = { .color.float32 = { 0.2f, 0.2f, 0.2f, 0.2f } },
        [1] = { .depthStencil = { 1.0f, 0 } },
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

    err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);
    assert(!err);

    vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  demo->pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline_layout,
            0, 1, &demo->desc_set, 0, NULL);

    vkCmdBindDynamicViewportState(cmd_buf, demo->dynamic_viewport);
    vkCmdBindDynamicLineWidthState(cmd_buf,  demo->dynamic_line_width);
    vkCmdBindDynamicDepthBiasState(cmd_buf,  demo->dynamic_depth_bias);
    vkCmdBindDynamicBlendState(cmd_buf, demo->dynamic_blend);
    vkCmdBindDynamicDepthBoundsState(cmd_buf, demo->dynamic_depth_bounds);
    vkCmdBindDynamicStencilState(cmd_buf, demo->dynamic_stencil);

    vkCmdDraw(cmd_buf, 0, 12 * 3, 0, 1);
    vkCmdEndRenderPass(cmd_buf);

    err = vkEndCommandBuffer(cmd_buf);
    assert(!err);
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

    vkUnmapMemory(demo->device, demo->uniform_data.mem);
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
    VkFence nullFence = { VK_NULL_HANDLE };

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
    err = vkQueueSubmit(demo->queue, 1, &demo->buffers[demo->current_buffer].cmd,
            nullFence);
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

    // Check the surface properties and formats
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

#define WORK_AROUND_CODE
#ifdef WORK_AROUND_CODE
    // After the proper code was created, other parts of this demo were
    // modified to only support DEMO_BUFFER_COUNT number of command buffers,
    // images, etc.  Live with that for now.
    // TODO: Rework this demo code to live with the number of buffers returned
    // by vkCreateSwapchainKHR().
    uint32_t desiredNumberOfSwapchainImages = DEMO_BUFFER_COUNT;
#else  // WORK_AROUND_CODE
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
#endif // WORK_AROUND_CODE

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfProperties.supportedTransforms & VK_SURFACE_TRANSFORM_NONE_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_NONE_KHR;
    } else {
        preTransform = surfProperties.currentTransform;
    }

    const VkSwapchainCreateInfoKHR swap_chain = {
        .sType = VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_KHR,
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
#ifdef WORK_AROUND_CODE
    // After the proper code was created, other parts of this demo were
    // modified to only support DEMO_BUFFER_COUNT number of command buffers,
    // images, etc.  Live with that for now.
    // TODO: Rework this demo code to live with the number of buffers returned
    // by vkCreateSwapchainKHR().
    demo->swapchainImageCount = DEMO_BUFFER_COUNT;
#endif // WORK_AROUND_CODE

    demo->buffers = (SwapchainBuffers*)malloc(sizeof(SwapchainBuffers)*demo->swapchainImageCount);
    assert(demo->buffers);

    for (i = 0; i < demo->swapchainImageCount; i++) {
        VkImageViewCreateInfo color_image_view = {
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
                .aspect = VK_IMAGE_ASPECT_COLOR,
                .baseMipLevel = 0,
                .mipLevels = 1,
                .baseArraySlice = 0,
                .arraySize = 1
            },
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .flags = 0,
        };

        demo->buffers[i].image = swapchainImages[i];

        demo_set_image_layout(demo, demo->buffers[i].image,
                               VK_IMAGE_ASPECT_COLOR,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        color_image_view.image = demo->buffers[i].image;

        err = vkCreateImageView(demo->device,
                &color_image_view, &demo->buffers[i].view);
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
            .aspect = VK_IMAGE_ASPECT_DEPTH,
            .baseMipLevel = 0,
            .mipLevels = 1,
            .baseArraySlice = 0,
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

    err = vkGetImageMemoryRequirements(demo->device,
                    demo->depth.image, &mem_reqs);

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
                           VK_IMAGE_ASPECT_DEPTH,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    /* create image view */
    view.image = demo->depth.image;
    err = vkCreateImageView(demo->device, &view, &demo->depth.view);
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
        .memoryTypeIndex = 0,
    };

    VkMemoryRequirements mem_reqs;

    err = vkCreateImage(demo->device, &image_create_info,
            &tex_obj->image);
    assert(!err);

    err = vkGetImageMemoryRequirements(demo->device, tex_obj->image, &mem_reqs);
    assert(!err);

    mem_alloc.allocationSize = mem_reqs.size;

    err = memory_type_from_properties(demo, mem_reqs.memoryTypeBits, mem_props, &mem_alloc.memoryTypeIndex);
    assert(!err);

    /* allocate memory */
    err = vkAllocMemory(demo->device, &mem_alloc,
                &(tex_obj->mem));
    assert(!err);

    /* bind memory */
    err = vkBindImageMemory(demo->device, tex_obj->image,
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

        err = vkGetImageSubresourceLayout(demo->device, tex_obj->image, &subres, &layout);
        assert(!err);

        err = vkMapMemory(demo->device, tex_obj->mem, 0, 0, 0, &data);
        assert(!err);

        if (!loadTexture(filename, data, &layout, &tex_width, &tex_height)) {
            fprintf(stderr, "Error loading texture: %s\n", filename);
        }

        vkUnmapMemory(demo->device, tex_obj->mem);
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
    vkDestroyImage(demo->device, tex_objs->image);
}

static void demo_prepare_textures(struct demo *demo)
{
    const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties props;
    VkResult U_ASSERT_ONLY err;
    uint32_t i;

    err = vkGetPhysicalDeviceFormatProperties(demo->gpu, tex_format, &props);
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
            .addressModeU = VK_TEX_ADDRESS_MODE_CLAMP,
            .addressModeV = VK_TEX_ADDRESS_MODE_CLAMP,
            .addressModeW = VK_TEX_ADDRESS_MODE_CLAMP,
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
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR, 0, 1, 0, 1 },
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

void demo_prepare_cube_data_buffer(struct demo *demo)
{
    VkBufferCreateInfo buf_info;
    VkBufferViewCreateInfo view_info;
    VkMemoryAllocInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext = NULL,
        .allocationSize = 0,
        .memoryTypeIndex = 0,
    };
    VkMemoryRequirements mem_reqs;
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
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(data);
    err = vkCreateBuffer(demo->device, &buf_info, &demo->uniform_data.buf);
    assert(!err);

    err = vkGetBufferMemoryRequirements(demo->device, demo->uniform_data.buf, &mem_reqs);
    assert(!err);

    alloc_info.allocationSize = mem_reqs.size;
    err = memory_type_from_properties(demo,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(!err);

    err = vkAllocMemory(demo->device, &alloc_info, &(demo->uniform_data.mem));
    assert(!err);

    err = vkMapMemory(demo->device, demo->uniform_data.mem, 0, 0, 0, (void **) &pData);
    assert(!err);

    memcpy(pData, &data, sizeof data);

    vkUnmapMemory(demo->device, demo->uniform_data.mem);

    err = vkBindBufferMemory(demo->device,
            demo->uniform_data.buf,
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

static VkShader demo_prepare_shader(struct demo* demo,
                                      VkShaderStage stage,
                                      VkShaderModule* pShaderModule,
                                      const void* code,
                                      size_t size)
{
    VkShaderModuleCreateInfo moduleCreateInfo;
    VkShaderCreateInfo shaderCreateInfo;
    VkShader shader;
    VkResult err;


    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;

    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    shaderCreateInfo.pNext = NULL;
    shaderCreateInfo.pName = "main";

    if (!demo->use_glsl) {
        moduleCreateInfo.codeSize = size;
        moduleCreateInfo.pCode = code;
        moduleCreateInfo.flags = 0;
        err = vkCreateShaderModule(demo->device, &moduleCreateInfo, pShaderModule);
        if (err) {
            free((void *) moduleCreateInfo.pCode);
        }

        shaderCreateInfo.flags = 0;
        shaderCreateInfo.module = *pShaderModule;
        shaderCreateInfo.pName = "main";
        shaderCreateInfo.stage = stage;
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

        err = vkCreateShaderModule(demo->device, &moduleCreateInfo, pShaderModule);
        if (err) {
            free((void *) moduleCreateInfo.pCode);
        }

        shaderCreateInfo.flags = 0;
        shaderCreateInfo.module = *pShaderModule;
        shaderCreateInfo.pName = "main";
        shaderCreateInfo.stage = stage;
        err = vkCreateShader(demo->device, &shaderCreateInfo, &shader);
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

        return demo_prepare_shader(demo, VK_SHADER_STAGE_VERTEX, &demo->vert_shader_module,
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

        return demo_prepare_shader(demo, VK_SHADER_STAGE_VERTEX, &demo->vert_shader_module,
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

        return demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT, &demo->frag_shader_module,
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

        return demo_prepare_shader(demo, VK_SHADER_STAGE_FRAGMENT, &demo->frag_shader_module,
                                   (const void *) fragShaderText,
                                   strlen(fragShaderText));
    }
}

static void demo_prepare_pipeline(struct demo *demo)
{
    VkGraphicsPipelineCreateInfo pipeline;
    VkPipelineCacheCreateInfo pipelineCache;
    VkPipelineInputAssemblyStateCreateInfo ia;
    VkPipelineRasterStateCreateInfo        rs;
    VkPipelineColorBlendStateCreateInfo    cb;
    VkPipelineDepthStencilStateCreateInfo  ds;
    VkPipelineViewportStateCreateInfo      vp;
    VkPipelineMultisampleStateCreateInfo   ms;
    VkResult U_ASSERT_ONLY err;

    memset(&pipeline, 0, sizeof(pipeline));
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.layout = demo->pipeline_layout;

    memset(&ia, 0, sizeof(ia));
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    memset(&rs, 0, sizeof(rs));
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTER_STATE_CREATE_INFO;
    rs.fillMode = VK_FILL_MODE_SOLID;
    rs.cullMode = VK_CULL_MODE_BACK;
    rs.frontFace = VK_FRONT_FACE_CCW;
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

    shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX;
    shaderStages[0].shader = demo_prepare_vs(demo);

    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT;
    shaderStages[1].shader = demo_prepare_fs(demo);

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

    err = vkCreatePipelineCache(demo->device, &pipelineCache, &demo->pipelineCache);
    assert(!err);

    pipeline.pVertexInputState   = NULL;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterState        = &rs;
    pipeline.pColorBlendState    = &cb;
    pipeline.pMultisampleState   = &ms;
    pipeline.pViewportState      = &vp;
    pipeline.pDepthStencilState  = &ds;
    pipeline.pStages             = shaderStages;
    pipeline.renderPass          = demo->render_pass;

    pipeline.renderPass = demo->render_pass;

    err = vkCreateGraphicsPipelines(demo->device, demo->pipelineCache, 1, &pipeline, &demo->pipeline);
    assert(!err);

    for (uint32_t i = 0; i < pipeline.stageCount; i++) {
        vkDestroyShader(demo->device, shaderStages[i].shader);
    }
    vkDestroyShaderModule(demo->device, demo->frag_shader_module);
    vkDestroyShaderModule(demo->device, demo->vert_shader_module);
}

static void demo_prepare_dynamic_states(struct demo *demo)
{
    VkDynamicViewportStateCreateInfo viewport_create;
    VkDynamicLineWidthStateCreateInfo line_width;
    VkDynamicDepthBiasStateCreateInfo depth_bias;
    VkDynamicBlendStateCreateInfo blend;
    VkDynamicDepthBoundsStateCreateInfo depth_bounds;
    VkDynamicStencilStateCreateInfo stencil;
    VkResult U_ASSERT_ONLY err;

    memset(&viewport_create, 0, sizeof(viewport_create));
    viewport_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_VIEWPORT_STATE_CREATE_INFO;
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

    memset(&line_width, 0, sizeof(line_width));
    line_width.sType = VK_STRUCTURE_TYPE_DYNAMIC_LINE_WIDTH_STATE_CREATE_INFO;
    line_width.lineWidth = 1.0;

    memset(&depth_bias, 0, sizeof(depth_bias));
    depth_bias.sType = VK_STRUCTURE_TYPE_DYNAMIC_DEPTH_BIAS_STATE_CREATE_INFO;
    depth_bias.depthBias = 0.0f;
    depth_bias.depthBiasClamp = 0.0f;
    depth_bias.slopeScaledDepthBias = 0.0f;

    memset(&blend, 0, sizeof(blend));
    blend.sType = VK_STRUCTURE_TYPE_DYNAMIC_BLEND_STATE_CREATE_INFO;
    blend.blendConst[0] = 1.0f;
    blend.blendConst[1] = 1.0f;
    blend.blendConst[2] = 1.0f;
    blend.blendConst[3] = 1.0f;

    memset(&depth_bounds, 0, sizeof(depth_bounds));
    depth_bounds.sType = VK_STRUCTURE_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE_CREATE_INFO;
    depth_bounds.minDepthBounds = 0.0f;
    depth_bounds.maxDepthBounds = 1.0f;

    memset(&stencil, 0, sizeof(stencil));
    stencil.sType = VK_STRUCTURE_TYPE_DYNAMIC_STENCIL_STATE_CREATE_INFO;
    stencil.stencilReference = 0;
    stencil.stencilCompareMask = 0xff;
    stencil.stencilWriteMask = 0xff;

    err = vkCreateDynamicViewportState(demo->device, &viewport_create, &demo->dynamic_viewport);
    assert(!err);

    err = vkCreateDynamicLineWidthState(demo->device, &line_width, &demo->dynamic_line_width);
    assert(!err);

    err = vkCreateDynamicDepthBiasState(demo->device, &depth_bias, &demo->dynamic_depth_bias);
    assert(!err);

    err = vkCreateDynamicBlendState(demo->device,
            &blend, &demo->dynamic_blend);
    assert(!err);

    err = vkCreateDynamicDepthBoundsState(demo->device,
            &depth_bounds, &demo->dynamic_depth_bounds);
    assert(!err);

    err = vkCreateDynamicStencilState(demo->device,
            &stencil, &stencil, &demo->dynamic_stencil);
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

    vkUpdateDescriptorSets(demo->device, 2, writes, 0, NULL);
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
        attachments[0] = demo->buffers[i].view;
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

    demo_prepare_buffers(demo);
    demo_prepare_depth(demo);
    demo_prepare_textures(demo);
    demo_prepare_cube_data_buffer(demo);

    demo_prepare_descriptor_layout(demo);
    demo_prepare_render_pass(demo);
    demo_prepare_pipeline(demo);
    demo_prepare_dynamic_states(demo);

    for (uint32_t i = 0; i < demo->swapchainImageCount; i++) {
        err = vkCreateCommandBuffer(demo->device, &cmd, &demo->buffers[i].cmd);
        assert(!err);
    }

    demo_prepare_descriptor_pool(demo);
    demo_prepare_descriptor_set(demo);

    demo_prepare_framebuffers(demo);

    for (uint32_t i = 0; i < demo->swapchainImageCount; i++) {
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

static void demo_cleanup(struct demo *demo)
{
    uint32_t i;

    demo->prepared = false;

    for (i = 0; i < DEMO_BUFFER_COUNT; i++) {
        vkDestroyFramebuffer(demo->device, demo->framebuffers[i]);
    }
    vkFreeDescriptorSets(demo->device, demo->desc_pool, 1, &demo->desc_set);
    vkDestroyDescriptorPool(demo->device, demo->desc_pool);

    vkDestroyDynamicViewportState(demo->device, demo->dynamic_viewport);
    vkDestroyDynamicLineWidthState(demo->device, demo->dynamic_line_width);
    vkDestroyDynamicDepthBiasState(demo->device, demo->dynamic_depth_bias);
    vkDestroyDynamicBlendState(demo->device, demo->dynamic_blend);
    vkDestroyDynamicDepthBoundsState(demo->device, demo->dynamic_depth_bounds);
    vkDestroyDynamicStencilState(demo->device, demo->dynamic_stencil);

    vkDestroyPipeline(demo->device, demo->pipeline);
    vkDestroyPipelineCache(demo->device, demo->pipelineCache);
    vkDestroyRenderPass(demo->device, demo->render_pass);
    vkDestroyPipelineLayout(demo->device, demo->pipeline_layout);
    vkDestroyDescriptorSetLayout(demo->device, demo->desc_layout);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++) {
        vkDestroyImageView(demo->device, demo->textures[i].view);
        vkDestroyImage(demo->device, demo->textures[i].image);
        vkFreeMemory(demo->device, demo->textures[i].mem);
        vkDestroySampler(demo->device, demo->textures[i].sampler);
    }
    demo->fpDestroySwapchainKHR(demo->device, demo->swap_chain);

    vkDestroyImageView(demo->device, demo->depth.view);
    vkDestroyImage(demo->device, demo->depth.image);
    vkFreeMemory(demo->device, demo->depth.mem);

    vkDestroyBufferView(demo->device, demo->uniform_data.view);
    vkDestroyBuffer(demo->device, demo->uniform_data.buf);
    vkFreeMemory(demo->device, demo->uniform_data.mem);

    for (i = 0; i < demo->swapchainImageCount; i++) {
        vkDestroyImageView(demo->device, demo->buffers[i].view);
        vkDestroyCommandBuffer(demo->device, demo->buffers[i].cmd);
    }
    free(demo->buffers);

    vkDestroyCommandPool(demo->device, demo->cmd_pool);
    vkDestroyDevice(demo->device);
    if (demo->validate) {
        demo->dbgDestroyMsgCallback(demo->inst, demo->msg_callback);
    }
    vkDestroyInstance(demo->inst);

#ifndef _WIN32
    xcb_destroy_window(demo->connection, demo->window);
    xcb_disconnect(demo->connection);
#endif // _WIN32
}

// On MS-Windows, make this a global, so it's available to WndProc()
struct demo demo;

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

    demo->curFrame++;

    if (demo->frameCount != INT_MAX && demo->curFrame == demo->frameCount)
    {
        demo->quit=true;
        demo_cleanup(demo);
        ExitProcess(0);
    }

}

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
        demo->curFrame++;
        if (demo->frameCount != INT_MAX && demo->curFrame == demo->frameCount)
            demo->quit = true;

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

    // Force the x/y coordinates to 100,100 results are identical in consecutive runs
    const uint32_t coords[] = {100,  100};
    xcb_configure_window(demo->connection, demo->window,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
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

static void demo_init_vk(struct demo *demo)
{
    VkResult err;
    char *extension_names[64];
    VkExtensionProperties *instance_extensions;
    VkPhysicalDevice *physical_devices;
    VkLayerProperties *instance_layers;
    VkLayerProperties *device_layers;
    uint32_t instance_extension_count = 0;
    uint32_t instance_layer_count = 0;
    uint32_t enabled_extension_count = 0;
    uint32_t enabled_layer_count = 0;

    char *instance_validation_layers[] = {
        "Threading",
        "MemTracker",
        "ObjectTracker",
        "DrawState",
        "ParamChecker",
        "ShaderChecker",
    };

    char *device_validation_layers[] = {
        "Threading",
        "MemTracker",
        "ObjectTracker",
        "DrawState",
        "ParamChecker",
        "ShaderChecker",
    };

    /* Look for validation layers */
    VkBool32 validation_found = 0;
    err = vkGetGlobalLayerProperties(&instance_layer_count, NULL);
    assert(!err);

    instance_layers = malloc(sizeof(VkLayerProperties) * instance_layer_count);
    err = vkGetGlobalLayerProperties(&instance_layer_count, instance_layers);
    assert(!err);

    if (demo->validate) {
        validation_found = demo_check_layers(ARRAY_SIZE(instance_validation_layers), instance_validation_layers,
                                             instance_layer_count, instance_layers);
        if (!validation_found) {
            ERR_EXIT("vkGetGlobalLayerProperties failed to find"
                     "required validation layer.\n\n"
                     "Please look at the Getting Started guide for additional "
                     "information.\n",
                     "vkCreateInstance Failure");
        }
        enabled_layer_count = ARRAY_SIZE(instance_validation_layers);
    }

    err = vkGetGlobalExtensionProperties(NULL, &instance_extension_count, NULL);
    assert(!err);

    VkBool32 WSIextFound = 0;
    memset(extension_names, 0, sizeof(extension_names));
    instance_extensions = malloc(sizeof(VkExtensionProperties) * instance_extension_count);
    err = vkGetGlobalExtensionProperties(NULL, &instance_extension_count, instance_extensions);
    assert(!err);
    for (uint32_t i = 0; i < instance_extension_count; i++) {
        if (!strcmp("VK_EXT_KHR_swapchain", instance_extensions[i].extName)) {
            WSIextFound = 1;
            extension_names[enabled_extension_count++] = "VK_EXT_KHR_swapchain";
        }
        if (!strcmp(VK_DEBUG_REPORT_EXTENSION_NAME, instance_extensions[i].extName)) {
            if (demo->validate) {
                extension_names[enabled_extension_count++] = VK_DEBUG_REPORT_EXTENSION_NAME;
            }
        }
        assert(enabled_extension_count < 64);
    }
    if (!WSIextFound) {
        ERR_EXIT("vkGetGlobalExtensionProperties failed to find the "
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
    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pAppInfo = &app,
        .pAllocCb = NULL,
        .layerCount = enabled_layer_count,
        .ppEnabledLayerNames = (const char *const*) ((demo->validate) ? instance_validation_layers : NULL),
        .extensionCount = enabled_extension_count,
        .ppEnabledExtensionNames = (const char *const*) extension_names,
    };
    const VkDeviceQueueCreateInfo queue = {
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

    /* Make initial call to query gpu_count, then second call for gpu info*/
    err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, NULL);
    assert(!err && gpu_count > 0);
    physical_devices = malloc(sizeof(VkPhysicalDevice) * gpu_count);
    err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, physical_devices);
    assert(!err);
    /* For cube demo we just grab the first physical device */
    demo->gpu = physical_devices[0];
    free(physical_devices);

    /* Look for validation layers */
    validation_found = 0;
    enabled_layer_count = 0;
    uint32_t device_layer_count = 0;
    err = vkGetPhysicalDeviceLayerProperties(demo->gpu, &device_layer_count, NULL);
    assert(!err);

    device_layers = malloc(sizeof(VkLayerProperties) * device_layer_count);
    err = vkGetPhysicalDeviceLayerProperties(demo->gpu, &device_layer_count, device_layers);
    assert(!err);

    if (demo->validate) {
        validation_found = demo_check_layers(ARRAY_SIZE(device_validation_layers), device_validation_layers,
                                             device_layer_count, device_layers);
        if (!validation_found) {
            ERR_EXIT("vkGetPhysicalDeviceLayerProperties failed to find"
                     "a required validation layer.\n\n"
                     "Please look at the Getting Started guide for additional "
                     "information.\n",
                     "vkCreateDevice Failure");
        }
        enabled_layer_count = ARRAY_SIZE(device_validation_layers);
    }

    uint32_t device_extension_count = 0;
    VkExtensionProperties *device_extensions = NULL;
    err = vkGetPhysicalDeviceExtensionProperties(
              demo->gpu, NULL, &device_extension_count, NULL);
    assert(!err);

    WSIextFound = 0;
    enabled_extension_count = 0;
    memset(extension_names, 0, sizeof(extension_names));
    device_extensions = malloc(sizeof(VkExtensionProperties) * device_extension_count);
    err = vkGetPhysicalDeviceExtensionProperties(
              demo->gpu, NULL, &device_extension_count, device_extensions);
    assert(!err);

    for (uint32_t i = 0; i < device_extension_count; i++) {
        if (!strcmp("VK_EXT_KHR_device_swapchain", device_extensions[i].extName)) {
            WSIextFound = 1;
            extension_names[enabled_extension_count++] = "VK_EXT_KHR_device_swapchain";
        }
        assert(enabled_extension_count < 64);
    }
    if (!WSIextFound) {
        ERR_EXIT("vkGetPhysicalDeviceExtensionProperties failed to find the "
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
        demo->dbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback) vkGetInstanceProcAddr(demo->inst, "vkDbgDestroyMsgCallback");
        if (!demo->dbgCreateMsgCallback) {
            ERR_EXIT("GetProcAddr: Unable to find vkDbgCreateMsgCallback\n",
                     "vkGetProcAddr Failure");
        }
        if (!demo->dbgDestroyMsgCallback) {
            ERR_EXIT("GetProcAddr: Unable to find vkDbgDestroyMsgCallback\n",
                     "vkGetProcAddr Failure");
        }
        demo->dbgBreakCallback = (PFN_vkDbgMsgCallback) vkGetInstanceProcAddr(demo->inst, "vkDbgBreakCallback");
        if (!demo->dbgBreakCallback) {
            ERR_EXIT("GetProcAddr: Unable to find vkDbgBreakCallback\n",
                     "vkGetProcAddr Failure");
        }

        PFN_vkDbgMsgCallback callback;

        if (!demo->use_break) {
            callback = dbgFunc;
        } else {
            callback = demo->dbgBreakCallback;
        }
        err = demo->dbgCreateMsgCallback(
                  demo->inst,
                  VK_DBG_REPORT_ERROR_BIT | VK_DBG_REPORT_WARN_BIT,
                  callback, NULL,
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

    free(device_layers);

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
    assert(!err);

    /* Call with NULL data to get count */
    err = vkGetPhysicalDeviceQueueFamilyProperties(demo->gpu, &demo->queue_count, NULL);
    assert(!err);
    assert(demo->queue_count >= 1);

    demo->queue_props = (VkQueueFamilyProperties *) malloc(demo->queue_count * sizeof(VkQueueFamilyProperties));
    err = vkGetPhysicalDeviceQueueFamilyProperties(demo->gpu, &demo->queue_count, demo->queue_props);
    assert(!err);
    assert(demo->queue_count >= 1);
}

static void demo_init_vk_wsi(struct demo *demo)
{
    VkResult err;
    uint32_t i;

    // Construct the WSI surface description:
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

    // Iterate over each queue to learn whether it supports presenting to WSI:
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
                 "WSI Initialization Failure");
    }

    // TODO: Add support for separate queues, including presentation,
    //       synchronization, and appropriate tracking for QueueSubmit
    // While it is possible for an application to use a separate graphics and a
    // present queues, this demo program assumes it is only using one:
    if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
        ERR_EXIT("Could not find a common graphics and a present queue\n",
                 "WSI Initialization Failure");
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

    demo->quit = false;
    demo->curFrame = 0;

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

static void demo_init(struct demo *demo, int argc, char **argv)
{
    vec3 eye = {0.0f, 3.0f, 5.0f};
    vec3 origin = {0, 0, 0};
    vec3 up = {0.0f, 1.0f, 0.0};

    memset(demo, 0, sizeof(*demo));
    demo->frameCount = INT_MAX;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--use_staging") == 0) {
            demo->use_staging_buffer = true;
            continue;
        }
        if (strcmp(argv[i], "--use_glsl") == 0) {
            demo->use_glsl = true;
            continue;
        }
        if (strcmp(argv[i], "--break") == 0) {
            demo->use_break = true;
            continue;
        }
        if (strcmp(argv[i], "--validate") == 0) {
            demo->validate = true;
            continue;
        }
        if (strcmp(argv[i], "--c") == 0 &&
            demo->frameCount == INT_MAX &&
            i < argc-1 &&
            sscanf(argv[i+1],"%d", &demo->frameCount) == 1 &&
            demo->frameCount >= 0)
        {
            i++;
            continue;
        }

        fprintf(stderr, "Usage:\n  %s [--use_staging] [--validate] [--break] [--c <framecount>]\n", APP_SHORT_NAME);
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
    demo_init_vk_wsi(&demo);

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
    demo_init_vk_wsi(&demo);

    demo_prepare(&demo);
    demo_run(&demo);

    demo_cleanup(&demo);

    return 0;
}
#endif // _WIN32
