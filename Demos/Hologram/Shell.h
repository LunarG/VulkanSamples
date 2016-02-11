/*
 * Copyright (C) 2016 Google, Inc.
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

#ifndef SHELL_H
#define SHELL_H

#include <queue>
#include <vector>
#include <stdexcept>
#include <vulkan/vulkan.h>
#include <android/log.h>

#include "Game.h"

class Game;

class Shell {
public:
    Shell(const Shell &sh) = delete;
    Shell &operator=(const Shell &sh) = delete;
    virtual ~Shell() {}

    struct BackBuffer {
        uint32_t image_index;

        VkSemaphore acquire_semaphore;
        VkSemaphore render_semaphore;

        // signaled when this struct is ready for reuse
        VkFence present_fence;
    };

    struct Context {
        VkInstance instance;

        VkPhysicalDevice physical_dev;
        uint32_t game_queue_family;
        uint32_t present_queue_family;

        VkDevice dev;
        VkQueue game_queue;
        VkQueue present_queue;

        std::queue<BackBuffer> back_buffers;

        VkSurfaceKHR surface;
        VkSurfaceFormatKHR format;

        VkSwapchainKHR swapchain;
        VkExtent2D extent;

        BackBuffer acquired_back_buffer;
    };
    const Context &context() const { return ctx_; }

    enum LogPriority {
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERR,
    };
    virtual void log(LogPriority priority, const char *msg);

    virtual void run() = 0;
    virtual void quit() = 0;

protected:
    Shell(Game &game);

    void init_vk();
    void cleanup_vk();

    void create_context();
    void destroy_context();

    void resize_swapchain(uint32_t width_hint, uint32_t height_hint);

    void add_game_time(float time);

    void acquire_back_buffer();
    void present_back_buffer();

    Game &game_;
    const Game::Settings &settings_;

    std::vector<const char *> global_extensions_;
    std::vector<const char *> device_extensions_;

private:
    void assert_all_global_extensions() const;
    bool has_all_device_extensions(VkPhysicalDevice phy) const;

    // Current enabled in debug builds, but should be an option
    bool validate_;
    bool check_layers(const uint32_t check_count,
                      const char* check_names[],
                      const uint32_t layer_count,
                      const VkLayerProperties* layers);

    // called by init_vk
    virtual PFN_vkGetInstanceProcAddr load_vk() = 0;
    virtual bool can_present(VkPhysicalDevice phy, uint32_t queue_family) = 0;
    void init_instance();
    virtual void init_debug_callback(VkInstance instance) = 0;
    virtual void cleanup_debug_callback(VkInstance instance) = 0;
    void init_physical_dev();

    // called by create_context
    void create_dev();
    void create_back_buffers();
    void destroy_back_buffers();
    virtual VkSurfaceKHR create_surface(VkInstance instance) = 0;
    void create_swapchain();
    void destroy_swapchain();

    void fake_present();

    Context ctx_;

    const float game_tick_;
    float game_time_;
};

#endif // SHELL_H
