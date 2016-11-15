/*
 * Copyright (C) 2016 Google, Inc.
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
 */

#ifndef SHELL_H
#define SHELL_H

#include <queue>
#include <vector>
#include <stdexcept>
#include <vulkan/vulkan.h>

#include "Game.h"
#include "Validation.h" //for printing log messages

class Game;

class Shell {
public:
    Shell(const Shell &sh) = delete;
    Shell &operator=(const Shell &sh) = delete;

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
        BackBuffer    acquired_back_buffer;

        VkSurfaceKHR surface;
        VkSurfaceFormatKHR format;

        VkSwapchainKHR swapchain;
        VkExtent2D extent;
    };
    virtual const Context &context() const { return ctx_; }

    enum LogPriority {
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERR,
    };

    virtual void log(LogPriority priority, const char *msg) const{
        switch(priority){
            case LOG_DEBUG : LOGD("%s",msg); break;
            case LOG_INFO  : LOGI("%s",msg); break;
            case LOG_WARN  : LOGW("%s",msg); break;
            case LOG_ERR   : LOGE("%s",msg); break;
            default        : LOG( "%s",msg);
        }
    }

    virtual void quit() = 0;

protected:
    Shell(Game &game):ctx_(){}
    virtual ~Shell(){}
    Context ctx_;
};

#endif // SHELL_H
