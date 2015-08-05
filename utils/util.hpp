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

#include "vulkan/vulkan.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#define APP_NAME_STR_LEN 80
#else  // _WIN32
#include <xcb/xcb.h>
#endif // _WIN32

#include <vulkan/vk_wsi_swapchain.h>
#include <vulkan/vk_wsi_device_swapchain.h>
#include <vulkan/vk_debug_report_lunarg.h>

std::string get_base_data_dir();
std::string get_data_dir( std::string filename );

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

typedef struct _swap_chain_buffers {
    VkImage image;
    VkCmdBuffer cmd;
    VkAttachmentView view;
} swap_chain_buffers;

/*
 * Structure for tracking information used / created / modified
 * by utility functions.
 */
struct sample_info {
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
    VkPlatformHandleXcbWSI platform_handle_xcb;
#endif // _WIN32
    bool prepared;
    bool use_staging_buffer;
    bool use_glsl;

    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    uint32_t graphics_queue_family_index;
    VkPhysicalDeviceProperties gpu_props;
    std::vector<VkPhysicalDeviceQueueProperties> queue_props;
    std::vector<VkPhysicalDeviceMemoryProperties> memory_properties;

    VkFramebuffer framebuffer;
    int width, height;
    VkFormat format;

    PFN_vkGetPhysicalDeviceSurfaceSupportWSI fpGetPhysicalDeviceSurfaceSupportWSI;
    PFN_vkGetSurfaceInfoWSI fpGetSurfaceInfoWSI;
    PFN_vkCreateSwapChainWSI fpCreateSwapChainWSI;
    PFN_vkDestroySwapChainWSI fpDestroySwapChainWSI;
    PFN_vkGetSwapChainInfoWSI fpGetSwapChainInfoWSI;
    PFN_vkAcquireNextImageWSI fpAcquireNextImageWSI;
    PFN_vkQueuePresentWSI fpQueuePresentWSI;
    VkSurfaceDescriptionWindowWSI surface_description;
    VkSwapChainWSI swap_chain;
    std::vector<swap_chain_buffers> buffers;

    VkCmdPool cmd_pool;

    struct {
        VkFormat format;

        VkImage image;
        VkDeviceMemory mem;
        VkAttachmentView view;
    } depth;

    std::vector<struct texture_object> textures;

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

    VkDynamicViewportState viewport;
    VkDynamicRasterState raster;
    VkDynamicColorBlendState color_blend;
    VkDynamicDepthStencilState depth_stencil;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    std::vector<VkFramebuffer> framebuffers;

    PFN_vkDbgCreateMsgCallback dbgCreateMsgCallback;
    PFN_vkDbgDestroyMsgCallback dbgDestroyMsgCallback;
    PFN_vkDbgMsgCallback dbgBreakCallback;
    std::vector<VkDbgMsgCallback> msg_callbacks;
};
