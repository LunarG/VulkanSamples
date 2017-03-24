/*
 * LunarGravity - gravityengine.hpp
 *
 * Copyright (C) 2017 LunarG, Inc.
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
 * Author: Mark Young <marky@lunarg.com>
 */

#pragma once

#include <vector>
#include <string>
#include <vulkan/vulkan.h>

#include "gravitylogger.hpp"
#include "gravityinstanceextif.hpp"
#include "gravitydeviceextif.hpp"
#include "gravitydevicememory.hpp"

struct VulkanString;
class GravityClock;
class GravityWindow;
struct GravitySettingGroup;
struct GravityDeviceMemory;
class GravityDeviceMemoryManager;
class GravityScene;

enum GravitySystemBatteryStatus {
    GRAVITY_BATTERY_STATUS_NONE = 0,
    GRAVITY_BATTERY_STATUS_DISCHARGING_HIGH,      // > 66%
    GRAVITY_BATTERY_STATUS_DISCHARGING_MID,       // > 33%
    GRAVITY_BATTERY_STATUS_DISCHARGING_LOW,       // < 33%
    GRAVITY_BATTERY_STATUS_DISCHARGING_CRITICAL,  // < 5%
    GRAVITY_BATTERY_STATUS_CHARGING,
};

#define MAX_NUM_BACK_BUFFERS 4

struct GravityQueue {
    VkQueue vk_queue;
    uint32_t family_index;
};

struct GravityCmdBuffer {
    bool recording;
    VkCommandBuffer vk_cmd_buf;
    std::vector<GravityCmdBuffer> child_cmd_bufs;
};

struct GraivtySwapchainImage {
    VkImage vk_image;
    VkImageView vk_image_view;
    VkFence vk_fence;
    VkCommandBuffer vk_present_cmd_buf;
};

struct GravitySwapchainSurface {
    VkFormat vk_format;
    VkColorSpaceKHR vk_color_space;
    uint32_t frame_index;
    uint32_t cur_framebuffer;
    VkPresentModeKHR vk_present_mode;
    VkSemaphore vk_image_acquired_semaphores[MAX_NUM_BACK_BUFFERS];
    VkSemaphore vk_draw_complete_semaphores[MAX_NUM_BACK_BUFFERS];
    VkSemaphore vk_image_ownership_semaphores[MAX_NUM_BACK_BUFFERS];
    VkFence vk_fences[MAX_NUM_BACK_BUFFERS];
    VkSwapchainKHR vk_swapchain;
    std::vector<GraivtySwapchainImage> swapchain_images;
};

struct GravityDepthStencilSurface {
    VkFormat vk_format;
    VkImage vk_image;
    GravityDeviceMemory dev_memory;
    VkImageView vk_image_view;
};

class GravityEngine {
   public:
    // Create a protected constructor
    GravityEngine();

    // We don't want any copy constructors
    GravityEngine(const GravityEngine &gfx_engine) = delete;
    GravityEngine &operator=(const GravityEngine &gfx_engine) = delete;

    // Make the destructor public
    virtual ~GravityEngine();

    virtual bool Init(std::vector<std::string> &arguments);
    virtual void AppendUsageString(std::string &usage);
    virtual void PrintUsage(std::string &usage);

    virtual bool ProcessEvents() = 0;
    void Loop();
    virtual bool Update(float comp_time, float game_time);
    virtual bool BeginDrawFrame();
    virtual bool Draw();
    virtual bool EndDrawFrame();

   protected:
    GravitySystemBatteryStatus SystemBatteryStatus(void);
    bool QueryWindowSystem(std::vector<VkExtensionProperties> &ext_props, uint32_t &ext_count, const char **desired_extensions);
    int CompareGpus(VkPhysicalDeviceProperties &gpu_0, VkPhysicalDeviceProperties &gpu_1);

    bool SetupInitialGraphicsDevice();
    bool SetupSwapchain(GravityLogger &logger);
    void CleanupSwapchain();

    bool SetupDepthStencilSurface(GravityLogger &logger);
    void CleanupDepthStencilSurface();

    bool m_print_usage;
    bool m_debug_enabled;
    bool m_quit;
    bool m_paused;
    std::string m_app_name;
    int64_t m_app_version;
    std::string m_engine_name;
    int64_t m_engine_version;
    uint64_t m_cur_frame;

    // Settings
    GravitySettingGroup *m_settings;

    // Graphics device items
    uint32_t m_num_phys_devs;
    uint32_t m_num_backbuffers;

    // Clock
    GravityClock *m_clock;

    // Window
    GravityWindow *m_window;

    // Scenes
    GravityScene *m_cur_scene;
    GravityScene *m_next_scene;

    // Vulkan Instance items
    VkInstance m_vk_inst;
    bool m_validation_enabled;
    GravityInstanceExtIf *m_inst_ext_if;
    VkDebugReportCallbackEXT m_dbg_report_callback;

    // Swapchain surface info
    GravitySwapchainSurface m_swapchain_surface;

    // Depth/stencil info
    GravityDepthStencilSurface m_depth_stencil_surface;

    // Vulkan Physical Device items
    VkPhysicalDevice m_vk_phys_dev;
    bool m_separate_present_queue;
    GravityQueue m_graphics_queue;
    GravityQueue m_present_queue;
    GravityDeviceMemoryManager *m_dev_memory_mgr;

    // Vulkan Logical Device items
    VkDevice m_vk_device;
    GravityDeviceExtIf *m_dev_ext_if;

    // Command buffer information
    VkCommandPool m_vk_graphics_cmd_pool;
    GravityCmdBuffer m_graphics_cmd_buffer;
    VkCommandPool m_vk_present_cmd_pool;
};
