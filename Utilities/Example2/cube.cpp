/*
* Copyright (c) 2015-2016 The Khronos Group Inc.
* Copyright (c) 2015-2016 Valve Corporation
* Copyright (c) 2015-2016 LunarG, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: Jeremy Hayes <jeremy@lunarg.com>
*/

#include "cube.h"

#if defined(WIN32)
#pragma warning(disable: 4996)
#include <windows.h>
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xutil.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <memory>

#if defined(VK_USE_PLATFORM_MIR_KHR)
#warning "Cubepp does not have code for Mir at this time"
#endif

#define VULKAN_HPP_TYPESAFE_CONVERSION
#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>
#include <vulkan/vk_sdk_platform.h>

#include "linmath.h"

#ifndef NDEBUG
#define VERIFY(x) assert(x)
#else
#define VERIFY(x) ((void)(x))
#endif

#define APP_SHORT_NAME "cube"
#ifdef _WIN32
#define APP_NAME_STR_LEN 80
#endif

// Allow a maximum of two outstanding presentation operations.
#define FRAME_LAG 2

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#ifdef _WIN32
#define ERR_EXIT(err_msg, err_class)                                          \
    do {                                                                      \
        if (!suppress_popups) MessageBox(nullptr, err_msg, err_class, MB_OK); \
        exit(1);                                                              \
    } while (0)
#else
#define ERR_EXIT(err_msg, err_class) \
    do {                             \
        printf("%s\n", err_msg);     \
        fflush(stdout);              \
        exit(1);                     \
    } while (0)
#endif

struct texture_object {
    vk::Sampler sampler;

    vk::Image image;
    vk::ImageLayout imageLayout{vk::ImageLayout::eUndefined};

    vk::MemoryAllocateInfo mem_alloc;
    vk::DeviceMemory mem;
    vk::ImageView view;

    int32_t tex_width{0};
    int32_t tex_height{0};
};

static char const *const tex_files[] = {"lunarg.ppm"};

//static int validation_error = 0;

struct vkcube_vs_uniform {
    // Must start with MVP
    float mvp[4][4];
    float position[12 * 3][4];
    float color[12 * 3][4];
};

struct vktexcube_vs_uniform {
    // Must start with MVP
    float mvp[4][4];
    float position[12 * 3][4];
    float attr[12 * 3][4];
};

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
// clang-format off
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
    0.0f, 1.0f,  // -X side
    1.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Z side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Y side
    1.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // +Y side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 0.0f,  // +X side
    0.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // +Z side
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
};
// clang-format on

typedef struct {
    vk::Image image;
    vk::CommandBuffer cmd;
    vk::CommandBuffer graphics_to_present_cmd;
    vk::ImageView view;
} SwapchainBuffers;

#ifdef _WIN32
// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
static void handle_ping(void *data, wl_shell_surface *shell_surface, uint32_t serial) {
    wl_shell_surface_pong(shell_surface, serial);
}

