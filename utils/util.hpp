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

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
#pragma comment(linker, "/subsystem:console")
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX              /* Don't let Windows define min() or max() */
#include <windows.h>
#include <vulkan.h>
#include <vk_wsi_swapchain.h>
#include <vk_wsi_device_swapchain.h>
#include <vk_debug_report_lunarg.h>
#define APP_NAME_STR_LEN 80
#else  // _WIN32
#include <xcb/xcb.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include <vulkan/vk_ext_khr_swapchain.h>
#include <vulkan/vk_ext_khr_device_swapchain.h>
#include <vulkan/vk_debug_report_lunarg.h>
#endif // _WIN32

#define SAMPLE_BUFFER_COUNT 2

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                         \
{                                                                        \
    info.fp##entrypoint = (PFN_vk##entrypoint) vkGetInstanceProcAddr(inst, "vk"#entrypoint); \
    if (info.fp##entrypoint == NULL) {                                   \
        std::cout << "vkGetDeviceProcAddr failed to find vk"#entrypoint; \
        exit(-1);                                                        \
    }                                                                    \
}

#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
    info.fp##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);   \
    if (info.fp##entrypoint == NULL) {                                   \
        std::cout << "vkGetDeviceProcAddr failed to find vk"#entrypoint; \
        exit(-1);                                                        \
    }                                                                    \
}

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

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

/*
 * Keep each of our swap chain buffers' image, command buffer and view in one spot
 */
typedef struct _swap_chain_buffers {
    VkImage image;
    VkImageView view;
} swap_chain_buffer;

/*
 * A layer can expose extensions, keep track of those
 * extensions here.
 */
typedef struct {
    VkLayerProperties properties;
    std::vector<VkExtensionProperties> extensions;
} layer_properties;

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
    VkPlatformHandleXcbKHR platform_handle_xcb;
#endif // _WIN32
    bool prepared;
    bool use_staging_buffer;

    std::vector<const char *> instance_layer_names;
    std::vector<const char *> instance_extension_names;
    std::vector<layer_properties> instance_layer_properties;
    std::vector<VkExtensionProperties> instance_extension_properties;
    VkInstance inst;

    std::vector<const char *> device_layer_names;
    std::vector<const char *> device_extension_names;
    std::vector<layer_properties> device_layer_properties;
    std::vector<VkExtensionProperties> device_extension_properties;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    uint32_t graphics_queue_family_index;
    VkPhysicalDeviceProperties gpu_props;
    std::vector<VkQueueFamilyProperties> queue_props;
    VkPhysicalDeviceMemoryProperties memory_properties;

    VkFramebuffer framebuffers[SAMPLE_BUFFER_COUNT];
    int width, height;
    VkFormat format;

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
    std::vector<swap_chain_buffer> buffers;

    VkCmdPool cmd_pool;

    struct {
        VkFormat format;

        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } depth;

    std::vector<struct texture_object> textures;

    struct {
        VkBuffer buf;
        VkDeviceMemory mem;
        VkDescriptorInfo desc;
    } uniform_data;

    struct {
        VkBuffer buf;
        VkDeviceMemory mem;
        VkDescriptorInfo desc;
    } vertex_buffer;
    VkVertexInputBindingDescription vi_binding;
    VkVertexInputAttributeDescription vi_attribs[2];

    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 Model;
    glm::mat4 MVP;

    VkCmdBuffer cmd;  // Buffer for initialization commands
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    VkDynamicViewportState dyn_viewport;
    VkDynamicLineWidthState dyn_line_width;
    VkDynamicBlendState dyn_blend;
    VkDynamicDepthBiasState dyn_depth_bias;
    VkDynamicDepthBoundsState dyn_depth_bounds;
    VkDynamicStencilState dyn_stencil;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;
    VkPipelineShaderStageCreateInfo shaderStages[2];

    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    PFN_vkDbgCreateMsgCallback dbgCreateMsgCallback;
    PFN_vkDbgDestroyMsgCallback dbgDestroyMsgCallback;
    PFN_vkDbgMsgCallback dbgBreakCallback;
    std::vector<VkDbgMsgCallback> msg_callbacks;

    uint32_t current_buffer;
    uint32_t queue_count;
};

VkResult memory_type_from_properties(struct sample_info &info, uint32_t typeBits, VkFlags properties, uint32_t *typeIndex);

void set_image_layout(
        struct sample_info &demo,
        VkImage image,
        VkImageAspectFlags aspectMask,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout);

bool read_ppm(char const*const filename, int *width, int *height, uint64_t rowPitch, char *dataPtr);
void extract_version(uint32_t version, uint32_t &major, uint32_t &minor, uint32_t &patch);
bool GLSLtoSPV(const VkShaderStage shader_type, const char *pshader, std::vector<unsigned int> &spirv);
void init_glslang();
void finalize_glslang();
void wait_seconds(int seconds);
