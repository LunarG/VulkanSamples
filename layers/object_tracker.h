/* Copyright (c) 2015-2017 The Khronos Group Inc.
 * Copyright (c) 2015-2017 Valve Corporation
 * Copyright (c) 2015-2017 LunarG, Inc.
 * Copyright (C) 2015-2017 Google Inc.
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
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#include <mutex>

#include "vk_enum_string_helper.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_table.h"
#include "vk_layer_utils.h"
#include "vulkan/vk_layer.h"

namespace object_tracker {

// Object Tracker ERROR codes
enum OBJECT_TRACK_ERROR {
    OBJTRACK_NONE,                      // Used for INFO & other non-error messages
    OBJTRACK_UNKNOWN_OBJECT,            // Updating uses of object that's not in global object list
    OBJTRACK_INTERNAL_ERROR,            // Bug with data tracking within the layer
    OBJTRACK_OBJECT_LEAK,               // OBJECT was not correctly freed/destroyed
    OBJTRACK_INVALID_OBJECT,            // Object used that has never been created
    OBJTRACK_DESCRIPTOR_POOL_MISMATCH,  // Descriptor Pools specified incorrectly
    OBJTRACK_COMMAND_POOL_MISMATCH,     // Command Pools specified incorrectly
    OBJTRACK_ALLOCATOR_MISMATCH,        // Created with custom allocator but destroyed without
};

// Object Status -- used to track state of individual objects
typedef VkFlags ObjectStatusFlags;
enum ObjectStatusFlagBits {
    OBJSTATUS_NONE = 0x00000000,                      // No status is set
    OBJSTATUS_FENCE_IS_SUBMITTED = 0x00000001,        // Fence has been submitted
    OBJSTATUS_VIEWPORT_BOUND = 0x00000002,            // Viewport state object has been bound
    OBJSTATUS_RASTER_BOUND = 0x00000004,              // Viewport state object has been bound
    OBJSTATUS_COLOR_BLEND_BOUND = 0x00000008,         // Viewport state object has been bound
    OBJSTATUS_DEPTH_STENCIL_BOUND = 0x00000010,       // Viewport state object has been bound
    OBJSTATUS_GPU_MEM_MAPPED = 0x00000020,            // Memory object is currently mapped
    OBJSTATUS_COMMAND_BUFFER_SECONDARY = 0x00000040,  // Command Buffer is of type SECONDARY
    OBJSTATUS_CUSTOM_ALLOCATOR = 0x00000080,          // Allocated with custom allocator
};

// Object and state information structure
struct OBJTRACK_NODE {
    uint64_t handle;                         // Object handle (new)
    VulkanObjectType object_type;            // Object type identifier
    ObjectStatusFlags status;                // Object state
    uint64_t parent_object;                  // Parent object
};

// Track Queue information
struct OT_QUEUE_INFO {
    uint32_t queue_node_index;
    VkQueue queue;
};

// Layer name string to be logged with validation messages.
const char LayerName[] = "ObjectTracker";

struct instance_extension_enables {
    bool wsi_enabled;
    bool xlib_enabled;
    bool xcb_enabled;
    bool wayland_enabled;
    bool mir_enabled;
    bool android_enabled;
    bool win32_enabled;
    bool display_enabled;
};

struct device_extension_enables{
    bool wsi;
    bool wsi_display_swapchain;
    bool wsi_display_extension;
    bool objtrack_extensions;
    bool khr_descriptor_update_template;
    bool khr_maintenance1;
    bool khr_push_descriptor;
    bool khx_device_group;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    bool khx_external_memory_win32;
#endif // VK_USE_PLATFORM_WIN32_KHR
    bool khx_external_memory_fd;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    bool khx_external_semaphore_win32;
#endif // VK_USE_PLATFORM_WIN32_KHR
    bool khx_external_semaphore_fd;
    bool ext_display_control;
    bool ext_discard_rectangles;
    bool nv_clip_space_w_scaling;
    bool nvx_device_generated_commands;
    bool google_display_timing;
};

typedef std::unordered_map<uint64_t, OBJTRACK_NODE *> object_map_type;

struct layer_data {
    VkInstance instance;
    VkPhysicalDevice physical_device;

    uint64_t num_objects[kVulkanObjectTypeMax + 1];
    uint64_t num_total_objects;

    debug_report_data *report_data;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    // The following are for keeping track of the temporary callbacks that can
    // be used in vkCreateInstance and vkDestroyInstance:
    uint32_t num_tmp_callbacks;
    VkDebugReportCallbackCreateInfoEXT *tmp_dbg_create_infos;
    VkDebugReportCallbackEXT *tmp_callbacks;

    device_extension_enables enables;

    std::vector<VkQueueFamilyProperties> queue_family_properties;

    // Vector of unordered_maps per object type to hold OBJTRACK_NODE info
    std::vector<object_map_type> object_map;
    // Special-case map for swapchain images
    std::unordered_map<uint64_t, OBJTRACK_NODE *> swapchainImageMap;
    // Map of queue information structures, one per queue
    std::unordered_map<VkQueue, OT_QUEUE_INFO *> queue_info_map;

    VkLayerDispatchTable dispatch_table;
    // Default constructor
    layer_data()
        : instance(nullptr),
          physical_device(nullptr),
          num_objects{},
          num_total_objects(0),
          report_data(nullptr),
          num_tmp_callbacks(0),
          tmp_dbg_create_infos(nullptr),
          tmp_callbacks(nullptr),
          object_map{},
          dispatch_table{} {
        object_map.resize(kVulkanObjectTypeMax + 1);
        memset(&enables, 0, sizeof(enables));
    }
};

static std::unordered_map<void *, struct instance_extension_enables> instanceExtMap;
static std::unordered_map<void *, layer_data *> layer_data_map;
static device_table_map ot_device_table_map;
static instance_table_map ot_instance_table_map;
static std::mutex global_lock;
static uint64_t object_track_index = 0;

#include "vk_dispatch_table_helper.h"

}  // namespace object_tracker