static void handle_configure(void *data, wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {}

static void handle_popup_done(void *data, wl_shell_surface *shell_surface) {}

static const wl_shell_surface_listener shell_surface_listener = {handle_ping, handle_configure, handle_popup_done};

static void handle_announce_global_object(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface,
                                          uint32_t version) {}

static void handle_announce_global_object_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {}

static const wl_registry_listener registry_listener = {handle_announce_global_object, handle_announce_global_object_remove};
#elif defined(VK_USE_PLATFORM_MIR_KHR)
#endif

struct Demo {
    Demo()
        :
#if defined(VK_USE_PLATFORM_WIN32_KHR)
          connection{nullptr},
          window{nullptr},
          minsize(POINT{0, 0}),  // Use explicit construction to avoid MSVC error C2797.
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
          xlib_window{0},
          xlib_wm_delete_window{0},
          display{nullptr},
#elif defined(VK_USE_PLATFORM_XCB_KHR)
          xcb_window{0},
          screen{nullptr},
          connection{nullptr},
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
          display{nullptr},
          registry{nullptr},
          compositor{nullptr},
          window{nullptr},
          shell{nullptr},
          shell_surface{nullptr},
#elif defined(VK_USE_PLATFORM_MIR_KHR)
#endif
          prepared{false},
          use_staging_buffer{false},
          use_xlib{false},
          graphics_queue_family_index{0},
          present_queue_family_index{0},
          enabled_extension_count{0},
          enabled_layer_count{0},
          width{0},
          height{0},
          swapchainImageCount{0},
          frame_index{0},
          spin_angle{0.0f},
          spin_increment{0.0f},
          pause{false},
          quit{false},
          curFrame{0},
          frameCount{0},
          validate{false},
          use_break{false},
          suppress_popups{false},
          current_buffer{0},
          queue_family_count{0} {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        memset(name, '\0', APP_NAME_STR_LEN);
#endif
        memset(projection_matrix, 0, sizeof(projection_matrix));
        memset(view_matrix, 0, sizeof(view_matrix));
        memset(model_matrix, 0, sizeof(model_matrix));
    }

    void build_image_ownership_cmd(uint32_t const &i) {
        auto const cmd_buf_info = vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
        auto result = buffers[i].graphics_to_present_cmd.begin(&cmd_buf_info);
        VERIFY(result == vk::Result::eSuccess);

        auto const image_ownership_barrier =
            vk::ImageMemoryBarrier()
                .setSrcAccessMask(vk::AccessFlags())
                .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                .setOldLayout(vk::ImageLayout::ePresentSrcKHR)
                .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
                .setSrcQueueFamilyIndex(graphics_queue_family_index)
                .setDstQueueFamilyIndex(present_queue_family_index)
                .setImage(buffers[i].image)
                .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        buffers[i].graphics_to_present_cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::DependencyFlagBits(), 0, nullptr, 0, nullptr, 1, &image_ownership_barrier);

        result = buffers[i].graphics_to_present_cmd.end();
        VERIFY(result == vk::Result::eSuccess);
    }

    vk::Bool32 check_layers(uint32_t check_count, char const *const *const check_names, uint32_t layer_count,
                            vk::LayerProperties *layers) {
        for (uint32_t i = 0; i < check_count; i++) {
            vk::Bool32 found = VK_FALSE;
            for (uint32_t j = 0; j < layer_count; j++) {
                if (!strcmp(check_names[i], layers[j].layerName)) {
                    found = VK_TRUE;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
                return 0;
            }
        }
        return VK_TRUE;
    }

    void cleanup() {
        prepared = false;
        device.waitIdle();

        // Wait for fences from present operations
        for (uint32_t i = 0; i < FRAME_LAG; i++) {
            device.waitForFences(1, &fences[i], VK_TRUE, UINT64_MAX);
            device.destroyFence(fences[i], nullptr);
            device.destroySemaphore(image_acquired_semaphores[i], nullptr);
            device.destroySemaphore(draw_complete_semaphores[i], nullptr);
            if (separate_present_queue) {
                device.destroySemaphore(image_ownership_semaphores[i], nullptr);
            }
        }

        for (uint32_t i = 0; i < swapchainImageCount; i++) {
            device.destroyFramebuffer(framebuffers[i], nullptr);
        }
        device.destroyDescriptorPool(desc_pool, nullptr);

        device.destroyPipeline(pipeline, nullptr);
        device.destroyPipelineCache(pipelineCache, nullptr);
        device.destroyRenderPass(render_pass, nullptr);
        device.destroyPipelineLayout(pipeline_layout, nullptr);
        device.destroyDescriptorSetLayout(desc_layout, nullptr);

        for (uint32_t i = 0; i < texture_count; i++) {
            device.destroyImageView(textures[i].view, nullptr);
            device.destroyImage(textures[i].image, nullptr);
            device.freeMemory(textures[i].mem, nullptr);
            device.destroySampler(textures[i].sampler, nullptr);
        }
        device.destroySwapchainKHR(swapchain, nullptr);

        device.destroyImageView(depth.view, nullptr);
        device.destroyImage(depth.image, nullptr);
        device.freeMemory(depth.mem, nullptr);

        device.destroyBuffer(uniform_data.buf, nullptr);
        device.freeMemory(uniform_data.mem, nullptr);

        for (uint32_t i = 0; i < swapchainImageCount; i++) {
            device.destroyImageView(buffers[i].view, nullptr);
            device.freeCommandBuffers(cmd_pool, 1, &buffers[i].cmd);
        }

        device.destroyCommandPool(cmd_pool, nullptr);

        if (separate_present_queue) {
            device.destroyCommandPool(present_cmd_pool, nullptr);
        }
        device.waitIdle();
        device.destroy(nullptr);
    }

    void create_device() {
        float const priorities[1] = {0.0};

        vk::DeviceQueueCreateInfo queues[2];
        queues[0].setQueueFamilyIndex(graphics_queue_family_index);
        queues[0].setQueueCount(1);
        queues[0].setPQueuePriorities(priorities);

        auto deviceInfo = vk::DeviceCreateInfo()
                              .setQueueCreateInfoCount(1)
                              .setPQueueCreateInfos(queues)
                              .setEnabledLayerCount(0)
                              .setPpEnabledLayerNames(nullptr)
                              .setEnabledExtensionCount(enabled_extension_count)
                              .setPpEnabledExtensionNames((const char *const *)extension_names)
                              .setPEnabledFeatures(nullptr);

        if (separate_present_queue) {
            queues[1].setQueueFamilyIndex(present_queue_family_index);
            queues[1].setQueueCount(1);
            queues[1].setPQueuePriorities(priorities);
            deviceInfo.setQueueCreateInfoCount(2);
        }

        auto result = gpu.createDevice(&deviceInfo, nullptr, &device);
        VERIFY(result == vk::Result::eSuccess);
    }

    void destroy_texture_image(texture_object *tex_objs) {
        // clean up staging resources
        device.freeMemory(tex_objs->mem, nullptr);
        device.destroyImage(tex_objs->image, nullptr);
    }

    void draw() {
        // Ensure no more than FRAME_LAG presentations are outstanding
        device.waitForFences(1, &fences[frame_index], VK_TRUE, UINT64_MAX);
        device.resetFences(1, &fences[frame_index]);

        // Get the index of the next available swapchain image:
        auto result = device.acquireNextImageKHR(swapchain, UINT64_MAX, image_acquired_semaphores[frame_index], fences[frame_index],
                                                 &current_buffer);
        if (result == vk::Result::eErrorOutOfDateKHR) {
            // swapchain is out of date (e.g. the window was resized) and
            // must be recreated:
            frame_index += 1;
            frame_index %= FRAME_LAG;

            resize();
            draw();
            return;
        } else if (result == vk::Result::eSuboptimalKHR) {
            // swapchain is not as optimal as it could be, but the platform's
            // presentation engine will still present the image correctly.
        } else {
            VERIFY(result == vk::Result::eSuccess);
        }

        // Wait for the image acquired semaphore to be signaled to ensure
        // that the image won't be rendered to until the presentation
        // engine has fully released ownership to the application, and it is
        // okay to render to the image.
        vk::PipelineStageFlags const pipe_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        auto const submit_info = vk::SubmitInfo()
                                     .setPWaitDstStageMask(&pipe_stage_flags)
                                     .setWaitSemaphoreCount(1)
                                     .setPWaitSemaphores(&image_acquired_semaphores[frame_index])
                                     .setCommandBufferCount(1)
                                     .setPCommandBuffers(&buffers[current_buffer].cmd)
                                     .setSignalSemaphoreCount(1)
                                     .setPSignalSemaphores(&draw_complete_semaphores[frame_index]);

        result = graphics_queue.submit(1, &submit_info, vk::Fence());
        VERIFY(result == vk::Result::eSuccess);

        if (separate_present_queue) {
            // If we are using separate queues, change image ownership to the
            // present queue before presenting, waiting for the draw complete
            // semaphore and signalling the ownership released semaphore when
            // finished
            auto const present_submit_info = vk::SubmitInfo()
                                                 .setPWaitDstStageMask(&pipe_stage_flags)
                                                 .setWaitSemaphoreCount(1)
                                                 .setPWaitSemaphores(&draw_complete_semaphores[frame_index])
                                                 .setCommandBufferCount(1)
                                                 .setPCommandBuffers(&buffers[current_buffer].graphics_to_present_cmd)
                                                 .setSignalSemaphoreCount(1)
                                                 .setPSignalSemaphores(&image_ownership_semaphores[frame_index]);

            result = present_queue.submit(1, &present_submit_info, vk::Fence());
            VERIFY(result == vk::Result::eSuccess);
        }

        // If we are using separate queues we have to wait for image ownership,
        // otherwise wait for draw complete
        auto const presentInfo = vk::PresentInfoKHR()
                                     .setWaitSemaphoreCount(1)
                                     .setPWaitSemaphores(separate_present_queue ? &image_ownership_semaphores[frame_index]
                                                                                : &draw_complete_semaphores[frame_index])
                                     .setSwapchainCount(1)
                                     .setPSwapchains(&swapchain)
                                     .setPImageIndices(&current_buffer);

        result = present_queue.presentKHR(&presentInfo);
        frame_index += 1;
        frame_index %= FRAME_LAG;
        if (result == vk::Result::eErrorOutOfDateKHR) {
            // swapchain is out of date (e.g. the window was resized) and
            // must be recreated:
            resize();
        } else if (result == vk::Result::eSuboptimalKHR) {
            // swapchain is not as optimal as it could be, but the platform's
            // presentation engine will still present the image correctly.
        } else {
            VERIFY(result == vk::Result::eSuccess);
        }
    }

    void draw_build_cmd(vk::CommandBuffer commandBuffer) {
        auto const commandInfo = vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

        vk::ClearValue const clearValues[2] = {vk::ClearColorValue(std::array<float, 4>({{0.2f, 0.2f, 0.2f, 0.2f}})),
                                               vk::ClearDepthStencilValue(1.0f, 0u)};

        auto const passInfo = vk::RenderPassBeginInfo()
                                  .setRenderPass(render_pass)
                                  .setFramebuffer(framebuffers[current_buffer])
                                  .setRenderArea(vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D((uint32_t)width, (uint32_t)height)))
                                  .setClearValueCount(2)
                                  .setPClearValues(clearValues);

        auto result = commandBuffer.begin(&commandInfo);
        VERIFY(result == vk::Result::eSuccess);

        commandBuffer.beginRenderPass(&passInfo, vk::SubpassContents::eInline);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, 1, &desc_set, 0, nullptr);

        auto const viewport =
            vk::Viewport().setWidth((float)width).setHeight((float)height).setMinDepth((float)0.0f).setMaxDepth((float)1.0f);
        commandBuffer.setViewport(0, 1, &viewport);

        vk::Rect2D const scissor(vk::Offset2D(0, 0), vk::Extent2D(width, height));
        commandBuffer.setScissor(0, 1, &scissor);
        commandBuffer.draw(12 * 3, 1, 0, 0);
        // Note that ending the renderpass changes the image's layout from
        // COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
        commandBuffer.endRenderPass();

        if (separate_present_queue) {
            // We have to transfer ownership from the graphics queue family to
            // the
            // present queue family to be able to present.  Note that we don't
            // have
            // to transfer from present queue family back to graphics queue
            // family at
            // the start of the next frame because we don't care about the
            // image's
            // contents at that point.
            auto const image_ownership_barrier =
                vk::ImageMemoryBarrier()
                    .setSrcAccessMask(vk::AccessFlags())
                    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                    .setOldLayout(vk::ImageLayout::ePresentSrcKHR)
                    .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
                    .setSrcQueueFamilyIndex(graphics_queue_family_index)
                    .setDstQueueFamilyIndex(present_queue_family_index)
                    .setImage(buffers[current_buffer].image)
                    .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

            commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                          vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlagBits(), 0, nullptr, 0,
                                          nullptr, 1, &image_ownership_barrier);
        }

        result = commandBuffer.end();
        VERIFY(result == vk::Result::eSuccess);
    }

    void flush_init_cmd() {
        // TODO: hmm.
        // This function could get called twice if the texture uses a staging
        // buffer
        // In that case the second call should be ignored
        if (!cmd) {
            return;
        }

        auto result = cmd.end();
        VERIFY(result == vk::Result::eSuccess);

        auto const fenceInfo = vk::FenceCreateInfo();
        vk::Fence fence;
        device.createFence(&fenceInfo, nullptr, &fence);

        vk::CommandBuffer const commandBuffers[] = {cmd};
        auto const submitInfo = vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(commandBuffers);

        result = graphics_queue.submit(1, &submitInfo, fence);
        VERIFY(result == vk::Result::eSuccess);

        result = device.waitForFences(1, &fence, VK_TRUE, UINT64_MAX);
        VERIFY(result == vk::Result::eSuccess);

        device.freeCommandBuffers(cmd_pool, 1, commandBuffers);
        device.destroyFence(fence, nullptr);

        cmd = vk::CommandBuffer();
    }

    void init(int argc, char **argv) {
        vec3 eye = {0.0f, 3.0f, 5.0f};
        vec3 origin = {0, 0, 0};
        vec3 up = {0.0f, 1.0f, 0.0};

        presentMode = vk::PresentModeKHR::eFifo;
        frameCount = UINT32_MAX;
        use_xlib = false;

        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--use_staging") == 0) {
                use_staging_buffer = true;
                continue;
            }
            if ((strcmp(argv[i], "--present_mode") == 0) && (i < argc - 1)) {
                presentMode = (vk::PresentModeKHR)atoi(argv[i + 1]);
                i++;
                continue;
            }
            if (strcmp(argv[i], "--break") == 0) {
                use_break = true;
                continue;
            }
            if (strcmp(argv[i], "--validate") == 0) {
                validate = true;
                continue;
            }
            if (strcmp(argv[i], "--xlib") == 0) {
                fprintf(stderr, "--xlib is deprecated and no longer does anything");
                continue;
            }
            if (strcmp(argv[i], "--c") == 0 && frameCount == UINT32_MAX && i < argc - 1 &&
                sscanf(argv[i + 1], "%d", &frameCount) == 1) {
                i++;
                continue;
            }
            if (strcmp(argv[i], "--suppress_popups") == 0) {
                suppress_popups = true;
                continue;
            }

            fprintf(stderr,
                    "Usage:\n  %s [--use_staging] [--validate] [--break] "
                    "[--c <framecount>] [--suppress_popups] [--present_mode <present mode enum>]\n"
                    "VK_PRESENT_MODE_IMMEDIATE_KHR = %d\n"
                    "VK_PRESENT_MODE_MAILBOX_KHR = %d\n"
                    "VK_PRESENT_MODE_FIFO_KHR = %d\n"
                    "VK_PRESENT_MODE_FIFO_RELAXED_KHR = %d\n",
                    APP_SHORT_NAME, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_FIFO_KHR,
                    VK_PRESENT_MODE_FIFO_RELAXED_KHR);
            fflush(stderr);
            exit(1);
        }

        init_vk();

        width = 500;
        height = 500;

        spin_angle = 4.0f;
        spin_increment = 0.2f;
        pause = false;

        mat4x4_perspective(projection_matrix, (float)degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
        mat4x4_look_at(view_matrix, eye, origin, up);
        mat4x4_identity(model_matrix);

        projection_matrix[1][1] *= -1;  // Flip projection matrix from GL to Vulkan orientation.
    }

    void init_vk() {
        //extension_names[enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        //return;

        vk::Result result;

        /* Look for device extensions */
        uint32_t device_extension_count = 0;
        vk::Bool32 swapchainExtFound = VK_FALSE;
        enabled_extension_count = 0;
        memset(extension_names, 0, sizeof(extension_names));

        result = gpu.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, nullptr);
        VERIFY(result == vk::Result::eSuccess);

        if (device_extension_count > 0) {
            std::unique_ptr<vk::ExtensionProperties[]> device_extensions(new vk::ExtensionProperties[device_extension_count]);
            result = gpu.enumerateDeviceExtensionProperties(nullptr, &device_extension_count, device_extensions.get());
            VERIFY(result == vk::Result::eSuccess);

            for (uint32_t i = 0; i < device_extension_count; i++) {
                if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName)) {
                    swapchainExtFound = 1;
                    extension_names[enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
                }
                assert(enabled_extension_count < 64);
            }
        }

        if (!swapchainExtFound) {
            ERR_EXIT(
                "vkEnumerateDeviceExtensionProperties failed to find "
                "the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
                " extension.\n\n"
                "Do you have a compatible Vulkan installable client "
                "driver (ICD) installed?\n"
                "Please look at the Getting Started guide for additional "
                "information.\n",
                "vkCreateInstance Failure");
        }

        gpu.getProperties(&gpu_props);

        /* Call with nullptr data to get count */
        gpu.getQueueFamilyProperties(&queue_family_count, nullptr);
        assert(queue_family_count >= 1);

        queue_props.reset(new vk::QueueFamilyProperties[queue_family_count]);
        gpu.getQueueFamilyProperties(&queue_family_count, queue_props.get());

        // Query fine-grained feature support for this device.
        //  If app has specific feature requirements it should check supported
        //  features based on this query
        vk::PhysicalDeviceFeatures physDevFeatures;
        gpu.getFeatures(&physDevFeatures);
    }

    //-----------------------------------------vk_physical_device----------------------------------------

    void set_physical_device(VkPhysicalDevice physical_device) {
        gpu = physical_device;
    }

    //-----------------------------------------vk_swapchain----------------------------------------
    void init_vk_swapchain(VkSurfaceKHR surface) {
        this->surface = surface;

        // Iterate over each queue to learn whether it supports presenting:
        std::unique_ptr<vk::Bool32[]> supportsPresent(new vk::Bool32[queue_family_count]);
        for (uint32_t i = 0; i < queue_family_count; i++) {
            gpu.getSurfaceSupportKHR(i, surface, &supportsPresent[i]);
        }

        uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
        uint32_t presentQueueFamilyIndex = UINT32_MAX;
        for (uint32_t i = 0; i < queue_family_count; i++) {
            if (queue_props[i].queueFlags & vk::QueueFlagBits::eGraphics) {
                if (graphicsQueueFamilyIndex == UINT32_MAX) {
                    graphicsQueueFamilyIndex = i;
                }

                if (supportsPresent[i] == VK_TRUE) {
                    graphicsQueueFamilyIndex = i;
                    presentQueueFamilyIndex = i;
                    break;
                }
            }
        }

        if (presentQueueFamilyIndex == UINT32_MAX) {
            // If didn't find a queue that supports both graphics and present,
            // then
            // find a separate present queue.
            for (uint32_t i = 0; i < queue_family_count; ++i) {
                if (supportsPresent[i] == VK_TRUE) {
                    presentQueueFamilyIndex = i;
                    break;
                }
            }
        }

        // Generate error if could not find both a graphics and a present queue
        if (graphicsQueueFamilyIndex == UINT32_MAX || presentQueueFamilyIndex == UINT32_MAX) {
            ERR_EXIT("Could not find both graphics and present queues\n", "Swapchain Initialization Failure");
        }

        graphics_queue_family_index = graphicsQueueFamilyIndex;
        present_queue_family_index = presentQueueFamilyIndex;
        separate_present_queue = (graphics_queue_family_index != present_queue_family_index);

        create_device();

        device.getQueue(graphics_queue_family_index, 0, &graphics_queue);
        if (!separate_present_queue) {
            present_queue = graphics_queue;
        } else {
            device.getQueue(present_queue_family_index, 0, &present_queue);
        }

        // Get the list of VkFormat's that are supported:
        uint32_t formatCount;
        auto result = gpu.getSurfaceFormatsKHR(surface, &formatCount, nullptr);
        VERIFY(result == vk::Result::eSuccess);

        std::unique_ptr<vk::SurfaceFormatKHR[]> surfFormats(new vk::SurfaceFormatKHR[formatCount]);
        result = gpu.getSurfaceFormatsKHR(surface, &formatCount, surfFormats.get());
        VERIFY(result == vk::Result::eSuccess);

        // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
        // the surface has no preferred format.  Otherwise, at least one
        // supported format will be returned.
        if (formatCount == 1 && surfFormats[0].format == vk::Format::eUndefined) {
            format = vk::Format::eB8G8R8A8Unorm;
        } else {
            assert(formatCount >= 1);
            format = surfFormats[0].format;
        }
        color_space = surfFormats[0].colorSpace;

        quit = false;
        curFrame = 0;

        // Create semaphores to synchronize acquiring presentable buffers before
        // rendering and waiting for drawing to be complete before presenting
        auto const semaphoreCreateInfo = vk::SemaphoreCreateInfo();

        // Create fences that we can use to throttle if we get too far
        // ahead of the image presents
        auto const fence_ci = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
        for (uint32_t i = 0; i < FRAME_LAG; i++) {
            device.createFence(&fence_ci, nullptr, &fences[i]);
            result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &image_acquired_semaphores[i]);
            VERIFY(result == vk::Result::eSuccess);

            result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &draw_complete_semaphores[i]);
            VERIFY(result == vk::Result::eSuccess);

            if (separate_present_queue) {
                result = device.createSemaphore(&semaphoreCreateInfo, nullptr, &image_ownership_semaphores[i]);
                VERIFY(result == vk::Result::eSuccess);
            }
        }
        frame_index = 0;

        // Get Memory information and properties
        gpu.getMemoryProperties(&memory_properties);
    }

    //------------------------------------------prepare--------------------------------------------
    void prepare() {
        auto const cmd_pool_info = vk::CommandPoolCreateInfo().setQueueFamilyIndex(graphics_queue_family_index);
        auto result = device.createCommandPool(&cmd_pool_info, nullptr, &cmd_pool);
        VERIFY(result == vk::Result::eSuccess);

        auto const cmd = vk::CommandBufferAllocateInfo()
                             .setCommandPool(cmd_pool)
                             .setLevel(vk::CommandBufferLevel::ePrimary)
                             .setCommandBufferCount(1);

        result = device.allocateCommandBuffers(&cmd, &this->cmd);
        VERIFY(result == vk::Result::eSuccess);

        auto const cmd_buf_info = vk::CommandBufferBeginInfo().setPInheritanceInfo(nullptr);

        result = this->cmd.begin(&cmd_buf_info);
        VERIFY(result == vk::Result::eSuccess);

        prepare_buffers();
        prepare_depth();
        prepare_textures();
        prepare_cube_data_buffer();

        prepare_descriptor_layout();
        prepare_render_pass();
        prepare_pipeline();

        for (uint32_t i = 0; i < swapchainImageCount; ++i) {
            result = device.allocateCommandBuffers(&cmd, &buffers[i].cmd);
            VERIFY(result == vk::Result::eSuccess);
        }

        if (separate_present_queue) {
            auto const present_cmd_pool_info = vk::CommandPoolCreateInfo().setQueueFamilyIndex(present_queue_family_index);

            result = device.createCommandPool(&present_cmd_pool_info, nullptr, &present_cmd_pool);
            VERIFY(result == vk::Result::eSuccess);

            auto const present_cmd = vk::CommandBufferAllocateInfo()
                                         .setCommandPool(present_cmd_pool)
                                         .setLevel(vk::CommandBufferLevel::ePrimary)
                                         .setCommandBufferCount(1);

            for (uint32_t i = 0; i < swapchainImageCount; i++) {
                result = device.allocateCommandBuffers(&present_cmd, &buffers[i].graphics_to_present_cmd);
                VERIFY(result == vk::Result::eSuccess);

                build_image_ownership_cmd(i);
            }
        }

        prepare_descriptor_pool();
        prepare_descriptor_set();

        prepare_framebuffers();

        for (uint32_t i = 0; i < swapchainImageCount; ++i) {
            current_buffer = i;
            draw_build_cmd(buffers[i].cmd);
        }

        /*
         * Prepare functions above may generate pipeline commands
         * that need to be flushed before beginning the render loop.
         */
        flush_init_cmd();
        if (staging_texture.image) {
            destroy_texture_image(&staging_texture);
        }

        current_buffer = 0;
        prepared = true;
    }

    void prepare_buffers() {
        vk::SwapchainKHR oldSwapchain = swapchain;

        // Check the surface capabilities and formats
        vk::SurfaceCapabilitiesKHR surfCapabilities;
        auto result = gpu.getSurfaceCapabilitiesKHR(surface, &surfCapabilities);
        VERIFY(result == vk::Result::eSuccess);

        uint32_t presentModeCount;
        result = gpu.getSurfacePresentModesKHR(surface, &presentModeCount, nullptr);
        VERIFY(result == vk::Result::eSuccess);

        std::unique_ptr<vk::PresentModeKHR[]> presentModes(new vk::PresentModeKHR[presentModeCount]);
        result = gpu.getSurfacePresentModesKHR(surface, &presentModeCount, presentModes.get());
        VERIFY(result == vk::Result::eSuccess);

        vk::Extent2D swapchainExtent;
        // width and height are either both -1, or both not -1.
        if (surfCapabilities.currentExtent.width == (uint32_t)-1) {
            // If the surface size is undefined, the size is set to
            // the size of the images requested.
            swapchainExtent.width = width;
            swapchainExtent.height = height;
        } else {
            // If the surface size is defined, the swap chain size must match
            swapchainExtent = surfCapabilities.currentExtent;
            width = surfCapabilities.currentExtent.width;
            height = surfCapabilities.currentExtent.height;
        }

        // The FIFO present mode is guaranteed by the spec to be supported
        // and to have no tearing.  It's a great default present mode to use.
        vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

        //  There are times when you may wish to use another present mode.  The
        //  following code shows how to select them, and the comments provide some
        //  reasons you may wish to use them.
        //
        // It should be noted that Vulkan 1.0 doesn't provide a method for
        // synchronizing rendering with the presentation engine's display.  There
        // is a method provided for throttling rendering with the display, but
        // there are some presentation engines for which this method will not work.
        // If an application doesn't throttle its rendering, and if it renders much
        // faster than the refresh rate of the display, this can waste power on
        // mobile devices.  That is because power is being spent rendering images
        // that may never be seen.

        // VK_PRESENT_MODE_IMMEDIATE_KHR is for applications that don't care
        // about
        // tearing, or have some way of synchronizing their rendering with the
        // display.
        // VK_PRESENT_MODE_MAILBOX_KHR may be useful for applications that
        // generally render a new presentable image every refresh cycle, but are
        // occasionally early.  In this case, the application wants the new
        // image
        // to be displayed instead of the previously-queued-for-presentation
        // image
        // that has not yet been displayed.
        // VK_PRESENT_MODE_FIFO_RELAXED_KHR is for applications that generally
        // render a new presentable image every refresh cycle, but are
        // occasionally
        // late.  In this case (perhaps because of stuttering/latency concerns),
        // the application wants the late image to be immediately displayed,
        // even
        // though that may mean some tearing.

        if (presentMode != swapchainPresentMode) {
            for (size_t i = 0; i < presentModeCount; ++i) {
                if (presentModes[i] == presentMode) {
                    swapchainPresentMode = presentMode;
                    break;
                }
            }
        }

        if (swapchainPresentMode != presentMode) {
            ERR_EXIT("Present mode specified is not supported\n", "Present mode unsupported");
        }

        // Determine the number of VkImage's to use in the swap chain (we desire
        // to
        // own only 1 image at a time, besides the images being displayed and
        // queued for display):
        uint32_t desiredNumberOfSwapchainImages = surfCapabilities.minImageCount + 1;
        // If maxImageCount is 0, we can ask for as many images as we want,
        // otherwise
        // we're limited to maxImageCount
        if ((surfCapabilities.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
            // Application must settle for fewer images than desired:
            desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
        }

        vk::SurfaceTransformFlagBitsKHR preTransform;
        if (surfCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
            preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        } else {
            preTransform = surfCapabilities.currentTransform;
        }

        auto const swapchain_ci = vk::SwapchainCreateInfoKHR()
                                      .setSurface(surface)
                                      .setMinImageCount(desiredNumberOfSwapchainImages)
                                      .setImageFormat(format)
                                      .setImageColorSpace(color_space)
                                      .setImageExtent({swapchainExtent.width, swapchainExtent.height})
                                      .setImageArrayLayers(1)
                                      .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                                      .setImageSharingMode(vk::SharingMode::eExclusive)
                                      .setQueueFamilyIndexCount(0)
                                      .setPQueueFamilyIndices(nullptr)
                                      .setPreTransform(preTransform)
                                      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
                                      .setPresentMode(swapchainPresentMode)
                                      .setClipped(true)
                                      .setOldSwapchain(oldSwapchain);

        result = device.createSwapchainKHR(&swapchain_ci, nullptr, &swapchain);
        VERIFY(result == vk::Result::eSuccess);

        // If we just re-created an existing swapchain, we should destroy the
        // old
        // swapchain at this point.
        // Note: destroying the swapchain also cleans up all its associated
        // presentable images once the platform is done with them.
        if (oldSwapchain) {
            device.destroySwapchainKHR(oldSwapchain, nullptr);
        }

        result = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, nullptr);
        VERIFY(result == vk::Result::eSuccess);

        std::unique_ptr<vk::Image[]> swapchainImages(new vk::Image[swapchainImageCount]);
        result = device.getSwapchainImagesKHR(swapchain, &swapchainImageCount, swapchainImages.get());
        VERIFY(result == vk::Result::eSuccess);

        buffers.reset(new SwapchainBuffers[swapchainImageCount]);

        for (uint32_t i = 0; i < swapchainImageCount; ++i) {
            auto const color_image_view =
                vk::ImageViewCreateInfo()
                    .setImage(swapchainImages[i])
                    .setViewType(vk::ImageViewType::e2D)
                    .setFormat(format)
                    .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

            buffers[i].image = swapchainImages[i];

            result = device.createImageView(&color_image_view, nullptr, &buffers[i].view);
            VERIFY(result == vk::Result::eSuccess);
        }
    }

    void prepare_cube_data_buffer() {
        mat4x4 VP;
        mat4x4_mul(VP, projection_matrix, view_matrix);

        mat4x4 MVP;
        mat4x4_mul(MVP, VP, model_matrix);

        vktexcube_vs_uniform data;
        memcpy(data.mvp, MVP, sizeof(MVP));
        //    dumpMatrix("MVP", MVP)
        for (int32_t i = 0; i < 12 * 3; i++) {
            data.position[i][0] = g_vertex_buffer_data[i * 3];
            data.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
            data.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
            data.position[i][3] = 1.0f;
            data.attr[i][0] = g_uv_buffer_data[2 * i];
            data.attr[i][1] = g_uv_buffer_data[2 * i + 1];
            data.attr[i][2] = 0;
            data.attr[i][3] = 0;
        }

        auto const buf_info = vk::BufferCreateInfo().setSize(sizeof(data)).setUsage(vk::BufferUsageFlagBits::eUniformBuffer);
        auto result = device.createBuffer(&buf_info, nullptr, &uniform_data.buf);
        VERIFY(result == vk::Result::eSuccess);

        vk::MemoryRequirements mem_reqs;
        device.getBufferMemoryRequirements(uniform_data.buf, &mem_reqs);

        uniform_data.mem_alloc.setAllocationSize(mem_reqs.size);
        uniform_data.mem_alloc.setMemoryTypeIndex(0);

        bool const pass = memory_type_from_properties(
            mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            &uniform_data.mem_alloc.memoryTypeIndex);
        VERIFY(pass);

        result = device.allocateMemory(&uniform_data.mem_alloc, nullptr, &(uniform_data.mem));
        VERIFY(result == vk::Result::eSuccess);

        auto pData = device.mapMemory(uniform_data.mem, 0, uniform_data.mem_alloc.allocationSize, vk::MemoryMapFlags());
        VERIFY(pData.result == vk::Result::eSuccess);

        memcpy(pData.value, &data, sizeof data);

        device.unmapMemory(uniform_data.mem);

        result = device.bindBufferMemory(uniform_data.buf, uniform_data.mem, 0);
        VERIFY(result == vk::Result::eSuccess);

        uniform_data.buffer_info.buffer = uniform_data.buf;
        uniform_data.buffer_info.offset = 0;
        uniform_data.buffer_info.range = sizeof(data);
    }

    void prepare_depth() {
        depth.format = vk::Format::eD16Unorm;

        auto const image = vk::ImageCreateInfo()
                               .setImageType(vk::ImageType::e2D)
                               .setFormat(depth.format)
                               .setExtent({(uint32_t)width, (uint32_t)height, 1})
                               .setMipLevels(1)
                               .setArrayLayers(1)
                               .setSamples(vk::SampleCountFlagBits::e1)
                               .setTiling(vk::ImageTiling::eOptimal)
                               .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
                               .setSharingMode(vk::SharingMode::eExclusive)
                               .setQueueFamilyIndexCount(0)
                               .setPQueueFamilyIndices(nullptr)
                               .setInitialLayout(vk::ImageLayout::eUndefined);

        auto result = device.createImage(&image, nullptr, &depth.image);
        VERIFY(result == vk::Result::eSuccess);

        vk::MemoryRequirements mem_reqs;
        device.getImageMemoryRequirements(depth.image, &mem_reqs);

        depth.mem_alloc.setAllocationSize(mem_reqs.size);
        depth.mem_alloc.setMemoryTypeIndex(0);

        auto const pass =
            memory_type_from_properties(mem_reqs.memoryTypeBits, vk::MemoryPropertyFlagBits(0), &depth.mem_alloc.memoryTypeIndex);
        VERIFY(pass);

        result = device.allocateMemory(&depth.mem_alloc, nullptr, &depth.mem);
        VERIFY(result == vk::Result::eSuccess);

        result = device.bindImageMemory(depth.image, depth.mem, 0);
        VERIFY(result == vk::Result::eSuccess);

        auto const view = vk::ImageViewCreateInfo()
                              .setImage(depth.image)
                              .setViewType(vk::ImageViewType::e2D)
                              .setFormat(depth.format)
                              .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
        result = device.createImageView(&view, nullptr, &depth.view);
        VERIFY(result == vk::Result::eSuccess);
    }

    void prepare_descriptor_layout() {
        vk::DescriptorSetLayoutBinding const layout_bindings[2] = {vk::DescriptorSetLayoutBinding()
                                                                       .setBinding(0)
                                                                       .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                                                       .setDescriptorCount(1)
                                                                       .setStageFlags(vk::ShaderStageFlagBits::eVertex)
                                                                       .setPImmutableSamplers(nullptr),
                                                                   vk::DescriptorSetLayoutBinding()
                                                                       .setBinding(1)
                                                                       .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                                                                       .setDescriptorCount(texture_count)
                                                                       .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                                                                       .setPImmutableSamplers(nullptr)};

        auto const descriptor_layout = vk::DescriptorSetLayoutCreateInfo().setBindingCount(2).setPBindings(layout_bindings);

        auto result = device.createDescriptorSetLayout(&descriptor_layout, nullptr, &desc_layout);
        VERIFY(result == vk::Result::eSuccess);

        auto const pPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo().setSetLayoutCount(1).setPSetLayouts(&desc_layout);

        result = device.createPipelineLayout(&pPipelineLayoutCreateInfo, nullptr, &pipeline_layout);
        VERIFY(result == vk::Result::eSuccess);
    }

    void prepare_descriptor_pool() {
        vk::DescriptorPoolSize const poolSizes[2] = {
            vk::DescriptorPoolSize().setType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(1),
            vk::DescriptorPoolSize().setType(vk::DescriptorType::eCombinedImageSampler).setDescriptorCount(texture_count)};

        auto const descriptor_pool = vk::DescriptorPoolCreateInfo().setMaxSets(1).setPoolSizeCount(2).setPPoolSizes(poolSizes);

        auto result = device.createDescriptorPool(&descriptor_pool, nullptr, &desc_pool);
        VERIFY(result == vk::Result::eSuccess);
    }

    void prepare_descriptor_set() {
        auto const alloc_info =
            vk::DescriptorSetAllocateInfo().setDescriptorPool(desc_pool).setDescriptorSetCount(1).setPSetLayouts(&desc_layout);
        auto result = device.allocateDescriptorSets(&alloc_info, &desc_set);
        VERIFY(result == vk::Result::eSuccess);

        vk::DescriptorImageInfo tex_descs[texture_count];
        for (uint32_t i = 0; i < texture_count; i++) {
            tex_descs[i].setSampler(textures[i].sampler);
            tex_descs[i].setImageView(textures[i].view);
            tex_descs[i].setImageLayout(vk::ImageLayout::eGeneral);
        }

        vk::WriteDescriptorSet writes[2];

        writes[0].setDstSet(desc_set);
        writes[0].setDescriptorCount(1);
        writes[0].setDescriptorType(vk::DescriptorType::eUniformBuffer);
        writes[0].setPBufferInfo(&uniform_data.buffer_info);

        writes[1].setDstSet(desc_set);
        writes[1].setDstBinding(1);
        writes[1].setDescriptorCount(texture_count);
        writes[1].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
        writes[1].setPImageInfo(tex_descs);

        device.updateDescriptorSets(2, writes, 0, nullptr);
    }

    void prepare_framebuffers() {
        vk::ImageView attachments[2];
        attachments[1] = depth.view;

        auto const fb_info = vk::FramebufferCreateInfo()
                                 .setRenderPass(render_pass)
                                 .setAttachmentCount(2)
                                 .setPAttachments(attachments)
                                 .setWidth((uint32_t)width)
                                 .setHeight((uint32_t)height)
                                 .setLayers(1);

        framebuffers.reset(new vk::Framebuffer[swapchainImageCount]);

        for (uint32_t i = 0; i < swapchainImageCount; i++) {
            attachments[0] = buffers[i].view;
            auto const result = device.createFramebuffer(&fb_info, nullptr, &framebuffers[i]);
            VERIFY(result == vk::Result::eSuccess);
        }
    }

    vk::ShaderModule prepare_fs() {
        size_t size = 0;
        void *fragShaderCode = read_spv("cube-frag.spv", &size);
        if (!fragShaderCode) {
            ERR_EXIT("Failed to load cube-frag.spv", "Load Shader Failure");
        }

        frag_shader_module = prepare_shader_module(fragShaderCode, size);

        free(fragShaderCode);

        return frag_shader_module;
    }

    void prepare_pipeline() {
        vk::PipelineCacheCreateInfo const pipelineCacheInfo;
        auto result = device.createPipelineCache(&pipelineCacheInfo, nullptr, &pipelineCache);
        VERIFY(result == vk::Result::eSuccess);

        vk::PipelineShaderStageCreateInfo const shaderStageInfo[2] = {
            vk::PipelineShaderStageCreateInfo().setStage(vk::ShaderStageFlagBits::eVertex).setModule(prepare_vs()).setPName("main"),
            vk::PipelineShaderStageCreateInfo()
                .setStage(vk::ShaderStageFlagBits::eFragment)
                .setModule(prepare_fs())
                .setPName("main")};

        vk::PipelineVertexInputStateCreateInfo const vertexInputInfo;

        auto const inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo().setTopology(vk::PrimitiveTopology::eTriangleList);

        // TODO: Where are pViewports and pScissors set?
        auto const viewportInfo = vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1);

        auto const rasterizationInfo = vk::PipelineRasterizationStateCreateInfo()
                                           .setDepthClampEnable(VK_FALSE)
                                           .setRasterizerDiscardEnable(VK_FALSE)
                                           .setPolygonMode(vk::PolygonMode::eFill)
                                           .setCullMode(vk::CullModeFlagBits::eBack)
                                           .setFrontFace(vk::FrontFace::eCounterClockwise)
                                           .setDepthBiasEnable(VK_FALSE)
                                           .setLineWidth(1.0f);

        auto const multisampleInfo = vk::PipelineMultisampleStateCreateInfo();

        auto const stencilOp = vk::StencilOpState()
                                   .setFailOp(vk::StencilOp::eKeep)
                                   .setPassOp(vk::StencilOp::eKeep)
                                   .setCompareOp(vk::CompareOp::eAlways);

        auto const depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo()
                                          .setDepthTestEnable(VK_TRUE)
                                          .setDepthWriteEnable(VK_TRUE)
                                          .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                                          .setDepthBoundsTestEnable(VK_FALSE)
                                          .setStencilTestEnable(VK_FALSE)
                                          .setFront(stencilOp)
                                          .setBack(stencilOp);

        vk::PipelineColorBlendAttachmentState const colorBlendAttachments[1] = {
            vk::PipelineColorBlendAttachmentState().setColorWriteMask(
                vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                vk::ColorComponentFlagBits::eA)};

        auto const colorBlendInfo =
            vk::PipelineColorBlendStateCreateInfo().setAttachmentCount(1).setPAttachments(colorBlendAttachments);

        vk::DynamicState const dynamicStates[2] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

        auto const dynamicStateInfo = vk::PipelineDynamicStateCreateInfo().setPDynamicStates(dynamicStates).setDynamicStateCount(2);

        auto const pipeline = vk::GraphicsPipelineCreateInfo()
                                  .setStageCount(2)
                                  .setPStages(shaderStageInfo)
                                  .setPVertexInputState(&vertexInputInfo)
                                  .setPInputAssemblyState(&inputAssemblyInfo)
                                  .setPViewportState(&viewportInfo)
                                  .setPRasterizationState(&rasterizationInfo)
                                  .setPMultisampleState(&multisampleInfo)
                                  .setPDepthStencilState(&depthStencilInfo)
                                  .setPColorBlendState(&colorBlendInfo)
                                  .setPDynamicState(&dynamicStateInfo)
                                  .setLayout(pipeline_layout)
                                  .setRenderPass(render_pass);

        result = device.createGraphicsPipelines(pipelineCache, 1, &pipeline, nullptr, &this->pipeline);
        VERIFY(result == vk::Result::eSuccess);

        device.destroyShaderModule(frag_shader_module, nullptr);
        device.destroyShaderModule(vert_shader_module, nullptr);
    }

    void prepare_render_pass() {
        // The initial layout for the color and depth attachments will be LAYOUT_UNDEFINED
        // because at the start of the renderpass, we don't care about their contents.
        // At the start of the subpass, the color attachment's layout will be transitioned
        // to LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the depth stencil attachment's layout
        // will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the end of
        // the renderpass, the color attachment's layout will be transitioned to
        // LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as part of
        // the renderpass, no barriers are necessary.
        const vk::AttachmentDescription attachments[2] = {vk::AttachmentDescription()
                                                              .setFormat(format)
                                                              .setSamples(vk::SampleCountFlagBits::e1)
                                                              .setLoadOp(vk::AttachmentLoadOp::eClear)
                                                              .setStoreOp(vk::AttachmentStoreOp::eStore)
                                                              .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                                                              .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                                              .setInitialLayout(vk::ImageLayout::eUndefined)
                                                              .setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
                                                          vk::AttachmentDescription()
                                                              .setFormat(depth.format)
                                                              .setSamples(vk::SampleCountFlagBits::e1)
                                                              .setLoadOp(vk::AttachmentLoadOp::eClear)
                                                              .setStoreOp(vk::AttachmentStoreOp::eDontCare)
                                                              .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                                                              .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                                                              .setInitialLayout(vk::ImageLayout::eUndefined)
                                                              .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)};

        auto const color_reference = vk::AttachmentReference().setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

        auto const depth_reference =
            vk::AttachmentReference().setAttachment(1).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

        auto const subpass = vk::SubpassDescription()
                                 .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                                 .setInputAttachmentCount(0)
                                 .setPInputAttachments(nullptr)
                                 .setColorAttachmentCount(1)
                                 .setPColorAttachments(&color_reference)
                                 .setPResolveAttachments(nullptr)
                                 .setPDepthStencilAttachment(&depth_reference)
                                 .setPreserveAttachmentCount(0)
                                 .setPPreserveAttachments(nullptr);

        auto const rp_info = vk::RenderPassCreateInfo()
                                 .setAttachmentCount(2)
                                 .setPAttachments(attachments)
                                 .setSubpassCount(1)
                                 .setPSubpasses(&subpass)
                                 .setDependencyCount(0)
                                 .setPDependencies(nullptr);

        auto result = device.createRenderPass(&rp_info, nullptr, &render_pass);
        VERIFY(result == vk::Result::eSuccess);
    }

    vk::ShaderModule prepare_shader_module(const void *code, size_t size) {
        auto const moduleCreateInfo = vk::ShaderModuleCreateInfo().setCodeSize(size).setPCode((uint32_t const *)code);

        vk::ShaderModule module;
        auto result = device.createShaderModule(&moduleCreateInfo, nullptr, &module);
        VERIFY(result == vk::Result::eSuccess);

        return module;
    }

    void prepare_texture_image(const char *filename, texture_object *tex_obj, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                               vk::MemoryPropertyFlags required_props) {
        int32_t tex_width;
        int32_t tex_height;
        if (!loadTexture(filename, nullptr, nullptr, &tex_width, &tex_height)) {
            ERR_EXIT("Failed to load textures", "Load Texture Failure");
        }

        tex_obj->tex_width = tex_width;
        tex_obj->tex_height = tex_height;

        auto const image_create_info = vk::ImageCreateInfo()
                                           .setImageType(vk::ImageType::e2D)
                                           .setFormat(vk::Format::eR8G8B8A8Unorm)
                                           .setExtent({(uint32_t)tex_width, (uint32_t)tex_height, 1})
                                           .setMipLevels(1)
                                           .setArrayLayers(1)
                                           .setSamples(vk::SampleCountFlagBits::e1)
                                           .setTiling(tiling)
                                           .setUsage(usage)
                                           .setSharingMode(vk::SharingMode::eExclusive)
                                           .setQueueFamilyIndexCount(0)
                                           .setPQueueFamilyIndices(nullptr)
                                           .setInitialLayout(vk::ImageLayout::ePreinitialized);

        auto result = device.createImage(&image_create_info, nullptr, &tex_obj->image);
        VERIFY(result == vk::Result::eSuccess);

        vk::MemoryRequirements mem_reqs;
        device.getImageMemoryRequirements(tex_obj->image, &mem_reqs);

        tex_obj->mem_alloc.setAllocationSize(mem_reqs.size);
        tex_obj->mem_alloc.setMemoryTypeIndex(0);

        auto pass = memory_type_from_properties(mem_reqs.memoryTypeBits, required_props, &tex_obj->mem_alloc.memoryTypeIndex);
        VERIFY(pass == true);

        result = device.allocateMemory(&tex_obj->mem_alloc, nullptr, &(tex_obj->mem));
        VERIFY(result == vk::Result::eSuccess);

        result = device.bindImageMemory(tex_obj->image, tex_obj->mem, 0);
        VERIFY(result == vk::Result::eSuccess);

        if (required_props & vk::MemoryPropertyFlagBits::eHostVisible) {
            auto const subres =
                vk::ImageSubresource().setAspectMask(vk::ImageAspectFlagBits::eColor).setMipLevel(0).setArrayLayer(0);
            vk::SubresourceLayout layout;
            device.getImageSubresourceLayout(tex_obj->image, &subres, &layout);

            auto data = device.mapMemory(tex_obj->mem, 0, tex_obj->mem_alloc.allocationSize);
            VERIFY(data.result == vk::Result::eSuccess);

            if (!loadTexture(filename, (uint8_t *)data.value, &layout, &tex_width, &tex_height)) {
                fprintf(stderr, "Error loading texture: %s\n", filename);
            }

            device.unmapMemory(tex_obj->mem);
        }

        tex_obj->imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    }

    void prepare_textures() {
        vk::Format const tex_format = vk::Format::eR8G8B8A8Unorm;
        vk::FormatProperties props;
        gpu.getFormatProperties(tex_format, &props);

        for (uint32_t i = 0; i < texture_count; i++) {
            if ((props.linearTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage) && !use_staging_buffer) {
                /* Device can texture using linear textures */
                prepare_texture_image(tex_files[i], &textures[i], vk::ImageTiling::eLinear, vk::ImageUsageFlagBits::eSampled,
                                      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
                // Nothing in the pipeline needs to be complete to start, and don't allow fragment
                // shader to run until layout transition completes
                set_image_layout(textures[i].image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized,
                                 textures[i].imageLayout, vk::AccessFlagBits::eHostWrite, vk::PipelineStageFlagBits::eTopOfPipe,
                                 vk::PipelineStageFlagBits::eFragmentShader);
                staging_texture.image = vk::Image();
            } else if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage) {
                /* Must use staging buffer to copy linear texture to optimized */

                prepare_texture_image(tex_files[i], &staging_texture, vk::ImageTiling::eLinear,
                                      vk::ImageUsageFlagBits::eTransferSrc,
                                      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

                prepare_texture_image(tex_files[i], &textures[i], vk::ImageTiling::eOptimal,
                                      vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                                      vk::MemoryPropertyFlagBits::eDeviceLocal);

                set_image_layout(staging_texture.image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized,
                                 vk::ImageLayout::eTransferSrcOptimal, vk::AccessFlagBits::eHostWrite,
                                 vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer);

                set_image_layout(textures[i].image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::ePreinitialized,
                                 vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eHostWrite,
                                 vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer);

                auto const subresource = vk::ImageSubresourceLayers()
                                             .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                             .setMipLevel(0)
                                             .setBaseArrayLayer(0)
                                             .setLayerCount(1);

                auto const copy_region =
                    vk::ImageCopy()
                        .setSrcSubresource(subresource)
                        .setSrcOffset({0, 0, 0})
                        .setDstSubresource(subresource)
                        .setDstOffset({0, 0, 0})
                        .setExtent({(uint32_t)staging_texture.tex_width, (uint32_t)staging_texture.tex_height, 1});

                cmd.copyImage(staging_texture.image, vk::ImageLayout::eTransferSrcOptimal, textures[i].image,
                              vk::ImageLayout::eTransferDstOptimal, 1, &copy_region);

                set_image_layout(textures[i].image, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eTransferDstOptimal,
                                 textures[i].imageLayout, vk::AccessFlagBits::eTransferWrite, vk::PipelineStageFlagBits::eTransfer,
                                 vk::PipelineStageFlagBits::eFragmentShader);
            } else {
                assert(!"No support for R8G8B8A8_UNORM as texture image format");
            }

            auto const samplerInfo = vk::SamplerCreateInfo()
                                         .setMagFilter(vk::Filter::eNearest)
                                         .setMinFilter(vk::Filter::eNearest)
                                         .setMipmapMode(vk::SamplerMipmapMode::eNearest)
                                         .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                                         .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                                         .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                                         .setMipLodBias(0.0f)
                                         .setAnisotropyEnable(VK_FALSE)
                                         .setMaxAnisotropy(1)
                                         .setCompareEnable(VK_FALSE)
                                         .setCompareOp(vk::CompareOp::eNever)
                                         .setMinLod(0.0f)
                                         .setMaxLod(0.0f)
                                         .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
                                         .setUnnormalizedCoordinates(VK_FALSE);

            auto result = device.createSampler(&samplerInfo, nullptr, &textures[i].sampler);
            VERIFY(result == vk::Result::eSuccess);

            auto const viewInfo = vk::ImageViewCreateInfo()
                                      .setImage(textures[i].image)
                                      .setViewType(vk::ImageViewType::e2D)
                                      .setFormat(tex_format)
                                      .setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

            result = device.createImageView(&viewInfo, nullptr, &textures[i].view);
            VERIFY(result == vk::Result::eSuccess);
        }
    }

    vk::ShaderModule prepare_vs() {
        size_t size = 0;
        void *vertShaderCode = read_spv("cube-vert.spv", &size);
        if (!vertShaderCode) {
            ERR_EXIT("Failed to load cube-vert.spv", "Load Shader Failure");
        }

        vert_shader_module = prepare_shader_module(vertShaderCode, size);

        free(vertShaderCode);

        return vert_shader_module;
    }

    char *read_spv(const char *filename, size_t *psize) {
        FILE *fp = fopen(filename, "rb");
        if (!fp) {
            return nullptr;
        }

        fseek(fp, 0L, SEEK_END);
        long int size = ftell(fp);

        fseek(fp, 0L, SEEK_SET);

        void *shader_code = malloc(size);
        size_t retval = fread(shader_code, size, 1, fp);
        VERIFY(retval == 1);

        *psize = size;

        fclose(fp);

        return (char *)shader_code;
    }

    void resize() {
        uint32_t i;

        // Don't react to resize until after first initialization.
        if (!prepared) {
            return;
        }

        // In order to properly resize the window, we must re-create the
        // swapchain
        // AND redo the command buffers, etc.
        //
        // First, perform part of the cleanup() function:
        prepared = false;
        auto result = device.waitIdle();
        VERIFY(result == vk::Result::eSuccess);

        for (i = 0; i < swapchainImageCount; i++) {
            device.destroyFramebuffer(framebuffers[i], nullptr);
        }

        device.destroyDescriptorPool(desc_pool, nullptr);

        device.destroyPipeline(pipeline, nullptr);
        device.destroyPipelineCache(pipelineCache, nullptr);
        device.destroyRenderPass(render_pass, nullptr);
        device.destroyPipelineLayout(pipeline_layout, nullptr);
        device.destroyDescriptorSetLayout(desc_layout, nullptr);

        for (i = 0; i < texture_count; i++) {
            device.destroyImageView(textures[i].view, nullptr);
            device.destroyImage(textures[i].image, nullptr);
            device.freeMemory(textures[i].mem, nullptr);
            device.destroySampler(textures[i].sampler, nullptr);
        }

        device.destroyImageView(depth.view, nullptr);
        device.destroyImage(depth.image, nullptr);
        device.freeMemory(depth.mem, nullptr);

        device.destroyBuffer(uniform_data.buf, nullptr);
        device.freeMemory(uniform_data.mem, nullptr);

        for (i = 0; i < swapchainImageCount; i++) {
            device.destroyImageView(buffers[i].view, nullptr);
            device.freeCommandBuffers(cmd_pool, 1, &buffers[i].cmd);
        }

        device.destroyCommandPool(cmd_pool, nullptr);
        if (separate_present_queue) {
            device.destroyCommandPool(present_cmd_pool, nullptr);
        }

        // Second, re-perform the prepare() function, which will re-create the
        // swapchain.
        prepare();
    }

    void set_image_layout(vk::Image image, vk::ImageAspectFlags aspectMask, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                          vk::AccessFlags srcAccessMask, vk::PipelineStageFlags src_stages, vk::PipelineStageFlags dest_stages) {
        assert(cmd);

        auto DstAccessMask = [](vk::ImageLayout const &layout) {
            vk::AccessFlags flags;

            switch (layout) {
                case vk::ImageLayout::eTransferDstOptimal:
                    // Make sure anything that was copying from this image has
                    // completed
                    flags = vk::AccessFlagBits::eTransferWrite;
                    break;
                case vk::ImageLayout::eColorAttachmentOptimal:
                    flags = vk::AccessFlagBits::eColorAttachmentWrite;
                    break;
                case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                    flags = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                    break;
                case vk::ImageLayout::eShaderReadOnlyOptimal:
                    // Make sure any Copy or CPU writes to image are flushed
                    flags = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
                    break;
                case vk::ImageLayout::eTransferSrcOptimal:
                    flags = vk::AccessFlagBits::eTransferRead;
                    break;
                case vk::ImageLayout::ePresentSrcKHR:
                    flags = vk::AccessFlagBits::eMemoryRead;
                    break;
                default:
                    break;
            }

            return flags;
        };

        auto const barrier = vk::ImageMemoryBarrier()
                                 .setSrcAccessMask(srcAccessMask)
                                 .setDstAccessMask(DstAccessMask(newLayout))
                                 .setOldLayout(oldLayout)
                                 .setNewLayout(newLayout)
                                 .setSrcQueueFamilyIndex(0)
                                 .setDstQueueFamilyIndex(0)
                                 .setImage(image)
                                 .setSubresourceRange(vk::ImageSubresourceRange(aspectMask, 0, 1, 0, 1));

        cmd.pipelineBarrier(src_stages, dest_stages, vk::DependencyFlagBits(), 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void update_data_buffer() {
        mat4x4 VP;
        mat4x4_mul(VP, projection_matrix, view_matrix);

        // Rotate around the Y axis
        mat4x4 Model;
        mat4x4_dup(Model, model_matrix);
        mat4x4_rotate(model_matrix, Model, 0.0f, 1.0f, 0.0f, (float)degreesToRadians(spin_angle));

        mat4x4 MVP;
        mat4x4_mul(MVP, VP, model_matrix);

        auto data = device.mapMemory(uniform_data.mem, 0, uniform_data.mem_alloc.allocationSize, vk::MemoryMapFlags());
        VERIFY(data.result == vk::Result::eSuccess);

        memcpy(data.value, (const void *)&MVP[0][0], sizeof(MVP));

        device.unmapMemory(uniform_data.mem);
    }

    bool loadTexture(const char *filename, uint8_t *rgba_data, vk::SubresourceLayout *layout, int32_t *width, int32_t *height) {
        FILE *fPtr = fopen(filename, "rb");
        if (!fPtr) {
            return false;
        }

        char header[256];
        char *cPtr = fgets(header, 256, fPtr);  // P6
        if (cPtr == nullptr || strncmp(header, "P6\n", 3)) {
            fclose(fPtr);
            return false;
        }

        do {
            cPtr = fgets(header, 256, fPtr);
            if (cPtr == nullptr) {
                fclose(fPtr);
                return false;
            }
        } while (!strncmp(header, "#", 1));

        sscanf(header, "%u %u", width, height);
        if (rgba_data == nullptr) {
            fclose(fPtr);
            return true;
        }

        char *result = fgets(header, 256, fPtr);  // Format
        VERIFY(result != nullptr);
        if (cPtr == nullptr || strncmp(header, "255\n", 3)) {
            fclose(fPtr);
            return false;
        }

        for (int y = 0; y < *height; y++) {
            uint8_t *rowPtr = rgba_data;

            for (int x = 0; x < *width; x++) {
                size_t s = fread(rowPtr, 3, 1, fPtr);
                (void)s;
                rowPtr[3] = 255; /* Alpha of 1 */
                rowPtr += 4;
            }

            rgba_data += layout->rowPitch;
        }

        fclose(fPtr);
        return true;
    }

    bool memory_type_from_properties(uint32_t typeBits, vk::MemoryPropertyFlags requirements_mask, uint32_t *typeIndex) {
        // Search memtypes to find first index with those properties
        for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
            if ((typeBits & 1) == 1) {
                // Type is available, does it match user properties?
                if ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
                    *typeIndex = i;
                    return true;
                }
            }
            typeBits >>= 1;
        }

        // No memory types matched, return failure
        return false;
    }

    vk::SurfaceKHR surface;
    bool prepared;
    bool use_staging_buffer;
    bool use_xlib;
    bool separate_present_queue;

//    vk::Instance inst;  // not used
    vk::PhysicalDevice gpu;
    vk::Device device;
    vk::Queue graphics_queue;
    vk::Queue present_queue;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    vk::Semaphore image_acquired_semaphores[FRAME_LAG];
    vk::Semaphore draw_complete_semaphores[FRAME_LAG];
    vk::Semaphore image_ownership_semaphores[FRAME_LAG];
    vk::PhysicalDeviceProperties gpu_props;
    std::unique_ptr<vk::QueueFamilyProperties[]> queue_props;
    vk::PhysicalDeviceMemoryProperties memory_properties;

    uint32_t enabled_extension_count;
    uint32_t enabled_layer_count;
    char const *extension_names[64];
    char const *enabled_layers[64];

    uint32_t width;
    uint32_t height;
    vk::Format format;
    vk::ColorSpaceKHR color_space;

    uint32_t swapchainImageCount;
    vk::SwapchainKHR swapchain;
    std::unique_ptr<SwapchainBuffers[]> buffers;
    vk::PresentModeKHR presentMode;
    vk::Fence fences[FRAME_LAG];
    uint32_t frame_index;

    vk::CommandPool cmd_pool;
    vk::CommandPool present_cmd_pool;

    struct {
        vk::Format format;
        vk::Image image;
        vk::MemoryAllocateInfo mem_alloc;
        vk::DeviceMemory mem;
        vk::ImageView view;
    } depth;

    static int32_t const texture_count = 1;
    texture_object textures[texture_count];
    texture_object staging_texture;

    struct {
        vk::Buffer buf;
        vk::MemoryAllocateInfo mem_alloc;
        vk::DeviceMemory mem;
        vk::DescriptorBufferInfo buffer_info;
    } uniform_data;

    vk::CommandBuffer cmd;  // Buffer for initialization commands
    vk::PipelineLayout pipeline_layout;
    vk::DescriptorSetLayout desc_layout;
    vk::PipelineCache pipelineCache;
    vk::RenderPass render_pass;
    vk::Pipeline pipeline;

    mat4x4 projection_matrix;
    mat4x4 view_matrix;
    mat4x4 model_matrix;

    float spin_angle;
    float spin_increment;
    bool pause;

    vk::ShaderModule vert_shader_module;
    vk::ShaderModule frag_shader_module;

    vk::DescriptorPool desc_pool;
    vk::DescriptorSet desc_set;

    std::unique_ptr<vk::Framebuffer[]> framebuffers;

    bool quit;
    uint32_t curFrame;
    uint32_t frameCount;
    bool validate;
    bool use_break;
    bool suppress_popups;

    uint32_t current_buffer;
    uint32_t queue_family_count;
};

//===================CCube==================
CCube::CCube(){
    demo = new Demo;
}

void CCube::InitDevice(VkPhysicalDevice physical_device){
    demo->set_physical_device(physical_device);
}

void CCube::InitSwapchain(VkSurfaceKHR surface){
    demo->init(0,0);
    demo->init_vk_swapchain(surface);
    demo->prepare();
}

void CCube::Resize(){
    demo->resize();
}

void CCube::Draw(){
    demo->update_data_buffer();
    demo->draw();
}

void CCube::Cleanup(){
    demo->cleanup();
}

CCube::~CCube(){
    delete demo;
}
//==========================================

